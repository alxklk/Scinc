// A graphic backend for tv.h: the same cell grid, drawn in a window instead of a
// terminal. Text and symbols use the stroke font from graphic/font.h and letters.h,
// or with TV_GFX_RASTER the interpreter's 9x16 bitmap font through stexts, which is
// much quicker, and with TV_GFX_SMALL its 6x12 one. Both fonts hold ASCII, the IBM
// PC symbols and Windows-1251 (tvrmap.h says where each character is); tvrfont9.h
// and tvrfont6.h hold the Latin-1 letters and the few signs they do not have.
// Box lines, shades and blocks are filled rectangles on whole pixels instead of the
// font's, so they join exactly from cell to cell. Pictures of TvPixels are blitted
// at the render target's pixels, and the cells over them drawn again on top.
//
// tv.h includes this when the program defines TV_GFX before including tv.h,
// at the point where everything the backend uses from tv.h has been defined.
// TV_GFX_COLS and TV_GFX_ROWS set the size of the desktop in cells, G_SCREEN_SCALE
// the starting pixel scale; Ctrl+= and Ctrl+- change it, Ctrl+0 goes back.
#ifndef TVGFX_H
#define TVGFX_H

// Scinc's preprocessor loses track of an #ifdef nested in a skipped block, so these
// settings use none: each mode sets GFX_CW first and the later ones see it is set.
#ifdef TV_GFX_SMALL
#define TV_GFX_RASTER
#define GFX_CW 6
#define GFX_CH 12
#define GFX_STEXT 0
#define GFX_COLS 150
#define GFX_ROWS 42
#define GFX_FONT6
#endif
#ifdef TV_GFX_RASTER
#define GFX_RASTER_ON
#endif
#ifndef GFX_RASTER_ON
#define GFX_CW 10
#define GFX_CH 20
#define GFX_STROKE
#endif
#ifndef GFX_CW
#define GFX_CW 9
#define GFX_CH 16
#define GFX_STEXT 1
#define GFX_FONT9
#endif
#ifndef GFX_COLS
#define GFX_COLS 100
#define GFX_ROWS 32
#endif
#ifndef TV_GFX_COLS
#define TV_GFX_COLS GFX_COLS
#endif
#ifndef TV_GFX_ROWS
#define TV_GFX_ROWS GFX_ROWS
#endif

#define G_SCREEN_WIDTH (TV_GFX_COLS*GFX_CW)
#define G_SCREEN_HEIGHT (TV_GFX_ROWS*GFX_CH)

#ifndef G_SCREEN_SCALE
#define G_SCREEN_SCALE 1
#endif

#define G_SCREEN_MODE 1
#define NO_GREEDY_EVENT
#include "graphics.h"
// ../../ws.h opens the window from a global initialiser, before main, so the check
// for a display has to come first in the same way.
int TvCheckDisplay()
{
	TvNeedDisplay();
	return 1;
}
int gfxHasDisplay=TvCheckDisplay();
#include "../../ws.h"
#ifdef GFX_RASTER_ON
#include "../tui/tvrmap.h"
#endif
#ifdef GFX_FONT6
#include "../tui/tvrfont6.h"
#endif
#ifdef GFX_FONT9
#include "../tui/tvrfont9.h"
#endif
#ifdef GFX_STROKE
#include "../../graphic/font.h"
#include "../../graphic/letters.h"
#include "../../graphic/gtext.h"
#endif

Graph tvG;
#ifndef TV_GFX_RASTER
CFont* gfxFont;
#endif
int gfxList[TV_CELLS];
int gfxN=0;
int gfxCurX=-1;
int gfxCurY=-1;
int gfxCtrl=0;
int gfxAlt=0;
int gfxShift=0;
int gfxEatChar=0;    // the key event was the keypad's + - * with a modifier: drop its character
int gfxComboSent=0;  // the key event already gave this Alt or Ctrl combination
int gfxBtn=-1;       // the mouse button held, or -1
int gfxMX=0;
int gfxMY=0;
int gfxCloseReq=0;   // the window's close button was pressed
float gfxPresented=0.0; // Time() of the last Present
int tvCloseCmd=0;    // the command the close button sends; 0 quits at once
// The window can be resized ('WMOV' brings the size), but its render target keeps
// the size it was made with, and a window smaller than it shows its top left part.
// So the grid shrinks and grows within the render target at once; only a window
// grown past it is made anew, once the size has stayed the same for a moment, as
// big as the window then is and where it was (as Sectorly does).
int gfxCols=TV_GFX_COLS;   // the grid
int gfxRows=TV_GFX_ROWS;
int gfxRtCols=TV_GFX_COLS; // the render target, in cells
int gfxRtRows=TV_GFX_ROWS;
int gfxResized=0;     // the grid changed: GfxReadEvent returns so that TvRun sees it
int gfxWantCols=0;    // the size the window was given, in cells, past the render target
int gfxWantRows=0;
float gfxSizedAt=-1.0;  // Time() it was given that size; -1 nothing to do
int gfxWinX=-1;       // the window's last real place: under a reparenting window
int gfxWinY=-1;       // manager 'WMOV' also reports its offset in the frame
int gfxOldWin=-1;     // the window made anew, whose events are left alone
// The pixel scale, G_SCREEN_SCALE at first: Ctrl+= and Ctrl+- change it, Ctrl+0
// goes back, and the program can call GfxSetScale. A new scale makes the window
// anew, as big on the screen as it was, so the grid gets more or fewer cells.
int gfxScale=G_SCREEN_SCALE;
int gfxWinW=TV_GFX_COLS*GFX_CW*G_SCREEN_SCALE;  // the window's size on the screen
int gfxWinH=TV_GFX_ROWS*GFX_CH*G_SCREEN_SCALE;

void GfxInit()
{
#ifndef TV_GFX_RASTER
	gfxFont=(CFont*)malloc(sizeof(CFont));
	CFontMaker fm;
	fm.Init(*gfxFont,6);
	MakeLetters(fm);
#endif
	SetEventCallback([](SScincEvent e)->int
	{
		if((gfxOldWin>=0)&&(e.w==gfxOldWin))return 1;
		if((e.type=='WDEL')||(e.type=='WDST'))
		{
			gfxCloseReq=1;
			return 1;
		}
		// moved or resized: x y the place, z h the size in screen pixels
		if(e.type=='WMOV')
		{
			if(e.w!=mainWin)return 1;
			int offX=0;
			int offY=0;
			wsys.GetWindowPos(mainWin,&offX,&offY);
			if((e.x!=offX)||(e.y!=offY))
			{
				gfxWinX=e.x;
				gfxWinY=e.y;
			}
			gfxWinW=e.z;
			gfxWinH=e.h;
			int c=e.z/(GFX_CW*gfxScale);
			int r=e.h/(GFX_CH*gfxScale);
			if(c>TV_MAXW)c=TV_MAXW;
			if(r>TV_MAXH)r=TV_MAXH;
			if((c>gfxRtCols)||(r>gfxRtRows))
			{
				gfxWantCols=c;
				gfxWantRows=r;
				gfxSizedAt=Time();
				return 1;
			}
			gfxSizedAt=-1.0;
			// smaller than this the window shows part of the screen
			if(c<20)c=20;
			if(r<6)r=6;
			if(c>gfxRtCols)c=gfxRtCols;
			if(r>gfxRtRows)r=gfxRtRows;
			if((c!=gfxCols)||(r!=gfxRows))
			{
				gfxCols=c;
				gfxRows=r;
				gfxResized=1;
			}
			return 1;
		}
		return 0;
	});
	tvG.rgba32(0xFF000000);
	tvG.FillRT();
}

int GfxCols()
{
	return gfxCols;
}

int GfxRows()
{
	return gfxRows;
}

int GfxCellPxW()
{
	return GFX_CW;
}

int GfxCellPxH()
{
	return GFX_CH;
}

int* gfxPix=0;          // pixels from TvAppPixels on their way to the screen
int gfxPixN=0;

// Draws the pictures of TvPixels when they changed or any of their cells did, all
// of them in order so that the front one wins where they overlap, and has the
// cells over them (a window in front, a menu) drawn again on top.
void GfxPixels()
{
	int need=TvPixChanged();
	for(int k=0;(k<tvPixN)&&!need;k++)
	{
		int* r=&tvPixR[k*9];
		for(int y=r[5];y<r[7];y++)
			for(int x=r[4];x<r[6];x++)
			{
				int i=y*TV_MAXW+x;
				if((tvCh[i]!=tvPrevCh[i])||(tvAt[i]!=tvPrevAt[i]))
					if(TvIsPixCell(tvCh[i],tvAt[i]))need=1;
			}
	}
	if(!need)return;
	for(int k=0;k<tvPixN;k++)
	{
		int* r=&tvPixR[k*9];
		int id=tvPixId[k];
		int pw=(r[6]-r[4])*GFX_CW;
		int ph=(r[7]-r[5])*GFX_CH;
		if(pw*ph>gfxPixN)
		{
			if(gfxPix)free(gfxPix);
			gfxPixN=pw*ph;
			gfxPix=(int*)malloc(gfxPixN*sizeof(int));
		}
		TvAppPixels(id,gfxPix,r[2]*GFX_CW,r[3]*GFX_CH,(r[4]-r[0])*GFX_CW,(r[5]-r[1])*GFX_CH,pw,ph);
		tvG.BitBlt(gfxPix,pw,ph,r[4]*GFX_CW,r[5]*GFX_CH);
		for(int y=r[5];y<r[7];y++)
			for(int x=r[4];x<r[6];x++)
			{
				int i=y*TV_MAXW+x;
				if(TvIsPixCell(tvCh[i],tvAt[i])&&(tvAt[i]==(TV_PIXAT|id)))
				{
					tvPrevCh[i]=tvCh[i];
					tvPrevAt[i]=tvAt[i];
				}
				else tvPrevCh[i]=-1;     // drawn again over the picture
			}
	}
	TvPixCommit();
}

// a*(256-t)/256 + b*t/256 for each channel
int GfxMix(int a, int b, int t)
{
	int r=0xFF000000;
	for(int sh=0;sh<24;sh+=8)
	{
		int ca=(a>>sh)&255;
		int cb=(b>>sh)&255;
		r=r|(((ca*(256-t)+cb*t)>>8)<<sh);
	}
	return r;
}

// ---------------------------------------------------------------- box drawing

void GfxH(int x0, int x1, int y, int th, int col)
{
	tvG.fillrect(x0,y-th/2,x1-x0+1,th,col);
}

void GfxV(int x, int y0, int y1, int th, int col)
{
	tvG.fillrect(x-th/2,y0,th,y1-y0+1,col);
}

// Arms of a light or heavy box character, as 2 bits per direction N,E,S,W:
// 0 none, 1 light, 2 heavy. Returns -1 if ch is not one of them.
int GfxArms(int ch)
{
	if(ch==0x2500)return 0x11<<2;      // ─  E,W
	if(ch==0x2501)return 0x22<<2;      // ━
	if(ch==0x2502)return 0x11;         // │  N,S
	if(ch==0x2503)return 0x22;         // ┃
	// arms packed as N | E<<2 | S<<4 | W<<6
	if(ch==0x250C)return (1<<2)|(1<<4);
	if(ch==0x250F)return (2<<2)|(2<<4);
	if(ch==0x2510)return (1<<4)|(1<<6);
	if(ch==0x2513)return (2<<4)|(2<<6);
	if(ch==0x2514)return 1|(1<<2);
	if(ch==0x2517)return 2|(2<<2);
	if(ch==0x2518)return 1|(1<<6);
	if(ch==0x251B)return 2|(2<<6);
	if(ch==0x251C)return 1|(1<<2)|(1<<4);
	if(ch==0x2523)return 2|(2<<2)|(2<<4);
	if(ch==0x2524)return 1|(1<<4)|(1<<6);
	if(ch==0x252B)return 2|(2<<4)|(2<<6);
	if(ch==0x252C)return (1<<2)|(1<<4)|(1<<6);
	if(ch==0x2533)return (2<<2)|(2<<4)|(2<<6);
	if(ch==0x2534)return 1|(1<<2)|(1<<6);
	if(ch==0x253B)return 2|(2<<2)|(2<<6);
	if(ch==0x253C)return 1|(1<<2)|(1<<4)|(1<<6);
	if(ch==0x254B)return 2|(2<<2)|(2<<4)|(2<<6);
	if(ch==0x2574)return 1<<6;
	if(ch==0x2575)return 1;
	if(ch==0x2576)return 1<<2;
	if(ch==0x2577)return 1<<4;
	if(ch==0x2578)return 2<<6;
	if(ch==0x2579)return 2;
	if(ch==0x257A)return 2<<2;
	if(ch==0x257B)return 2<<4;
	return -1;
}

void GfxArmsDraw(int arms, int px, int py, int col)
{
	int cx=px+GFX_CW/2;
	int cy=py+GFX_CH/2;
	int r=px+GFX_CW-1;
	int b=py+GFX_CH-1;
	int n=arms&3;
	int e=(arms>>2)&3;
	int s=(arms>>4)&3;
	int w=(arms>>6)&3;
	// heavy lines are 3 px and meet in a 3x3 square in the middle
	if(n==1)GfxV(cx,py,cy,1,col);
	if(n==2)GfxV(cx,py,cy+1,3,col);
	if(s==1)GfxV(cx,cy,b,1,col);
	if(s==2)GfxV(cx,cy-1,b,3,col);
	if(w==1)GfxH(px,cx,cy,1,col);
	if(w==2)GfxH(px,cx+1,cy,3,col);
	if(e==1)GfxH(cx,r,cy,1,col);
	if(e==2)GfxH(cx-1,r,cy,3,col);
}

// The double lines of window frames run 2 px either side of the middle.
int GfxDouble(int ch, int px, int py, int col)
{
	int cx=px+GFX_CW/2;
	int cy=py+GFX_CH/2;
	int r=px+GFX_CW-1;
	int b=py+GFX_CH-1;
	if(ch==0x2550)
	{
		GfxH(px,r,cy-2,1,col);
		GfxH(px,r,cy+2,1,col);
	}
	else if(ch==0x2551)
	{
		GfxV(cx-2,py,b,1,col);
		GfxV(cx+2,py,b,1,col);
	}
	else if(ch==0x2554) // ╔
	{
		GfxH(cx-2,r,cy-2,1,col);
		GfxV(cx-2,cy-2,b,1,col);
		GfxH(cx+2,r,cy+2,1,col);
		GfxV(cx+2,cy+2,b,1,col);
	}
	else if(ch==0x2557) // ╗
	{
		GfxH(px,cx+2,cy-2,1,col);
		GfxV(cx+2,cy-2,b,1,col);
		GfxH(px,cx-2,cy+2,1,col);
		GfxV(cx-2,cy+2,b,1,col);
	}
	else if(ch==0x255A) // ╚
	{
		GfxV(cx-2,py,cy+2,1,col);
		GfxH(cx-2,r,cy+2,1,col);
		GfxV(cx+2,py,cy-2,1,col);
		GfxH(cx+2,r,cy-2,1,col);
	}
	else if(ch==0x255D) // ╝
	{
		GfxV(cx+2,py,cy+2,1,col);
		GfxH(px,cx+2,cy+2,1,col);
		GfxV(cx-2,py,cy-2,1,col);
		GfxH(px,cx-2,cy-2,1,col);
	}
	else return 0;
	return 1;
}

// Checker board cells, made once for a pair of colours: phase 0 starts with a
// lit pixel at the cell's corner, phase 1 with a dark one.
int gfxChk[2*GFX_CW*GFX_CH];
int gfxChkFg=-1;
int gfxChkBg=-1;

void GfxCheckerCells(int fg, int bg)
{
	if((fg==gfxChkFg)&&(bg==gfxChkBg))return;
	gfxChkFg=fg;
	gfxChkBg=bg;
	for(int k=0;k<2;k++)
		for(int y=0;y<GFX_CH;y++)
			for(int x=0;x<GFX_CW;x++)
			{
				int c=bg;
				if(((x+y+k)&1)==0)c=fg;
				gfxChk[(k*GFX_CH+y)*GFX_CW+x]=c;
			}
}

// The braille dots of bits (dot 1 the lowest bit), as squares on a 2x4 grid.
void GfxBraille(int bits, int px, int py, int fg)
{
	int d=(GFX_CW+2)/4;
	for(int k=0;k<8;k++)
		if((bits>>k)&1)
		{
			// dots 1 2 3 down the left, 4 5 6 down the right, then 7 and 8 under them
			int c=k/3;
			int r=k%3;
			if(k>=6)
			{
				c=k-6;
				r=3;
			}
			tvG.fillrect(px+(2*c+1)*GFX_CW/4-d/2,py+(2*r+1)*GFX_CH/8-d/2,d,d,fg);
		}
}

// Draws box lines, blocks and shades as rectangles on whole pixels. Returns 0
// for the characters the font draws.
int GfxShape(int ch, int px, int py, int fg, int bg)
{
	if(ch<=32)return 1;
	if(ch<0x2000)return 0;
	int arms=GfxArms(ch);
	if(arms>=0)
	{
		GfxArmsDraw(arms,px,py,fg);
		return 1;
	}
	if(GfxDouble(ch,px,py,fg))return 1;
	if(ch==0x2591)tvG.fillrect(px,py,GFX_CW,GFX_CH,GfxMix(bg,fg,64));
	else if(ch==0x2592)tvG.fillrect(px,py,GFX_CW,GFX_CH,GfxMix(bg,fg,128));
	else if(ch==0x2593)tvG.fillrect(px,py,GFX_CW,GFX_CH,GfxMix(bg,fg,192));
	else if(ch==0x2588)tvG.fillrect(px,py,GFX_CW,GFX_CH,fg);
	else if(ch==0x2580)tvG.fillrect(px,py,GFX_CW,GFX_CH/2,fg);
	else if(ch==0x2584)tvG.fillrect(px,py+GFX_CH/2,GFX_CW,GFX_CH/2,fg);
	else if(ch==0x258C)tvG.fillrect(px,py,GFX_CW/2,GFX_CH,fg);
	else if(ch==0x2590)tvG.fillrect(px+GFX_CW/2,py,GFX_CW/2,GFX_CH,fg);
	else if(ch==G_QUADS)
	{
		tvG.fillrect(px,py,GFX_CW/2,GFX_CH/2,fg);
		tvG.fillrect(px+GFX_CW/2,py+GFX_CH/2,GFX_CW-GFX_CW/2,GFX_CH-GFX_CH/2,fg);
	}
	else if(ch==G_CHECKER)
	{
		// the phase by the cell's place on the screen, so the cells join seamlessly
		GfxCheckerCells(fg,bg);
		tvG.BitBlt(&gfxChk[((px+py)&1)*GFX_CW*GFX_CH],GFX_CW,GFX_CH,px,py);
	}
	else if(ch==G_BRAILLE)GfxBraille(0x95,px,py,fg);
	else return 0;
	return 1;
}

// ---------------------------------------------------------------- glyphs

// the font's character cell (x -1..5, y -4..12) stretched over a 10x20 cell
#ifdef TV_GFX_RASTER
char gfxOne[2];
char gfxRun[TV_MAXW+1];

// The byte that draws ch in the interpreter's fonts, or 0 when they do not have it.
int GfxByte(int ch)
{
	if(ch<128)
	{
		if(ch>32)return ch;
		return 0;
	}
	int lo=0;
	int hi=RM_N-1;
	while(lo<=hi)
	{
		int m=(lo+hi)/2;
		int c=rmCode[m];
		if(c==ch)return rmByte[m];
		if(c<ch)lo=m+1;
		else hi=m-1;
	}
	return 0;
}

// One character: through stexts when the font has it, otherwise from the glyph
// table as runs of pixels, and anything neither has as a small box.
void GfxGlyph(int ch, int px, int py, int col)
{
	int b=GfxByte(ch);
	if(b)
	{
		gfxOne[0]=b;
		gfxOne[1]=0;
		stexts(&gfxOne[0],px,py,col,GFX_STEXT);
		return;
	}
	int lo=0;
	int hi=RF_N-1;
	int k=-1;
	while(lo<=hi)
	{
		int m=(lo+hi)/2;
		int c=rfCode[m];
		if(c==ch)
		{
			k=m;
			break;
		}
		if(c<ch)lo=m+1;
		else hi=m-1;
	}
	if(k<0)
	{
		int bw=GFX_CW-4;
		int bh=GFX_CH/2;
		int by=py+GFX_CH/4;
		tvG.fillrect(px+2,by,bw,1,col);
		tvG.fillrect(px+2,by+bh-1,bw,1,col);
		tvG.fillrect(px+2,by,1,bh,col);
		tvG.fillrect(px+1+bw,by,1,bh,col);
		return;
	}
	for(int r=0;r<RF_H;r++)
	{
		int v=rfBits[k*RF_H+r];
		int start=-1;
		for(int x=0;x<=RF_W;x++)
		{
			int on=0;
			if(x<RF_W)on=(v>>(RF_W-1-x))&1;
			if(on)
			{
				if(start<0)start=x;
			}
			else if(start>=0)
			{
				tvG.fillrect(px+start,py+r,x-start,1,col);
				start=-1;
			}
		}
	}
}
#else
float gfxSX=GFX_CW/6.0;
float gfxSY=GFX_CH/16.0;

// 1 for a solid glyph (■ ▲ ♥ ...) that fills itself, 0 for strokes, -1 for none
int GfxGlyphKind(int ch)
{
	CGlyph* pgl=FindGlyph(*gfxFont,ch);
	if(!pgl)
		return -1;
	CGlyph gl=*pgl;
	if(gl.n<=0)
		return -1;
	return gl.col;
}

// Draws one character from the font, or a small empty box if it has no glyph.
void GfxGlyph(int ch, int px, int py, int col)
{
	int kind=GfxGlyphKind(ch);
	float ox=px+gfxSX;
	float oy=py+12*gfxSY;
	tvG.rgba32(col);
	if(kind==1)
	{
		DrawGlyph(tvG,*gfxFont,ch,ox,oy,gfxSX,gfxSY);
		return;
	}
	tvG.clear();
	if(kind==0)
		DrawGlyph(tvG,*gfxFont,ch,ox,oy,gfxSX,gfxSY);
	else
	{
		tvG.M(px+2,py+5);
		tvG.L(px+GFX_CW-2,py+5);
		tvG.L(px+GFX_CW-2,py+16);
		tvG.L(px+2,py+16);
		tvG.close();
	}
	tvG.fin();
	tvG.rgba32(col);
	tvG.width(1.2,1.2);
	tvG.stroke();
}
#endif

// ---------------------------------------------------------------- frames

// The window anew with c x r cells at gfxScale, where the old one was: the last
// real place less the offset in the frame.
//
// TO FIX: under the interpreter the new window sometimes comes up behind other
// windows. CWinSys cannot give a window a bigger render target, so it is deleted
// and made again. The X window it makes (Cumulus/src/ws_x11.cpp, CreateOSWindow)
// has no _NET_WM_PID, WM_CLASS or _NET_WM_USER_TIME, and when the old one goes the
// focus passes to some other window; if the button is still held on the frame
// (a pause of more than 0.25 s while resizing) the new one is mapped during the
// window manager's resize grab. Its focus stealing prevention then stacks the
// new window below the focused one. The GL backend and the native SDL build keep
// their one window and do not have this.
// - Here: make the new window before deleting the old one, so that ours still
//   has the focus when it is mapped, and wait longer before making it.
// - Better, in the interpreter: a CWinSys call that resizes a window and its
//   render target (Cumulus has ResizeOSWindow), so that it is never made anew;
//   or at least those properties on new windows, or _NET_ACTIVE_WINDOW for them.
void GfxNewWindow(int c, int r)
{
	gfxSizedAt=-1.0;
	int offX=0;
	int offY=0;
	wsys.GetWindowPos(mainWin,&offX,&offY);
	gfxOldWin=mainWin;
	wsys.DeleteWindow(mainWin);
	mainWin=wsys.CreateWindow(c*GFX_CW,r*GFX_CH,gfxScale,gfxScale,G_SCREEN_MODE);
	if(gfxWinX>=0)wsys.SetWindowPos(mainWin,gfxWinX-offX,gfxWinY-offY);
	tvG.SetActiveRT(wsys.GetWindowRT(mainWin));
	gfxRtCols=c;
	gfxRtRows=r;
	gfxCols=c;
	gfxRows=r;
	gfxResized=1;
}

// The window grown past its render target to gfxWantCols x gfxWantRows cells.
void GfxRemake()
{
	int c=gfxWantCols;
	int r=gfxWantRows;
	if(c<gfxRtCols)c=gfxRtCols;
	if(r<gfxRtRows)r=gfxRtRows;
	GfxNewWindow(c,r);
}

int GfxScale()
{
	return gfxScale;
}

int GfxScale0()
{
	return G_SCREEN_SCALE;
}

// A new pixel scale, 1 to 4; the grid gets new columns and rows at the next
// TvCheckSize.
void GfxSetScale(int s)
{
	if(s<1)s=1;
	if(s>4)s=4;
	if(s==gfxScale)return;
	gfxScale=s;
	int c=gfxWinW/(GFX_CW*s);
	int r=gfxWinH/(GFX_CH*s);
	if(c>TV_MAXW)c=TV_MAXW;
	if(r>TV_MAXH)r=TV_MAXH;
	if(c<20)c=20;
	if(r<6)r=6;
	GfxNewWindow(c,r);
}

void GfxPresent()
{
	wsys.Present(mainWin);
	gfxPresented=Time();
}

// Draws the cells that changed since the last call, like TvFlush does for a terminal.
void GfxFlush()
{
	if((tvCurX!=gfxCurX)||(tvCurY!=gfxCurY))
	{
		if(gfxCurX>=0)tvPrevCh[gfxCurY*TV_MAXW+gfxCurX]=-1;
		if(tvCurX>=0)tvPrevCh[tvCurY*TV_MAXW+tvCurX]=-1;
		gfxCurX=tvCurX;
		gfxCurY=tvCurY;
	}
	GfxPixels();
	gfxN=0;
	for(int y=0;y<tvH;y++)
	{
		int row=y*TV_MAXW;
		for(int x=0;x<tvW;x++)
		{
			int i=row+x;
			int ch=tvCh[i];
			int at=tvAt[i];
			if((ch!=tvPrevCh[i])||(at!=tvPrevAt[i]))
			{
				tvPrevCh[i]=ch;
				tvPrevAt[i]=at;
				if(ch==TV_PIXCH)
				{
					if(TvIsPixCell(ch,at))continue;   // GfxPixels drew it
					ch=' ';                           // a shadow fell on it
					tvCh[i]=' ';
				}
				int px=x*GFX_CW;
				int py=y*GFX_CH;
				int fg=TvAtFg(at);
				int bg=TvAtBg(at);
				tvG.fillrect(px,py,GFX_CW,GFX_CH,bg);
				if(!GfxShape(ch,px,py,fg,bg))
				{
					gfxList[gfxN]=i;
					gfxN++;
				}
			}
		}
	}
#ifdef TV_GFX_RASTER
	// cells side by side in one colour go to stexts as one string; the bitmap
	// glyphs fill their cells exactly, so nothing needs clipping
	int k=0;
	while(k<gfxN)
	{
		int i=gfxList[k];
		int fg=TvAtFg(tvAt[i]);
		int px=(i%TV_MAXW)*GFX_CW;
		int py=(i/TV_MAXW)*GFX_CH;
		int b=GfxByte(tvCh[i]);
		k++;
		if(!b)
		{
			GfxGlyph(tvCh[i],px,py,fg);
			continue;
		}
		gfxRun[0]=b;
		int n=1;
		while(k<gfxN)
		{
			int j=gfxList[k];
			if(j!=i+n)break;
			if(j%TV_MAXW==0)break;
			if(TvAtFg(tvAt[j])!=fg)break;
			int b2=GfxByte(tvCh[j]);
			if(!b2)break;
			gfxRun[n]=b2;
			n++;
			k++;
		}
		gfxRun[n]=0;
		stexts(&gfxRun[0],px,py,fg,GFX_STEXT);
	}
#endif
#ifndef TV_GFX_RASTER
	// the strokes, each clipped to its cell, since some glyphs reach a little
	// past it and a neighbour that does not change would keep those pixels
	// (one small stroke per cell is also faster than one big path per colour)
	for(int k=0;k<gfxN;k++)
	{
		int i=gfxList[k];
		int px=(i%TV_MAXW)*GFX_CW;
		int py=(i/TV_MAXW)*GFX_CH;
		tvG.Clip(px,py,GFX_CW,GFX_CH);
		GfxGlyph(tvCh[i],px,py,TvAtFg(tvAt[i]));
	}
	tvG.UnClip();
#endif
	if(gfxCurX>=0)
	{
		int i=gfxCurY*TV_MAXW+gfxCurX;
		tvG.fillrect(gfxCurX*GFX_CW,gfxCurY*GFX_CH+GFX_CH-3,GFX_CW,2,TvAtFg(tvAt[i]));
	}
	GfxPresent();
}

// ---------------------------------------------------------------- input

int GfxSpecialKey(int code)
{
	if(code==KEYCODE_UP)return K_UP;
	if(code==KEYCODE_DOWN)return K_DOWN;
	if(code==KEYCODE_LEFT)return K_LEFT;
	if(code==KEYCODE_RIGHT)return K_RIGHT;
	if(code==KEYCODE_PGUP)return K_PGUP;
	if(code==KEYCODE_PGDN)return K_PGDN;
	if(code==KEYCODE_HOME)return K_HOME;
	if(code==KEYCODE_END)return K_END;
	if(code==KEYCODE_INS)return K_INS;
	if(code==KEYCODE_DEL)return K_DEL;
	if(code==KEYCODE_BACKSPACE)return K_BACKSPACE;
	if(code==KEYCODE_ENTER)return K_ENTER;
	if(code==KEYCODE_ESC)return K_ESC;
	if(code==KEYCODE_TAB)
	{
		if(gfxShift)return K_BTAB;
		return K_TAB;
	}
	if((code>=KEYCODE_F1)&&(code<=KEYCODE_F12))return K_F1+code-KEYCODE_F1;
	if(gfxShift||gfxAlt||gfxCtrl)
	{
		if(code==KEYCODE_GR_ADD)return K_KPADD;
		if(code==KEYCODE_GR_SUB)return K_KPSUB;
		if(code==KEYCODE_GR_MUL)return K_KPMUL;
	}
	return 0;
}

void GfxModifier(int code, int down)
{
	if((code==KEYCODE_LCTRL)||(code==KEYCODE_RCTRL))gfxCtrl=down;
	if((code==KEYCODE_LALT)||(code==KEYCODE_RALT))gfxAlt=down;
	if((code==KEYCODE_LSHIFT)||(code==KEYCODE_RSHIFT))gfxShift=down;
}

void GfxMouseAt(int x, int y)
{
	gfxMX=x;
	gfxMY=y;
	evX=x/GFX_CW;
	evY=y/GFX_CH;
	evHalf=0;
	if(y%GFX_CH>=GFX_CH/2)evHalf=1;
	evSubX=x%GFX_CW;
	evSubY=y%GFX_CH;
}

// Turns one window event into a tv.h event, or leaves evType at EV_NONE.
void GfxEvent(SScincEvent& e)
{
	int t=e.type;
	if(t=='KBUP')
	{
		GfxModifier(e.x,0);
		if((!gfxAlt)&&(!gfxCtrl))gfxComboSent=0;
		return;
	}
	if(t=='KBDN')
	{
		int code=e.x;
		GfxModifier(code,1);
		int k=GfxSpecialKey(code);
		if(k!=0)
		{
			evType=EV_KEY;
			evKey=k;
			if((k>=K_KPADD)&&(k<=K_KPMUL))gfxEatChar=1;
			if(gfxShift)evMod=evMod|MOD_SHIFT;
			if(gfxAlt)evMod=evMod|MOD_ALT;
			if(gfxCtrl)evMod=evMod|MOD_CTRL;
			return;
		}
		if(gfxCtrl&&(!gfxAlt))
		{
			int z=0;
			if((code=='=')||(code=='+'))z=gfxScale+1;
			if(code=='-')z=gfxScale-1;
			if(code=='0')z=G_SCREEN_SCALE;
			if(z>0)
			{
				gfxComboSent=1;
				GfxSetScale(z);
				return;
			}
		}
		if((code>32)&&(code<127))
		{
			// Alt and Ctrl combinations come from the key, not from the typed character
			if(gfxAlt)
			{
				evType=EV_KEY;
				evKey=K_ALT+LowerKey(code);
				gfxComboSent=1;
			}
			else if(gfxCtrl)
			{
				int c=LowerKey(code);
				if((c>='a')&&(c<='z'))
				{
					evType=EV_KEY;
					evKey=c-'a'+1;
					gfxComboSent=1;
				}
			}
		}
		return;
	}
	if(t=='CHAR')
	{
		int c=e.x;
		if(gfxEatChar)
		{
			gfxEatChar=0;
			if((c=='+')||(c=='-')||(c=='*'))return;
		}
		if(gfxAlt||gfxCtrl)
		{
			// X sends no key code for Alt+letter, and none for any letter under
			// a non-Latin layout, so the character stands in for it
			if(gfxComboSent)
			{
				gfxComboSent=0;
				return;
			}
			if(gfxAlt)
			{
				if((c>32)&&(c<127))
				{
					evType=EV_KEY;
					evKey=K_ALT+LowerKey(c);
				}
			}
			else if((c>0)&&(c<27))
			{
				evType=EV_KEY;
				evKey=c;
			}
			return;
		}
		if((c<32)||(c==127))return;
		evType=EV_KEY;
		evKey=c;
		if(c>=128)
		{
			// a non-ASCII character goes in evUtf, as from the terminal
			if(c<0x800)
			{
				evUtf[0]=0xC0|(c>>6);
				evUtf[1]=0x80|(c&63);
				evUtfN=2;
			}
			else
			{
				evUtf[0]=0xE0|(c>>12);
				evUtf[1]=0x80|((c>>6)&63);
				evUtf[2]=0x80|(c&63);
				evUtfN=3;
			}
			evUtf[evUtfN]=0;
			evKey='?';
		}
		return;
	}
	if((t=='MLDN')||(t=='MRDN')||(t=='MMDN'))
	{
		GfxMouseAt(e.x,e.y);
		evBtn=0;
		if(t=='MMDN')evBtn=1;
		if(t=='MRDN')evBtn=2;
		gfxBtn=evBtn;
		evType=EV_MDOWN;
		return;
	}
	if((t=='MLUP')||(t=='MRUP')||(t=='MMUP'))
	{
		GfxMouseAt(e.x,e.y);
		evBtn=gfxBtn;
		if(evBtn<0)evBtn=0;
		gfxBtn=-1;
		tvPixDrag=0;
		evType=EV_MUP;
		return;
	}
	if(t=='MMOV')
	{
		int ox=evX;
		int oy=evY;
		int osx=evSubX;
		int osy=evSubY;
		GfxMouseAt(e.x,e.y);
		if(gfxBtn>=0)
			if((evX!=ox)||(evY!=oy)||(tvPixDrag&&((evSubX!=osx)||(evSubY!=osy))))
			{
				evBtn=gfxBtn;
				evType=EV_MDRAG;
			}
		return;
	}
	if((t=='MWUP')||(t=='MWDN'))
	{
		evX=gfxMX/GFX_CW;
		evY=gfxMY/GFX_CH;
		evType=EV_WHEEL;
		evWheel=1;
		if(t=='MWDN')evWheel=-1;
		return;
	}
}

SScincEvent gfxHeld;  // an event GfxPending took from the queue to look at
int gfxHeldN=0;

// 1 when input is waiting: TvRun then handles it before drawing, so a fast drag
// costs one frame for all the moves that came in meanwhile, not one each.
int GfxPending()
{
	if(gfxHeldN)return 1;
	Poll();
	if(GetScincEvent(gfxHeld))
	{
		gfxHeldN=1;
		return 1;
	}
	return 0;
}

void TvCommand(int cmd);

// Waits up to timeoutMs for the next event, like TvReadEvent does for a terminal.
void GfxReadEvent(int timeoutMs)
{
	evType=EV_NONE;
	evMod=0;
	evUtfN=0;
	float end=Time()+timeoutMs*0.001;
	while(true)
	{
		Poll();
		if((gfxSizedAt>=0.0)&&(Time()-gfxSizedAt>0.25))GfxRemake();
		if(gfxResized)
		{
			// what lay past the old grid shows now: all of it is drawn again
			gfxResized=0;
			tvG.rgba32(0xFF000000);
			tvG.FillRT();
			for(int i=0;i<TV_CELLS;i++)
				tvPrevCh[i]=-1;
			return;     // TvRun finds the new size
		}
		if(gfxCloseReq)
		{
			gfxCloseReq=0;
			if(tvCloseCmd!=0)TvCommand(tvCloseCmd);
			else tvRunning=0;
			return;
		}
		if(gfxHeldN)
		{
			gfxHeldN=0;
			GfxEvent(gfxHeld);
			if(evType!=EV_NONE)return;
		}
		SScincEvent e;
		while(GetScincEvent(e))
		{
			GfxEvent(e);
			if(evType!=EV_NONE)return;
		}
		// keep the window painted when nothing changes
		if(Time()-gfxPresented>0.5)GfxPresent();
		float left=end-Time();
		if(left<=0)return;
		if(left>0.05)left=0.05;
		WaitForScincEvent(left);
	}
}

#endif // TVGFX_H
