// A Turbo Vision style desktop in the terminal, written in Scinc and drawn with
// ANSI escape sequences: a file browser and text editor, a calculator, a calendar, an
// ASCII table, a sliding puzzle, the Circuitfall and Sonar games, a Mandelbrot explorer,
// the Game of Life, a hex viewer, an ANSI paint program, a screen saver, colour themes
// and dialogs with the standard controls.
//
//   ./build_so.sh                  once, builds the terminal input library
//   Scinc tvdemo.cpp [file]        from this folder
//
// tvdemo_gfx.cpp builds the same desktop drawn in a graphic window (tvgfx.h) with the
// stroke font, tvdemo_raster.cpp and tvdemo_small.cpp with the interpreter's quicker
// 9x16 and 6x12 bitmap fonts. tvdemo_gl.cpp and tvdemo_gl_small.cpp draw the same
// two fonts with OpenGL (tvgl.h) from a glyph atlas, in a resizable, zoomable window.
#include <stdio.h>
// tvdemo writes every hook of tv.h itself
#define TVAPP_DRAW
#define TVAPP_KEY
#define TVAPP_MOUSE
#define TVAPP_COMMAND
#define TVAPP_BUTTON
#define TVAPP_CLOSED
#define TVAPP_LISTITEM
#define TVAPP_CANCLOSE
#define TVAPP_PASTE
#define TVAPP_TICK
#define TV_TERM_HINT "Scinc tvdemo.cpp"
#include "tv.h"
#include "tvapp.h"
#include "tvedit.h"
#include "circuitfall.h"
#include "tvtoys.h"
#include "tvapps.h"

#pragma STACK_SIZE 655360

typedef char* pchar;

#define W_CALC 2
#define W_CAL 3
#define W_ASCII 4
#define W_PUZZLE 5
#define W_CONTROLS 6
#define W_FILES 7

#define CM_QUIT 100
#define CM_OPEN 101
#define CM_DOCALC 102
#define CM_CALENDAR 103
#define CM_ASCII 104
#define CM_PUZZLE 105
#define CM_CONTROLS 106
#define CM_ABOUT 107
#define CM_HELP 108
#define CM_DIRUP 109
#define CM_DIRHOME 110
#define CM_CIRCUIT 111
#define CM_CIRCUIT2 112
#define CM_PREFS 113
#define CM_PROGRESS 114
#define CM_MANDEL 115
#define CM_LIFE 116
#define CM_SAVERNOW 117
#define CM_SONAR 118
#define CM_HEX 119
#define CM_PAINT 120
#define CM_LINENUMS 121
#define CM_WRAP 122
#define CM_SAVERWIN 124
#define CM_THEMEED 125
#define CM_SVNOW 400    // + a saver kind, SV_RANDOM the last
#define CM_SVWIN 420
#define CM_PIX 440      // + 1 to 4: the pixel size of a graphic window
#define CM_CALCKEY 200
#define CM_THEMEPICK 300
#define CM_VOLUME 301
#define CM_DESKPICK 302

#define W_PREFS 8
#define W_PROGRESS 9

// ---------------------------------------------------------------- calculator

char calcDisp[32];
float calcAcc=0.;
int calcOp=0;
int calcFresh=1;
int calcError=0;

void CalcShow(float v)
{
	if((v>=1000000000000.)||(v<=-1000000000000.))
	{
		calcError=1;
		StrCpy(&calcDisp[0],"Error",32);
		return;
	}
	// fixed notation, since CalcValue reads the display back and knows no exponents
	snprintf(&calcDisp[0],32,"%.10f",v);
	int n=StrLen(&calcDisp[0]);
	while(calcDisp[n-1]=='0')
	{
		n--;
		calcDisp[n]=0;
	}
	if(calcDisp[n-1]=='.')calcDisp[n-1]=0;
}

float CalcValue()
{
	float v=0.;
	float sign=1.;
	float scale=0.;
	int i=0;
	if(calcDisp[0]=='-')
	{
		sign=-1.;
		i=1;
	}
	while(calcDisp[i]!=0)
	{
		int c=calcDisp[i];
		if(c=='.')
			scale=1.;
		else
		{
			v=v*10.+(c-'0');
			scale=scale*10.;
		}
		i++;
	}
	if(scale>0.)v=v/scale;
	return v*sign;
}

void CalcApply()
{
	float v=CalcValue();
	float r=v;
	if(calcOp=='+')r=calcAcc+v;
	if(calcOp=='-')r=calcAcc-v;
	if(calcOp=='*')r=calcAcc*v;
	if(calcOp=='/')
	{
		if(v==0.)
		{
			calcError=1;
			StrCpy(&calcDisp[0],"Error",32);
			return;
		}
		r=calcAcc/v;
	}
	calcAcc=r;
	CalcShow(r);
}

void CalcReset()
{
	StrCpy(&calcDisp[0],"0",32);
	calcAcc=0.;
	calcOp=0;
	calcFresh=1;
	calcError=0;
}

void CalcKey(int k)
{
	if(k=='C')
	{
		CalcReset();
		return;
	}
	if(calcError)return;
	if(((k>='0')&&(k<='9'))||(k=='.'))
	{
		if(calcFresh)
		{
			StrCpy(&calcDisp[0],"0",32);
			calcFresh=0;
		}
		int n=StrLen(&calcDisp[0]);
		if(k=='.')
		{
			for(int i=0;i<n;i++)
				if(calcDisp[i]=='.')return;
		}
		if(n>=14)return;
		if((n==1)&&(calcDisp[0]=='0')&&(k!='.'))n=0;
		calcDisp[n]=k;
		calcDisp[n+1]=0;
	}
	else if(k=='<')
	{
		if(calcFresh)return;
		int n=StrLen(&calcDisp[0]);
		n--;
		if((n<=0)||((n==1)&&(calcDisp[0]=='-')))
			StrCpy(&calcDisp[0],"0",32);
		else
			calcDisp[n]=0;
	}
	else if(k=='~')
	{
		CalcShow(-CalcValue());
	}
	else if(k=='%')
	{
		CalcShow(calcAcc*CalcValue()/100.);
		calcFresh=1;
	}
	else
	{
		// + - * / =
		if(!calcFresh)
			CalcApply();
		else if(calcOp==0)
			calcAcc=CalcValue();
		if(k=='=')
			calcOp=0;
		else
			calcOp=k;
		calcFresh=1;
	}
}

int OpenCalculator()
{
	int win=TvNewCentered(W_CALC,"Calculator",26,16,WF_DIALOG);
	if(win<0)return -1;
	CalcReset();
	char* labels="C|<|%|+/-|7|8|9|/|4|5|6|*|1|2|3|-|0|.|=|+";
	char lbl[8];
	for(int i=0;i<20;i++)
	{
		TvItemAt(labels,i,&lbl[0],8);
		int bx=2+(i%4)*6;
		int by=4+(i/4)*2;
		TvAdd(win,C_BUTTON,bx,by,4,2,&lbl[0],CM_CALCKEY+i);
	}
	return win;
}

void DrawCalc(int win)
{
	TWin& t=tvWin[win];
	TvFill(t.x+2,t.y+2,22,1,' ',0x1F);
	int n=StrLen(&calcDisp[0]);
	TvText(t.x+23-n,t.y+2,&calcDisp[0],0x1F);
}

// The key for a button number, from the label order above.
int CalcKeyOf(int i)
{
	char* keys="C<%~789/456*123-0.=+";
	int k=keys[i];
	return k;
}

int CalcFromKeyboard(int key)
{
	if((key=='c')||(key=='C'))
	{
		CalcKey('C');
		return 1;
	}
	if(key==K_BACKSPACE)
	{
		CalcKey('<');
		return 1;
	}
	if(key==K_ENTER)
	{
		CalcKey('=');
		return 1;
	}
	for(int i=0;i<20;i++)
	{
		int k=CalcKeyOf(i);
		if((k==key)&&(k!='~'))
		{
			CalcKey(k);
			return 1;
		}
	}
	return 0;
}

// ---------------------------------------------------------------- calendar

// Window fields: a year, b month.

int DaysIn(int y, int m)
{
	if(m==2)
	{
		if((y%400==0)||((y%4==0)&&(y%100!=0)))return 29;
		return 28;
	}
	if((m==4)||(m==6)||(m==9)||(m==11))return 30;
	return 31;
}

// 0 Sunday .. 6 Saturday, Zeller's congruence
int WeekDay(int y, int m, int d)
{
	if(m<3)
	{
		m=m+12;
		y--;
	}
	int k=y%100;
	int j=y/100;
	int h=(d+13*(m+1)/5+k+k/4+j/4+5*j)%7;
	return (h+6)%7;
}

void DrawCalendar(int win)
{
	TWin& t=tvWin[win];
	char* names="January|February|March|April|May|June|July|August|September|October|November|December";
	char head[32];
	char mon[16];
	TvItemAt(names,t.b-1,&mon[0],16);
	snprintf(&head[0],32,"%s %i",&mon[0],t.a);
	TvText(t.x+(t.w-StrLen(&head[0]))/2,t.y+1,&head[0],A_DLG_FRAME);
	TvPut(t.x+2,t.y+1,G_LEFT,0x71);
	TvPut(t.x+t.w-3,t.y+1,G_RIGHT,0x71);
	TvText(t.x+2,t.y+2,"Su Mo Tu We Th Fr Sa",A_DLG);
	int first=WeekDay(t.a,t.b,1);
	int n=DaysIn(t.a,t.b);
	int today=0;
	if((term_time(3)==t.a)&&(term_time(4)==t.b))
		today=term_time(5);
	char num[4];
	for(int d=1;d<=n;d++)
	{
		int cell=first+d-1;
		int cx=t.x+2+(cell%7)*3;
		int cy=t.y+3+cell/7;
		snprintf(&num[0],4,"%2i",d);
		int at=A_DLG;
		if(d==today)at=0x1E;
		TvText(cx,cy,&num[0],at);
	}
}

void CalShift(TWin& t, int dir)
{
	t.b=t.b+dir;
	if(t.b<1)
	{
		t.b=12;
		t.a--;
	}
	if(t.b>12)
	{
		t.b=1;
		t.a++;
	}
}

// ---------------------------------------------------------------- ASCII table

// Window field: a selected code.

// codes 1..31 and 127 show the symbols of the IBM PC character set, as DOS did
int AsciiLow(int c)
{
	if(c==1)return 0x263A;
	if(c==2)return 0x263B;
	if(c==3)return 0x2665;
	if(c==4)return 0x2666;
	if(c==5)return 0x2663;
	if(c==6)return 0x2660;
	if(c==7)return 0x2022;
	if(c==8)return 0x25D8;
	if(c==9)return 0x25CB;
	if(c==10)return 0x25D9;
	if(c==11)return 0x2642;
	if(c==12)return 0x2640;
	if(c==13)return 0x266A;
	if(c==14)return 0x266B;
	if(c==15)return 0x263C;
	if(c==16)return 0x25BA;
	if(c==17)return 0x25C4;
	if(c==18)return 0x2195;
	if(c==19)return 0x203C;
	if(c==20)return 0xB6;
	if(c==21)return 0xA7;
	if(c==22)return 0x25AC;
	if(c==23)return 0x21A8;
	if(c==24)return 0x2191;
	if(c==25)return 0x2193;
	if(c==26)return 0x2192;
	if(c==27)return 0x2190;
	if(c==28)return 0x221F;
	if(c==29)return 0x2194;
	if(c==30)return 0x25B2;
	if(c==31)return 0x25BC;
	return ' ';
}

int AsciiGlyph(int c)
{
	if(c<32)return AsciiLow(c);
	if(c==127)return 0x2302;
	if((c>=128)&&(c<160))return 0xB7;
	return c;
}

void DrawAscii(int win)
{
	TWin& t=tvWin[win];
	for(int c=0;c<256;c++)
	{
		int at=A_DLG;
		if(c==t.a)at=0x2F;
		TvPut(t.x+1+c%32,t.y+1+c/32,AsciiGlyph(c),at);
	}
	char s[40];
	snprintf(&s[0],40," Char:   Dec: %3i  Hex: %02X ",t.a,t.a);
	TvText(t.x+1,t.y+9,&s[0],A_DLG);
	TvPut(t.x+8,t.y+9,AsciiGlyph(t.a),A_DLG_FRAME);
}

int AsciiKey(TWin& t, int key)
{
	int v=t.a;
	if(key==K_LEFT)v--;
	else if(key==K_RIGHT)v++;
	else if(key==K_UP)v=v-32;
	else if(key==K_DOWN)v=v+32;
	else if(key==K_HOME)v=0;
	else if(key==K_END)v=255;
	else return 0;
	if((v>=0)&&(v<256))t.a=v;
	return 1;
}

// ---------------------------------------------------------------- sliding puzzle

int board[16];
int boardMoves=0;
int boardRng=1;

int NextRandom()
{
	boardRng=boardRng*1103515245+12345;
	return (boardRng>>16)&0x7fff;
}

int BoardSolved()
{
	for(int i=0;i<15;i++)
		if(board[i]!=i+1)return 0;
	return 1;
}

// Moves the tile at position p into the gap if they are neighbours.
int BoardMove(int p)
{
	if((p<0)||(p>=16))return 0;
	int gap=0;
	for(int i=0;i<16;i++)
		if(board[i]==0)gap=i;
	int dx=(p%4)-(gap%4);
	int dy=(p/4)-(gap/4);
	if(dx<0)dx=-dx;
	if(dy<0)dy=-dy;
	if(dx+dy!=1)return 0;
	board[gap]=board[p];
	board[p]=0;
	return 1;
}

void BoardShuffle()
{
	boardRng=term_time(1)*60+term_time(2)+1;
	for(int i=0;i<16;i++)
		board[i]=(i+1)%16;
	int gap=15;
	for(int i=0;i<400;i++)
	{
		int d=NextRandom()%4;
		int p=-1;
		if((d==0)&&(gap%4>0))p=gap-1;
		if((d==1)&&(gap%4<3))p=gap+1;
		if((d==2)&&(gap>=4))p=gap-4;
		if((d==3)&&(gap<12))p=gap+4;
		if(p>=0)
		{
			BoardMove(p);
			gap=p;
		}
	}
	boardMoves=0;
}

void DrawPuzzle(int win)
{
	TWin& t=tvWin[win];
	char s[8];
	for(int i=0;i<16;i++)
	{
		int px=t.x+2+(i%4)*5;
		int py=t.y+1+(i/4)*2;
		if(board[i]!=0)
		{
			int at=0x1F;
			if((board[i]-1)%8<4)
			{
				if((board[i]-1)%2==1)at=0x2F;
			}
			else
			{
				if((board[i]-1)%2==0)at=0x2F;
			}
			TvFill(px,py,4,2,' ',at);
			snprintf(&s[0],8,"%2i",board[i]);
			TvText(px+1,py,&s[0],at);
		}
	}
	snprintf(&s[0],8,"%i",boardMoves);
	TvText(t.x+2,t.y+9,"Moves:",A_DLG);
	TvText(t.x+9,t.y+9,&s[0],A_DLG);
	if(BoardSolved())
		TvText(t.x+13,t.y+9,"Solved!",0x7A);
}

int PuzzleKey(int key)
{
	int gap=0;
	for(int i=0;i<16;i++)
		if(board[i]==0)gap=i;
	int p=-1;
	if((key==K_LEFT)&&(gap%4<3))p=gap+1;
	if((key==K_RIGHT)&&(gap%4>0))p=gap-1;
	if((key==K_UP)&&(gap<12))p=gap+4;
	if((key==K_DOWN)&&(gap>=4))p=gap-4;
	if(p<0)
	{
		if((key==K_LEFT)||(key==K_RIGHT)||(key==K_UP)||(key==K_DOWN))return 1;
		return 0;
	}
	if(BoardMove(p))boardMoves++;
	return 1;
}

// ---------------------------------------------------------------- controls dialog

char ctlName[64];
int ctlOptA=1;
int ctlOptB=0;
int ctlOptC=1;
int ctlMode=1;
int ctlColor=2;
int ctlVolume=12;
int ctlSize=1;
int ctlVolCtl=-1;   // the volume slider of the open Controls dialog
int ctlVolBar=-1;   // and the bar that follows it

void RunControls()
{
	int win=TvNewCentered(W_CONTROLS,"Controls",58,23,WF_DIALOG);
	if(win<0)return;
	TvAdd(win,C_LABEL,3,2,10,1,"~N~ame:",0);
	int inp=TvAdd(win,C_INPUT,10,2,44,1,&ctlName[0],0);
	tvCtl[inp].val=StrLen(&ctlName[0]);

	TvAdd(win,C_LABEL,3,4,10,1,"Options",0);
	int a=TvAdd(win,C_CHECK,3,5,20,1,"~S~ound",0);
	int b=TvAdd(win,C_CHECK,3,6,20,1,"~M~ouse trails",0);
	int c=TvAdd(win,C_CHECK,3,7,20,1,"~A~uto save",0);
	tvCtl[a].val=ctlOptA;
	tvCtl[b].val=ctlOptB;
	tvCtl[c].val=ctlOptC;

	TvAdd(win,C_LABEL,3,9,10,1,"Mode",0);
	int r[3];
	r[0]=TvAdd(win,C_RADIO,3,10,20,1,"~E~asy",0);
	r[1]=TvAdd(win,C_RADIO,3,11,20,1,"No~r~mal",0);
	r[2]=TvAdd(win,C_RADIO,3,12,20,1,"~H~ard",0);
	for(int i=0;i<3;i++)
		tvCtl[r[i]].val=(ctlMode==i);

	TvAdd(win,C_LABEL,27,4,10,1,"~C~olour",0);
	int lst=TvAdd(win,C_LIST,27,5,27,8,"Black|Blue|Green|Cyan|Red|Magenta|Brown|Light gray|Dark gray|Light blue|Light green|Light cyan|Light red|Yellow|White",0);
	tvCtl[lst].val=ctlColor;

	TvAdd(win,C_LABEL,3,14,10,1,"~V~olume",0);
	ctlVolCtl=TvAdd(win,C_SLIDER,12,14,21,1,"",CM_VOLUME);
	tvCtl[ctlVolCtl].count=20;
	tvCtl[ctlVolCtl].val=ctlVolume;
	ctlVolBar=TvAdd(win,C_PROGRESS,12,15,21,1,"",0);
	tvCtl[ctlVolBar].count=20;
	tvCtl[ctlVolBar].val=ctlVolume;
	TvAdd(win,C_LABEL,38,14,10,1,"Si~z~e",0);
	int sz=TvAdd(win,C_COMBO,44,14,10,1,"Small|Medium|Large|Huge",0);
	tvCtl[sz].val=ctlSize;

	int ok=TvAdd(win,C_BUTTON,16,19,12,2,"O~K~",CM_OK);
	tvCtl[ok].isDefault=1;
	TvAdd(win,C_BUTTON,31,19,12,2,"Cancel",CM_CANCEL);
	tvWin[win].focus=inp;

	int res=TvExec(win);
	if(res==CM_OK)
	{
		StrCpy(&ctlName[0],&tvCtl[inp].text[0],64);
		ctlOptA=tvCtl[a].val;
		ctlOptB=tvCtl[b].val;
		ctlOptC=tvCtl[c].val;
		for(int i=0;i<3;i++)
			if(tvCtl[r[i]].val)ctlMode=i;
		ctlColor=tvCtl[lst].val;
		ctlVolume=tvCtl[ctlVolCtl].val;
		ctlSize=tvCtl[sz].val;
		char colour[24];
		TvItemAt(&tvCtl[lst].text[0],ctlColor,&colour[0],24);
		char size[16];
		TvItemAt(&tvCtl[sz].text[0],ctlSize,&size[0],16);
		char msg[200];
		snprintf(&msg[0],200,"Name: %s\nColour: %s\nOptions: %i %i %i, mode %i\nVolume %i, size %s",&ctlName[0],&colour[0],ctlOptA,ctlOptB,ctlOptC,ctlMode,ctlVolume,&size[0]);
		TvClose(win);
		TvMessage("You chose",&msg[0],0);
	}
	else
		TvClose(win);
	ctlVolCtl=-1;
}

void DrawControls(int win)
{
	TWin& t=tvWin[win];
	char s[8];
	snprintf(&s[0],8,"%2i",tvCtl[ctlVolCtl].val);
	TvText(t.x+34,t.y+14,&s[0],A_DLG);
}

// ---------------------------------------------------------------- preferences

#define CFG_FILE "tvdemo.cfg"
int prefTheme=0;
int prefScale=0;     // the pixel size of a graphic window, 0 the program's own
int prefsThemeCtl=-1; // the theme combo box of the open Preferences

// The settings file holds: theme, saver on, saver minutes, saver kind, line numbers,
// wrap, the saver numbering (3 has Random at 7, 2 had it at 6, none at 4), and the
// pixel size of a graphic window, and the desktop pattern.
void PrefsLoad()
{
	char buf[64];
	int n=fs_load(CFG_FILE,&buf[0],63);
	if(n<=0)return;
	buf[n]=0;
	int v[9];
	int k=0;
	int i=0;
	while((k<9)&&(buf[i]!=0))
	{
		if((buf[i]>='0')&&(buf[i]<='9'))
		{
			int x=0;
			while((buf[i]>='0')&&(buf[i]<='9'))
			{
				x=x*10+buf[i]-'0';
				i++;
			}
			v[k]=x;
			k++;
		}
		else i++;
	}
	if(k>=1)
		prefTheme=v[0];
	if(k>=4)
	{
		svOn=(v[1]!=0);
		if((v[2]>=1)&&(v[2]<=30))svMinutes=v[2];
		if(v[3]<=SV_RANDOM)svKind=v[3];
		if((k<7)&&(v[3]==4))svKind=SV_RANDOM;
		if((k>=7)&&(v[6]==2)&&(v[3]==6))svKind=SV_RANDOM;
		if((k>=7)&&(v[6]==3)&&(v[3]==7))svKind=SV_RANDOM;
		if((k>=7)&&(v[6]==4)&&(v[3]==8))svKind=SV_RANDOM;
		if((k>=7)&&(v[6]==5)&&(v[3]==9))svKind=SV_RANDOM;
	}
	if(k>=6)
	{
		edLineNums=(v[4]!=0);
		edWrap=(v[5]!=0);
	}
	if((k>=8)&&(v[7]<=4))prefScale=v[7];
	if((k>=9)&&(v[8]<TE_DESKN))teDeskPick=v[8];
}

void PrefsSave()
{
	char buf[64];
#ifdef TV_GFX
	prefScale=GfxScale();
#endif
	snprintf(&buf[0],64,"%i %i %i %i %i %i 6 %i %i\n",prefTheme,svOn,svMinutes,svKind,edLineNums,edWrap,prefScale,teDeskPick);
	if(fs_save_begin(CFG_FILE))
	{
		int ok=fs_save_write(&buf[0],StrLen(&buf[0]));
		fs_save_end(ok);
	}
}

int prefMinCtl=-1;   // the saver delay slider of the open dialog

void RunPrefs()
{
	int win=TvNewCentered(W_PREFS,"Preferences",48,15,WF_DIALOG);
	if(win<0)return;
	TvAdd(win,C_LABEL,3,2,10,1,"~T~heme",0);
	char names[200];
	TeThemeNames(&names[0],200);
	int th=TvAdd(win,C_COMBO,14,2,30,1,&names[0],CM_THEMEPICK);
	tvCtl[th].val=prefTheme;
	prefsThemeCtl=th;
	TvAdd(win,C_LABEL,3,3,10,1,"~D~esktop",0);
	int dk=TvAdd(win,C_COMBO,14,3,30,1,teDeskNames,CM_DESKPICK);
	tvCtl[dk].val=teDeskPick;
	int oldDesk=teDeskPick;
	int on=TvAdd(win,C_CHECK,2,5,30,1,"~S~creen saver",0);
	tvCtl[on].val=svOn;
	TvAdd(win,C_LABEL,3,6,10,1,"~K~ind",0);
	int kind=TvAdd(win,C_COMBO,14,6,30,1,"Stars|Plasma|Matrix|Flame|Reaction-diffusion|Fluid|Landscape|Labyrinth|Objects|Nyan cat|Random",0);
	tvCtl[kind].val=svKind;
	TvAdd(win,C_LABEL,3,8,10,1,"~A~fter",0);
	prefMinCtl=TvAdd(win,C_SLIDER,14,8,20,1,"",0);
	tvCtl[prefMinCtl].count=29;
	tvCtl[prefMinCtl].val=svMinutes-1;
	int ok=TvAdd(win,C_BUTTON,10,11,12,2,"O~K~",CM_OK);
	tvCtl[ok].isDefault=1;
	TvAdd(win,C_BUTTON,25,11,12,2,"Cancel",CM_CANCEL);
	tvWin[win].focus=th;
	int res=TvExec(win);
	if(res==CM_OK)
	{
		prefTheme=tvCtl[th].val;
		svOn=tvCtl[on].val;
		svKind=tvCtl[kind].val;
		svMinutes=tvCtl[prefMinCtl].val+1;
		teDeskPick=tvCtl[dk].val;
		PrefsSave();
	}
	else teDeskPick=oldDesk;
	prefMinCtl=-1;
	prefsThemeCtl=-1;
	TvClose(win);
	// the combo box showed each theme as it was picked
	TeUseTheme(prefTheme);
}

void DrawPrefs(int win)
{
	TWin& t=tvWin[win];
	char s[16];
	snprintf(&s[0],16,"%i min",tvCtl[prefMinCtl].val+1);
	TvText(t.x+36,t.y+8,&s[0],A_DLG);
}

// ---------------------------------------------------------------- progress bar demo

int progWin=-1;
int progBar=-1;

// The timer runs fast while something moves, slowly while the saver waits for idle time.
void TickUpdate()
{
	int ms=0;
	if(svOn)ms=500;
	if((cfWin>=0)||(progWin>=0)||(mbWin>=0)||(lfWin>=0)||(snWin>=0)||tvScreenTaken)ms=30;
	if(SvWindows()>0)ms=30;
	if(ms!=tvTickMs)
	{
		tvTickMs=ms;
		tvNextTick=term_ms()+ms;
	}
}

void RunProgress()
{
	int win=TvNewCentered(W_PROGRESS,"Copying",46,9,WF_DIALOG);
	if(win<0)return;
	TvAdd(win,C_LABEL,3,2,30,1,"Copying 120 imaginary files...",0);
	progBar=TvAdd(win,C_PROGRESS,3,3,40,1,"",0);
	tvCtl[progBar].count=120;
	TvAdd(win,C_BUTTON,17,5,12,2,"Cancel",CM_CANCEL);
	progWin=win;
	TickUpdate();
	int res=TvExec(win);
	progWin=-1;
	TvClose(win);
	if(res==CM_OK)TvMessage("Copying","All 120 files were copied.",0);
}

void ProgressTick()
{
	if(progWin<0)return;
	TCtl& c=tvCtl[progBar];
	c.val++;
	tvDirty=1;
	if(c.val>=c.count)tvWin[progWin].result=CM_OK;
}

// ---------------------------------------------------------------- editor options

int itemLineNums=-1;  // the menu items whose hints show on or off
int itemWrap=-1;
int itemPix=-1;       // the first of the 1x..4x items of Options > Pixel size
int pixShown=0;       // the size they show

// A dot by the pixel size in use: Ctrl+= and Ctrl+- change it too, so each
// window drawn looks whether it is still the one shown.
void ShowPixSize()
{
#ifdef TV_GFX
	if(itemPix<0)return;
	pixShown=GfxScale();
	for(int k=0;k<4;k++)
	{
		tvItem[itemPix+k].hint[0]=0;
		if(k+1==pixShown)StrCpy(&tvItem[itemPix+k].hint[0],"\xe2\x80\xa2",12);
	}
#endif
}

void ShowToggles()
{
	char* on="on";
	char* off="off";
	ShowPixSize();
	if(edLineNums)StrCpy(&tvItem[itemLineNums].hint[0],on,12);
	else StrCpy(&tvItem[itemLineNums].hint[0],off,12);
	if(edWrap)StrCpy(&tvItem[itemWrap].hint[0],on,12);
	else StrCpy(&tvItem[itemWrap].hint[0],off,12);
	// the cursor stays in view in every editor window
	for(int i=0;i<TV_MAXWIN;i++)
		if(tvWin[i].used)
			if(tvWin[i].type==W_VIEW)
			{
				tvWin[i].e=0;
				EdReveal(i);
			}
	tvDirty=1;
}

// ---------------------------------------------------------------- context menus

int popEdit=-1;
int popWin=-1;

int ContextMenu(int win, int x, int y)
{
	TWin& t=tvWin[win];
	int menu=popWin;
	if(t.type==W_VIEW)menu=popEdit;
	if(x<0)TvOpenPopup(menu,-1,-1);
	else TvOpenPopup(menu,t.x+x,t.y+y);
	return 1;
}

// ---------------------------------------------------------------- file browser

#define FD_PATH ED_PATH

char fdDir[FD_PATH];
int fdList=-1;
int fdHiddenBox=-1;
int fdHidden=0;
int fdNameBox=-1;   // the file name field when saving, otherwise -1
int fdLastSel=-1;

// Lists a directory and selects selectName if it is there. Returns 0 on failure,
// leaving the current directory and listing as they were.
int FdLoad(char* path, char* selectName)
{
	char resolved[FD_PATH];
	int n=fs_list(path,&resolved[0],FD_PATH,fdHidden);
	if(n<0)return 0;
	StrCpy(&fdDir[0],&resolved[0],FD_PATH);
	TCtl& c=tvCtl[fdList];
	c.count=n;
	c.val=0;
	c.top=0;
	int k=fs_index(selectName);
	if(k>=0)c.val=k;
	return 1;
}

void FdJoin(char* out, char* name)
{
	snprintf(out,FD_PATH,"%s/%s",&fdDir[0],name);
}

// Lists the parent directory with the directory we left selected.
void FdGoUp()
{
	int base=0;
	int k=0;
	while(fdDir[k]!=0)
	{
		if(fdDir[k]=='/')base=k+1;
		k++;
	}
	char left[256];
	StrCpy(&left[0],&fdDir[base],256);
	char path[FD_PATH];
	FdJoin(&path[0],"..");
	if(!FdLoad(&path[0],&left[0]))
		TvMessage("Error","Cannot read the parent directory.",0);
}

// Enters the list's selected directory. Returns 0 if a file is selected.
int FdEnterSelected()
{
	char name[256];
	int isDir=fs_name(tvCtl[fdList].val,&name[0],256);
	if(!isDir)return 0;
	if((name[0]=='.')&&(name[1]=='.')&&(name[2]==0))
	{
		FdGoUp();
		return 1;
	}
	char path[FD_PATH];
	FdJoin(&path[0],&name[0]);
	if(!FdLoad(&path[0],""))
		TvMessage("Error","Cannot read this directory.",0);
	return 1;
}

void TvAppListItem(int win, int ctl, int i, char* out, int max)
{
	if(tvWin[win].type==W_THEME)
	{
		TeListItem(ctl,i,out,max);
		return;
	}
	int isDir=fs_name(i,out,max-1);
	if(isDir)
	{
		int n=StrLen(out);
		out[n]='/';
		out[n+1]=0;
	}
}

void DrawFiles(int win)
{
	TWin& t=tvWin[win];
	// picking a file in the list fills in the name to save under
	if(fdNameBox>=0)
	{
		TCtl& lst=tvCtl[fdList];
		if((lst.val!=fdLastSel)&&(t.focus==fdList))
		{
			char picked[200];
			if(!fs_name(lst.val,&picked[0],200))
			{
				StrCpy(&tvCtl[fdNameBox].text[0],&picked[0],200);
				tvCtl[fdNameBox].val=StrLen(&picked[0]);
				tvCtl[fdNameBox].top=0;
			}
		}
		fdLastSel=lst.val;
	}
	// the check box has no callback, so notice a change here
	if(tvCtl[fdHiddenBox].val!=fdHidden)
	{
		fdHidden=tvCtl[fdHiddenBox].val;
		t.focus=fdList;
		char keep[256];
		fs_name(tvCtl[fdList].val,&keep[0],256);
		char path[FD_PATH];
		StrCpy(&path[0],&fdDir[0],FD_PATH);
		FdLoad(&path[0],&keep[0]);
	}
	// the end of a long path is the part worth showing
	int room=t.w-6;
	int cols=TvCols(&fdDir[0]);
	int skip=0;
	if(cols>room)skip=cols-room+1;
	int i=0;
	for(int k=0;k<skip;k++)
		TvUtf8(&fdDir[0],i);
	int x=t.x+3;
	if(skip>0)
	{
		TvPut(x,t.y+2,G_LEFT,0x71);
		x++;
	}
	TvText(x,t.y+2,&fdDir[i],A_DLG);
	// details of the selected entry
	char info[80];
	char name[256];
	TCtl& c=tvCtl[fdList];
	fs_name(c.val,&name[0],256);
	fs_info(c.val,&info[0],80);
	int y=t.y+t.h-3;
	TvFill(t.x+2,y,t.w-4,1,' ',0x30);
	int e=TvTextN(t.x+3,y,&name[0],0x3F,t.w-StrLen(&info[0])-10);
	TvText(t.x+t.w-3-StrLen(&info[0]),y,&info[0],0x30);
}

// Letters jump to the next entry that starts with them, Backspace goes up.
int FilesKey(int win, int key)
{
	TWin& t=tvWin[win];
	if(t.focus!=fdList)return 0;
	if(key==K_BACKSPACE)
	{
		t.result=CM_DIRUP;
		return 1;
	}
	if((key>' ')&&(key<127))
	{
		TCtl& c=tvCtl[fdList];
		if(c.count>0)
		{
			int k=fs_find(key,c.val);
			if(k>=0)c.val=k;
		}
		return 1;
	}
	return 0;
}

// Builds the file dialog. Returns the window, or -1 with a message shown.
int FdOpenDialog(char* title, char* okLabel, int saving)
{
	if((tvW<50)||(tvH<18))
	{
		TvMessage(title,"The terminal is too small.",0);
		return -1;
	}
	int w=72;
	if(w>tvW-2)w=tvW-2;
	int h=24;
	if(h>tvH-2)h=tvH-2;
	int win=TvNewCentered(W_FILES,title,w,h,WF_DIALOG);
	if(win<0)return -1;
	int rows=h-8;
	if(saving)rows=h-10;
	TvAdd(win,C_LABEL,3,3,10,1,"~F~iles",0);
	fdList=TvAdd(win,C_LIST,3,4,w-23,rows,"",0);
	tvCtl[fdList].virt=1;
	int bx=w-18;
	int b=TvAdd(win,C_BUTTON,bx,4,14,2,okLabel,CM_OK);
	tvCtl[b].isDefault=1;
	TvAdd(win,C_BUTTON,bx,6,14,2,"~U~p",CM_DIRUP);
	TvAdd(win,C_BUTTON,bx,8,14,2,"Ho~m~e",CM_DIRHOME);
	TvAdd(win,C_BUTTON,bx,10,14,2,"Cancel",CM_CANCEL);
	fdHiddenBox=TvAdd(win,C_CHECK,bx-1,12,16,1,"~H~idden",0);
	tvCtl[fdHiddenBox].val=fdHidden;
	fdNameBox=-1;
	if(saving)
	{
		TvAdd(win,C_LABEL,3,h-5,6,1,"~N~ame:",0);
		fdNameBox=TvAdd(win,C_INPUT,9,h-5,w-13,1,"",0);
	}
	tvWin[win].focus=fdList;
	return win;
}

// Starts the dialog in this directory, or the last one. Returns 0 if none can be read.
int FdStart(int win, char* dir)
{
	char path[FD_PATH];
	if(dir[0]!=0)
		StrCpy(&path[0],dir,FD_PATH);
	else if(fdDir[0]==0)
		StrCpy(&path[0],".",FD_PATH);
	else
		StrCpy(&path[0],&fdDir[0],FD_PATH);
	if(FdLoad(&path[0],""))return 1;
	if(FdLoad(".",""))return 1;
	if(FdLoad("/",""))return 1;
	TvClose(win);
	TvMessage("Error","Cannot read this directory.",0);
	return 0;
}

// The file's name ends with ext.
int EndsWith(char* s, char* ext)
{
	int n=StrLen(s);
	int m=StrLen(ext);
	if(n<m)return 0;
	for(int i=0;i<m;i++)
		if(LowerKey(s[n-m+i])!=ext[i])return 0;
	return 1;
}

// Opens a picked file: .ans files in ANSI paint, binary ones in the hex viewer if
// wanted, the rest in the editor. hex sends every file to the hex viewer.
void OpenPicked(char* path, int hex)
{
	if(hex)
	{
		HxOpen(path);
		return;
	}
	if(EndsWith(path,".ans"))
	{
		if(PtCanClose(ptWin))PtLoad(path);
		return;
	}
	char head[4096];
	int n=fs_load(path,&head[0],4096);
	int binary=0;
	for(int i=0;i<n;i++)
		if(head[i]==0)binary=1;
	if(binary)
	{
		int r=TvChoose("Open","This looks like a binary file.","~H~ex view|~T~ext|~C~ancel");
		if(r==0)HxOpen(path);
		if(r==1)OpenFile(path);
		return;
	}
	OpenFile(path);
}

void RunOpen(int hex)
{
	char* title="Open a file";
	if(hex)title="Hex view";
	int win=FdOpenDialog(title,"~O~pen",0);
	if(win<0)return;
	if(!FdStart(win,""))return;
	char path[FD_PATH];
	while(tvRunning)
	{
		// after a button changes the directory, the keys belong to the list again
		tvWin[win].focus=fdList;
		int res=TvExec(win);
		if(res==CM_DIRUP)
			FdGoUp();
		else if(res==CM_DIRHOME)
		{
			fs_home(&path[0],FD_PATH);
			if(!FdLoad(&path[0],""))
				TvMessage("Error","Cannot read the home directory.",0);
		}
		else if(res==CM_OK)
		{
			char name[256];
			fs_name(tvCtl[fdList].val,&name[0],256);
			if(name[0]!=0)
				if(!FdEnterSelected())
				{
					FdJoin(&path[0],&name[0]);
					TvClose(win);
					OpenPicked(&path[0],hex);
					return;
				}
		}
		else
			break;
	}
	TvClose(win);
}

// Asks for a name and saves the editor window there, or the ANSI paint picture
// when ew<0. Returns 1 if it was saved.
int RunSaveAs(int ew)
{
	char* old=&ptPath[0];
	if(ew>=0)old=&ed[ew].path[0];
	int win=FdOpenDialog("Save as","~S~ave",1);
	if(win<0)return 0;
	char dir[FD_PATH];
	dir[0]=0;
	char first[256];
	first[0]=0;
	if(old[0]!=0)
	{
		int base=EdBase(old);
		StrCpy(&first[0],&old[base],256);
		StrCpy(&dir[0],old,FD_PATH);
		if(base>1)dir[base-1]=0;
		else dir[base]=0;
	}
	else if(ew<0)StrCpy(&first[0],"picture.ans",256);
	if(!FdStart(win,&dir[0]))return 0;
	StrCpy(&tvCtl[fdNameBox].text[0],&first[0],200);
	tvCtl[fdNameBox].val=StrLen(&first[0]);
	char path[FD_PATH];
	char full[FD_PATH];
	int saved=0;
	int done=0;
	fdLastSel=tvCtl[fdList].val;
	while(tvRunning&&!done)
	{
		tvWin[win].focus=fdNameBox;
		int res=TvExec(win);
		if(res==CM_DIRUP)
			FdGoUp();
		else if(res==CM_DIRHOME)
		{
			fs_home(&path[0],FD_PATH);
			if(!FdLoad(&path[0],""))
				TvMessage("Error","Cannot read the home directory.",0);
		}
		else if(res==CM_OK)
		{
			int used=0;
			if(tvWin[win].focus==fdList)
				used=FdEnterSelected();
			char* typed=&tvCtl[fdNameBox].text[0];
			if((!used)&&(typed[0]!=0))
			{
				if(typed[0]=='/')StrCpy(&path[0],typed,FD_PATH);
				else FdJoin(&path[0],typed);
				if(fs_isdir(&path[0]))
				{
					if(!FdLoad(&path[0],""))
						TvMessage("Error","Cannot read this directory.",0);
					typed[0]=0;
					tvCtl[fdNameBox].val=0;
				}
				else
				{
					int where=-2;
					if(StrLen(&path[0])<FD_PATH-1)where=EdAbsPath(&path[0],&full[0]);
					int other=-1;
					if(where>=0)
						if(ew>=0)other=EdFindPath(&full[0]);
					if(where==-2)
						TvMessage("Save as","That path is too long.",0);
					else if(where<0)
						TvMessage("Save as","There is no such directory.",0);
					else if((other>=0)&&(other!=ew))
						TvMessage("Save as","That file is open in another window.",0);
					else
					{
						int go=1;
						if((where==1)&&(other!=ew))
							go=(TvMessage("Save as","This file already exists.\nReplace it?",1)==CM_YES);
						if(go)
						{
							tvWin[win].result=0;
							if(ew>=0)saved=EdSaveAs(ew,&full[0]);
							else saved=PtSave(&full[0]);
							done=saved;
						}
					}
				}
			}
		}
		else
			done=1;
	}
	TvClose(win);
	fdNameBox=-1;
	return saved;
}

// ---------------------------------------------------------------- help

void OpenHelp()
{
	int win=NewViewer("Help");
	if(win<0)return;
	TWin& t=tvWin[win];
	ed[win].ro=1;
	t.w=62;
	t.h=20;
	if(t.h>tvH-2)t.h=tvH-2;
	t.x=(tvW-t.w)/2;
	t.y=1;
	char* text=
		"Turbo Vision style desktop written in Scinc\n"
		"\n"
		"Keyboard\n"
		"  F10, Alt+letter   open a menu\n"
		"  F2, F3            save, browse for a file to open\n"
		"  F5, Ctrl+W        zoom, close the window\n"
		"  F6, F7            next, previous window\n"
		"  Alt+1..9          go to a window by number\n"
		"  1..9 a b c        click a number on the menu bar to go\n"
		"                    there (12 windows, dialogs aside)\n"
		"  Alt+0             the list of windows\n"
		"  Alt+N             the Windows menu, to pick a window\n"
		"  Double click title  zoom or restore the window\n"
		"  Tab, Shift+Tab    move between dialog controls\n"
		"  Space, Enter      toggle, press\n"
		"  Esc               cancel a dialog or close a menu\n"
		"  Shift+F10         context menu (or click the right button)\n"
		"  In the file list: letters jump to a name, Backspace goes up\n"
		"  Alt+X, Ctrl+Q     quit\n"
		"  Ctrl+= Ctrl+- Ctrl+0  in a graphic window: bigger, smaller\n"
		"                    pixels (1x to 4x), and back; also\n"
		"                    Options > Pixel size\n"
		"\n"
		"Editor\n"
		"  Ctrl+N, Ctrl+O, Ctrl+S   new, open, save\n"
		"  Ctrl+Z, Ctrl+Y           undo, redo\n"
		"  Ctrl+X, Ctrl+C, Ctrl+V   cut, copy, paste\n"
		"  Ctrl+A                   select all\n"
		"  Ctrl+F, Ctrl+L           find, find next\n"
		"  Ctrl+R, Ctrl+G           replace, go to line\n"
		"  Shift+arrows, Home, End  select\n"
		"  Alt+Shift+arrows         select a block of columns\n"
		"  Ctrl+arrows              move by word\n"
		"  Ctrl+Home, Ctrl+End      start, end of the file\n"
		"  Tab, Shift+Tab           indent, unindent the selected lines\n"
		"  Ctrl+B                   go to the matching bracket\n"
		"  Brackets next to the cursor show their partner, or red if\n"
		"  it has none. In C, Enter after { indents one more level and\n"
		"  } on an empty line takes a level away.\n"
		"  Options has line numbers and word wrap, Window, List (Alt+0)\n"
		"  switches between or closes the open windows.\n"
		"  A * on the frame means there are unsaved changes.\n"
		"  The clipboard is the editor's own. To copy to other programs,\n"
		"  hold Shift and drag, which most terminals leave to themselves.\n"
		"  Their paste comes in as one undoable edit.\n"
		"\n"
		"Mouse\n"
		"  Drag a title bar to move a window, and its bottom right\n"
		"  corner to resize it.\n"
		"  Click the box on the left of the title to close the\n"
		"  window, and the arrow on the right to zoom it.\n"
		"  Drag a scroll bar thumb, or use the wheel, to scroll.\n"
		"  Double click a list entry to open it, or a word to select it.\n"
		"  Drag over text to select it.\n"
		"\n"
		"Mandelbrot\n"
		"  The wheel zooms at the pointer, a drag moves the picture and\n"
		"  a double click zooms in. + - zoom, arrows move, [ ] change\n"
		"  the iterations, P the palette, Home starts again.\n"
		"\n"
		"Game of Life\n"
		"  Draw cells with the mouse (on a live cell it erases). Space\n"
		"  runs and pauses, N steps, C clears, R fills at random, + -\n"
		"  change the speed, 1-4 put a gun, an R-pentomino, an acorn\n"
		"  and a spaceship in the middle.\n"
		"\n"
		"Sonar\n"
		"  Open the sea without hitting a mine. A number counts the\n"
		"  mines around its cell. Right click (or B) drops a buoy on a\n"
		"  mine, a click on a number with all its buoys opens the rest.\n"
		"  Three pings (P or the middle button) open a 3x3 patch safely.\n"
		"  1-3 choose the size, N starts again.\n"
		"\n"
		"Hex view (File menu, or open a binary file)\n"
		"  Arrows, PgUp, PgDn, Ctrl+Home, Ctrl+End move, G or Ctrl+G\n"
		"  goes to an offset.\n"
		"\n"
		"ANSI paint\n"
		"  Pick the ink, the paper and a character below the picture, or\n"
		"  type a character. Tab changes the tool: Brush, Text, Fill, Pick.\n"
		"  Ctrl+Z undoes, F2 saves an .ans file, F3 opens one.\n"
		"\n"
		"Screen savers\n"
		"  Stars, plasma, matrix rain, flame, reaction-diffusion, fluid,\n"
		"  landscape, labyrinth, objects and a nyan cat,\n"
		"  full screen after the idle time set in Options, Preferences, or\n"
		"  at once from Options, Start screen saver. Start saver in a window\n"
		"  (or Saver in a window in the = menu) opens one in a window, where\n"
		"  Space shows the next one and 1-9 and 0 pick one.\n"
		"  Reaction-diffusion grows Gray-Scott patterns (coral, mitosis,\n"
		"  maze, worms, holes, bubbles): P the next pattern, R starts it\n"
		"  again, dragging the mouse drops chemical. Fluid is a stable\n"
		"  fluids simulation with three coloured jets: dragging stirs it,\n"
		"  R clears it. Landscape flies over generated land (red desert,\n"
		"  islands, glacier, dusk): the arrows steer and climb, N gives new\n"
		"  land.\n"
		"  Labyrinth walks through generated mazes (stone halls, hedge\n"
		"  garden, station) until it finds the gate out: the arrows walk\n"
		"  yourself (the walker takes over after five seconds), M shows\n"
		"  the map, N gives a new maze.\n"
		"  Objects turns a box, metaballs, a torus, a torus knot or a\n"
		"  cylinder in copper, silver, gold, red plastic, cheese, pink\n"
		"  plaster or red-hot iron, or a small molecule or a piece of\n"
		"  diamond or graphene, drawn by its own scanline renderer: O the\n"
		"  next object, M the next material, N both anew.\n"
		"\n"
		"Options, Preferences sets the colour theme and the screen saver.\n"
		"Options, Theme editor makes the Custom theme: pick an item on the\n"
		"left (menus, windows, dialogs, controls, the editor and its syntax\n"
		"colours, and the palette games and paint use), Text or Background,\n"
		"and its colour from the terminal's 16, xterm's 256, or any 24-bit\n"
		"colour with the RGB/HSV square and sliders. The radio button by a\n"
		"slider picks the channel the square leaves out. Syntax colours and\n"
		"line numbers can have a clear background: the editor's. Save keeps\n"
		"the theme, Save as keeps it under a new name; Classic blue is built\n"
		"in and stays as it is.\n"
		"\n"
		"Your terminal may catch F10 or Alt keys for itself. If it\n"
		"does, use the mouse or Ctrl+Q.\n";
	char line[80];
	int o=0;
	int i=0;
	while(text[i]!=0)
	{
		if(text[i]==10)
		{
			line[o]=0;
			VwAddLine(t,&line[0]);
			o=0;
		}
		else if(o<78)
		{
			line[o]=text[i];
			o++;
		}
		i++;
	}
}

// ---------------------------------------------------------------- application hooks

int FindWin(int type)
{
	for(int i=0;i<TV_MAXWIN;i++)
		if(tvWin[i].used)
			if(tvWin[i].type==type)return i;
	return -1;
}

void TvAppDraw(int win)
{
#ifdef TV_GFX
	if(GfxScale()!=pixShown)ShowPixSize();
#endif
	int type=tvWin[win].type;
	if(type==W_VIEW)DrawViewer(win);
	else if(type==W_CALC)DrawCalc(win);
	else if(type==W_CAL)DrawCalendar(win);
	else if(type==W_ASCII)DrawAscii(win);
	else if(type==W_PUZZLE)DrawPuzzle(win);
	else if(type==W_FILES)DrawFiles(win);
	else if(type==W_CIRCUIT)CfDraw(win);
	else if(type==W_CONTROLS)DrawControls(win);
	else if(type==W_PREFS)DrawPrefs(win);
	else if(type==W_MANDEL)MbDraw(win);
	else if(type==W_LIFE)LfDraw(win);
	else if(type==W_SAVER)SvDraw(tvWin[win].a,tvWin[win].x+1,tvWin[win].y+1,tvWin[win].w-2,tvWin[win].h-2);
	else if(type==W_SONAR)SnDraw(win);
	else if(type==W_HEX)HxDraw(win);
	else if(type==W_PAINT)PtDraw(win);
	else if(type==W_THEME)TeDraw(win);
}

int TvAppKey(int win, int key)
{
	TWin& t=tvWin[win];
	int type=t.type;
	if(type==W_VIEW)return ViewerKey(win,key);
	if(type==W_CALC)return CalcFromKeyboard(key);
	if(type==W_ASCII)return AsciiKey(t,key);
	if(type==W_PUZZLE)return PuzzleKey(key);
	if(type==W_FILES)return FilesKey(win,key);
	if(type==W_CIRCUIT)return CfKey(win,key);
	if(type==W_MANDEL)return MbKey(win,key);
	if(type==W_LIFE)return LfKey(win,key);
	if(type==W_SAVER)return SvWinKey(win,key);
	if(type==W_SONAR)return SnKey(win,key);
	if(type==W_HEX)return HxKey(win,key);
	if(type==W_PAINT)return PtKey(win,key);
	if(type==W_THEME)return TeKey(win,key);
	if(type==W_CAL)
	{
		if((key==K_LEFT)||(key==K_PGUP))
		{
			CalShift(t,-1);
			return 1;
		}
		if((key==K_RIGHT)||(key==K_PGDN))
		{
			CalShift(t,1);
			return 1;
		}
	}
	return 0;
}

int TvAppMouse(int win, int ev, int x, int y)
{
	TWin& t=tvWin[win];
	int type=t.type;
	if(type==W_SONAR)return SnMouse(win,ev,x,y);
	if(type==W_THEME)return TeMouse(win,ev,x,y);
	if(type==W_SAVER)
		if(ev!=EV_RDOWN)return SvWinMouse(win,ev,x,y);
	if(ev==EV_RDOWN)
	{
		if(t.flags&WF_DIALOG)return 0;
		return ContextMenu(win,x,y);
	}
	if(type==W_VIEW)return ViewerMouse(win,ev,x,y);
	if(type==W_CIRCUIT)return CfMouse(win,ev,x,y);
	if(type==W_MANDEL)return MbMouse(win,ev,x,y);
	if(type==W_LIFE)return LfMouse(win,ev,x,y);
	if(type==W_HEX)return HxMouse(win,ev,x,y);
	if(type==W_PAINT)return PtMouse(win,ev,x,y);
	if(ev!=EV_MDOWN)return 0;
	if(type==W_CAL)
	{
		if(y==1)
		{
			if(x<=3)CalShift(t,-1);
			if(x>=t.w-4)CalShift(t,1);
		}
		return 1;
	}
	if(type==W_ASCII)
	{
		int cx=x-1;
		int cy=y-1;
		if((cx>=0)&&(cx<32)&&(cy>=0)&&(cy<8))
			t.a=cy*32+cx;
		return 1;
	}
	if(type==W_PUZZLE)
	{
		int cx=(x-2)/5;
		int cy=(y-1)/2;
		if((x>=2)&&(y>=1)&&(cx<4)&&(cy<4))
			if(BoardMove(cy*4+cx))boardMoves++;
		return 1;
	}
	return 0;
}

void TvAppButton(int win, int ctl, int cmd)
{
	if(cmd==CM_THEMEPICK)
	{
		TeUseTheme(tvCtl[ctl].val);
		return;
	}
	if(cmd==CM_DESKPICK)
	{
		// shown at once, over whichever theme the dialog's combo box has
		teDeskPick=tvCtl[ctl].val;
		if(prefsThemeCtl>=0)TeUseTheme(tvCtl[prefsThemeCtl].val);
		return;
	}
	if((cmd>=CM_TEDEPTH)&&(cmd<=CM_TETHEME))
	{
		TeButton(win,ctl,cmd);
		return;
	}
	if(cmd==CM_VOLUME)
	{
		tvCtl[ctlVolBar].val=tvCtl[ctl].val;
		return;
	}
	if(cmd>=CM_CALCKEY)
		CalcKey(CalcKeyOf(cmd-CM_CALCKEY));
}

void TvAppClosed(int win)
{
	if(tvWin[win].type==W_VIEW)
		EdClosed(win);
	if(tvWin[win].type==W_CIRCUIT)
		CfClosed(win);
	if(win==mbWin)mbWin=-1;
	if(win==lfWin)lfWin=-1;
	if(win==snWin)snWin=-1;
	if(win==ptWin)ptWin=-1;
	if(win==hxWin)
	{
		free(hxBuf);
		hxWin=-1;
	}
	TickUpdate();
}

void TvAppTick()
{
	CfTick();
	ProgressTick();
	MbTick();
	LfTick();
	SvTick();
	TickUpdate();
}

int TvAppCanClose(int win)
{
	if(win==ptWin)return PtCanClose(win);
	return EdCanClose(win);
}

int TvAppPaste(int win, char* s, int n)
{
	return EdPaste(win,s,n);
}

void TvAppCommand(int cmd)
{
	// the editor's commands that mean something to the hex viewer and ANSI paint
	int top=TvTop();
	if(top>=0)
	{
		if((top==hxWin)&&(cmd==CM_GOTO))
		{
			HxGoto(top);
			return;
		}
		if(top==ptWin)
		{
			if(cmd==CM_UNDO)
			{
				PtUndo();
				return;
			}
			if((cmd==CM_SAVE)&&(ptPath[0]!=0))
			{
				PtSave(&ptPath[0]);
				return;
			}
			if((cmd==CM_SAVE)||(cmd==CM_SAVEAS))
			{
				RunSaveAs(-1);
				return;
			}
		}
	}
	if(EdCommand(cmd))return;
	if(cmd==CM_QUIT)
	{
		if(ptWin>=0)
			if(!PtCanClose(ptWin))return;
		int r=EdCanQuit();
		if(r==1)tvRunning=0;
		else if(r==2)
			if(TvMessage("Quit","Leave the desktop?",1)==CM_YES)
				tvRunning=0;
	}
	else if(cmd==CM_OPEN)RunOpen(0);
	else if(cmd==CM_HEX)RunOpen(1);
	else if(cmd==CM_SONAR)SnOpen();
	else if(cmd==CM_LINENUMS)
	{
		edLineNums=!edLineNums;
		ShowToggles();
		PrefsSave();
	}
	else if(cmd==CM_WRAP)
	{
		edWrap=!edWrap;
		ShowToggles();
		PrefsSave();
	}
	else if(cmd==CM_PAINT)PtOpen();
	else if(cmd==CM_DOCALC)
	{
		int w=FindWin(W_CALC);
		if(w>=0)TvToFront(w);
		else OpenCalculator();
	}
	else if(cmd==CM_CALENDAR)
	{
		int w=FindWin(W_CAL);
		if(w>=0)TvToFront(w);
		else
		{
			w=TvNewCentered(W_CAL,"Calendar",24,11,WF_DIALOG);
			if(w>=0)
			{
				tvWin[w].a=term_time(3);
				tvWin[w].b=term_time(4);
			}
		}
	}
	else if(cmd==CM_ASCII)
	{
		int w=FindWin(W_ASCII);
		if(w>=0)TvToFront(w);
		else TvNewCentered(W_ASCII,"ASCII table",34,11,WF_DIALOG);
	}
	else if(cmd==CM_PUZZLE)
	{
		int w=FindWin(W_PUZZLE);
		if(w>=0)TvToFront(w);
		else
		{
			BoardShuffle();
			TvNewCentered(W_PUZZLE,"Puzzle",24,11,WF_DIALOG);
		}
	}
	else if(cmd==CM_CIRCUIT)CfOpen(CF_LOOPS);
	else if(cmd==CM_CIRCUIT2)CfOpen(CF_WIRES);
	else if(cmd==CM_CONTROLS)RunControls();
	else if(cmd==CM_PREFS)RunPrefs();
	else if(cmd==CM_THEMEED)
	{
		int r=TeRun(prefTheme);
		if(r>=0)
		{
			prefTheme=r;
			PrefsSave();
		}
	}
	else if(cmd==CM_PROGRESS)RunProgress();
	else if(cmd==CM_MANDEL)MbOpen();
	else if(cmd==CM_LIFE)LfOpen();
	else if(cmd==CM_SAVERNOW)SvStart();
	else if(cmd==CM_SAVERWIN)SvOpenWindow(svKind);
	else if((cmd>=CM_SVNOW)&&(cmd<=CM_SVNOW+SV_RANDOM))SvStartKind(cmd-CM_SVNOW);
	else if((cmd>=CM_SVWIN)&&(cmd<=CM_SVWIN+SV_RANDOM))SvOpenWindow(cmd-CM_SVWIN);
	else if((cmd>CM_PIX)&&(cmd<=CM_PIX+4))
	{
#ifdef TV_GFX
		GfxSetScale(cmd-CM_PIX);
		ShowPixSize();
		PrefsSave();
#endif
	}
	else if(cmd==CM_HELP)OpenHelp();
	else if(cmd==CM_ABOUT)
		TvMessage("About","Turbo Vision style desktop\nWritten in Scinc, drawn with ANSI escapes",0);
	TickUpdate();
}

// ---------------------------------------------------------------- main

// One item per saver kind, then Random, giving commands base+kind.
void SaverItems(int base)
{
	TvMenuItem("~S~tars","",base+SV_STARS,0);
	TvMenuItem("~P~lasma","",base+SV_PLASMA,0);
	TvMenuItem("~M~atrix","",base+SV_MATRIX,0);
	TvMenuItem("~F~lame","",base+SV_FLAME,0);
	TvMenuItem("~R~eaction-diffusion","",base+SV_REACT,0);
	TvMenuItem("Fl~u~id","",base+SV_FLUID,0);
	TvMenuItem("~L~andscape","",base+SV_LAND,0);
	TvMenuItem("La~b~yrinth","",base+SV_MAZE,0);
	TvMenuItem("~O~bjects","",base+SV_OBJ,0);
	TvMenuItem("~N~yan cat","",base+SV_NYAN,0);
	TvMenuLine();
	TvMenuItem("R~a~ndom","",base+SV_RANDOM,0);
}

int main(int argc, pchar* argv)
{
	if(!TvStart(CM_QUIT))return 1;
	EdInit();
	StrCpy(&ctlName[0],"Scinc",64);
	fdDir[0]=0;
	PrefsLoad();
#ifdef TV_GFX
	if(prefScale>0)GfxSetScale(prefScale);
#endif
	TeLoadThemes();
	if(prefTheme>=teThN)prefTheme=0;
	TeUseTheme(prefTheme);
	TickUpdate();

	TvMenuAdd("~=~");
	TvMenuItem("~A~bout...","",CM_ABOUT,0);
	TvMenuLine();
	TvMenuItem("~C~alculator","",CM_DOCALC,0);
	TvMenuItem("Cal~e~ndar","",CM_CALENDAR,0);
	TvMenuItem("A~S~CII table","",CM_ASCII,0);
	TvMenuItem("~P~uzzle","",CM_PUZZLE,0);
	TvMenuItem("Circuitfall: ~l~oops","",CM_CIRCUIT,0);
	TvMenuItem("Circuitfall: ~w~ires","",CM_CIRCUIT2,0);
	TvMenuItem("~M~andelbrot","",CM_MANDEL,0);
	TvMenuItem("~G~ame of Life","",CM_LIFE,0);
	TvMenuItem("S~o~nar","",CM_SONAR,0);
	TvMenuItem("ANSI pa~i~nt","",CM_PAINT,0);
	int itemDeskSaver=TvMenuSub("Sa~v~er in a window");
	TvMenuAdd("~F~ile");
	TvMenuItem("~N~ew","Ctrl+N",CM_NEW,14);
	TvMenuItem("~O~pen...","F3",CM_OPEN,K_F3);
	TvMenuItem("~S~ave","Ctrl+S",CM_SAVE,19);
	TvMenuItem("Save ~a~s...","",CM_SAVEAS,0);
	TvMenuItem("~H~ex view...","",CM_HEX,0);
	TvMenuLine();
	TvMenuItem("E~x~it","Alt+X",CM_QUIT,K_ALT+'x');
	TvMenuAdd("~E~dit");
	TvMenuItem("~U~ndo","Ctrl+Z",CM_UNDO,26);
	TvMenuItem("~R~edo","Ctrl+Y",CM_REDO,25);
	TvMenuLine();
	TvMenuItem("Cu~t~","Ctrl+X",CM_CUT,24);
	TvMenuItem("~C~opy","Ctrl+C",CM_COPY,3);
	TvMenuItem("~P~aste","Ctrl+V",CM_PASTE,22);
	TvMenuLine();
	TvMenuItem("Select ~a~ll","Ctrl+A",CM_SELALL,1);
	TvMenuAdd("~S~earch");
	TvMenuItem("~F~ind...","Ctrl+F",CM_FIND,6);
	TvMenuItem("Find ~n~ext","Ctrl+L",CM_FINDNEXT,12);
	TvMenuItem("~R~eplace...","Ctrl+R",CM_REPLACE,18);
	TvMenuLine();
	TvMenuItem("~G~o to line...","Ctrl+G",CM_GOTO,7);
	TvMenuItem("~M~atching bracket","Ctrl+B",CM_MATCH,2);
	TvMenuAdd("~D~ialogs");
	TvMenuItem("~C~ontrols...","",CM_CONTROLS,0);
	TvMenuItem("~M~essage...","",CM_ABOUT,0);
	TvMenuItem("~P~rogress...","",CM_PROGRESS,0);
	TvMenuAdd("~O~ptions");
	TvMenuItem("~P~references...","",CM_PREFS,0);
	TvMenuItem("~T~heme editor...","",CM_THEMEED,0);
	int itemSaverNow=TvMenuSub("Start screen ~s~aver");
	int itemSaverWin=TvMenuSub("Start saver in a w~i~ndow");
	TvMenuLine();
	TvMenuItem("~L~ine numbers","off",CM_LINENUMS,0);
	itemLineNums=tvNItem-1;
	TvMenuItem("~W~ord wrap","off",CM_WRAP,0);
	itemWrap=tvNItem-1;
#ifdef TV_GFX
	int itemPixSub=TvMenuSub("Pi~x~el size");
#endif
	TvMenuAdd("~W~indow");
	TvMenuItem("~N~ext","F6",CM_NEXT,K_F6);
	TvMenuItem("~P~revious","F7",CM_PREV,K_F7);
	TvMenuItem("~Z~oom","F5",CM_ZOOM,K_F5);
	TvMenuItem("~C~lose","Ctrl+W",CM_CLOSE,23);
	TvMenuLine();
	TvMenuItem("C~a~scade","",CM_CASCADE,0);
	TvMenuItem("~T~ile","",CM_TILE,0);
	TvMenuItem("~L~ist...","Alt+0",CM_WINLIST,K_ALT+'0');
	TvWindowsMenu("Wi~n~dows");
	TvMenuAdd("~H~elp");
	TvMenuItem("~K~eys and mouse","F1",CM_HELP,K_F1);

	// the savers, full screen and in a window; the second also from the = menu
	TvSubmenuAdd(itemSaverNow);
	SaverItems(CM_SVNOW);
	int subWin=TvSubmenuAdd(itemSaverWin);
	SaverItems(CM_SVWIN);
	TvSubmenuShare(itemDeskSaver,subWin);
#ifdef TV_GFX
	TvSubmenuAdd(itemPixSub);
	TvMenuItem("~1~x","",CM_PIX+1,0);
	itemPix=tvNItem-1;
	TvMenuItem("~2~x","",CM_PIX+2,0);
	TvMenuItem("~3~x","",CM_PIX+3,0);
	TvMenuItem("~4~x","",CM_PIX+4,0);
#endif

	popEdit=TvPopupAdd();
	TvMenuItem("~U~ndo","Ctrl+Z",CM_UNDO,0);
	TvMenuItem("~R~edo","Ctrl+Y",CM_REDO,0);
	TvMenuLine();
	TvMenuItem("Cu~t~","Ctrl+X",CM_CUT,0);
	TvMenuItem("~C~opy","Ctrl+C",CM_COPY,0);
	TvMenuItem("~P~aste","Ctrl+V",CM_PASTE,0);
	TvMenuItem("Select ~a~ll","Ctrl+A",CM_SELALL,0);
	TvMenuLine();
	TvMenuItem("~F~ind...","Ctrl+F",CM_FIND,0);
	TvMenuItem("~G~o to line...","Ctrl+G",CM_GOTO,0);
	TvMenuLine();
	TvMenuItem("C~l~ose","Ctrl+W",CM_CLOSE,0);
	popWin=TvPopupAdd();
	TvMenuItem("~Z~oom","F5",CM_ZOOM,0);
	TvMenuItem("~N~ext","F6",CM_NEXT,0);
	TvMenuItem("~C~lose","Ctrl+W",CM_CLOSE,0);
	tvDeskPopup=TvPopupAdd();
	TvMenuItem("~N~ew file","Ctrl+N",CM_NEW,0);
	TvMenuItem("~O~pen...","F3",CM_OPEN,0);
	TvMenuLine();
	TvMenuItem("~C~alculator","",CM_DOCALC,0);
	TvMenuItem("Cal~e~ndar","",CM_CALENDAR,0);
	TvMenuLine();
	TvMenuItem("C~a~scade","",CM_CASCADE,0);
	TvMenuItem("~T~ile","",CM_TILE,0);
	TvMenuLine();
	TvMenuItem("~P~references...","",CM_PREFS,0);

	// the status line shows the main keys of the front window: items after
	// TvStatusFor(type) show for that window type, TV_ST_OTHER ones for the rest. A
	// key given with a command works everywhere, as a menu item's does.
	TvStatusFor(TV_ST_OTHER);
	TvStatusAdd("~F1~ Help",CM_HELP,0);
	TvStatusAdd("~F2~ Save",CM_SAVE,K_F2);
	TvStatusAdd("~F3~ Open",CM_OPEN,15); // Ctrl+O opens too
	TvStatusAdd("~F6~ Next",CM_NEXT,0);
	TvStatusFor(W_VIEW);
	TvStatusAdd("~F2~ Save",CM_SAVE,0);
	TvStatusAdd("~Ctrl+F~ Find",CM_FIND,0);
	TvStatusAdd("~Ctrl+L~ Next",CM_FINDNEXT,0);
	TvStatusAdd("~Ctrl+R~ Replace",CM_REPLACE,0);
	TvStatusAdd("~Ctrl+G~ Line",CM_GOTO,0);
	TvStatusFor(W_MANDEL);
	TvStatusAdd("~+~ In",0,'+');
	TvStatusAdd("~-~ Out",0,'-');
	TvStatusAdd("~[~ ~]~ Iterations",0,0);
	TvStatusAdd("~P~ Palette",0,'p');
	TvStatusAdd("~Home~ Reset",0,K_HOME);
	TvStatusFor(W_LIFE);
	TvStatusAdd("~Space~ Run",0,' ');
	TvStatusAdd("~N~ Step",0,'n');
	TvStatusAdd("~C~ Clear",0,'c');
	TvStatusAdd("~R~ Random",0,'r');
	TvStatusAdd("~+~ ~-~ Speed",0,0);
	TvStatusAdd("~1~-~4~ Shapes",0,0);
	TvStatusFor(W_SONAR);
	TvStatusAdd("~B~ Buoy",0,'b');
	TvStatusAdd("~P~ Ping",0,'p');
	TvStatusAdd("~N~ New",0,'n');
	TvStatusAdd("~1~-~3~ Size",0,0);
	TvStatusFor(W_CIRCUIT);
	TvStatusAdd("~N~ New",0,'n');
	TvStatusAdd("~M~ Mode",0,'m');
	TvStatusAdd("~P~ Pause",0,'p');
	TvStatusFor(W_SAVER);
	TvStatusAdd("~Space~ Next",0,' ');
	TvStatusAdd("~1~-~9~ Pick",0,0);
	TvStatusFor(W_HEX);
	TvStatusAdd("~G~ Go to",0,'g');
	TvStatusAdd("~PgUp~ ~PgDn~ Page",0,0);
	TvStatusFor(W_PAINT);
	TvStatusAdd("~Tab~ Tool",0,K_TAB);
	TvStatusAdd("~Ctrl+Z~ Undo",CM_UNDO,0);
	TvStatusAdd("~F2~ Save",CM_SAVE,0);
	TvStatusAdd("~F3~ Open",CM_OPEN,0);
	TvStatusFor(W_CAL);
	TvStatusAdd("~PgUp~ Earlier",0,K_PGUP);
	TvStatusAdd("~PgDn~ Later",0,K_PGDN);
	TvStatusFor(TV_ST_DIALOG);
	TvStatusAdd("~Tab~ Next field",0,K_TAB);
	TvStatusAdd("~Space~ Toggle",0,' ');
	TvStatusAdd("~Enter~ OK",0,K_ENTER);
	TvStatusAdd("~Esc~ Cancel",0,K_ESC);
	TvStatusFor(TV_ST_ALL);
	TvStatusAdd("~F10~ Menu",CM_MENU,0);
	TvStatusAdd("~Alt+X~ Exit",CM_QUIT,17); // Ctrl+Q quits too

	ShowToggles();
	if(argc>1)
		OpenPicked(argv[1],0);
	else
		OpenHelp();

	TvRun(-1);
#ifdef TV_GFX
	if(GfxScale()!=prefScale)PrefsSave();   // changed by Ctrl+= or Ctrl+-
#endif
	TvStop();
	return 0;
}
