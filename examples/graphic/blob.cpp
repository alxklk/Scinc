#include <stdio.h>

#include "graphics.h"

Graph g;

#define M_PI 3.141592654

void Spiral(float x, float y, float r, int N, float da, float a0)
{
	g.M(x-r,y);
	g.l(r*2,0);
}

int main()
{
	while(true)
	{
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

		Present();
	}
	return 0;
}