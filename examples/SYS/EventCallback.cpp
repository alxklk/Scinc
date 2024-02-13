#include <stdio.h>
#define G_SCREEN_MODE 2
#define G_SCREEN_SCALE 1

#include "graphics.h"

Graph g;

#define M_PI 3.141592654

void Spiral(float x, float y, float r, int N, float da, float a0)
{
	g.M(x-r,y);
	g.l(r*2,0);
}

bool close=false;

int ScincEventCallback(SScincEvent ev)
{
	//printf(" Event %i %i %i %i %i\n", ev.type, ev.x,  ev.y,  ev.z,  ev.w);
	//printf(" Event %c%c%c%c\n",(ev.type&0xff000000)>>24,(ev.type&0xff0000)>>16,(ev.type&0xff00)>>8,(ev.type&0xff));
	if(ev.type=='WKIL')
	{
		close=true;
		printf("Close\n");
	}
	if(ev.type=='WCHK')
	{
		printf(" Event %c%c%c%c\n",(ev.type&0xff000000)>>24,(ev.type&0xff0000)>>16,(ev.type&0xff00)>>8,(ev.type&0xff));
		return 32;
	}
	return 1;
}

int main()
{
	SetEventCallback(&ScincEventCallback);
	while(true)
	{
		//Poll();
		double T=Time()*.01;
		g.rgba32(0xffffffff);
		g.FillRT();

		g.clear();
		Spiral(320,245,15,180*8,M_PI/120,T*6);
		g.fin();
		g.rgba32(0x90000000);
		g.width(20,2.);
		g.stroke();

		g.clear();
		Spiral(320,240,15,180*8,M_PI/120,T*6);
		g.fin();
		g.rgba32(0xffc07040);
		g.width(18,18);
		g.stroke();

		g.clear();
		Spiral(320,247,18,180*8,M_PI/120,T*6);
		g.fin();
		g.rgba32(0xc0ffa040);
		g.width(10,1.);
		g.stroke();

		g.clear();
		Spiral(320,235,18,180*8,M_PI/120,T*6);
		g.fin();
		g.rgba32(0xffffc040);
		g.width(12,1.5);
		g.stroke();


		g.clear();
		Spiral(320,232,15,180*8,M_PI/120,T*6);
		g.fin();
		g.rgba32(0xfffffff0);
		g.width(8,1.0);
		g.stroke();

		Poll();
		Present();
		if(close)
			break;
	}
	printf("Closing window\n");
	return 0;
}