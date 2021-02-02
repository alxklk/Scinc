
#define G_SCREEN_WIDTH 640
#define G_SCREEN_HEIGHT 480
#define G_SCREEN_SCALE 2
#define G_SCREEN_MODE 1

#include "graphics.h"

Graph g;

double T;

float fx(float x)
{
	return (cos(x*13+T)*300*sin(T*1.1)+cos(x*27.5-T*2.3)*250*sin(T*.9)+sin(x*3.1)*150*sin(T*.7))*.25;
}

float fy(float x)
{
	return (sin(x*13+T)*300*sin(T*1.1)+sin(x*27.5-T*.3)*250*sin(T*.9)+cos(x*3.1)*150*sin(T*.7))*.25;
}

int main()
{
	while(true)
	{
		g.rgba32(0xff203040);
		g.FillRT();
		T=Time();
		float a=0;
		float cx=G_SCREEN_WIDTH/2;
		float cy =G_SCREEN_HEIGHT/2;
		float x0=cx+fx(a);
		float y0=cy+fy(a);
		for(int i=0;i<2500;i++)
		{
			g.rgb(.25+.25*cos(a*13*5),.75+.25*sin(a*13*4),.25+.25*sin(a*13*3));
			a+=0.005;
			float x1=cx+fx(a);
			float y1=cy+fy(a);
			g.hairline(x0,y0,x1,y1);
			x0=x1;
			y0=y1;
		}
		Present();
	}
	return 0;
}