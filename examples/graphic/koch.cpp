#include "graphics.h"
#define G_SCREEN_MODE 2

#include "../ws.h"

Graph g;

void Rec0(float s, float x, float y, int depth)
{
	if(depth==8)
	{
		g.l(x,y);
	}
	else
	{
		float dx=x/3.;
		float dy=y/3.;
		Rec0(s,dx-dy*s,dy+dx*s,depth+1);
		Rec0(s,dx+dy*s*2.,dy-dx*s*2.,depth+1);
		Rec0(s,dx-dy*s,dy+dx*s,depth+1);
	}
}

int main()
{
	while(true)
	{
		g.rgba32(0xff000000);
		g.FillRT();
		g.clear();
		g.M(20,480);
		g.l(0,-240);
		Rec0(.8*sin(Time()),600,0,0);
		g.l(0,240);
		g.close();
		g.fin();
		g.rgba32(0xff808080);
		g.fill1();
		g.rgba32(0xffffffff);
		g.stroke();
		Present();
	}
	return 0;
}