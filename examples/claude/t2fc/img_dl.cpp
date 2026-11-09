// img_dl: decodes pictures for t2fc and draws them into the tui screen as half-block
// cells, two square pixels per cell, the way tvdemo's Mandelbrot is drawn.
//
// Formats: PNG, JPEG (turned upright by its EXIF orientation), GIF (animated too),
// BMP, TGA, PSD, HDR and PNM through stb_image (stb_image.h, public domain), and
// WebP, animated too, through libwebp when build_so.sh finds it.
//
// Pictures live in numbered slots and are decoded on a thread of their own, so the
// desktop never waits for a big file: img_open starts it, img_state says when it is
// ready. A newer img_open on the same slot wins over one still being decoded.
#include <algorithm>
#include <cmath>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <strings.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_FAILURE_USERMSG
#include "stb_image.h"

#ifdef HAVE_WEBP
#include <webp/decode.h>
#include <webp/demux.h>
#endif

#include "../../../include/ScincVM.h"
#include "../../../include/ScincBinder.h"

#define IMG_SLOTS 32
#define TV_RGB 0x40000000   // as in tv.h: two 15-bit colours, (fg<<15)|bg

struct Frame
{
	std::vector<uint8_t> rgba;
	int delay=0;      // ms
};

struct Picture
{
	int w=0;
	int h=0;
	std::string format;
	std::vector<Frame> frames;
};

struct Slot
{
	std::mutex m;
	std::string want;          // the file asked for
	int wantGen=0;
	int doneGen=0;
	bool busy=false;           // a thread is decoding for this slot
	int state=0;               // 0 nothing, 1 decoding, 2 ready, -1 failed
	std::shared_ptr<Picture> pic;
	std::string err;
	// the last picture drawn, kept while nothing changes
	std::vector<uint32_t> cache;
	int cacheKeyI[6]={-1,0,0,0,0,0};
	double cacheKeyF[4]={0,0,0,0};
	int version=0;             // bumped by every change to pic
};

static Slot slots[IMG_SLOTS];

static bool validSlot(int s)
{
	return (s>=0)&&(s<IMG_SLOTS);
}

static void copyOut(const std::string& s, char* buf, int max)
{
	if(max<=0)return;
	int n=(int)s.size();
	if(n>max-1)n=max-1;
	memcpy(buf,s.data(),n);
	buf[n]=0;
}

static bool readFile(const std::string& path, std::vector<uint8_t>& data, std::string& err)
{
	FILE* f=fopen(path.c_str(),"rb");
	if(!f)
	{
		err=strerror(errno);
		return false;
	}
	fseek(f,0,SEEK_END);
	long n=ftell(f);
	fseek(f,0,SEEK_SET);
	if((n<0)||(n>(1L<<30)))
	{
		fclose(f);
		err="The file is too big.";
		return false;
	}
	data.resize(n);
	size_t got=n?fread(data.data(),1,n,f):0;
	fclose(f);
	if((long)got!=n)
	{
		err="The file could not be read.";
		return false;
	}
	return true;
}

// ---------------------------------------------------------------- formats

static bool hasExt(const char* path, const char* ext)
{
	size_t n=strlen(path);
	size_t e=strlen(ext);
	return (n>=e)&&(strcasecmp(path+n-e,ext)==0);
}

static const char* sniff(const uint8_t* d, size_t n, const char* path)
{
	if((n>=8)&&!memcmp(d,"\x89PNG\r\n\x1a\n",8))return "PNG";
	if((n>=3)&&(d[0]==0xFF)&&(d[1]==0xD8)&&(d[2]==0xFF))return "JPEG";
	if((n>=4)&&!memcmp(d,"GIF8",4))return "GIF";
	if((n>=2)&&!memcmp(d,"BM",2))return "BMP";
	if((n>=12)&&!memcmp(d,"RIFF",4)&&!memcmp(d+8,"WEBP",4))return "WebP";
	if((n>=4)&&!memcmp(d,"8BPS",4))return "PSD";
	if((n>=6)&&(!memcmp(d,"#?RADIANCE",n<10?n:10)||!memcmp(d,"#?RGBE",6)))return "HDR";
	if((n>=2)&&(d[0]=='P')&&((d[1]=='5')||(d[1]=='6')))return "PNM";
	if(hasExt(path,".tga"))return "TGA";
	return nullptr;
}

// The EXIF orientation of a JPEG, 1 (upright) to 8.
static int jpegOrientation(const uint8_t* d, size_t n)
{
	size_t i=2;
	while(i+4<=n)
	{
		if(d[i]!=0xFF)return 1;
		int marker=d[i+1];
		size_t len=(d[i+2]<<8)|d[i+3];
		if((marker==0xDA)||(marker==0xD9))return 1;   // the picture data starts
		if((marker==0xE1)&&(i+4+len-2<=n)&&(len>=16)&&!memcmp(d+i+4,"Exif\0\0",6))
		{
			const uint8_t* t=d+i+10;
			size_t tn=len-8;
			bool le=(t[0]=='I');
			auto u16=[&](size_t o)->int{
				if(o+2>tn)return 0;
				return le?(t[o]|(t[o+1]<<8)):((t[o]<<8)|t[o+1]);
			};
			auto u32=[&](size_t o)->size_t{
				if(o+4>tn)return 0;
				if(le)return t[o]|(t[o+1]<<8)|(t[o+2]<<16)|((size_t)t[o+3]<<24);
				return ((size_t)t[o]<<24)|(t[o+1]<<16)|(t[o+2]<<8)|t[o+3];
			};
			size_t ifd=u32(4);
			int count=u16(ifd);
			for(int k=0;k<count;k++)
			{
				size_t e=ifd+2+k*12;
				if(e+12>tn)break;
				if(u16(e)==0x112)
				{
					int o=u16(e+8);
					if((o>=1)&&(o<=8))return o;
					return 1;
				}
			}
			return 1;
		}
		i+=2+len;
	}
	return 1;
}

// Turns a picture as an EXIF orientation says (or 6 to turn it a quarter clockwise).
static void orient(Picture& p, int o)
{
	if(o<=1)return;
	int W=p.w;
	int H=p.h;
	bool swap=(o>=5);
	int nw=swap?H:W;
	int nh=swap?W:H;
	for(Frame& f : p.frames)
	{
		std::vector<uint8_t> out((size_t)nw*nh*4);
		const uint32_t* src=(const uint32_t*)f.rgba.data();
		uint32_t* dst=(uint32_t*)out.data();
		for(int y=0;y<nh;y++)
			for(int x=0;x<nw;x++)
			{
				int sx=x;
				int sy=y;
				if(o==2){sx=W-1-x;sy=y;}
				else if(o==3){sx=W-1-x;sy=H-1-y;}
				else if(o==4){sx=x;sy=H-1-y;}
				else if(o==5){sx=y;sy=x;}
				else if(o==6){sx=y;sy=H-1-x;}
				else if(o==7){sx=W-1-y;sy=H-1-x;}
				else if(o==8){sx=W-1-y;sy=x;}
				dst[(size_t)y*nw+x]=src[(size_t)sy*W+sx];
			}
		f.rgba.swap(out);
	}
	p.w=nw;
	p.h=nh;
}

#ifdef HAVE_WEBP
static bool decodeWebp(const std::vector<uint8_t>& data, Picture& p, std::string& err)
{
	WebPData wd;
	wd.bytes=data.data();
	wd.size=data.size();
	WebPAnimDecoderOptions opt;
	WebPAnimDecoderOptionsInit(&opt);
	opt.color_mode=MODE_RGBA;
	opt.use_threads=1;
	WebPAnimDecoder* dec=WebPAnimDecoderNew(&wd,&opt);
	if(!dec)
	{
		err="This WebP file could not be decoded.";
		return false;
	}
	WebPAnimInfo info;
	WebPAnimDecoderGetInfo(dec,&info);
	p.w=info.canvas_width;
	p.h=info.canvas_height;
	int prev=0;
	while(WebPAnimDecoderHasMoreFrames(dec))
	{
		uint8_t* buf=nullptr;
		int ts=0;
		if(!WebPAnimDecoderGetNext(dec,&buf,&ts))break;
		Frame f;
		f.rgba.assign(buf,buf+(size_t)p.w*p.h*4);
		f.delay=ts-prev;
		prev=ts;
		p.frames.push_back(std::move(f));
	}
	WebPAnimDecoderDelete(dec);
	if(p.frames.empty())
	{
		err="This WebP file has no pictures.";
		return false;
	}
	return true;
}
#endif

static bool decode(const std::string& path, Picture& p, std::string& err)
{
	std::vector<uint8_t> data;
	if(!readFile(path,data,err))return false;
	const char* fmt=sniff(data.data(),data.size(),path.c_str());
	if(!fmt)
	{
		err="This is not a picture t2fc knows.";
		return false;
	}
	p.format=fmt;
	if(!strcmp(fmt,"WebP"))
	{
#ifdef HAVE_WEBP
		return decodeWebp(data,p,err);
#else
		err="WebP needs libwebp: build img_dl.so with libwebp-dev installed.";
		return false;
#endif
	}
	if(data.size()>INT32_MAX)
	{
		err="The file is too big.";
		return false;
	}
	int comp=0;
	if(!strcmp(fmt,"GIF"))
	{
		int* delays=nullptr;
		int frames=0;
		stbi_uc* px=stbi_load_gif_from_memory(data.data(),(int)data.size(),&delays,&p.w,&p.h,&frames,&comp,4);
		if(!px)
		{
			err=stbi_failure_reason();
			return false;
		}
		size_t one=(size_t)p.w*p.h*4;
		for(int k=0;k<frames;k++)
		{
			Frame f;
			f.rgba.assign(px+one*k,px+one*(k+1));
			f.delay=delays?delays[k]:100;
			p.frames.push_back(std::move(f));
		}
		stbi_image_free(px);
		if(delays)STBI_FREE(delays);
		return !p.frames.empty();
	}
	stbi_uc* px=stbi_load_from_memory(data.data(),(int)data.size(),&p.w,&p.h,&comp,4);
	if(!px)
	{
		err=stbi_failure_reason();
		return false;
	}
	Frame f;
	f.rgba.assign(px,px+(size_t)p.w*p.h*4);
	stbi_image_free(px);
	p.frames.push_back(std::move(f));
	if(!strcmp(fmt,"JPEG"))orient(p,jpegOrientation(data.data(),data.size()));
	return true;
}

static void worker(int s)
{
	Slot& sl=slots[s];
	while(true)
	{
		std::string path;
		int gen;
		{
			std::lock_guard<std::mutex> lk(sl.m);
			if(sl.doneGen==sl.wantGen)
			{
				sl.busy=false;
				return;
			}
			gen=sl.wantGen;
			path=sl.want;
		}
		auto pic=std::make_shared<Picture>();
		std::string err;
		bool ok=false;
		if(!path.empty())ok=decode(path,*pic,err);
		std::lock_guard<std::mutex> lk(sl.m);
		if((gen==sl.wantGen)&&!path.empty())
		{
			if(ok)
			{
				sl.pic=pic;
				sl.state=2;
				sl.err.clear();
			}
			else
			{
				sl.pic.reset();
				sl.state=-1;
				sl.err=err;
			}
			sl.version++;
		}
		sl.doneGen=gen;
	}
}

// ---------------------------------------------------------------- the API

// 1 if the file looks like a picture img_open can decode.
int img_is(char* path)
{
	FILE* f=fopen(path,"rb");
	if(!f)return 0;
	uint8_t head[16];
	size_t n=fread(head,1,sizeof head,f);
	fclose(f);
	const char* fmt=sniff(head,n,path);
	if(!fmt)return 0;
#ifndef HAVE_WEBP
	if(!strcmp(fmt,"WebP"))return 0;
#endif
	return 1;
}

// Starts decoding a file into slot s; img_state tells when it is done.
int img_open(int s, char* path)
{
	if(!validSlot(s))return 0;
	Slot& sl=slots[s];
	std::lock_guard<std::mutex> lk(sl.m);
	sl.want=path;
	sl.wantGen++;
	sl.state=1;
	sl.pic.reset();
	sl.version++;
	if(!sl.busy)
	{
		sl.busy=true;
		std::thread(worker,s).detach();
	}
	return 1;
}

// Forgets slot s's picture.
void img_free(int s)
{
	if(!validSlot(s))return;
	Slot& sl=slots[s];
	std::lock_guard<std::mutex> lk(sl.m);
	sl.want.clear();
	sl.wantGen++;
	sl.state=0;
	sl.pic.reset();
	sl.cache.clear();
	sl.cache.shrink_to_fit();
	sl.version++;
	if(!sl.busy)sl.doneGen=sl.wantGen;
}

// 0 nothing, 1 decoding, 2 ready, -1 failed (img_error says why).
int img_state(int s)
{
	if(!validSlot(s))return 0;
	std::lock_guard<std::mutex> lk(slots[s].m);
	return slots[s].state;
}

void img_error(int s, char* buf, int max)
{
	if(!validSlot(s))return;
	std::lock_guard<std::mutex> lk(slots[s].m);
	copyOut(slots[s].err,buf,max);
}

void img_format(int s, char* buf, int max)
{
	if(!validSlot(s))return;
	std::lock_guard<std::mutex> lk(slots[s].m);
	copyOut(slots[s].pic?slots[s].pic->format:std::string(),buf,max);
}

// k: 0 width, 1 height, 2 frames, 3 1 if any pixel is not opaque.
int img_info(int s, int k)
{
	if(!validSlot(s))return 0;
	std::shared_ptr<Picture> p;
	{
		std::lock_guard<std::mutex> lk(slots[s].m);
		p=slots[s].pic;
	}
	if(!p)return 0;
	if(k==0)return p->w;
	if(k==1)return p->h;
	if(k==2)return (int)p->frames.size();
	if(k==3)
	{
		for(const Frame& f : p->frames)
			for(size_t i=3;i<f.rgba.size();i+=4)
				if(f.rgba[i]!=255)return 1;
		return 0;
	}
	return 0;
}

// How long frame f stays, in ms.
int img_delay(int s, int f)
{
	if(!validSlot(s))return 100;
	std::shared_ptr<Picture> p;
	{
		std::lock_guard<std::mutex> lk(slots[s].m);
		p=slots[s].pic;
	}
	if(!p||(f<0)||(f>=(int)p->frames.size()))return 100;
	int d=p->frames[f].delay;
	if(d<20)d=100;    // as browsers do with 0 and 10 ms GIF frames
	return d;
}

// Turns the picture a quarter clockwise.
void img_rotate(int s)
{
	if(!validSlot(s))return;
	Slot& sl=slots[s];
	std::lock_guard<std::mutex> lk(sl.m);
	if(!sl.pic)return;
	auto p=std::make_shared<Picture>(*sl.pic);
	orient(*p,6);
	sl.pic=p;
	sl.version++;
}

static inline int to15(uint32_t c)
{
	return (((c>>19)&31)<<10)|(((c>>11)&31)<<5)|((c>>3)&31);
}

// Works out the screen pixels: pw*ph, where screen pixel px,py covers the source
// from cx+(px-pw/2)*sx on, sx wide and sy high. Shrinking averages every source
// pixel a screen pixel covers; enlarging repeats them. Clear pixels show a
// checkerboard, and around the picture is bg.
static void render(const Picture& p, int frame, double cx, double cy, double sx, double sy,
	int pw, int ph, uint32_t bg, std::vector<uint32_t>& out)
{
	out.assign((size_t)pw*ph,bg);
	const uint8_t* src=p.frames[frame].rgba.data();
	double ox=cx-pw*0.5*sx;
	double oy=cy-ph*0.5*sy;
	// the source columns of each screen column
	std::vector<int> x0(pw);
	std::vector<int> x1(pw);
	for(int px=0;px<pw;px++)
	{
		double a=ox+px*sx;
		double b=a+sx;
		int i0=(int)std::floor(a+0.5);
		int i1=(int)std::floor(b+0.5);
		if(i1<=i0)
		{
			i0=(int)std::floor(a+sx*0.5);
			i1=i0+1;
		}
		x0[px]=i0;
		x1[px]=i1;
	}
	for(int py=0;py<ph;py++)
	{
		double a=oy+py*sy;
		double b=a+sy;
		int j0=(int)std::floor(a+0.5);
		int j1=(int)std::floor(b+0.5);
		if(j1<=j0)
		{
			j0=(int)std::floor(a+sy*0.5);
			j1=j0+1;
		}
		int cj0=std::max(j0,0);
		int cj1=std::min(j1,p.h);
		if(cj1<=cj0)continue;
		for(int px=0;px<pw;px++)
		{
			int i0=x0[px];
			int i1=x1[px];
			int ci0=std::max(i0,0);
			int ci1=std::min(i1,p.w);
			if(ci1<=ci0)continue;
			uint64_t r=0;
			uint64_t g=0;
			uint64_t bl=0;
			uint64_t al=0;
			for(int j=cj0;j<cj1;j++)
			{
				const uint8_t* row=src+((size_t)j*p.w+ci0)*4;
				for(int i=ci0;i<ci1;i++)
				{
					unsigned aa=row[3];
					r+=row[0]*aa;
					g+=row[1]*aa;
					bl+=row[2]*aa;
					al+=aa;
					row+=4;
				}
			}
			uint64_t inside=(uint64_t)(ci1-ci0)*(cj1-cj0);
			uint64_t all=(uint64_t)(i1-i0)*(j1-j0);
			// what shows through: the checkerboard, as much as the pixels let through
			uint32_t under=(((px>>2)+(py>>2))&1)?0x9A9A9A:0x666666;
			double cover=(double)al/(255.0*inside);          // opacity of the picture part
			double frac=(double)inside/(double)all;          // how much of it is picture
			double cr=0;
			double cg=0;
			double cb=0;
			if(al>0)
			{
				cr=(double)r/al;
				cg=(double)g/al;
				cb=(double)bl/al;
			}
			auto ch=[&](uint32_t c,int sh){return (double)((c>>sh)&255);};
			double fr=cr*cover+ch(under,16)*(1-cover);
			double fg=cg*cover+ch(under,8)*(1-cover);
			double fb=cb*cover+ch(under,0)*(1-cover);
			fr=fr*frac+ch(bg,16)*(1-frac);
			fg=fg*frac+ch(bg,8)*(1-frac);
			fb=fb*frac+ch(bg,0)*(1-frac);
			int R=(int)(fr+0.5);
			int G=(int)(fg+0.5);
			int B=(int)(fb+0.5);
			out[(size_t)py*pw+px]=(R<<16)|(G<<8)|B;
		}
	}
}

// Draws frame f of slot s into cells x0,y0 .. x0+cw-1,y0+ch-1 of the screen arrays
// (stride cells a row), inside the clip rectangle. cx,cy is the source point at the
// middle and scale the source pixels per screen pixel down, a cell being two screen
// pixels one above the other; aspect is a screen pixel's width over its height.
// uphalf is the upper half block character. Returns 0 when there is no picture.
int img_draw(int s, int f, double cx, double cy, double scale, double aspect, int x0, int y0, int cw, int ch,
	int* cells, int* attrs, int stride, int clx0, int cly0, int clx1, int cly1, int uphalf, int bg)
{
	if(!validSlot(s)||(cw<=0)||(ch<=0)||!(scale>0))return 0;
	Slot& sl=slots[s];
	std::shared_ptr<Picture> p;
	int version;
	{
		std::lock_guard<std::mutex> lk(sl.m);
		p=sl.pic;
		version=sl.version;
	}
	if(!p||p->frames.empty())return 0;
	if((f<0)||(f>=(int)p->frames.size()))f=0;
	int pw=cw;
	int ph=ch*2;
	bool same=(sl.cacheKeyI[0]==version)&&(sl.cacheKeyI[1]==f)&&(sl.cacheKeyI[2]==pw)&&
		(sl.cacheKeyI[3]==ph)&&(sl.cacheKeyI[4]==bg)&&(sl.cacheKeyF[0]==cx)&&
		(sl.cacheKeyF[1]==cy)&&(sl.cacheKeyF[2]==scale)&&(sl.cacheKeyF[3]==aspect)&&
		(sl.cache.size()==(size_t)pw*ph);
	if(!same)
	{
		if(!(aspect>0))aspect=1;
		render(*p,f,cx,cy,scale*aspect,scale,pw,ph,(uint32_t)bg,sl.cache);
		sl.cacheKeyF[3]=aspect;
		sl.cacheKeyI[0]=version;
		sl.cacheKeyI[1]=f;
		sl.cacheKeyI[2]=pw;
		sl.cacheKeyI[3]=ph;
		sl.cacheKeyI[4]=bg;
		sl.cacheKeyF[0]=cx;
		sl.cacheKeyF[1]=cy;
		sl.cacheKeyF[2]=scale;
	}
	const std::vector<uint32_t>& pix=sl.cache;
	for(int r=0;r<ch;r++)
	{
		int y=y0+r;
		if((y<cly0)||(y>=cly1))continue;
		for(int c=0;c<cw;c++)
		{
			int x=x0+c;
			if((x<clx0)||(x>=clx1))continue;
			int t=to15(pix[(size_t)(2*r)*pw+c]);
			int b=to15(pix[(size_t)(2*r+1)*pw+c]);
			int i=y*stride+x;
			if(t==b)
			{
				cells[i]=' ';
				attrs[i]=TV_RGB|(b<<15)|b;
			}
			else
			{
				cells[i]=uphalf;
				attrs[i]=TV_RGB|(t<<15)|b;
			}
		}
	}
	return 1;
}

extern "C" int img_is_wrapper(ScincVM* s, void*){ScincBoundCall(s,img_is);return 0;}
extern "C" int img_open_wrapper(ScincVM* s, void*){ScincBoundCall(s,img_open);return 0;}
extern "C" int img_free_wrapper(ScincVM* s, void*){ScincBoundCall(s,img_free);return 0;}
extern "C" int img_state_wrapper(ScincVM* s, void*){ScincBoundCall(s,img_state);return 0;}
extern "C" int img_error_wrapper(ScincVM* s, void*){ScincBoundCall(s,img_error);return 0;}
extern "C" int img_format_wrapper(ScincVM* s, void*){ScincBoundCall(s,img_format);return 0;}
extern "C" int img_info_wrapper(ScincVM* s, void*){ScincBoundCall(s,img_info);return 0;}
extern "C" int img_delay_wrapper(ScincVM* s, void*){ScincBoundCall(s,img_delay);return 0;}
extern "C" int img_rotate_wrapper(ScincVM* s, void*){ScincBoundCall(s,img_rotate);return 0;}
extern "C" int img_draw_wrapper(ScincVM* s, void*){ScincBoundCall(s,img_draw);return 0;}
