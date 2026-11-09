// An OpenGL backend for tv.h: the cell grid in an SDL window through the scgl
// wrapper in ../opengl (build it there with ./build_so.sh).
//
// Every glyph is drawn once, at start, into a texture atlas: the interpreter's bitmap
// font (9x16, or 6x12 with TV_GL_SMALL) from tvglfont9.h / tvglfont6.h, plus box
// lines, shades, blocks and a box for unknown characters. The screen itself is a
// second, small texture with two texels per cell: foreground and background colour,
// with the atlas slot of the glyph in their alpha bytes. A frame is one quad whose
// fragment shader finds the cell under each pixel, looks up its slot and colours,
// and mixes them by the atlas. So a flush only writes the changed cells into that
// texture and uploads the rows they are on; nothing is drawn per character.
//
// The window can be resized (the grid follows it) and zoomed: Ctrl+= and Ctrl+-
// change the pixel scale, Ctrl+0 goes back to the starting one.
//
// After each swap glFinish waits until the frame is really done, so the driver
// cannot queue frames ahead and add their delay to mouse drags. Ctrl+F12 turns
// that off and on to compare, Ctrl+F11 does the same for vsync (without it a
// frame shows at once, but may tear); the window title says which are on.
// TV_GL_NOFINISH and TV_GL_NOVSYNC start with them off.
//
// tv.h includes this instead of tvgfx.h when the program defines both TV_GFX and
// TV_GL. TV_GFX_COLS and TV_GFX_ROWS set the starting size in cells, TV_GL_SCALE
// the starting zoom.
#ifndef TVGL_H
#define TVGL_H

#define SCGL_DL "../opengl/scgl_dl"
#include "../opengl/scgl.h"

// flat blocks only: Scinc's preprocessor loses track of nested ones that are skipped
#ifdef TV_GL_SMALL
#include "../tui/tvglfont6.h"
#define GL_COLS0 150
#define GL_ROWS0 42
#endif
#ifndef TV_GL_SMALL
#include "../tui/tvglfont9.h"
#define GL_COLS0 100
#define GL_ROWS0 32
#endif
#ifndef TV_GFX_COLS
#define TV_GFX_COLS GL_COLS0
#endif
#ifndef TV_GFX_ROWS
#define TV_GFX_ROWS GL_ROWS0
#endif
#ifndef TV_GL_SCALE
#define TV_GL_SCALE 2
#endif
#ifndef TV_GL_TITLE
#define TV_GL_TITLE "tv.h"
#endif

#define GFX_CW GL_W
#define GFX_CH GL_H
#define GL_PERROW 32     // atlas slots per row
#define GL_SLOTROWS 16   // so at most 512 slots: the cell texels keep 9 bits of slot
#define GL_DIRECT 9984   // characters below 0x2700 find their slot in glSlotOf

int glAtlasW=GL_PERROW*GL_W;
int glAtlasH=GL_SLOTROWS*GL_H;
int* glAtlas;           // ints 0xAARRGGBB, the coverage in every channel
int glSlots=0;          // slots in use
int glBlank=0;          // slot of the space
int glUnknown=0;        // slot of the box for characters without a glyph
int glSlotOf[GL_DIRECT];
int glCells[38400];     // 2*TV_MAXW x TV_MAXH texels: fg | slot low byte, bg | slot high bits
int glTexAtlas=0;
int glTexCells=0;
int glProg=0;
int glUAtlas=0;
int glUCells=0;
int glUCellPx=0;
int glUScale=0;
int glUOrigin=0;
int glUWinH=0;
int glUGrid=0;
int glUCellTex=0;
int glUAtlasSize=0;
int glUGlyph=0;
int glUCursor=0;
int glWinW=0;           // drawable size in pixels
int glWinH=0;
int glScale=TV_GL_SCALE;
int glOX=0;             // where the grid starts in the window, centred
int glOY=0;
int glCurX=-1;
int glCurY=-1;
int glBtn=-1;           // the mouse button held, or -1
int glMX=0;
int glMY=0;
int glEv[5];
float glPresented=0.0;  // glwTime() of the last frame
int tvCloseCmd=0;       // the command the close button sends; 0 quits at once
int glOk=0;             // the window and the shader are there
int glFinishOn=1;       // glFinish after every swap
int glVsyncOn=1;        // swaps wait for the display's vertical blank

// ---------------------------------------------------------------- the atlas

// Sets a rectangle of a slot to coverage v (0..255), clipped to the cell.
void GlRect(int slot, int x, int y, int w, int h, int v)
{
	int x1=x+w;
	int y1=y+h;
	if(x<0)x=0;
	if(y<0)y=0;
	if(x1>GL_W)x1=GL_W;
	if(y1>GL_H)y1=GL_H;
	int sx=(slot%GL_PERROW)*GL_W;
	int sy=(slot/GL_PERROW)*GL_H;
	int pix=0xFF000000|(v*0x010101);
	for(int yy=y;yy<y1;yy++)
	{
		int row=(sy+yy)*glAtlasW+sx;
		for(int xx=x;xx<x1;xx++)
			glAtlas[row+xx]=pix;
	}
}

void GlH(int slot, int x0, int x1, int y, int th)
{
	GlRect(slot,x0,y-th/2,x1-x0+1,th,255);
}

void GlV(int slot, int x, int y0, int y1, int th)
{
	GlRect(slot,x-th/2,y0,th,y1-y0+1,255);
}

// Arms of a light or heavy box character, 2 bits per direction packed as
// N | E<<2 | S<<4 | W<<6: 0 none, 1 light, 2 heavy. -1 if ch is not one of them.
int GlArms(int ch)
{
	if(ch==0x2500)return 0x11<<2;      // ─
	if(ch==0x2501)return 0x22<<2;      // ━
	if(ch==0x2502)return 0x11;         // │
	if(ch==0x2503)return 0x22;         // ┃
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

// Draws a box line, double line, shade or block character into a slot, as
// tvgfx.h draws them: on whole pixels, so they join from cell to cell.
// Returns 0 for any other character.
int GlShape(int slot, int ch)
{
	int cx=GL_W/2;
	int cy=GL_H/2;
	int r=GL_W-1;
	int b=GL_H-1;
	int arms=GlArms(ch);
	if(arms>=0)
	{
		int n=arms&3;
		int e=(arms>>2)&3;
		int s=(arms>>4)&3;
		int w=(arms>>6)&3;
		// heavy lines are 3 px and meet in a 3x3 square in the middle
		if(n==1)GlV(slot,cx,0,cy,1);
		if(n==2)GlV(slot,cx,0,cy+1,3);
		if(s==1)GlV(slot,cx,cy,b,1);
		if(s==2)GlV(slot,cx,cy-1,b,3);
		if(w==1)GlH(slot,0,cx,cy,1);
		if(w==2)GlH(slot,0,cx+1,cy,3);
		if(e==1)GlH(slot,cx,r,cy,1);
		if(e==2)GlH(slot,cx-1,r,cy,3);
		return 1;
	}
	// double lines run 2 px either side of the middle
	if(ch==0x2550)
	{
		GlH(slot,0,r,cy-2,1);
		GlH(slot,0,r,cy+2,1);
	}
	else if(ch==0x2551)
	{
		GlV(slot,cx-2,0,b,1);
		GlV(slot,cx+2,0,b,1);
	}
	else if(ch==0x2554) // ╔
	{
		GlH(slot,cx-2,r,cy-2,1);
		GlV(slot,cx-2,cy-2,b,1);
		GlH(slot,cx+2,r,cy+2,1);
		GlV(slot,cx+2,cy+2,b,1);
	}
	else if(ch==0x2557) // ╗
	{
		GlH(slot,0,cx+2,cy-2,1);
		GlV(slot,cx+2,cy-2,b,1);
		GlH(slot,0,cx-2,cy+2,1);
		GlV(slot,cx-2,cy+2,b,1);
	}
	else if(ch==0x255A) // ╚
	{
		GlV(slot,cx-2,0,cy+2,1);
		GlH(slot,cx-2,r,cy+2,1);
		GlV(slot,cx+2,0,cy-2,1);
		GlH(slot,cx+2,r,cy-2,1);
	}
	else if(ch==0x255D) // ╝
	{
		GlV(slot,cx+2,0,cy+2,1);
		GlH(slot,0,cx+2,cy+2,1);
		GlV(slot,cx-2,0,cy-2,1);
		GlH(slot,0,cx-2,cy-2,1);
	}
	// shades mix the two colours, as tvgfx.h's GfxMix does
	else if(ch==0x2591)GlRect(slot,0,0,GL_W,GL_H,64);
	else if(ch==0x2592)GlRect(slot,0,0,GL_W,GL_H,128);
	else if(ch==0x2593)GlRect(slot,0,0,GL_W,GL_H,192);
	else if(ch==0x2588)GlRect(slot,0,0,GL_W,GL_H,255);
	else if(ch==0x2580)GlRect(slot,0,0,GL_W,GL_H/2,255);
	else if(ch==0x2584)GlRect(slot,0,GL_H/2,GL_W,GL_H/2,255);
	else if(ch==0x258C)GlRect(slot,0,0,GL_W/2,GL_H,255);
	else if(ch==0x2590)GlRect(slot,GL_W/2,0,GL_W/2,GL_H,255);
	else return 0;
	return 1;
}

// Fills the atlas and glSlotOf: the font's glyphs in glCode order, the shapes,
// then the box for unknown characters.
void GlBuildAtlas()
{
	glAtlas=malloc(glAtlasW*glAtlasH*sizeof(int));
	for(int i=0;i<glAtlasW*glAtlasH;i++)
		glAtlas[i]=0xFF000000;
	glSlots=0;
	for(int k=0;k<GL_N;k++)
	{
		int slot=glSlots;
		glSlots++;
		for(int y=0;y<GL_H;y++)
		{
			int v=glBits[k*GL_H+y];
			for(int x=0;x<GL_W;x++)
				if((v>>(GL_W-1-x))&1)
					GlRect(slot,x,y,1,1,255);
		}
	}
	glUnknown=glSlots;
	glSlots++;
	int bw=GL_W-4;
	int bh=GL_H/2;
	int by=GL_H/4;
	GlRect(glUnknown,2,by,bw,1,255);
	GlRect(glUnknown,2,by+bh-1,bw,1,255);
	GlRect(glUnknown,2,by,1,bh,255);
	GlRect(glUnknown,1+bw,by,1,bh,255);
	for(int i=0;i<GL_DIRECT;i++)
		glSlotOf[i]=glUnknown;
	for(int k=0;k<GL_N;k++)
	{
		int c=glCode[k];
		if(c<GL_DIRECT)glSlotOf[c]=k;
	}
	for(int ch=0x2500;ch<0x25A0;ch++)
	{
		if(GlShape(glSlots,ch))
		{
			glSlotOf[ch]=glSlots;
			glSlots++;
		}
	}
	glBlank=glSlotOf[32];
	for(int i=0;i<=32;i++)
		glSlotOf[i]=glBlank;
}

int GlSlot(int ch)
{
	if(ch<0)return glBlank;
	if(ch<GL_DIRECT)return glSlotOf[ch];
	int lo=0;
	int hi=GL_N-1;
	while(lo<=hi)
	{
		int m=(lo+hi)/2;
		int c=glCode[m];
		if(c==ch)return m;
		if(c<ch)lo=m+1;
		else hi=m-1;
	}
	return glUnknown;
}

// ---------------------------------------------------------------- the shader

char* glVS=
	"#version 120\n"
	"void main(){ gl_Position=gl_Vertex; }\n";

char* glFS=
	"#version 120\n"
	"uniform sampler2D atlas;\n"
	"uniform sampler2D cells;\n"
	"uniform vec2 cellPx;    // a cell on screen, in pixels\n"
	"uniform float scale;\n"
	"uniform vec2 origin;    // the grid's top left corner, y down\n"
	"uniform float winH;\n"
	"uniform vec2 grid;      // columns, rows\n"
	"uniform vec2 cellTex;   // size of the cell texture\n"
	"uniform vec2 atlasSize;\n"
	"uniform vec2 glyph;     // a glyph in the atlas, in texels\n"
	"uniform vec3 cursor;    // column, row, shown\n"
	"void main(){\n"
	"  vec2 p=vec2(gl_FragCoord.x,winH-gl_FragCoord.y)-origin;\n"
	"  vec2 c=floor(p/cellPx);\n"
	"  if(c.x<0.0||c.y<0.0||c.x>=grid.x||c.y>=grid.y){ gl_FragColor=vec4(0.0,0.0,0.0,1.0); return; }\n"
	"  vec2 g=floor((p-c*cellPx)/scale);\n"
	"  vec4 f=texture2D(cells,vec2(2.0*c.x+0.5,c.y+0.5)/cellTex);\n"
	"  vec4 b=texture2D(cells,vec2(2.0*c.x+1.5,c.y+0.5)/cellTex);\n"
	"  float slot=floor(f.a*255.0+0.5)+256.0*floor(b.a*255.0+0.5);\n"
	"  float sy=floor(slot/32.0);\n"
	"  vec2 s=vec2(slot-sy*32.0,sy);\n"
	"  float a=texture2D(atlas,(s*glyph+g+0.5)/atlasSize).r;\n"
	"  if(cursor.z>0.5 && c==cursor.xy && g.y>=glyph.y-3.0 && g.y<glyph.y-1.0) a=1.0;\n"
	"  gl_FragColor=vec4(mix(b.rgb,f.rgb,a),1.0);\n"
	"}\n";

int GlTexture()
{
	int t=glwGenTexture();
	glBindTexture(GL_TEXTURE_2D,t);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	return t;
}

void GlTitle()
{
	if(glVsyncOn)
	{
		if(glFinishOn)glwSetTitle(TV_GL_TITLE " - vsync on (Ctrl+F11), glFinish on (Ctrl+F12)");
		else glwSetTitle(TV_GL_TITLE " - vsync on (Ctrl+F11), glFinish off (Ctrl+F12)");
	}
	else
	{
		if(glFinishOn)glwSetTitle(TV_GL_TITLE " - vsync off (Ctrl+F11), glFinish on (Ctrl+F12)");
		else glwSetTitle(TV_GL_TITLE " - vsync off (Ctrl+F11), glFinish off (Ctrl+F12)");
	}
}

void GfxInit()
{
	if(!glwInit(TV_GFX_COLS*GL_W*glScale,TV_GFX_ROWS*GL_H*glScale,TV_GL_TITLE))
	{
		printf("tvgl.h: cannot open the OpenGL window\n");
		return;
	}
	glProg=glwCreateProgram(glVS,glFS);
	if(!glProg)return;
	glOk=1;
	glUAtlas=glGetUniformLocation(glProg,"atlas");
	glUCells=glGetUniformLocation(glProg,"cells");
	glUCellPx=glGetUniformLocation(glProg,"cellPx");
	glUScale=glGetUniformLocation(glProg,"scale");
	glUOrigin=glGetUniformLocation(glProg,"origin");
	glUWinH=glGetUniformLocation(glProg,"winH");
	glUGrid=glGetUniformLocation(glProg,"grid");
	glUCellTex=glGetUniformLocation(glProg,"cellTex");
	glUAtlasSize=glGetUniformLocation(glProg,"atlasSize");
	glUGlyph=glGetUniformLocation(glProg,"glyph");
	glUCursor=glGetUniformLocation(glProg,"cursor");
	GlBuildAtlas();
	glTexAtlas=GlTexture();
	glwTexImage2D(glAtlasW,glAtlasH,glAtlas);
	for(int i=0;i<2*TV_CELLS;i++)
		glCells[i]=0;
	glTexCells=GlTexture();
	glwTexImage2D(2*TV_MAXW,TV_MAXH,&glCells[0]);
	glWinW=glwWidth();
	glWinH=glwHeight();
#ifdef TV_GL_NOFINISH
	glFinishOn=0;
#endif
#ifdef TV_GL_NOVSYNC
	glVsyncOn=0;
#endif
	glwSwapInterval(glVsyncOn);
	GlTitle();
}

int GfxCols()
{
	int n=glWinW/(GL_W*glScale);
	if(n>TV_MAXW)n=TV_MAXW;
	if(n<20)n=20;
	return n;
}

int GfxRows()
{
	int n=glWinH/(GL_H*glScale);
	if(n>TV_MAXH)n=TV_MAXH;
	if(n<8)n=8;
	return n;
}

// ---------------------------------------------------------------- frames

void GlDraw()
{
	if(!glOk)return;
	glOX=(glWinW-tvW*GL_W*glScale)/2;
	glOY=(glWinH-tvH*GL_H*glScale)/2;
	if(glOX<0)glOX=0;
	if(glOY<0)glOY=0;
	glViewport(0,0,glWinW,glWinH);
	glUseProgram(glProg);
	glActiveTexture(GL_TEXTURE0+1);
	glBindTexture(GL_TEXTURE_2D,glTexCells);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D,glTexAtlas);
	glUniform1i(glUAtlas,0);
	glUniform1i(glUCells,1);
	glUniform2f(glUCellPx,GL_W*glScale,GL_H*glScale);
	glUniform1f(glUScale,glScale);
	glUniform2f(glUOrigin,glOX,glOY);
	glUniform1f(glUWinH,glWinH);
	glUniform2f(glUGrid,tvW,tvH);
	glUniform2f(glUCellTex,2*TV_MAXW,TV_MAXH);
	glUniform2f(glUAtlasSize,glAtlasW,glAtlasH);
	glUniform2f(glUGlyph,GL_W,GL_H);
	float shown=0.0;
	if(tvCurX>=0)shown=1.0;
	glUniform3f(glUCursor,tvCurX,tvCurY,shown);
	glBegin(GL_QUADS);
	glVertex2f(-1,-1);
	glVertex2f(1,-1);
	glVertex2f(1,1);
	glVertex2f(-1,1);
	glEnd();
	glwSwap();
	if(glFinishOn)glFinish();
	glPresented=glwTime();
}

// Writes the cells that changed since the last call into the cell texture,
// uploads the rows they are on, and draws the frame.
void GfxFlush()
{
	if(!glOk)return;
	int y0=TV_MAXH;
	int y1=-1;
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
				int slot=GlSlot(ch);
				int t=2*i;
				glCells[t]=(TvAtFg(at)&0xFFFFFF)|((slot&255)<<24);
				glCells[t+1]=(TvAtBg(at)&0xFFFFFF)|((slot>>8)<<24);
				if(y<y0)y0=y;
				y1=y;
			}
		}
	}
	if(y1>=0)
	{
		glBindTexture(GL_TEXTURE_2D,glTexCells);
		glwTexSubImage2D(0,y0,2*TV_MAXW,y1-y0+1,&glCells[0],y0*2*TV_MAXW);
	}
	GlDraw();
}

// ---------------------------------------------------------------- input

int GlSpecialKey(int k, int mods)
{
	if(k==GLW_KEY_UP)return K_UP;
	if(k==GLW_KEY_DOWN)return K_DOWN;
	if(k==GLW_KEY_LEFT)return K_LEFT;
	if(k==GLW_KEY_RIGHT)return K_RIGHT;
	if(k==GLW_KEY_PGUP)return K_PGUP;
	if(k==GLW_KEY_PGDN)return K_PGDN;
	if(k==GLW_KEY_HOME)return K_HOME;
	if(k==GLW_KEY_END)return K_END;
	if(k==GLW_KEY_INSERT)return K_INS;
	if(k==GLW_KEY_DELETE)return K_DEL;
	if(k==GLW_KEY_BACKSPACE)return K_BACKSPACE;
	if(k==GLW_KEY_ENTER)return K_ENTER;
	if(k==GLW_KEY_ESC)return K_ESC;
	if(k==GLW_KEY_MENU)return K_MENU;
	if(k==GLW_KEY_TAB)
	{
		if(mods&1)return K_BTAB;
		return K_TAB;
	}
	if((k>=GLW_KEY_F1)&&(k<GLW_KEY_F1+12))return K_F1+k-GLW_KEY_F1;
	return 0;
}

void GlMouseAt(int x, int y)
{
	glMX=x;
	glMY=y;
	int px=x-glOX;
	int py=y-glOY;
	if(px<0)px=0;
	if(py<0)py=0;
	int cw=GL_W*glScale;
	int chh=GL_H*glScale;
	evX=px/cw;
	evY=py/chh;
	if(evX>=tvW)evX=tvW-1;
	if(evY>=tvH)evY=tvH-1;
	evHalf=0;
	if(py%chh>=chh/2)evHalf=1;
}

// A new zoom: the grid gets new columns and rows at the next TvCheckSize.
void GlZoom(int s)
{
	if(s<1)s=1;
	if(s>8)s=8;
	glScale=s;
	GlDraw();
}

// Turns one queued window event into a tv.h event, or leaves evType at EV_NONE.
void GlEvent()
{
	int t=glEv[0];
	if(t==GLW_EV_KEY)
	{
		int k=glEv[1];
		int mods=glEv[2];
		int letter=glEv[3];
		int mod=0;
		if(mods&1)mod=mod|MOD_SHIFT;
		if(mods&2)mod=mod|MOD_CTRL;
		if(mods&4)mod=mod|MOD_ALT;
		if((mods&2)&&(k==GLW_KEY_F1+11))
		{
			glFinishOn=1-glFinishOn;
			GlTitle();
			return;
		}
		if((mods&2)&&(k==GLW_KEY_F1+10))
		{
			glVsyncOn=1-glVsyncOn;
			glwSwapInterval(glVsyncOn);
			GlTitle();
			return;
		}
		int sk=GlSpecialKey(k,mods);
		if(sk!=0)
		{
			evType=EV_KEY;
			evKey=sk;
			evMod=mod;
			return;
		}
		if(letter==0)letter=k;
		if(mods&2)
		{
			if((k=='=')||(k=='+'))
			{
				GlZoom(glScale+1);
				return;
			}
			if(k=='-')
			{
				GlZoom(glScale-1);
				return;
			}
			if(letter=='0')
			{
				GlZoom(TV_GL_SCALE);
				return;
			}
		}
		// Alt and Ctrl combinations come from the key's place, so they also work
		// under a non-Latin layout; plain characters come as text events
		if(mods&4)
		{
			if((letter>32)&&(letter<127))
			{
				evType=EV_KEY;
				evKey=K_ALT+LowerKey(letter);
			}
		}
		else if(mods&2)
		{
			if((letter>='a')&&(letter<='z'))
			{
				evType=EV_KEY;
				evKey=letter-'a'+1;
			}
		}
		return;
	}
	if(t==GLW_EV_TEXT)
	{
		int c=glEv[1];
		if(glEv[2]&6)return;
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
			else if(c<0x10000)
			{
				evUtf[0]=0xE0|(c>>12);
				evUtf[1]=0x80|((c>>6)&63);
				evUtf[2]=0x80|(c&63);
				evUtfN=3;
			}
			else
			{
				evUtf[0]=0xF0|(c>>18);
				evUtf[1]=0x80|((c>>12)&63);
				evUtf[2]=0x80|((c>>6)&63);
				evUtf[3]=0x80|(c&63);
				evUtfN=4;
			}
			evUtf[evUtfN]=0;
			evKey='?';
		}
		return;
	}
	if(t==GLW_EV_MDOWN)
	{
		GlMouseAt(glEv[1],glEv[2]);
		evBtn=glEv[3];
		glBtn=evBtn;
		evType=EV_MDOWN;
		return;
	}
	if(t==GLW_EV_MUP)
	{
		GlMouseAt(glEv[1],glEv[2]);
		evBtn=glBtn;
		if(evBtn<0)evBtn=0;
		glBtn=-1;
		evType=EV_MUP;
		return;
	}
	if(t==GLW_EV_MMOVE)
	{
		int ox=evX;
		int oy=evY;
		GlMouseAt(glEv[1],glEv[2]);
		if(glBtn>=0)
			if((evX!=ox)||(evY!=oy))
			{
				evBtn=glBtn;
				evType=EV_MDRAG;
			}
		return;
	}
	if(t==GLW_EV_WHEEL)
	{
		GlMouseAt(glMX,glMY);
		evType=EV_WHEEL;
		evWheel=1;
		if(glEv[1]<0)evWheel=-1;
		return;
	}
}

// 1 when input is waiting: TvRun then handles it before drawing, so a fast drag
// costs one frame for all the moves that came in meanwhile, not one each.
int GfxPending()
{
	if(!glOk)return 0;
	glwPoll();
	return glwEventCount()>0;
}

void TvCommand(int cmd);

// Waits up to timeoutMs for the next event, like TvReadEvent does for a terminal.
// A resize or zoom returns with no event, so that TvRun checks the size.
void GfxReadEvent(int timeoutMs)
{
	evType=EV_NONE;
	evMod=0;
	evUtfN=0;
	if(!glOk)
	{
		tvRunning=0;
		return;
	}
	float end=glwTime()+timeoutMs*0.001;
	while(true)
	{
		if(!glwPoll())
		{
			glwCancelQuit();
			if(tvCloseCmd!=0)TvCommand(tvCloseCmd);
			else tvRunning=0;
			return;
		}
		if(glwResized())
		{
			glWinW=glwWidth();
			glWinH=glwHeight();
			GlDraw();
			return;
		}
		int scale=glScale;
		while(glwNextEvent(&glEv[0]))
		{
			GlEvent();
			if(evType!=EV_NONE)return;
			if(glScale!=scale)return;
		}
		// keep the window painted when nothing changes
		if(glwTime()-glPresented>0.5)GlDraw();
		float left=end-glwTime();
		if(left<=0)return;
		int ms=left*1000;
		if(ms>50)ms=50;
		if(ms<1)ms=1;
		glwWaitEvent(ms);
	}
}

#endif // TVGL_H
