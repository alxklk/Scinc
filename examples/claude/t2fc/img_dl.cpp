// img_dl: decodes pictures for t2fc and draws them into the tui screen as half-block
// cells, two square pixels per cell, the way tvdemo's Mandelbrot is drawn.
//
// Formats: PNG, JPEG (turned upright by its EXIF orientation), GIF (animated too),
// BMP, TGA, PSD, HDR and PNM through stb_image (stb_image.h, public domain), and
// WebP, animated too, through libwebp. libwebp is opened only when a WebP file is,
// so img_dl.so needs nothing beyond the C and C++ libraries and still loads on a
// machine without libwebp (it then just cannot show WebP); build_so.sh needs the
// libwebp headers for it, or leaves WebP out. SVG and SVGZ are drawn by librsvg,
// opened the same way (no headers needed).
//
// Videos (MP4, MKV, WebM, AVI, MOV, ...) play through ffmpeg when it is on the PATH,
// or else GStreamer's gst-launch-1.0: it decodes them scaled down to raw RGBA frames
// on a pipe, a thread here reads them at the video's pace, and each frame becomes
// the slot's picture in turn. With neither they are not pictures.
//
// Pictures live in numbered slots and are decoded on a thread of their own, so the
// desktop never waits for a big file: img_open starts it, img_state says when it is
// ready. A newer img_open on the same slot wins over one still being decoded.
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_FAILURE_USERMSG
#include "stb_image.h"

#ifdef HAVE_WEBP
#include <dlfcn.h>
#include <webp/decode.h>
#include <webp/demux.h>

// libwebpdemux's functions, found at run time
static int (*pOptionsInit)(WebPAnimDecoderOptions*,int)=nullptr;
static WebPAnimDecoder* (*pNew)(const WebPData*,const WebPAnimDecoderOptions*,int)=nullptr;
static int (*pGetInfo)(const WebPAnimDecoder*,WebPAnimInfo*)=nullptr;
static int (*pHasMore)(const WebPAnimDecoder*)=nullptr;
static int (*pGetNext)(WebPAnimDecoder*,uint8_t**,int*)=nullptr;
static void (*pDelete)(WebPAnimDecoder*)=nullptr;
static std::mutex webpLock;
static int webpState=0;     // 0 not tried, 1 there, -1 missing

static bool webpLoad()
{
	std::lock_guard<std::mutex> lk(webpLock);
	if(webpState)return webpState>0;
	webpState=-1;
	void* h=dlopen("libwebpdemux.so.2",RTLD_NOW|RTLD_LOCAL);
	if(!h)h=dlopen("libwebpdemux.so",RTLD_NOW|RTLD_LOCAL);
	if(!h)return false;
	pOptionsInit=(int(*)(WebPAnimDecoderOptions*,int))dlsym(h,"WebPAnimDecoderOptionsInitInternal");
	pNew=(WebPAnimDecoder*(*)(const WebPData*,const WebPAnimDecoderOptions*,int))dlsym(h,"WebPAnimDecoderNewInternal");
	pGetInfo=(int(*)(const WebPAnimDecoder*,WebPAnimInfo*))dlsym(h,"WebPAnimDecoderGetInfo");
	pHasMore=(int(*)(const WebPAnimDecoder*))dlsym(h,"WebPAnimDecoderHasMoreFrames");
	pGetNext=(int(*)(WebPAnimDecoder*,uint8_t**,int*))dlsym(h,"WebPAnimDecoderGetNext");
	pDelete=(void(*)(WebPAnimDecoder*))dlsym(h,"WebPAnimDecoderDelete");
	if(!pOptionsInit||!pNew||!pGetInfo||!pHasMore||!pGetNext||!pDelete)return false;
	webpState=1;
	return true;
}
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
	std::vector<uint32_t> pcache;  // the same for img_pixels
	double pcacheKey[13]={-1,0,0,0,0,0,0,0,0,0,0,0,0};
	int cacheKeyI[6]={-1,0,0,0,0,0};
	double cacheKeyF[4]={0,0,0,0};
	int version=0;             // bumped by every change to pic
	std::shared_ptr<struct Video> vid;   // the video playing here
	int vMaxW=480;             // what img_video_opts set for the next video
	int vMaxH=360;
	int vFps=12;
	bool vLoop=true;
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

// ---------------------------------------------------------------- SVG
// SVG (and gzipped SVGZ) through librsvg, opened at run time like libwebp: no
// headers are needed, the few functions are declared here, and cairo and GObject
// are found through librsvg's own dependencies.
#include <dlfcn.h>

struct SvgError { uint32_t domain; int code; char* message; };
struct SvgDims { int width; int height; double em; double ex; };
struct SvgRect { double x; double y; double width; double height; };

static void* (*svgNewFromFile)(const char*,SvgError**)=nullptr;
static void (*svgGetDimensions)(void*,SvgDims*)=nullptr;
static int (*svgSizeInPixels)(void*,double*,double*)=nullptr;           // 2.52+
static int (*svgRenderDocument)(void*,void*,const SvgRect*,SvgError**)=nullptr;  // 2.46+
static int (*svgRenderCairo)(void*,void*)=nullptr;
static void (*svgUnref)(void*)=nullptr;
static void (*svgErrorFree)(SvgError*)=nullptr;
static void* (*crSurfaceCreate)(int,int,int)=nullptr;
static void* (*crCreate)(void*)=nullptr;
static void (*crScale)(void*,double,double)=nullptr;
static void (*crDestroy)(void*)=nullptr;
static void (*crSurfaceFlush)(void*)=nullptr;
static void (*crSurfaceDestroy)(void*)=nullptr;
static int (*crSurfaceStatus)(void*)=nullptr;
static unsigned char* (*crSurfaceData)(void*)=nullptr;
static int (*crSurfaceStride)(void*)=nullptr;
static std::mutex svgLock;
static int svgState=0;     // 0 not tried, 1 there, -1 missing

static bool svgLoad()
{
	std::lock_guard<std::mutex> lk(svgLock);
	if(svgState)return svgState>0;
	svgState=-1;
	void* h=dlopen("librsvg-2.so.2",RTLD_NOW|RTLD_LOCAL);
	if(!h)h=dlopen("librsvg-2.so",RTLD_NOW|RTLD_LOCAL);
	if(!h)return false;
	#define SVG_SYM(v,name) *(void**)&v=dlsym(h,name)
	SVG_SYM(svgNewFromFile,"rsvg_handle_new_from_file");
	SVG_SYM(svgGetDimensions,"rsvg_handle_get_dimensions");
	SVG_SYM(svgSizeInPixels,"rsvg_handle_get_intrinsic_size_in_pixels");
	SVG_SYM(svgRenderDocument,"rsvg_handle_render_document");
	SVG_SYM(svgRenderCairo,"rsvg_handle_render_cairo");
	SVG_SYM(svgUnref,"g_object_unref");
	SVG_SYM(svgErrorFree,"g_error_free");
	SVG_SYM(crSurfaceCreate,"cairo_image_surface_create");
	SVG_SYM(crCreate,"cairo_create");
	SVG_SYM(crScale,"cairo_scale");
	SVG_SYM(crDestroy,"cairo_destroy");
	SVG_SYM(crSurfaceFlush,"cairo_surface_flush");
	SVG_SYM(crSurfaceDestroy,"cairo_surface_destroy");
	SVG_SYM(crSurfaceStatus,"cairo_surface_status");
	SVG_SYM(crSurfaceData,"cairo_image_surface_get_data");
	SVG_SYM(crSurfaceStride,"cairo_image_surface_get_stride");
	#undef SVG_SYM
	if(!svgNewFromFile||!svgGetDimensions||!(svgRenderDocument||svgRenderCairo)||!svgUnref||
		!svgErrorFree||!crSurfaceCreate||!crCreate||!crScale||!crDestroy||!crSurfaceFlush||
		!crSurfaceDestroy||!crSurfaceStatus||!crSurfaceData||!crSurfaceStride)return false;
	svgState=1;
	return true;
}

// The pixels an SVG is drawn into: its own size, but small drawings (icons) are
// drawn bigger, up to SVG_MIN on the longer side, since they would otherwise only
// be shown with their pixels repeated; and none longer than SVG_MAX.
#define SVG_MIN 1024
#define SVG_MAX 8192

static bool decodeSvg(const std::string& path, Picture& p, std::string& err)
{
	if(!svgLoad())
	{
		err="SVG needs librsvg (librsvg-2.so.2), which this machine does not have.";
		return false;
	}
	SvgError* e=nullptr;
	void* h=svgNewFromFile(path.c_str(),&e);
	if(!h)
	{
		err=(e&&e->message)?e->message:"This SVG file could not be read.";
		if(e)svgErrorFree(e);
		return false;
	}
	double w=0,hh=0;
	if(!svgSizeInPixels||!svgSizeInPixels(h,&w,&hh)||(w<=0)||(hh<=0))
	{
		SvgDims d={0,0,0,0};
		svgGetDimensions(h,&d);
		w=d.width;
		hh=d.height;
	}
	if((w<=0)||(hh<=0))
	{
		w=SVG_MIN;
		hh=SVG_MIN;
	}
	double k=1;
	double longer=std::max(w,hh);
	if(longer<SVG_MIN)k=SVG_MIN/longer;
	if(longer*k>SVG_MAX)k=SVG_MAX/longer;
	p.w=std::max(1,(int)std::lround(w*k));
	p.h=std::max(1,(int)std::lround(hh*k));
	void* surf=crSurfaceCreate(0,p.w,p.h);   // CAIRO_FORMAT_ARGB32
	if(crSurfaceStatus(surf))
	{
		crSurfaceDestroy(surf);
		svgUnref(h);
		err="Not enough memory for this SVG.";
		return false;
	}
	void* cr=crCreate(surf);
	bool ok;
	if(svgRenderDocument)
	{
		SvgRect vp={0,0,(double)p.w,(double)p.h};
		ok=svgRenderDocument(h,cr,&vp,&e)!=0;
	}
	else
	{
		crScale(cr,p.w/w,p.h/hh);
		ok=svgRenderCairo(h,cr)!=0;
	}
	crDestroy(cr);
	crSurfaceFlush(surf);
	if(!ok)
	{
		err=(e&&e->message)?e->message:"This SVG file could not be drawn.";
		if(e)svgErrorFree(e);
		crSurfaceDestroy(surf);
		svgUnref(h);
		return false;
	}
	// cairo's premultiplied ARGB in native order (B G R A bytes) to straight RGBA
	const unsigned char* src=crSurfaceData(surf);
	int stride=crSurfaceStride(surf);
	Frame f;
	f.rgba.resize((size_t)p.w*p.h*4);
	for(int y=0;y<p.h;y++)
	{
		const uint32_t* row=(const uint32_t*)(src+(size_t)y*stride);
		uint8_t* o=&f.rgba[(size_t)y*p.w*4];
		for(int x=0;x<p.w;x++)
		{
			uint32_t v=row[x];
			int a=v>>24;
			int r=(v>>16)&255,g=(v>>8)&255,b=v&255;
			if(a&&(a<255))
			{
				r=std::min(255,(r*255+a/2)/a);
				g=std::min(255,(g*255+a/2)/a);
				b=std::min(255,(b*255+a/2)/a);
			}
			o[x*4]=r;o[x*4+1]=g;o[x*4+2]=b;o[x*4+3]=a;
		}
	}
	crSurfaceDestroy(surf);
	svgUnref(h);
	p.frames.push_back(std::move(f));
	return true;
}

// An SVG by its name (.svg, or .svgz gzipped) and first bytes, or "<svg" without.
static bool sniffSvg(const uint8_t* d, size_t n, const char* path)
{
	if(hasExt(path,".svgz"))return (n>=2)&&(d[0]==0x1F)&&(d[1]==0x8B);
	size_t i=0;
	if((n>=3)&&(d[0]==0xEF)&&(d[1]==0xBB)&&(d[2]==0xBF))i=3;
	while((i<n)&&isspace(d[i]))i++;
	if((n-i>=4)&&!memcmp(d+i,"<svg",4))return true;
	return hasExt(path,".svg")&&(i<n)&&(d[i]=='<');
}

static const char* sniff(const uint8_t* d, size_t n, const char* path)
{
	if(sniffSvg(d,n,path))return "SVG";
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
	if(!webpLoad())
	{
		err="WebP needs libwebp (libwebpdemux.so.2), which this machine does not have.";
		return false;
	}
	WebPAnimDecoderOptions opt;
	if(!pOptionsInit(&opt,WEBP_DEMUX_ABI_VERSION))
	{
		err="This libwebp is too old.";
		return false;
	}
	opt.color_mode=MODE_RGBA;
	opt.use_threads=1;
	WebPAnimDecoder* dec=pNew(&wd,&opt,WEBP_DEMUX_ABI_VERSION);
	if(!dec)
	{
		err="This WebP file could not be decoded.";
		return false;
	}
	WebPAnimInfo info;
	pGetInfo(dec,&info);
	p.w=info.canvas_width;
	p.h=info.canvas_height;
	int prev=0;
	while(pHasMore(dec))
	{
		uint8_t* buf=nullptr;
		int ts=0;
		if(!pGetNext(dec,&buf,&ts))break;
		Frame f;
		f.rgba.assign(buf,buf+(size_t)p.w*p.h*4);
		f.delay=ts-prev;
		prev=ts;
		p.frames.push_back(std::move(f));
	}
	pDelete(dec);
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
	if(!strcmp(fmt,"SVG"))return decodeSvg(path,p,err);
	if(!strcmp(fmt,"WebP"))
	{
#ifdef HAVE_WEBP
		return decodeWebp(data,p,err);
#else
		err="WebP needs libwebp: build img_dl.so with the libwebp headers installed.";
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

// ---------------------------------------------------------------- videos

struct Video
{
	std::mutex m;              // after the slot's lock, never the other way round
	std::condition_variable cv;
	bool stop=false;
	bool paused=false;
	bool atEnd=false;
	long long seek=-1;         // ms to go to, or -1
	pid_t pid=0;
	pid_t probePid=0;          // gst-discoverer, while it runs
	int probed=0;              // GStreamer: 0 being looked at, 1 a video, -1 none
	std::string path;
	int maxW=480;
	int maxH=360;
	int fps=12;
	bool loop=true;
	// what is known about it
	long long pos=0;           // ms of the frame shown
	long long dur=0;           // ms, 0 when unknown
	int srcW=0;
	int srcH=0;
	int shown=0;               // counts the frames shown
	std::string codec;
};

// The program that decodes videos: ffmpeg, or else GStreamer's gst-launch-1.0 (with
// gst-discoverer-1.0 to learn the size and length), or none, when videos are not
// pictures at all. T2FC_VIDEO=ffmpeg, gstreamer or none picks one.
#define ENG_NONE 0
#define ENG_FFMPEG 1
#define ENG_GST 2
static int engineState=-1;
static std::mutex engineLock;

static bool onPath(const char* prog)
{
	const char* path=getenv("PATH");
	if(!path)path="/usr/bin:/bin";
	std::string all=path;
	size_t a=0;
	while(a<=all.size())
	{
		size_t b=all.find(':',a);
		if(b==std::string::npos)b=all.size();
		std::string f=(b>a?all.substr(a,b-a):std::string("."))+"/"+prog;
		if(access(f.c_str(),X_OK)==0)return true;
		a=b+1;
	}
	return false;
}

static int videoEngine()
{
	std::lock_guard<std::mutex> lk(engineLock);
	if(engineState>=0)return engineState;
	bool ff=onPath("ffmpeg");
	bool gst=onPath("gst-launch-1.0")&&onPath("gst-discoverer-1.0");
	const char* want=getenv("T2FC_VIDEO");
	engineState=ENG_NONE;
	if(want&&!strcmp(want,"none"))engineState=ENG_NONE;
	else if(want&&!strcmp(want,"gstreamer"))engineState=gst?ENG_GST:ENG_NONE;
	else if(want&&!strcmp(want,"ffmpeg"))engineState=ff?ENG_FFMPEG:ENG_NONE;
	else if(ff)engineState=ENG_FFMPEG;
	else if(gst)engineState=ENG_GST;
	return engineState;
}

// The container of a video, from its first bytes (16 are enough), or nullptr.
static const char* sniffVideo(const uint8_t* d, size_t n, const char* path)
{
	if((n>=12)&&!memcmp(d+4,"ftyp",4))
	{
		// ISO media, but HEIF/AVIF pictures and M4A sound are that too
		static const char* notVideo[]={"heic","heix","heim","heis","mif1","msf1","avif","avis","M4A ","M4B ","M4P ","F4A ","F4B "};
		for(const char* b : notVideo)
			if(!memcmp(d+8,b,4))return nullptr;
		if(!memcmp(d+8,"qt  ",4))return "MOV";
		return "MP4";
	}
	if(hasExt(path,".mka")||hasExt(path,".weba"))return nullptr;   // sound only
	if((n>=4)&&(d[0]==0x1A)&&(d[1]==0x45)&&(d[2]==0xDF)&&(d[3]==0xA3))return hasExt(path,".webm")?"WebM":"MKV";
	if((n>=12)&&!memcmp(d,"RIFF",4)&&!memcmp(d+8,"AVI ",4))return "AVI";
	if((n>=4)&&!memcmp(d,"FLV\x01",4))return "FLV";
	if((n>=8)&&!memcmp(d,"\x30\x26\xB2\x75\x8E\x66\xCF\x11",8))return "WMV";
	if((n>=4)&&(d[0]==0)&&(d[1]==0)&&(d[2]==1)&&((d[3]==0xBA)||(d[3]==0xB3)))return "MPEG";
	if((n>=1)&&(d[0]==0x47)&&(hasExt(path,".ts")||hasExt(path,".m2ts")||hasExt(path,".mts")))return "MPEG-TS";
	if((n>=4)&&!memcmp(d,"OggS",4)&&hasExt(path,".ogv"))return "Ogg";
	return nullptr;
}

static long long nowMs()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();
}

// "00:01:23.45" into ms, or -1.
static long long parseClock(const char* t)
{
	int h=0,m=0;
	double sec=0;
	if(sscanf(t,"%d:%d:%lf",&h,&m,&sec)!=3)return -1;
	return (long long)((h*3600+m*60)*1000LL+sec*1000.0+0.5);
}

// The first "WxH" in a line of ffmpeg's ("1280x720", not "0x31637661").
static bool parseSize(const std::string& l, int& w, int& h)
{
	for(size_t i=0;i+2<l.size();i++)
	{
		if((l[i]<'1')||(l[i]>'9'))continue;
		if((i>0)&&(l[i-1]!=' '))continue;
		size_t k=i;
		while((k<l.size())&&isdigit((unsigned char)l[k]))k++;
		if((k>=l.size())||(l[k]!='x'))continue;
		size_t j=k+1;
		if((j>=l.size())||!isdigit((unsigned char)l[j]))continue;
		while((j<l.size())&&isdigit((unsigned char)l[j]))j++;
		if((j<l.size())&&(l[j]!=' ')&&(l[j]!=',')&&(l[j]!='['))continue;
		w=atoi(l.c_str()+i);
		h=atoi(l.c_str()+k+1);
		if((w>0)&&(h>0))return true;
	}
	return false;
}

// Starts a program with stdin from /dev/null; out gets its standard output, err its
// standard error. Returns its pid, 0 if it could not be started.
// GStreamer's hardware decoders (NVIDIA, VA, V4L2) take seconds to start, which a
// small picture is not worth: they are made the last choice.
static const char* gstSoftware=
	"nvh264dec:0,nvh265dec:0,nvav1dec:0,nvvp8dec:0,nvvp9dec:0,nvmpeg2videodec:0,"
	"nvmpeg4videodec:0,nvmpegvideodec:0,nvjpegdec:0,"
	"vah264dec:0,vah265dec:0,vaav1dec:0,vavp8dec:0,vavp9dec:0,vampeg2dec:0,vajpegdec:0,"
	"vaapih264dec:0,vaapih265dec:0,vaapiav1dec:0,vaapivp8dec:0,vaapivp9dec:0,vaapimpeg2dec:0,"
	"v4l2h264dec:0,v4l2h265dec:0,v4l2vp8dec:0,v4l2vp9dec:0,v4l2mpeg2dec:0,v4l2mpeg4dec:0";

static pid_t spawnProg(std::vector<std::string>& args, int& out, int& err)
{
	int po[2],pe[2];
	if(pipe2(po,O_CLOEXEC)!=0)return 0;
	if(pipe2(pe,O_CLOEXEC)!=0)
	{
		close(po[0]);
		close(po[1]);
		return 0;
	}
	std::vector<char*> argv;
	for(std::string& a : args)argv.push_back((char*)a.c_str());
	argv.push_back(nullptr);
	posix_spawn_file_actions_t fa;
	posix_spawn_file_actions_init(&fa);
	posix_spawn_file_actions_addopen(&fa,0,"/dev/null",O_RDONLY,0);
	posix_spawn_file_actions_adddup2(&fa,po[1],1);
	posix_spawn_file_actions_adddup2(&fa,pe[1],2);
	pid_t pid=0;
	// the environment, with gstSoftware for GStreamer
	std::vector<std::string> envs;
	std::string rank=std::string("GST_PLUGIN_FEATURE_RANK=")+gstSoftware;
	for(char** e=environ;*e;e++)
	{
		if(!strncmp(*e,"GST_PLUGIN_FEATURE_RANK=",24))
		{
			// what is set there wins, so it goes after
			rank=rank+","+(*e+24);
			continue;
		}
		envs.push_back(*e);
	}
	envs.push_back(rank);
	std::vector<char*> envp;
	for(std::string& e : envs)envp.push_back((char*)e.c_str());
	envp.push_back(nullptr);
	int r=posix_spawnp(&pid,argv[0],&fa,nullptr,argv.data(),envp.data());
	posix_spawn_file_actions_destroy(&fa);
	close(po[1]);
	close(pe[1]);
	if(r!=0)
	{
		close(po[0]);
		close(pe[0]);
		return 0;
	}
	out=po[0];
	err=pe[0];
	return pid;
}

// Records a started program as the video's, or kills it when the video has stopped.
static void videoPid(Video& v, pid_t pid)
{
	std::lock_guard<std::mutex> lk(v.m);
	v.pid=pid;
	if(v.stop&&pid)kill(pid,SIGKILL);
}

// Reads a line of fd into line, without its end; false at the end of it.
static bool readLine(int fd, std::string& line)
{
	line.clear();
	char c;
	while(read(fd,&c,1)==1)
	{
		if((c=='\n')||(c=='\r'))
		{
			if(line.empty())continue;
			return true;
		}
		line+=c;
	}
	return !line.empty();
}

// ffmpeg from ms start, frames no bigger than maxW*maxH. Reads what it says before
// the frames: the length, the video stream, and w*h, the size of the frames it
// sends (0 when it sends none; last is then its last words).
static pid_t startFfmpeg(Video& v, long long start, int& out, int& err, int& w, int& h, std::string& last)
{
	char ss[32];
	snprintf(ss,sizeof ss,"%.3f",start/1000.0);
	char vf[256];
	// square pixels, the frame rate, then no bigger than maxW*maxH
	snprintf(vf,sizeof vf,"scale=trunc(iw*sar/2)*2:ih,setsar=1,fps=%d,"
		"scale=w=min(%d\\,iw):h=min(%d\\,ih):force_original_aspect_ratio=decrease:flags=area",
		v.fps,v.maxW,v.maxH);
	std::vector<std::string> args={"ffmpeg","-hide_banner","-nostdin","-nostats","-loglevel","info",
		"-ss",ss,"-i",v.path,"-an","-sn","-dn","-vf",vf,"-pix_fmt","rgba","-f","rawvideo","pipe:1"};
	pid_t pid=spawnProg(args,out,err);
	if(!pid)return 0;
	videoPid(v,pid);
	std::string line;
	bool output=false;
	w=0;
	h=0;
	while((w==0)&&readLine(err,line))
	{
		size_t at;
		if((at=line.find("Duration: "))!=std::string::npos)
		{
			long long d=parseClock(line.c_str()+at+10);
			std::lock_guard<std::mutex> lk(v.m);
			if(d>0)v.dur=d;
		}
		if(line.find("Output #0")!=std::string::npos)output=true;
		if((at=line.find("Video: "))!=std::string::npos)
		{
			int a=0,b=0;
			if(output)
			{
				if(parseSize(line,a,b))
				{
					w=a;
					h=b;
				}
			}
			else
			{
				std::string codec=line.substr(at+7);
				size_t e=codec.find_first_of(" ,");
				if(e!=std::string::npos)codec.resize(e);
				parseSize(line,a,b);
				std::lock_guard<std::mutex> lk(v.m);
				if(v.codec.empty())
				{
					v.codec=codec;
					v.srcW=a;
					v.srcH=b;
				}
			}
		}
		last=line;
	}
	return pid;
}

// GStreamer's gst-discoverer-1.0 on the video: its length, codec, size and pixel
// shape. False when it has no video stream.
static bool gstDiscover(Video& v)
{
	std::vector<std::string> args={"gst-discoverer-1.0",v.path};
	int out=-1,err=-1;
	pid_t pid=spawnProg(args,out,err);
	if(!pid)return false;
	{
		std::lock_guard<std::mutex> lk(v.m);
		v.probePid=pid;
		if(v.stop)kill(pid,SIGKILL);
	}
	close(err);
	std::string line;
	bool inVideo=false;
	bool found=false;
	int w=0,h=0;
	long long dur=0;
	std::string codec;
	while(readLine(out,line))
	{
		size_t at;
		size_t k=line.find_first_not_of(' ');
		if(k==std::string::npos)continue;
		std::string t=line.substr(k);
		if(!t.compare(0,10,"Duration: "))dur=parseClock(t.c_str()+10);
		else if(!t.compare(0,7,"video #"))
		{
			inVideo=!found;
			found=true;
			if(inVideo&&((at=t.find(": "))!=std::string::npos))
			{
				codec=t.substr(at+2);
				size_t e=codec.find(" (");
				if(e!=std::string::npos)codec.resize(e);
			}
		}
		else if(!t.compare(0,7,"audio #")||!t.compare(0,10,"subtitles #")||!t.compare(0,11,"container #"))inVideo=false;
		else if(inVideo&&!t.compare(0,7,"Width: "))w=atoi(t.c_str()+7);
		else if(inVideo&&!t.compare(0,8,"Height: "))h=atoi(t.c_str()+8);
	}
	close(out);
	waitpid(pid,nullptr,0);
	std::lock_guard<std::mutex> lk(v.m);
	v.probePid=0;
	bool ok=found&&(w>0)&&(h>0);
	v.probed=ok?1:-1;
	if(ok)
	{
		if(dur>0)v.dur=dur;
		v.codec=codec;
		v.srcW=w;
		v.srcH=h;
	}
	v.cv.notify_all();
	return ok;
}

// gst-discoverer is slow to start (seconds), so it runs beside the playing.
static void gstProbe(std::shared_ptr<Video> v)
{
	gstDiscover(*v);
}

// gst-launch-1.0 from the start (it cannot seek: the frames before are skipped),
// w*h frames of fps a second on its standard output, the video fitted in with
// borders.
static pid_t startGst(Video& v, int w, int h, int& out, int& err)
{
	// gst-launch joins its arguments into one pipeline description: the file name
	// goes in quotes
	std::string loc="location=\"";
	for(char c : v.path)
	{
		if((c=='"')||(c=='\\'))loc+='\\';
		loc+=c;
	}
	loc+='"';
	char caps[200];
	snprintf(caps,sizeof caps,"video/x-raw,format=RGBA,width=%d,height=%d,framerate=%d/1,pixel-aspect-ratio=1/1",w,h,v.fps);
	std::vector<std::string> args={"gst-launch-1.0","-q","filesrc",loc,"!","decodebin","!",
		"videoconvert","!","videoscale","!","videorate","!",caps,"!","fdsink","fd=1"};
	pid_t pid=spawnProg(args,out,err);
	if(pid)videoPid(v,pid);
	return pid;
}

static bool readAll(int fd, uint8_t* buf, size_t n)
{
	size_t got=0;
	while(got<n)
	{
		ssize_t r=read(fd,buf+got,n-got);
		if(r<0)
		{
			if(errno==EINTR)continue;
			return false;
		}
		if(r==0)return false;
		got+=r;
	}
	return true;
}

// Publishes a frame, or an error, if the video still plays in slot s.
static bool videoPublish(int s, const std::shared_ptr<Video>& v, std::shared_ptr<Picture> pic, const std::string& err)
{
	Slot& sl=slots[s];
	std::lock_guard<std::mutex> lk(sl.m);
	if(sl.vid!=v)return false;
	if(pic)
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
	return true;
}

static void videoRun(int s, std::shared_ptr<Video> v)
{
	int engine=videoEngine();
	long long start=0;
	bool everShown=false;
	// GStreamer: frames of the whole maxW*maxH, as the size is not known in time
	int gw=v->maxW&~1;
	int gh=v->maxH&~1;
	if(engine==ENG_GST)std::thread(gstProbe,v).detach();
	int frameMs=1000/v->fps;
	while(true)
	{
		int out=-1,err=-1,w=0,h=0;
		std::string last;
		long long t0=start;         // the time of the first frame the program sends
		pid_t pid=0;
		if(engine==ENG_GST)
		{
			pid=startGst(*v,gw,gh,out,err);
			w=gw;
			h=gh;
			t0=0;
		}
		else pid=startFfmpeg(*v,start,out,err,w,h,last);
		if(!pid)
		{
			videoPublish(s,v,nullptr,engine==ENG_GST?"gst-launch-1.0 could not be started.":"ffmpeg could not be started.");
			return;
		}
		fcntl(err,F_SETFL,fcntl(err,F_GETFL)|O_NONBLOCK);
		long long target=start;     // frames before this are skipped
		long long wall0=0;          // the frame of time tw0 was shown at wall0
		long long tw0=-1;
		int n=0;                    // frames read
		int shownHere=0;
		if(w>0)
		{
			size_t bytes=(size_t)w*h*4;
			std::vector<uint8_t> skip;
			while(true)
			{
				long long t=t0+(long long)n*1000/v->fps;
				bool show=t+frameMs/2>=target;
				std::shared_ptr<Picture> pic;
				uint8_t* buf;
				if(show)
				{
					pic=std::make_shared<Picture>();
					pic->w=w;
					pic->h=h;
					pic->frames.resize(1);
					pic->frames[0].rgba.resize(bytes);
					pic->frames[0].delay=frameMs;
					buf=pic->frames[0].rgba.data();
				}
				else
				{
					skip.resize(bytes);
					buf=skip.data();
				}
				if(!readAll(out,buf,bytes))break;
				n++;
				char junk[4096];
				ssize_t r;
				while((r=read(err,junk,sizeof junk-1))>0)
				{
					junk[r]=0;
					std::string words(junk);
					while(!words.empty()&&((words.back()=='\n')||(words.back()=='\r')))words.pop_back();
					size_t nl=words.find_last_of("\n\r");
					if(!words.empty())last=nl==std::string::npos?words:words.substr(nl+1);
				}
				if(!show)
				{
					std::lock_guard<std::mutex> lk(v->m);
					if(v->stop||((v->seek>=0)&&(v->seek<t)))break;
					continue;
				}
				{
					std::lock_guard<std::mutex> lk(v->m);
					pic->format=v->codec;
					v->pos=t;
					v->shown++;
				}
				if(!videoPublish(s,v,pic,""))break;
				everShown=true;
				shownHere++;
				if(tw0<0)
				{
					tw0=t;
					wall0=nowMs();
				}
				// wait for the next frame's time, or a pause, seek or stop
				long long next=t+frameMs;
				std::unique_lock<std::mutex> lk(v->m);
				while(true)
				{
					if(v->stop)break;
					if(v->seek>=0)
					{
						// GStreamer goes on reading up to a later time; anything
						// else starts again from there
						if((engine==ENG_GST)&&(v->seek>t))
						{
							target=v->seek;
							v->seek=-1;
							tw0=-1;
						}
						break;
					}
					if(v->paused)
					{
						v->cv.wait(lk);
						tw0=next;
						wall0=nowMs();
						continue;
					}
					long long left=wall0+(next-tw0)-nowMs();
					if(left<=0)break;
					v->cv.wait_for(lk,std::chrono::milliseconds(left));
				}
				if(v->stop||(v->seek>=0))break;
			}
		}
		kill(pid,SIGKILL);
		close(out);
		close(err);
		waitpid(pid,nullptr,0);
		std::unique_lock<std::mutex> lk(v->m);
		v->pid=0;
		if(v->stop)return;
		if(v->seek>=0)
		{
			start=v->seek;
			v->seek=-1;
			v->atEnd=false;
			continue;
		}
		if((shownHere==0)&&(!everShown||(start==0)))
		{
			// GStreamer: what gst-discoverer says, when it is done
			if(engine==ENG_GST)
				v->cv.wait_for(lk,std::chrono::seconds(15),[&]{return v->stop||(v->probed!=0);});
			if(v->stop)return;
			lk.unlock();
			std::string why=last;
			if(v->codec.empty())why="There is no video in this file.";
			else if(why.empty())why="No frames came from the video.";
			videoPublish(s,v,nullptr,why);
			return;
		}
		// the end: again from the start, or stay on the last frame until told
		if(v->loop)
		{
			start=0;
			continue;
		}
		v->paused=true;
		v->atEnd=true;
		while(!v->stop&&(v->seek<0)&&v->paused)v->cv.wait(lk);
		if(v->stop)return;
		v->atEnd=false;
		if(v->seek>=0)
		{
			start=v->seek;
			v->seek=-1;
		}
		else start=0;
	}
}

// Ends the video of a slot whose lock is held.
static void videoStop(Slot& sl)
{
	if(!sl.vid)return;
	std::shared_ptr<Video> v=sl.vid;
	sl.vid.reset();
	std::lock_guard<std::mutex> lk(v->m);
	v->stop=true;
	if(v->pid)kill(v->pid,SIGKILL);
	if(v->probePid)kill(v->probePid,SIGKILL);
	v->cv.notify_all();
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
	if(!fmt)return (sniffVideo(head,n,path)&&(videoEngine()!=ENG_NONE))?1:0;
#ifndef HAVE_WEBP
	if(!strcmp(fmt,"WebP"))return 0;
#endif
	if(!strcmp(fmt,"SVG")&&!svgLoad())return 0;
	return 1;
}

// As img_is, for the first bytes of a file read some other way (from an archive);
// name gives the extension TGA is known by.
int img_sniff(char* head, int n, char* name)
{
	const char* fmt=sniff((const uint8_t*)head,n<0?0:(size_t)n,name);
	if(!fmt)return (sniffVideo((const uint8_t*)head,n<0?0:(size_t)n,name)&&(videoEngine()!=ENG_NONE))?1:0;
#ifndef HAVE_WEBP
	if(!strcmp(fmt,"WebP"))return 0;
#endif
	if(!strcmp(fmt,"SVG")&&!svgLoad())return 0;
	return 1;
}

// Starts decoding a file into slot s; img_state tells when it is done.
int img_open(int s, char* path)
{
	if(!validSlot(s))return 0;
	Slot& sl=slots[s];
	// a video plays through ffmpeg instead
	uint8_t head[16];
	size_t hn=0;
	FILE* f=fopen(path,"rb");
	if(f)
	{
		hn=fread(head,1,sizeof head,f);
		fclose(f);
	}
	bool video=!sniff(head,hn,path)&&sniffVideo(head,hn,path)&&(videoEngine()!=ENG_NONE);
	std::lock_guard<std::mutex> lk(sl.m);
	videoStop(sl);
	sl.state=1;
	sl.pic.reset();
	sl.version++;
	if(video)
	{
		sl.want.clear();
		sl.wantGen++;
		if(!sl.busy)sl.doneGen=sl.wantGen;
		auto v=std::make_shared<Video>();
		v->path=path;
		v->maxW=sl.vMaxW;
		v->maxH=sl.vMaxH;
		v->fps=sl.vFps;
		v->loop=sl.vLoop;
		sl.vid=v;
		std::thread(videoRun,s,v).detach();
		return 1;
	}
	sl.want=path;
	sl.wantGen++;
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
	videoStop(sl);
	sl.want.clear();
	sl.wantGen++;
	sl.state=0;
	sl.pic.reset();
	sl.cache.clear();
	sl.cache.shrink_to_fit();
	sl.pcache.clear();
	sl.pcache.shrink_to_fit();
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

// k: 0 width, 1 height, 2 frames, 3 1 if any pixel is not opaque. For a video
// (the frames as ffmpeg sends them, scaled down): 4 1 for a video, 5 the time of
// the frame shown in ms, 6 the length in ms (0 unknown), 7 1 when paused, 8 counts
// the frames shown, 9 and 10 the video's own width and height, 11 1 at the end.
int img_info(int s, int k)
{
	if(!validSlot(s))return 0;
	std::shared_ptr<Picture> p;
	{
		std::lock_guard<std::mutex> lk(slots[s].m);
		p=slots[s].pic;
		if(k>=4)
		{
			std::shared_ptr<Video> v=slots[s].vid;
			if(!v)return 0;
			std::lock_guard<std::mutex> vl(v->m);
			if(k==4)return 1;
			if(k==5)return (int)v->pos;
			if(k==6)return (int)v->dur;
			if(k==7)return v->paused?1:0;
			if(k==8)return v->shown;
			if(k==9)return v->srcW;
			if(k==10)return v->srcH;
			if(k==11)return v->atEnd?1:0;
			return 0;
		}
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

// How the next video opened in slot s plays: frames no bigger than maxW*maxH, fps
// of them a second, and again from the start at the end (loop 1) or not.
void img_video_opts(int s, int maxW, int maxH, int fps, int loop)
{
	if(!validSlot(s))return;
	Slot& sl=slots[s];
	std::lock_guard<std::mutex> lk(sl.m);
	sl.vMaxW=std::max(16,maxW);
	sl.vMaxH=std::max(16,maxH);
	sl.vFps=std::min(60,std::max(1,fps));
	sl.vLoop=loop!=0;
}

// Controls the video of slot s: op 0 pauses (arg 1) or plays (0), 1 toggles that,
// 2 goes to arg ms, 3 goes arg ms on (back when negative).
void img_video_ctl(int s, int op, int arg)
{
	if(!validSlot(s))return;
	Slot& sl=slots[s];
	std::lock_guard<std::mutex> lk(sl.m);
	std::shared_ptr<Video> v=sl.vid;
	if(!v)return;
	std::lock_guard<std::mutex> vl(v->m);
	if(op==0)v->paused=arg!=0;
	else if(op==1)v->paused=!v->paused;
	else if((op==2)||(op==3))
	{
		long long t=arg;
		if(op==3)t=(v->seek>=0?v->seek:v->pos)+arg;
		if((v->dur>0)&&(t>v->dur-500))t=v->dur-500;
		if(t<0)t=0;
		v->seek=t;
		v->pos=t;
	}
	v->cv.notify_all();
}

// 0: shrinking averages the source pixels a screen pixel covers; 1: a Lanczos
// filter (a = 3) where the picture is shown smaller on both axes. Enlarging
// repeats the pixels either way.
static int filterMode=0;

void img_filter(int mode)
{
	filterMode=mode;
}

static double lanczos3(double x)
{
	if(x<0)x=-x;
	if(x<1e-9)return 1.0;
	if(x>=3.0)return 0.0;
	double px=3.14159265358979323846*x;
	return 3.0*std::sin(px)*std::sin(px/3.0)/(px*px);
}

// The taps of one screen pixel along an axis: source pixels first..first+n-1 with
// these weights, which add up to 1; first<0 when its middle is off the picture.
struct Taps
{
	int first=-1;
	std::vector<float> w;
};

static void lanczosTaps(double o, double s, int n, int size, std::vector<Taps>& out)
{
	out.assign(n,Taps());
	double r=3.0*s;
	for(int k=0;k<n;k++)
	{
		double c=o+(k+0.5)*s;        // the middle, in source pixels
		if((c<0)||(c>=size))continue;
		int a=(int)std::floor(c-r);
		int b=(int)std::ceil(c+r);
		if(a<0)a=0;
		if(b>size)b=size;
		Taps& t=out[k];
		t.first=a;
		t.w.resize(b-a);
		double sum=0;
		for(int i=a;i<b;i++)
		{
			double w=lanczos3((i+0.5-c)/s);
			t.w[i-a]=(float)w;
			sum+=w;
		}
		if(sum!=0)
			for(float& w:t.w)
				w=(float)(w/sum);
	}
}

static inline double clamp255(double v)
{
	if(v<0)return 0;
	if(v>255)return 255;
	return v;
}

// render's work with the Lanczos filter: each source row is filtered across once,
// into a ring of rows as many as a screen pixel's taps down, and the rows are
// filtered down from there. Colours are weighted by their opacity.
static void renderLanczos(const Picture& p, int frame, double ox, double oy, double sx, double sy,
	int pw, int ph, uint32_t bg, std::vector<uint32_t>& out, int cox, int coy, int csh)
{
	const uint8_t* src=p.frames[frame].rgba.data();
	std::vector<Taps> tx;
	std::vector<Taps> ty;
	lanczosTaps(ox,sx,pw,p.w,tx);
	lanczosTaps(oy,sy,ph,p.h,ty);
	int ring=(int)std::ceil(6.0*sy)+3;
	std::vector<float> rows((size_t)ring*pw*4);
	std::vector<int> rowOf(ring,-1);    // the source row each ring row holds
	for(int py=0;py<ph;py++)
	{
		const Taps& v=ty[py];
		if(v.first<0)continue;
		int nv=(int)v.w.size();
		for(int j=v.first;j<v.first+nv;j++)
		{
			int slot=j%ring;
			if(rowOf[slot]==j)continue;
			rowOf[slot]=j;
			float* dst=&rows[(size_t)slot*pw*4];
			const uint8_t* line=src+(size_t)j*p.w*4;
			for(int px=0;px<pw;px++)
			{
				const Taps& h=tx[px];
				float r=0,g=0,b=0,a=0;
				if(h.first>=0)
				{
					const uint8_t* q=line+(size_t)h.first*4;
					for(float w:h.w)
					{
						float aw=w*q[3];
						r+=aw*q[0];
						g+=aw*q[1];
						b+=aw*q[2];
						a+=aw;
						q+=4;
					}
				}
				dst[px*4]=r;
				dst[px*4+1]=g;
				dst[px*4+2]=b;
				dst[px*4+3]=a;
			}
		}
		for(int px=0;px<pw;px++)
		{
			if(tx[px].first<0)continue;
			double r=0,g=0,b=0,a=0;
			for(int k=0;k<nv;k++)
			{
				const float* q=&rows[((size_t)((v.first+k)%ring)*pw+px)*4];
				double w=v.w[k];
				r+=w*q[0];
				g+=w*q[1];
				b+=w*q[2];
				a+=w*q[3];
			}
			uint32_t under=((((px+cox)>>csh)+((py+coy)>>csh))&1)?0x9A9A9A:0x666666;
			double cover=a/255.0;
			if(cover<0)cover=0;
			if(cover>1)cover=1;
			double cr=0,cg=0,cb=0;
			if(a>1e-6)
			{
				cr=clamp255(r/a);
				cg=clamp255(g/a);
				cb=clamp255(b/a);
			}
			auto ch=[&](uint32_t c,int sh){return (double)((c>>sh)&255);};
			int R=(int)(cr*cover+ch(under,16)*(1-cover)+0.5);
			int G=(int)(cg*cover+ch(under,8)*(1-cover)+0.5);
			int B=(int)(cb*cover+ch(under,0)*(1-cover)+0.5);
			out[(size_t)py*pw+px]=(R<<16)|(G<<8)|B;
		}
	}
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
	int pw, int ph, uint32_t bg, std::vector<uint32_t>& out, int cox=0, int coy=0, int csh=2)
{
	out.assign((size_t)pw*ph,bg);
	const uint8_t* src=p.frames[frame].rgba.data();
	double ox=cx-pw*0.5*sx;
	double oy=cy-ph*0.5*sy;
	if((filterMode==1)&&(sx>1.0)&&(sy>1.0))
	{
		renderLanczos(p,frame,ox,oy,sx,sy,pw,ph,bg,out,cox,coy,csh);
		return;
	}
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
			uint32_t under=((((px+cox)>>csh)+((py+coy)>>csh))&1)?0x9A9A9A:0x666666;
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
		(sl.cacheKeyI[3]==ph)&&(sl.cacheKeyI[4]==bg)&&(sl.cacheKeyI[5]==filterMode)&&(sl.cacheKeyF[0]==cx)&&
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
		sl.cacheKeyI[5]=filterMode;
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

// Real pixels for a graphic window: the view is fullW*fullH pixels with source point
// cx,cy in the middle and scale source pixels per pixel; out gets the w*h of them
// at ox,oy, as 0xFFRRGGBB.
int img_pixels(int s, int f, double cx, double cy, double scale, int fullW, int fullH,
	int ox, int oy, int w, int h, int bg, int* out)
{
	if(!validSlot(s)||(w<=0)||(h<=0)||!(scale>0))return 0;
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
	double key[13]={(double)version,(double)f,cx,cy,scale,(double)fullW,(double)fullH,
		(double)ox,(double)oy,(double)w,(double)h,(double)bg,(double)filterMode};
	bool same=(sl.pcache.size()==(size_t)w*h);
	for(int k=0;(k<13)&&same;k++)
		if(sl.pcacheKey[k]!=key[k])same=false;
	if(!same)
	{
		// the middle of the part asked for, in the source
		double x0=cx-fullW*0.5*scale+ox*scale;
		double y0=cy-fullH*0.5*scale+oy*scale;
		render(*p,f,x0+w*0.5*scale,y0+h*0.5*scale,scale,scale,w,h,(uint32_t)bg,sl.pcache,ox,oy,3);
		for(int k=0;k<13;k++)
			sl.pcacheKey[k]=key[k];
	}
	const uint32_t* src=sl.pcache.data();
	for(size_t i=0;i<(size_t)w*h;i++)
		out[i]=(int)(0xFF000000u|src[i]);
	return 1;
}

extern "C" int img_pixels_wrapper(ScincVM* s, void*){ScincBoundCall(s,img_pixels);return 0;}
extern "C" int img_is_wrapper(ScincVM* s, void*){ScincBoundCall(s,img_is);return 0;}
extern "C" int img_sniff_wrapper(ScincVM* s, void*){ScincBoundCall(s,img_sniff);return 0;}
extern "C" int img_open_wrapper(ScincVM* s, void*){ScincBoundCall(s,img_open);return 0;}
extern "C" int img_free_wrapper(ScincVM* s, void*){ScincBoundCall(s,img_free);return 0;}
extern "C" int img_state_wrapper(ScincVM* s, void*){ScincBoundCall(s,img_state);return 0;}
extern "C" int img_error_wrapper(ScincVM* s, void*){ScincBoundCall(s,img_error);return 0;}
extern "C" int img_format_wrapper(ScincVM* s, void*){ScincBoundCall(s,img_format);return 0;}
extern "C" int img_info_wrapper(ScincVM* s, void*){ScincBoundCall(s,img_info);return 0;}
extern "C" int img_delay_wrapper(ScincVM* s, void*){ScincBoundCall(s,img_delay);return 0;}
extern "C" int img_rotate_wrapper(ScincVM* s, void*){ScincBoundCall(s,img_rotate);return 0;}
extern "C" int img_video_opts_wrapper(ScincVM* s, void*){ScincBoundCall(s,img_video_opts);return 0;}
extern "C" int img_video_ctl_wrapper(ScincVM* s, void*){ScincBoundCall(s,img_video_ctl);return 0;}
extern "C" int img_draw_wrapper(ScincVM* s, void*){ScincBoundCall(s,img_draw);return 0;}
extern "C" int img_filter_wrapper(ScincVM* s, void*){ScincBoundCall(s,img_filter);return 0;}
