// A small Turbo Vision style text UI: overlapping windows, pull-down menus, a
// status line, modal dialogs and the usual controls, driven by keyboard and mouse.
//
// A program defines the TvApp* functions declared below, builds its menus and
// status line, opens windows and calls TvRun(-1).
//
// Scinc notes: && and || evaluate both sides, so bounds are checked in a separate
// if. Colours are VGA attributes, background<<4 | foreground, as in Turbo Vision.
#ifndef TV_H
#define TV_H

// Scinc looks for #include files from the working directory, so these headers name
// each other as ../tui/x.h, which works both here and from a sibling folder (../t2fc).
#include "../tui/term.h"

// ---------------------------------------------------------------- application hooks

void TvAppDraw(int win);                             // paint the window interior
int  TvAppKey(int win, int key);                     // return 1 if the key was used
int  TvAppMouse(int win, int ev, int x, int y);      // x,y relative to the window
void TvAppCommand(int cmd);                          // menu, status line and hot keys
void TvAppButton(int win, int ctl, int cmd);         // a non-modal window's button, or a
                                                     // slider or combo box that changed
void TvAppClosed(int win);
void TvAppListItem(int win, int ctl, int i, char* out, int max); // items of a virtual list
int  TvAppCanClose(int win);                         // return 0 to keep the window open
int  TvAppPaste(int win, char* s, int n);            // return 1 if the pasted text was used
void TvAppTick();                                    // called every tvTickMs while that is above 0
void TvAppPixels(int id, int* buf, int fullW, int fullH, int ox, int oy, int w, int h);
                                                     // pixels for TvPixels, graphic backends only
int  TvAppStatus(int win);                           // the status line context of the front
                                                     // window (see TvStatusFor)

// ---------------------------------------------------------------- keys and events

#define K_ENTER 13
#define K_TAB 9
#define K_ESC 27
#define K_BACKSPACE 127
#define K_UP 1000
#define K_DOWN 1001
#define K_LEFT 1002
#define K_RIGHT 1003
#define K_HOME 1004
#define K_END 1005
#define K_PGUP 1006
#define K_PGDN 1007
#define K_INS 1008
#define K_DEL 1009
#define K_BTAB 1010
#define K_MENU 1011  // the context menu key
#define K_F1 1101
#define K_F2 1102
#define K_F3 1103
#define K_F4 1104
#define K_F5 1105
#define K_F6 1106
#define K_F7 1107
#define K_F8 1108
#define K_F9 1109
#define K_F10 1110
#define K_F11 1111
#define K_F12 1112
#define K_ALT 2000 // K_ALT+'x'

#define EV_NONE 0
#define EV_KEY 1
#define EV_MDOWN 2
#define EV_MUP 3
#define EV_MDRAG 4
#define EV_WHEEL 5
#define EV_PASTE 6
#define EV_RDOWN 7   // TvAppMouse: a right click, or Shift+F10 with x,y at -1

#define MOD_SHIFT 1
#define MOD_ALT 2
#define MOD_CTRL 4

int evType=0;
int evKey=0;
int evX=0;
int evY=0;
int evBtn=0;
int evWheel=0;
int evHalf=-1;      // which half of the cell the mouse is in, 0 top, 1 bottom, -1 not known
int evMod=0;        // MOD_* held with a cursor or function key
char evUtf[5];      // the bytes of a non-ASCII character, whose evKey is '?'
int evUtfN=0;

#define TV_PASTEMAX 1000000
char* tvPaste;      // the text of an EV_PASTE, with LF line ends
int tvPasteN=0;
int tvPasteLost=0;  // set if it did not all fit

// ---------------------------------------------------------------- screen

#define TV_MAXW 240
#define TV_MAXH 80
#define TV_CELLS 19200
#define TV_OUTMAX 8192

#define G_DHORZ 0x2550
#define G_DVERT 0x2551
#define G_DTL 0x2554
#define G_DTR 0x2557
#define G_DBL 0x255A
#define G_DBR 0x255D
#define G_HORZ 0x2500
#define G_VERT 0x2502
#define G_TL 0x250C
#define G_TR 0x2510
#define G_BL 0x2514
#define G_BR 0x2518
#define G_LIGHT 0x2591
#define G_MEDIUM 0x2592
#define G_FULL 0x2588
#define G_LOWHALF 0x2584
#define G_UPHALF 0x2580
#define G_UP 0x25B2
#define G_DOWN 0x25BC
#define G_LEFT 0x25C4
#define G_RIGHT 0x25BA
#define G_BOX 0x25A0
#define G_ZOOM 0x2195
#define G_MENU 0x2261
#define G_DOT 0x2022

int tvCh[TV_CELLS];
int tvAt[TV_CELLS];
int tvPrevCh[TV_CELLS];
int tvPrevAt[TV_CELLS];
int tvW=80;
int tvH=25;
int clipX0=0;
int clipY0=0;
int clipX1=80;
int clipY1=25;
int tvCurX=-1;
int tvCurY=-1;
int tvCurShown=0;
int tvDirty=1;        // the screen needs drawing again
int tvVga[8];

// Colour themes. A cell keeps its VGA attribute and the theme decides the colours:
// tvFg and tvBg hold 0xFFRRGGBB for the 16 foreground and background indices, kept
// apart because the same index plays different parts, such as black text on grey
// dialogs and the black of shadows. The classic theme sends the terminal's own 16
// colours, the others 24-bit colours, or the nearest of xterm's 256 when COLORTERM does
// not say the terminal has 24-bit colour. The custom theme can use any of the three:
// tvCustDepth says which, and tvCustFg and tvCustBg hold VGA colour numbers (4), xterm
// palette numbers (8) or 0xRRGGBB (24).
#define TH_CLASSIC 0
#define TH_DARK 1
#define TH_MONO 2
#define TH_CUSTOM 3
int tvTheme=0;
int tvFg[16];
int tvBg[16];
int tvTrueColor=1;   // the terminal shows 24-bit colours; otherwise themes use xterm's 256
int tvFg256[16];     // the nearest of those 256 to tvFg and tvBg
int tvBg256[16];
int tvDepth=4;       // how the theme sends colours: 4 the terminal's 16, 8 the 256, 24 bits
int tvFgI[16];       // at depth 4 VGA numbers, at depth 8 palette numbers
int tvBgI[16];
int tvBtnShadow=0x70;  // a button's shadow: index 0 or 8, whichever text colour is darker
int tvCustDepth=4;
int tvCustFg[16];
int tvCustBg[16];

// The xterm 256 colour palette entry nearest to 0xRRGGBB: the 6x6x6 cube or the grey ramp.
int TvTo256(int c)
{
	int v[3];
	v[0]=(c>>16)&255;
	v[1]=(c>>8)&255;
	v[2]=c&255;
	int idx=16;
	int dist=0;
	for(int k=0;k<3;k++)
	{
		// the cube levels are 0, 95, 135, 175, 215, 255
		int n=0;
		if(v[k]>=48)n=1;
		if(v[k]>=115)n=(v[k]-35)/40;
		if(n>5)n=5;
		int lv=0;
		if(n>0)lv=55+n*40;
		if(k==0)idx=idx+n*36;
		if(k==1)idx=idx+n*6;
		if(k==2)idx=idx+n;
		dist=dist+(v[k]-lv)*(v[k]-lv);
	}
	int avg=(v[0]+v[1]+v[2])/3;
	int g=(avg-3)/10;
	if(g<0)g=0;
	if(g>23)g=23;
	int gv=8+g*10;
	int gd=0;
	for(int k=0;k<3;k++)
		gd=gd+(v[k]-gv)*(v[k]-gv);
	if(gd<dist)return 232+g;
	return idx;
}

// An attribute with TV_C256 set holds two colours of the xterm 256 colour palette,
// (fg<<8)|bg, which themes leave alone. Pictures use it with half blocks, see TvPix.
#define TV_C256 0x10000
int tvRgb256[256];

// An attribute with TV_RGB set holds two colours of 15 bits, 5 per channel,
// (fg<<15)|bg: 32 levels a channel where the 256 palette has 6. Sent as 24-bit colour,
// or as the nearest of the 256 when the terminal has no 24-bit colour. Test it before
// TV_C256, whose bit lies inside the fg here. See TvPixRgb.
#define TV_RGB 0x40000000

// Roles: what the desktop draws with. A role attribute, TV_ROLE|r, takes the colours
// of role r in the current theme. The built-in themes give a role the palette colours
// of its VGA attribute in tvRoleDef; the custom theme sets each one itself. A role
// with a clear background (the syntax colours and line numbers in the custom theme,
// when tvCustRBg is -1) takes the editor's background. Test TV_RGB before TV_ROLE,
// whose bit lies inside the TV_RGB fg, and TV_ROLE before TV_C256.
#define TV_ROLE 0x20000000
#define R_DESK 0
#define R_MENU 1
#define R_MENU_HOT 2
#define R_MENU_SEL 3
#define R_MENU_SELHOT 4
#define R_STATUS 5
#define R_STATUS_HOT 6
#define R_STATUS_SEL 7
#define R_STATUS_SELHOT 8
#define R_SHADOW 9
#define R_WIN 10
#define R_WIN_FRAME 11
#define R_WIN_PASSIVE 12
#define R_WIN_ICON 13
#define R_WIN_PRESSED 14
#define R_DLG 15
#define R_DLG_FRAME 16
#define R_DLG_PASSIVE 17
#define R_DLG_ICON 18
#define R_DLG_PRESSED 19
#define R_HELP 20
#define R_HELP_FRAME 21
#define R_HELP_PASSIVE 22
#define R_HELP_ICON 23
#define R_HELP_PRESSED 24
#define R_LABEL_HOT 25
#define R_BTN 26
#define R_BTN_DEF 27
#define R_BTN_FOCUS 28
#define R_BTN_HOT 29
#define R_BTN_SHADOW 30
#define R_CHECK 31
#define R_CHECK_FOCUS 32
#define R_CHECK_HOT 33
#define R_INPUT 34
#define R_INPUT_ARROW 35
#define R_SLIDER 36
#define R_SLIDER_TRACK 37
#define R_SLIDER_FOCUS 38
#define R_PROGRESS 39
#define R_LIST 40
#define R_LIST_SEL 41
#define R_LIST_FOCUS 42
#define R_SCROLL 43
#define R_COMBO_ARROW 44
#define R_ED_TEXT 45
#define R_ED_KEYWORD 46
#define R_ED_COMMENT 47
#define R_ED_STRING 48
#define R_ED_NUMBER 49
#define R_ED_PREPROC 50
#define R_ED_SYMBOL 51
#define R_ED_LINENO 52
#define R_ED_LINECUR 53
#define R_ED_SELECT 54
#define R_ED_FOUND 55
#define R_ED_MATCH 56
#define R_ED_SCROLL 57
#define R_N 58
#define A_DESK (TV_ROLE|0)
#define A_MENU (TV_ROLE|1)
#define A_MENU_HOT (TV_ROLE|2)
#define A_MENU_SEL (TV_ROLE|3)
#define A_MENU_SELHOT (TV_ROLE|4)
#define A_STATUS (TV_ROLE|5)
#define A_STATUS_HOT (TV_ROLE|6)
#define A_STATUS_SEL (TV_ROLE|7)
#define A_STATUS_SELHOT (TV_ROLE|8)
#define A_SHADOW (TV_ROLE|9)
#define A_WIN (TV_ROLE|10)
#define A_WIN_FRAME (TV_ROLE|11)
#define A_WIN_PASSIVE (TV_ROLE|12)
#define A_WIN_ICON (TV_ROLE|13)
#define A_WIN_PRESSED (TV_ROLE|14)
#define A_DLG (TV_ROLE|15)
#define A_DLG_FRAME (TV_ROLE|16)
#define A_DLG_PASSIVE (TV_ROLE|17)
#define A_DLG_ICON (TV_ROLE|18)
#define A_DLG_PRESSED (TV_ROLE|19)
#define A_HELP (TV_ROLE|20)
#define A_HELP_FRAME (TV_ROLE|21)
#define A_HELP_PASSIVE (TV_ROLE|22)
#define A_HELP_ICON (TV_ROLE|23)
#define A_HELP_PRESSED (TV_ROLE|24)
#define A_LABEL_HOT (TV_ROLE|25)
#define A_BTN (TV_ROLE|26)
#define A_BTN_DEF (TV_ROLE|27)
#define A_BTN_FOCUS (TV_ROLE|28)
#define A_BTN_HOT (TV_ROLE|29)
#define A_BTN_SHADOW (TV_ROLE|30)
#define A_CHECK (TV_ROLE|31)
#define A_CHECK_FOCUS (TV_ROLE|32)
#define A_CHECK_HOT (TV_ROLE|33)
#define A_INPUT (TV_ROLE|34)
#define A_INPUT_ARROW (TV_ROLE|35)
#define A_SLIDER (TV_ROLE|36)
#define A_SLIDER_TRACK (TV_ROLE|37)
#define A_SLIDER_FOCUS (TV_ROLE|38)
#define A_PROGRESS (TV_ROLE|39)
#define A_LIST (TV_ROLE|40)
#define A_LIST_SEL (TV_ROLE|41)
#define A_LIST_FOCUS (TV_ROLE|42)
#define A_SCROLL (TV_ROLE|43)
#define A_COMBO_ARROW (TV_ROLE|44)
#define A_ED_TEXT (TV_ROLE|45)
#define A_ED_KEYWORD (TV_ROLE|46)
#define A_ED_COMMENT (TV_ROLE|47)
#define A_ED_STRING (TV_ROLE|48)
#define A_ED_NUMBER (TV_ROLE|49)
#define A_ED_PREPROC (TV_ROLE|50)
#define A_ED_SYMBOL (TV_ROLE|51)
#define A_ED_LINENO (TV_ROLE|52)
#define A_ED_LINECUR (TV_ROLE|53)
#define A_ED_SELECT (TV_ROLE|54)
#define A_ED_FOUND (TV_ROLE|55)
#define A_ED_MATCH (TV_ROLE|56)
#define A_ED_SCROLL (TV_ROLE|57)
int tvRoleDef[58]={0x71,0x70,0x74,0x20,0x24,0x70,0x74,0x20,0x24,0x08,0x1E,0x1F,0x17,0x1A,0x1E,0x70,0x7F,0x78,0x7A,0x7E,0x30,0x3F,0x37,0x3A,0x3E,0x7E,0x20,0x2B,0x2F,0x2E,0x70,0x30,0x3F,0x3E,0x1F,0x1A,0x79,0x78,0x74,0x1F,0x30,0x3E,0x2F,0x31,0x20,0x1E,0x1F,0x17,0x1B,0x1D,0x1A,0x1C,0x17,0x1E,0x30,0x4F,0x2F,0x13};
int tvRFgI[58];      // the role colours as tvFgI and tvBgI hold them for the depth
int tvRBgI[58];
int tvRFg[58];       // 0xFFRRGGBB
int tvRBg[58];
int tvRFg256[58];
int tvRBg256[58];
int tvCustRFg[58];   // the custom theme's roles, as tvCustFg; -1 a clear background
int tvCustRBg[58];

// A role whose background can be clear: the editor's syntax colours and line numbers.
int TvRoleClear(int r)
{
	return ((r>=R_ED_KEYWORD)&&(r<=R_ED_LINECUR));
}

// 0xRRGGBB to 15 bits and back (to 0xFFRRGGBB).
int TvTo15(int c)
{
	return (((c>>19)&31)<<10)|(((c>>11)&31)<<5)|((c>>3)&31);
}

int TvFrom15(int v)
{
	int r=(v>>10)&31;
	int g=(v>>5)&31;
	int b=v&31;
	r=(r<<3)|(r>>2);
	g=(g<<3)|(g>>2);
	b=(b<<3)|(b>>2);
	return 0xFF000000|(r<<16)|(g<<8)|b;
}

// The 0xFFRRGGBB colours of a cell attribute.
int TvAtFg(int at)
{
	if(at&TV_RGB)return TvFrom15((at>>15)&32767);
	if(at&TV_ROLE)return tvRFg[at&255];
	if(at&TV_C256)return tvRgb256[(at>>8)&255];
	return tvFg[at&15];
}

int TvAtBg(int at)
{
	if(at&TV_RGB)return TvFrom15(at&32767);
	if(at&TV_ROLE)return tvRBg[at&255];
	if(at&TV_C256)return tvRgb256[at&255];
	return tvBg[(at>>4)&15];
}

// Reads 16 colours written as six hex digits each, separated by spaces.
void TvParsePal(int* dst, char* s)
{
	int i=0;
	for(int k=0;k<16;k++)
	{
		while(s[i]==' ')i++;
		int v=0;
		for(int d=0;d<6;d++)
		{
			int c=s[i];
			int n=c-'0';
			if(c>='a')n=c-'a'+10;
			v=v*16+n;
			i++;
		}
		dst[k]=0xFF000000|v;
	}
}

// The VGA colours in their own order: blue is 1, red 4.
char* tvVgaHex="000000 0000aa 00aa00 00aaaa aa0000 aa00aa aa5500 aaaaaa 555555 5555ff 55ff55 55ffff ff5555 ff55ff ffff55 ffffff";

// Rec. 601 brightness of 0xRRGGBB, 0..255.
int TvLuma(int c)
{
	return (((c>>16)&255)*299+((c>>8)&255)*587+(c&255)*114)/1000;
}

// The 0xFFRRGGBB of a custom theme value at depth d.
int TvCustRgb(int d, int v)
{
	if(d==4)
	{
		int vga[16];
		TvParsePal(&vga[0],tvVgaHex);
		return vga[v&15];
	}
	if(d==8)return tvRgb256[v&255];
	return 0xFF000000|v;
}

void TvSetTheme(int n)
{
	char* vga=tvVgaHex;
	tvTheme=n;
	tvDepth=24;
	for(int i=0;i<16;i++)
	{
		tvFgI[i]=i;
		tvBgI[i]=i;
	}
	if(n==TH_CUSTOM)
	{
		tvDepth=tvCustDepth;
		for(int i=0;i<16;i++)
		{
			tvFgI[i]=tvCustFg[i];
			tvBgI[i]=tvCustBg[i];
			tvFg[i]=TvCustRgb(tvDepth,tvCustFg[i]);
			tvBg[i]=TvCustRgb(tvDepth,tvCustBg[i]);
		}
	}
	else if(n==TH_DARK)
	{
		TvParsePal(&tvFg[0],"d4d4d4 4a4a50 6a9955 4ec9b0 f47067 c586c0 d7ba7d 9a9a9a 18181a 569cd6 89d185 9cdcfe f48771 d7a0f0 e8dfa8 ffffff");
		TvParsePal(&tvBg[0],"0c0c0c 1e1e1e 2e6b40 264f78 6b2020 5a2a5a 6b5a1e 333337 252526 3a5a9a 3c8a50 3a8a9a a04040 a050a0 a09030 d0d0d0");
	}
	else if(n==TH_MONO)
	{
		TvParsePal(&tvFg[0],"000000 8a8a8a b0b0b0 b0b0b0 ffffff b0b0b0 b0b0b0 a8a8a8 606060 d8d8d8 d8d8d8 d8d8d8 d8d8d8 d8d8d8 e0e0e0 ffffff");
		TvParsePal(&tvBg[0],"000000 141414 505050 838383 303030 303030 303030 a4a4a4 000000 909090 909090 909090 909090 909090 a0a0a0 e0e0e0");
	}
	else
	{
		tvTheme=TH_CLASSIC;
		tvDepth=4;
		TvParsePal(&tvFg[0],vga);
		TvParsePal(&tvBg[0],vga);
	}
	for(int i=0;i<16;i++)
	{
		tvFg256[i]=TvTo256(tvFg[i]);
		tvBg256[i]=TvTo256(tvBg[i]);
	}
	tvBtnShadow=0x70;
	if(TvLuma(tvFg[8])<TvLuma(tvFg[0]))tvBtnShadow=0x78;
	for(int r=0;r<R_N;r++)
	{
		int def=tvRoleDef[r];
		if(r==R_BTN_SHADOW)def=tvBtnShadow;
		int fg=def&15;
		int bg=(def>>4)&15;
		int clear=TvRoleClear(r);
		if(n==TH_CUSTOM)
		{
			tvRFgI[r]=tvCustRFg[r];
			tvRFg[r]=TvCustRgb(tvDepth,tvCustRFg[r]);
			clear=(tvCustRBg[r]<0);
			if(!clear)
			{
				tvRBgI[r]=tvCustRBg[r];
				tvRBg[r]=TvCustRgb(tvDepth,tvCustRBg[r]);
			}
		}
		else
		{
			tvRFgI[r]=tvFgI[fg];
			tvRFg[r]=tvFg[fg];
			tvRBgI[r]=tvBgI[bg];
			tvRBg[r]=tvBg[bg];
		}
		// R_ED_TEXT comes before the roles that can be clear
		if(clear&&(r!=R_ED_TEXT))
		{
			tvRBgI[r]=tvRBgI[R_ED_TEXT];
			tvRBg[r]=tvRBg[R_ED_TEXT];
		}
		tvRFg256[r]=TvTo256(tvRFg[r]);
		tvRBg256[r]=TvTo256(tvRBg[r]);
	}
	// the cells keep their attributes, so all of them are sent again
	for(int i=0;i<TV_CELLS;i++)
		tvPrevCh[i]=-1;
	tvDirty=1;
}

char tvOut[TV_OUTMAX];
int tvOutN=0;

void OutFlush()
{
	tvOut[tvOutN]=0;
	fputs(&tvOut[0],stdout);
	fflush(stdout);
	tvOutN=0;
}

void OutByte(int c)
{
	tvOut[tvOutN]=c;
	tvOutN++;
	if(tvOutN>=TV_OUTMAX-1)
		OutFlush();
}

void OutStr(char* s)
{
	int i=0;
	while(s[i]!=0)
	{
		OutByte(s[i]);
		i++;
	}
}

void OutInt(int n)
{
	if(n>=100)OutByte('0'+n/100);
	if(n>=10)OutByte('0'+n/10%10);
	OutByte('0'+n%10);
}

void OutUtf8(int c)
{
	if(c<0x80)
		OutByte(c);
	else if(c<0x800)
	{
		OutByte(0xC0|(c>>6));
		OutByte(0x80|(c&63));
	}
	else
	{
		OutByte(0xE0|(c>>12));
		OutByte(0x80|((c>>6)&63));
		OutByte(0x80|(c&63));
	}
}

void OutMove(int x, int y)
{
	OutByte(27);
	OutByte('[');
	OutInt(y+1);
	OutByte(';');
	OutInt(x+1);
	OutByte('H');
}

// The first 16 are the VGA colours in ANSI order, as the graphic backend shows them.
void TvInitRgb256()
{
	int vga[16];
	TvParsePal(&vga[0],"000000 aa0000 00aa00 aa5500 0000aa aa00aa 00aaaa aaaaaa 555555 ff5555 55ff55 ffff55 5555ff ff55ff 55ffff ffffff");
	for(int i=0;i<16;i++)
		tvRgb256[i]=vga[i];
	for(int i=0;i<216;i++)
	{
		int r=i/36;
		int g=i/6%6;
		int b=i%6;
		if(r>0)r=55+r*40;
		if(g>0)g=55+g*40;
		if(b>0)b=55+b*40;
		tvRgb256[16+i]=0xFF000000|(r<<16)|(g<<8)|b;
	}
	for(int i=0;i<24;i++)
	{
		int v=8+i*10;
		tvRgb256[232+i]=0xFF000000|(v<<16)|(v<<8)|v;
	}
}

void OutRgb(int c)
{
	OutInt((c>>16)&255);
	OutByte(';');
	OutInt((c>>8)&255);
	OutByte(';');
	OutInt(c&255);
}

void OutAttr(int at)
{
	if(at&TV_RGB)
	{
		int f=TvAtFg(at);
		int b=TvAtBg(at);
		if(tvTrueColor)
		{
			OutStr("\x1b[38;2;");
			OutRgb(f);
			OutStr(";48;2;");
			OutRgb(b);
		}
		else
		{
			OutStr("\x1b[38;5;");
			OutInt(TvTo256(f));
			OutStr(";48;5;");
			OutInt(TvTo256(b));
		}
		OutByte('m');
		return;
	}
	int fgI=0;
	int bgI=0;
	int fgC=0;
	int bgC=0;
	int fg256=0;
	int bg256=0;
	if(at&TV_ROLE)
	{
		int r=at&255;
		fgI=tvRFgI[r];
		bgI=tvRBgI[r];
		fgC=tvRFg[r];
		bgC=tvRBg[r];
		fg256=tvRFg256[r];
		bg256=tvRBg256[r];
	}
	else if(at&TV_C256)
	{
		OutStr("\x1b[38;5;");
		OutInt((at>>8)&255);
		OutStr(";48;5;");
		OutInt(at&255);
		OutByte('m');
		return;
	}
	else
	{
		int fi=at&15;
		int bi=(at>>4)&15;
		fgI=tvFgI[fi];
		bgI=tvBgI[bi];
		fgC=tvFg[fi];
		bgC=tvBg[bi];
		fg256=tvFg256[fi];
		bg256=tvBg256[bi];
	}
	if(tvDepth==8)
	{
		OutStr("\x1b[38;5;");
		OutInt(fgI&255);
		OutStr(";48;5;");
		OutInt(bgI&255);
		OutByte('m');
		return;
	}
	if((tvDepth==24)&&!tvTrueColor)
	{
		OutStr("\x1b[38;5;");
		OutInt(fg256);
		OutStr(";48;5;");
		OutInt(bg256);
		OutByte('m');
		return;
	}
	if(tvDepth==24)
	{
		OutStr("\x1b[38;2;");
		OutRgb(fgC);
		OutStr(";48;2;");
		OutRgb(bgC);
		OutByte('m');
		return;
	}
	int fg=fgI&15;
	int bg=bgI&15;
	int f=tvVga[fg&7]+30;
	if(fg>7)f=f+60;
	int b=tvVga[bg&7]+40;
	if(bg>7)b=b+60;
	OutByte(27);
	OutByte('[');
	OutInt(f);
	OutByte(';');
	OutInt(b);
	OutByte('m');
}

#ifdef TV_GFX
void GfxFlush();
#endif

// Sends only the cells that changed since the last call.
void TvFlush()
{
#ifdef TV_GFX
	GfxFlush();
	return;
#endif
	int curAt=-1;
	int penX=-1;
	int penY=-1;
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
				if((x!=penX)||(y!=penY))
					OutMove(x,y);
				if(at!=curAt)
				{
					OutAttr(at);
					curAt=at;
				}
				OutUtf8(ch);
				penX=x+1;
				penY=y;
			}
		}
	}
	if(tvCurX>=0)
	{
		OutMove(tvCurX,tvCurY);
		if(!tvCurShown)OutStr("\x1b[?25h");
		tvCurShown=1;
	}
	else
	{
		if(tvCurShown)OutStr("\x1b[?25l");
		tvCurShown=0;
	}
	OutFlush();
}

void TvClip(int x0, int y0, int x1, int y1)
{
	clipX0=x0;
	clipY0=y0;
	clipX1=x1;
	clipY1=y1;
	if(clipX0<0)clipX0=0;
	if(clipY0<0)clipY0=0;
	if(clipX1>tvW)clipX1=tvW;
	if(clipY1>tvH)clipY1=tvH;
}

void TvPut(int x, int y, int ch, int at)
{
	if((x<clipX0)||(x>=clipX1)||(y<clipY0)||(y>=clipY1))
		return;
	int i=y*TV_MAXW+x;
	tvCh[i]=ch;
	tvAt[i]=at;
}

void TvFill(int x, int y, int w, int h, int ch, int at)
{
	for(int j=0;j<h;j++)
		for(int i=0;i<w;i++)
			TvPut(x+i,y+j,ch,at);
}

// Two square pixels in one cell, top and bottom, as xterm 256 colours.
void TvPix(int x, int y, int top, int bottom)
{
	if(top==bottom)TvPut(x,y,' ',TV_C256|bottom);
	else TvPut(x,y,G_UPHALF,TV_C256|(top<<8)|bottom);
}

// Two square pixels in one cell, top and bottom, as 0xRRGGBB (kept to 5 bits a
// channel, see TV_RGB).
void TvPixRgb(int x, int y, int top, int bottom)
{
	int t=TvTo15(top);
	int b=TvTo15(bottom);
	if(t==b)TvPut(x,y,' ',TV_RGB|(b<<15)|b);
	else TvPut(x,y,G_UPHALF,TV_RGB|(t<<15)|b);
}

// ---------------------------------------------------------------- pixel pictures

// The graphic backends can show real pixels in place of cells: TvPixels marks a
// rectangle of cells, and when the screen is flushed the backend asks the program
// for the pixels there with TvAppPixels: fullW*fullH pixels for the whole rectangle,
// of which it wants the w*h at ox,oy (the part inside the clip), as 0xAARRGGBB.
// Whatever is drawn over those cells afterwards (a window in front, a menu) shows
// instead, as any cell would. A terminal has no pixels: TvPixels does nothing there
// and TvHasPixels says 0, so a program draws half blocks instead (TvPixRgb).
// key tells whether the picture changed: the same id, place and key as in the
// last flush means the pixels need not be asked for again.
#define TV_PIXCH 0x10FFFE      // the character of a cell showing pixels
#define TV_PIXAT 0x0F000000    // | id, its attribute
#define TV_MAXPIX 8
int tvPixN=0;
int tvPixId[TV_MAXPIX];
int tvPixR[TV_MAXPIX*9];       // x, y, w, h, the visible x0, y0, x1, y1, key
int tvPixPrevN=-1;
int tvPixPrev[TV_MAXPIX*10];
int tvPixPrevCW=0;
int tvPixPrevCH=0;

int TvHasPixels()
{
#ifdef TV_GFX
	return 1;
#endif
	return 0;
}

int GfxCellPxW();
int GfxCellPxH();

// The pixels in a cell: the backend's, or the two half blocks of a terminal.
int TvCellPxW()
{
#ifdef TV_GFX
	return GfxCellPxW();
#endif
	return 1;
}

int TvCellPxH()
{
#ifdef TV_GFX
	return GfxCellPxH();
#endif
	return 2;
}

void TvPixels(int id, int x, int y, int w, int h, int key)
{
	if(!TvHasPixels())return;
	if(tvPixN>=TV_MAXPIX)return;
	int x0=x;
	int y0=y;
	int x1=x+w;
	int y1=y+h;
	if(x0<clipX0)x0=clipX0;
	if(y0<clipY0)y0=clipY0;
	if(x1>clipX1)x1=clipX1;
	if(y1>clipY1)y1=clipY1;
	if((x1<=x0)||(y1<=y0))return;
	for(int j=y0;j<y1;j++)
		for(int i=x0;i<x1;i++)
		{
			tvCh[j*TV_MAXW+i]=TV_PIXCH;
			tvAt[j*TV_MAXW+i]=TV_PIXAT|id;
		}
	int* r=&tvPixR[tvPixN*9];
	tvPixId[tvPixN]=id;
	r[0]=x;
	r[1]=y;
	r[2]=w;
	r[3]=h;
	r[4]=x0;
	r[5]=y0;
	r[6]=x1;
	r[7]=y1;
	r[8]=key;
	tvPixN++;
}

// 1 if a cell shows pixels (a shadow over it does not).
int TvIsPixCell(int ch, int at)
{
	if(ch!=TV_PIXCH)return 0;
	if((at&0x7F000000)!=TV_PIXAT)return 0;
	return 1;
}

// Have the pictures changed since the last flush (TvPixCommit)?
int TvPixChanged()
{
	if(tvPixN!=tvPixPrevN)return 1;
	if((TvCellPxW()!=tvPixPrevCW)||(TvCellPxH()!=tvPixPrevCH))return 1;
	for(int k=0;k<tvPixN;k++)
	{
		if(tvPixPrev[k*10]!=tvPixId[k])return 1;
		for(int j=0;j<9;j++)
			if(tvPixPrev[k*10+1+j]!=tvPixR[k*9+j])return 1;
	}
	return 0;
}

void TvPixCommit()
{
	tvPixPrevN=tvPixN;
	tvPixPrevCW=TvCellPxW();
	tvPixPrevCH=TvCellPxH();
	for(int k=0;k<tvPixN;k++)
	{
		tvPixPrev[k*10]=tvPixId[k];
		for(int j=0;j<9;j++)
			tvPixPrev[k*10+1+j]=tvPixR[k*9+j];
	}
}

int StrLen(char* s)
{
	int n=0;
	while(s[n]!=0)n++;
	return n;
}

void StrCpy(char* d, char* s, int max)
{
	int i=0;
	while(i<max-1)
	{
		if(s[i]==0)break;
		d[i]=s[i];
		i++;
	}
	d[i]=0;
}

// Decodes the UTF-8 character at s[i] and moves i past it. Invalid bytes give '?'.
int TvUtf8(char* s, int& i)
{
	int c=s[i];
	i++;
	if(c<0x80)return c;
	int more=0;
	int v=0;
	if((c>=0xC2)&&(c<0xE0))
	{
		more=1;
		v=c&31;
	}
	else if((c>=0xE0)&&(c<0xF0))
	{
		more=2;
		v=c&15;
	}
	else if((c>=0xF0)&&(c<0xF5))
	{
		more=3;
		v=c&7;
	}
	else return '?';
	for(int k=0;k<more;k++)
	{
		int n=s[i];
		if((n&0xC0)!=0x80)return '?';
		v=(v<<6)|(n&63);
		i++;
	}
	// beyond the three-byte range OutUtf8 can send
	if(v>0xFFFF)return '?';
	return v;
}

// The number of screen columns a string takes.
int TvCols(char* s)
{
	int n=0;
	int i=0;
	while(s[i]!=0)
	{
		TvUtf8(s,i);
		n++;
	}
	return n;
}

// The columns taken by the bytes from a up to b, which are character boundaries.
int TvColsBetween(char* s, int a, int b)
{
	int n=0;
	while(a<b)
	{
		TvUtf8(s,a);
		n++;
	}
	return n;
}

// The byte offset of the character before byte offset i.
int TvPrevChar(char* s, int i)
{
	int p=0;
	int q=0;
	while(q<i)
	{
		p=q;
		TvUtf8(s,q);
	}
	return p;
}

// Draws at most maxCols columns. Returns the column after the text.
int TvTextN(int x, int y, char* s, int at, int maxCols)
{
	int i=0;
	int n=0;
	while((s[i]!=0)&&(n<maxCols))
	{
		TvPut(x,y,TvUtf8(s,i),at);
		x++;
		n++;
	}
	return x;
}

int TvText(int x, int y, char* s, int at)
{
	return TvTextN(x,y,s,at,1000000);
}

// "~F~ile": text between tildes is drawn in the hot colour; "~~" is a tilde.
int TvHotText(int x, int y, char* s, int at, int hot)
{
	int i=0;
	int cur=at;
	while(s[i]!=0)
	{
		if((s[i]=='~')&&(s[i+1]=='~'))
		{
			TvPut(x,y,'~',cur);
			x++;
			i++;
		}
		else if(s[i]=='~')
		{
			if(cur==at)cur=hot;
			else cur=at;
		}
		else
		{
			TvPut(x,y,s[i],cur);
			x++;
		}
		i++;
	}
	return x;
}

int HotLen(char* s)
{
	int n=0;
	int i=0;
	while(s[i]!=0)
	{
		if((s[i]=='~')&&(s[i+1]=='~'))
		{
			n++;
			i++;
		}
		else if(s[i]!='~')n++;
		i++;
	}
	return n;
}

int LowerKey(int c)
{
	if((c>='A')&&(c<='Z'))
		return c+32;
	return c;
}

// The lower-case hot character of "~F~ile", or 0.
int HotChar(char* s)
{
	int i=0;
	while(s[i]!=0)
	{
		if((s[i]=='~')&&(s[i+1]=='~'))i++;
		else if(s[i]=='~')
		{
			if(s[i+1]==0)return 0;
			return LowerKey(s[i+1]);
		}
		i++;
	}
	return 0;
}

void TvFrame(int x, int y, int w, int h, int dbl, int at)
{
	int hz=G_HORZ;
	int vt=G_VERT;
	int tl=G_TL;
	int tr=G_TR;
	int bl=G_BL;
	int br=G_BR;
	if(dbl)
	{
		hz=G_DHORZ;
		vt=G_DVERT;
		tl=G_DTL;
		tr=G_DTR;
		bl=G_DBL;
		br=G_DBR;
	}
	for(int i=1;i<w-1;i++)
	{
		TvPut(x+i,y,hz,at);
		TvPut(x+i,y+h-1,hz,at);
	}
	for(int j=1;j<h-1;j++)
	{
		TvPut(x,y+j,vt,at);
		TvPut(x+w-1,y+j,vt,at);
	}
	TvPut(x,y,tl,at);
	TvPut(x+w-1,y,tr,at);
	TvPut(x,y+h-1,bl,at);
	TvPut(x+w-1,y+h-1,br,at);
}

// Darkens what is under the shadow and keeps the characters visible.
void TvShadow(int x, int y, int w, int h)
{
	for(int j=1;j<=h;j++)
	{
		for(int i=0;i<2;i++)
		{
			int px=x+w+i;
			int py=y+j;
			if((px>=clipX0)&&(px<clipX1)&&(py>=clipY0)&&(py<clipY1))
				tvAt[py*TV_MAXW+px]=A_SHADOW;
		}
	}
	for(int i=2;i<w;i++)
	{
		int px=x+i;
		int py=y+h;
		if((px>=clipX0)&&(px<clipX1)&&(py>=clipY0)&&(py<clipY1))
			tvAt[py*TV_MAXW+px]=A_SHADOW;
	}
}

// A vertical scroll bar with arrows and a thumb.
void TvVBar(int x, int y, int h, int pos, int max, int at)
{
	TvPut(x,y,G_UP,at);
	TvPut(x,y+h-1,G_DOWN,at);
	for(int j=1;j<h-1;j++)
		TvPut(x,y+j,G_MEDIUM,at);
	if(h<3)
		return;
	int t=0;
	if(max>0)
		t=pos*(h-3)/max;
	TvPut(x,y+1+t,G_BOX,at);
}

// Where a click at row my lands: 0 up arrow, 1 page up, 2 thumb, 3 page down, 4 down arrow.
int TvVBarHit(int y, int h, int pos, int max, int my)
{
	if(my<=y)return 0;
	if(my>=y+h-1)return 4;
	int t=0;
	if(max>0)
		t=pos*(h-3)/max;
	if(my<y+1+t)return 1;
	if(my>y+1+t)return 3;
	return 2;
}

// The scroll position for a thumb dragged to row my, the inverse of the thumb
// placement in TvVBar. Rounds up so that the thumb lands on the row under the mouse.
int TvVBarPos(int y, int h, int max, int my)
{
	int cells=h-3;
	if(cells<=0)return 0;
	int row=my-(y+1);
	if(row<0)row=0;
	if(row>cells)row=cells;
	return (row*max+cells-1)/cells;
}

// ---------------------------------------------------------------- windows and controls

#define TV_MAXWIN 16
#define TV_MAXNUM 12   // windows other than modal dialogs; the rest are kept for those
#define TV_MAXCTL 96

#define C_LABEL 0
#define C_BUTTON 1
#define C_CHECK 2
#define C_RADIO 3
#define C_INPUT 4
#define C_LIST 5
#define C_SLIDER 6    // val from 0 to count
#define C_PROGRESS 7  // val from 0 to count, shown as a bar with the percentage
#define C_COMBO 8     // val is the item picked from a drop-down list, items as for C_LIST
#define C_VIEW 9      // an area the application draws in TvAppDraw and serves in TvAppKey
                      // (when it has the focus) and TvAppMouse; a press captures the mouse

#define CM_OK 1
#define CM_CANCEL 2
#define CM_YES 3
#define CM_NO 4
#define CM_NEXT 10
#define CM_PREV 11
#define CM_ZOOM 12
#define CM_CLOSE 13
#define CM_CASCADE 14
#define CM_TILE 15
#define CM_MENU 16
#define CM_WINLIST 17   // the window list dialog
#define CM_WINGO 20     // CM_WINGO+w brings window w to the front (the Windows menu)
#define CM_USER 100

#define WF_RESIZE 1
#define WF_DIALOG 2
#define WF_HELP 4    // the cyan of Turbo Vision's help windows

struct TWin
{
	int used;
	int type;
	int flags;
	int modal;
	int result;
	int num;
	int x;
	int y;
	int w;
	int h;
	int zoomed;
	int zx;
	int zy;
	int zw;
	int zh;
	int focus;
	int a;
	int b;
	int c;
	int d;
	int e;
	int f;
	char title[48];
};

struct TCtl
{
	int used;
	int win;
	int type;
	int x;
	int y;
	int w;
	int h;
	int val;
	int top;
	int cmd;
	int isDefault;
	int group;
	int virt;   // a list asks TvAppListItem for its items
	int count;  // the item count of a virtual list
	char text[200];
};

TWin tvWin[TV_MAXWIN];
int tvWinDirty[TV_MAXWIN];  // set by the program for a window with unsaved changes;
                            // the window list marks it with *
TCtl tvCtl[TV_MAXCTL];
int tvZ[TV_MAXWIN];  // back to front
int tvNZ=0;
int tvRunning=1;
int tvTickMs=0;      // set above 0 for a TvAppTick timer, for animation and games
int tvNextTick=0;
int tvLastInput=0;   // term_ms() of the last key, click or paste, for a screen saver
int tvScreenTaken=0; // the program draws the whole screen itself and calls TvFlush, as a
                     // screen saver does; the next key or click only clears this

#define TV_COMBOROWS 8
int comboCtl=-1;     // the combo box whose list is open, or -1
int comboSel=0;
int comboTop=0;

int dragWin=-1;
int dragMode=0;      // 1 move, 2 resize, 3 pressed control, 4 captured by the app, 5 list thumb,
                     // 6 slider, 7 pressed close or zoom box (dragCtl 1 or 2), 8 pressed
                     // status line item (dragCtl its number); 3, 7 and 8 act on release
int dragDX=0;
int dragDY=0;
int dragCtl=-1;
int pressedInside=0;
int tvTitleWin=-1;   // the window whose title was clicked last, and when, for double clicks
int tvTitleMs=0;

int TvTop()
{
	if(tvNZ==0)return -1;
	return tvZ[tvNZ-1];
}

void TvToFront(int id)
{
	int found=-1;
	for(int i=0;i<tvNZ;i++)
		if(tvZ[i]==id)found=i;
	if(found<0)return;
	for(int i=found;i<tvNZ-1;i++)
		tvZ[i]=tvZ[i+1];
	tvZ[tvNZ-1]=id;
	tvDirty=1;
}

// The windows open other than modal dialogs.
int TvOpenCount()
{
	int n=0;
	for(int i=0;i<TV_MAXWIN;i++)
		if(tvWin[i].used&&!tvWin[i].modal)n++;
	return n;
}

// The smallest window number no other window has, or 0.
int TvFreeNum(int self)
{
	int found=0;
	for(int n=TV_MAXNUM;n>=1;n--)
	{
		int taken=0;
		for(int i=0;i<TV_MAXWIN;i++)
			if(tvWin[i].used)
				if((i!=self)&&(tvWin[i].num==n))taken=1;
		if(!taken)found=n;
	}
	return found;
}

// How a window number is shown: 1..9, then a, b, c.
int TvNumChar(int n)
{
	if(n>=10)return 'a'+n-10;
	return '0'+n;
}

int TvNew(int type, char* title, int x, int y, int w, int h, int flags)
{
	int id=-1;
	for(int i=TV_MAXWIN-1;i>=0;i--)
		if(!tvWin[i].used)id=i;
	if(id<0)return -1;
	// an ordinary window must leave the last slots to modal dialogs
	if(!(flags&WF_DIALOG))
		if(TvOpenCount()>=TV_MAXNUM)return -1;
	TWin& t=tvWin[id];
	t.used=1;
	t.type=type;
	t.flags=flags;
	t.modal=0;
	t.result=0;
	t.x=x;
	t.y=y;
	t.w=w;
	t.h=h;
	t.zoomed=0;
	t.focus=-1;
	t.a=0;
	t.b=0;
	t.c=0;
	t.d=0;
	t.e=0;
	t.f=0;
	StrCpy(&t.title[0],title,48);
	tvWinDirty[id]=0;
	// the smallest free number, 1..9 then a, b, c: a dialog gets one later if it
	// turns out not to be modal (TvNumberWindows)
	t.num=0;
	if(!(flags&WF_DIALOG))t.num=TvFreeNum(id);
	tvZ[tvNZ]=id;
	tvNZ++;
	tvDirty=1;
	return id;
}

// A window of size w*h in the middle of the desktop.
int TvNewCentered(int type, char* title, int w, int h, int flags)
{
	int x=(tvW-w)/2;
	int y=1+(tvH-2-h)/2;
	if(x<0)x=0;
	if(y<1)y=1;
	return TvNew(type,title,x,y,w,h,flags);
}

void TvClose(int id)
{
	if(id<0)return;
	if(!tvWin[id].used)return;
	TvAppClosed(id);
	if(comboCtl>=0)
		if(tvCtl[comboCtl].win==id)comboCtl=-1;
	for(int i=0;i<TV_MAXCTL;i++)
		if(tvCtl[i].used)
			if(tvCtl[i].win==id)tvCtl[i].used=0;
	int n=0;
	for(int i=0;i<tvNZ;i++)
	{
		if(tvZ[i]!=id)
		{
			tvZ[n]=tvZ[i];
			n++;
		}
	}
	tvNZ=n;
	tvWin[id].used=0;
	if(dragWin==id)
	{
		dragWin=-1;
		dragMode=0;
	}
	tvDirty=1;
}

int TvFocusable(int type)
{
	return (type!=C_LABEL)&&(type!=C_PROGRESS);
}

int TvAdd(int win, int type, int x, int y, int w, int h, char* text, int cmd)
{
	int id=-1;
	for(int i=TV_MAXCTL-1;i>=0;i--)
		if(!tvCtl[i].used)id=i;
	if(id<0)return -1;
	TCtl& c=tvCtl[id];
	c.used=1;
	c.win=win;
	c.type=type;
	c.x=x;
	c.y=y;
	c.w=w;
	c.h=h;
	c.val=0;
	c.top=0;
	c.cmd=cmd;
	c.isDefault=0;
	c.group=0;
	c.virt=0;
	c.count=0;
	StrCpy(&c.text[0],text,200);
	if(TvFocusable(type))
		if(tvWin[win].focus<0)tvWin[win].focus=id;
	return id;
}

// List items are stored in the control text, separated by '|'.
int TvItemCount(char* s)
{
	if(s[0]==0)return 0;
	int n=1;
	int i=0;
	while(s[i]!=0)
	{
		if(s[i]=='|')n++;
		i++;
	}
	return n;
}

void TvItemAt(char* s, int n, char* out, int max)
{
	int i=0;
	while(n>0)
	{
		if(s[i]==0)break;
		if(s[i]=='|')n--;
		i++;
	}
	int o=0;
	while(o<max-1)
	{
		if((s[i]==0)||(s[i]=='|'))break;
		out[o]=s[i];
		o++;
		i++;
	}
	out[o]=0;
}

int TvListCount(TCtl& c)
{
	if(c.virt)return c.count;
	return TvItemCount(&c.text[0]);
}

// The text of item k of a list or combo box.
// ---------------------------------------------------------------- the window list

#define W_WINLIST 1000

int tvWlWin[TV_MAXWIN];   // the windows the list shows, front first
int tvWlN=0;

void TvWinListFill(int self)
{
	tvWlN=0;
	for(int z=tvNZ-1;z>=0;z--)
		if(tvZ[z]!=self)
		{
			tvWlWin[tvWlN]=tvZ[z];
			tvWlN++;
		}
}

void TvWinListItem(int i, char* out, int max)
{
	int w=tvWlWin[i];
	char* mark="";
	if(tvWinDirty[w])mark=" *";
	char num[4];
	num[0]=' ';
	num[1]=0;
	if(tvWin[w].num>0)num[0]=TvNumChar(tvWin[w].num);
	snprintf(out,max,"%s  %s%s",&num[0],&tvWin[w].title[0],mark);
}

void TvCtlItem(int id, int k, char* out, int max)
{
	TCtl& c=tvCtl[id];
	if(tvWin[c.win].type==W_WINLIST)TvWinListItem(k,out,max);
	else if(c.virt)TvAppListItem(c.win,id,k,out,max);
	else TvItemAt(&c.text[0],k,out,max);
}

// Tells the control's command that its value changed.
void TvChanged(int win, int id)
{
	tvDirty=1;
	if(tvCtl[id].cmd!=0)TvAppButton(win,id,tvCtl[id].cmd);
}

void TvSliderSet(int win, int id, int v)
{
	TCtl& c=tvCtl[id];
	if(v>c.count)v=c.count;
	if(v<0)v=0;
	if(v==c.val)return;
	c.val=v;
	TvChanged(win,id);
}

// The slider value for a click or drag at column mx.
int TvSliderAt(int win, int id, int mx)
{
	TCtl& c=tvCtl[id];
	int cells=c.w-1;
	if(cells<1)return 0;
	int p=mx-(tvWin[win].x+c.x);
	if(p<0)p=0;
	if(p>cells)p=cells;
	return (p*c.count+cells/2)/cells;
}

// ---------------------------------------------------------------- combo box drop-down


void TvComboBox(int& x, int& y, int& w, int& h)
{
	TCtl& c=tvCtl[comboCtl];
	TWin& t=tvWin[c.win];
	int rows=TvListCount(c);
	if(rows>TV_COMBOROWS)rows=TV_COMBOROWS;
	if(rows<1)rows=1;
	w=c.w;
	h=rows+2;
	x=t.x+c.x;
	y=t.y+c.y+1;
	if(y+h>tvH-1)y=t.y+c.y-h;
	if(y<1)y=1;
}

void TvComboOpen(int id)
{
	comboCtl=id;
	comboSel=tvCtl[id].val;
	comboTop=0;
	tvDirty=1;
}

void TvComboClose(int pick)
{
	int id=comboCtl;
	comboCtl=-1;
	tvDirty=1;
	if(pick)
		if(tvCtl[id].val!=comboSel)
		{
			tvCtl[id].val=comboSel;
			TvChanged(tvCtl[id].win,id);
		}
}

void TvComboClamp()
{
	TCtl& c=tvCtl[comboCtl];
	int n=TvListCount(c);
	int x;
	int y;
	int w;
	int h;
	TvComboBox(x,y,w,h);
	if(comboSel>=n)comboSel=n-1;
	if(comboSel<0)comboSel=0;
	if(comboSel<comboTop)comboTop=comboSel;
	if(comboSel>=comboTop+h-2)comboTop=comboSel-(h-2)+1;
}

void TvDrawCombo()
{
	if(comboCtl<0)return;
	int x;
	int y;
	int w;
	int h;
	TvComboBox(x,y,w,h);
	TvComboClamp();
	int n=TvListCount(tvCtl[comboCtl]);
	int rows=h-2;
	TvClip(0,0,tvW,tvH);
	TvShadow(x,y,w,h);
	TvFill(x,y,w,h,' ',A_LIST);
	TvFrame(x,y,w,h,0,A_LIST);
	char item[200];
	for(int j=0;j<rows;j++)
	{
		int k=comboTop+j;
		if(k<n)
		{
			int at=A_LIST;
			if(k==comboSel)
			{
				at=A_LIST_FOCUS;
				TvFill(x+1,y+1+j,w-2,1,' ',at);
			}
			TvCtlItem(comboCtl,k,&item[0],200);
			TvTextN(x+1,y+1+j,&item[0],at,w-2);
		}
	}
	if(n>rows)TvVBar(x+w-1,y+1,rows,comboTop,n-rows,A_SCROLL);
	tvCurX=-1;
}

void TvComboKey(int key)
{
	tvDirty=1;
	if((key==K_ESC)||(key==K_F4))
	{
		TvComboClose(0);
		return;
	}
	if((key==K_ENTER)||(key==' ')||(key==K_TAB))
	{
		TvComboClose(1);
		return;
	}
	int n=TvListCount(tvCtl[comboCtl]);
	if(key==K_UP)comboSel--;
	else if(key==K_DOWN)comboSel++;
	else if(key==K_PGUP)comboSel=comboSel-TV_COMBOROWS;
	else if(key==K_PGDN)comboSel=comboSel+TV_COMBOROWS;
	else if(key==K_HOME)comboSel=0;
	else if(key==K_END)comboSel=n-1;
	else if((key>' ')&&(key<127))
	{
		// the next item that starts with this letter
		char item[200];
		for(int s=1;s<=n;s++)
		{
			int k=(comboSel+s)%n;
			TvCtlItem(comboCtl,k,&item[0],200);
			if(LowerKey(item[0])==LowerKey(key))
			{
				comboSel=k;
				break;
			}
		}
	}
	TvComboClamp();
}

void TvComboMouse(int ev)
{
	int x;
	int y;
	int w;
	int h;
	TvComboBox(x,y,w,h);
	int n=TvListCount(tvCtl[comboCtl]);
	int rows=h-2;
	tvDirty=1;
	if(ev==EV_WHEEL)
	{
		comboSel=comboSel-evWheel;
		TvComboClamp();
		return;
	}
	int inRows=(evY>y)&&(evY<y+h-1);
	if((evX==x+w-1)&&inRows&&(n>rows))
	{
		if(ev==EV_MDOWN)
		{
			int hit=TvVBarHit(y+1,rows,comboTop,n-rows,evY);
			if(hit==0)comboSel--;
			if(hit==1)comboSel=comboSel-rows;
			if(hit==3)comboSel=comboSel+rows;
			if(hit==4)comboSel++;
			TvComboClamp();
		}
		return;
	}
	if(inRows&&(evX>x)&&(evX<x+w-1))
	{
		int k=comboTop+evY-y-1;
		if(k<n)
		{
			comboSel=k;
			if(ev==EV_MUP)TvComboClose(1);
		}
		return;
	}
	if(ev==EV_MDOWN)TvComboClose(0);
}

void TvDrawCtl(int id, TWin& t)
{
	TCtl& c=tvCtl[id];
	int x=t.x+c.x;
	int y=t.y+c.y;
	int focused=(t.focus==id)&&(TvTop()==c.win);
	if(c.type==C_LABEL)
	{
		TvHotText(x,y,&c.text[0],A_DLG,A_LABEL_HOT);
	}
	else if(c.type==C_BUTTON)
	{
		int pressed=(dragMode==3)&&(dragCtl==id)&&pressedInside;
		int at=A_BTN;
		if(c.isDefault)at=A_BTN_DEF;
		if(focused)at=A_BTN_FOCUS;
		int bx=x;
		if(pressed)bx=x+1;
		TvFill(bx,y,c.w,1,' ',at);
		TvHotText(bx+(c.w-HotLen(&c.text[0]))/2,y,&c.text[0],at,A_BTN_HOT);
		if(!pressed)
		{
			// the dark themes make index 0 a light text colour, so they shade with 8
			int sh=A_BTN_SHADOW;
			TvPut(x+c.w,y,G_LOWHALF,sh);
			for(int i=1;i<=c.w;i++)
				TvPut(x+i,y+1,G_UPHALF,sh);
		}
	}
	else if((c.type==C_CHECK)||(c.type==C_RADIO))
	{
		int at=A_CHECK;
		if(focused)at=A_CHECK_FOCUS;
		TvFill(x,y,c.w,1,' ',at);
		if(c.type==C_CHECK)
		{
			TvText(x+1,y,"[ ]",at);
			if(c.val)TvPut(x+2,y,'X',at);
		}
		else
		{
			TvText(x+1,y,"( )",at);
			if(c.val)TvPut(x+2,y,G_DOT,at);
		}
		TvHotText(x+5,y,&c.text[0],at,A_CHECK_HOT);
	}
	else if(c.type==C_INPUT)
	{
		// val and top are byte offsets, on character boundaries
		int len=StrLen(&c.text[0]);
		if(c.val>len)c.val=len;
		if(c.val<c.top)c.top=c.val;
		while(TvColsBetween(&c.text[0],c.top,c.val)>c.w-1)
			TvUtf8(&c.text[0],c.top);
		TvFill(x,y,c.w,1,' ',A_INPUT);
		TvTextN(x,y,&c.text[c.top],A_INPUT,c.w);
		if(c.top>0)TvPut(x,y,G_LEFT,A_INPUT_ARROW);
		if(focused)
		{
			tvCurX=x+TvColsBetween(&c.text[0],c.top,c.val);
			tvCurY=y;
		}
	}
	else if(c.type==C_SLIDER)
	{
		int p=0;
		if(c.count>0)p=(c.val*(c.w-1)+c.count/2)/c.count;
		for(int i=0;i<c.w;i++)
		{
			if(i<p)TvPut(x+i,y,G_DHORZ,A_SLIDER);
			else TvPut(x+i,y,G_HORZ,A_SLIDER_TRACK);
		}
		int at=A_SLIDER;
		if(focused)at=A_SLIDER_FOCUS;
		TvPut(x+p,y,G_FULL,at);
	}
	else if(c.type==C_PROGRESS)
	{
		int fill=0;
		int pct=0;
		if(c.count>0)
		{
			fill=c.val*c.w/c.count;
			pct=c.val*100/c.count;
		}
		char s[8];
		snprintf(&s[0],8,"%i",pct);
		int l=StrLen(&s[0]);
		s[l]='%';
		s[l+1]=0;
		l++;
		int sx=(c.w-l)/2;
		for(int i=0;i<c.w;i++)
		{
			int at=A_DLG_PASSIVE;
			int ch=G_LIGHT;
			if(i<fill)
			{
				at=A_PROGRESS;
				ch=' ';
			}
			if((i>=sx)&&(i<sx+l))
			{
				ch=s[i-sx];
				if(i>=fill)at=A_DLG;
			}
			TvPut(x+i,y,ch,at);
		}
	}
	else if(c.type==C_COMBO)
	{
		char item[200];
		item[0]=0;
		if(c.val<TvListCount(c))TvCtlItem(id,c.val,&item[0],200);
		int at=A_INPUT;
		if(focused)at=A_LIST_FOCUS;
		TvFill(x,y,c.w-1,1,' ',at);
		TvTextN(x+1,y,&item[0],at,c.w-3);
		TvPut(x+c.w-1,y,G_DOWN,A_COMBO_ARROW);
	}
	else if(c.type==C_LIST)
	{
		int n=TvListCount(c);
		char item[256];
		int saveX1=clipX1;
		if(clipX1>x+c.w-1)clipX1=x+c.w-1;
		if(c.val<c.top)c.top=c.val;
		if(c.val>=c.top+c.h)c.top=c.val-c.h+1;
		for(int j=0;j<c.h;j++)
		{
			int at=A_LIST;
			int k=c.top+j;
			if(k==c.val)
			{
				at=A_LIST_SEL;
				if(focused)at=A_LIST_FOCUS;
			}
			TvFill(x,y+j,c.w-1,1,' ',at);
			if(k<n)
			{
				TvCtlItem(id,k,&item[0],256);
				TvText(x+1,y+j,&item[0],at);
			}
		}
		clipX1=saveX1;
		int mx=n-c.h;
		if(mx<0)mx=0;
		TvVBar(x+c.w-1,y,c.h,c.top,mx,A_SCROLL);
	}
}

void TvDrawWin(int id)
{
	TWin& t=tvWin[id];
	int active=(TvTop()==id);
	int dialog=t.flags&WF_DIALOG;
	int bg=A_WIN;
	int fr=A_WIN_FRAME;
	int pas=A_WIN_PASSIVE;
	int icon=A_WIN_ICON;
	int press=A_WIN_PRESSED;
	if(dialog)
	{
		bg=A_DLG;
		fr=A_DLG_FRAME;
		pas=A_DLG_PASSIVE;
		icon=A_DLG_ICON;
		press=A_DLG_PRESSED;
	}
	if(t.flags&WF_HELP)
	{
		bg=A_HELP;
		fr=A_HELP_FRAME;
		pas=A_HELP_PASSIVE;
		icon=A_HELP_ICON;
		press=A_HELP_PRESSED;
	}
	TvClip(0,1,tvW,tvH-1);
	TvShadow(t.x,t.y,t.w,t.h);
	TvFill(t.x,t.y,t.w,t.h,' ',bg);
	int at=pas;
	if(active)at=fr;
	int dragging=(dragWin==id)&&((dragMode==1)||(dragMode==2));
	if(dragging)at=icon;
	TvFrame(t.x,t.y,t.w,t.h,active&&!dragging,at);
	// title
	int tl=TvCols(&t.title[0]);
	if(tl>t.w-16)tl=t.w-16;
	if(tl>0)
	{
		int tx=t.x+(t.w-tl-2)/2;
		TvPut(tx,t.y,' ',at);
		int e=TvTextN(tx+1,t.y,&t.title[0],at,tl);
		TvPut(e,t.y,' ',at);
	}
	if(active&&!dragging)
	{
		// a pressed box shows in yellow until the button is released
		int boxAt=icon;
		int zoomAt=boxAt;
		if((dragMode==7)&&(dragWin==id)&&pressedInside)
		{
			if(dragCtl==1)boxAt=press;
			if(dragCtl==2)zoomAt=press;
		}
		TvPut(t.x+2,t.y,'[',at);
		TvPut(t.x+3,t.y,G_BOX,boxAt);
		TvPut(t.x+4,t.y,']',at);
		if(t.flags&WF_RESIZE)
		{
			TvPut(t.x+t.w-5,t.y,'[',at);
			TvPut(t.x+t.w-4,t.y,G_ZOOM,zoomAt);
			TvPut(t.x+t.w-3,t.y,']',at);
			TvPut(t.x+t.w-1,t.y+t.h-1,G_BR,icon);
			TvPut(t.x+t.w-2,t.y+t.h-1,G_HORZ,icon);
		}
	}
	if((t.num>0)&&(t.w>12))
		TvPut(t.x+t.w-7,t.y,TvNumChar(t.num),at);
	TvClip(t.x+1,t.y+1,t.x+t.w-1,t.y+t.h-1);
	if(clipY0<1)clipY0=1;
	if(clipY1>tvH-1)clipY1=tvH-1;
	int cx0=clipX0;
	int cy0=clipY0;
	int cx1=clipX1;
	int cy1=clipY1;
	TvAppDraw(id);
	for(int i=0;i<TV_MAXCTL;i++)
	{
		if(tvCtl[i].used)
			if(tvCtl[i].win==id)
			{
				clipX0=cx0;
				clipY0=cy0;
				clipX1=cx1;
				clipY1=cy1;
				TvDrawCtl(i,t);
			}
	}
}

// ---------------------------------------------------------------- menus and status line

#define TV_MAXMENU 16
#define TV_MAXITEM 128
#define TV_MAXSTATUS 96

struct TMenu
{
	char title[16];
	int first;
	int count;
	int popup;  // 1 a context menu, 2 a submenu; neither is on the menu bar
};

struct TItem
{
	char text[32];
	char hint[12];
	int cmd;
	int key;
	int sub;    // the submenu this item opens, or -1
};

struct TStatus
{
	char text[24];
	int cmd;
	int key;
	int ctx;   // where it shows: a window type, a TV_ST_* context, or what TvAppStatus names
};

// Status line contexts. Items added after TvStatusFor(ctx) show only while the front
// window's context is ctx; that is TvAppStatus(win), by default the window type, or
// TV_ST_DIALOG for a modal dialog.
#define TV_ST_ALL -1     // everywhere, but in modal dialogs that have items of their own
#define TV_ST_DESK -2    // no window open
#define TV_ST_DIALOG -3  // a modal dialog
#define TV_ST_OTHER -4   // any context that has no items of its own
int tvStatusFor=TV_ST_ALL;
int tvStCtx=TV_ST_DESK; // the context the status line shows, from TvStatusUpdate
int tvStOwn=0;          // that context has items of its own

TMenu tvMenu[TV_MAXMENU];
TItem tvItem[TV_MAXITEM];
TStatus tvStatus[TV_MAXSTATUS];
int tvNMenu=0;
int tvNItem=0;
int tvNStatus=0;
int menuOpen=-1;
int menuSel=0;
int popX=0;          // where the open context menu is
int popY=0;
int popArmed=0;      // the mouse has moved or pressed in the open context menu
int menuSub=-1;      // the open submenu of the open menu, or -1
int subSel=0;
int subX=0;          // where the open submenu is
int subY=0;
int tvDeskPopup=-1;  // the context menu for a right click on the desktop, or -1
int tvClockSec=-1;

void TvMenuAdd(char* title)
{
	TMenu& m=tvMenu[tvNMenu];
	StrCpy(&m.title[0],title,16);
	m.first=tvNItem;
	m.count=0;
	m.popup=0;
	tvNMenu++;
}

// Starts a context menu, filled with TvMenuItem like the others. Add these after
// the menu bar's menus. Returns its number for TvOpenPopup.
int TvPopupAdd()
{
	TvMenuAdd("");
	tvMenu[tvNMenu-1].popup=1;
	return tvNMenu-1;
}

void TvMenuItem(char* text, char* hint, int cmd, int key)
{
	TItem& it=tvItem[tvNItem];
	StrCpy(&it.text[0],text,32);
	StrCpy(&it.hint[0],hint,12);
	it.cmd=cmd;
	it.key=key;
	it.sub=-1;
	tvNItem++;
	tvMenu[tvNMenu-1].count++;
}

// Adds an item that opens a submenu, marked with an arrow, and returns the item's
// number for TvSubmenuAdd. Enter, Right, its hot key or a click opens the submenu;
// Left or Esc goes back.
int TvMenuSub(char* text)
{
	TvMenuItem(text,"",0,0);
	return tvNItem-1;
}

// Starts the submenu of item (from TvMenuSub), filled with TvMenuItem like the
// others. Add these after the menu bar's menus, as context menus are. Several items
// may open the same submenu: TvSubmenuShare(item, sub).
int TvSubmenuAdd(int item)
{
	TvMenuAdd("");
	tvMenu[tvNMenu-1].popup=2;
	tvItem[item].sub=tvNMenu-1;
	return tvNMenu-1;
}

void TvSubmenuShare(int item, int sub)
{
	tvItem[item].sub=sub;
}

void TvMenuLine()
{
	TvMenuItem("","",0,0);
}

// The Windows menu: one item per open window, filled anew each time it opens.
int tvWinMenu=-1;
int tvWinMenuSel=0;

// Adds the Windows menu to the menu bar, with this title. Picking an item brings
// that window to the front.
void TvWindowsMenu(char* title)
{
	TvMenuAdd(title);
	tvWinMenu=tvNMenu-1;
	for(int i=0;i<TV_MAXWIN;i++)
		TvMenuItem("","",0,0);
}

// One Windows menu item for window w: "~N~ title *", with Alt+N as the hint.
void TvWinMenuSet(int k, int w)
{
	TItem& it=tvItem[tvMenu[tvWinMenu].first+k];
	TWin& t=tvWin[w];
	// the end of a long title, which holds the file name; tildes are doubled
	char ti[64];
	int n=0;
	int len=StrLen(&t.title[0]);
	int from=0;
	if(len>24)
	{
		from=len-21;
		StrCpy(&ti[0],"...",4);
		n=3;
	}
	for(int i=from;i<len;i++)
	{
		if(t.title[i]=='~')
		{
			ti[n]='~';
			n++;
		}
		ti[n]=t.title[i];
		n++;
	}
	ti[n]=0;
	char* mark="";
	if(tvWinDirty[w])mark=" *";
	if(t.num>0)
	{
		snprintf(&it.text[0],32,"~%c~ %s%s",TvNumChar(t.num),&ti[0],mark);
		it.hint[0]=0;
		if(t.num<=9)snprintf(&it.hint[0],12,"Alt+%d",t.num);
	}
	else
	{
		snprintf(&it.text[0],32,"  %s%s",&ti[0],mark);
		it.hint[0]=0;
	}
	it.cmd=CM_WINGO+w;
	it.key=0;
	it.sub=-1;
}

// Fills the Windows menu: numbered windows by number, then the others, and the
// front window highlighted.
void TvWinMenuFill()
{
	TMenu& m=tvMenu[tvWinMenu];
	int top=TvTop();
	int k=0;
	tvWinMenuSel=0;
	for(int pass=0;pass<2;pass++)
		for(int key=0;key<=TV_MAXWIN;key++)
			for(int w=0;w<TV_MAXWIN;w++)
				if(tvWin[w].used)
				{
					int want=0;
					if((pass==0)&&(tvWin[w].num==key)&&(key>0))want=1;
					if((pass==1)&&(tvWin[w].num==0)&&(w==key)&&!tvWin[w].modal)want=1;
					if(want)
					{
						if(w==top)tvWinMenuSel=k;
						TvWinMenuSet(k,w);
						k++;
					}
				}
	if(k==0)
	{
		TItem& it=tvItem[m.first];
		StrCpy(&it.text[0],"  (no windows)",32);
		it.hint[0]=0;
		it.cmd=0;
		it.key=0;
		it.sub=-1;
		k=1;
	}
	m.count=k;
}

void TvStatusAdd(char* text, int cmd, int key)
{
	if(tvNStatus>=TV_MAXSTATUS)return;
	TStatus& s=tvStatus[tvNStatus];
	StrCpy(&s.text[0],text,24);
	s.cmd=cmd;
	s.key=key;
	s.ctx=tvStatusFor;
	tvNStatus++;
}

// The context of the status line items added from now on (TV_ST_ALL at the start).
// An item with a command sends it when clicked, and its key sends it everywhere, as
// a menu item's does. An item with no command names a key of the window: the key
// goes to the window as usual, and a click on the item sends it there.
void TvStatusFor(int ctx)
{
	tvStatusFor=ctx;
}

void TvStatusUpdate()
{
	int top=TvTop();
	tvStCtx=TV_ST_DESK;
	if(top>=0)tvStCtx=TvAppStatus(top);
	tvStOwn=0;
	for(int i=0;i<tvNStatus;i++)
		if(tvStatus[i].ctx==tvStCtx)tvStOwn=1;
}

int TvStatusShown(int i);

// The space between status line items: two, or one when they do not fit so.
int TvStatusGap()
{
	int n=0;
	int w=1;
	for(int i=0;i<tvNStatus;i++)
		if(TvStatusShown(i))
		{
			w=w+HotLen(&tvStatus[i].text[0]);
			n++;
		}
	if(w+2*(n-1)>tvW)return 1;
	return 2;
}

// 1 if status line item i shows now (after TvStatusUpdate).
int TvStatusShown(int i)
{
	int c=tvStatus[i].ctx;
	if(c==tvStCtx)return 1;
	if(tvStCtx==TV_ST_DIALOG)
		if(tvStOwn)return 0;
	if(c==TV_ST_ALL)return 1;
	if(c==TV_ST_OTHER)
		if(!tvStOwn)return 1;
	return 0;
}

int TvMenuX(int n)
{
	int x=1;
	for(int i=0;i<n;i++)
		x=x+HotLen(&tvMenu[i].title[0])+2;
	return x;
}

void TvMenuBox(int n, int& x, int& y, int& w, int& h)
{
	int wid=10;
	TMenu& m=tvMenu[n];
	for(int i=0;i<m.count;i++)
	{
		TItem& it=tvItem[m.first+i];
		int l=HotLen(&it.text[0])+StrLen(&it.hint[0])+6;
		if(l>wid)wid=l;
	}
	x=TvMenuX(n)-1;
	y=1;
	w=wid;
	h=m.count+2;
	if(m.popup)
	{
		x=popX;
		y=popY;
		if(m.popup==2)
		{
			x=subX;
			y=subY;
		}
		// room for the shadow above the status line
		if(y+h>tvH-2)y=tvH-2-h;
		if(y<1)y=1;
	}
	if(x+w>tvW)x=tvW-w;
	if(x<0)x=0;
}

int TvBarCount();

// The window with number n, or -1.
int TvNumWin(int n)
{
	for(int i=0;i<TV_MAXWIN;i++)
		if(tvWin[i].used)
			if(tvWin[i].num==n)return i;
	return -1;
}

int tvBarStep=2;   // columns per window number on the menu bar

// Where window number n is on the menu bar, or -1 when it is not shown. The
// numbers sit right-aligned before the clock, two columns apart when there is
// room, else one.
int TvBarNumX(int n)
{
	if(TvNumWin(n)<0)return -1;
	int count=0;
	int k=0;
	for(int i=1;i<=TV_MAXNUM;i++)
		if(TvNumWin(i)>=0)
		{
			if(i<n)k++;
			count++;
		}
	int left=0;
	int bars=TvBarCount();
	if(bars>0)left=TvMenuX(bars-1)+HotLen(&tvMenu[bars-1].title[0])+2;
	int right=tvW-11;
	tvBarStep=2;
	if(right-left<count*2)tvBarStep=1;
	int x=right-(count-1-k)*tvBarStep;
	if(x<left)return -1;
	return x;
}

void TvDrawMenuBar()
{
	TvClip(0,0,tvW,tvH);
	TvFill(0,0,tvW,1,' ',A_MENU);
	for(int i=0;i<tvNMenu;i++)
	if(!tvMenu[i].popup)
	{
		int x=TvMenuX(i);
		int at=A_MENU;
		int hot=A_MENU_HOT;
		if(menuOpen==i)
		{
			at=A_MENU_SEL;
			hot=A_MENU_SELHOT;
			TvFill(x-1,0,HotLen(&tvMenu[i].title[0])+2,1,' ',at);
		}
		TvHotText(x,0,&tvMenu[i].title[0],at,hot);
	}
	char clk[16];
	snprintf(&clk[0],16,"%02i:%02i:%02i",term_time(0),term_time(1),term_time(2));
	TvText(tvW-9,0,&clk[0],A_MENU);
	// the window numbers, to bring one to the front with a click
	int top=TvTop();
	for(int n=1;n<=TV_MAXNUM;n++)
	{
		int w=TvNumWin(n);
		int x=TvBarNumX(n);
		if((w>=0)&&(x>=0))
		{
			int at=A_MENU;
			if(w==top)at=A_MENU_SEL;
			if(tvBarStep==2)TvPut(x-1,0,' ',at);
			TvPut(x,0,TvNumChar(n),at);
			if(tvBarStep==2)TvPut(x+1,0,' ',at);
		}
	}
}

// Draws menu n with item sel highlighted.
void TvDrawMenuList(int n, int sel)
{
	int x;
	int y;
	int w;
	int h;
	TvMenuBox(n,x,y,w,h);
	TvClip(0,0,tvW,tvH);
	TvShadow(x,y,w,h);
	TvFill(x,y,w,h,' ',A_MENU);
	TvFrame(x,y,w,h,0,A_MENU);
	TMenu& m=tvMenu[n];
	for(int i=0;i<m.count;i++)
	{
		TItem& it=tvItem[m.first+i];
		int ry=y+1+i;
		if(it.text[0]==0)
		{
			for(int k=1;k<w-1;k++)
				TvPut(x+k,ry,G_HORZ,A_MENU);
			TvPut(x,ry,0x251C,A_MENU);
			TvPut(x+w-1,ry,0x2524,A_MENU);
		}
		else
		{
			int at=A_MENU;
			int hot=A_MENU_HOT;
			if(i==sel)
			{
				at=A_MENU_SEL;
				hot=A_MENU_SELHOT;
				TvFill(x+1,ry,w-2,1,' ',at);
			}
			TvHotText(x+2,ry,&it.text[0],at,hot);
			TvText(x+w-2-StrLen(&it.hint[0]),ry,&it.hint[0],at);
			if(it.sub>=0)TvPut(x+w-2,ry,0x25BA,at);
		}
	}
}

void TvDrawDropDown()
{
	if(menuOpen<0)return;
	TvDrawMenuList(menuOpen,menuSel);
	if(menuSub>=0)TvDrawMenuList(menuSub,subSel);
}

void TvDrawStatus()
{
	TvClip(0,0,tvW,tvH);
	TvFill(0,tvH-1,tvW,1,' ',A_STATUS);
	TvStatusUpdate();
	int gap=TvStatusGap();
	int x=1;
	for(int i=0;i<tvNStatus;i++)
	{
		if(!TvStatusShown(i))continue;
		int at=A_STATUS;
		int hot=A_STATUS_HOT;
		if((dragMode==8)&&(dragCtl==i)&&pressedInside)
		{
			at=A_STATUS_SEL;
			hot=A_STATUS_SELHOT;
		}
		x=TvHotText(x,tvH-1,&tvStatus[i].text[0],at,hot);
		x=x+gap;
	}
}

void TvRedraw()
{
	tvCurX=-1;
	tvPixN=0;
	TvClip(0,0,tvW,tvH);
	TvFill(0,1,tvW,tvH-2,G_LIGHT,A_DESK);
	for(int i=0;i<tvNZ;i++)
		TvDrawWin(tvZ[i]);
	TvDrawMenuBar();
	TvDrawStatus();
	TvDrawCombo();
	TvDrawDropDown();
	if(menuOpen>=0)tvCurX=-1;
	TvFlush();
	tvDirty=0;
}

// ---------------------------------------------------------------- input decoding

int ReadParam(int& c)
{
	int n=0;
	while((c>='0')&&(c<='9'))
	{
		n=n*10+c-'0';
		c=term_read(50);
	}
	return n;
}

void DecodeMouse()
{
	int c=term_read(50);
	int b=ReadParam(c);
	c=term_read(50);
	int x=ReadParam(c);
	c=term_read(50);
	int y=ReadParam(c);
	evX=x-1;
	evY=y-1;
	evBtn=b&3;
	if(b&64)
	{
		evType=EV_WHEEL;
		evWheel=1;
		if(b&1)evWheel=-1;
		return;
	}
	if(c=='m')
		evType=EV_MUP;
	else if(b&32)
		evType=EV_MDRAG;
	else
		evType=EV_MDOWN;
}

// Reads bracketed paste up to its closing ESC[201~, or until the input goes quiet.
void ReadPaste()
{
	char* close="\x1b[201~";
	int match=0;
	int cr=0;
	tvPasteN=0;
	tvPasteLost=0;
	while(true)
	{
		int c=term_read(500);
		if(c<0)break;
		int want=close[match];
		if(c==want)
		{
			match++;
			if(match==6)break;
		}
		else
		{
			// nothing but ESC starts the closing sequence, so a failed match is plain text
			for(int k=0;k<match;k++)
			{
				if(tvPasteN<TV_PASTEMAX)
				{
					tvPaste[tvPasteN]=close[k];
					tvPasteN++;
				}
				else tvPasteLost=1;
			}
			match=0;
			if(c==27)
				match=1;
			else
			{
				// a line end is CR, LF or CR LF, and a NUL cannot be held in a line
				int skip=(c==0);
				int isCr=(c==13);
				if(isCr)c=10;
				else if((c==10)&&cr)skip=1;
				cr=isCr;
				if(!skip)
				{
					if(tvPasteN<TV_PASTEMAX)
					{
						tvPaste[tvPasteN]=c;
						tvPasteN++;
					}
					else tvPasteLost=1;
				}
			}
		}
	}
	evType=EV_PASTE;
}

void DecodeCSI()
{
	int c=term_read(50);
	if(c=='<')
	{
		DecodeMouse();
		return;
	}
	int p=ReadParam(c);
	evMod=0;
	if(c==';')
	{
		c=term_read(50);
		int m=ReadParam(c);
		if(m>0)evMod=m-1;
		while(c==';')
		{
			c=term_read(50);
			ReadParam(c);
		}
	}
	if((c=='~')&&(p==200))
	{
		ReadPaste();
		return;
	}
	evType=EV_KEY;
	evKey=0;
	if(c=='A')evKey=K_UP;
	if(c=='B')evKey=K_DOWN;
	if(c=='C')evKey=K_RIGHT;
	if(c=='D')evKey=K_LEFT;
	if(c=='H')evKey=K_HOME;
	if(c=='F')evKey=K_END;
	if(c=='Z')evKey=K_BTAB;
	// F1..F4 with a modifier: ESC [1;2S is Shift+F4
	if((c>='P')&&(c<='S'))evKey=K_F1+c-'P';
	if(c=='~')
	{
		if((p==1)||(p==7))evKey=K_HOME;
		if((p==4)||(p==8))evKey=K_END;
		if(p==2)evKey=K_INS;
		if(p==3)evKey=K_DEL;
		if(p==5)evKey=K_PGUP;
		if(p==6)evKey=K_PGDN;
		if(p==29)evKey=K_MENU;
		if((p>=11)&&(p<=15))evKey=K_F1+p-11;
		if((p>=17)&&(p<=21))evKey=K_F6+p-17;
		if((p>=23)&&(p<=24))evKey=K_F11+p-23;
	}
	if(evKey==0)evType=EV_NONE;
}

// TV_GL (with TV_GFX) picks the OpenGL backend instead of the Graph one. The
// choice is made with a marker, since #ifdef blocks must not nest here.
#ifdef TV_GFX
#ifndef TV_TERM_HINT
#define TV_TERM_HINT "the terminal build of this program"
#endif
// Leaves at once, with a word on what to run instead, when there is no display to
// open the window on: over ssh, or on a text console. A shared memory or OpenGL
// window does not work across the network even with X11 forwarding.
void TvNoDisplay(char* why)
{
	printf("%s\n",why);
	printf("This build draws in a window, so it needs a local graphic desktop; over ssh\n");
	printf("or on a text console run " TV_TERM_HINT " instead.\n");
	exit(1);
}

void TvNeedDisplay()
{
	if(!run_has_display())TvNoDisplay("No graphic display: neither DISPLAY nor WAYLAND_DISPLAY is set.");
}
#endif

#ifdef TV_GFX
#define TV_GFX_GRAPH
#endif
#ifdef TV_GL
#undef TV_GFX_GRAPH
#include "../tui/tvgl.h"
#endif
#ifdef TV_GFX_GRAPH
#include "../tui/tvgfx.h"
#endif

void TvReadEvent(int timeoutMs)
{
#ifdef TV_GFX
	GfxReadEvent(timeoutMs);
	return;
#endif
	evType=EV_NONE;
	evMod=0;
	evUtfN=0;
	int c=term_read(timeoutMs);
	if(c<0)return;
	if(c==27)
	{
		int n=term_read(25);
		if(n<0)
		{
			evType=EV_KEY;
			evKey=K_ESC;
		}
		else if(n=='[')
			DecodeCSI();
		else if(n=='O')
		{
			int k=term_read(50);
			evType=EV_KEY;
			evKey=0;
			if((k>='P')&&(k<='S'))evKey=K_F1+k-'P';
			if(k=='A')evKey=K_UP;
			if(k=='B')evKey=K_DOWN;
			if(k=='C')evKey=K_RIGHT;
			if(k=='D')evKey=K_LEFT;
			if(k=='H')evKey=K_HOME;
			if(k=='F')evKey=K_END;
			if(evKey==0)evType=EV_NONE;
		}
		else
		{
			evType=EV_KEY;
			evKey=K_ALT+LowerKey(n);
		}
		return;
	}
	if(c>=0xC0)
	{
		// a multi-byte UTF-8 character: evUtf holds it, evKey is a stand-in
		int more=1;
		if(c>=0xE0)more=2;
		if(c>=0xF0)more=3;
		evUtf[0]=c;
		evUtfN=1;
		for(int i=0;i<more;i++)
		{
			int n=term_read(10);
			if(n>=0)
			{
				evUtf[evUtfN]=n;
				evUtfN++;
			}
		}
		evUtf[evUtfN]=0;
		if(evUtfN!=more+1)evUtfN=0;
		c='?';
	}
	if(c==8)c=K_BACKSPACE;
	if(c==10)c=K_ENTER;
	evType=EV_KEY;
	evKey=c;
}

// ---------------------------------------------------------------- behaviour

void TvZoom(int id)
{
	if(id<0)return;
	TWin& t=tvWin[id];
	if(!(t.flags&WF_RESIZE))return;
	if(t.zoomed)
	{
		t.x=t.zx;
		t.y=t.zy;
		t.w=t.zw;
		t.h=t.zh;
		t.zoomed=0;
	}
	else
	{
		t.zx=t.x;
		t.zy=t.y;
		t.zw=t.w;
		t.zh=t.h;
		t.x=0;
		t.y=1;
		t.w=tvW;
		t.h=tvH-2;
		t.zoomed=1;
	}
	tvDirty=1;
}

void TvCascade()
{
	int n=0;
	for(int i=0;i<tvNZ;i++)
	{
		TWin& t=tvWin[tvZ[i]];
		t.zoomed=0;
		t.x=n*2;
		t.y=1+n;
		if(t.flags&WF_RESIZE)
		{
			t.w=tvW-t.x;
			t.h=tvH-1-t.y;
		}
		n++;
	}
	tvDirty=1;
}

void TvTile()
{
	int cnt=0;
	for(int i=0;i<tvNZ;i++)
		if(tvWin[tvZ[i]].flags&WF_RESIZE)cnt++;
	if(cnt==0)return;
	int cols=1;
	while(cols*cols<cnt)cols++;
	int rows=(cnt+cols-1)/cols;
	int cw=tvW/cols;
	int ch=(tvH-2)/rows;
	int n=0;
	for(int i=0;i<tvNZ;i++)
	{
		TWin& t=tvWin[tvZ[i]];
		if(t.flags&WF_RESIZE)
		{
			t.zoomed=0;
			t.x=(n%cols)*cw;
			t.y=1+(n/cols)*ch;
			t.w=cw;
			t.h=ch;
			if(n%cols==cols-1)t.w=tvW-t.x;
			if(n/cols==rows-1)t.h=tvH-1-t.y;
			n++;
		}
	}
	tvDirty=1;
}

void TvOpenMenu(int n);

void TvWinList();

void TvCommand(int cmd)
{
	tvDirty=1;
	int top=TvTop();
	if(cmd==CM_NEXT)
	{
		// the back window comes to the front
		if(tvNZ>1)TvToFront(tvZ[0]);
	}
	else if(cmd==CM_PREV)
	{
		if(tvNZ>1)
		{
			for(int i=tvNZ-1;i>0;i--)
				tvZ[i]=tvZ[i-1];
			tvZ[0]=top;
		}
	}
	else if(cmd==CM_ZOOM)TvZoom(top);
	else if(cmd==CM_CLOSE)
	{
		if(top>=0)
			if(TvAppCanClose(top))TvClose(top);
	}
	else if(cmd==CM_CASCADE)TvCascade();
	else if(cmd==CM_TILE)TvTile();
	else if(cmd==CM_MENU)TvOpenMenu(0);
	else if(cmd==CM_WINLIST)
	{
		if(top>=0)
			if(tvWin[top].modal)return;
		TvWinList();
	}
	else if((cmd>=CM_WINGO)&&(cmd<CM_WINGO+TV_MAXWIN))
	{
		if(top>=0)
			if(tvWin[top].modal)return;
		if(tvWin[cmd-CM_WINGO].used)TvToFront(cmd-CM_WINGO);
	}
	else TvAppCommand(cmd);
}

void TvFire(int win, int id)
{
	TCtl& c=tvCtl[id];
	if(tvWin[win].modal)
		tvWin[win].result=c.cmd;
	else
		TvAppButton(win,id,c.cmd);
	tvDirty=1;
}

void TvSelect(int win, int id)
{
	TCtl& c=tvCtl[id];
	if(c.type==C_CHECK)
	{
		c.val=!c.val;
		if(c.cmd!=0)TvChanged(win,id);
	}
	else if(c.type==C_RADIO)
	{
		for(int i=0;i<TV_MAXCTL;i++)
			if(tvCtl[i].used)
				if((tvCtl[i].win==win)&&(tvCtl[i].type==C_RADIO)&&(tvCtl[i].group==c.group))
					tvCtl[i].val=0;
		c.val=1;
		if(c.cmd!=0)TvChanged(win,id);
	}
	else if(c.type==C_BUTTON)
		TvFire(win,id);
	else if(c.type==C_COMBO)
		TvComboOpen(id);
	tvDirty=1;
}

void TvFocusStep(int win, int dir)
{
	TWin& t=tvWin[win];
	int i=t.focus;
	if(i<0)i=0;
	for(int k=0;k<TV_MAXCTL;k++)
	{
		i=(i+dir+TV_MAXCTL)%TV_MAXCTL;
		if(tvCtl[i].used)
			if((tvCtl[i].win==win)&&TvFocusable(tvCtl[i].type))
			{
				t.focus=i;
				tvDirty=1;
				return;
			}
	}
}

void TvInputInsert(TCtl& c, char* s, int n)
{
	int len=StrLen(&c.text[0]);
	if(len+n>198)return;
	for(int i=len;i>=c.val;i--)
		c.text[i+n]=c.text[i];
	for(int i=0;i<n;i++)
		c.text[c.val+i]=s[i];
	c.val=c.val+n;
}

// Keys for the focused control. Returns 1 if the key was used.
int TvCtlKey(int win, int key)
{
	TWin& t=tvWin[win];
	if(t.focus<0)return 0;
	TCtl& c=tvCtl[t.focus];
	if(c.type==C_INPUT)
	{
		int len=StrLen(&c.text[0]);
		if(key==K_LEFT){c.val=TvPrevChar(&c.text[0],c.val);return 1;}
		if(key==K_RIGHT){if(c.val<len)TvUtf8(&c.text[0],c.val);return 1;}
		if(key==K_HOME){c.val=0;return 1;}
		if(key==K_END){c.val=len;return 1;}
		if(key==K_BACKSPACE)
		{
			if(c.val>0)
			{
				int p=TvPrevChar(&c.text[0],c.val);
				int gone=c.val-p;
				for(int i=p;i<=len-gone;i++)
					c.text[i]=c.text[i+gone];
				c.val=p;
			}
			return 1;
		}
		if(key==K_DEL)
		{
			if(c.val<len)
			{
				int q=c.val;
				TvUtf8(&c.text[0],q);
				int gone=q-c.val;
				for(int i=c.val;i<=len-gone;i++)
					c.text[i]=c.text[i+gone];
			}
			return 1;
		}
		if(evUtfN>0)
		{
			TvInputInsert(c,&evUtf[0],evUtfN);
			return 1;
		}
		if((key>=32)&&(key<127))
		{
			char one[2];
			one[0]=key;
			one[1]=0;
			TvInputInsert(c,&one[0],1);
			return 1;
		}
		return 0;
	}
	if(c.type==C_LIST)
	{
		int n=TvListCount(c);
		int v=c.val;
		if(key==K_UP)v--;
		else if(key==K_DOWN)v++;
		else if(key==K_PGUP)v=v-c.h;
		else if(key==K_PGDN)v=v+c.h;
		else if(key==K_HOME)v=0;
		else if(key==K_END)v=n-1;
		else return 0;
		if(v>=n)v=n-1;
		if(v<0)v=0;
		c.val=v;
		return 1;
	}
	if(c.type==C_SLIDER)
	{
		int v=c.val;
		int step=c.count/10;
		if(step<1)step=1;
		if(key==K_LEFT)v--;
		else if(key==K_RIGHT)v++;
		else if(key==K_PGDN)v=v-step;
		else if(key==K_PGUP)v=v+step;
		else if(key==K_HOME)v=0;
		else if(key==K_END)v=c.count;
		else return 0;
		TvSliderSet(win,t.focus,v);
		return 1;
	}
	if(c.type==C_COMBO)
	{
		if((key==K_F4)||((key==K_DOWN)&&(evMod&MOD_ALT)))
		{
			TvComboOpen(t.focus);
			return 1;
		}
		int n=TvListCount(c);
		int v=c.val;
		if(key==K_UP)v--;
		else if(key==K_DOWN)v++;
		else if(key==K_HOME)v=0;
		else if(key==K_END)v=n-1;
		else if(key!=' ')return 0;
		if(v>=n)v=n-1;
		if(v<0)v=0;
		if(v!=c.val)
		{
			c.val=v;
			TvChanged(win,t.focus);
		}
		if(key!=' ')return 1;
	}
	if(key==' ')
	{
		TvSelect(win,t.focus);
		return 1;
	}
	return 0;
}

// Presses the window's default button. Returns 1 if it has one.
int TvFireDefault(int win)
{
	for(int i=0;i<TV_MAXCTL;i++)
		if(tvCtl[i].used)
			if((tvCtl[i].win==win)&&tvCtl[i].isDefault)
			{
				TvFire(win,i);
				return 1;
			}
	return 0;
}

int TvDialogKey(int win, int key)
{
	TWin& t=tvWin[win];
	if(TvCtlKey(win,key))
	{
		tvDirty=1;
		return 1;
	}
	if(key==K_TAB){TvFocusStep(win,1);return 1;}
	if(key==K_BTAB){TvFocusStep(win,-1);return 1;}
	if((key==K_DOWN)||(key==K_RIGHT)){if(t.focus>=0){TvFocusStep(win,1);return 1;}}
	if((key==K_UP)||(key==K_LEFT)){if(t.focus>=0){TvFocusStep(win,-1);return 1;}}
	if(key==K_ENTER)
	{
		if(t.focus>=0)
			if(tvCtl[t.focus].type==C_BUTTON)
			{
				TvFire(win,t.focus);
				return 1;
			}
		return TvFireDefault(win);
	}
	// hot letters: Alt+letter anywhere, the bare letter unless typing in a field
	int letter=0;
	if(key>K_ALT)
		letter=key-K_ALT;
	else if(key<127)
	{
		letter=LowerKey(key);
		if(t.focus>=0)
			if(tvCtl[t.focus].type==C_INPUT)letter=0;
	}
	if(letter!=0)
	{
		for(int i=0;i<TV_MAXCTL;i++)
			if(tvCtl[i].used)
				if(tvCtl[i].win==win)
					if(HotChar(&tvCtl[i].text[0])==letter)
					{
						if(tvCtl[i].type==C_LABEL)
						{
							// a label moves the focus to the control after it
							t.focus=i;
							TvFocusStep(win,1);
						}
						else
						{
							t.focus=i;
							TvSelect(win,i);
						}
						return 1;
					}
	}
	return 0;
}

int TvCtlAt(int win, int mx, int my)
{
	TWin& t=tvWin[win];
	for(int i=0;i<TV_MAXCTL;i++)
	{
		if(tvCtl[i].used)
			if((tvCtl[i].win==win)&&TvFocusable(tvCtl[i].type))
			{
				TCtl& c=tvCtl[i];
				int x=t.x+c.x;
				int y=t.y+c.y;
				if((mx>=x)&&(mx<x+c.w)&&(my>=y)&&(my<y+c.h))
					return i;
			}
	}
	return -1;
}

// Sends every EV_MDRAG to this window until the button is released, wherever the
// pointer goes. For an application's own scroll bar thumb.
void TvCaptureMouse(int win)
{
	dragWin=win;
	dragMode=4;
}

void TvListDragThumb(int win, int id, int my)
{
	TWin& t=tvWin[win];
	TCtl& c=tvCtl[id];
	int mxTop=TvListCount(c)-c.h;
	if(mxTop<0)mxTop=0;
	c.top=TvVBarPos(t.y+c.y,c.h,mxTop,my);
	// drawing scrolls the selection into view, so keep it inside the visible rows
	if(c.val<c.top)c.val=c.top;
	if(c.val>=c.top+c.h)c.val=c.top+c.h-1;
}

int lastClickCtl=-1;
float lastClickTime=0.;

void TvCtlMouseDown(int win, int id, int mx, int my)
{
	TWin& t=tvWin[win];
	TCtl& c=tvCtl[id];
	t.focus=id;
	tvDirty=1;
	if(c.type==C_BUTTON)
	{
		dragMode=3;
		dragCtl=id;
		dragWin=win;
		pressedInside=1;
	}
	else if((c.type==C_CHECK)||(c.type==C_RADIO))
	{
		dragMode=3;
		dragCtl=id;
		dragWin=win;
		pressedInside=1;
	}
	else if(c.type==C_SLIDER)
	{
		TvSliderSet(win,id,TvSliderAt(win,id,mx));
		dragMode=6;
		dragCtl=id;
		dragWin=win;
	}
	else if(c.type==C_COMBO)
		TvComboOpen(id);
	else if(c.type==C_VIEW)
	{
		TvCaptureMouse(win);
		TvAppMouse(win,EV_MDOWN,mx-t.x,my-t.y);
	}
	else if(c.type==C_INPUT)
	{
		int cols=mx-(t.x+c.x);
		int p=c.top;
		while((cols>0)&&(c.text[p]!=0))
		{
			TvUtf8(&c.text[0],p);
			cols--;
		}
		c.val=p;
	}
	else if(c.type==C_LIST)
	{
		int n=TvListCount(c);
		int ry=my-(t.y+c.y);
		if(mx==t.x+c.x+c.w-1)
		{
			int mxTop=n-c.h;
			if(mxTop<0)mxTop=0;
			int h=TvVBarHit(t.y+c.y,c.h,c.top,mxTop,my);
			if(h==2)
			{
				dragWin=win;
				dragCtl=id;
				dragMode=5;
				return;
			}
			int v=c.val;
			if(h==0)v--;
			if(h==1)v=v-c.h;
			if(h==3)v=v+c.h;
			if(h==4)v++;
			if(v>=n)v=n-1;
			if(v<0)v=0;
			c.val=v;
		}
		else if(c.top+ry<n)
		{
			// a second click on the same row soon after is a double click
			int k=c.top+ry;
			float now=Time();
			if((k==c.val)&&(id==lastClickCtl)&&(now-lastClickTime<0.4))
			{
				lastClickCtl=-1;
				TvFireDefault(win);
				return;
			}
			c.val=k;
			lastClickCtl=id;
			lastClickTime=now;
		}
	}
}

int TvWinAt(int x, int y)
{
	for(int i=tvNZ-1;i>=0;i--)
	{
		TWin& t=tvWin[tvZ[i]];
		if((x>=t.x)&&(x<t.x+t.w)&&(y>=t.y)&&(y<t.y+t.h))
			return tvZ[i];
	}
	return -1;
}

// The number of menus on the menu bar, which come before the context menus.
int TvBarCount()
{
	int n=0;
	while(n<tvNMenu)
	{
		if(tvMenu[n].popup)break;
		n++;
	}
	return n;
}

void TvCloseMenu()
{
	menuOpen=-1;
	menuSub=-1;
	tvDirty=1;
}

void TvOpenMenu(int n)
{
	menuOpen=n;
	menuSub=-1;
	menuSel=0;
	if(n==tvWinMenu)
	{
		TvWinMenuFill();
		menuSel=tvWinMenuSel;
	}
	else if(tvItem[tvMenu[n].first].text[0]==0)menuSel=1;
	tvDirty=1;
}

// Opens a context menu with its top left corner at x,y, or at the text cursor
// (else the middle of the top window) when x<0.
void TvOpenPopup(int n, int x, int y)
{
	if((n<0)||(n>=tvNMenu))return;
	if(x<0)
	{
		x=tvW/2;
		y=tvH/2;
		if(tvCurX>=0)
		{
			x=tvCurX;
			y=tvCurY+1;
		}
		else
		{
			int top=TvTop();
			if(top>=0)
			{
				x=tvWin[top].x+tvWin[top].w/3;
				y=tvWin[top].y+2;
			}
		}
	}
	popX=x;
	popY=y;
	popArmed=0;
	TvOpenMenu(n);
}

// Opens the submenu of the selected item, beside it: on the right, or on the left
// when the right has no room.
void TvOpenSub()
{
	int s=tvItem[tvMenu[menuOpen].first+menuSel].sub;
	if(s<0)return;
	int x;
	int y;
	int w;
	int h;
	TvMenuBox(menuOpen,x,y,w,h);
	menuSub=s;
	subSel=0;
	if(tvItem[tvMenu[s].first].text[0]==0)subSel=1;
	subX=x+w-1;
	subY=y+menuSel;
	int sx;
	int sy;
	int sw;
	int sh;
	TvMenuBox(s,sx,sy,sw,sh);
	if(x+w-1+sw>tvW)subX=x-sw+1;
	tvDirty=1;
}

void TvMenuPick()
{
	int n=menuOpen;
	int sel=menuSel;
	if(menuSub>=0)
	{
		n=menuSub;
		sel=subSel;
	}
	TItem& it=tvItem[tvMenu[n].first+sel];
	if(it.sub>=0)
	{
		TvOpenSub();
		return;
	}
	int cmd=it.cmd;
	TvCloseMenu();
	if(cmd!=0)TvCommand(cmd);
}

// Moves the highlight in the submenu if one is open, else in the menu.
void TvMenuMove(int dir)
{
	int n=menuOpen;
	int sel=menuSel;
	if(menuSub>=0)
	{
		n=menuSub;
		sel=subSel;
	}
	TMenu& m=tvMenu[n];
	for(int k=0;k<m.count;k++)
	{
		sel=(sel+dir+m.count)%m.count;
		if(tvItem[m.first+sel].text[0]!=0)break;
	}
	if(menuSub>=0)subSel=sel;
	else menuSel=sel;
}

// The item of menu n whose hot letter is key, or -1.
int TvMenuHot(int n, int key)
{
	TMenu& m=tvMenu[n];
	for(int i=0;i<m.count;i++)
		if(HotChar(&tvItem[m.first+i].text[0])==LowerKey(key))return i;
	return -1;
}

void TvMenuKey(int key)
{
	tvDirty=1;
	if(menuSub>=0)
	{
		if((key==K_ESC)||(key==K_LEFT))
		{
			menuSub=-1;
			return;
		}
		if((key==K_UP)||(key==K_DOWN)||(key==K_ENTER)||(key<127))
		{
			if(key==K_UP)TvMenuMove(-1);
			else if(key==K_DOWN)TvMenuMove(1);
			else if(key==K_ENTER)TvMenuPick();
			else
			{
				int i=TvMenuHot(menuSub,key);
				if(i>=0)
				{
					subSel=i;
					TvMenuPick();
				}
			}
			return;
		}
		// Right, F10 and Alt+letter go on to the menu bar
		menuSub=-1;
		if(tvMenu[menuOpen].popup&&(key==K_RIGHT))return;
	}
	if((key==K_RIGHT)&&(tvItem[tvMenu[menuOpen].first+menuSel].sub>=0))TvOpenSub();
	else if(key==K_ESC)TvCloseMenu();
	else if(key==K_F10)TvCloseMenu();
	else if(tvMenu[menuOpen].popup&&((key==K_LEFT)||(key==K_RIGHT))){}
	else if(key==K_LEFT)TvOpenMenu((menuOpen+TvBarCount()-1)%TvBarCount());
	else if(key==K_RIGHT)TvOpenMenu((menuOpen+1)%TvBarCount());
	else if(key==K_UP)TvMenuMove(-1);
	else if(key==K_DOWN)TvMenuMove(1);
	else if(key==K_ENTER)TvMenuPick();
	else if(key>K_ALT)
	{
		for(int i=0;i<tvNMenu;i++)
			if(HotChar(&tvMenu[i].title[0])==key-K_ALT)TvOpenMenu(i);
	}
	else if(key<127)
	{
		int i=TvMenuHot(menuOpen,key);
		if(i>=0)
		{
			menuSel=i;
			TvMenuPick();
		}
	}
}

// Returns 1 if the menu is open or was opened by this click.
int TvMenuMouse(int ev)
{
	if(evY==0)
	{
		if((ev==EV_MDOWN)||(menuOpen>=0))
			for(int i=0;i<TvBarCount();i++)
			{
				int x=TvMenuX(i);
				if((evX>=x-1)&&(evX<x+HotLen(&tvMenu[i].title[0])+1))
				{
					if((ev==EV_MDOWN)&&(menuOpen==i))
						TvCloseMenu();
					else if(menuOpen!=i)
						TvOpenMenu(i);
					return 1;
				}
			}
		if((ev==EV_MDOWN)&&(menuOpen<0))
			for(int n=1;n<=TV_MAXNUM;n++)
			{
				int x=TvBarNumX(n);
				if(x>=0)
					if((evX==x)||((tvBarStep==2)&&(evX==x-1)))
					{
						TvToFront(TvNumWin(n));
						return 1;
					}
			}
		if(menuOpen<0)return 1;
	}
	if(menuOpen<0)return 0;
	int x;
	int y;
	int w;
	int h;
	if(menuSub>=0)
	{
		TvMenuBox(menuSub,x,y,w,h);
		if((evX>=x)&&(evX<x+w)&&(evY>=y)&&(evY<y+h))
		{
			int i=evY-y-1;
			if((evX>x)&&(evX<x+w-1)&&(i>=0)&&(i<h-2))
				if(tvItem[tvMenu[menuSub].first+i].text[0]!=0)
				{
					subSel=i;
					tvDirty=1;
					if(ev!=EV_MUP)popArmed=1;
					else if(popArmed||!tvMenu[menuOpen].popup)
						TvMenuPick();
				}
			return 1;
		}
	}
	TvMenuBox(menuOpen,x,y,w,h);
	if((evX>x)&&(evX<x+w-1)&&(evY>y)&&(evY<y+h-1))
	{
		int i=evY-y-1;
		TItem& it=tvItem[tvMenu[menuOpen].first+i];
		if((it.text[0]!=0)&&(it.sub>=0))
		{
			// pressing or dragging onto it opens its submenu; releasing there keeps it
			menuSel=i;
			if(ev!=EV_MUP)popArmed=1;
			if(menuSub!=it.sub)TvOpenSub();
			tvDirty=1;
		}
		else if(it.text[0]!=0)
		{
			menuSel=i;
			menuSub=-1;
			tvDirty=1;
			// the release of the click that opened a context menu picks nothing
			if(ev!=EV_MUP)popArmed=1;
			else if(popArmed||!tvMenu[menuOpen].popup)
				TvMenuPick();
		}
		return 1;
	}
	if(ev==EV_MDOWN)TvCloseMenu();
	return 1;
}

// The close (1) or zoom (2) box of window win under x,y, or 0.
int TvFramePart(int win, int x, int y)
{
	if(win<0)return 0;
	if(TvWinAt(x,y)!=win)return 0;
	TWin& t=tvWin[win];
	int rx=x-t.x;
	if(y!=t.y)return 0;
	if((rx>=2)&&(rx<=4))return 1;
	if(t.flags&WF_RESIZE)
		if((rx>=t.w-5)&&(rx<=t.w-3))return 2;
	return 0;
}

void TvKey();

// A click on status line item i: its command, or its key to the front window.
void TvStatusPress(int i)
{
	TStatus& s=tvStatus[i];
	if(s.cmd!=0)
	{
		TvCommand(s.cmd);
		return;
	}
	if(s.key==0)return;
	evKey=s.key;
	evMod=0;
	TvKey();
}

// The status line item at column x, or -1.
int TvStatusAt(int x)
{
	TvStatusUpdate();
	int gap=TvStatusGap();
	int sx=1;
	for(int i=0;i<tvNStatus;i++)
	{
		if(!TvStatusShown(i))continue;
		int l=HotLen(&tvStatus[i].text[0]);
		if((x>=sx)&&(x<sx+l))return i;
		sx=sx+l+gap;
	}
	return -1;
}

void TvMouse()
{
	int ev=evType;
	if(comboCtl>=0)
	{
		TvComboMouse(ev);
		return;
	}
	if(ev==EV_WHEEL)
	{
		int win=TvWinAt(evX,evY);
		if(win<0)return;
		int top=TvTop();
		if(tvWin[top].modal)
			if(win!=top)return;
		TWin& t=tvWin[win];
		int c=TvCtlAt(win,evX,evY);
		tvDirty=1;
		if(c>=0)
		{
			int ty=tvCtl[c].type;
			if((ty==C_LIST)||(ty==C_COMBO))
			{
				t.focus=c;
				if(evWheel>0)TvCtlKey(win,K_UP);
				else TvCtlKey(win,K_DOWN);
				return;
			}
			if(ty==C_SLIDER)
			{
				t.focus=c;
				TvSliderSet(win,c,tvCtl[c].val+evWheel);
				return;
			}
		}
		TvAppMouse(win,EV_WHEEL,evX-t.x,evY-t.y);
		return;
	}
	// a drag in progress owns the mouse
	if(dragMode!=0)
	{
		tvDirty=1;
		if(ev==EV_MUP)
		{
			int mode=dragMode;
			int win=dragWin;
			int ctl=dragCtl;
			dragMode=0;
			dragWin=-1;
			dragCtl=-1;
			if(!pressedInside)return;
			if(mode==3)
				if(tvCtl[ctl].used)TvSelect(win,ctl);
			if(mode==7)
				if(tvWin[win].used)
				{
					if(ctl==2)TvZoom(win);
					else if(tvWin[win].modal)tvWin[win].result=CM_CANCEL;
					else if(TvAppCanClose(win))TvClose(win);
				}
			if(mode==8)
			{
				if(evY==tvH-1)TvStatusPress(ctl);
			}
			return;
		}
		if(dragMode==3)
		{
			pressedInside=(TvCtlAt(dragWin,evX,evY)==dragCtl);
			return;
		}
		if(dragMode==7)
		{
			pressedInside=(TvFramePart(dragWin,evX,evY)==dragCtl);
			return;
		}
		if(dragMode==8)
		{
			pressedInside=(evY==tvH-1)&&(TvStatusAt(evX)==dragCtl);
			return;
		}
		if(dragMode==5)
		{
			TvListDragThumb(dragWin,dragCtl,evY);
			return;
		}
		if(dragMode==6)
		{
			TvSliderSet(dragWin,dragCtl,TvSliderAt(dragWin,dragCtl,evX));
			return;
		}
		TWin& t=tvWin[dragWin];
		if(dragMode==4)
		{
			TvAppMouse(dragWin,EV_MDRAG,evX-t.x,evY-t.y);
			return;
		}
		if(dragMode==1)
		{
			t.x=evX-dragDX;
			t.y=evY-dragDY;
			if(t.y<1)t.y=1;
			if(t.y>tvH-2)t.y=tvH-2;
			if(t.x>tvW-4)t.x=tvW-4;
			if(t.x<4-t.w)t.x=4-t.w;
		}
		else if(dragMode==2)
		{
			t.w=evX-t.x+1;
			t.h=evY-t.y+1;
			if(t.w<16)t.w=16;
			if(t.h<5)t.h=5;
		}
		t.zoomed=0;
		return;
	}
	if(ev==EV_MUP)
	{
		if(menuOpen>=0)TvMenuMouse(ev);
		return;
	}
	int top=TvTop();
	int modal=0;
	if(top>=0)modal=tvWin[top].modal;
	if(!modal)
		if(TvMenuMouse(ev))
			return;
	if(ev!=EV_MDOWN)
		return;
	tvDirty=1;
	if(evBtn==2)
	{
		// a right click asks the window under it for a context menu
		if(evY==tvH-1)return;
		int win=TvWinAt(evX,evY);
		if(win<0)
		{
			if(!modal)
				if(tvDeskPopup>=0)TvOpenPopup(tvDeskPopup,evX,evY);
			return;
		}
		if(modal)
			if(win!=top)return;
		TvToFront(win);
		TWin& t=tvWin[win];
		int rx=evX-t.x;
		int ry=evY-t.y;
		if((rx>0)&&(ry>0)&&(rx<t.w-1)&&(ry<t.h-1))
			TvAppMouse(win,EV_RDOWN,rx,ry);
		return;
	}
	if(evY==tvH-1)
	{
		int item=TvStatusAt(evX);
		if(item>=0)
			if(modal)
				if(tvStatus[item].cmd!=0)item=-1;
		if(item>=0)
		{
			dragMode=8;
			dragCtl=item;
			dragWin=-1;
			pressedInside=1;
		}
		return;
	}
	int win=TvWinAt(evX,evY);
	if(win<0)return;
	if(modal)
		if(win!=top)return;
	TvToFront(win);
	TWin& t=tvWin[win];
	int rx=evX-t.x;
	int ry=evY-t.y;
	if(ry==0)
	{
		int part=TvFramePart(win,evX,evY);
		if(part>0)
		{
			dragMode=7;
			dragWin=win;
			dragCtl=part;
			pressedInside=1;
			return;
		}
		// a double click on the title zooms the window, or puts it back
		int now=term_ms();
		if((t.flags&WF_RESIZE)&&(tvTitleWin==win)&&(now-tvTitleMs<400))
		{
			tvTitleWin=-1;
			TvZoom(win);
			return;
		}
		tvTitleWin=win;
		tvTitleMs=now;
		dragWin=win;
		dragMode=1;
		dragDX=rx;
		dragDY=ry;
		return;
	}
	if((ry==t.h-1)&&(rx>=t.w-2))
		if(t.flags&WF_RESIZE)
		{
			dragWin=win;
			dragMode=2;
			return;
		}
	int c=TvCtlAt(win,evX,evY);
	if(c>=0)
	{
		TvCtlMouseDown(win,c,evX,evY);
		return;
	}
	TvAppMouse(win,EV_MDOWN,rx,ry);
}

void TvKey()
{
	int key=evKey;
	tvDirty=1;
	if(comboCtl>=0)
	{
		TvComboKey(key);
		return;
	}
	if(menuOpen>=0)
	{
		TvMenuKey(key);
		return;
	}
	int top=TvTop();
	int modal=0;
	if(top>=0)modal=tvWin[top].modal;
	if(!modal)
	{
		if((key==K_MENU)||((key==K_F10)&&(evMod&MOD_SHIFT)))
		{
			if(top>=0)TvAppMouse(top,EV_RDOWN,-1,-1);
			else if(tvDeskPopup>=0)TvOpenPopup(tvDeskPopup,-1,-1);
			return;
		}
		if(key==K_F10)
		{
			TvOpenMenu(0);
			return;
		}
		if(key>K_ALT)
		{
			int letter=key-K_ALT;
			for(int i=0;i<tvNMenu;i++)
				if(HotChar(&tvMenu[i].title[0])==letter)
				{
					TvOpenMenu(i);
					return;
				}
			if((letter>='1')&&(letter<='9'))
			{
				for(int i=0;i<TV_MAXWIN;i++)
					if(tvWin[i].used)
						if(tvWin[i].num==letter-'0')
						{
							TvToFront(i);
							return;
						}
			}
		}
		for(int i=0;i<tvNStatus;i++)
			if((tvStatus[i].cmd!=0)&&(tvStatus[i].key!=0)&&(tvStatus[i].key==key))
			{
				TvCommand(tvStatus[i].cmd);
				return;
			}
		for(int i=0;i<tvNItem;i++)
			if((tvItem[i].key!=0)&&(tvItem[i].key==key))
			{
				TvCommand(tvItem[i].cmd);
				return;
			}
	}
	if(top<0)return;
	TWin& t=tvWin[top];
	if(TvAppKey(top,key))return;
	if(TvDialogKey(top,key))return;
	if(key==K_ESC)
	{
		if(t.modal)t.result=CM_CANCEL;
		else if(t.flags&WF_DIALOG)TvClose(top);
	}
}

void TvCheckSize()
{
	int w=0;
	int h=0;
#ifdef TV_GFX
	w=GfxCols();
	h=GfxRows();
#endif
#ifndef TV_GFX
	w=term_cols();
	h=term_rows();
#endif
	if(w>TV_MAXW)w=TV_MAXW;
	if(h>TV_MAXH)h=TV_MAXH;
	if((w==tvW)&&(h==tvH))return;
	tvW=w;
	tvH=h;
	for(int i=0;i<TV_CELLS;i++)
		tvPrevCh[i]=-1;
	for(int i=0;i<TV_MAXWIN;i++)
	{
		if(tvWin[i].used)
		{
			TWin& t=tvWin[i];
			if(t.zoomed)
			{
				t.w=tvW;
				t.h=tvH-2;
			}
			if(t.y>tvH-2)t.y=tvH-2;
			if(t.x>tvW-4)t.x=tvW-4;
		}
	}
#ifndef TV_GFX
	OutStr("\x1b[0m\x1b[2J");
#endif
	tvDirty=1;
}

// Pasted text goes to the application or else, up to the first line end, to a field.
void TvPasteEvent()
{
	tvDirty=1;
	if((menuOpen>=0)||(comboCtl>=0))return;
	int top=TvTop();
	if(top<0)return;
	if(TvAppPaste(top,tvPaste,tvPasteN))return;
	int f=tvWin[top].focus;
	if(f<0)return;
	if(tvCtl[f].type!=C_INPUT)return;
	int n=0;
	while(n<tvPasteN)
	{
		int ch=tvPaste[n];
		if((ch==10)||(ch==13))break;
		n++;
	}
	TvInputInsert(tvCtl[f],tvPaste,n);
}

// Runs until the program quits or, with stopWin>=0, until that modal window has a result.
int TvMessage(char* title, char* text, int askYesNo);

// A dialog that stays open without TvExec is a window like the others: it gets a
// number, or is closed when TV_MAXNUM windows are open already.
void TvNumberWindows()
{
	for(int i=0;i<TV_MAXWIN;i++)
		if(tvWin[i].used&&!tvWin[i].modal&&(tvWin[i].num==0))
		{
			int n=TvFreeNum(i);
			if((n==0)||(TvOpenCount()>TV_MAXNUM))
			{
				TvClose(i);
				TvMessage("Windows","Too many windows are open.",0);
			}
			else
			{
				tvWin[i].num=n;
				tvDirty=1;
			}
		}
}

int TvRun(int stopWin)
{
	int behind=0;
	while(tvRunning)
	{
		TvNumberWindows();
		if(stopWin>=0)
			if(tvWin[stopWin].result!=0)
				return tvWin[stopWin].result;
		// while keys are queued up, catch up on them first and redraw now and then
		if(tvDirty&&!tvScreenTaken)
		{
			behind++;
			int pending=0;
#ifdef TV_GFX
			pending=GfxPending();
#endif
#ifndef TV_GFX
			pending=term_pending();
#endif
			if((behind>=16)||!pending)
			{
				TvRedraw();
				behind=0;
			}
		}
		int wait=200;
		if(tvTickMs>0)
		{
			int left=tvNextTick-term_ms();
			if(left<0)left=0;
			if(left<wait)wait=left;
		}
		TvReadEvent(wait);
		if((evType!=EV_NONE)&&(evType!=EV_MUP)&&(evType!=EV_MDRAG))
		{
			tvLastInput=term_ms();
			if(tvScreenTaken)
			{
				tvScreenTaken=0;
				tvDirty=1;
				evType=EV_NONE;
			}
		}
		if(evType==EV_NONE)
		{
			int sec=term_time(2);
			if(sec!=tvClockSec)
			{
				tvClockSec=sec;
				tvDirty=1;
			}
			TvCheckSize();
		}
		else if(evType==EV_KEY)
			TvKey();
		else if(evType==EV_PASTE)
			TvPasteEvent();
		else
			TvMouse();
		if(tvTickMs>0)
		{
			int now=term_ms();
			if(now>=tvNextTick)
			{
				tvNextTick=now+tvTickMs;
				TvAppTick();
			}
		}
	}
	return 0;
}

// Runs a window as a modal dialog and returns the command that ended it.
// The window stays open so the caller can read its controls, then TvClose it.
int TvExec(int win)
{
	tvWin[win].modal=1;
	tvWin[win].num=0;
	tvWin[win].result=0;
	TvToFront(win);
	return TvRun(win);
}

// Window > List: every window, front first, with its number and a * for unsaved
// changes. Switch brings one to the front; Close closes it as its close box would.
void TvWinList()
{
	int win=TvNewCentered(W_WINLIST,"Windows",56,18,WF_DIALOG);
	if(win<0)return;
	TvAdd(win,C_LABEL,3,2,10,1,"~W~indows",0);
	int lst=TvAdd(win,C_LIST,3,3,34,13,"",0);
	tvCtl[lst].virt=1;
	int b=TvAdd(win,C_BUTTON,40,3,13,2,"~S~witch",CM_OK);
	tvCtl[b].isDefault=1;
	TvAdd(win,C_BUTTON,40,6,13,2,"~C~lose",CM_CLOSE);
	TvAdd(win,C_BUTTON,40,9,13,2,"Cancel",CM_CANCEL);
	tvWin[win].focus=lst;
	while(tvRunning)
	{
		TvWinListFill(win);
		tvCtl[lst].count=tvWlN;
		if(tvCtl[lst].val>=tvWlN)tvCtl[lst].val=tvWlN-1;
		if(tvCtl[lst].val<0)tvCtl[lst].val=0;
		int res=TvExec(win);
		int pick=-1;
		if(tvWlN>0)pick=tvWlWin[tvCtl[lst].val];
		if((res==CM_CLOSE)&&(pick>=0))
		{
			tvWin[win].focus=lst;
			if(TvAppCanClose(pick))TvClose(pick);
			TvToFront(win);
		}
		else
		{
			TvClose(win);
			if((res==CM_OK)&&(pick>=0))TvToFront(pick);
			return;
		}
	}
	TvClose(win);
}

int TvLineCount(char* s)
{
	int n=1;
	int i=0;
	while(s[i]!=0)
	{
		if(s[i]==10)n++;
		i++;
	}
	return n;
}

// Shows a message with a row of buttons, given as "~Y~es|~N~o", at row y or centred if
// y<0. Returns the number of the button pressed, or -1 for Esc or the close box.
int TvChooseAt(char* title, char* text, char* labels, int y)
{
	int lines=TvLineCount(text);
	int longest=0;
	int cur=0;
	int i=0;
	while(text[i]!=0)
	{
		if(text[i]==10)
		{
			if(cur>longest)longest=cur;
			cur=0;
		}
		else
		{
			// count characters, not bytes
			int c=text[i];
			if((c&0xC0)!=0x80)cur++;
		}
		i++;
	}
	if(cur>longest)longest=cur;
	int count=TvItemCount(labels);
	char lbl[32];
	int bw=4;
	for(int k=0;k<count;k++)
	{
		TvItemAt(labels,k,&lbl[0],32);
		int l=HotLen(&lbl[0])+4;
		if(l>bw)bw=l;
	}
	int rowW=count*(bw+2)-1;
	int w=longest+6;
	if(w<rowW+4)w=rowW+4;
	if(w<30)w=30;
	if(w>tvW)w=tvW;
	int h=lines+6;
	int win=TvNewCentered(0,title,w,h,WF_DIALOG);
	if(win<0)return -1;
	if(y>=0)tvWin[win].y=y;
	// one label per line
	char buf[200];
	int line=0;
	int o=0;
	i=0;
	while(true)
	{
		int c=text[i];
		if((c==0)||(c==10))
		{
			buf[o]=0;
			int l=TvCols(&buf[0]);
			TvAdd(win,C_LABEL,(w-l)/2,2+line,l,1,&buf[0],0);
			line++;
			o=0;
			if(c==0)break;
		}
		else if(o<198)
		{
			buf[o]=c;
			o++;
		}
		i++;
	}
	int bx=(w-rowW)/2;
	for(int k=0;k<count;k++)
	{
		TvItemAt(labels,k,&lbl[0],32);
		int b=TvAdd(win,C_BUTTON,bx+k*(bw+2),h-3,bw,2,&lbl[0],CM_USER+k);
		if(k==0)tvCtl[b].isDefault=1;
	}
	int r=TvExec(win);
	TvClose(win);
	if(r>=CM_USER)return r-CM_USER;
	return -1;
}

int TvChoose(char* title, char* text, char* labels)
{
	return TvChooseAt(title,text,labels,-1);
}

// Shows a message with one or two buttons. Returns CM_OK, CM_YES, CM_NO or CM_CANCEL.
int TvMessage(char* title, char* text, int askYesNo)
{
	if(askYesNo)
	{
		int r=TvChoose(title,text,"~Y~es|~N~o");
		if(r==0)return CM_YES;
		if(r==1)return CM_NO;
		return CM_CANCEL;
	}
	if(TvChoose(title,text,"O~K~")==0)return CM_OK;
	return CM_CANCEL;
}

void TvInit()
{
	tvVga[0]=0;
	tvVga[1]=4;
	tvVga[2]=2;
	tvVga[3]=6;
	tvVga[4]=1;
	tvVga[5]=5;
	tvVga[6]=3;
	tvVga[7]=7;
	for(int i=0;i<TV_MAXWIN;i++)
	{
		tvWin[i].used=0;
		tvWinDirty[i]=0;
	}
	for(int i=0;i<TV_MAXCTL;i++)
		tvCtl[i].used=0;
	for(int i=0;i<TV_CELLS;i++)
	{
		tvCh[i]=' ';
		tvAt[i]=0;
		tvPrevCh[i]=-1;
		tvPrevAt[i]=-1;
	}
	tvPaste=malloc(TV_PASTEMAX+1);
	TvSetTheme(TH_CLASSIC);
	TvInitRgb256();
	tvLastInput=term_ms();
#ifdef TV_GFX
	TvNeedDisplay();
	GfxInit();
	tvW=GfxCols();
	tvH=GfxRows();
	return;
#endif
	tvTrueColor=term_truecolor();
	tvW=term_cols();
	tvH=term_rows();
	if(tvW>TV_MAXW)tvW=TV_MAXW;
	if(tvH>TV_MAXH)tvH=TV_MAXH;
	// alternate screen, no autowrap, hidden cursor, mouse buttons and drags in SGR form, bracketed paste
	OutStr("\x1b[?1049h\x1b[?7l\x1b[?25l\x1b[?1000h\x1b[?1002h\x1b[?1006h\x1b[?2004h\x1b[0m\x1b[2J");
	OutFlush();
}

void TvDone()
{
#ifdef TV_GFX
	return;
#endif
	OutStr("\x1b[?2004l\x1b[?1006l\x1b[?1002l\x1b[?1000l\x1b[?7h\x1b[?25h\x1b[0m\x1b[?1049l");
	OutFlush();
	term_close();
}

// Gives the terminal back for a while, as TvDone does, so that another program can
// use it; TvResume takes it again and draws everything anew. Both do nothing in a
// graphic window.
void TvSuspend()
{
	TvDone();
}

void TvResume()
{
#ifdef TV_GFX
	return;
#endif
	term_open();
	OutStr("\x1b[?1049h\x1b[?7l\x1b[?25l\x1b[?1000h\x1b[?1002h\x1b[?1006h\x1b[?2004h\x1b[0m\x1b[2J");
	OutFlush();
	for(int i=0;i<TV_CELLS;i++)
	{
		tvPrevCh[i]=-1;
		tvPrevAt[i]=-1;
	}
	tvCurShown=0;
	tvDirty=1;
	TvCheckSize();
}

#endif // TV_H
