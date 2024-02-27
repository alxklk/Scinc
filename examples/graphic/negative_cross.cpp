#include <stdio.h>
#define G_SCREEN_WIDTH 512
#define G_SCREEN_HEIGHT 512
#define G_SCREEN_SCALE 1
#include "graphics.h"
#include "../ws.h"

#define M_PI 3.141592654

Graph g;

void Rect(float x, float y, float dx, float dy)
{
	g.M(x-dx/2+dy/2,y-dy/2-dx/2);
	g.l(dx,dy);
	g.l(-dy,dx);
	g.l(-dx,-dy);
	g.close();
}

int main()
{
	int n=0;
	while(true)
	{
		float T=sin(Time())*M_PI*.5;
		g.t_0(0,0);
		g.t_x(1,0);
		g.t_y(0,1);
		g.clear();
		g.gray(1);
		g.FillRT();
		g.clear();
		float L=120;
		float L1=90;
		g.M(256-L,256-L);
		g.M(256+L,256-L);
		g.M(256-L,256+L);
		g.M(256+L,256+L);
		g.fin();
		g.gray(0);
		g.width(L1,L1);
		g.stroke();
		g.clear();
		g.gray(1);
		//T=1;
		g.t_0(256,256);
		float tt=(sin(T)*.5+.5)*M_PI*.5;
		g.t_x(cos(tt), sin(tt));
		g.t_y(sin(tt),-cos(tt));
		float r=75;
		float r1=100;
		float t0=(sin(T)*.125+.125+.25)*M_PI;
		float t1=t0-M_PI/2;
		float t2=t0-M_PI*3/2;
		float t3=t0-M_PI;
	
		Rect(-r,-r,sin(t0)*r1,cos(t0)*r1);
		Rect(+r,-r,sin(t1)*r1,cos(t1)*r1);
		Rect(-r,+r,sin(t2)*r1,cos(t2)*r1);
		Rect(+r,+r,sin(t3)*r1,cos(t3)*r1);
		g.fin();
		g.fill1();
		//g.clear();
		//g.rgba(1,1,0,.5);
		//t0=(-1*.125+.125+.25)*M_PI;
		//t1=t0-M_PI/2;
		//t2=t0-M_PI*3/2;
		//t3=t0-M_PI;
		//Rect(256-r,256-r,sin(t0)*r1,cos(t0)*r1);
		//Rect(256+r,256-r,sin(t1)*r1,cos(t1)*r1);
		//Rect(256-r,256+r,sin(t2)*r1,cos(t2)*r1);
		//Rect(256+r,256+r,sin(t3)*r1,cos(t3)*r1);
		//g.fin();
		//g.fill1();
		Present();
	}
	return 0;
}