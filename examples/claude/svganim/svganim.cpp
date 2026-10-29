// svganim.cpp - a test sheet for svg.h animations (.cen pivots + .ami tracks)
//
//   cd examples/claude/svganim && Scinc svganim.cpp
//
// Each row draws one animated test SVG from data/ (one feature per row) at
// fixed times t=0, 0.25 .. 3.0, so the whole curve of a track can be read
// off at a glance; the last column plays it live. The sheet is also saved
// to svganim.png on start.
//
// The test sprite (data/arm.svg) is an orange arm with a yellow tip, hinged
// at the grey hub; the thin outline is where the arm rests. data/arm.cen
// puts the arm's pivot on the hub. Rows 16-17 use data/anon.svg: three
// groups without an id (red, green, blue bars, top to bottom) and a yellow
// rect without an id, to show how elements without an id are named.
//
// The animation format, as used by games/bear_bees and graphic/SVG:
//   <target entity="<svg id>" param="rotation|x|y|scale_x|scale_y|hue|
//           R*|G*|B*|A+|t" interpolation="linear|sin|spline"
//           timeline="mirror|repeat|extra" times="..." keys="..."/>

#include <stdio.h>

#define G_SCREEN_WIDTH 1080
#define G_SCREEN_HEIGHT 1000
#define G_SCREEN_SCALE 1
#define G_SCREEN_MODE 1

#include "graphics.h"
#include "svg.h"
#include "../../ws.h"
#include "../../graphic/font.h"
#include "../../graphic/letters.h"
#include "../../graphic/gtext.h"

Graph g;
CFont* pfont=0;

#define NR 18
#define NCOL 13          // t = 0, 0.25 .. 3.0
#define CELL 52
#define LBL_W 330
#define TOP 44
#define LIVE_X (LBL_W+NCOL*CELL+16)

int spr[NR];
int anim[NR];
char* label[NR];

void Text(char* s, float x, float y, float sc, int c)
{
	g.clear();
	DrawText(g,*pfont,s,0,x,y,sc,sc);
	g.fin();
	g.rgba32(c);
	g.width(1.1,2.);
	g.stroke();
}

// kind: 0 arm.svg, 1 arm_use.svg, 2 anon.svg; cen: 0 the svg's own .cen,
// 1 arm_off.cen (offset 20,0), 2 arm_frame.cen (pivot frame turned 90deg)
void Row(int i, char* text, int kind, int cen)
{
	char* svg="data/arm.svg";
	char* cf="data/arm.cen";
	if(kind==1)
	{
		svg="data/arm_use.svg";
		cf="data/arm_use.cen";
	}
	if(kind==2)
	{
		svg="data/anon.svg";
		cf="data/anon.cen";
	}
	if(cen==1)cf="data/arm_off.cen";
	if(cen==2)cf="data/arm_frame.cen";
	char ami[32];
	snprintf(ami,31,"data/r%i%i.ami",(i+1)/10,(i+1)%10);
	spr[i]=SVGLoad(svg,cf);
	anim[i]=SVGLoadAnimation(spr[i],ami);
	label[i]=text;
}

void Setup()
{
	Row(0,"rotation linear mirror  0..1 -> 0..90",0,0);
	Row(1,"rotation sin mirror  0..1 -> 0..90",0,0);
	Row(2,"rotation spline repeat  0,.5,1 -> 0,90,0",0,0);
	Row(3,"rotation linear repeat  0..1 -> 0..90",0,0);
	Row(4,"rotation linear extra  0..1 -> 0..90",0,0);
	Row(5,"rotation sin mirror  1..1.5 -> 0..90",0,0);
	Row(6,"x 0..-20, y 0..20  linear mirror",0,0);
	Row(7,"scale_x 1..0.5, scale_y 1..2.5",0,0);
	Row(8,"hue linear extra  hue = t",0,0);
	Row(9,"R* .2  G* 1  B* 1  (extra, one key)",0,0);
	Row(10,"A+ -0.6  (extra, one key)",0,0);
	Row(11,"row 1 + t extra  0,1 -> 0,.5",0,0);
	Row(12,"<use> clone: t extra .5,1.5->0,1  R* .3",1,0);
	Row(13,"row 1 with .cen offset 20,0",0,1);
	Row(14,"x 0..-20, .cen pivot frame 0,1,-1,0",0,2);
	Row(15,"anon: *G1 rotation, *R0 A+ -0.7",2,0);
	Row(16,"anon: *G4 rotation, *P2 A+ -0.7",2,0);
	Row(17,"anon: *G0 rotation 0..1 -> 0..0.5",2,0);
}

void Cell(int r, float x, float t)
{
	float y=TOP+r*CELL;
	float s=(CELL-4)/100.0;
	SVGSetTransform(x+2,y+2,s,0,0,s);
	SVGDrawAnimated(spr[r],anim[r],t);
}

void DrawSheet()
{
	g.rgba32(0xff10131c);
	g.FillRT();
	char s[32];
	Text("t =",LBL_W-40,TOP-14,1.3,0xff8890b0);
	for(int c=0;c<NCOL;c++)
	{
		snprintf(s,31,"%g",c*0.25);
		int col=0xff8890b0;
		if(c%4==0)col=0xffffe090;   // whole seconds
		Text(s,LBL_W+c*CELL+8,TOP-14,1.3,col);
	}
	Text("live",LIVE_X+8,TOP-14,1.3,0xffffe090);
	for(int r=0;r<NR;r++)
	{
		snprintf(s,31,"%i",r+1);
		Text(s,8,TOP+r*CELL+31,1.3,0xff8890b0);
		Text(label[r],34,TOP+r*CELL+31,1.3,0xffe0e4f0);
		for(int c=0;c<NCOL;c++)Cell(r,LBL_W+c*CELL,c*0.25);
	}
}

void DrawLive(float t)
{
	float x=LIVE_X;
	float y=TOP-30;
	float w=CELL+8;
	float h=NR*CELL+40;
	g.rgba32(0xff10131c);
	g.fillrect(x,y,w,h);
	char s[16];
	snprintf(s,15,"%.2f",t);
	Text(s,LIVE_X+8,TOP-14,1.3,0xffffe090);
	for(int r=0;r<NR;r++)Cell(r,LIVE_X,t);
}

int main()
{
	pfont=(CFont*)malloc(sizeof(CFont));
	CFontMaker fm;
	fm.Init(*pfont,6);
	MakeLetters(fm);
	Setup();

	DrawSheet();
	DrawLive(0);
	g.WriteImage("svganim.png","png","");

	// the sheet stays; only the live column is redrawn
	float T0=Time();
	while(true)
	{
		float t=Time()-T0;
		t=t-int(t/3)*3;
		DrawLive(t);
		Present();
		Wait(0.03);
	}
	return 0;
}
