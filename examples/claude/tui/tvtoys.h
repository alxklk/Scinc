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
#define SV_KINDS 7
#define SV_RANDOM 7    // the setting only: a kind chosen at each start
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
#define RD_PAL 48
float rdU[15680];
float rdV[15680];
int rdCur=0;           // 0 or RD_N: the half that holds the present
int rdL[RD_W];         // the column to the left, and right, wrapped
int rdR[RD_W];
int rdPreset=0;
int rdAge=0;           // frames since the preset was chosen
float rdF=0.0545;
float rdK=0.062;
int rdPal[288];        // RD_PRESETS colour ramps of RD_PAL
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
// 256 x 256 and wraps; its height is made by diamond-square, its colours by height,
// sun from one side and water below a level. Far land fades into the sky.
#define LM_N 65536
#define LM_FAR 240.
#define LM_VS 0.35     // map height 0..255 to land height: hills, not needles
int lmH[LM_N];         // height 0..LM_TOP
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
int lmSky[PIX_H];      // the sky's colour per screen row
int lmHaze=0;          // the colour far land fades into
int lmKeyH[6];         // the colour ramp of this world: heights 0..255 at keys
int lmKeyC[6];
int lmSea=0;           // the water level, 0 for none
int lmWater=0;
int lmSkyTop=0;

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
		if(n==0)ToyGradient("001 013 135 255 554 543",9);
		else if(n==1)ToyGradient("000 010 031 152 454 555",9);
		else if(n==2)ToyGradient("000 102 214 435 555",11);
		else if(n==3)ToyGradient("100 300 520 541 555",11);
		else if(n==4)ToyGradient("000 011 033 254 553",11);
		else ToyGradient("001 003 014 035 155 555",9);
		for(int i=0;i<RD_PAL;i++)
		{
			int k=i*toyGradN/RD_PAL;
			rdPal[n*RD_PAL+i]=toyGrad[k];
		}
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

// a+(b-a)*t for each channel of two 0xRRGGBB colours, t 0..256
int LmMix(int a, int b, int t)
{
	int r=((a>>16)&255)+((((b>>16)&255)-((a>>16)&255))*t>>8);
	int g=((a>>8)&255)+((((b>>8)&255)-((a>>8)&255))*t>>8);
	int bl=(a&255)+(((b&255)-(a&255))*t>>8);
	return (r<<16)|(g<<8)|bl;
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
	return LmMix(lmKeyC[i],lmKeyC[i+1],t);
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
	int step=256;
	float sc=1.;
	while(step>1)
	{
		int half=step/2;
		for(int y=0;y<256;y+=step)
			for(int x=0;x<256;x+=step)
			{
				int x1=(x+step)&255;
				int y1=(y+step)&255;
				float a=lmTmp[y*256+x]+lmTmp[y*256+x1]+lmTmp[y1*256+x]+lmTmp[y1*256+x1];
				lmTmp[(y+half)*256+x+half]=a*0.25+LmRand()*sc;
			}
		for(int y=0;y<256;y+=half)
		{
			int x=(y+half)%step;
			while(x<256)
			{
				float a=lmTmp[y*256+((x+half)&255)]+lmTmp[y*256+((x-half)&255)];
				a=a+lmTmp[((y+half)&255)*256+x]+lmTmp[((y-half)&255)*256+x];
				lmTmp[y*256+x]=a*0.25+LmRand()*sc;
				x+=step;
			}
		}
		step=half;
		sc=sc*0.52;
	}
	// smoothed once, then stretched to 0..255
	float lo=1000.;
	float hi=-1000.;
	for(int y=0;y<256;y++)
		for(int x=0;x<256;x++)
		{
			int i=y*256+x;
			float a=lmTmp[i]*4.+lmTmp[y*256+((x+1)&255)]+lmTmp[y*256+((x-1)&255)];
			a=a+lmTmp[((y+1)&255)*256+x]+lmTmp[((y-1)&255)*256+x];
			a=a/8.;
			lmH[i]=int(a*1000.);
			if(a<lo)lo=a;
			if(a>hi)hi=a;
		}
	int ilo=int(lo*1000.);
	int span=int(hi*1000.)-ilo+1;
	for(int i=0;i<LM_N;i++)
		lmH[i]=(lmH[i]-ilo)*255/span;
	// colours: the ramp, lit by the slope towards the sun (down and to the left)
	for(int y=0;y<256;y++)
		for(int x=0;x<256;x++)
		{
			int i=y*256+x;
			int h=lmH[i];
			int d=h-lmH[((y+2)&255)*256+((x-2)&255)];
			int l=230+d*9;
			if(l<90)l=90;
			if(l>330)l=330;
			int c=LmLight(LmRamp(h),l);
			if(h<lmSea)
			{
				// water: flat, darker with depth, a glint of the land's light
				c=LmMix(lmWater,0x000000,(lmSea-h)*3);
				lmH[i]=lmSea;
			}
			lmC[i]=c;
		}
	lmX=128.;
	lmY=128.;
	lmZ=lmH[128*256+128]*LM_VS+20.;
}

// The land's height (in LM_VS units) under x,y.
float LmHeightAt(float x, float y)
{
	return lmH[(int(y)&255)*256+(int(x)&255)]*LM_VS;
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
	if(lmX<0.)lmX=lmX+256.;
	if(lmX>=256.)lmX=lmX-256.;
	if(lmY<0.)lmY=lmY+256.;
	if(lmY>=256.)lmY=lmY-256.;
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

void LmDraw(int w, int h)
{
	int horizon=h*2/5;
	float scale=h*1.1;
	for(int y=0;y<h;y++)
	{
		int t=y*256/(horizon+1);
		if(t>256)t=256;
		lmSky[y]=LmMix(lmSkyTop,lmHaze,t);
	}
	for(int x=0;x<w;x++)
		lmYBuf[x]=h;
	float sn=sin(lmA);
	float cs=cos(lmA);
	float z=2.;
	float dz=0.5;
	while(z<LM_FAR)
	{
		// the row of samples at distance z spans the view, 90 degrees wide
		float lx=lmX+cs*z+sn*z;
		float ly=lmY+sn*z-cs*z;
		float rx=lmX+cs*z-sn*z;
		float ry=lmY+sn*z+cs*z;
		float sx=(rx-lx)/w;
		float sy=(ry-ly)/w;
		float inv=scale/z;
		int fog=int(z*z*256./(LM_FAR*LM_FAR));
		for(int x=0;x<w;x++)
		{
			int i=(int(ly)&255)*256+(int(lx)&255);
			int top=int((lmZ-lmH[i]*LM_VS)*inv)+horizon;
			if(top<0)top=0;
			int yb=lmYBuf[x];
			if(top<yb)
			{
				int c=LmMix(lmC[i],lmHaze,fog);
				for(int y=top;y<yb;y++)
					svPix[y*PIX_W+x]=c;
				lmYBuf[x]=top;
			}
			lx=lx+sx;
			ly=ly+sy;
		}
		z=z+dz;
		dz=dz+0.012;
	}
	for(int x=0;x<w;x++)
		for(int y=0;y<lmYBuf[x];y++)
			svPix[y*PIX_W+x]=lmSky[y];
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
	RdInit();
	FdInit();
	LmMake(ToyRandom()%4);
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
	if(kind==SV_REACT)RdDraw(w,h);
	else if((kind==SV_FLUID)||(kind==SV_LAND))
	{
		if(kind==SV_FLUID)FdDraw(w,h);
		else LmDraw(w,h);
		ToyShowRgb(&svPix[0],x0,y0,w,h);
		return;
	}
	else if(kind==SV_PLASMA)
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
	return "Stars";
}

// The window's title: the kind, and the pattern for reaction-diffusion.
void SvTitle(int win)
{
	TWin& t=tvWin[win];
	if(t.a==SV_REACT)snprintf(&t.title[0],48,"Reaction-diffusion: %s",RdName(rdPreset));
	else if(t.a==SV_LAND)snprintf(&t.title[0],48,"Landscape: %s",LmName(lmWorld));
	else StrCpy(&t.title[0],SvName(t.a),48);
}

// A saver in a window; field a is its kind. Space or 1-7 change it. In
// reaction-diffusion P picks the next pattern and R starts it again; in the fluid
// R clears it. Dragging the mouse drops chemical or stirs the fluid. Over the
// landscape the arrows steer and climb, N makes new land.
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
	else if((key>='1')&&(key<='7'))t.a=key-'1';
	else if((t.a==SV_LAND)&&(key==K_LEFT))lmTurn=lmTurn-0.004;
	else if((t.a==SV_LAND)&&(key==K_RIGHT))lmTurn=lmTurn+0.004;
	else if((t.a==SV_LAND)&&(key==K_UP))lmLift=lmLift+4.;
	else if((t.a==SV_LAND)&&(key==K_DOWN))
	{
		lmLift=lmLift-4.;
		if(lmLift<-10.)lmLift=-10.;
	}
	else if((t.a==SV_LAND)&&(k=='n'))LmMake((lmWorld+1)%4);
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
