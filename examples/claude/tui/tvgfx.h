// A graphic backend for tv.h: the same cell grid, drawn in a window instead of a
// terminal. Text and symbols use the stroke font from graphic/font.h and letters.h.
// Box lines, shades and blocks are filled rectangles on whole pixels instead of the
// font's, so they join exactly from cell to cell.
//
// tv.h includes this when the program defines TV_GFX before including tv.h,
// at the point where everything the backend uses from tv.h has been defined.
// TV_GFX_COLS and TV_GFX_ROWS set the size of the desktop in cells.
#ifndef TVGFX_H
#define TVGFX_H

#ifndef TV_GFX_COLS
#define TV_GFX_COLS 100
#endif
#ifndef TV_GFX_ROWS
#define TV_GFX_ROWS 32
#endif

#define GFX_CW 10
#define GFX_CH 20

#define G_SCREEN_WIDTH 1000
#define G_SCREEN_HEIGHT 640

#ifndef G_SCREEN_SCALE
#define G_SCREEN_SCALE 1
#endif

#define G_SCREEN_MODE 1
#define NO_GREEDY_EVENT
#include "graphics.h"
#include "../../ws.h"
#include "../../graphic/font.h"
#include "../../graphic/letters.h"
#include "../../graphic/gtext.h"

Graph tvG;
CFont* gfxFont;
int gfxPal[16];
int gfxList[TV_CELLS];
int gfxN=0;
int gfxCurX=-1;
int gfxCurY=-1;
int gfxCtrl=0;
int gfxAlt=0;
int gfxShift=0;
int gfxComboSent=0;  // the key event already gave this Alt or Ctrl combination
int gfxBtn=-1;       // the mouse button held, or -1
int gfxMX=0;
int gfxMY=0;
int gfxCloseReq=0;   // the window's close button was pressed
float gfxPresented=0.0; // Time() of the last Present
int tvCloseCmd=0;    // the command the close button sends; 0 quits at once

void GfxInit()
{
	gfxPal[0]=0xFF000000;
	gfxPal[1]=0xFF0000AA;
	gfxPal[2]=0xFF00AA00;
	gfxPal[3]=0xFF00AAAA;
	gfxPal[4]=0xFFAA0000;
	gfxPal[5]=0xFFAA00AA;
	gfxPal[6]=0xFFAA5500;
	gfxPal[7]=0xFFAAAAAA;
	gfxPal[8]=0xFF555555;
	gfxPal[9]=0xFF5555FF;
	gfxPal[10]=0xFF55FF55;
	gfxPal[11]=0xFF55FFFF;
	gfxPal[12]=0xFFFF5555;
	gfxPal[13]=0xFFFF55FF;
	gfxPal[14]=0xFFFFFF55;
	gfxPal[15]=0xFFFFFFFF;
	gfxFont=(CFont*)malloc(sizeof(CFont));
	CFontMaker fm;
	fm.Init(*gfxFont,6);
	MakeLetters(fm);
	SetEventCallback([](SScincEvent e)->int
	{
		if((e.type=='WDEL')||(e.type=='WDST'))
		{
			gfxCloseReq=1;
			return 1;
		}
		return 0;
	});
	tvG.rgba32(0xFF000000);
	tvG.FillRT();
}

int GfxCols()
{
	return TV_GFX_COLS;
}

int GfxRows()
{
	return TV_GFX_ROWS;
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
	else return 0;
	return 1;
}

// ---------------------------------------------------------------- glyphs

// the font's character cell (x -1..5, y -4..12) stretched over a 10x20 cell
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

// ---------------------------------------------------------------- frames

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
				int px=x*GFX_CW;
				int py=y*GFX_CH;
				int fg=gfxPal[at&15];
				int bg=gfxPal[(at>>4)&15];
				tvG.fillrect(px,py,GFX_CW,GFX_CH,bg);
				if(!GfxShape(ch,px,py,fg,bg))
				{
					gfxList[gfxN]=i;
					gfxN++;
				}
			}
		}
	}
	// the strokes, each clipped to its cell, since some glyphs reach a little
	// past it and a neighbour that does not change would keep those pixels
	// (one small stroke per cell is also faster than one big path per colour)
	for(int k=0;k<gfxN;k++)
	{
		int i=gfxList[k];
		int px=(i%TV_MAXW)*GFX_CW;
		int py=(i/TV_MAXW)*GFX_CH;
		tvG.Clip(px,py,GFX_CW,GFX_CH);
		GfxGlyph(tvCh[i],px,py,gfxPal[tvAt[i]&15]);
	}
	tvG.UnClip();
	if(gfxCurX>=0)
	{
		int i=gfxCurY*TV_MAXW+gfxCurX;
		tvG.fillrect(gfxCurX*GFX_CW,gfxCurY*GFX_CH+GFX_CH-3,GFX_CW,2,gfxPal[tvAt[i]&15]);
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
			if(gfxShift)evMod=evMod|MOD_SHIFT;
			if(gfxAlt)evMod=evMod|MOD_ALT;
			if(gfxCtrl)evMod=evMod|MOD_CTRL;
			return;
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
		evType=EV_MUP;
		return;
	}
	if(t=='MMOV')
	{
		int ox=evX;
		int oy=evY;
		GfxMouseAt(e.x,e.y);
		if(gfxBtn>=0)
			if((evX!=ox)||(evY!=oy))
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
		if(gfxCloseReq)
		{
			gfxCloseReq=0;
			if(tvCloseCmd!=0)TvCommand(tvCloseCmd);
			else tvRunning=0;
			return;
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
