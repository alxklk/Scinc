#include <stdio.h>

#include "graphics.h"

Graph g;

#define M_PI 3.141592654

void RoundRect(float x, float y, float w, float h, float r)
{
	g.M(x,y+r);
	if(r>0)g.a(r,r,0,0,1,r,-r);
	g.l(w-r*2,0);
	if(r>0)g.a(r,r,0,0,1,r,r);
	g.l(0,h-r*2);
	if(r>0)g.a(r,r,0,0,1,-r,r);
	g.l(-w+r*2,0);
	if(r>0)g.a(r,r,0,0,1,-r,-r);
	g.close();
}


int main()
{
	while(true)
	{
		double T=Time()*.1;
		g.rgba32(0xffffffff);
		g.FillRT();
		g.t_0(0,0);

		//if(0)
		{
		g.clear();
		RoundRect(100,110,250,150,20);
		g.fin();
		g.rgba32(0x80000000);
		g.width(12,1.5);
		g.stroke();

		g.clear();
		RoundRect(100,100,250,150,20);
		g.fin();
		g.rgba32(0xff3070c0);
		g.width(10,10);
		g.fill2();
		g.rgba32(0xff60a0ff);
		g.width(10,.75);
		g.fill2();

		g.clear();
		RoundRect(115,110,220,8,4);
		g.fin();
		g.rgba32(0xc0ffffff);
		g.width(3,.5);
		g.fill2();

		g.clear();
		RoundRect(125,190,200,30,15);
		g.fin();
		g.rgba32(0xff90d0ff);
		g.width(25,.5);
		g.fill2();
		}

		g.clear();

		g.graddef(0);
		g.gradstop(1,0,0,0,0);
		g.gradstop(0,0,0,0,1);
		g.gradtype(1);
		g.gradmethod(2);
		g.gradend();
		g.g_t(150,400,15,0,0,15);

		g.graduse(0);

		g.M(100,350);
		g.l(100,0);
		g.a(50,50,180,0,0,100,-50);
		g.fin();
		g.rgba32(0xff000000);
		g.width(25,1);
		g.stroke();
		g.fill1();
		g.graduse(-1);


		Present();
	}
	return 0;
}