
#define G_SCREEN_WIDTH 300
#define G_SCREEN_HEIGHT 150
#define G_SCREEN_SCALE 4
#define G_SCREEN_MODE 2

#include "graphics.h"
#include "../ws.h"

Graph g;
#define M_PI 3.1415926535897932384626433832795

void Clock(int x, int y, int size, double t)
{
	for(int i=0;i<10;i++)
	{
		double a=i/10.0*2*M_PI;
		g.M(x+sin(a)*size     ,y+cos(a)*size);
		g.L(x+sin(a)*size*0.95,y+cos(a)*size*0.95);
	}
	g.M(x,y);
	g.L(x+sin(-t*2*M_PI)*size*0.9,y+cos(-t*2*M_PI)*size*0.9);
}

float fract(float x)
{
	return x-(int)x;
}

int main()
{
	double T;

	while(true)
	{
		g.rgba32(0xff203040);
		g.FillRT();
		T=Time();

		double t0=fract(T);

		g.rgba32(0xffffffff);
		g.width(3,3);
		g.clear();

		Clock(75,75,60,t0);
		Clock(75+150,75,60,fract(t0*10));

		g.fin();
		g.stroke();
		Wait(1./125.-(Time()-T));
		Present();
	}
	return 0;
}