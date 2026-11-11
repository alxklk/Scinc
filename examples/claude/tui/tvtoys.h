// Pictures drawn with half blocks, two square pixels to a cell in xterm's 256
// colours: a Mandelbrot set to explore, Conway's Game of Life to paint with the
// mouse, and screen savers (stars, plasma, ..., a nyan cat) that start when the
// desktop is idle.
//
// The program calls ToyTick from TvAppTick, and the Mb* and Lf* hooks from its own.
#ifndef TVTOYS_H
#define TVTOYS_H

// guarded here too: Scinc reads a header again on every #include, and it can only
// read 16 files in all
#ifndef TV_H
#include "../tui/tv.h"
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

// The same from pixels of 0xRRGGBB.
void ToyShowRgb(int* pix, int x0, int y0, int w, int h)
{
	for(int y=0;y<h;y+=2)
	{
		int row=y*PIX_W;
		for(int x=0;x<w;x++)
			TvPixRgb(x0+x,y0+y/2,pix[row+x],pix[row+PIX_W+x]);
	}
}

// a+(b-a)*t for each channel of two 0xRRGGBB colours, t 0..256
int ToyMix(int a, int b, int t)
{
	int r=((a>>16)&255)+((((b>>16)&255)-((a>>16)&255))*t>>8);
	int g=((a>>8)&255)+((((b>>8)&255)-((a>>8)&255))*t>>8);
	int bl=(a&255)+(((b&255)-(a&255))*t>>8);
	return (r<<16)|(g<<8)|bl;
}

// Fills dst[0..n-1] with a smooth ramp of 0xRRGGBB through keys written as for
// ToyGradient (three digits 0..5 of red, green, blue, as in xterm's colour cube).
// A cyclic ramp goes on from the last key back to the first.
void ToyRamp(int* dst, int n, char* keys, int cyclic)
{
	int key[9];
	int nk=0;
	int i=0;
	while((keys[i]!=0)&&(nk<8))
	{
		if(keys[i]==' ')i++;
		else
		{
			int c=0;
			for(int k=0;k<3;k++)
			{
				int d=keys[i+k]-'0';
				int v=0;
				if(d>0)v=55+d*40;
				c=(c<<8)|v;
			}
			key[nk]=c;
			nk++;
			i+=3;
		}
	}
	if(cyclic)
	{
		key[nk]=key[0];
		nk++;
	}
	int last=n-1;
	if(cyclic)last=n;
	for(int j=0;j<n;j++)
	{
		int pos=j*(nk-1)*256/last;
		int k=pos>>8;
		int t=pos&255;
		if(k>=nk-1)
		{
			k=nk-2;
			t=256;
		}
		dst[j]=ToyMix(key[k],key[k+1],t);
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
#define SV_REACT 4
#define SV_FLUID 5
#define SV_LAND 6
#define SV_MAZE 7
#define SV_OBJ 8
#define SV_NYAN 9
#define SV_KINDS 10
#define SV_RANDOM 10   // the setting only: a kind chosen at each start
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
int svPlasma[256];     // the plasma's colours, 0xRRGGBB, a cycle
float mxHead[TV_MAXW]; // the matrix rain: per column the row of the bright head,
float mxSpeed[TV_MAXW];// how far it falls a frame and its trail length
int mxLen[TV_MAXW];
int mxCh[TV_CELLS];
int mxGreen[8];
int flHeat[38880];     // the fire, PIX_W x (PIX_H+2), heat 0..255
int flCol[256];        // heat squared to 0xRRGGBB, see SvInit
int svMX=0;            // the last mouse position in a saver window, in its grid
int svMY=0;

// ---- reaction-diffusion
// Gray-Scott: two chemicals on a wrapped grid. U feeds in at rate f, V eats it
// (U+2V -> 3V) and dies at rate k; V diffuses half as fast as U. Each (f,k) grows its
// own pattern. The grid is drawn scaled to the screen; two halves, read one, write
// the other.
#define RD_W 112
#define RD_H 70
#define RD_N 7840
#define RD_PRESETS 6
#define RD_PAL 256
float rdU[15680];
float rdV[15680];
int rdCur=0;           // 0 or RD_N: the half that holds the present
int rdL[RD_W];         // the column to the left, and right, wrapped
int rdR[RD_W];
int rdPreset=0;
int rdAge=0;           // frames since the preset was chosen
float rdF=0.0545;
float rdK=0.062;
int rdPal[1536];       // RD_PRESETS colour ramps of RD_PAL, 0xRRGGBB
float rdXf[PIX_W];     // for drawing: the grid column of each pixel column
float rdYf[PIX_H];

// ---- fluid
// Stam's stable fluids in a closed box: velocity moved along itself, then made
// divergence free by solving for pressure; three dyes ride on it. Vorticity
// confinement puts back the small swirls the coarse grid smooths away. Three jets
// wander and push coloured dye at one another. The grid has a border of one cell.
#define FD_W 64
#define FD_H 40
#define FD_S 66
#define FD_N 2772
float fdU[FD_N];
float fdV[FD_N];
float fdU0[FD_N];
float fdV0[FD_N];
float fdP[FD_N];
float fdDiv[FD_N];
float fdW[FD_N];       // the curl
float fdR[FD_N];
float fdG[FD_N];
float fdB[FD_N];
float fdR0[FD_N];
float fdG0[FD_N];
float fdB0[FD_N];
int fdT=0;
float fdJet[9];        // the jets' colours, r g b each
int fdTone[65];        // see FdTone

// ---- landscape
// A flight over generated land, drawn the voxel way: for each distance, near to
// far, one row of map samples across the view; each sample is a vertical strip on
// its screen column, drawn only above what nearer land already covers. The map is
// 512 x 512 and wraps; its height is made by diamond-square, its colours by
// height, sun from one side and water below a level.
// Near land is sampled between map points (heights and colours blended), far land
// at the nearest one. Far land fades into the sky.
#define LM_SZ 512
#define LM_MASK 511
#define LM_SH 9        // log2 LM_SZ
#define LM_N 262144
#define LM_FAR 240.
#define LM_HS 64       // map heights are 0..255 in 1/LM_HS steps
#define LM_VS 0.35     // map height 0..255 to land height: hills, not needles
int lmH[LM_N];         // height 0..255*LM_HS
int lmC[LM_N];         // colour 0xRRGGBB, lit
float lmTmp[LM_N];
int lmWorld=0;         // which colour set
int lmAge=0;           // frames over this land
float lmX=0.;          // the camera
float lmY=0.;
float lmZ=60.;
float lmA=0.;          // heading, radians
float lmTurn=0.;       // steering from the keys, added to the wander
float lmLift=0.;       // height above the land from the keys
int lmYBuf[PIX_W];     // per column, the topmost row land has covered
int lmCBuf[PIX_W];     // per column, the colour drawn there, -1 for none yet
int lmRBuf[PIX_W];     // per column, the distance step that drew it
int lmRow=0;           // the distance step being drawn
int lmSky[PIX_H];      // the sky's colour per screen row
int lmHaze=0;          // the colour far land fades into
int lmKeyH[6];         // the colour ramp of this world: heights 0..255 at keys
int lmKeyC[6];
int lmSea=0;           // the water level, 0 for none
int lmWater=0;
int lmSkyTop=0;

// ---- labyrinth
// A walk through a generated maze, drawn by casting a ray per screen column to the
// first wall (textured, shaded by side and fading with distance) and the floor and
// ceiling a row at a time. The walker explores as a person would, into dead ends
// and back, until it finds the gate out; then a new maze. MZ_C x MZ_C cells are
// laid out on a map of blocks twice as fine: walls between them are blocks too.
#define MZ_C 11        // cells across and down
#define MZ_CN 121
#define MZ_B 23        // blocks across and down: 2*MZ_C+1
#define MZ_BN 529
#define MZ_TN 1024     // a texture: 32 x 32
#define MZ_LV 16       // shades from near to far
int mzMap[MZ_BN];      // 0 open, 1 wall, 2 the gate
int mzSeen[MZ_CN];     // cells the walker has been to since it last set out
int mzVisit[MZ_CN];    // and in this maze at all, for the map
int mzStack[MZ_CN];    // its way back
int mzSp=0;
int mzTex[MZ_TN];      // the textures as made, before shading
int mzWallS[65536];    // wall, gate: MZ_LV shades, two sides each
int mzFloorS[16384];   // floor, ceiling: MZ_LV shades each
int mzCeilS[16384];
int mzSky[PIX_H];      // the sky by screen row, where there is no ceiling
int mzCeilOn=1;
int mzFog=0;           // the colour far things fade into
float mzFogK=1.;       // shades per block of distance
int mzWorld=0;
float mzX=1.5;         // the walker, in blocks
float mzY=1.5;
float mzA=0.;          // heading, radians
int mzTX=0;            // the cell it walks to
int mzTY=0;
int mzGoal=0;          // the cell by the gate
int mzAtGoal=0;
int mzFade=0;          // 1..MZ_FADE fading out, -MZ_FADE..-1 fading in
int mzManual=0;        // frames left before a key's walk gives way to the walker
int mzMapOn=0;        // the map, in a window only
int mzFull=0;         // drawing the full screen saver

// ---- objects
// One object at a time, turning: a rounded box, three metaballs, a torus, a torus
// knot, a rounded cylinder or a molecule. Made of triangles, drawn by a scanline
// rasteriser of our own with a depth buffer and Phong shading (the normal carried
// across and lit at each pixel), and of balls, drawn as discs with a true
// sphere's depth and normal per pixel. The metals reflect a
// studio around them through a sphere map; the others are lit, and the cheese,
// plaster and hot iron are patterned by where on the object a pixel is.
#define OB_MAXV 9000
#define OB_MAXT 3600
#define OB_SHAPES 6
#define OB_MAXK 200     // balls
#define OB_MATS 7
#define OB_LIFE 500    // frames an object stays
#define OB_FADE 16
float obPX[OB_MAXV];   // the mesh: positions and normals, the object's own axes
float obPY[OB_MAXV];
float obPZ[OB_MAXV];
float obNX[OB_MAXV];
float obNY[OB_MAXV];
float obNZ[OB_MAXV];
int obTA[OB_MAXT];     // triangles: three vertices each
int obTB[OB_MAXT];
int obTC[OB_MAXT];
int obTCol[OB_MAXT];   // a triangle's own colour, -1 for the material's
int obCurCol=-1;       // the colour new triangles get
int obNV=0;
int obNT=0;
float obKX[OB_MAXK];   // the balls: middle, radius, colour (-1 the material's)
float obKY[OB_MAXK];
float obKZ[OB_MAXK];
float obKR[OB_MAXK];
int obKC[OB_MAXK];
int obNK=0;
int obMol=0;           // the molecule shown
int obMolFlat=0;       // it lies flat (or straight): it faces you and rolls
float obMb[9];         // the metaballs' middles
float obMf[5832];      // the field over the metaballs' grid, 18 x 18 x 18
int obMi[5832];        // and whether inside
float obMR=1.;         // the material's colour and kind as chosen; atoms and
float obMG=1.;         // bonds have their own
float obMB=1.;
int obMKind=0;
float obWX[OB_MAXV];   // turned and moved in front of the eye
float obWY[OB_MAXV];
float obWZ[OB_MAXV];
float obVX[OB_MAXV];   // normals turned
float obVY[OB_MAXV];
float obVZ[OB_MAXV];
float obSX[OB_MAXV];   // on the screen
float obSY[OB_MAXV];
float obZB[PIX_N];     // depth per pixel
int obEnv[4096];       // the studio as a 64 x 64 sphere map
float obSpec[256];     // highlight by the cosine to the half vector, 0..255
int obHeat[256];       // the hot iron's colours
// ---- molecules (genmols.py)
// Atoms: element (0 H, 1 C, 2 N, 3 O, 4 Cl), x, y, z in thousandths of an
// angstrom about the middle. Bonds: two atoms, numbered within the molecule,
// and the order (4 aromatic). The molecule's atoms and bonds start at
// obMolA0[n] and obMolB0[n]. The pieces of crystal come last.
#define OB_MOLS 10
#define OB_CRYST 8    // the first piece of crystal
int obMolAtom[376]={
	3,0,-398,0,0,763,199,0,0,-763,199,0,1,890,-159,6,
	1,-468,511,-6,3,-1459,-393,461,0,892,-1042,-641,0,1140,-504,1015,
	0,1668,529,-335,0,-469,1387,649,0,-737,835,-1016,0,-1457,-1163,-133,
	1,0,0,0,0,-1045,-317,22,0,529,-541,-788,0,466,-215,964,
	0,51,1073,-198,2,0,0,296,0,-187,921,-99,0,-704,-623,-99,
	0,891,-298,-99,3,1405,0,0,1,0,0,0,3,-1405,0,0,
	1,-600,0,0,1,600,0,0,0,-1666,0,0,0,1666,0,0,
	1,12,0,0,3,-1212,0,0,0,600,-932,0,0,600,932,0,
	1,-775,1160,0,1,-1392,-91,0,1,-617,-1251,0,1,775,-1160,0,
	1,1392,91,0,1,617,1251,0,0,-1378,2064,0,0,-2476,-162,0,
	0,-1098,-2225,0,0,1378,-2064,0,0,2476,162,0,0,1098,2225,0,
	1,-923,-2675,-860,1,923,-2675,860,1,-2705,-892,-796,1,-2642,892,986,
	1,986,-892,2642,1,-796,892,2705,1,-860,-892,923,1,0,-1784,0,
	1,860,-892,-923,1,-1782,0,63,1,-923,892,-860,1,63,0,1782,
	1,923,892,860,1,-1846,-1784,-1719,1,-986,-892,-2642,1,-63,0,-1782,
	1,796,892,-2705,1,1846,-1784,1719,1,2705,-892,796,1,1782,0,-63,
	1,2642,892,-986,1,-1719,1783,1846,1,-860,2675,923,1,0,1783,0,
	1,860,2675,-923,1,1719,1783,-1846,1,-1230,-3550,0,1,1230,-3550,0,
	1,-2460,-2840,0,1,-2460,-1420,0,1,0,-2840,0,1,0,-1420,0,
	1,2460,-2840,0,1,2460,-1420,0,1,-3689,-710,0,1,-3689,710,0,
	1,-1230,-710,0,1,-1230,710,0,1,1230,-710,0,1,1230,710,0,
	1,3689,-710,0,1,3689,710,0,1,-2460,1420,0,1,-2460,2840,0,
	1,0,1420,0,1,0,2840,0,1,2460,1420,0,1,2460,2840,0,
	1,-1230,3550,0,1,1230,3550,0};
int obMolBond[309]={
	0,1,1,0,2,1,0,1,1,1,2,1,0,3,1,0,4,1,
	0,5,1,1,6,1,1,7,1,2,8,1,0,1,1,0,2,1,
	0,3,1,0,4,1,0,1,1,0,2,1,0,3,1,0,1,2,
	1,2,2,0,1,3,0,2,1,1,3,1,0,1,2,0,2,1,
	0,3,1,0,1,4,1,2,4,2,3,4,3,4,4,4,5,4,
	5,0,4,0,6,1,1,7,1,2,8,1,3,9,1,4,10,1,
	5,11,1,0,7,1,0,13,1,1,7,1,1,17,1,2,9,1,
	2,13,1,3,9,1,3,21,1,4,11,1,4,17,1,5,11,1,
	5,21,1,6,7,1,6,9,1,6,11,1,7,8,1,8,15,1,
	8,19,1,9,10,1,10,15,1,10,23,1,11,12,1,12,19,1,
	12,23,1,13,14,1,14,15,1,15,16,1,16,25,1,17,18,1,
	18,19,1,19,20,1,20,25,1,21,22,1,22,23,1,23,24,1,
	24,25,1,0,2,4,0,4,4,1,4,4,1,6,4,2,3,4,
	3,8,4,3,10,4,4,5,4,5,10,4,5,12,4,6,7,4,
	7,12,4,7,14,4,8,9,4,9,16,4,10,11,4,11,16,4,
	11,18,4,12,13,4,13,18,4,13,20,4,14,15,4,15,20,4,
	16,17,4,17,22,4,18,19,4,19,22,4,19,23,4,20,21,4,
	21,23,4};
int obMolA0[11]={0,3,12,17,21,24,28,32,44,70,94};
int obMolB0[11]={0,2,10,14,17,19,22,25,37,73,103};
char* obMolNames="Water|Ethanol|Methane|Ammonia|Carbon dioxide|Acetylene|Formaldehyde|Benzene|Diamond|Graphene";
char* obMolForms="H2O|C2H6O|CH4|NH3|CO2|C2H2|CH2O|C6H6|C|C";
// ---- end of molecules
int obShape=0;
int obMat=0;
int obAge=0;
int obFade=0;          // as mzFade
int obKind=0;          // 0 metal, 1 plastic, 2 cheese, 3 plaster, 4 hot iron
float obBR=1.;         // the material's colour, 0..1
float obBG=1.;
float obBB=1.;
float obM00=1.;        // the turn
float obM01=0.;
float obM02=0.;
float obM10=0.;
float obM11=1.;
float obM12=0.;
float obM20=0.;
float obM21=0.;
float obM22=1.;
float obT=0.;          // time in seconds, for the iron's glow
float obIX=0.;         // the eye's ray through the pixel being shaded, z 1
float obIY=0.;
float obHW=0.;         // the screen's middle and 1/focal length
float obHH=0.;
float obIF=0.;
float obDist=3.4;      // the eye from the object's middle
float obFitH=1.08;     // how far the object reaches across and up, turning
float obFitV=1.08;

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

// ---------------------------------------------------------------- reaction-diffusion

char* RdName(int n)
{
	if(n==1)return "Mitosis";
	if(n==2)return "Maze";
	if(n==3)return "Worms";
	if(n==4)return "Holes";
	if(n==5)return "Bubbles";
	return "Coral";
}

void RdBlob(int cx, int cy, int r)
{
	for(int dy=-r;dy<=r;dy++)
		for(int dx=-r;dx<=r;dx++)
			if(dx*dx+dy*dy<=r*r)
			{
				int x=(cx+dx+RD_W)%RD_W;
				int y=(cy+dy+RD_H)%RD_H;
				int i=rdCur+y*RD_W+x;
				rdU[i]=0.5;
				rdV[i]=0.25+(ToyRandom()%100)*0.001;
			}
}

// Drops a few round blobs of V into the present.
void RdSeed(int count)
{
	for(int k=0;k<count;k++)
	{
		int cx=ToyRandom()%RD_W;
		int cy=ToyRandom()%RD_H;
		int r=2+ToyRandom()%3;
		RdBlob(cx,cy,r);
	}
}

void RdPreset(int n)
{
	rdPreset=n;
	rdAge=0;
	if(n==0)
	{
		rdF=0.0545;
		rdK=0.062;
	}
	else if(n==1)
	{
		rdF=0.0367;
		rdK=0.0649;
	}
	else if(n==2)
	{
		rdF=0.029;
		rdK=0.057;
	}
	else if(n==3)
	{
		rdF=0.078;
		rdK=0.061;
	}
	else if(n==4)
	{
		rdF=0.039;
		rdK=0.058;
	}
	else
	{
		rdF=0.018;
		rdK=0.051;
	}
}

// Empties the dish and starts preset n from seeds.
void RdReset(int n)
{
	for(int i=0;i<15680;i++)
	{
		rdU[i]=1.;
		rdV[i]=0.;
	}
	rdCur=0;
	RdPreset(n);
	RdSeed(18);
}

void RdInit()
{
	for(int x=0;x<RD_W;x++)
	{
		rdL[x]=(x+RD_W-1)%RD_W;
		rdR[x]=(x+1)%RD_W;
	}
	for(int n=0;n<RD_PRESETS;n++)
	{
		if(n==0)ToyRamp(&rdPal[n*RD_PAL],RD_PAL,"001 013 135 255 554 543",0);
		else if(n==1)ToyRamp(&rdPal[n*RD_PAL],RD_PAL,"000 010 031 152 454 555",0);
		else if(n==2)ToyRamp(&rdPal[n*RD_PAL],RD_PAL,"000 102 214 435 555",0);
		else if(n==3)ToyRamp(&rdPal[n*RD_PAL],RD_PAL,"100 300 520 541 555",0);
		else if(n==4)ToyRamp(&rdPal[n*RD_PAL],RD_PAL,"000 011 033 254 553",0);
		else ToyRamp(&rdPal[n*RD_PAL],RD_PAL,"001 003 014 035 155 555",0);
	}
	RdReset(ToyRandom()%RD_PRESETS);
}

// One step of the reaction from one half of the arrays into the other. The
// Laplacian is the mean of the four neighbours less the cell; U diffuses at 0.8 of
// it, V at 0.4. (At 1 a small seed spreads thin and dies before it reacts.)
void RdStep()
{
	int src=rdCur;
	int dst=RD_N-rdCur;
	float f=rdF;
	float fk=rdF+rdK;
	for(int y=0;y<RD_H;y++)
	{
		int row=src+y*RD_W;
		int up=src+((y+RD_H-1)%RD_H)*RD_W;
		int dn=src+((y+1)%RD_H)*RD_W;
		int out=dst+y*RD_W;
		for(int x=0;x<RD_W;x++)
		{
			int i=row+x;
			float a=rdU[i];
			float b=rdV[i];
			int l=row+rdL[x];
			int r=row+rdR[x];
			float la=(rdU[l]+rdU[r]+rdU[up+x]+rdU[dn+x])*0.25-a;
			float lb=(rdV[l]+rdV[r]+rdV[up+x]+rdV[dn+x])*0.25-b;
			float abb=a*b*b;
			rdU[out+x]=a+0.8*la-abb+f*(1.-a);
			rdV[out+x]=b+0.4*lb+abb-fk*b;
		}
	}
	rdCur=dst;
}

// A few steps a frame; a new preset every 40 seconds or so, grown on the old
// pattern, and fresh seeds when the dish has died out.
void RdFrame()
{
	for(int k=0;k<6;k++)
		RdStep();
	rdAge++;
	if(rdAge%50==0)
	{
		int live=0;
		for(int i=0;i<RD_N;i+=7)
			if(rdV[rdCur+i]>0.1)live++;
		if(live<RD_N/700)RdSeed(12);
	}
	if(rdAge>800)
	{
		RdPreset((rdPreset+1)%RD_PRESETS);
		RdSeed(6);
	}
}

void RdDraw(int w, int h)
{
	for(int x=0;x<w;x++)
		rdXf[x]=x*RD_W*1./w;
	for(int y=0;y<h;y++)
		rdYf[y]=y*RD_H*1./h;
	int pal=rdPreset*RD_PAL;
	for(int y=0;y<h;y++)
	{
		float fy=rdYf[y];
		int y0=int(fy);
		float ty=fy-y0;
		int r0=rdCur+y0*RD_W;
		int r1=rdCur+((y0+1)%RD_H)*RD_W;
		int row=y*PIX_W;
		for(int x=0;x<w;x++)
		{
			float fx=rdXf[x];
			int x0=int(fx);
			float tx=fx-x0;
			int x1=rdR[x0];
			float top=rdV[r0+x0]+(rdV[r0+x1]-rdV[r0+x0])*tx;
			float bot=rdV[r1+x0]+(rdV[r1+x1]-rdV[r1+x0])*tx;
			int c=int((top+(bot-top)*ty)*(RD_PAL/0.42));
			if(c>=RD_PAL)c=RD_PAL-1;
			if(c<0)c=0;
			svPix[row+x]=rdPal[pal+c];
		}
	}
}

// ---------------------------------------------------------------- fluid

void FdInit()
{
	for(int i=0;i<FD_N;i++)
	{
		fdU[i]=0.;
		fdV[i]=0.;
		fdP[i]=0.;
		fdW[i]=0.;
		fdR[i]=0.;
		fdG[i]=0.;
		fdB[i]=0.;
	}
	fdT=0;
	for(int i=0;i<=64;i++)
		fdTone[i]=int(255.*(1.-exp(-2.2*i/32.)));
	// orange, azure, magenta
	fdJet[0]=1.;
	fdJet[1]=0.45;
	fdJet[2]=0.05;
	fdJet[3]=0.05;
	fdJet[4]=0.55;
	fdJet[5]=1.;
	fdJet[6]=0.85;
	fdJet[7]=0.1;
	fdJet[8]=0.75;
}

// Pushes the fluid at grid point x,y (inside the border) by dx,dy and adds dye.
void FdPush(float x, float y, float dx, float dy, float r, float g, float b)
{
	int cx=int(x);
	int cy=int(y);
	for(int j=cy-3;j<=cy+3;j++)
		for(int i=cx-3;i<=cx+3;i++)
			if((i>=1)&&(i<=FD_W)&&(j>=1)&&(j<=FD_H))
			{
				float ex=i-x;
				float ey=j-y;
				float d=ex*ex+ey*ey;
				if(d<9.)
				{
					float a=1.-d/9.;
					int k=j*FD_S+i;
					fdU[k]=fdU[k]+dx*a;
					fdV[k]=fdV[k]+dy*a;
					fdR[k]=fdR[k]+r*a;
					fdG[k]=fdG[k]+g*a;
					fdB[k]=fdB[k]+b*a;
				}
			}
}

// The walls: no flow through them, so the border copies its neighbour with the
// velocity across the wall reversed (s 1 for u, 2 for v, 0 for the rest).
void FdBound(float* a, int s)
{
	for(int i=1;i<=FD_W;i++)
	{
		float t=a[FD_S+i];
		float b=a[FD_H*FD_S+i];
		if(s==2)
		{
			t=-t;
			b=-b;
		}
		a[i]=t;
		a[(FD_H+1)*FD_S+i]=b;
	}
	for(int j=1;j<=FD_H;j++)
	{
		float l=a[j*FD_S+1];
		float r=a[j*FD_S+FD_W];
		if(s==1)
		{
			l=-l;
			r=-r;
		}
		a[j*FD_S]=l;
		a[j*FD_S+FD_W+1]=r;
	}
	a[0]=(a[1]+a[FD_S])*0.5;
	a[FD_W+1]=(a[FD_W]+a[FD_S+FD_W+1])*0.5;
	a[(FD_H+1)*FD_S]=(a[FD_H*FD_S]+a[(FD_H+1)*FD_S+1])*0.5;
	a[(FD_H+1)*FD_S+FD_W+1]=(a[(FD_H+1)*FD_S+FD_W]+a[FD_H*FD_S+FD_W+1])*0.5;
}

// The grid position a cell's content came from a step ago, read with bilinear
// weights; FdBack sets these for FdRead.
int fdI0=0;
float fdS=0.;
float fdT1=0.;

void FdBack(int i, int j)
{
	int k=j*FD_S+i;
	float x=i-fdU0[k];
	float y=j-fdV0[k];
	if(x<0.5)x=0.5;
	if(x>FD_W+0.5)x=FD_W+0.5;
	if(y<0.5)y=0.5;
	if(y>FD_H+0.5)y=FD_H+0.5;
	int x0=int(x);
	int y0=int(y);
	fdS=x-x0;
	fdT1=y-y0;
	fdI0=y0*FD_S+x0;
}

float FdRead(float* a)
{
	int k=fdI0;
	float top=a[k]+(a[k+1]-a[k])*fdS;
	float bot=a[k+FD_S]+(a[k+FD_S+1]-a[k+FD_S])*fdS;
	return top+(bot-top)*fdT1;
}

void FdFrame()
{
	fdT++;
	float t=fdT;
	// the jets: each wanders the box, aims at the middle give or take, and
	// pauses now and then
	for(int n=0;n<3;n++)
	{
		float ph=n*2.094;
		if(sin(t*0.011+ph*1.7)<-0.55)continue;
		float x=FD_W*0.5+FD_W*0.36*sin(t*0.0071*(n+2)+ph);
		float y=FD_H*0.5+FD_H*0.34*sin(t*0.0053*(n+3)+ph*2.);
		float ax=FD_W*0.5-x;
		float ay=FD_H*0.5-y;
		float a=atan2(ay,ax)+1.1*sin(t*0.017*(n+1)+ph);
		float sp=0.22;
		FdPush(x,y,cos(a)*sp,sin(a)*sp,fdJet[n*3]*0.12,fdJet[n*3+1]*0.12,fdJet[n*3+2]*0.12);
	}
	// vorticity confinement
	for(int j=1;j<=FD_H;j++)
		for(int i=1;i<=FD_W;i++)
		{
			int k=j*FD_S+i;
			float c=(fdV[k+1]-fdV[k-1]-fdU[k+FD_S]+fdU[k-FD_S])*0.5;
			fdW[k]=c;
			// its size, in fdDiv until the pressure needs that
			if(c<0.)c=-c;
			fdDiv[k]=c;
		}
	for(int j=2;j<FD_H;j++)
		for(int i=2;i<FD_W;i++)
		{
			int k=j*FD_S+i;
			float nx=(fdDiv[k+1]-fdDiv[k-1])*0.5;
			float ny=(fdDiv[k+FD_S]-fdDiv[k-FD_S])*0.5;
			float len=sqrt(nx*nx+ny*ny)+0.00001;
			float e=0.12*fdW[k]/len;
			fdU[k]=fdU[k]+ny*e;
			fdV[k]=fdV[k]-nx*e;
		}
	// carry the velocity along itself, a little damped
	for(int k=0;k<FD_N;k++)
	{
		fdU0[k]=fdU[k];
		fdV0[k]=fdV[k];
	}
	for(int j=1;j<=FD_H;j++)
		for(int i=1;i<=FD_W;i++)
		{
			FdBack(i,j);
			int k=j*FD_S+i;
			fdU[k]=FdRead(&fdU0[0])*0.996;
			fdV[k]=FdRead(&fdV0[0])*0.996;
		}
	FdBound(&fdU[0],1);
	FdBound(&fdV[0],2);
	// pressure: Gauss-Seidel from last frame's answer, then subtract its gradient
	for(int j=1;j<=FD_H;j++)
		for(int i=1;i<=FD_W;i++)
		{
			int k=j*FD_S+i;
			fdDiv[k]=-0.5*(fdU[k+1]-fdU[k-1]+fdV[k+FD_S]-fdV[k-FD_S]);
		}
	for(int it=0;it<12;it++)
	{
		for(int j=1;j<=FD_H;j++)
		{
			int k=j*FD_S+1;
			for(int i=1;i<=FD_W;i++)
			{
				fdP[k]=(fdDiv[k]+fdP[k-1]+fdP[k+1]+fdP[k-FD_S]+fdP[k+FD_S])*0.25;
				k++;
			}
		}
		FdBound(&fdP[0],0);
	}
	for(int j=1;j<=FD_H;j++)
		for(int i=1;i<=FD_W;i++)
		{
			int k=j*FD_S+i;
			fdU[k]=fdU[k]-0.5*(fdP[k+1]-fdP[k-1]);
			fdV[k]=fdV[k]-0.5*(fdP[k+FD_S]-fdP[k-FD_S]);
		}
	FdBound(&fdU[0],1);
	FdBound(&fdV[0],2);
	// the dyes ride on the new velocity and slowly fade
	for(int k=0;k<FD_N;k++)
	{
		fdU0[k]=fdU[k];
		fdV0[k]=fdV[k];
		fdR0[k]=fdR[k];
		fdG0[k]=fdG[k];
		fdB0[k]=fdB[k];
	}
	for(int j=1;j<=FD_H;j++)
		for(int i=1;i<=FD_W;i++)
		{
			FdBack(i,j);
			int k=j*FD_S+i;
			fdR[k]=FdRead(&fdR0[0])*0.988;
			fdG[k]=FdRead(&fdG0[0])*0.988;
			fdB[k]=FdRead(&fdB0[0])*0.988;
		}
	FdBound(&fdR[0],0);
	FdBound(&fdG[0],0);
	FdBound(&fdB[0],0);
}

// One dye, 0 and up, as a channel 0..255: 1-exp(-2.2v), so that thin dye still
// shows and thick dye saturates instead of clipping: fdTone's 64 steps up to v=2,
// read between its entries.
int FdTone(float v)
{
	if(v<=0.)return 0;
	float f=v*32.;
	int i=int(f);
	if(i>=64)return fdTone[64];
	return fdTone[i]+int((fdTone[i+1]-fdTone[i])*(f-i));
}

void FdDraw(int w, int h)
{
	for(int x=0;x<w;x++)
		rdXf[x]=0.5+x*FD_W*1./w;
	for(int y=0;y<h;y++)
		rdYf[y]=0.5+y*FD_H*1./h;
	for(int y=0;y<h;y++)
	{
		float fy=rdYf[y];
		int y0=int(fy);
		float ty=fy-y0;
		int row=y*PIX_W;
		for(int x=0;x<w;x++)
		{
			float fx=rdXf[x];
			int x0=int(fx);
			float tx=fx-x0;
			int k=y0*FD_S+x0;
			float w00=(1.-tx)*(1.-ty);
			float w10=tx*(1.-ty);
			float w01=(1.-tx)*ty;
			float w11=tx*ty;
			float r=fdR[k]*w00+fdR[k+1]*w10+fdR[k+FD_S]*w01+fdR[k+FD_S+1]*w11;
			float g=fdG[k]*w00+fdG[k+1]*w10+fdG[k+FD_S]*w01+fdG[k+FD_S+1]*w11;
			float b=fdB[k]*w00+fdB[k+1]*w10+fdB[k+FD_S]*w01+fdB[k+FD_S+1]*w11;
			svPix[row+x]=(FdTone(r)<<16)|(FdTone(g)<<8)|FdTone(b);
		}
	}
}

// ---------------------------------------------------------------- landscape

char* LmName(int n)
{
	if(n==1)return "Islands";
	if(n==2)return "Glacier";
	if(n==3)return "Dusk";
	return "Red desert";
}

// A colour scaled by l/256, clipped.
int LmLight(int c, int l)
{
	int r=((c>>16)&255)*l>>8;
	int g=((c>>8)&255)*l>>8;
	int b=(c&255)*l>>8;
	if(r>255)r=255;
	if(g>255)g=255;
	if(b>255)b=255;
	return (r<<16)|(g<<8)|b;
}

void LmKey(int i, int h, int c)
{
	lmKeyH[i]=h;
	lmKeyC[i]=c;
}

// The world's colours: a ramp of six keys by height, the sky, the haze, the water.
void LmColours(int n)
{
	lmSea=0;
	if(n==1)
	{
		LmKey(0,0,0xC8B070);
		LmKey(1,40,0x5C8A3A);
		LmKey(2,110,0x2F5E2A);
		LmKey(3,170,0x6B5B45);
		LmKey(4,215,0x8E8C88);
		LmKey(5,255,0xF4F6FA);
		lmSea=70;
		lmWater=0x1E5A8C;
		lmSkyTop=0x2E6FD0;
		lmHaze=0xB8D4EC;
	}
	else if(n==2)
	{
		LmKey(0,0,0x40566E);
		LmKey(1,60,0x7E93A8);
		LmKey(2,120,0xAFC4D6);
		LmKey(3,170,0xD8E6F2);
		LmKey(4,215,0xF2F8FF);
		LmKey(5,255,0xFFFFFF);
		lmSea=55;
		lmWater=0x1C3A56;
		lmSkyTop=0x0E2650;
		lmHaze=0x9FB6D2;
	}
	else if(n==3)
	{
		LmKey(0,0,0x2A1E3A);
		LmKey(1,60,0x4A2E52);
		LmKey(2,120,0x7A3F58);
		LmKey(3,170,0xB0605A);
		LmKey(4,215,0xE09A6A);
		LmKey(5,255,0xFFD9A0);
		lmSkyTop=0x1A1440;
		lmHaze=0xE8946A;
	}
	else
	{
		LmKey(0,0,0x3A1408);
		LmKey(1,60,0x7A2E12);
		LmKey(2,120,0xB04A1C);
		LmKey(3,170,0xD27A38);
		LmKey(4,215,0xE8B070);
		LmKey(5,255,0xF6DEB0);
		lmSkyTop=0x5A2A20;
		lmHaze=0xE6A880;
	}
}

int LmRamp(int h)
{
	int i=0;
	while((i<4)&&(h>lmKeyH[i+1]))i++;
	int t=(h-lmKeyH[i])*256/(lmKeyH[i+1]-lmKeyH[i]);
	if(t>256)t=256;
	return ToyMix(lmKeyC[i],lmKeyC[i+1],t);
}

float LmRand()
{
	return (ToyRandom()%2001-1000)*0.001;
}

// New land for world n.
void LmMake(int n)
{
	lmWorld=n;
	lmAge=0;
	LmColours(n);
	toyRng=toyRng+term_ms();
	for(int i=0;i<LM_N;i++)
		lmTmp[i]=0.;
	int step=LM_SZ;
	float sc=0.7;      // the widest step weaker, so the hills keep their height
	while(step>1)
	{
		int half=step/2;
		for(int y=0;y<LM_SZ;y+=step)
			for(int x=0;x<LM_SZ;x+=step)
			{
				int x1=(x+step)&LM_MASK;
				int y1=(y+step)&LM_MASK;
				float a=lmTmp[y*LM_SZ+x]+lmTmp[y*LM_SZ+x1]+lmTmp[y1*LM_SZ+x]+lmTmp[y1*LM_SZ+x1];
				lmTmp[(y+half)*LM_SZ+x+half]=a*0.25+LmRand()*sc;
			}
		for(int y=0;y<LM_SZ;y+=half)
		{
			int x=(y+half)%step;
			while(x<LM_SZ)
			{
				float a=lmTmp[y*LM_SZ+((x+half)&LM_MASK)]+lmTmp[y*LM_SZ+((x-half)&LM_MASK)];
				a=a+lmTmp[((y+half)&LM_MASK)*LM_SZ+x]+lmTmp[((y-half)&LM_MASK)*LM_SZ+x];
				lmTmp[y*LM_SZ+x]=a*0.25+LmRand()*sc;
				x+=step;
			}
		}
		step=half;
		if(step==LM_SZ/2)sc=1.;
		else sc=sc*0.52;
	}
	// smoothed once, then stretched to 0..255*LM_HS
	for(int pass=0;pass<1;pass++)
	{
		for(int y=0;y<LM_SZ;y++)
			for(int x=0;x<LM_SZ;x++)
			{
				int i=y*LM_SZ+x;
				float a=lmTmp[i]*4.+lmTmp[y*LM_SZ+((x+1)&LM_MASK)]+lmTmp[y*LM_SZ+((x-1)&LM_MASK)];
				a=a+lmTmp[((y+1)&LM_MASK)*LM_SZ+x]+lmTmp[((y-1)&LM_MASK)*LM_SZ+x];
				lmH[i]=int(a*125000.);
			}
		for(int i=0;i<LM_N;i++)
			lmTmp[i]=lmH[i]*0.000001;
	}
	float lo=1000.;
	float hi=-1000.;
	for(int i=0;i<LM_N;i++)
	{
		if(lmTmp[i]<lo)lo=lmTmp[i];
		if(lmTmp[i]>hi)hi=lmTmp[i];
	}
	float k=(255.*LM_HS)/(hi-lo+0.000001);
	for(int i=0;i<LM_N;i++)
		lmH[i]=int((lmTmp[i]-lo)*k);
	// colours: the ramp, lit by the slope towards the sun (down and to the left)
	for(int y=0;y<LM_SZ;y++)
		for(int x=0;x<LM_SZ;x++)
		{
			int i=y*LM_SZ+x;
			int h=lmH[i]/LM_HS;
			int d=(lmH[i]-lmH[((y+2)&LM_MASK)*LM_SZ+((x-2)&LM_MASK)])*9/LM_HS;
			int l=230+d;
			if(l<90)l=90;
			if(l>330)l=330;
			int c=LmLight(LmRamp(h),l);
			if(h<lmSea)
			{
				// water: flat, darker with depth
				c=ToyMix(lmWater,0x000000,(lmSea-h)*3);
				lmH[i]=lmSea*LM_HS;
			}
			lmC[i]=c;
		}
	lmX=LM_SZ*0.5;
	lmY=LM_SZ*0.5;
	lmZ=lmH[(LM_SZ/2)*LM_SZ+(LM_SZ/2)]*(LM_VS/LM_HS)+20.;
}

// The land's height (in LM_VS units) under x,y, between map points.
float LmHeightAt(float x, float y)
{
	float mx=x+5120.;
	float my=y+5120.;
	int ix=int(mx);
	int iy=int(my);
	float fx=mx-ix;
	float fy=my-iy;
	int r0=(iy&LM_MASK)*LM_SZ;
	int r1=((iy+1)&LM_MASK)*LM_SZ;
	int x0=ix&LM_MASK;
	int x1=(ix+1)&LM_MASK;
	float a=lmH[r0+x0]+(lmH[r0+x1]-lmH[r0+x0])*fx;
	float b=lmH[r1+x0]+(lmH[r1+x1]-lmH[r1+x0])*fx;
	return (a+(b-a)*fy)*(LM_VS/LM_HS);
}

// Flies on: forward, the heading wandering slowly, the height eased towards the
// highest land a little ahead plus a margin. A new world every two minutes or so.
void LmFrame()
{
	lmAge++;
	if(lmAge>2400)LmMake((lmWorld+1)%4);
	float t=lmAge;
	lmA=lmA+0.0045*sin(t*0.0037)+0.003*sin(t*0.011+1.)+lmTurn;
	float dx=cos(lmA);
	float dy=sin(lmA);
	lmX=lmX+dx*0.8;
	lmY=lmY+dy*0.8;
	if(lmX<0.)lmX=lmX+LM_SZ;
	if(lmX>=LM_SZ)lmX=lmX-LM_SZ;
	if(lmY<0.)lmY=lmY+LM_SZ;
	if(lmY>=LM_SZ)lmY=lmY-LM_SZ;
	float top=0.;
	for(int k=0;k<6;k++)
	{
		float h=LmHeightAt(lmX+dx*k*6.,lmY+dy*k*6.);
		if(h>top)top=h;
	}
	float want=top+14.+lmLift;
	lmZ=lmZ+(want-lmZ)*0.04;
	if(lmZ<LmHeightAt(lmX,lmY)+4.)lmZ=LmHeightAt(lmX,lmY)+4.;
}

// Land from row top down to yb on column x: colour c at the top, fading to the
// colour of the land drawn below it by the step before, so that the steps
// between one distance and the next don't show as bands. Below a ridge (nearer
// land drawn earlier) it stays solid, keeping the edge sharp.
void LmStrip(int x, int top, int yb, int c)
{
	int p=lmCBuf[x];
	if(lmRBuf[x]!=lmRow-1)p=-1;
	lmYBuf[x]=top;
	lmCBuf[x]=c;
	lmRBuf[x]=lmRow;
	if(p<0)
	{
		for(int y=top;y<yb;y++)
			svPix[y*PIX_W+x]=c;
		return;
	}
	int n=yb-top;
	int r=(c>>16)<<16;
	int g=((c>>8)&255)<<16;
	int b=(c&255)<<16;
	int dr=(((p>>16)<<16)-r)/n;
	int dg=((((p>>8)&255)<<16)-g)/n;
	int db=(((p&255)<<16)-b)/n;
	for(int y=top;y<yb;y++)
	{
		svPix[y*PIX_W+x]=((r>>16)<<16)|((g>>16)<<8)|(b>>16);
		r+=dr;
		g+=dg;
		b+=db;
	}
}

void LmDraw(int w, int h)
{
	int horizon=h*2/5;
	float scale=h*1.1;
	for(int y=0;y<h;y++)
	{
		int t=y*256/(horizon+1);
		if(t>256)t=256;
		lmSky[y]=ToyMix(lmSkyTop,lmHaze,t);
	}
	for(int x=0;x<w;x++)
	{
		lmYBuf[x]=h;
		lmCBuf[x]=-1;
		lmRBuf[x]=-2;
	}
	lmRow=0;
	float sn=sin(lmA);
	float cs=cos(lmA);
	// heights below are map heights times 256; zq is the camera's height so
	float zq=lmZ*LM_HS*256./LM_VS;
	// map points per screen column at distance z are 2z/w: below a half, blend
	float near=w*0.25;
	float z=1.5;
	while(z<LM_FAR)
	{
		// the row of samples at distance z spans the view, 90 degrees wide; in
		// 16.16 fixed point, kept positive
		float lx=lmX+cs*z+sn*z+5120.;
		float ly=lmY+sn*z-cs*z+5120.;
		int px=int(lx*65536.);
		int py=int(ly*65536.);
		int sx=int((-2.*sn*z)*65536./w);
		int sy=int((2.*cs*z)*65536./w);
		float inv=scale/z*LM_VS/(LM_HS*256.);
		int fog=int(z*z*256./(LM_FAR*LM_FAR));
		if(z<near)
		{
			for(int x=0;x<w;x++)
			{
				int ix=(px>>16)&LM_MASK;
				int iy=(py>>16)&LM_MASK;
				int fx=(px>>8)&255;
				int fy=(py>>8)&255;
				int i=(iy<<LM_SH)+ix;
				int i1=(iy<<LM_SH)+((ix+1)&LM_MASK);
				int r1=((iy+1)&LM_MASK)<<LM_SH;
				int j0=r1+ix;
				int j1=r1+((ix+1)&LM_MASK);
				int a=(lmH[i]<<8)+(lmH[i1]-lmH[i])*fx;
				int b=(lmH[j0]<<8)+(lmH[j1]-lmH[j0])*fx;
				int top=int((zq-a-((b-a)>>8)*fy)*inv)+horizon;
				if(top<0)top=0;
				int yb=lmYBuf[x];
				if(top<yb)
				{
					int c=ToyMix(ToyMix(lmC[i],lmC[i1],fx),ToyMix(lmC[j0],lmC[j1],fx),fy);
					LmStrip(x,top,yb,ToyMix(c,lmHaze,fog));
				}
				px+=sx;
				py+=sy;
			}
		}
		else
		{
			for(int x=0;x<w;x++)
			{
				int i=(((py>>16)&LM_MASK)<<LM_SH)+((px>>16)&LM_MASK);
				int top=int((zq-(lmH[i]<<8))*inv)+horizon;
				if(top<0)top=0;
				int yb=lmYBuf[x];
				if(top<yb)
				{
					LmStrip(x,top,yb,ToyMix(lmC[i],lmHaze,fog));
				}
				px+=sx;
				py+=sy;
			}
		}
		z=z+0.15+z*0.025;
		lmRow++;
	}
	for(int x=0;x<w;x++)
		for(int y=0;y<lmYBuf[x];y++)
			svPix[y*PIX_W+x]=lmSky[y];
}

// ---------------------------------------------------------------- labyrinth

#define MZ_FADE 24
#define MZ_WORLDS 3

char* MzName(int n)
{
	if(n==1)return "Hedge garden";
	if(n==2)return "Station";
	return "Stone halls";
}

float MzAbs(float v)
{
	if(v<0.)return -v;
	return v;
}

// A colour with some noise: each channel moved by up to +-n.
int MzJitter(int c, int n)
{
	int d=ToyRandom()%(2*n+1)-n;
	return LmLight(c,256+d*256/96);
}

// The textures, written into mzTex, 32 x 32, texel (x,y) at y*32+x.

// Stone blocks in courses of eight, each block its own shade, mortar between.
void MzStone(int base, int mortar)
{
	for(int y=0;y<32;y+=8)
	{
		int off=(y/8%2)*8+ToyRandom()%4;
		int bx=0;
		while(bx<32)
		{
			int bw=10+ToyRandom()%9;
			int bc=MzJitter(base,22);
			for(int yy=y;yy<y+8;yy++)
				for(int xx=bx;xx<bx+bw;xx++)
				{
					int c=MzJitter(bc,8);
					// light from above: the top edge brighter, the bottom darker
					if(yy==y+1)c=LmLight(c,290);
					if(yy==y+6)c=LmLight(c,210);
					if((yy==y+7)||(xx==bx))c=MzJitter(mortar,6);
					mzTex[yy*32+((xx+off)&31)]=c;
				}
			bx+=bw;
		}
	}
}

// Leaves: dark green with clusters of lighter and darker ones.
void MzHedge()
{
	for(int i=0;i<MZ_TN;i++)
		mzTex[i]=MzJitter(0x2C5E26,10);
	for(int k=0;k<150;k++)
	{
		int x=ToyRandom()%32;
		int y=ToyRandom()%32;
		int c=0x4F8A3A;
		int r=ToyRandom()%3;
		if(r==1)c=0x1C3F19;
		if(r==2)c=0x6FA34C;
		c=MzJitter(c,14);
		mzTex[y*32+x]=c;
		mzTex[y*32+((x+1)&31)]=LmLight(c,230);
		mzTex[((y+1)&31)*32+x]=LmLight(c,200);
	}
}

// Metal panels 16 wide with seams, rivets in the corners, a light strip across.
void MzPanels()
{
	for(int y=0;y<32;y++)
		for(int x=0;x<32;x++)
		{
			int c=MzJitter(0x6A7783,4);
			c=LmLight(c,240+(y%16)*3);
			if(((x%16)==0)||((y%16)==0))c=0x2E353C;
			if(((x%16)==15)||((y%16)==15))c=0x8D9AA6;
			if((((x%16)==2)||((x%16)==13))&&(((y%16)==2)||((y%16)==13)))c=0xB8C4CE;
			if((y==7)||(y==8))c=0x55E0F0;
			if((y==6)||(y==9))c=0x2A7F8C;
			mzTex[y*32+x]=c;
		}
}

// Gravel, or any speckled ground.
void MzSpeckle(int base, int n)
{
	for(int i=0;i<MZ_TN;i++)
		mzTex[i]=MzJitter(base,n);
}

// Square tiles of size t with a line between, each tile its own shade.
void MzTiles(int base, int line, int t, int n)
{
	for(int ty=0;ty<32;ty+=t)
		for(int tx=0;tx<32;tx+=t)
		{
			int tc=MzJitter(base,n);
			for(int y=ty;y<ty+t;y++)
				for(int x=tx;x<tx+t;x++)
				{
					int c=MzJitter(tc,5);
					if((x==tx)||(y==ty))c=line;
					mzTex[y*32+x]=c;
				}
		}
}

// The gate: a glowing doorway in a frame.
void MzGate(int frame, int glow, int core)
{
	for(int y=0;y<32;y++)
		for(int x=0;x<32;x++)
		{
			int c=MzJitter(frame,10);
			if((x>=6)&&(x<26)&&(y>=4))
			{
				int dx=x-16;
				if(dx<0)dx=-dx;
				int t=dx*26+(32-y)*3;
				if(t>255)t=255;
				c=ToyMix(core,glow,t);
			}
			if(((x==5)||(x==26))&&(y>=3))c=LmLight(frame,320);
			if((y==3)&&(x>=5)&&(x<=26))c=LmLight(frame,320);
			mzTex[y*32+x]=c;
		}
}

// Copies mzTex into a shaded table: MZ_LV steps towards the fog colour, darker on
// one side for walls (sides 2) or not (sides 1). glow lessens the fog.
void MzShade(int* dst, int sides, int glow)
{
	for(int lv=0;lv<MZ_LV;lv++)
		for(int sd=0;sd<sides;sd++)
		{
			int t=lv*256/MZ_LV;
			if(glow)t=t/3;
			int o=(lv*sides+sd)*MZ_TN;
			for(int i=0;i<MZ_TN;i++)
			{
				int c=mzTex[i];
				if(sd==1)c=LmLight(c,200);
				dst[o+i]=ToyMix(c,mzFog,t);
			}
		}
}

// The look of world n: textures, sky or ceiling, fog.
void MzLook(int n)
{
	mzCeilOn=1;
	if(n==1)
	{
		mzFog=0xC9DCEA;
		mzFogK=0.9;
		MzHedge();
		MzShade(&mzWallS[0],2,0);
		MzGate(0xD8D2C4,0xFFF4C0,0xFFFFFF);
		MzShade(&mzWallS[32768],2,1);
		MzSpeckle(0xB4A486,18);
		MzShade(&mzFloorS[0],1,0);
		mzCeilOn=0;
		for(int y=0;y<PIX_H;y++)
			mzSky[y]=ToyMix(0x3E7FD6,mzFog,y*256/(PIX_H/2+1));
	}
	else if(n==2)
	{
		mzFog=0x06090E;
		mzFogK=1.3;
		MzPanels();
		MzShade(&mzWallS[0],2,0);
		MzGate(0x3A4048,0xD040F0,0xF8E0FF);
		MzShade(&mzWallS[32768],2,1);
		MzTiles(0x2C323A,0x1A1E24,8,6);
		MzShade(&mzFloorS[0],1,0);
		MzTiles(0x3A424C,0x22282E,16,4);
		for(int y=0;y<32;y++)
			for(int x=0;x<32;x++)
				if((x>=5)&&(x<11)&&(y>=5)&&(y<11))mzTex[y*32+x]=0xD8F4FF;
		MzShade(&mzCeilS[0],1,0);
	}
	else
	{
		mzFog=0x0A0705;
		mzFogK=1.2;
		MzStone(0x8A7E6A,0x3A332A);
		MzShade(&mzWallS[0],2,0);
		MzGate(0x5A4E3E,0xFFA030,0xFFF0B0);
		MzShade(&mzWallS[32768],2,1);
		MzTiles(0x6E6456,0x3A342C,16,14);
		MzShade(&mzFloorS[0],1,0);
		MzStone(0x4A4238,0x2A241E);
		MzShade(&mzCeilS[0],1,0);
	}
}

// Whether cell c has an open way in direction d (0 east, 1 south, 2 west, 3 north).
int MzOpen(int c, int d)
{
	int x=(c%MZ_C)*2+1;
	int y=(c/MZ_C)*2+1;
	if(d==0)x++;
	else if(d==1)y++;
	else if(d==2)x--;
	else y--;
	return mzMap[y*MZ_B+x]==0;
}

int MzNext(int c, int d)
{
	if(d==0)return c+1;
	if(d==1)return c+MZ_C;
	if(d==2)return c-1;
	return c-MZ_C;
}

// The walker sets out afresh from cell c: nothing seen yet.
void MzSetOut(int c)
{
	for(int i=0;i<MZ_CN;i++)
		mzSeen[i]=0;
	mzSp=1;
	mzStack[0]=c;
	mzTX=c%MZ_C;
	mzTY=c/MZ_C;
}

// A new maze in world n: a depth-first carve from the first cell, a few walls
// knocked out to make loops, the gate in the outer wall by the far corner.
void MzMake(int n)
{
	mzWorld=n;
	toyRng=toyRng+term_ms();
	MzLook(n);
	for(int i=0;i<MZ_BN;i++)
		mzMap[i]=1;
	for(int i=0;i<MZ_CN;i++)
	{
		mzSeen[i]=0;
		mzVisit[i]=0;
	}
	int dirs[4];
	mzSp=1;
	mzStack[0]=0;
	mzSeen[0]=1;
	mzMap[MZ_B+1]=0;
	while(mzSp>0)
	{
		int c=mzStack[mzSp-1];
		int cx=c%MZ_C;
		int cy=c/MZ_C;
		int k=0;
		if((cx<MZ_C-1)&&!mzSeen[c+1]){dirs[k]=0;k++;}
		if((cy<MZ_C-1)&&!mzSeen[c+MZ_C]){dirs[k]=1;k++;}
		if((cx>0)&&!mzSeen[c-1]){dirs[k]=2;k++;}
		if((cy>0)&&!mzSeen[c-MZ_C]){dirs[k]=3;k++;}
		if(k==0)
		{
			mzSp--;
			continue;
		}
		int d=dirs[ToyRandom()%k];
		int nc=MzNext(c,d);
		int x=cx*2+1;
		int y=cy*2+1;
		if(d==0)x++;
		else if(d==1)y++;
		else if(d==2)x--;
		else y--;
		mzMap[y*MZ_B+x]=0;
		mzMap[((nc/MZ_C)*2+1)*MZ_B+(nc%MZ_C)*2+1]=0;
		mzSeen[nc]=1;
		mzStack[mzSp]=nc;
		mzSp++;
	}
	for(int k=0;k<MZ_C;k++)
	{
		// a wall between two cells: odd x and even y, or the other way round
		int x=ToyRandom()%(MZ_B-2)+1;
		int y=ToyRandom()%(MZ_C-1)*2+2;
		if(ToyRandom()%2)
		{
			int t=x;
			x=y;
			y=t;
		}
		if((x%2)!=(y%2))mzMap[y*MZ_B+x]=0;
	}
	mzGoal=MZ_CN-1;
	mzMap[(MZ_B-2)*MZ_B+MZ_B-1]=2;
	mzX=1.5;
	mzY=1.5;
	mzA=0.;
	if(!MzOpen(0,0))mzA=1.5708;
	mzAtGoal=0;
	mzManual=0;
	MzSetOut(0);
}

// The walker is at the middle of the cell it walked to: on to the next. Ways not
// yet taken first, in any order; with none left, back the way it came.
void MzArrive()
{
	int c=mzTY*MZ_C+mzTX;
	mzSeen[c]=1;
	mzVisit[c]=1;
	if(c==mzGoal)
	{
		mzAtGoal=1;
		return;
	}
	int dirs[4];
	int k=0;
	for(int d=0;d<4;d++)
		if(MzOpen(c,d)&&!mzSeen[MzNext(c,d)])
		{
			dirs[k]=d;
			k++;
		}
	int nc=-1;
	if(k>0)
	{
		nc=MzNext(c,dirs[ToyRandom()%k]);
		mzStack[mzSp]=nc;
		mzSp++;
	}
	else
	{
		mzSp--;
		if(mzSp<=0)
		{
			MzSetOut(c);
			return;
		}
		nc=mzStack[mzSp-1];
	}
	mzTX=nc%MZ_C;
	mzTY=nc/MZ_C;
}

// Turns the heading towards angle want by at most step; the difference left.
float MzTurn(float want, float step)
{
	float d=want-mzA;
	while(d>3.14159265)d=d-6.2831853;
	while(d<-3.14159265)d=d+6.2831853;
	if(d>step)mzA=mzA+step;
	else if(d<-step)mzA=mzA-step;
	else mzA=mzA+d;
	if(mzA>3.14159265)mzA=mzA-6.2831853;
	if(mzA<-3.14159265)mzA=mzA+6.2831853;
	return MzAbs(d);
}

void MzFrame()
{
	if(mzFade>0)
	{
		mzFade++;
		if(mzFade>MZ_FADE)
		{
			MzMake((mzWorld+1)%MZ_WORLDS);
			mzFade=-MZ_FADE;
		}
		return;
	}
	if(mzFade<0)mzFade++;
	if(mzManual>0)
	{
		mzManual--;
		// back to walking: from the cell it stands in (or the one left of or
		// above the passage it is in)
		if(mzManual==0)MzSetOut(((int(mzY)-1)/2)*MZ_C+(int(mzX)-1)/2);
		return;
	}
	if(mzAtGoal)
	{
		// face the gate, then fade out
		if(MzTurn(0.,0.1)<0.01)mzFade=1;
		return;
	}
	float tx=mzTX*2+1.5;
	float ty=mzTY*2+1.5;
	float ex=tx-mzX;
	float ey=ty-mzY;
	float dist=sqrt(ex*ex+ey*ey);
	if(dist<0.01)
	{
		mzX=tx;
		mzY=ty;
		MzArrive();
		return;
	}
	float left=MzTurn(atan2(ey,ex),0.11);
	if(left>0.35)return;
	float step=0.09;
	if(left>0.1)step=0.04;
	if(step>dist)step=dist;
	mzX=mzX+ex/dist*step;
	mzY=mzY+ey/dist*step;
}

// A step by the keys: forward (f 1) or back (-1), sliding along walls.
void MzStep(float f)
{
	float dx=cos(mzA)*0.2*f;
	float dy=sin(mzA)*0.2*f;
	float mx=0.25;
	if(dx<0.)mx=-0.25;
	float my=0.25;
	if(dy<0.)my=-0.25;
	if(mzMap[int(mzY)*MZ_B+int(mzX+dx+mx)]==0)mzX=mzX+dx;
	if(mzMap[int(mzY+dy+my)*MZ_B+int(mzX)]==0)mzY=mzY+dy;
	mzManual=100;
	int c=((int(mzY)-1)/2)*MZ_C+(int(mzX)-1)/2;
	if(((int(mzX)%2)==1)&&((int(mzY)%2)==1))mzVisit[c]=1;
}

void MzKeyTurn(float a)
{
	mzA=mzA+a;
	mzManual=100;
}

// The map in the top left corner, a block a pixel, or two on a big view.
void MzDrawMap(int w, int h)
{
	int s=2;
	if((w<MZ_B*5)||(h<MZ_B*3))s=1;
	if((w<MZ_B*2)||(h<MZ_B+2))return;
	for(int by=0;by<MZ_B;by++)
		for(int bx=0;bx<MZ_B;bx++)
		{
			int v=mzMap[by*MZ_B+bx];
			int c=0x101418;
			if(v==1)c=0x9AA4AE;
			if(v==2)c=0xFFD040;
			if((v==0)&&((bx%2)==1)&&((by%2)==1)&&mzVisit[((by-1)/2)*MZ_C+(bx-1)/2])c=0x2E4A66;
			if((bx==int(mzX))&&(by==int(mzY)))c=0xFF4040;
			for(int y=0;y<s;y++)
				for(int x=0;x<s;x++)
					svPix[(1+by*s+y)*PIX_W+1+bx*s+x]=c;
		}
}

void MzDraw(int w, int h)
{
	int hor=h/2;
	float focal=w/1.32;
	float dx=cos(mzA);
	float dy=sin(mzA);
	float px=-dy*0.66;
	float py=dx*0.66;
	// floor and ceiling (or sky), a row and its mirror at a time; in 16.16 fixed
	// point, 32 texels a block
	for(int y=hor;y<h;y++)
	{
		float rd=focal*0.5/(y-hor+0.5);
		int fx=int((mzX+rd*(dx-px))*65536.);
		int fy=int((mzY+rd*(dy-py))*65536.);
		int sx=int(rd*2.*px*65536./w);
		int sy=int(rd*2.*py*65536./w);
		int lv=int(rd*mzFogK);
		if(lv>MZ_LV-1)lv=MZ_LV-1;
		int fo=lv*MZ_TN;
		int row=y*PIX_W;
		int yc=2*hor-1-y;
		int crow=yc*PIX_W;
		if(yc<0)crow=row;
		if(mzCeilOn)
			for(int x=0;x<w;x++)
			{
				int t=(((fy>>11)&31)<<5)|((fx>>11)&31);
				svPix[crow+x]=mzCeilS[fo+t];
				svPix[row+x]=mzFloorS[fo+t];
				fx+=sx;
				fy+=sy;
			}
		else
		{
			for(int x=0;x<w;x++)
			{
				svPix[row+x]=mzFloorS[fo+((((fy>>11)&31)<<5)|((fx>>11)&31))];
				fx+=sx;
				fy+=sy;
			}
			if(yc>=0)
			{
				int c=mzSky[yc*PIX_H/h];
				for(int x=0;x<w;x++)
					svPix[crow+x]=c;
			}
		}
	}
	// the walls, a column at a time
	for(int x=0;x<w;x++)
	{
		float cam=2.*x/w-1.;
		float rx=dx+px*cam;
		float ry=dy+py*cam;
		int mx=int(mzX);
		int my=int(mzY);
		float ddx=1000000.;
		float ddy=1000000.;
		if(MzAbs(rx)>0.000001)ddx=MzAbs(1./rx);
		if(MzAbs(ry)>0.000001)ddy=MzAbs(1./ry);
		int stx=1;
		int sty=1;
		float sdx=(mx+1.-mzX)*ddx;
		float sdy=(my+1.-mzY)*ddy;
		if(rx<0.)
		{
			stx=-1;
			sdx=(mzX-mx)*ddx;
		}
		if(ry<0.)
		{
			sty=-1;
			sdy=(mzY-my)*ddy;
		}
		int side=0;
		int hit=0;
		int guard=0;
		while((hit==0)&&(guard<80))
		{
			if(sdx<sdy)
			{
				sdx=sdx+ddx;
				mx+=stx;
				side=0;
			}
			else
			{
				sdy=sdy+ddy;
				my+=sty;
				side=1;
			}
			if((mx<0)||(my<0)||(mx>=MZ_B)||(my>=MZ_B))hit=1;
			else hit=mzMap[my*MZ_B+mx];
			guard++;
		}
		float perp=sdy-ddy;
		float wx=mzX+perp*rx;
		if(side==0)
		{
			perp=sdx-ddx;
			wx=mzY+perp*ry;
		}
		if(perp<0.05)perp=0.05;
		wx=wx-int(wx);
		int tx=int(wx*32.)&31;
		if(((side==0)&&(rx>0.))||((side==1)&&(ry<0.)))tx=31-tx;
		int lv=int(perp*mzFogK);
		if(lv>MZ_LV-1)lv=MZ_LV-1;
		int base=(lv*2+side)*MZ_TN+tx;
		if(hit==2)base=base+32768;
		int lh=int(focal/perp);
		if(lh<1)lh=1;
		int step=32*65536/lh;
		int y0=hor-lh/2;
		int y1=y0+lh;
		int tp=0;
		if(y0<0)
		{
			tp=-y0*step;
			y0=0;
		}
		if(y1>h)y1=h;
		for(int y=y0;y<y1;y++)
		{
			svPix[y*PIX_W+x]=mzWallS[base+(((tp>>16)&31)<<5)];
			tp+=step;
		}
	}
	if(mzMapOn&&!mzFull)MzDrawMap(w,h);
	if(mzFade!=0)
	{
		int f=mzFade;
		if(f<0)f=-f;
		int l=256-f*256/MZ_FADE;
		for(int y=0;y<h;y++)
			for(int x=0;x<w;x++)
				svPix[y*PIX_W+x]=LmLight(svPix[y*PIX_W+x],l);
	}
}

// ---------------------------------------------------------------- objects

// The key light, from the upper left in front, and the half vector between it and
// the eye (which looks along +z; y is down).
#define OB_LX -0.4508
#define OB_LY -0.6211
#define OB_LZ -0.6411
#define OB_HX -0.2488
#define OB_HY -0.3428
#define OB_HZ -0.9058

char* ObShapeName(int n)
{
	if(n==1)return "Metaballs";
	if(n==2)return "Torus";
	if(n==3)return "Torus knot";
	if(n==4)return "Cylinder";
	if(n==5)return "Molecule";
	return "Box";
}

char* ObMatName(int n)
{
	if(n==1)return "Silver";
	if(n==2)return "Gold";
	if(n==3)return "Red plastic";
	if(n==4)return "Cheese";
	if(n==5)return "Pink plaster";
	if(n==6)return "Red-hot iron";
	return "Copper";
}

// A vertex; its number.
int ObV(float x, float y, float z, float nx, float ny, float nz)
{
	if(obNV>=OB_MAXV)return OB_MAXV-1;
	obPX[obNV]=x;
	obPY[obNV]=y;
	obPZ[obNV]=z;
	obNX[obNV]=nx;
	obNY[obNV]=ny;
	obNZ[obNV]=nz;
	obNV++;
	return obNV-1;
}

// A triangle, turned to face the way its vertices' normals point (outwards), so
// that the back faces can be told by the winding. Empty ones are left out.
void ObT(int a, int b, int c)
{
	if(obNT>=OB_MAXT)return;
	float ux=obPX[b]-obPX[a];
	float uy=obPY[b]-obPY[a];
	float uz=obPZ[b]-obPZ[a];
	float vx=obPX[c]-obPX[a];
	float vy=obPY[c]-obPY[a];
	float vz=obPZ[c]-obPZ[a];
	float nx=uy*vz-uz*vy;
	float ny=uz*vx-ux*vz;
	float nz=ux*vy-uy*vx;
	if(nx*nx+ny*ny+nz*nz<0.0000000001)return;
	float d=nx*(obNX[a]+obNX[b]+obNX[c])+ny*(obNY[a]+obNY[b]+obNY[c])+nz*(obNZ[a]+obNZ[b]+obNZ[c]);
	obTA[obNT]=a;
	obTB[obNT]=b;
	obTC[obNT]=c;
	obTCol[obNT]=obCurCol;
	if(d<0.)
	{
		obTB[obNT]=c;
		obTC[obNT]=b;
	}
	obNT++;
}

// Triangles over a grid of (nu+1) x (nv+1) vertices from vertex base on, row by row.
void ObGridTris(int base, int nu, int nv)
{
	for(int j=0;j<nv;j++)
		for(int i=0;i<nu;i++)
		{
			int a=base+j*(nu+1)+i;
			int b=a+1;
			int c=a+nu+2;
			int d=a+nu+1;
			ObT(a,b,c);
			ObT(a,c,d);
		}
}

// A box with rounded edges: each face a grid, its points pulled in to a smaller
// box and pushed out again by the rounding along their way out of it.
void ObBox()
{
	float hs=0.62;
	float inn=0.42;
	float k[13];
	k[0]=-0.62; k[1]=-0.6; k[2]=-0.56; k[3]=-0.5; k[4]=-0.42; k[5]=-0.21; k[6]=0.;
	k[7]=0.21; k[8]=0.42; k[9]=0.5; k[10]=0.56; k[11]=0.6; k[12]=0.62;
	for(int f=0;f<6;f++)
	{
		int base=obNV;
		float s=1.;
		if(f>=3)s=-1.;
		int ax=f%3;
		for(int j=0;j<13;j++)
			for(int i=0;i<13;i++)
			{
				float q[3];
				q[ax]=hs*s;
				q[(ax+1)%3]=k[i];
				q[(ax+2)%3]=k[j];
				float p[3];
				for(int e=0;e<3;e++)
				{
					p[e]=q[e];
					if(p[e]>inn)p[e]=inn;
					if(p[e]<-inn)p[e]=-inn;
				}
				float dx=q[0]-p[0];
				float dy=q[1]-p[1];
				float dz=q[2]-p[2];
				float l=sqrt(dx*dx+dy*dy+dz*dz);
				dx=dx/l;
				dy=dy/l;
				dz=dz/l;
				ObV(p[0]+dx*(hs-inn),p[1]+dy*(hs-inn),p[2]+dz*(hs-inn),dx,dy,dz);
			}
		ObGridTris(base,12,12);
	}
}

// Three metaballs wandering about each other, now apart, now run together: the
// field is the sum of r*r/d*d over the balls, the surface where it is 1. It is
// found by marching tetrahedra: the field on a grid, each cube of the grid cut
// into six tetrahedra, and in each the surface crossing its edges as one or two
// triangles; normals from the field's slope.
#define OB_MG 18
float ObField(float x, float y, float z)
{
	float f=0.;
	for(int i=0;i<3;i++)
	{
		float dx=x-obMb[i*3];
		float dy=y-obMb[i*3+1];
		float dz=z-obMb[i*3+2];
		f=f+0.07/(dx*dx+dy*dy+dz*dz+0.000001);
	}
	return f;
}

// A vertex where the surface crosses from grid point a (field fa) to b.
int ObBlobV(float* a, float fa, float* b, float fb)
{
	float t=(1.-fa)/(fb-fa);
	float px=a[0]+(b[0]-a[0])*t;
	float py=a[1]+(b[1]-a[1])*t;
	float pz=a[2]+(b[2]-a[2])*t;
	float nx=0.;
	float ny=0.;
	float nz=0.;
	for(int i=0;i<3;i++)
	{
		float ex=px-obMb[i*3];
		float ey=py-obMb[i*3+1];
		float ez=pz-obMb[i*3+2];
		float e2=ex*ex+ey*ey+ez*ez+0.000001;
		float k=1./(e2*e2);
		nx=nx+ex*k;
		ny=ny+ey*k;
		nz=nz+ez*k;
	}
	float l=1./sqrt(nx*nx+ny*ny+nz*nz+0.0000001);
	return ObV(px,py,pz,nx*l,ny*l,nz*l);
}

// The corners of a cube, x y z each 0 or 1, and its six tetrahedra about the
// diagonal from corner 0 to corner 6.
int obCubeC[24]={0,0,0, 1,0,0, 1,1,0, 0,1,0, 0,0,1, 1,0,1, 1,1,1, 0,1,1};
int obTets[24]={0,5,1,6, 0,1,2,6, 0,2,3,6, 0,3,7,6, 0,7,4,6, 0,4,5,6};

void ObBlob()
{
	for(int i=0;i<3;i++)
	{
		obMb[i*3]=0.5*sin(obT*(0.43+i*0.11)+i*2.1);
		obMb[i*3+1]=0.42*sin(obT*(0.37+i*0.07)+i*1.3+0.5);
		obMb[i*3+2]=0.45*sin(obT*(0.29+i*0.13)+i*2.9+1.);
	}
	float lo=-1.05;
	float st=2.1/(OB_MG-1);
	for(int z=0;z<OB_MG;z++)
		for(int y=0;y<OB_MG;y++)
			for(int x=0;x<OB_MG;x++)
			{
				int g=(z*OB_MG+y)*OB_MG+x;
				float f=ObField(lo+x*st,lo+y*st,lo+z*st);
				obMf[g]=f;
				obMi[g]=f>1.;
			}
	int dy=OB_MG;
	int dz=OB_MG*OB_MG;
	float cp[24];
	float cf[8];
	for(int z=0;z<OB_MG-1;z++)
		for(int y=0;y<OB_MG-1;y++)
			for(int x=0;x<OB_MG-1;x++)
			{
				// the surface passes through only where the corners differ
				int g=(z*OB_MG+y)*OB_MG+x;
				int nin=obMi[g]+obMi[g+1]+obMi[g+dy]+obMi[g+dy+1]+obMi[g+dz]+obMi[g+dz+1]+obMi[g+dz+dy]+obMi[g+dz+dy+1];
				if((nin==0)||(nin==8))continue;
				for(int k=0;k<8;k++)
				{
					int gx=x+obCubeC[k*3];
					int gy=y+obCubeC[k*3+1];
					int gz=z+obCubeC[k*3+2];
					cf[k]=obMf[(gz*OB_MG+gy)*OB_MG+gx];
					cp[k*3]=lo+gx*st;
					cp[k*3+1]=lo+gy*st;
					cp[k*3+2]=lo+gz*st;
				}
				for(int t=0;t<6;t++)
				{
					int q[4];
					int ins[4];
					int ni=0;
					for(int k=0;k<4;k++)
					{
						q[k]=obTets[t*4+k];
						ins[k]=cf[q[k]]>1.;
						ni=ni+ins[k];
					}
					if((ni==0)||(ni==4))continue;
					if((ni==1)||(ni==3))
					{
						// the one corner unlike the others, cut off
						int odd=0;
						for(int k=0;k<4;k++)
							if(ins[k]==(ni==1))odd=k;
						int o=q[odd];
						int vs[3];
						int m=0;
						for(int k=0;k<4;k++)
							if(k!=odd)
							{
								vs[m]=ObBlobV(&cp[o*3],cf[o],&cp[q[k]*3],cf[q[k]]);
								m++;
							}
						ObT(vs[0],vs[1],vs[2]);
					}
					else
					{
						// two in, two out: a four-sided cut
						int a=-1;
						int b=-1;
						int c=-1;
						int d=-1;
						for(int k=0;k<4;k++)
						{
							if(ins[k])
							{
								if(a<0)a=q[k];
								else b=q[k];
							}
							else
							{
								if(c<0)c=q[k];
								else d=q[k];
							}
						}
						int v0=ObBlobV(&cp[a*3],cf[a],&cp[c*3],cf[c]);
						int v1=ObBlobV(&cp[a*3],cf[a],&cp[d*3],cf[d]);
						int v2=ObBlobV(&cp[b*3],cf[b],&cp[d*3],cf[d]);
						int v3=ObBlobV(&cp[b*3],cf[b],&cp[c*3],cf[c]);
						ObT(v0,v1,v2);
						ObT(v0,v2,v3);
					}
				}
			}
}

void ObTorus()
{
	int base=obNV;
	for(int j=0;j<=18;j++)
		for(int i=0;i<=40;i++)
		{
			float u=i*6.2831853/40.;
			float v=j*6.2831853/18.;
			float nx=cos(v)*cos(u);
			float ny=sin(v);
			float nz=cos(v)*sin(u);
			ObV(0.7*cos(u)+0.3*nx,0.3*ny,0.7*sin(u)+0.3*nz,nx,ny,nz);
		}
	ObGridTris(base,40,18);
}

// The (2,3) torus knot: a tube around the curve that winds twice round and three
// times through a torus, its ring at each point square to the curve.
void ObKnotAt(float t, float* c)
{
	float r=2.+cos(3.*t);
	c[0]=r*cos(2.*t)*0.33;
	c[1]=sin(3.*t)*0.33;
	c[2]=r*sin(2.*t)*0.33;
}

void ObKnot()
{
	int base=obNV;
	int nu=128;
	int nv=10;
	float c[3];
	float c2[3];
	for(int i=0;i<=nu;i++)
	{
		float t=i*6.2831853/nu;
		ObKnotAt(t,&c[0]);
		ObKnotAt(t+0.001,&c2[0]);
		float tx=c2[0]-c[0];
		float ty=c2[1]-c[1];
		float tz=c2[2]-c[2];
		float l=sqrt(tx*tx+ty*ty+tz*tz);
		tx=tx/l;
		ty=ty/l;
		tz=tz/l;
		// away from the torus' middle circle, made square to the curve
		float ux=c[0]-0.66*cos(2.*t);
		float uy=c[1];
		float uz=c[2]-0.66*sin(2.*t);
		float d=ux*tx+uy*ty+uz*tz;
		ux=ux-d*tx;
		uy=uy-d*ty;
		uz=uz-d*tz;
		l=sqrt(ux*ux+uy*uy+uz*uz);
		ux=ux/l;
		uy=uy/l;
		uz=uz/l;
		float bx=ty*uz-tz*uy;
		float by=tz*ux-tx*uz;
		float bz=tx*uy-ty*ux;
		for(int j=0;j<=nv;j++)
		{
			float v=j*6.2831853/nv;
			float nx=ux*cos(v)+bx*sin(v);
			float ny=uy*cos(v)+by*sin(v);
			float nz=uz*cos(v)+bz*sin(v);
			ObV(c[0]+nx*0.14,c[1]+ny*0.14,c[2]+nz*0.14,nx,ny,nz);
		}
	}
	// here the rings are the rows: nv+1 vertices each, nu of them
	ObGridTris(base,nv,nu);
}

// A cylinder with rounded edges: a profile (out from the axis, along it) of the
// flat end, a quarter circle, the side, a quarter circle and the other end,
// turned round the axis.
void ObCylinder()
{
	float r=0.62;
	float hh=0.7;
	float rr=0.18;
	float pr[30];
	float py[30];
	float nr[30];
	float ny[30];
	int n=0;
	for(int e=0;e<2;e++)
	{
		float s=-1.;
		if(e==1)s=1.;
		// the end, from the axis out (then back in for the second end, below)
		for(int i=0;i<4;i++)
		{
			pr[n]=(r-rr)*i/4.;
			py[n]=-hh;
			nr[n]=0.;
			ny[n]=-1.;
			n++;
		}
		for(int i=0;i<=6;i++)
		{
			float a=i*1.5707963/6.;
			pr[n]=r-rr+rr*sin(a);
			py[n]=-hh+rr-rr*cos(a);
			nr[n]=sin(a);
			ny[n]=-cos(a);
			n++;
		}
		if(e==0)
		{
			// the side between the two quarter circles
			for(int i=1;i<3;i++)
			{
				pr[n]=r;
				py[n]=-hh+rr+(2.*(hh-rr))*i/3.;
				nr[n]=1.;
				ny[n]=0.;
				n++;
			}
		}
	}
	// the second end was laid out like the first: mirror it and reverse it
	int h=n-11;
	for(int i=0;i<11;i++)
		py[h+i]=-py[h+i];
	for(int i=0;i<11;i++)
		ny[h+i]=-ny[h+i];
	for(int i=0;i<5;i++)
	{
		int a=h+i;
		int b=n-1-i;
		float t=pr[a];pr[a]=pr[b];pr[b]=t;
		t=py[a];py[a]=py[b];py[b]=t;
		t=nr[a];nr[a]=nr[b];nr[b]=t;
		t=ny[a];ny[a]=ny[b];ny[b]=t;
	}
	int base=obNV;
	for(int j=0;j<n;j++)
		for(int i=0;i<=40;i++)
		{
			float u=i*6.2831853/40.;
			ObV(pr[j]*cos(u),py[j],pr[j]*sin(u),nr[j]*cos(u),ny[j],nr[j]*sin(u));
		}
	ObGridTris(base,40,n-1);
}

void ObMaterial(int m);

// Atoms: colours as chemists draw them, radii a third of the van der Waals radii.
int ObAtomColour(int e)
{
	if(e==0)return 0xE8E8E8;
	if(e==2)return 0x3050F8;
	if(e==3)return 0xFF0D0D;
	if(e==4)return 0x1FF01F;
	return 0x909090;
}

float ObAtomR(int e)
{
	if(e==0)return 0.504;
	if(e==2)return 0.651;
	if(e==3)return 0.638;
	if(e==4)return 0.735;
	return 0.714;
}

// An atom's radius as shown: smaller in a piece of crystal, to let the lattice
// show through.
float ObMolR(int e)
{
	if(obMol>=OB_CRYST)return ObAtomR(e)*0.55;
	return ObAtomR(e);
}

// A bond's stick from p to q, radius r, colour c: an open tube of eight sides.
void ObStick(float* p, float* q, float r, int c)
{
	float dx=q[0]-p[0];
	float dy=q[1]-p[1];
	float dz=q[2]-p[2];
	float l=sqrt(dx*dx+dy*dy+dz*dz);
	dx=dx/l;
	dy=dy/l;
	dz=dz/l;
	// two directions square to it
	float ax=0.;
	float ay=0.;
	float az=1.;
	if((dz>0.9)||(dz<-0.9))
	{
		ax=1.;
		az=0.;
	}
	float ux=dy*az-dz*ay;
	float uy=dz*ax-dx*az;
	float uz=dx*ay-dy*ax;
	l=sqrt(ux*ux+uy*uy+uz*uz);
	ux=ux/l;
	uy=uy/l;
	uz=uz/l;
	float vx=dy*uz-dz*uy;
	float vy=dz*ux-dx*uz;
	float vz=dx*uy-dy*ux;
	int base=obNV;
	for(int j=0;j<2;j++)
		for(int i=0;i<=8;i++)
		{
			float a=i*0.785398;
			float nx=ux*cos(a)+vx*sin(a);
			float ny=uy*cos(a)+vy*sin(a);
			float nz=uz*cos(a)+vz*sin(a);
			float* e=p;
			if(j==1)e=q;
			ObV(e[0]+nx*r,e[1]+ny*r,e[2]+nz*r,nx,ny,nz);
		}
	obCurCol=c;
	ObGridTris(base,8,1);
	obCurCol=-1;
}

// Atom k (numbered in the whole table) scaled by s into p.
void ObAtomAt(int k, float s, float* p)
{
	p[0]=obMolAtom[k*4+1]*0.001*s;
	p[1]=obMolAtom[k*4+2]*0.001*s;
	p[2]=obMolAtom[k*4+3]*0.001*s;
}

// Molecule obMol in balls and sticks, made to fit: each bond two sticks, half in
// each atom's colour; double and triple bonds as two or three thinner ones, side
// by side in the plane of a neighbouring bond.
void ObMolecule()
{
	ObMaterial(3);
	int a0=obMolA0[obMol];
	int na=obMolA0[obMol+1]-a0;
	float ext=0.;
	for(int i=0;i<na;i++)
	{
		int k=(a0+i)*4;
		float x=obMolAtom[k+1]*0.001;
		float y=obMolAtom[k+2]*0.001;
		float z=obMolAtom[k+3]*0.001;
		float e=sqrt(x*x+y*y+z*z)+ObMolR(obMolAtom[k]);
		if(e>ext)ext=e;
	}
	float s=1./ext;
	if(s>0.7)s=0.7;
	// it turns about the upright mostly: across it reaches as far as it does
	// out from the upright, up and down as it does along it (and a little more,
	// for the tilt). A flat one (its least extent is along z) rolls facing you
	// instead: it reaches as far as it does out from z every way.
	float eh=0.;
	float ev=0.;
	float er=0.;
	obMolFlat=1;
	for(int i=0;i<na;i++)
	{
		int k=(a0+i)*4;
		float x=obMolAtom[k+1]*0.001;
		float y=obMolAtom[k+2]*0.001;
		float z=obMolAtom[k+3]*0.001;
		float r=ObMolR(obMolAtom[k]);
		if((z>0.3)||(z<-0.3))obMolFlat=0;
		float e=sqrt(x*x+z*z)+r;
		if(e>eh)eh=e;
		e=sqrt(x*x+y*y)+r;
		if(e>er)er=e;
		if(y<0.)y=-y;
		if(y+r>ev)ev=y+r;
	}
	// shown 1.33 times nearer than a snug fit, to fill the screen
	obFitH=eh*s*1.05/1.33;
	obFitV=(ev*s*1.25+eh*s*0.25)/1.33;
	if(obMolFlat)
	{
		obFitH=er*s*1.02;
		obFitV=er*s*1.02;
	}
	for(int i=0;i<na;i++)
	{
		if(obNK>=OB_MAXK)break;
		float p[3];
		ObAtomAt(a0+i,s,&p[0]);
		obKX[obNK]=p[0];
		obKY[obNK]=p[1];
		obKZ[obNK]=p[2];
		obKR[obNK]=ObMolR(obMolAtom[(a0+i)*4])*s;
		obKC[obNK]=ObAtomColour(obMolAtom[(a0+i)*4]);
		obNK++;
	}
	int b0=obMolB0[obMol];
	int nb=obMolB0[obMol+1]-b0;
	for(int i=0;i<nb;i++)
	{
		int ia=obMolBond[(b0+i)*3];
		int ib=obMolBond[(b0+i)*3+1];
		int ord=obMolBond[(b0+i)*3+2];
		float p[3];
		float q[3];
		float m[3];
		ObAtomAt(a0+ia,s,&p[0]);
		ObAtomAt(a0+ib,s,&q[0]);
		float dx=q[0]-p[0];
		float dy=q[1]-p[1];
		float dz=q[2]-p[2];
		float l=sqrt(dx*dx+dy*dy+dz*dz);
		dx=dx/l;
		dy=dy/l;
		dz=dz/l;
		int ca=ObAtomColour(obMolAtom[(a0+ia)*4]);
		int cb=ObAtomColour(obMolAtom[(a0+ib)*4]);
		int n=1;
		if((ord==2)||(ord==3))n=ord;
		float r=0.11*s;
		if(n==2)r=0.075*s;
		if(n==3)r=0.06*s;
		// the side to put them by: towards another atom bonded to either end
		float ox=0.;
		float oy=0.;
		float oz=0.;
		if(n>1)
		{
			int other=-1;
			for(int j=0;j<nb;j++)
			{
				int ja=obMolBond[(b0+j)*3];
				int jb=obMolBond[(b0+j)*3+1];
				if((ja==ia)&&(jb!=ib))other=jb;
				else if((jb==ia)&&(ja!=ib))other=ja;
				else if((ja==ib)&&(jb!=ia))other=jb;
				else if((jb==ib)&&(ja!=ia))other=ja;
			}
			float w[3];
			w[0]=0.;
			w[1]=1.;
			w[2]=0.;
			if(other>=0)
			{
				ObAtomAt(a0+other,s,&w[0]);
				w[0]=w[0]-p[0];
				w[1]=w[1]-p[1];
				w[2]=w[2]-p[2];
			}
			float d=w[0]*dx+w[1]*dy+w[2]*dz;
			ox=w[0]-d*dx;
			oy=w[1]-d*dy;
			oz=w[2]-d*dz;
			float ol=sqrt(ox*ox+oy*oy+oz*oz)+0.000001;
			ox=ox/ol*0.13*s;
			oy=oy/ol*0.13*s;
			oz=oz/ol*0.13*s;
		}
		for(int k=0;k<n;k++)
		{
			float f=k-(n-1)*0.5;
			float pp[3];
			float qq[3];
			pp[0]=p[0]+ox*f;
			pp[1]=p[1]+oy*f;
			pp[2]=p[2]+oz*f;
			qq[0]=q[0]+ox*f;
			qq[1]=q[1]+oy*f;
			qq[2]=q[2]+oz*f;
			m[0]=(pp[0]+qq[0])*0.5;
			m[1]=(pp[1]+qq[1])*0.5;
			m[2]=(pp[2]+qq[2])*0.5;
			ObStick(&pp[0],&m[0],r,ca);
			ObStick(&m[0],&qq[0],r,cb);
		}
	}
}

// Part n of a list "a|b|c" into out.
void ObPart(char* list, int n, char* out, int max)
{
	int i=0;
	while((n>0)&&(list[i]!=0))
	{
		if(list[i]=='|')n--;
		i++;
	}
	int o=0;
	while((list[i]!=0)&&(list[i]!='|')&&(o<max-1))
	{
		out[o]=list[i];
		o++;
		i++;
	}
	out[o]=0;
}

void ObBuild()
{
	obNV=0;
	obNT=0;
	obNK=0;
	obCurCol=-1;
	obFitH=1.08;
	obFitV=1.08;
	if(obShape==1)ObBlob();
	else if(obShape==5)ObMolecule();
	else if(obShape==2)ObTorus();
	else if(obShape==3)ObKnot();
	else if(obShape==4)ObCylinder();
	else ObBox();
}

// The studio, seen along direction x,y,z (y down): a sky darkening upwards, a
// floor darkening downwards, and three soft lights, all without edges, as 0..1
// colour in c.
float ObLobe(float x, float y, float z, float lx, float ly, float lz, int p)
{
	float d=x*lx+y*ly+z*lz;
	if(d<=0.)return 0.;
	float v=d;
	for(int i=1;i<p;i++)
		v=v*d;
	return v;
}

void ObStudio(float x, float y, float z, float* c)
{
	float l=1./sqrt(x*x+y*y+z*z);
	x=x*l;
	y=y*l;
	z=z*l;
	float e=-y;
	if(e>=0.)
	{
		c[0]=0.56-e*0.36;
		c[1]=0.58-e*0.35;
		c[2]=0.64-e*0.32;
	}
	else
	{
		float t=-e*2.5;
		if(t>1.)t=1.;
		c[0]=0.5-t*0.38;
		c[1]=0.48-t*0.37;
		c[2]=0.45-t*0.35;
	}
	// a warm key light up on the left behind the eye, a cool one on the right,
	// one high behind the object
	float k=0.75*ObLobe(x,y,z,-0.45,-0.7,-0.55,6);
	float f=0.4*ObLobe(x,y,z,0.85,-0.25,-0.46,4);
	float b=0.5*ObLobe(x,y,z,0.,-0.75,0.66,5);
	c[0]=c[0]+k+f*0.75+b*0.9;
	c[1]=c[1]+k*0.95+f*0.85+b*0.95;
	c[2]=c[2]+k*0.85+f+b;
}

// The sphere map: for each point of a disc, the normal of a ball there facing the
// eye, the eye's ray mirrored in it, and the studio along that; nine looks a
// texel, so that the strips' edges are soft.
void ObMakeEnv()
{
	float c[3];
	for(int j=0;j<64;j++)
		for(int i=0;i<64;i++)
		{
			float sr=0.;
			float sg=0.;
			float sb=0.;
			for(int k=0;k<9;k++)
			{
				float nx=(i+0.17+(k%3)*0.33)/32.-1.;
				float ny=(j+0.17+(k/3)*0.33)/32.-1.;
				float l=nx*nx+ny*ny;
				if(l>1.)
				{
					l=sqrt(l);
					nx=nx/l;
					ny=ny/l;
					l=1.;
				}
				float nz=-sqrt(1.-l);
				// mirror of (0,0,1), the way the eye looks
				float d=2.*nz;
				ObStudio(-d*nx,-d*ny,1.-d*nz,&c[0]);
				sr=sr+c[0];
				sg=sg+c[1];
				sb=sb+c[2];
			}
			int r=int(sr*160./9.);
			int g=int(sg*160./9.);
			int b=int(sb*160./9.);
			obEnv[j*64+i]=(r<<20)|(g<<10)|b;
		}
}

// The material: colour, kind, and the highlight's spread and strength.
void ObMaterial(int m)
{
	obMat=m;
	float sp=40.;
	float st=1.;
	obKind=0;
	if(m==0){obBR=0.96;obBG=0.62;obBB=0.48;sp=30.;}
	else if(m==1){obBR=0.95;obBG=0.95;obBB=0.97;sp=50.;}
	else if(m==2){obBR=1.;obBG=0.78;obBB=0.32;sp=40.;}
	else if(m==3){obKind=1;obBR=0.78;obBG=0.05;obBB=0.04;sp=70.;st=0.9;}
	else if(m==4){obKind=2;obBR=1.;obBG=0.84;obBB=0.34;sp=9.;st=0.22;}
	else if(m==5){obKind=3;obBR=0.95;obBG=0.62;obBB=0.68;sp=4.;st=0.;}
	else {obKind=4;obBR=1.;obBG=0.3;obBB=0.1;sp=12.;st=0.25;}
	obMR=obBR;
	obMG=obBG;
	obMB=obBB;
	obMKind=obKind;
	for(int i=0;i<256;i++)
		obSpec[i]=st*pow(i/255.,sp)*255.;
	int keys[6];
	keys[0]=0x1A0402;
	keys[1]=0x6E0C02;
	keys[2]=0xC82A04;
	keys[3]=0xF56A10;
	keys[4]=0xFFB838;
	keys[5]=0xFFF0B0;
	for(int i=0;i<256;i++)
	{
		int k=i*5/256;
		obHeat[i]=ToyMix(keys[k],keys[k+1],(i*5-k*256)%256);
	}
}

// A small hash of three whole numbers, 0..32767.
int ObHash(int x, int y, int z)
{
	int h=x*73856093+y*19349663+z*83492791;
	h=h*1103515245+12345;
	return (h>>16)&0x7fff;
}

int ObClamp(int v)
{
	if(v<0)return 0;
	if(v>255)return 255;
	return v;
}

// The colour at a point with normal n (in view, needn't be of length one) and
// object position o.
int ObShade(float nx, float ny, float nz, float ox, float oy, float oz)
{
	float l=1./sqrt(nx*nx+ny*ny+nz*nz+0.000001);
	nx=nx*l;
	ny=ny*l;
	nz=nz*l;
	float df=nx*OB_LX+ny*OB_LY+nz*OB_LZ;
	if(df<0.)df=0.;
	float nh=nx*OB_HX+ny*OB_HY+nz*OB_HZ;
	if(nh<0.)nh=0.;
	float sp=obSpec[int(nh*255.)];
	// the eye's ray mirrored, and the normal that would mirror the middle ray so:
	// where the sphere map has it
	float il=1./sqrt(obIX*obIX+obIY*obIY+1.);
	float ix=obIX*il;
	float iy=obIY*il;
	float iz=il;
	float d2=2.*(nx*ix+ny*iy+nz*iz);
	float mx=ix-d2*nx;
	float my=iy-d2*ny;
	float mz=iz-d2*nz-1.;
	float ml=1./sqrt(mx*mx+my*my+mz*mz+0.000001);
	mx=mx*ml;
	my=my*ml;
	int ei=int((my*0.5+0.5)*63.99)*64+int((mx*0.5+0.5)*63.99);
	int env=obEnv[ei];
	float er=(env>>20)&1023;
	float eg=(env>>10)&1023;
	float eb=env&1023;
	float r=0.;
	float g=0.;
	float b=0.;
	if(obKind==0)
	{
		// metal: the studio in its colour, a little light of its own, the highlight
		float k=0.9;
		r=obBR*(er*k+df*40.+10.)+sp*obBR;
		g=obBG*(eg*k+df*40.+10.)+sp*obBG;
		b=obBB*(eb*k+df*40.+10.)+sp*obBB;
	}
	else if(obKind==4)
	{
		// hot iron: glowing in slowly moving patches, hottest facing us
		float h=0.55+0.15*(sin(ox*5.+obT)+sin(oy*4.3-obT*0.7)+sin(oz*5.6+obT*0.5));
		h=h*(0.65-nz*0.35);
		int hi=int(h*255.);
		if(hi<0)hi=0;
		if(hi>255)hi=255;
		int c=obHeat[hi];
		r=((c>>16)&255)+sp;
		g=((c>>8)&255)+sp*0.8;
		b=(c&255)+sp*0.6;
	}
	else
	{
		float lit=0.16+0.84*df;
		float sh=sp;
		float br=obBR;
		float bg=obBG;
		float bb=obBB;
		if(obKind==2)
		{
			// cheese: soft light through it, and holes, one a cell at most
			lit=0.3+0.7*df;
			float cs=0.36;
			int cx=int(ox/cs+100.);
			int cy=int(oy/cs+100.);
			int cz=int(oz/cs+100.);
			int hs=ObHash(cx,cy,cz);
			if((hs&3)!=0)
			{
				float rr=cs*(0.16+(hs>>2&15)*0.008);
				float hx=(cx-100.+0.3+(hs>>6&7)*0.055)*cs;
				float hy=(cy-100.+0.3+(hs>>9&7)*0.055)*cs;
				float hz=(cz-100.+0.3+(hs>>12&7)*0.055)*cs;
				float dx=ox-hx;
				float dy=oy-hy;
				float dz=oz-hz;
				float dd=dx*dx+dy*dy+dz*dz;
				if(dd<rr*rr)
				{
					float t=sqrt(dd)/rr;
					lit=lit*(0.45+0.4*t);
					br=0.95;
					bg=0.68;
					bb=0.22;
					sh=0.;
				}
			}
		}
		else if(obKind==3)
		{
			// plaster: matte, a fine grain
			int gr=ObHash(int(ox*36.+500.),int(oy*36.+500.),int(oz*36.+500.));
			lit=lit*(0.94+(gr&15)*0.008);
		}
		r=br*lit*255.+sh;
		g=bg*lit*255.+sh;
		b=bb*lit*255.+sh;
		if(obKind==1)
		{
			// plastic: a faint reflection, stronger at the rim
			float f=0.06+0.25*(1.+nz)*(1.+nz);
			r=r+er*f*0.5;
			g=g+eg*f*0.5;
			b=b+eb*f*0.5;
		}
	}
	return (ObClamp(int(r))<<16)|(ObClamp(int(g))<<8)|ObClamp(int(b));
}

// What ObShade paints with: colour c as plastic, or the material (c<0).
void ObSetColour(int c)
{
	if(c<0)
	{
		obBR=obMR;
		obBG=obMG;
		obBB=obMB;
		obKind=obMKind;
		return;
	}
	obBR=((c>>16)&255)/255.;
	obBG=((c>>8)&255)/255.;
	obBB=(c&255)/255.;
	obKind=1;
}

// Ball i: a disc on the screen, each pixel at the depth and with the normal of
// the sphere there (seen square on, near enough at this distance).
void ObBall(int i, int w, int h, float f)
{
	float cx=obKX[i];
	float cy=obKY[i];
	float cz=obKZ[i];
	float wx=obM00*cx+obM01*cy+obM02*cz;
	float wy=obM10*cx+obM11*cy+obM12*cz;
	float wz=obM20*cx+obM21*cy+obM22*cz+obDist;
	if(wz<0.2)return;
	float r=obKR[i];
	float sx=obHW+wx*f/wz;
	float sy=obHH+wy*f/wz;
	float rs=r*f/wz;
	if(rs<0.3)return;
	int xa=int(sx-rs);
	int xb=int(sx+rs)+1;
	int ya=int(sy-rs);
	int yb=int(sy+rs)+1;
	if(xa<0)xa=0;
	if(ya<0)ya=0;
	if(xb>w)xb=w;
	if(yb>h)yb=h;
	ObSetColour(obKC[i]);
	float ir=1./rs;
	for(int y=ya;y<yb;y++)
	{
		float dy=(y+0.5-sy)*ir;
		float dy2=dy*dy;
		if(dy2>=1.)continue;
		obIY=(y+0.5-obHH)*obIF;
		int row=y*PIX_W;
		for(int x=xa;x<xb;x++)
		{
			float dx=(x+0.5-sx)*ir;
			float d2=dx*dx+dy2;
			if(d2>=1.)continue;
			float nz=-sqrt(1.-d2);
			float z=wz+r*nz;
			if(z<obZB[row+x])
			{
				obZB[row+x]=z;
				obIX=(x+0.5-obHW)*obIF;
				// the point on the ball in the object's own axes, for patterns
				float ox=cx+r*(obM00*dx+obM10*dy+obM20*nz);
				float oy=cy+r*(obM01*dx+obM11*dy+obM21*nz);
				float oz=cz+r*(obM02*dx+obM12*dy+obM22*nz);
				svPix[row+x]=ObShade(dx,dy,nz,ox,oy,oz);
			}
		}
	}
}

// One triangle into svPix and the depth buffer: attributes (depth, normal,
// position on the object) are planes over the screen, read at pixel centres a
// span at a time, and shaded per pixel.
void ObTri(int a, int b, int c, int w, int h)
{
	float x0=obSX[a];
	float y0=obSY[a];
	float x1=obSX[b];
	float y1=obSY[b];
	float x2=obSX[c];
	float y2=obSY[c];
	float area=(x1-x0)*(y2-y0)-(x2-x0)*(y1-y0);
	if((area<0.0001)&&(area>-0.0001))return;
	float ia=1./area;
	float ya=y0;
	if(y1<ya)ya=y1;
	if(y2<ya)ya=y2;
	float yb=y0;
	if(y1>yb)yb=y1;
	if(y2>yb)yb=y2;
	int ys=int(ya+0.5);
	int ye=int(yb+0.5);
	if(ys<0)ys=0;
	if(ye>h)ye=h;
	if(ys>=ye)return;
	// the planes: a value v at the first corner, and its change along x and y
	float v[21];
	float g[42];
	v[0]=obWZ[a];v[1]=obWZ[b];v[2]=obWZ[c];
	v[3]=obVX[a];v[4]=obVX[b];v[5]=obVX[c];
	v[6]=obVY[a];v[7]=obVY[b];v[8]=obVY[c];
	v[9]=obVZ[a];v[10]=obVZ[b];v[11]=obVZ[c];
	v[12]=obPX[a];v[13]=obPX[b];v[14]=obPX[c];
	v[15]=obPY[a];v[16]=obPY[b];v[17]=obPY[c];
	v[18]=obPZ[a];v[19]=obPZ[b];v[20]=obPZ[c];
	for(int k=0;k<7;k++)
	{
		float d1=v[k*3+1]-v[k*3];
		float d2=v[k*3+2]-v[k*3];
		g[k*2]=(d1*(y2-y0)-d2*(y1-y0))*ia;
		g[k*2+1]=(d2*(x1-x0)-d1*(x2-x0))*ia;
	}
	for(int y=ys;y<ye;y++)
	{
		float py=y+0.5;
		// where the scanline crosses the edges
		float xl=100000.;
		float xr=-100000.;
		for(int e=0;e<3;e++)
		{
			float ax=x0;
			float ay=y0;
			float bx=x1;
			float by=y1;
			if(e==1){ax=x1;ay=y1;bx=x2;by=y2;}
			if(e==2){ax=x2;ay=y2;bx=x0;by=y0;}
			if(((ay<=py)&&(by>py))||((by<=py)&&(ay>py)))
			{
				float x=ax+(py-ay)*(bx-ax)/(by-ay);
				if(x<xl)xl=x;
				if(x>xr)xr=x;
			}
		}
		int xs=int(xl+0.5);
		int xe=int(xr+0.5);
		if(xs<0)xs=0;
		if(xe>w)xe=w;
		if(xs>=xe)continue;
		float dx=xs+0.5-x0;
		float dy=py-y0;
		float z=v[0]+g[0]*dx+g[1]*dy;
		float zx=g[0];
		int row=y*PIX_W;
		float nx=v[3]+g[2]*dx+g[3]*dy;
		float ny=v[6]+g[4]*dx+g[5]*dy;
		float nz=v[9]+g[6]*dx+g[7]*dy;
		float ox=v[12]+g[8]*dx+g[9]*dy;
		float oy=v[15]+g[10]*dx+g[11]*dy;
		float oz=v[18]+g[12]*dx+g[13]*dy;
		obIY=(py-obHH)*obIF;
		for(int x=xs;x<xe;x++)
		{
			if(z<obZB[row+x])
			{
				obZB[row+x]=z;
				obIX=(x+0.5-obHW)*obIF;
				svPix[row+x]=ObShade(nx,ny,nz,ox,oy,oz);
			}
			z=z+zx;
			nx=nx+g[2];
			ny=ny+g[4];
			nz=nz+g[6];
			ox=ox+g[8];
			oy=oy+g[10];
			oz=oz+g[12];
		}
	}
}

// A new object, unlike the last, in any material.
void ObPick()
{
	int s=ToyRandom()%(OB_SHAPES-1);
	if(s>=obShape)s++;
	obShape=s;
	ObMaterial(ToyRandom()%OB_MATS);
	obMol=ToyRandom()%OB_MOLS;
	obAge=0;
	ObBuild();
}

void ObInit()
{
	obShape=ToyRandom()%OB_SHAPES;
	obMol=ToyRandom()%OB_MOLS;
	obFade=0;
	ObMakeEnv();
	ObMaterial(ToyRandom()%OB_MATS);
	obAge=0;
	ObBuild();
}

void ObFrame()
{
	obAge++;
	obT=obT+0.05;
	if(obT>1000.)obT=obT-1000.;
	if(obShape==1)ObBuild();
	if(obFade>0)
	{
		obFade++;
		if(obFade>OB_FADE)
		{
			ObPick();
			obFade=-OB_FADE;
		}
		return;
	}
	if(obFade<0)obFade++;
	if(obAge>OB_LIFE)obFade=1;
}

void ObDraw(int w, int h)
{
	// the backdrop: dark, lighter towards the floor
	for(int y=0;y<h;y++)
	{
		int c=ToyMix(0x14161C,0x2C2A2A,y*256/h);
		int row=y*PIX_W;
		for(int x=0;x<w;x++)
		{
			svPix[row+x]=c;
			obZB[row+x]=1000000.;
		}
	}
	// the turn: about the upright, then forward, a little roll; a molecule mostly
	// about the upright
	float t=obAge*0.05;
	float ay=t*0.5;
	float ax=t*0.31+0.4;
	float az=0.3*sin(t*0.2);
	if(obShape==5)
	{
		ax=0.3*sin(t*0.23)+0.2;
		az=0.12*sin(t*0.17);
		if(obMolFlat)
		{
			// facing you, rocking a little, rolling round
			ay=0.55*sin(t*0.29);
			ax=0.45*sin(t*0.23);
			az=t*0.25;
		}
	}
	float cy=cos(ay);
	float sy=sin(ay);
	float cx=cos(ax);
	float sx=sin(ax);
	float cz=cos(az);
	float sz=sin(az);
	// R = Rz * Rx * Ry
	float a00=cy;
	float a01=0.;
	float a02=sy;
	float a10=sx*sy;
	float a11=cx;
	float a12=-sx*cy;
	float a20=-cx*sy;
	float a21=sx;
	float a22=cx*cy;
	obM00=cz*a00-sz*a10;
	obM01=cz*a01-sz*a11;
	obM02=cz*a02-sz*a12;
	obM10=sz*a00+cz*a10;
	obM11=sz*a01+cz*a11;
	obM12=sz*a02+cz*a12;
	obM20=a20;
	obM21=a21;
	obM22=a22;
	float dist=obDist;
	float f=w*0.46*dist/obFitH;
	if(h*0.46*dist/obFitV<f)f=h*0.46*dist/obFitV;
	float hw=w*0.5;
	float hh=h*0.5;
	obHW=hw;
	obHH=hh;
	obIF=1./f;
	for(int i=0;i<obNV;i++)
	{
		float x=obPX[i];
		float y=obPY[i];
		float z=obPZ[i];
		float wx=obM00*x+obM01*y+obM02*z;
		float wy=obM10*x+obM11*y+obM12*z;
		float wz=obM20*x+obM21*y+obM22*z+dist;
		obWX[i]=wx;
		obWY[i]=wy;
		obWZ[i]=wz;
		obSX[i]=hw+wx*f/wz;
		obSY[i]=hh+wy*f/wz;
		x=obNX[i];
		y=obNY[i];
		z=obNZ[i];
		obVX[i]=obM00*x+obM01*y+obM02*z;
		obVY[i]=obM10*x+obM11*y+obM12*z;
		obVZ[i]=obM20*x+obM21*y+obM22*z;
	}
	obCurCol=-1;
	ObSetColour(-1);
	for(int k=0;k<obNT;k++)
	{
		int a=obTA[k];
		int b=obTB[k];
		int c=obTC[k];
		float ux=obWX[b]-obWX[a];
		float uy=obWY[b]-obWY[a];
		float uz=obWZ[b]-obWZ[a];
		float vx=obWX[c]-obWX[a];
		float vy=obWY[c]-obWY[a];
		float vz=obWZ[c]-obWZ[a];
		float nx=uy*vz-uz*vy;
		float ny=uz*vx-ux*vz;
		float nz=ux*vy-uy*vx;
		// facing away from the eye at the origin
		if(nx*obWX[a]+ny*obWY[a]+nz*obWZ[a]>=0.)continue;
		if(obTCol[k]!=obCurCol)
		{
			obCurCol=obTCol[k];
			ObSetColour(obCurCol);
		}
		ObTri(a,b,c,w,h);
	}
	for(int i=0;i<obNK;i++)
		ObBall(i,w,h,f);
	obCurCol=-1;
	ObSetColour(-1);
	if(obFade!=0)
	{
		int fd=obFade;
		if(fd<0)fd=-fd;
		int l=256-fd*256/OB_FADE;
		for(int y=0;y<h;y++)
			for(int x=0;x<w;x++)
				svPix[y*PIX_W+x]=LmLight(svPix[y*PIX_W+x],l);
	}
}

// A cube level 0..5, fractions too, as a channel 0..255.
int FlLevel(float l)
{
	if(l<=0.)return 0;
	if(l>=5.)return 255;
	if(l<1.)return int(l*95.);
	return int(55.+l*40.);
}

// ---- nyan cat
// After the Nyan example (examples/graphic/Nyan/nyan.cpp): a pop-tart cat bobbing on a
// rainbow among sparkling stars. The example's shapes, in its own coordinates (a
// 512 x 512 canvas), are filled pixel by pixel by inside tests, scaled to the screen
// with the cat in the middle; the rainbow reaches to the left edge and the stars
// cover the screen. Outlines are a pixel wide at least, small dots a pixel.
#define NY_WAVE 1024
float nyS=1.;          // pixels per unit of the example
float nyOX=0.;         // the example's point at pixel 0,0
float nyOY=0.;
int nyW=0;
int nyH=0;
int nySeed=1;
float nyWave[NY_WAVE]; // the rainbow's middle line, a point every 4 units leftwards

int NyRand()
{
	nySeed=(nySeed*1103515245+12345)&0x7ffffff;
	return nySeed;
}

float NyAbs(float x)
{
	if(x<0.)return -x;
	return x;
}

// At least half a pixel, in the example's units.
float NyThick(float r)
{
	float m=0.5/nyS;
	if(r<m)return m;
	return r;
}

// The pixel rows or columns that units a..b cover, clipped to 0..n-1.
int NyLo(float a, float o, int n)
{
	int p=int((a-o)*nyS)-1;
	if(p<0)p=0;
	return p;
}

int NyHi(float b, float o, int n)
{
	float f=(b-o)*nyS+1.;
	if(f<0.)return -1;
	int p=int(f);
	if(p>n-1)p=n-1;
	return p;
}

// The pixels within r of the segment a-b.
void NySeg(float ax, float ay, float bx, float by, float r, int col)
{
	r=NyThick(r);
	float x0=ax;
	float x1=bx;
	if(bx<ax)
	{
		x0=bx;
		x1=ax;
	}
	float y0=ay;
	float y1=by;
	if(by<ay)
	{
		y0=by;
		y1=ay;
	}
	int px0=NyLo(x0-r,nyOX,nyW);
	int px1=NyHi(x1+r,nyOX,nyW);
	int py0=NyLo(y0-r,nyOY,nyH);
	int py1=NyHi(y1+r,nyOY,nyH);
	float dx=bx-ax;
	float dy=by-ay;
	float l=dx*dx+dy*dy;
	float rr=r*r;
	for(int py=py0;py<=py1;py++)
	{
		float v=nyOY+(py+0.5)/nyS;
		for(int px=px0;px<=px1;px++)
		{
			float u=nyOX+(px+0.5)/nyS;
			float t=0.;
			if(l>0.)
			{
				t=((u-ax)*dx+(v-ay)*dy)/l;
				if(t<0.)t=0.;
				if(t>1.)t=1.;
			}
			float ex=ax+t*dx-u;
			float ey=ay+t*dy-v;
			if(ex*ex+ey*ey<=rr)svPix[py*PIX_W+px]=col;
		}
	}
}

void NyDot(float x, float y, float r, int col)
{
	NySeg(x,y,x,y,r,col);
}

// A rectangle x,y w x h with corners of radius r, grown by g (shrunk if negative).
void NyRect(float x, float y, float w, float h, float r, float g, int col)
{
	float cx=x+w/2.;
	float cy=y+h/2.;
	float hx=w/2.-r;
	float hy=h/2.-r;
	float rg=r+g;
	if(rg<0.)return;
	int px0=NyLo(x-g,nyOX,nyW);
	int px1=NyHi(x+w+g,nyOX,nyW);
	int py0=NyLo(y-g,nyOY,nyH);
	int py1=NyHi(y+h+g,nyOY,nyH);
	for(int py=py0;py<=py1;py++)
	{
		float qy=NyAbs(nyOY+(py+0.5)/nyS-cy)-hy;
		if(qy<0.)qy=0.;
		for(int px=px0;px<=px1;px++)
		{
			float qx=NyAbs(nyOX+(px+0.5)/nyS-cx)-hx;
			if(qx<0.)qx=0.;
			if(qx*qx+qy*qy<=rg*rg)svPix[py*PIX_W+px]=col;
		}
	}
}

// The head at offset ox,oy, grown by g: the lower part of an ellipse under the line
// y=260 and the polygon of the ears above it.
float nyHeadX[8];
float nyHeadY[8];

int NyInHead(float u, float v, float g)
{
	float ex=(u-322.5)/(42.+g);
	float ey=(v-277.56)/(39.+g);
	if((v>=260.-g)&&(ex*ex+ey*ey<=1.))return 1;
	int in=0;
	int j=7;
	for(int i=0;i<8;i++)
	{
		float xi=nyHeadX[i];
		float yi=nyHeadY[i];
		float xj=nyHeadX[j];
		float yj=nyHeadY[j];
		if((yi>v)!=(yj>v))
			if(u<(xj-xi)*(v-yi)/(yj-yi)+xi)in=!in;
		if(g>0.)
		{
			float dx=xj-xi;
			float dy=yj-yi;
			float t=((u-xi)*dx+(v-yi)*dy)/(dx*dx+dy*dy);
			if(t<0.)t=0.;
			if(t>1.)t=1.;
			float qx=xi+t*dx-u;
			float qy=yi+t*dy-v;
			if(qx*qx+qy*qy<=g*g)return 1;
		}
		j=i;
	}
	return in;
}

void NyHead(float ox, float oy, float g, int col)
{
	int px0=NyLo(280.+ox-g,nyOX,nyW);
	int px1=NyHi(365.+ox+g,nyOX,nyW);
	int py0=NyLo(237.+oy-g,nyOY,nyH);
	int py1=NyHi(318.+oy+g,nyOY,nyH);
	for(int py=py0;py<=py1;py++)
	{
		float v=nyOY+(py+0.5)/nyS-oy;
		for(int px=px0;px<=px1;px++)
			if(NyInHead(nyOX+(px+0.5)/nyS-ox,v,g))svPix[py*PIX_W+px]=col;
	}
}

// The lower half of a ring: centre x,y, radius r, line half width hw.
void NySmile(float x, float y, float r, float hw, int col)
{
	hw=NyThick(hw);
	int px0=NyLo(x-r-hw,nyOX,nyW);
	int px1=NyHi(x+r+hw,nyOX,nyW);
	int py0=NyLo(y-hw,nyOY,nyH);
	int py1=NyHi(y+r+hw,nyOY,nyH);
	for(int py=py0;py<=py1;py++)
	{
		float v=nyOY+(py+0.5)/nyS-y;
		if(v<-hw)continue;
		for(int px=px0;px<=px1;px++)
		{
			float u=nyOX+(px+0.5)/nyS-x;
			float d=sqrt(u*u+v*v)-r;
			if(v<0.)d=sqrt((NyAbs(u)-r)*(NyAbs(u)-r)+v*v);
			if(NyAbs(d)<=hw)svPix[py*PIX_W+px]=col;
		}
	}
}

// A star's sparkle at phase t (0..1 of its cycle): it grows, opens and is gone,
// drifting left.
void NyStar(float x, float y, float t)
{
	t=(t-int(t))*30.;
	x=x-t*100.;
	if(t<1.)
	{
		float r1=t*t*30.;
		if(r1>25.)r1=25.;
		float r2=t*50.;
		if(r2>25.)r2=25.;
		NySeg(x+r1,y,x+r2,y,1.5,0xffffff);
		NySeg(x-r1,y,x-r2,y,1.5,0xffffff);
		NySeg(x,y+r1,x,y+r2,1.5,0xffffff);
		NySeg(x,y-r1,x,y-r2,1.5,0xffffff);
		if(t>.5)
		{
			NyDot(x+17.,y+17.,1.5,0xffffff);
			NyDot(x-17.,y+17.,1.5,0xffffff);
			NyDot(x+17.,y-17.,1.5,0xffffff);
			NyDot(x-17.,y-17.,1.5,0xffffff);
		}
	}
	if((t<1.25)&&(t>.5))NyDot(x,y,1.5,0xffffff);
}

void NyDraw(int w, int h)
{
	float T=svT*0.05;
	nyW=w;
	nyH=h;
	nyS=w/440.;
	if(h/280.<nyS)nyS=h/280.;
	if(nyS<=0.)return;
	nyOX=256.-w/2./nyS;
	nyOY=262.-h/2./nyS;
	// the night, lighter in the middle as the example's soft round glow
	for(int py=0;py<h;py++)
	{
		float v=nyOY+(py+0.5)/nyS-256.;
		for(int px=0;px<w;px++)
		{
			float u=nyOX+(px+0.5)/nyS-256.;
			int k=int(sqrt(u*u+v*v)*256./240.);
			if(k>256)k=256;
			svPix[py*PIX_W+px]=ToyMix(0x1060a0,0x000040,k);
		}
	}
	// stars, as many to the area as in the example
	float sw=w/nyS+125.;
	float sh=h/nyS;
	int n=int(200.*sw*sh/262144.);
	if(n<20)n=20;
	if(n>600)n=600;
	nySeed=73661561;
	for(int i=0;i<n;i++)
	{
		float x=nyOX+(NyRand()%10000)*sw/10000.;
		float y=nyOY+(NyRand()%10000)*sh/10000.;
		NyStar(x,y,T*.05+(NyRand()%200)*.005);
	}
	// the rainbow: one wavy line, six bands along it
	float t=T*12.;
	int steps=int((200.-nyOX)/4.)+3;
	if(steps>NY_WAVE)steps=NY_WAVE;
	if(steps<2)steps=2;
	float y=-cos(t)*3.;
	for(int i=0;i<steps;i++)
	{
		nyWave[i]=y;
		int j=i;
		if(j>50)j=50;
		y=y+sin(i*.35-t)*(1.2+j*.05);
	}
	int px1=NyHi(206.,nyOX,w);
	for(int px=0;px<=px1;px++)
	{
		float f=(200.-(nyOX+(px+0.5)/nyS))/4.;
		if(f<0.)f=0.;
		int i=int(f);
		if(i>steps-2)i=steps-2;
		float k=f-i;
		float mid=nyWave[i]+(nyWave[i+1]-nyWave[i])*k;
		int py0=NyLo(215.+mid,nyOY,h);
		int py1=NyHi(302.+mid,nyOY,h);
		for(int py=py0;py<=py1;py++)
		{
			float v=nyOY+(py+0.5)/nyS-mid;
			int c=-1;
			if(NyAbs(v-226.)<=11.)c=0xff0000;
			if(NyAbs(v-240.)<=11.)c=0xff8000;
			if(NyAbs(v-254.)<=11.)c=0xffff00;
			if(NyAbs(v-268.)<=11.)c=0x00ff00;
			if(NyAbs(v-282.)<=11.)c=0x20a0ff;
			if(NyAbs(v-294.)<=8.)c=0x6000ff;
			if(c>=0)svPix[py*PIX_W+px]=c;
		}
	}
	// the cat bobs
	float bdx=-sin(t)*5.;
	float bdy=cos(t)*3.;
	float ol=1.5;
	if(ol<1./nyS)ol=1./nyS;
	// the tail and the legs, black under grey
	float tx[9];
	float ty[9];
	float ax=200.+bdx;
	float ay=270.+bdy;
	for(int i=0;i<9;i++)
	{
		float s=i/8.;
		float a=(1.-s)*(1.-s)*(1.-s);
		float b=3.*(1.-s)*(1.-s)*s;
		float c=3.*(1.-s)*s*s;
		float d=s*s*s;
		tx[i]=a*ax+b*(ax-10.)+c*(ax-20.)+d*(ax-40.+bdy);
		ty[i]=a*ay+b*(ay-5.)+c*(ay+bdy*2.-5.)+d*(ay-bdy*4.-10.);
	}
	float lx[8];
	lx[0]=235.;lx[1]=305.+bdx;lx[2]=235.-bdy*2.5-5.;lx[3]=320.+bdx;
	lx[4]=315.;lx[5]=305.-bdy;lx[6]=315.-bdx*2.;lx[7]=320.-bdy;
	float fx[8];
	fx[0]=210.;fx[1]=305.-bdy;fx[2]=210.-bdx*2.5-5.;fx[3]=320.-bdy;
	fx[4]=290.;fx[5]=305.-bdx*1.4;fx[6]=290.+bdy*3.;fx[7]=320.-bdx*1.4;
	for(int pass=0;pass<2;pass++)
	{
		int col=0x000000;
		float r=6.5;
		if(pass==1)
		{
			col=0xa0a0a0;
			r=4.;
		}
		for(int i=0;i<8;i++)
			NySeg(tx[i],ty[i],tx[i+1],ty[i+1],r,col);
		NySeg(lx[0],lx[1],lx[2],lx[3],r,col);
		NySeg(lx[4],lx[5],lx[6],lx[7],r,col);
	}
	for(int pass=0;pass<2;pass++)
	{
		int col=0x000000;
		float r=6.5;
		if(pass==1)
		{
			col=0xa0a0a0;
			r=4.;
		}
		NySeg(fx[0],fx[1],fx[2],fx[3],r,col);
		NySeg(fx[4],fx[5],fx[6],fx[7],r,col);
	}
	// the pop-tart: crust, icing, sprinkles
	NyRect(200.+bdx,210.+bdy,135.,100.,15.,ol,0x000000);
	NyRect(200.+bdx,210.+bdy,135.,100.,15.,-ol,0xe0c0a0);
	NyRect(210.+bdx,220.+bdy,115.,80.,20.,0.,0xff90f0);
	char* sp="5174251612";
	for(int i=0;i<10;i++)
		NyDot(215.+bdx+i*10.,220.+bdy+(sp[i]-'0')*10.,2.,0xff30d0);
	// the head
	float hx=bdx*.75;
	float hy=bdy*.75-bdx*.8;
	NyHead(hx,hy,ol,0x000000);
	NyHead(hx,hy,0.,0xa0a0a0);
	NyDot(295.+hx,290.+hy,3.5,0xf0a0a0);
	NyDot(355.+hx,290.+hy,3.5,0xf0a0a0);
	NyDot(305.+hx,278.+hy,4.,0x000000);
	NyDot(348.+hx,278.+hy,4.,0x000000);
	NyDot(302.+hx,276.+hy,1.5,0xffffff);
	NyDot(345.+hx,276.+hy,1.5,0xffffff);
	NySmile(317.+hx,290.+hy,10.,1.5,0x000000);
	NySmile(337.+hx,290.+hy,10.,1.5,0x000000);
	NyDot(330.+hx,282.+hy,1.5,0x000000);
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
	ToyRamp(&svPlasma[0],256,"005 055 050 550 500 505",1);
	RdInit();
	FdInit();
	LmMake(ToyRandom()%4);
	mzFade=0;
	mzMapOn=0;
	mzFull=0;
	MzMake(ToyRandom()%MZ_WORLDS);
	obT=0.;
	ObInit();
	char* hp="285 260 360 260 355 238 348 238 332 253 310 253 294 238 287 238";
	int hi=0;
	for(int k=0;k<16;k++)
	{
		int v=0;
		while(hp[hi]==' ')hi++;
		while((hp[hi]>='0')&&(hp[hi]<='9'))
		{
			v=v*10+hp[hi]-'0';
			hi++;
		}
		if(k%2)nyHeadY[k/2]=v;
		else nyHeadX[k/2]=v;
	}
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
	// black, deep red, red, orange, yellow, white: on a scale n of 0..31, red climbs
	// the colour cube's levels (0, 95, 135 ... 255) over 0..15, green over 12..27,
	// blue over 26..31, read between the levels
	for(int k=0;k<256;k++)
	{
		float n=k*31./255.;
		flCol[k]=(FlLevel(n/3.)<<16)|(FlLevel((n-12.)/3.)<<8)|FlLevel(n-26.);
	}
}

// Whether kind is on screen: full screen or in a window.
int SvShown(int kind)
{
	if(svActive&&(svCur==kind))return 1;
	for(int i=0;i<TV_MAXWIN;i++)
		if(tvWin[i].used)
			if((tvWin[i].type==W_SAVER)&&(tvWin[i].a==kind))return 1;
	return 0;
}

// Moves every animation one frame on, the rain and the fire at screen size. The
// two simulations, the costly ones, move only while shown.
void SvStep()
{
	SvInit();
	svT++;
	if(SvShown(SV_REACT))RdFrame();
	if(SvShown(SV_FLUID))FdFrame();
	if(SvShown(SV_LAND))LmFrame();
	if(SvShown(SV_MAZE))MzFrame();
	if(SvShown(SV_OBJ))ObFrame();
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

// Four sine waves summed (0..252) pick a colour from the cycle, which turns a step
// each frame: about once in 50 frames.
void PlasmaDraw(int w, int h)
{
	int t=svT;
	for(int y=0;y<h;y++)
	{
		int row=y*PIX_W;
		int a=svSin[(y*5-t*2)&255];
		for(int x=0;x<w;x++)
		{
			int v=svSin[(x*4+t)&255]+a+svSin[((x+y)*3+t*3)&255]+svSin[((x*x+y*y*4)/64+t)&255];
			svPix[row+x]=svPlasma[(v*5/4+t*5)&255];
		}
	}
}

// The fire at w x h: it burns at screen size, scaled to fit.
void FlameDraw(int w, int h)
{
	int fw=tvW;
	int fh=tvH*2;
	for(int y=0;y<h;y++)
	{
		int src=(y*fh/h)*PIX_W;
		for(int x=0;x<w;x++)
		{
			// squared, so that cooling air goes dark rather than red
			int v=flHeat[src+x*fw/w];
			int n=v*v/124;
			if(n>255)n=255;
			svPix[y*PIX_W+x]=flCol[n];
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
	if((kind==SV_REACT)||(kind==SV_FLUID)||(kind==SV_LAND)||(kind==SV_MAZE)||(kind==SV_OBJ)||(kind==SV_NYAN)||(kind==SV_PLASMA)||(kind==SV_FLAME))
	{
		if(kind==SV_PLASMA)PlasmaDraw(w,h);
		else if(kind==SV_FLAME)FlameDraw(w,h);
		else if(kind==SV_REACT)RdDraw(w,h);
		else if(kind==SV_FLUID)FdDraw(w,h);
		else if(kind==SV_MAZE)MzDraw(w,h);
		else if(kind==SV_OBJ)ObDraw(w,h);
		else if(kind==SV_NYAN)NyDraw(w,h);
		else LmDraw(w,h);
		ToyShowRgb(&svPix[0],x0,y0,w,h);
		return;
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

// Starts the full screen saver showing kind (SV_RANDOM for any).
void SvStartKind(int kind)
{
	SvInit();
	svCur=kind;
	if(svCur>=SV_KINDS)svCur=ToyRandom()%SV_KINDS;
	svActive=1;
	svNext=0;
	tvScreenTaken=1;
}

void SvStart()
{
	SvStartKind(svKind);
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
	if(kind==SV_REACT)return "Reaction-diffusion";
	if(kind==SV_FLUID)return "Fluid";
	if(kind==SV_LAND)return "Landscape";
	if(kind==SV_MAZE)return "Labyrinth";
	if(kind==SV_OBJ)return "Objects";
	if(kind==SV_NYAN)return "Nyan cat";
	return "Stars";
}

// The window's title: the kind, and the pattern for reaction-diffusion.
void SvTitle(int win)
{
	TWin& t=tvWin[win];
	if(t.a==SV_REACT)snprintf(&t.title[0],48,"Reaction-diffusion: %s",RdName(rdPreset));
	else if(t.a==SV_LAND)snprintf(&t.title[0],48,"Landscape: %s",LmName(lmWorld));
	else if(t.a==SV_MAZE)snprintf(&t.title[0],48,"Labyrinth: %s",MzName(mzWorld));
	else if(t.a==SV_OBJ)
	{
		if(obShape==5)
		{
			char nm[24];
			char fm[24];
			ObPart(obMolNames,obMol,&nm[0],24);
			ObPart(obMolForms,obMol,&fm[0],24);
			snprintf(&t.title[0],48,"Objects: %s, %s",&nm[0],&fm[0]);
		}
		else snprintf(&t.title[0],48,"Objects: %s, %s",ObShapeName(obShape),ObMatName(obMat));
	}
	else StrCpy(&t.title[0],SvName(t.a),48);
}

// A saver in a window; field a is its kind. Space, 1-9 or 0 change it. In
// reaction-diffusion P picks the next pattern and R starts it again; in the fluid
// R clears it. Dragging the mouse drops chemical or stirs the fluid. Over the
// landscape the arrows steer and climb, N makes new land. In the labyrinth the
// arrows walk (the walker takes over again after five seconds), M shows the map,
// N makes a new maze. Among the objects O shows the next object (and the next
// molecule), M the next material, N picks both anew.
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
	SvTitle(win);
}

int SvWinKey(int win, int key)
{
	TWin& t=tvWin[win];
	int k=LowerKey(key);
	if((key==' ')||(key==K_ENTER))t.a=(t.a+1)%SV_KINDS;
	else if((key>='1')&&(key<='9'))t.a=key-'1';
	else if(key=='0')t.a=SV_NYAN;
	else if((t.a==SV_LAND)&&(key==K_LEFT))lmTurn=lmTurn-0.004;
	else if((t.a==SV_LAND)&&(key==K_RIGHT))lmTurn=lmTurn+0.004;
	else if((t.a==SV_LAND)&&(key==K_UP))lmLift=lmLift+4.;
	else if((t.a==SV_LAND)&&(key==K_DOWN))
	{
		lmLift=lmLift-4.;
		if(lmLift<-10.)lmLift=-10.;
	}
	else if((t.a==SV_LAND)&&(k=='n'))LmMake((lmWorld+1)%4);
	else if((t.a==SV_MAZE)&&(key==K_UP))MzStep(1.);
	else if((t.a==SV_MAZE)&&(key==K_DOWN))MzStep(-1.);
	else if((t.a==SV_MAZE)&&(key==K_LEFT))MzKeyTurn(-0.12);
	else if((t.a==SV_MAZE)&&(key==K_RIGHT))MzKeyTurn(0.12);
	else if((t.a==SV_MAZE)&&(k=='m'))mzMapOn=!mzMapOn;
	else if((t.a==SV_MAZE)&&(k=='n'))
	{
		MzMake((mzWorld+1)%MZ_WORLDS);
		mzFade=-MZ_FADE;
	}
	else if((t.a==SV_OBJ)&&(k=='o'))
	{
		// the molecules one by one, then on to the next shape
		if((obShape==5)&&(obMol<OB_MOLS-1))obMol++;
		else
		{
			obShape=(obShape+1)%OB_SHAPES;
			obMol=0;
		}
		obAge=0;
		ObBuild();
	}
	else if((t.a==SV_OBJ)&&(k=='m')&&(obShape!=5))
	{
		ObMaterial((obMat+1)%OB_MATS);
		obAge=0;
	}
	else if((t.a==SV_OBJ)&&(k=='n'))ObPick();
	else if((t.a==SV_REACT)&&(k=='p'))
	{
		RdPreset((rdPreset+1)%RD_PRESETS);
		RdSeed(6);
	}
	else if((t.a==SV_REACT)&&(k=='r'))RdReset(rdPreset);
	else if((t.a==SV_FLUID)&&(k=='r'))FdInit();
	else return 0;
	SvTitle(win);
	tvDirty=1;
	return 1;
}

// The grid point under window-relative x,y of a saver window, in svMX,svMY: the
// reaction's grid, or the fluid's (inside its border).
void SvGridAt(int win, int x, int y)
{
	TWin& t=tvWin[win];
	int w=t.w-2;
	int h=(t.h-2)*2;
	int px=x-1;
	int py=(y-1)*2+1;
	if(evHalf==0)py--;
	if(t.a==SV_REACT)
	{
		svMX=px*RD_W/w;
		svMY=py*RD_H/h;
	}
	else
	{
		svMX=1+px*FD_W/w;
		svMY=1+py*FD_H/h;
	}
}

int SvWinMouse(int win, int ev, int x, int y)
{
	TWin& t=tvWin[win];
	if((t.a!=SV_REACT)&&(t.a!=SV_FLUID))return 0;
	if((ev!=EV_MDOWN)&&(ev!=EV_MDRAG))return 0;
	if(ev==EV_MDOWN)
	{
		if((x<1)||(y<1)||(x>=t.w-1)||(y>=t.h-1))return 0;
		TvCaptureMouse(win);
	}
	int lx=svMX;
	int ly=svMY;
	SvGridAt(win,x,y);
	if(t.a==SV_REACT)
		RdBlob((svMX+RD_W)%RD_W,(svMY+RD_H)%RD_H,2);
	else if(ev==EV_MDRAG)
	{
		// a push the way the mouse went, in white
		float dx=(svMX-lx)*0.5;
		float dy=(svMY-ly)*0.5;
		FdPush(svMX,svMY,dx,dy,0.25,0.25,0.25);
	}
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
	for(int i=0;i<TV_MAXWIN;i++)
		if(tvWin[i].used&&(tvWin[i].type==W_SAVER))SvTitle(i);
	if(svActive)
	{
		TvClip(0,0,tvW,tvH);
		mzFull=1;
		SvDraw(svCur,0,0,tvW,tvH);
		mzFull=0;
		tvCurX=-1;
		TvFlush();
	}
	else tvDirty=1;
}

#endif // TVTOYS_H
