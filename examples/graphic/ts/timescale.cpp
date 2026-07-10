
#define G_SCREEN_WIDTH 1280
#define G_SCREEN_HEIGHT 720
#define G_SCREEN_SCALE 1
#define G_SCREEN_MODE 3

#include "graphics.h"
#include "../../ws.h"

Graph g;
#define M_PI 3.1415926535897932384626433832795

void Clock(int x, int y, int size, double t)
{
	for(int i=0;i<4;i++)
	{
		double a=i/4.0*2*M_PI;
		g.M(x+sin(a)*size*.5     ,y+cos(a)*size*.5);
		g.L(x+sin(a)*size*0.52,y+cos(a)*size*0.52);
	}

	for(int i=0;i<100;i++)
	{
		double a=i/100.0*2*M_PI;
		g.M(x+sin(a)*size     ,y+cos(a)*size);
		g.L(x+sin(a)*size*0.975,y+cos(a)*size*0.975);
	}
	for(int i=0;i<10;i++)
	{
		double a=i/10.0*2*M_PI;
		g.M(x+sin(a)*size     ,y+cos(a)*size);
		g.L(x+sin(a)*size*0.75,y+cos(a)*size*0.75);
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
	double T=0;
	int i=0;

	while(true)
	{
		T=Time();
		T=i/120.0;
		g.rgba32(0xff708090);
		g.FillRT();

		double t0=fract(T);

		g.rgba32(0x80000000);
		g.width(12,1);
		g.clear();
		Clock(645,365,340,t0);
		g.fin();
		g.stroke();

		g.rgba32(0xffffffff);
		g.width(6,6);
		g.clear();
		Clock(640,360,340,t0);
		g.fin();
		g.stroke();
		//Wait(1./125.-(Time()-T));
		Present();
		{
			char fn[64];
			snprintf(fn,64,"ts%05i.jpg",i);
			//g.WriteImage(fn, "jpg", "");
		}
		i++;
		//if(i>60)break;
	}
	return 0;
}