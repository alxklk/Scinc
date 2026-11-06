// Pictures drawn with half blocks, two square pixels to a cell in xterm's 256
// colours: a Mandelbrot set to explore, Conway's Game of Life to paint with the
// mouse, and a screen saver (stars or plasma) that starts when the desktop is idle.
//
// The program calls ToyTick from TvAppTick, and the Mb* and Lf* hooks from its own.
#ifndef TVTOYS_H
#define TVTOYS_H

// guarded here too: Scinc reads a header again on every #include, and it can only
// read 16 files in all
#ifndef TV_H
#include "tv.h"
#endif

#define W_MANDEL 41
#define W_LIFE 42

#define PIX_W 240
#define PIX_H 160
#define PIX_N 38400

int toyRng=12345;

int ToyRandom()
{
	toyRng=toyRng*1103515245+12345;
	return (toyRng>>16)&0x7fff;
}

// A cyclic gradient through the 6x6x6 colour cube. keys holds r g b digits 0..5
// per key, "005 055 555"; steps colours are made between each key and the next.
int toyGrad[96];
int toyGradN=0;

void ToyGradient(char* keys, int steps)
{
	int kr[16];
	int kg[16];
	int kb[16];
	int nk=0;
	int i=0;
	while((keys[i]!=0)&&(nk<16))
	{
		if(keys[i]==' ')i++;
		else
		{
			kr[nk]=keys[i]-'0';
			kg[nk]=keys[i+1]-'0';
			kb[nk]=keys[i+2]-'0';
			nk++;
			i=i+3;
		}
	}
	toyGradN=0;
	for(int k=0;k<nk;k++)
	{
		int n=(k+1)%nk;
		for(int s=0;s<steps;s++)
		{
			if(toyGradN<96)
			{
				int r=(kr[k]*(steps-s)+kr[n]*s+steps/2)/steps;
				int g=(kg[k]*(steps-s)+kg[n]*s+steps/2)/steps;
				int b=(kb[k]*(steps-s)+kb[n]*s+steps/2)/steps;
				toyGrad[toyGradN]=16+36*r+6*g+b;
				toyGradN++;
			}
		}
	}
}

// Draws a w*h block of pixels, two rows per cell, from pix (stride PIX_W).
void ToyShow(int* pix, int x0, int y0, int w, int h)
{
	for(int y=0;y+1<h;y=y+2)
	{
		int row=y*PIX_W;
		for(int x=0;x<w;x++)
			TvPix(x0+x,y0+y/2,pix[row+x],pix[row+PIX_W+x]);
	}
}

// ---------------------------------------------------------------- Mandelbrot

int mbWin=-1;
float mbCX=-0.6;
float mbCY=0.;
float mbScale=0.03;    // the width of a pixel
float mbBase=0.03;     // and at zoom 1, where the whole set fits
int mbIter=96;
int mbPal=0;
int mbIt[PIX_N];       // iterations per pixel
int mbPix[PIX_N];      // and their colours
int mbW=0;
int mbH=0;
int mbRow=0;           // the next row to work out; mbH when the picture is done
float mbLastClick=0.;
int mbClickX=-1;
int mbClickY=-1;
int mbDragX=0;
int mbDragY=0;

void MbPalette()
{
	if(mbPal==1)ToyGradient("000 200 520 550 555 335 013",6);
	else if(mbPal==2)ToyGradient("010 050 555 040 020",8);
	else ToyGradient("002 025 055 555 550 520 401 103",5);
}

void MbRestart()
{
	mbRow=0;
	tvDirty=1;
}

void MbOpen()
{
	if(mbWin>=0)
	{
		TvToFront(mbWin);
		return;
	}
	int w=72;
	int h=24;
	if(w>tvW)w=tvW;
	if(h>tvH-2)h=tvH-2;
	mbWin=TvNewCentered(W_MANDEL,"Mandelbrot",w,h,WF_RESIZE);
	if(mbWin<0)return;
	for(int i=0;i<PIX_N;i++)
		mbPix[i]=16;
	mbW=0;
	mbScale=0.;
	MbPalette();
}

int MbColour(int n)
{
	if((n<0)||(n>=mbIter))return 16;
	return toyGrad[n%toyGradN];
}

// Works out rows for up to about 20 ms.
void MbTick()
{
	if(mbWin<0)return;
	if(mbRow>=mbH)return;
	int end=term_ms()+20;
	while((mbRow<mbH)&&(term_ms()<end))
	{
		float ci=mbCY+(mbRow-mbH/2)*mbScale;
		int row=mbRow*PIX_W;
		for(int x=0;x<mbW;x++)
		{
			float cr=mbCX+(x-mbW/2)*mbScale;
			// the main cardioid and the circle to its left are inside
			float qx=cr-0.25;
			float q=qx*qx+ci*ci;
			int n=0;
			if(q*(q+qx)<=0.25*ci*ci)n=mbIter;
			else if((cr+1.)*(cr+1.)+ci*ci<=0.0625)n=mbIter;
			else
			{
				float zr=0.;
				float zi=0.;
				while(n<mbIter)
				{
					float r2=zr*zr;
					float i2=zi*zi;
					if(r2+i2>4.)break;
					zi=2.*zr*zi+ci;
					zr=r2-i2+cr;
					n++;
				}
			}
			mbIt[row+x]=n;
			mbPix[row+x]=MbColour(n);
		}
		mbRow++;
	}
	tvDirty=1;
}

void MbDraw(int win)
{
	TWin& t=tvWin[win];
	int w=t.w-2;
	int h=(t.h-2)*2;
	if(w>PIX_W)w=PIX_W;
	if(h>PIX_H)h=PIX_H;
	if((w!=mbW)||(h!=mbH))
	{
		mbW=w;
		mbH=h;
		mbBase=2.9/mbW;
		if(2.5/mbH>mbBase)mbBase=2.5/mbH;
		if(mbScale==0.)
		{
			mbScale=mbBase;
			mbCX=-0.75;
			mbCY=0.;
		}
		MbRestart();
	}
	ToyShow(&mbPix[0],t.x+1,t.y+1,mbW,mbH);
	char s[80];
	float zoom=mbBase/mbScale;
	if(mbRow<mbH)
		snprintf(&s[0],80," zoom %.3g  iterations %i  drawing %i%% ",zoom,mbIter,mbRow*100/mbH);
	else
		snprintf(&s[0],80," zoom %.3g  iterations %i ",zoom,mbIter);
	TvClip(t.x,t.y,t.x+t.w-2,t.y+t.h);
	TvText(t.x+2,t.y+t.h-1,&s[0],0x1F);
}

// Zooms by f keeping the point at pixel px,py where it is.
void MbZoom(float f, int px, int py)
{
	float wx=mbCX+(px-mbW/2)*mbScale;
	float wy=mbCY+(py-mbH/2)*mbScale;
	mbScale=mbScale/f;
	if(mbScale<1e-15)mbScale=1e-15;
	if(mbScale>mbBase*4)mbScale=mbBase*4;
	mbCX=wx-(px-mbW/2)*mbScale;
	mbCY=wy-(py-mbH/2)*mbScale;
	MbRestart();
}

void MbSetIter(int n)
{
	if(n<16)n=16;
	if(n>4000)n=4000;
	mbIter=n;
	MbRestart();
}

int MbKey(int win, int key)
{
	float step=mbScale*mbW/8;
	if(key==K_LEFT)mbCX=mbCX-step;
	else if(key==K_RIGHT)mbCX=mbCX+step;
	else if(key==K_UP)mbCY=mbCY-step;
	else if(key==K_DOWN)mbCY=mbCY+step;
	else if((key=='+')||(key=='='))
	{
		MbZoom(2.,mbW/2,mbH/2);
		return 1;
	}
	else if(key=='-')
	{
		MbZoom(0.5,mbW/2,mbH/2);
		return 1;
	}
	else if(key==']')
	{
		MbSetIter(mbIter*3/2);
		return 1;
	}
	else if(key=='[')
	{
		MbSetIter(mbIter*2/3);
		return 1;
	}
	else if((key=='p')||(key=='P'))
	{
		mbPal=(mbPal+1)%3;
		MbPalette();
		for(int y=0;y<mbRow;y++)
			for(int x=0;x<mbW;x++)
				mbPix[y*PIX_W+x]=MbColour(mbIt[y*PIX_W+x]);
		tvDirty=1;
		return 1;
	}
	else if(key==K_HOME)
	{
		mbCX=-0.75;
		mbCY=0.;
		mbScale=mbBase;
		mbIter=96;
	}
	else return 0;
	MbRestart();
	return 1;
}

// The wheel zooms at the pointer, a drag moves the picture, a double click zooms in.
int MbMouse(int win, int ev, int x, int y)
{
	int px=x-1;
	int py=(y-1)*2+1;
	if((px<0)||(px>=mbW)||(py<0)||(py>=mbH))return 0;
	if(ev==EV_WHEEL)
	{
		if(evWheel>0)MbZoom(1.5,px,py);
		else MbZoom(1./1.5,px,py);
		return 1;
	}
	if(ev==EV_MDOWN)
	{
		float now=Time();
		if((x==mbClickX)&&(y==mbClickY)&&(now-mbLastClick<0.4))
		{
			MbZoom(2.,px,py);
			mbLastClick=0.;
			return 1;
		}
		mbLastClick=now;
		mbClickX=x;
		mbClickY=y;
		mbDragX=x;
		mbDragY=y;
		TvCaptureMouse(win);
		return 1;
	}
	if(ev==EV_MDRAG)
	{
		mbCX=mbCX-(x-mbDragX)*mbScale;
		mbCY=mbCY-(y-mbDragY)*2*mbScale;
		mbDragX=x;
		mbDragY=y;
		mbClickX=-1;
		MbRestart();
		return 1;
	}
	return 0;
}

// ---------------------------------------------------------------- Game of Life

int lfWin=-1;
int lfAge[PIX_N];      // 0 for a dead cell, else generations alive
int lfNew[PIX_N];
int lfPix[PIX_N];
int lfCol[16];
int lfW=0;
int lfH=0;
int lfRun=0;
int lfGen=0;
int lfPop=0;
int lfSpeed=3;         // index into the rates below
int lfNext=0;
int lfPaint=1;         // a drag draws (1) or erases (0) cells

int LfRate()
{
	if(lfSpeed==0)return 1;
	if(lfSpeed==1)return 2;
	if(lfSpeed==2)return 5;
	if(lfSpeed==3)return 10;
	if(lfSpeed==4)return 20;
	return 40;
}

void LfClear()
{
	for(int i=0;i<PIX_N;i++)
		lfAge[i]=0;
	lfGen=0;
	tvDirty=1;
}

// Puts a pattern in the middle; rows are separated by '/' and O marks a live cell.
void LfPattern(char* p)
{
	int w=0;
	int h=1;
	int cur=0;
	int i=0;
	while(p[i]!=0)
	{
		if(p[i]=='/')
		{
			h++;
			cur=0;
		}
		else
		{
			cur++;
			if(cur>w)w=cur;
		}
		i++;
	}
	int x0=(lfW-w)/2;
	int y0=(lfH-h)/2;
	if(x0<0)x0=0;
	if(y0<0)y0=0;
	int x=x0;
	int y=y0;
	i=0;
	while(p[i]!=0)
	{
		if(p[i]=='/')
		{
			y++;
			x=x0;
		}
		else
		{
			if((p[i]=='O')&&(x<lfW)&&(y<lfH))lfAge[y*PIX_W+x]=1;
			x++;
		}
		i++;
	}
	tvDirty=1;
}

void LfOpen()
{
	if(lfWin>=0)
	{
		TvToFront(lfWin);
		return;
	}
	int w=64;
	int h=24;
	if(w>tvW)w=tvW;
	if(h>tvH-2)h=tvH-2;
	lfWin=TvNewCentered(W_LIFE,"Game of Life",w,h,WF_RESIZE);
	if(lfWin<0)return;
	// young cells are hot, cells that have lived long cool down to blue
	char* c="231 229 227 220 214 208 202 197 162 127 92 57 63 69 75 81";
	int i=0;
	for(int k=0;k<16;k++)
	{
		int v=0;
		while(c[i]==' ')i++;
		while((c[i]>='0')&&(c[i]<='9'))
		{
			v=v*10+c[i]-'0';
			i++;
		}
		lfCol[k]=v;
	}
	LfClear();
	lfW=w-2;
	lfH=(h-2)*2;
	lfRun=0;
	LfPattern("........................O/......................O.O/............OO......OO............OO/...........O...O....OO............OO/OO........O.....O...OO/OO........O...O.OO....O.O/..........O.....O.......O/...........O...O/............OO");
}

// One generation on a board that wraps around at its edges.
void LfStep()
{
	int pop=0;
	for(int y=0;y<lfH;y++)
	{
		int up=((y+lfH-1)%lfH)*PIX_W;
		int mid=y*PIX_W;
		int dn=((y+1)%lfH)*PIX_W;
		for(int x=0;x<lfW;x++)
		{
			int l=(x+lfW-1)%lfW;
			int r=(x+1)%lfW;
			int n=0;
			if(lfAge[up+l]>0)n++;
			if(lfAge[up+x]>0)n++;
			if(lfAge[up+r]>0)n++;
			if(lfAge[mid+l]>0)n++;
			if(lfAge[mid+r]>0)n++;
			if(lfAge[dn+l]>0)n++;
			if(lfAge[dn+x]>0)n++;
			if(lfAge[dn+r]>0)n++;
			int a=lfAge[mid+x];
			int v=0;
			if(n==3)v=a+1;
			else if((n==2)&&(a>0))v=a+1;
			if(v>1000)v=1000;
			if(v>0)pop++;
			lfNew[mid+x]=v;
		}
	}
	for(int y=0;y<lfH;y++)
		for(int x=0;x<lfW;x++)
			lfAge[y*PIX_W+x]=lfNew[y*PIX_W+x];
	lfGen++;
	lfPop=pop;
	tvDirty=1;
}

void LfTick()
{
	if(lfWin<0)return;
	if(!lfRun)return;
	int now=term_ms();
	if(now<lfNext)return;
	lfNext=now+1000/LfRate();
	LfStep();
}

void LfDraw(int win)
{
	TWin& t=tvWin[win];
	int w=t.w-2;
	int h=(t.h-2)*2;
	if(w>PIX_W)w=PIX_W;
	if(h>PIX_H)h=PIX_H;
	if((w!=lfW)||(h!=lfH))
	{
		// cells that are now outside the board die
		for(int y=0;y<PIX_H;y++)
			for(int x=0;x<PIX_W;x++)
				if((x>=w)||(y>=h))lfAge[y*PIX_W+x]=0;
		lfW=w;
		lfH=h;
	}
	int pop=0;
	for(int y=0;y<lfH;y++)
		for(int x=0;x<lfW;x++)
		{
			int a=lfAge[y*PIX_W+x];
			int c=16;
			if(a>0)
			{
				pop++;
				if(a>16)a=16;
				c=lfCol[a-1];
			}
			lfPix[y*PIX_W+x]=c;
		}
	lfPop=pop;
	ToyShow(&lfPix[0],t.x+1,t.y+1,lfW,lfH);
	char s[80];
	char* state="paused";
	if(lfRun)state="running";
	snprintf(&s[0],80," generation %i  cells %i  %s %i/s ",lfGen,lfPop,state,LfRate());
	TvClip(t.x,t.y,t.x+t.w-2,t.y+t.h);
	TvText(t.x+2,t.y+t.h-1,&s[0],0x1F);
}

int LfKey(int win, int key)
{
	if(key==' ')
	{
		lfRun=!lfRun;
		lfNext=0;
	}
	else if((key=='n')||(key=='N')||(key==K_ENTER))
	{
		lfRun=0;
		LfStep();
	}
	else if((key=='c')||(key=='C'))
	{
		lfRun=0;
		LfClear();
	}
	else if((key=='r')||(key=='R'))
	{
		LfClear();
		for(int y=0;y<lfH;y++)
			for(int x=0;x<lfW;x++)
				if(ToyRandom()%4==0)lfAge[y*PIX_W+x]=1;
	}
	else if((key=='+')||(key=='='))
	{
		if(lfSpeed<5)lfSpeed++;
	}
	else if(key=='-')
	{
		if(lfSpeed>0)lfSpeed--;
	}
	else if(key=='1')LfPattern("........................O/......................O.O/............OO......OO............OO/...........O...O....OO............OO/OO........O.....O...OO/OO........O...O.OO....O.O/..........O.....O.......O/...........O...O/............OO");
	else if(key=='2')LfPattern(".OO/OO./.O.");
	else if(key=='3')LfPattern(".O...../...O.../OO..OOO");
	else if(key=='4')LfPattern(".O..O/O..../O...O/OOOO.");
	else return 0;
	tvDirty=1;
	return 1;
}

// Paints with the left button, a press on a live cell erases instead. Without the
// half of the cell (in a terminal) both pixels of a cell are painted.
int lfLX=0;
int lfLY=0;

void LfSet(int px, int py)
{
	for(int k=0;k<2;k++)
	{
		int y=py+k;
		if((k==0)||(evHalf<0))
			if((px>=0)&&(px<lfW)&&(y>=0)&&(y<lfH))
			{
				if(!lfPaint)lfAge[y*PIX_W+px]=0;
				else if(lfAge[y*PIX_W+px]==0)lfAge[y*PIX_W+px]=1;
			}
	}
}

// The pixel under the mouse, the top one of the cell when the half is not known.
void LfPaintTo(int x, int y)
{
	int px=x-1;
	int py=(y-1)*2;
	if(evHalf>0)py++;
	int dx=px-lfLX;
	int dy=py-lfLY;
	int n=dx;
	if(n<0)n=-n;
	int ay=dy;
	if(ay<0)ay=-ay;
	if(ay>n)n=ay;
	if(n==0)n=1;
	for(int k=1;k<=n;k++)
		LfSet(lfLX+(dx*k+n/2)/n,lfLY+(dy*k+n/2)/n);
	lfLX=px;
	lfLY=py;
	tvDirty=1;
}

int LfMouse(int win, int ev, int x, int y)
{
	if(ev==EV_MDOWN)
	{
		int px=x-1;
		int py=(y-1)*2;
		if(evHalf>0)py++;
		if((px<0)||(px>=lfW)||(py<0)||(py>=lfH))return 0;
		lfPaint=(lfAge[py*PIX_W+px]==0);
		TvCaptureMouse(win);
		lfLX=px;
		lfLY=py;
		LfSet(px,py);
		tvDirty=1;
		return 1;
	}
	if(ev==EV_MDRAG)
	{
		LfPaintTo(x,y);
		return 1;
	}
	return 0;
}

// ---------------------------------------------------------------- screen savers

// The savers run full screen after idle time, or in windows. One step a frame moves
// every animation; SvDraw then shows a kind in any block of cells. Windows of the
// same kind share its animation.

#define W_SAVER 46
#define SV_STARS 0
#define SV_PLASMA 1
#define SV_MATRIX 2
#define SV_FLAME 3
#define SV_KINDS 4
#define SV_RANDOM 4    // the setting only: a kind chosen at each start
#define SV_NSTARS 220

int svOn=1;
int svMinutes=5;
int svKind=SV_STARS;   // the setting
int svCur=SV_STARS;    // what the full screen shows
int svActive=0;
int svNext=0;
int svT=0;
int svReady=0;
int svPix[PIX_N];
float svX[SV_NSTARS];
float svY[SV_NSTARS];
float svZ[SV_NSTARS];
int svSin[256];
int svPlasma[96];
int svPlasmaN=0;
float mxHead[TV_MAXW]; // the matrix rain: per column the row of the bright head,
float mxSpeed[TV_MAXW];// how far it falls a frame and its trail length
int mxLen[TV_MAXW];
int mxCh[TV_CELLS];
int mxGreen[8];
int flHeat[38880];     // the fire, PIX_W x (PIX_H+2), heat 0..255
int flCol[32];

void SvStar(int i, float z)
{
	svX[i]=(ToyRandom()%2001-1000)*0.001;
	svY[i]=(ToyRandom()%2001-1000)*0.001;
	svZ[i]=z;
}

void SvDrop(int c, int above)
{
	mxHead[c]=-(ToyRandom()%(above+1));
	mxSpeed[c]=0.25+(ToyRandom()%60)*0.01;
	mxLen[c]=5+ToyRandom()%18;
}

int SvRandomChar()
{
	char* set="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ@#$%&*+=<>?";
	int c=set[ToyRandom()%47];
	return c;
}

// Sets up every animation once.
void SvInit()
{
	if(svReady)return;
	svReady=1;
	toyRng=toyRng+term_ms();
	for(int i=0;i<SV_NSTARS;i++)
		SvStar(i,0.05+(ToyRandom()%1000)*0.001);
	for(int i=0;i<256;i++)
		svSin[i]=int(31.5+31.5*sin(i*3.14159265/128.));
	// the plasma colours, then the Mandelbrot's back as they were
	ToyGradient("005 055 050 550 500 505",10);
	svPlasmaN=toyGradN;
	for(int i=0;i<toyGradN;i++)
		svPlasma[i]=toyGrad[i];
	MbPalette();
	for(int c=0;c<TV_MAXW;c++)
		SvDrop(c,TV_MAXH);
	for(int i=0;i<TV_CELLS;i++)
		mxCh[i]=SvRandomChar();
	char* g="22 28 34 40 46 82 120 194";
	int k=0;
	int i=0;
	while(k<8)
	{
		int v=0;
		while(g[i]==' ')i++;
		while((g[i]>='0')&&(g[i]<='9'))
		{
			v=v*10+g[i]-'0';
			i++;
		}
		mxGreen[k]=v;
		k++;
	}
	for(int n=0;n<38880;n++)
		flHeat[n]=0;
	// black, deep red, red, orange, yellow, white
	for(int n=0;n<32;n++)
	{
		int r=n/3;
		if(r>5)r=5;
		int gr=0;
		if(n>12)gr=(n-12)/3;
		if(gr>5)gr=5;
		int b=0;
		if(n>26)b=(n-26)*5/5;
		if(b>5)b=5;
		flCol[n]=16+36*r+6*gr+b;
	}
	flCol[0]=16;
}

// Moves every animation one frame on, the rain and the fire at screen size.
void SvStep()
{
	SvInit();
	svT++;
	for(int i=0;i<SV_NSTARS;i++)
	{
		svZ[i]=svZ[i]-0.012;
		if(svZ[i]<0.03)SvStar(i,1.);
	}
	for(int c=0;c<tvW;c++)
	{
		mxHead[c]=mxHead[c]+mxSpeed[c];
		if(mxHead[c]-mxLen[c]>tvH)SvDrop(c,tvH/2);
	}
	for(int k=0;k<tvW;k++)
		mxCh[(ToyRandom()%tvH)*TV_MAXW+ToyRandom()%tvW]=SvRandomChar();
	// fire: two rows of embers below the picture, each pixel the average of three
	// under it and one two rows down, cooled a little more in a short picture
	int w=tvW;
	int h=tvH*2;
	int cool=1+384/h;
	for(int x=0;x<w;x++)
	{
		int v=0;
		if(ToyRandom()%10<7)v=230+ToyRandom()%26;
		flHeat[h*PIX_W+x]=v;
		flHeat[(h+1)*PIX_W+x]=v;
	}
	for(int y=0;y<h;y++)
	{
		int row=y*PIX_W;
		int below=row+PIX_W;
		for(int x=0;x<w;x++)
		{
			int l=x-1;
			if(l<0)l=0;
			int r=x+1;
			if(r>=w)r=w-1;
			int v=(flHeat[below+l]+flHeat[below+x]+flHeat[below+r]+flHeat[below+PIX_W+x])/4;
			v=v-ToyRandom()%cool;
			if(v<0)v=0;
			flHeat[row+x]=v;
		}
	}
}

// Draws kind in the cells x0,y0 .. x0+wc-1,y0+hc-1.
void SvDraw(int kind, int x0, int y0, int wc, int hc)
{
	int w=wc;
	int h=hc*2;
	if(kind==SV_MATRIX)
	{
		for(int r=0;r<hc;r++)
			for(int c=0;c<wc;c++)
			{
				float d=mxHead[c%TV_MAXW]-r;
				int at=TV_C256|16;
				int ch=' ';
				if((d>=0.)&&(d<mxLen[c%TV_MAXW]))
				{
					int k=7-int(d*7./mxLen[c%TV_MAXW]);
					if(d<1.)k=7;
					if(k<0)k=0;
					at=TV_C256|(mxGreen[k]<<8)|16;
					if(d<1.)at=TV_C256|(231<<8)|16;
					ch=mxCh[(r%TV_MAXH)*TV_MAXW+c%TV_MAXW];
				}
				TvPut(x0+c,y0+r,ch,at);
			}
		return;
	}
	if(kind==SV_PLASMA)
	{
		int t=svT;
		for(int y=0;y<h;y++)
		{
			int row=y*PIX_W;
			int a=svSin[(y*5-t*2)&255];
			for(int x=0;x<w;x++)
			{
				int v=svSin[(x*4+t)&255]+a+svSin[((x+y)*3+t*3)&255]+svSin[((x*x+y*y*4)/64+t)&255];
				svPix[row+x]=svPlasma[(v/4+t)%svPlasmaN];
			}
		}
	}
	else if(kind==SV_FLAME)
	{
		// the fire burns at screen size, scaled to fit
		int fw=tvW;
		int fh=tvH*2;
		for(int y=0;y<h;y++)
		{
			int src=(y*fh/h)*PIX_W;
			for(int x=0;x<w;x++)
			{
				// squared, so that cooling air goes dark rather than red
				int v=flHeat[src+x*fw/w];
				int n=(v*v)>>10;
				if(n>31)n=31;
				svPix[y*PIX_W+x]=flCol[n];
			}
		}
	}
	else
	{
		for(int y=0;y<h;y++)
			for(int x=0;x<w;x++)
				svPix[y*PIX_W+x]=16;
		for(int i=0;i<SV_NSTARS;i++)
		{
			int sx=int(w/2+svX[i]/svZ[i]*w/2);
			int sy=int(h/2+svY[i]/svZ[i]*w/2);
			if((sx>=0)&&(sx<w)&&(sy>=0)&&(sy<h))
			{
				int b=int(232+(1.-svZ[i])*23.);
				if(b>255)b=255;
				svPix[sy*PIX_W+sx]=b;
				// near stars are two pixels across
				if(svZ[i]<0.3)
					if(sx+1<w)svPix[sy*PIX_W+sx+1]=b;
			}
		}
	}
	ToyShow(&svPix[0],x0,y0,w,h);
}

void SvStart()
{
	SvInit();
	svCur=svKind;
	if(svCur>=SV_KINDS)svCur=ToyRandom()%SV_KINDS;
	svActive=1;
	svNext=0;
	tvScreenTaken=1;
}

int SvWindows()
{
	int n=0;
	for(int i=0;i<TV_MAXWIN;i++)
		if(tvWin[i].used)
			if(tvWin[i].type==W_SAVER)n++;
	return n;
}

char* SvName(int kind)
{
	if(kind==SV_PLASMA)return "Plasma";
	if(kind==SV_MATRIX)return "Matrix";
	if(kind==SV_FLAME)return "Flame";
	return "Stars";
}

// A saver in a window; field a is its kind. Space or 1-4 change it.
void SvOpenWindow(int kind)
{
	if(kind>=SV_KINDS)kind=ToyRandom()%SV_KINDS;
	int w=60;
	int h=20;
	if(w>tvW)w=tvW;
	if(h>tvH-2)h=tvH-2;
	int win=TvNewCentered(W_SAVER,SvName(kind),w,h,WF_RESIZE);
	if(win<0)return;
	tvWin[win].a=kind;
	SvInit();
}

int SvWinKey(int win, int key)
{
	TWin& t=tvWin[win];
	if((key==' ')||(key==K_ENTER))t.a=(t.a+1)%SV_KINDS;
	else if((key>='1')&&(key<='4'))t.a=key-'1';
	else return 0;
	StrCpy(&t.title[0],SvName(t.a),48);
	tvDirty=1;
	return 1;
}

// Starts the full screen saver after svMinutes without input and runs the frames.
void SvTick()
{
	int now=term_ms();
	if(!tvScreenTaken)
	{
		svActive=0;
		if(svOn)
			if(now-tvLastInput>svMinutes*60000)SvStart();
	}
	int windows=SvWindows();
	if((!svActive)&&(windows==0))return;
	if(now<svNext)return;
	svNext=now+50;
	SvStep();
	if(svActive)
	{
		TvClip(0,0,tvW,tvH);
		SvDraw(svCur,0,0,tvW,tvH);
		tvCurX=-1;
		TvFlush();
	}
	else tvDirty=1;
}

#endif // TVTOYS_H
