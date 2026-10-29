// sheet.cpp - every sprite through its animation cycle, saved to sheet.png
//
//   Scinc sheet.cpp
//
// A quick way to check the SVGs and their .cen/.ami files after editing
// them: one row per sprite, the cycle at u = 0, 0.1 .. 1 (u=0 and u=1 must
// both look like the resting sprite), and a last column that plays the
// cycle live. The window stays open; only the live column is redrawn.
#define G_SCREEN_WIDTH 1040
#define G_SCREEN_HEIGHT 820
#define G_SCREEN_SCALE 1
#define G_SCREEN_MODE 1
#include <stdio.h>
#include "graphics.h"
#include "svg.h"
#include "../../ws.h"

#define NS 10
#define NCOL 11
#define CELL 80
#define LIVE_X (NCOL*CELL+40)

Graph g;
int spr[NS];
int anim[NS];

void Load(int i, char* name)
{
	char svg[48];
	char cen[48];
	char ami[48];
	snprintf(svg,47,"sprites/%s.svg",name);
	snprintf(cen,47,"sprites/%s.cen",name);
	snprintf(ami,47,"sprites/%s.ami",name);
	spr[i]=SVGLoad(svg,cen);
	anim[i]=SVGLoadAnimation(spr[i],ami);
}

void Cell(int r, float x, float u)
{
	float y=r*CELL+4;
	g.rgba32(0xff1a2250);
	g.fillrect(x+2,y+2,CELL-4,CELL-4);
	float s=(CELL-8)/100.0;
	SVGSetTransform(x+4,y+4,s,0,0,s);
	SVGDrawAnimated(spr[r],anim[r],u);
}

int main()
{
	Load(0,"dust");
	Load(1,"spark");
	Load(2,"star");
	Load(3,"twin");
	Load(4,"constellation");
	Load(5,"nebula");
	Load(6,"galaxy");
	Load(7,"quasar");
	Load(8,"comet");
	Load(9,"blackhole");

	g.rgba32(0xff0b1030);
	g.FillRT();
	for(int r=0;r<NS;r++)
	{
		for(int c=0;c<NCOL;c++)Cell(r,c*CELL,c*0.1);
		Cell(r,LIVE_X,0);
	}
	g.WriteImage("sheet.png","png","");

	float T0=Time();
	while(true)
	{
		float u=(Time()-T0)/1.2;
		u=u-int(u);
		for(int r=0;r<NS;r++)Cell(r,LIVE_X,u);
		Present();
		Wait(0.03);
	}
	return 0;
}
