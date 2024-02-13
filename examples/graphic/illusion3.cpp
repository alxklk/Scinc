#include <stdio.h>
#include "graphics.h"
#include "../ws.h"

Graph g;

#define M_PI 3.141592654

void Ngon(float x, float y, float r, int N, float a0)
{
	g.M(x+cos(a0)*r, y+sin(a0)*r);
	for(int i=0;i<N;i++)
	{
		float a=M_PI*2.*i/N+a0;
		g.L(x+cos(a)*r,y+sin(a)*r);
	}
	g.close();
}

void Ng1(float s)
{
	g.clear();
	Ngon(320,240,100.0*s,8,0);
	Ngon(320,240,100.0*s,8,M_PI/8);
	Ngon(320,240,108.3*s,8,0);
	Ngon(320,240,108.3*s,8,M_PI/8);
	g.fin();
	g.rgba32(0xff000000);
	g.width(1+1.5*s,s);
	g.stroke();
}

int main()
{
	while(true)
	{
		double T=Time();
		g.rgba32(0xff808080);
		g.FillRT();
		for(float i=0.1;i<1.1;i+=0.13)Ng1(.2+i*i*1.75);
		Present();
	}
	return 0;
}