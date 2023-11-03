
#define G_SCREEN_WIDTH 320
#define G_SCREEN_HEIGHT 320
#define G_SCREEN_SCALE 3
#define G_SCREEN_MODE 1

#define M_PI 3.141592654

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

CWinSys ws;

int main()
{
	int win0=ws.CreateWindow(320,320,2,2,1);
	int win1=ws.CreateWindow(320,320,1,1,1);
	while(true)
	{
		SScincEvent ev;
		while(GetScincEvent(ev))
		{
			//printf("w=%i\n", ev._w);
		}

		int rt0=ws.GetWindowRT(win0);
		g.SetActiveRT(rt0);
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
		int rt1=ws.GetWindowRT(win1);
		g.SetActiveRT(rt1);
		g.rgba32(0xffa0a020);
		g.FillRT();
		g.clear();
		#define N 800
		for(int i=0;i<N;i++)
		{
			float x=(cos(i*M_PI/N*2.*5+T*1.1)*.8+sin(i*M_PI/N*2.*29-T*0.12)*.2)*160+160;
			float y=(sin(i*M_PI/N*2.*4+T*.93)*.8+cos(i*M_PI/N*2.*27-T*0.37)*.2)*160+160;
			g.L(x,y);
		}
		g.close();
		g.fin();
		g.alpha(1);
		g.rgba32(0xffa000a0);
		g.fill2();
		ws.Present(win0);
		ws.Present(win1);
	}
	return 0;
}