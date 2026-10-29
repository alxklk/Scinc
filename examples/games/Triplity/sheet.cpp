// sheet.cpp - renders every sprite on game-like tiles into sheet.png
// (a quick way to check the SVGs after editing them)
#define G_SCREEN_WIDTH 1000
#define G_SCREEN_HEIGHT 260
#include <stdio.h>
#include "graphics.h"
#include "svg.h"
#include "../../ws.h"
Graph g;
int main()
{
	char* names[10];
	names[0]="sprites/dust.svg";names[1]="sprites/spark.svg";names[2]="sprites/star.svg";
	names[3]="sprites/twin.svg";names[4]="sprites/constellation.svg";names[5]="sprites/nebula.svg";
	names[6]="sprites/galaxy.svg";names[7]="sprites/quasar.svg";names[8]="sprites/comet.svg";
	names[9]="sprites/blackhole.svg";
	g.rgba32(0xff0b1030);
	g.FillRT();
	for(int i=0;i<10;i++)
	{
		int id=SVGLoad(names[i]);
		float x=i*100;
		g.rgba32(0xff1a2250);
		g.fillrect(x+4,4,92,92);
		SVGSetTransform(x+4,4,0.92,0,0,0.92);
		SVGDraw(id);
		g.rgba32(0xff1a2250);
		g.fillrect(x+4,104,92,152);
		SVGSetTransform(x+0,120,1.0,0,0,1.0);
		SVGDraw(id);
	}
	Present();
	g.WriteImage("sheet.png","png","");
	return 0;
}
