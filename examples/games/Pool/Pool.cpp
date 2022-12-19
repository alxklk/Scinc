#include "BallPhysics.h"

#define G_SCREEN_WIDTH 1280	
#define G_SCREEN_HEIGHT 720
#define G_SCREEN_SCALE 2

#include "graphics.h"

Graph g;
int main()
{
	puts("Fish there!\n");
	BallPhys ph;
	ph.Reset();
	int floor_level=G_SCREEN_HEIGHT-10;
	while(true)
	{
		float T=Time();
		g.rgba32(0xff000040);
		g.FillRT();
		g.fillrect(0,floor_level-500,500,500,0xff606040);
		int mx;
		int my;
		int mb;
		GetMouseState(mx,my,mb);
		if(mb&1)
		{
			ph.nodes[0].xf.x=-(ph.nodes[0].p.x-mx)*2000;
			ph.nodes[0].xf.y=-(ph.nodes[0].p.y-(floor_level-my))*2000;
		}
		else
		{
			ph.nodes[0].xf.Zero();
		}
		for(int i=0;i<2;i++)
		{
			ph.Step(0.001);
		}
		g.rgba32(0xffffffff);
		g.clear();

		for(int i=0;i<ph.NNodes;i++)
		{
			PH_Node& n0=ph.nodes[i];
			PH_Num3 p0=n0.p;
			{
				g.M(p0.x,floor_level-p0.y);
				g.l(.1,.1);
			}
		}
		g.fin();
		g.width(10,10);
		g.stroke();
		Present();
	}
	return 0;
}

