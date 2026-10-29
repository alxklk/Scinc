// ukrainian.cpp - review sheet for the Ukrainian letters of the stroke font
//
//   Scinc ukrainian.cpp          show the sheet
//   Scinc ukrainian.cpp -png     also save it to ukrainian.png
//
// The glyphs are in letters.h (MakeUkrainian), drawn on the same grid as
// the Latin ones: 4 wide, baseline 0, small letters 6 high, capitals 9,
// descenders down to -3. Each cell shows the capital and the small letter
// over faint guide lines (baseline, x-height, cap height); a
// letter drawn in gold shares the Latin glyph (fm.Alias), so changing it
// means giving it its own StartGlyph block. Below are sample lines at several
// sizes, drawn from UTF-8 text with DrawText.
#define G_SCREEN_WIDTH 1200
#define G_SCREEN_HEIGHT 860
#define G_SCREEN_SCALE 1
#define G_SCREEN_MODE 1
#include <stdio.h>
#include "graphics.h"
#include "../ws.h"
#include "font.h"
#include "letters.h"
#include "gtext.h"

typedef char* pchar;

#define NL 33
#define COLS 11
#define CELL_W 106
#define CELL_H 158
#define GRID_X 17
#define GRID_Y 46
#define GS 8.0

#define COL_BG 0xff142a2c
#define COL_CELL 0xff1c3a3c
#define COL_GUIDE 0xff2f5a5c
#define COL_TEXT 0xffeee6cc
#define COL_DIM 0xff8fb0a8
#define COL_GOLD 0xffe8b54a

Graph g;
CFont* pfont=0;
int upper[NL];
int lower[NL];

void Alphabet()
{
	// А Б В Г Ґ Д Е Є Ж З И І Ї Й К Л М Н О П Р С Т У Ф Х Ц Ч Ш Щ Ь Ю Я
	int k=0;
	for(int c=0x410;c<=0x42F;c++)
	{
		if(c==0x42A||c==0x42B||c==0x42D)
			continue;    // Ъ Ы Э are not Ukrainian; Ґ Є І Ї live elsewhere
		upper[k]=c;
		k++;
		if(c==0x413)
		{
			upper[k]=0x490;     // Ґ after Г
			k++;
		}
		if(c==0x415)
		{
			upper[k]=0x404;     // Є after Е
			k++;
		}
		if(c==0x418)
		{
			upper[k]=0x406;     // І, Ї after И
			k++;
			upper[k]=0x407;
			k++;
		}
	}
	for(int i=0;i<NL;i++)
	{
		int c=upper[i];
		if(c==0x490)lower[i]=0x491;
		else if(c>=0x410)lower[i]=c+0x20;
		else lower[i]=c+0x50;
	}
}

void Stroke(float w, int c)
{
	g.fin();
	g.rgba32(c);
	g.width(w,w);
	g.stroke();
}

void Line(float x, float y, float w, int c)
{
	g.clear();
	g.M(x,y);
	g.l(w,0);
	Stroke(1,c);
}

void Glyph(int code, float x, float y, float s, int c)
{
	g.clear();
	DrawGlyph(g,*pfont,code,x,y,s,s);
	Stroke(s*0.32,c);
}

void Text(char* s, float x, float y, float sc, int c)
{
	g.clear();
	DrawText(g,*pfont,s,0,x,y,sc,sc);
	float w=sc*0.32;
	if(w<1.3)w=1.3;
	Stroke(w,c);
}

// is this code an alias of a Latin glyph? (shares its drawing commands)
int IsAlias(int code)
{
	CGlyph* p=FindGlyph(*pfont,code);
	if(!p)return 0;
	CGlyph gl=*p;
	for(int i=33;i<127;i++)
		if((*pfont).g[i].n>0&&(*pfont).g[i].c0==gl.c0)return 1;
	return 0;
}

void Hex(int v, char* s)
{
	char* d="0123456789ABCDEF";
	s[0]='U';
	s[1]='+';
	for(int i=0;i<4;i++)s[2+i]=d[(v>>(12-4*i))&15];
	s[6]=0;
}

void Cell(int i)
{
	float x=GRID_X+(i%COLS)*CELL_W;
	float y=GRID_Y+(i/COLS)*CELL_H;
	g.rgba32(COL_CELL);
	g.fillrect(x+3,y+3,CELL_W-6,CELL_H-6);
	float base=y+100;
	Line(x+8,base,CELL_W-16,COL_GUIDE);
	Line(x+8,base-6*GS,CELL_W-16,COL_GUIDE);
	Line(x+8,base-9*GS,CELL_W-16,COL_GUIDE);
	float gx=x+(CELL_W-10*GS)/2;
	int cu=COL_TEXT;
	int cl=COL_TEXT;
	if(IsAlias(upper[i]))cu=COL_GOLD;
	if(IsAlias(lower[i]))cl=COL_GOLD;
	Glyph(upper[i],gx,base,GS,cu);
	Glyph(lower[i],gx+6*GS,base,GS,cl);
	char s[8];
	Hex(upper[i],s);
	Text(s,x+(CELL_W-70)/2,y+CELL_H-12,2,COL_DIM);
}

void Draw()
{
	g.rgba32(COL_BG);
	g.FillRT();
	Text("Ukrainian letters of the stroke font (letters.h)",GRID_X,30,2.6,COL_TEXT);
	Text("gold = shares the Latin glyph",1200-GRID_X-29*12,30,2,COL_GOLD);
	for(int i=0;i<NL;i++)Cell(i);
	float y=GRID_Y+3*CELL_H+48;
	Text("АБВГҐДЕЄЖЗИІЇЙКЛМНОПРСТУФХЦЧШЩЬЮЯ",GRID_X,y,4,COL_TEXT);
	y+=50.0;
	Text("абвгґдеєжзиіїйклмнопрстуфхцчшщьюя",GRID_X,y,4,COL_TEXT);
	y+=60.0;
	Text("Чуєш їх, доцю, га? Кумедна ж ти, прощайся без ґольфів!",GRID_X,y,3,COL_TEXT);
	y+=45.0;
	Text("Слава Україні! Мʼята, пʼять, обʼєкт, сім'я, п’ять.",GRID_X,y,3,COL_TEXT);
	y+=40.0;
	Text("Жаба = toad, ЖАБА = TOAD. Ґанок, їжак, щука, Юля, Єва, яйце.",GRID_X,y,2.5,COL_TEXT);
	y+=34.0;
	Text("Широкополі капелюхи, жовтогарячі ґудзики, щедрі пʼяльця й еластичні сітки.",GRID_X,y,2,COL_DIM);
}

int main(int argc, pchar* argv)
{
	pfont=(CFont*)malloc(sizeof(CFont));
	CFontMaker fm;
	fm.Init(*pfont,6);
	MakeLetters(fm);
	Alphabet();
	Draw();
	for(int i=1;i<argc;i++)
		if(argv[i][0]=='-'&&argv[i][1]=='p')
		{
			g.WriteImage("ukrainian.png","png","");
			puts("saved ukrainian.png");
		}
	while(true)
	{
		Present();
		Wait(0.1);
	}
	return 0;
}
