#include <stdio.h>

#include "graphics.h"

Graph g;

#define M_PI 3.141592654

void Spiral(float x, float y, float r, int N, float da, float a0)
{
	for(int i=0;i<N;i++)
	{
		float a=i*da;
		g.L(x+cos(a+a0)*a*r,y+sin(a+a0)*a*r);
	}
}

int main()
{
	while(true)
	{
		double T=Time();
		g.rgba32(0xffffffff);
		g.FillRT();

		g.clear();
		Spiral(330,250,15,180*8,M_PI/120,T*6);
		g.fin();
		g.rgba32(0x80c0a080);
		g.width(50,1.);
		g.stroke();

		g.rgba32(0x90000000);
		g.width(20,1.);
		g.stroke();

		g.clear();
		Spiral(320,240,15,180*8,M_PI/120,T*6);
		g.fin();
		g.rgba32(0xffc07040);
		g.width(18,18);
		g.stroke();
		g.rgba32(0xffffc040);
		g.width(17,1.5);
		g.stroke();

		g.clear();
		Spiral(315,235,15,180*8,M_PI/120,T*6);
		g.fin();
		g.rgba32(0xfffffff0);
		g.width(8,1.0);
		g.stroke();

		Present();
	}
	return 0;
}