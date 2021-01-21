#include <stdio.h>

#include "graphics.h"

Graph g;

#define M_PI 3.141592654

void Spiral(float x, float y, float r, int N, float da, float a0)
{
	g.M(x, y);
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
		g.rgba32(0xffffd080);
		g.FillRT();
		// g.clear();
		// Spiral(320,240,15,180*8,M_PI/120,T*6+M_PI);
		// g.fin();
		// g.rgba32(0xffffd080);
		// g.width(10,10);
		// g.stroke();
		g.clear();
		Spiral(320,240,15,180*8,M_PI/120,T*6);
		g.fin();
		g.rgba32(0xfffff0d0);
		g.width(37,37);
		g.stroke();

		g.rgba32(0xff003060);
		g.width(25,25);
		g.stroke();
		g.rgba32(0xff007090);
		g.width(10,10);
		g.stroke();
		Present();
	}
	return 0;
}