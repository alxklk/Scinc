#define G_SCREEN_WIDTH 320
#define G_SCREEN_HEIGHT 240
#ifdef __JS__
#define G_SCREEN_SCALE 2
#else
#define G_SCREEN_SCALE 4
#endif
#define G_SCREEN_MODE 1

#include "sound.h"
#include "graphics.h"
#include "penta.h"
#include "minos.h"
#include "../../ui/menu/menu.h"
#define NO_GREEDY_EVENT
#include "../../ws.h"

int gseed=4734234;

int MAXPIECE=25;

int irand(int& seed)
{
	seed=(seed*1103515245+12345)&0x7ffffff;
	return seed;
}

float frand(int& seed)
{
	return irand(seed)/float(0x7ffffff);
}

float Fmod(float x,float y)
{
	return x-int(x/y)*y;
}

float Fract(float x)
{
	return x-int(x);
}

float Fabs(float x)
{
	return x<0?-x:x;
}

float S_Curve(float x)
{
	return x*x*(-2.0f*x+3.0f);
}


float Min(float x,float y){return x<y?x:y;}

float bell_curve3(float x)
{
	float x2=x*x;
	return x2*16.*(x2-2.*x+1.);
}

#define M_PI 3.141592654

CSound snd;

int MakeShootSound(int len)
{
	int res=snd.snd_add(len);
	float dlen=1.0/len;
	{
		float t=0.;
		float f=0.;
		for(int i=0;i<len;i++)
		{
			float v=bell_curve3(f)*.05;
			float x=t*440.*12.*M_PI;
			float l=sin(x)*v*sin(f*M_PI*14);
			if(f<0.25)l=l+sin(x*8.)*bell_curve3(f/0.25)*0.25*cos(f*M_PI*14.);
			snd.snd_data(res,i,l,l);
			t+=1./48000.*(f*f)*2.2;
			f+=dlen;
		}
	}
	return res;
}

int adseed=384520983;

void AddDing(int len, float* a, int p, float w, float V)
{
	float l=48000*.1*(.5+1.5*frand(adseed));
	for(int i=0;i<l;i++)
	{
		int pi=p+i;
		if(pi>=len)
			break;
		float v=i/l;
		float vv=(1.-v);
		vv*=vv;
		//vv*=vv;
		a[pi]+=vv*sin(v*w);//*(1.-pi/float(len))*V;
	}
}

int MakeExplodeSound(int len, int seed)
{
	int res=snd.snd_add(len);
	//printf("E %i\n", len);
	float* eSB;
	eSB=(float*)malloc(sizeof(float)*len);
	//1printf("E %i\n", len);
	for(int i=0;i<len;i++)
	{
		eSB[i]=0;
	}
	for(int i=0;i<100;i++)
	{
		int p=frand(seed)*frand(seed)*frand(seed)*len*.9;
		float w=3000+frand(seed)*1500-i*15;
		//printf("%i  %f\n",p, w);
		AddDing(len, eSB, p, w, 1);
	}
	float max=0;
	for(int i=0;i<len;i++)
	{
		float x=eSB[i];
		if(x<0)x=-x;
		if(max<x)max=x;
	}
	for(int i=0;i<len;i++)
	{
		eSB[i]/=max;
		float v=(1.-i/float(len));
		eSB[i]*=v*v;
	}
	for(int i=0;i<len;i++)
	{
		snd.snd_data(res,i,eSB[i],eSB[i]);
	}
	return res;
}

int MakeEngineSound(int len, int seed)
{
	int res=snd.snd_add(len);
	{
		float t=0.;
		float fr0=frand(seed);
		float f=0.;
		float dlen=1./len;
		for(int i=0;i<len;i++)
		{
			float f=i/12000.0;
			float v=bell_curve3(f);
			float l=fr0*v*.125+frand(seed)*v*0.125;
			if(!(i&64))
				fr0=frand(seed);
			snd.snd_data(res,i,l,l);
			f+=dlen;
		}
	}
	return res;
}


#define ROT_SPEED 8
#define MOV_SPEED 8

#define HIT_LEFT 2
#define HIT_RIGHT 4
#define HIT_SIDES (HIT_LEFT|HIT_RIGHT)
#define HIT_BOTTOM 8
#define HIT_CELL 1

#ifndef __SCINC__
unsigned
#endif

int stdcols[12]=
{
	0xffe010a0,
	0xff8010f0,
	0xff0735ff,
	0xff0090ff,// cyan
	0xff00d070,// aquamarine
	0xff60c000,// lime
	0xffe8d000,// yellow
	0xffff8000,// orange
	0xffff0020,// red
	0xffffffff,
	0xff000000, 
	0xff808080 
};

Graph g;

int LitColor(int col, float l)
{
	float cr=((col    )&0xff)/255.;
	float cg=((col>> 8)&0xff)/255.;
	float cb=((col>>16)&0xff)/255.;
	cr*=l;
	cg*=l;
	cb*=l;
	cr+=l*l*l;
	cg+=l*l*l;
	cb+=l*l*l;
	if(cr>1)cr=1;
	if(cg>1)cg=1;
	if(cb>1)cb=1;
	return int(cr*255)|(int(cg*255)<<8)|(int(cb*255)<<16)|0xff000000;
}

void DrawAChip(float x, float y, float angle, float size, int col)
{
	float dx=size*cos(angle);
	float dy=size*sin(angle);
	g.clear();
	g.M(x+dx+dy,y-dy+dx);
	g.l(-2*dy,-2*dx);
	g.l(-2*dx, 2*dy);
	g.l( 2*dy, 2*dx);
	g.l( 2*dx,-2*dy);
	g.fin();
	g.rgba32(col);
	g.fill1();
	float l=sqrt(dx*dx+dy*dy);
	float ldx=dx/l;
	float ldy=dy/l;
	x-=.5;y-=.5;
	g.rgba32(LitColor(col,ldy*.5+.5));
	g.hairline(x+dx+dy,y-dy+dx,x+dx-dy,y-dy-dx);
	g.rgba32(LitColor(col,ldx*.5+.5));
	g.hairline(x+dx-dy,y-dy-dx,x-dx-dy,y+dy-dx);
	g.rgba32(LitColor(col,-ldy*.5+.5));
	g.hairline(x-dx-dy,y+dy-dx,x-dx+dy,y+dy+dx);
	g.rgba32(LitColor(col,-ldx*.5+.5));
	g.hairline(x-dx+dy,y+dy+dx,x+dx+dy,y-dy+dx);
}

void Background()
{
	float t=Time();
	/*
	g.clear();
	g.rgba(.4,.2,.0,1);
	g.FillRT();
	g.width(140,1);
	g.clear();
	g.M(0,0);g.l(320,0);g.fin();
	g.rgba(.2,.6,.9,1);
	//g.stroke();
	g.clear();
	g.M(0,240);g.l(320,0);g.fin();
	g.width(140,1);
	g.rgba(.9,.6,.2,1);
	//g.stroke();

	g.clear();
	g.M(160,0);g.l(0,240);g.fin();
	g.width(140,1);
	g.rgba(.9,.6,.0,1);
	//g.stroke();

	g.clear();
	g.M(0,120);g.l(320,0);g.fin();
	g.width(140,1);
	g.rgba(.2,.9,.8,1);
	//g.stroke();
	int seed=23452345;
	int seed1=76853345;
	g.clear();
	g.gray(1);
	g.alpha(1);
	g.clear();*/

	#define STRIP_HEIGHT 16
	for(int i=0;i<G_SCREEN_HEIGHT+STRIP_HEIGHT;i+=STRIP_HEIGHT)
	{
		int gr=int(sin(t*.147+i*.011-54)*50+68);
		int gg=int(sin(t*.149-i*.017+13)*50+68);
		int gb=int(sin(t*.141+i*.013-77)*50+68);
		g.Rect(0,i,G_SCREEN_WIDTH,STRIP_HEIGHT,0xff000000|(gr<<16)|(gg<<8)|gb);
	}

	float T=t*.5;
	
	g.clear();
	for(int i=0;i<6;i++)
	{
		float x=(cos(i*M_PI/100*2.*5+T*1.1 )*.8+sin(i*M_PI/100*2.*29-T*0.12)*.2)*150+160;
		float y=(sin(i*M_PI/100*2.*4+T*2.93)*.8+cos(i*M_PI/100*2.*27-T*0.37)*.2)*110+120;
		DrawAChip(x,y,i*M_PI/100*2.*4+T*(-2.645+i*1.21),5,stdcols[i&7]);
	}

	T-=0.1;
	for(int i=0;i<10;i++)
	{
		float x=(cos(i*M_PI/100*2.*5+T*1.1 )*.8+sin(i*M_PI/100*2.*29-T*0.12)*.2)*150+160;
		float y=(sin(i*M_PI/100*2.*4+T*2.93)*.8+cos(i*M_PI/100*2.*27-T*0.37)*.2)*110+120;
		DrawAChip(x,y,i*M_PI/100*2.*4+T*(-2.645+i*1.21),4,stdcols[(i+2)&7]);
	}

	T-=0.25;
	for(int i=0;i<10;i++)
	{
		float x=(cos(i*M_PI/100*2.*5+T*1.1 )*.8+sin(i*M_PI/100*2.*29-T*0.12)*.2)*150+160;
		float y=(sin(i*M_PI/100*2.*4+T*2.93)*.8+cos(i*M_PI/100*2.*27-T*0.37)*.2)*110+120;
		DrawAChip(x,y,i*M_PI/100*2.*4+T*(-2.645+i*1.21),2,stdcols[(i+5)&7]);
	}

}

struct sRemLine
{
	int y;
	double time;
	float rnd[32];
	int line[32];
};

struct IPOS
{
	int x;
	int y;
	float dy;
	int c;
};

class CBrickSet
{
public:
	IPOS p[32];
	int n;
	void Add(int x, int y, float dy, int c)
	{
		p[n].x=x;
		p[n].y=y;
		p[n].dy=dy;
		p[n].c=c;
		n++;
	}
};

struct SFloodFiller{
	int W;
	int H;
	int *field;
	float* fieldD;
	int *mask;
	int fill(int x, int y, CBrickSet& bs, int c)
	{
		if(x<0)
			return 0;
		if(y<0)
			return 0;
		if(y>=H)
			return 0;
		if(x>=W)
			return 0;
		if(c!=field[W*y+x])
			return 0;
		if(mask[W*y+x])
			return 0;
		if(bs.n>31)
			return 0;
		bs.Add(x,y,fieldD[W*y+x],c);
		mask[W*y+x]=1;
		return 1+
		fill(x+1, y  , bs, c)+
		fill(x-1, y  , bs, c)+
		fill(x  , y+1, bs, c)+
		fill(x  , y-1, bs, c);
	}
};

int FindPiece(int *field, float* fieldD, int* imask, int W, int H, int x, int y, CBrickSet& bs)
{
	if(!field[W*y+x])
	{
		return 0;
	}
	SFloodFiller ff;
	ff.mask=imask;
	ff.field=field;
	ff.fieldD=fieldD;
	ff.W=W;
	ff.H=H;
	return ff.fill(x,y,bs,field[W*y+x]);
};

struct MLFig
{
	int cnt;
	MLItem ml[16];
};

MLFig mlfs[25];

#define NREMLINES 6
#define W 12
#define H 24
class Game
{
public:
	float cellSize;
	int field[W*H];
	int mask[W*H];
	float fieldD[W*H];
	float x;
	float y;
	int fig5[5*5*25];
	sRemLine remLines[NREMLINES];
	int lastRemLine;
	int col;
	int nextcol;
	int piece;
	int nextPiece;
	int nextPA;
	int order;

	int sndExplode;
	int sndRocket;
	int sndShoot;
	bool gameOver;
	bool showMenu;
	bool unPaused;

	void InitFig()
	{
		sndExplode=MakeExplodeSound(48000,gseed);
		sndShoot=MakeShootSound(12000);
		sndRocket=MakeEngineSound(12000,gseed);

		for(int i=0;i<25;i++)
		{
			Shape s;
			s.cellSize=cellSize;
			s.Make(&(figtpl[i*5*5]),5,5);
			s.Optimize();
			s.Shrink();
			mlfs[i].cnt=s.MakeML(&(mlfs[i].ml[0]),16);
			for(int j=0;j<mlfs[i].cnt;j++)
			{
				mlfs[i].ml[j].x/=float(cellSize);
				mlfs[i].ml[j].y/=float(cellSize);
			}
		}
		for(int i=0;i<5*5*25;i++)
		{
			if(figtpl[i]==' ')
				fig5[i]=0x00;
			else
				fig5[i]=0x01;
		}

		for(int f=7;f<25;f++)
		{
			int w=0;
			int h=0;
			for(int i=0;i<5;i++)
			{
				for(int j=0;j<5;j++)
				{
					if(fig5[f*25+i*5+j])
					{
						if(h<i+1)h=i+1;
						if(w<j+1)w=j+1;
					}
				}
			}
			boxes[f*2]=w;
			boxes[f*2+1]=h;
			rotCenter[f*2]=w*.5;
			rotCenter[f*2+1]=h*.5;
		}

		//for(int i=0;i<25;i++)
		//{
		//	printf("{ %i, %i},\n", boxes[i*2],boxes[i*2+1]);
		//}
	}

	void Restore()
	{

	}

	void NewPiece()
	{
		falling=false;
		px=W/2;
		py=H;
		newpx=px;
		newpy=py;
		newpa=pa=nextPA;
		nextPA=irand(gseed)%4;
		col=nextcol;
		nextcol=irand(gseed)%7;
		piece=nextPiece;
		nextPiece=irand(gseed)%MAXPIECE;
		int tryToPut=Put(piece,px,py,pa,0,false);
		if(tryToPut!=0)
		{
			gameOver=true;
			//printf("Game Over\n");
		}
		order++;

	}
	void ClearField()
	{
		for(int i=0;i<W*H;i++)field[i]=0;
		for(int i=0;i<W*H;i++)fieldD[i]=0;
		for(int i=0;i<NREMLINES;i++)
			remLines[i].time=-10;
	}
	void Init()
	{
		//nextPiece=irand(gseed)%MAXPIECE;
		//nextcol=irand(gseed)%7;
		pa=0;
		nextPA=irand(gseed)%4;
		nextPiece=irand(gseed)%MAXPIECE;
		NewPiece();
		NewPiece();
		t=0;
		scores=0;
		unPaused=true;
		cellSize=8;
		x=G_SCREEN_WIDTH/2 -W*cellSize/2+.5;
		y=G_SCREEN_HEIGHT/2-H*cellSize/2+.5;
		//field=(int*)malloc(sizeof(int)*W*H);
		//fieldD=(float*)malloc(sizeof(float)*W*H);
		ClearField();
		lastRemLine=0;
		gameOver=false;
	}

	void DrawChip(int j, int i, float d, int col)
	{
		g.Rect(x+j*cellSize+1,y+i*cellSize+int(d*cellSize)+1, cellSize, cellSize, stdcols[col]);
	}

	void CalcPieceDelta(int piece, int a, float& dx, float& dy)
	{
		if(a==0){dx=Fract(rotCenter[piece*2  ]);dy=Fract(rotCenter[piece*2+1]);}
		if(a==1){dx=Fract(rotCenter[piece*2+1]);dy=Fract(rotCenter[piece*2  ]);}
		if(a==2){dx=Fract(rotCenter[piece*2  ]);dy=Fract(rotCenter[piece*2+1]);}
		if(a==3){dx=Fract(rotCenter[piece*2+1]);dy=Fract(rotCenter[piece*2  ]);}
	}

	void fixAA(int ia, float& x, float& y)
	{
		      if(ia==1){x=x+.5/cellSize;y=y-.5/cellSize;}
		else  if(ia==2){x=x-.5/cellSize;y=y-.5/cellSize;}
		else  if(ia==3){x=x-.5/cellSize;y=y+.5/cellSize;}
		else           {x=x+.5/cellSize;y=y+.5/cellSize;}
	}

	void DrawPiece(float x, float y, float pa, int piece, int col)
	{
		int ia=pa;
		float da=Fract(pa);
		// char s[64];
		// snprintf(s,64,"%f",da);
		// stext(s,x+100,y,0xff00ff00);
		float dx0;
		float dx1;
		float dy0;
		float dy1;
		CalcPieceDelta(piece, ia%4   ,dx0,dy0);
		CalcPieceDelta(piece,(ia+1)%4,dx1,dy1);
		fixAA( ia%4   ,dx0,dy0);
		fixAA((ia+1)%4,dx1,dy1);
		float cs=cellSize;
		//da=da*da;
		pa=ia+da;

		float angle=pa*M_PI/2.;
		x+=(dx0*(1.-da)+dx1*da)*cs;
		y+=(dy0*(1.-da)+dy1*da)*cs;

		float dx=cellSize*cos(angle);
		float dy=cellSize*sin(angle);

		{
			MLFig& mi=mlfs[piece];
			{
				g.clear();
				float cx;
				float cy;
				for(int i=0;i<mi.cnt;i++)
				{
					float cj=mi.ml[i].x-rotCenter[piece*2  ];
					float ci=mi.ml[i].y-rotCenter[piece*2+1];
					cx=x+cj*dx+ci*dy+3;
					cy=y-cj*dy+ci*dx+3;

					if('M'==mi.ml[i].cmd)
						g.M(cx,cy);
					else
						g.L(cx,cy);
				}
				g.fin();
				g.rgba32(0x40000000);
				g.fill1();
			}

			g.clear();
			float cx;
			float cy;
			for(int i=0;i<mi.cnt;i++)
			{
				float cj=mi.ml[i].x-rotCenter[piece*2  ];
				float ci=mi.ml[i].y-rotCenter[piece*2+1];
				cx=x+cj*dx+ci*dy;
				cy=y-cj*dy+ci*dx;

				if('M'==mi.ml[i].cmd)
					g.M(cx,cy);
				else
					g.L(cx,cy);
			}
			g.fin();
			g.rgba32(stdcols[col]);
			g.fill1();

			for(int i=0;i<mi.cnt;i++)
			{
				float cj=mi.ml[i].x-rotCenter[piece*2  ];
				float ci=mi.ml[i].y-rotCenter[piece*2+1];
				float ox=cx;
				float oy=cy;
				cx=x+cj*dx+ci*dy;
				cy=y-cj*dy+ci*dx;

				if('M'!=mi.ml[i].cmd)
				{
					g.clear();
					g.M(ox,oy);
					g.L(cx,cy);
					float ddx=cx-ox;
					float ddy=cy-oy;
					float l=sqrt(ddx*ddx+ddy*ddy);
					ddx/=l;
					ddy/=l;
					g.fin();
					float lb=(-ddx*1.5+ddy)*.25+.5;
					g.rgba32(LitColor(stdcols[col],lb));
					//g.stroke();
					g.hairline(ox,oy,cx,cy);
				}
			}
		}
	}

	bool IsLineFull(int y)
	{
		for(int j=0;j<W;j++)
			if((field[y*W+j]==0)||(fieldD[y*W+j]>0))
				return false;
		return true;
	}

	bool foolLines[H];
	int lines;
	int scores;
	int level;

	void RemoveLines()
	{
		for(int l=0;l<H;l++)
		{
			foolLines[l]=IsLineFull(l);
		}
		int i=0;
		int moreLines=0;
		while(i<H)
		{
			if(foolLines[i])
			{
				lines++;
				remLines[lastRemLine].y=i+moreLines;
				remLines[lastRemLine].time=t;
				for(int j=0;j<W;j++)
				{
					remLines[lastRemLine].line[j]=field[i*W+j]&0x0f;
					remLines[lastRemLine].rnd[j]=frand(gseed);
				}

				lastRemLine=(lastRemLine+1)%NREMLINES;
				moreLines++;
				for(int i1=i;i1<(H-1);i1++)
				{
					foolLines[i1]=foolLines[i1+1];
					for(int j=0;j<W;j++)
					{
						field[i1*W+j]=field[(i1+1)*W+j];
						if(field[i1*W+j])field[i1*W+j]+=(moreLines<<20);
						fieldD[i1*W+j]=fieldD[(i1+1)*W+j]+1.0f;
					}
				}
				for(int j=0;j<W;j++)
					field[(H-1)*W+j]=0;
				foolLines[H-1]=false;
			}
			else
				i++;
		}
		if(moreLines)snd.snd_play(sndExplode);

		if(moreLines==1)
		{
			scores+=10;
			//sound->Play(3);
		}
		if(moreLines==2)
		{
			scores+=30;
			//sound->Play(3);
		}
		if(moreLines==3)
		{
			scores+=60;
			//sound->Play(3);
		}
		if(moreLines==4)
		{
			scores+=100;
			//sound->Play(3);
		}
		level=lines/25;
	}

	bool falling;

	float t;

	void UpdateField(float dt)
	{
		if(pa>4.0)
		{
			pa-=4.0;
			newpa-=4;
		}
		{
			float delta=newpx-px;
			float fd=Min(Fabs(delta),dt*MOV_SPEED);
			if(delta>0)px+=fd;
			else px-=fd;
		}
		{
			float delta=newpy-py;
			float fd=Min(Fabs(delta),dt*MOV_SPEED);
			if(delta>0)py+=fd;
			else py-=fd;
		}
		{
			float delta=newpa-pa;
			float fd=Min(Fabs(delta),dt*ROT_SPEED);
			if(delta>0)pa+=fd;
			else pa-=fd;
		}
		if(unPaused)
		{
			if(falling)
			{
				for(int i=0;i<4;i++)
				{
					if(Down())
					{
						snd.snd_play(sndShoot);
						break;
					}
				}
			}
			else
				if(int(t)<int(t+dt))
					Down();
			t+=dt;
			RemoveLines();

			for(int i=0;i<H;i++)
			{
				for(int j=0;j<W;j++)
				{
					fieldD[i*W+j]-=dt*6;
					if(fieldD[i*W+j]<0)
						fieldD[i*W+j]=0;
				}
			}

			{
				for(int i=0;i<W*H;i++)mask[i]=0;
				for(int mfy=1;mfy<H;mfy++)
				{
					for(int mfx=0;mfx<W;mfx++)
					{
						if(field[mfx+(mfy)*W]!=0)
						{
							CBrickSet bs;
							bs.n=0;
							FindPiece(field, fieldD, mask, W, H, mfx, mfy, bs);
							if(bs.n)
							{
								bool shouldfall=true;
								for(int i=0;i<bs.n;i++)
								{
									if(bs.p[i].y==0)
									{
										shouldfall=false;
										break;
									}
									if(!
										(
										(field[bs.p[i].x+(bs.p[i].y-1)*W]==0)||
										(field[bs.p[i].x+(bs.p[i].y-1)*W]==field[bs.p[i].x+(bs.p[i].y)*W])
										)
									)
									{
										shouldfall=false;
										break;
									}
								}
								if(shouldfall)
								{
									for(int i=0;i<bs.n;i++)
									{
										field[bs.p[i].x+(bs.p[i].y)*W]=0;
										mask[bs.p[i].x+(bs.p[i].y)*W]=0;
										fieldD[bs.p[i].x+(bs.p[i].y)*W]=0.0f;
									}
									for(int i=0;i<bs.n;i++)
									{
										field[bs.p[i].x+(bs.p[i].y-1)*W]=bs.p[i].c;
										mask[bs.p[i].x+(bs.p[i].y-1)*W]=1;
										fieldD[bs.p[i].x+(bs.p[i].y-1)*W]=bs.p[i].dy+1.0f;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	bool Down()
	{
		int res=Put(piece,newpx,newpy-1,newpa%4,1,false);
		if(res&9)
		{
			Put(piece,newpx,newpy,newpa%4,col,true);
			NewPiece();
			falling=false;
			return true;
		}
		else
		{
			py=newpy;
			newpy--;
		}
		return false;
	}


	void Key(int key)
	{
		//printf("Key %i\n",key);
		if(key==4001)
		{
			Down();
		}
		else if(key==4002)
		{
			int res=Put(piece,newpx-1,newpy,newpa%4,1,false);
			if(res==0)
			{
				px=newpx;
				newpx--;
			}
		}
		else if(key==4003)
		{
			int res=Put(piece,newpx+1,newpy,newpa%4,1,false);
			if(res==0)
			{
				px=newpx;
				newpx++;
			}
		}
		else if(key==4000)
		{
			int res=Put(piece,newpx,newpy,(newpa+1)%4,1,false);
			if(res==0)
			{
				pa=newpa;
				newpa++;
				snd.snd_play(sndRocket);
			}
		}
		else if(key==4004)
		{
			py=newpy;
			newpy--;
		}
		else if(key==4005)
		{
			py=newpy;
			newpy++;
		}
		//else if(key==4004)
		//{
		//	pa=newpa;
		//	newpa++;
		//}
		//else if(key==4005)
		//{
		//	pa=newpa+4;
		//	newpa+=3;
		//}
		else if(key==4008)
		{
//			Init();
			Restore();
		}
		else if(key==5103)
		{
			piece=(piece+1)%7;
		}
		else if(key==32)
		{
			falling=true;
		}
	}

	void Mouse(int mx, int my, int mb)
	{
		// if(mb)
		// {
		// 	int i=int((my-y)/cellSize);
		// 	int j=int((mx-x)/cellSize);
		// 	if((i>=0)&&(i<H)&&(j>=0)&&(j<W))
		// 	{
		// 		field[j+(H-i-1)*W]=1;
		// 	}
		// }

		if(0)
		{
			int i=int((my-y)/cellSize);
			int j=int((mx-x)/cellSize);
			if((i>=0)&&(i<H)&&(j>=0)&&(j<W))
			{
				i=H-i-1;
				printf("mm: %i %i >",j,i);
				CBrickSet bs;
				bs.n=0;
				for(int i=0;i<W*H;i++)mask[i]=0;
				FindPiece(field,fieldD,mask,W,H,j,i,bs);
				printf("%i\n",bs.n);
			}
		}
	}

	float px;
	float py;
	float pa;
	int newpx;
	int newpy;
	int newpa;

	int Put(int piece, int newpx, int newpy, int ori, int col, bool put)
	{
		int hit=0;
		int dx=offsets[piece*8+ori*2];
		int dy=offsets[piece*8+ori*2+1];
		int boxw=boxes[piece*2  ];
		int boxh=boxes[piece*2+1];
		int orxi[4]={ 0, 1, 0,-1};
		int orxj[4]={ 1, 0,-1, 0};
		int oryi[4]={-1, 0, 1, 0};
		int oryj[4]={ 0, 1, 0,-1};
		for(int i=0;i<boxh;i++)
		{
			for(int j=0;j<boxw;j++)
			{
				if(fig5[piece*25+i*5+j])
				{
					int cx=newpx+i*orxi[ori]+j*orxj[ori]+dx;
					if(cx<0)hit|=HIT_LEFT;
					if(cx>=W)hit|=HIT_RIGHT;
					int cy=newpy+i*oryi[ori]+j*oryj[ori]+dy;
					if(cy<0)hit|=HIT_BOTTOM;
					int index=cy*W+cx;
					if(put)
					{
						if((cx>=0)&&(cx<W)&&(cy>=0)&&(cy<H))
							field[index]=col|(order<<4);
					}
					else
					{
						if((cx>=0)&&(cx<W)&&(cy>=0)&&(cy<H))
						{
							if(field[index])
								hit|=HIT_CELL;
						}
					}
				}
			}
		}
		return hit;
	}

	void RenderGrid()
	{
		g.clear();
		g.M(x,y);
		g.l(W*cellSize,0);
		g.l(0,H*cellSize);
		g.l(-W*cellSize,0);
		g.close();
		g.fin();
		g.rgb(1,1,1);
		g.alpha(.35);
		g.fill1();
		g.width(1,1);
		g.rgb(.9,.4,.3);
		g.stroke();

		g.alpha(.5);
		g.clear();
		for(int i=1;i<H;i++)
		{
			g.M(x,y+i*cellSize);
			g.l(W*cellSize,0);
		}
		for(int i=1;i<W;i++)
		{
			g.M(x+i*cellSize,y);
			g.l(0,H*cellSize);
		}
		g.fin();
		g.width(1.,1.);
		g.rgb(1.,.8,.7);
		g.stroke();

		g.clear();
		for(int i=1;i<H;i++)
		{
			g.M(x+1,y+1+i*cellSize);
			g.l(W*cellSize-1,0);
		}
		for(int i=1;i<W;i++)
		{
			g.M(x+1+i*cellSize,y+1);
			g.l(0,H*cellSize-1);
		}
		g.fin();
		g.width(1.,1.);
		g.rgb(.2,.3,.5);
		g.stroke();
	}

	void RenderField()
	{
		RenderGrid();

		float apx=int(px+256)-256;
		float dpx=px-apx;
		if(px<newpx)apx+=(dpx)*(dpx);
		else apx+=1-(1-dpx)*(1-dpx);

		float apy=int(py+256)-256;
		float dpy=py-apy;
		if(py<newpy)apy+=(dpy)*(dpy);
		else apy+=1-(1-dpy)*(1-dpy);

		if(1)
		{
			//newpa=int(Time())%4;
			//piece=6;
			int ori=newpa%4;


			// for(int i=0;i<W*H;i++)field[i]=0;
			// Put(ori,dx,dy,true);

			bool hit=Put(piece,newpx, newpy, ori, 1, false);
			char s[64];
			snprintf(s,64,"Score: %i", scores);
			stext(s,240,30,0xff00ffff);
		}


		for(int i=0;i<H;i++)
		{
			for(int j=0;j<W;j++)
			{
				if(field[(H-i-1)*W+j])
				{
					DrawChip(j,i, -fieldD[(H-i-1)*W+j], field[(H-i-1)*W+j]&0x0f);
				}
			}
		}
		for(int i=0;i<H;i++)
		{
			for(int j=0;j<W;j++)
			{
				int c=field[(H-i-1)*W+j];
				float d=-fieldD[(H-i-1)*W+j];
				if(c)
				{
					bool tp=true;
					bool bt=true;
					bool lt=true;
					bool rt=true;
					if((j<(W-1))&&(field[j+1+(H-i-1)*W]==c))rt=false;
					if((j>0    )&&(field[j-1+(H-i-1)*W]==c))lt=false;
					if((i<(H-1))&&(field[j+  (H-i-2)*W]==c))tp=false;
					if((i>0    )&&(field[j+  (H-i-0)*W]==c))bt=false;
					float X=x+j*cellSize;
					float Y=y+i*cellSize+int(d*cellSize);

					//g.rgba32(stdcols[c&0xf]);
					//if(!rt)g.hairline(X+cellSize,Y+1,X+cellSize,Y+cellSize);
					//if(!tp)g.hairline(X+1,Y+cellSize,X+cellSize,Y+cellSize);


					if(lt){g.rgba32(LitColor(col,.66));g.Rect(X+1,Y+1,1,cellSize,0x80ffffff);}
					if(bt){g.rgba32(LitColor(col,.20));g.Rect(X+1,Y+1,cellSize,1,0x80ffffff);}
					if(rt){g.rgba32(LitColor(col,.33));g.Rect(X+cellSize,Y+1,1,cellSize,0x80000000);}
					if(tp){g.rgba32(LitColor(col,.80));g.Rect(X+1,Y+cellSize,cellSize,1,0x80000000);}
				}
			}
		}

		// for(int i=0;i<H;i++)
		// {
		// 	if(fieldD[(H-i-1)*W])
		// 	{
		// 		char s[64];
		// 		int ly=i;
		// 		snprintf(s,64,"%3.2f",fieldD[(H-i-1)*W]);
		// 		stext(s,240,y+ly*cellSize,0xff00ff00);
		// 	}
		// }

		for(int i=NREMLINES-1;i>=0;i--)
		{
			float dt=(t-remLines[i].time);
			if(dt<0)
				continue;
			if(dt>2)
				continue;
			int ly=H-1-remLines[i].y;
			for(int j=0;j<W;j++)
			{
				int ci=remLines[i].line[j];
				float rnd=remLines[i].rnd[j];
				float lx=j/(float)W-.5;
				DrawAChip(
					x+j*cellSize+lx*Fabs(lx)*cellSize*20*dt,
					y+ly*cellSize+dt*dt*cellSize*10*(1.5+rnd),dt*(rnd-.5)*8,cellSize/2,stdcols[ci]);
			}
		}
		DrawPiece(x+(apx)*cellSize,y+H*cellSize-apy*cellSize,pa,piece,col);
		DrawPiece(50.5,70.5,nextPA, nextPiece, nextcol);
	}
};

Game game;

SMenu menu;
int menuPauseIndex=-1;

int Strcmp(char* l, char* r)
{
	for(int i=0;i<256;i++)
	{
		if(l[i]==r[i])
		{
			if(l[i]==0)
				return 0;
			continue;
		}
		if(l[i]!=0)return 1;
		if(r[i]!=0)return -1;
	}
}

int MenuCommand0(char* command, int arg)
{
	//printf(" Callback for menu '%s' %i\n", command, arg);
	if(Strcmp(command, "game_pause")==0)
	{
		game.unPaused=!game.unPaused;
	}
	else if(Strcmp(command, "game_new")==0)
	{
		printf("restart %i\n", menuPauseIndex);
		game.Init();
	}
	else if(Strcmp(command, "exit")==0)
	{
		printf("exit %i\n", menuPauseIndex);
		exit(0);
	}
	return 0;
}

int main()
{
	menu.Init();
	menu.bg=0x80ffffff;
	menu.cmdHandler=&MenuCommand0;

	menu.Create()
	.P("=")
		.M("About","about")
		.M("Exit","exit")
	.P("Game")
		.M("New","game_new")
		.M("Pause","game_pause").GetIndex(menuPauseIndex)
	.P("Help")
		.M("How to play","help")
	;

	float tframe0;
	tframe0=Time();
	game.Init();
	game.InitFig();
	while(true)
	{
		SScincEvent ev;
		while(GetScincEvent(ev))
		{
			if(menu.MenuHandleEvent(0,0,640,20,ev))
			{
				//printf(" Event %c%c%c%c handled by menu\n",(ev.type&0xff000000)>>24,(ev.type&0xff0000)>>16,(ev.type&0xff00)>>8,(ev.type&0xff));
				continue;
			}
		}
		if(game.unPaused)
			menu.mi[menuPauseIndex].name="Pause";
		else
			menu.mi[menuPauseIndex].name="Unpause";

		//game.piece=24;
		float tframe1=Time();
		float dt=tframe1-tframe0;
		tframe0=tframe1;
		Background();
		//Present();continue;
		game.UpdateField(dt);
		game.RenderField();

		float T=Time()*1.5;
		//DrawAChip(50,150,T,5,1);

		char s[64];
		int ga=int(game.pa);
		ga=ga%4;
		//snprintf(s,64,"piece %i angle %i",game.piece, ga);
		//stext(s,10,10,0xffffffff);

		if(!game.unPaused)stext("Paused",10,20,0xffff0000);
		if(game.gameOver)
		{
			stext("Game Over",134,30,0xffffff00);
			stext("Game Over",136,30,0xffffff00);
			stext("Game Over",135,31,0xffffff00);
			stext("Game Over",135,29,0xffffff00);
			stext("Game Over",135,30,0xffff0000);
		}
		stext("Next",40,30,0xff00ffff);

		int mb;
		int mx;
		int my;
		GetMouseState(mx, my, mb);
		game.Mouse(mx, my, mb);

		int key;
		int press;
		bool psof=false;
		if(GetKeyEvent(key,press))
		{
			if(press>0)
			{
				if(key=='1'){puts("Explode");snd.snd_play(game.sndExplode);}
				if(key=='2'){puts("Rocket");snd.snd_play(game.sndRocket);}
				if(key=='3'){puts("Shoot");snd.snd_play(game.sndShoot);}
				if(key=='4')
				{
					puts("BreakLine");
					int rl=H-3;//irand(gseed)%(H/2);
					for(int i=0;i<W;i++)
						game.field[rl*W+i]=irand(gseed)%7+1;
				}
				if(key==4010)
				{
					game.unPaused=!game.unPaused;
				}
				else if(key==1000)
				{
					exit(0);
				}
				else if(key==2001)
				{
					game.unPaused=!game.unPaused;
				}
				else if(key=='a')
				{
					offsets[game.piece*8+ga*2]--;
					psof=true;
				}
				else if(key=='d')
				{
					offsets[game.piece*8+ga*2]++;
					psof=true;
				}
				else if(key=='w')
				{
					offsets[game.piece*8+ga*2+1]--;
					psof=true;
				}
				else if(key=='s')
				{
					offsets[game.piece*8+ga*2+1]++;
					psof=true;
				}
				else
				{
					game.Key(key);
				}

				//printf("Key pressed %i %i\n", key, press);
			}
		}

/*
		char sof[128];
		snprintf(sof,64,"% 2i, % 2i, % 2i, % 2i, % 2i, % 2i, % 2i, % 2i,",
			offsets[game.piece*8+0],
			offsets[game.piece*8+1],
			offsets[game.piece*8+2],
			offsets[game.piece*8+3],
			offsets[game.piece*8+4],
			offsets[game.piece*8+5],
			offsets[game.piece*8+6],
			offsets[game.piece*8+7]
		);
		stext(sof,10,20,0xffffffff);
		if(psof)
		{
			puts(sof);
		}
*/
		menu.MenuDraw(g,0,0,640,20);
		snd.Poll();
		Present();
	}
	return 0;
}