#include <stdio.h>

#define G_SCREEN_SCALE 2

#include "graphics.h"

#define M_PI 3.141592654

Graph g;

float t;

int main()
{
	for(;;)
	{
		t=Time();
		g.rgb(0.3,.15,.1);
		g.FillRT();
		for(int i=0;i<200;i++)
		{
			g.clear();
			float dy=cos(t+i);
			g.M(320+sin(t+i)*i*.5*(1+sin(i*.15)*.2), 50+i*2+dy*i*.2);
			float cr=.6+sin(i*4+t*3  )*.4;
			float cg=.6+sin(i*7+t*2  )*.4;
			float cb=.6+sin(i*9+t*2.5)*.4;

			g.rgb(cr,cg,cb);
			g.fin();
			g.width(12,1);
			float a=.6+.4*dy;
			g.alpha(a*.2);
			g.stroke();
			g.alpha(a);
			g.width(3,3);
			g.stroke();
		}
		Present();
	}
	return 0;
}