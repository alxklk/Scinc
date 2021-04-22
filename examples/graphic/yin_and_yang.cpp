#include <stdio.h>
#define G_SCREEN_WIDTH 512
#define G_SCREEN_HEIGHT 512
#define G_SCREEN_SCALE 2
#include "graphics.h"

Graph g;

#define M_PI 3.141592654

int main()
{
	int n=0;
	while(true)
	{
		double cx=G_SCREEN_WIDTH/2;
		double cy=G_SCREEN_HEIGHT/2;
		double R=G_SCREEN_HEIGHT*(.35+.1*sin(Time()));
		double r=G_SCREEN_HEIGHT*.1*(1+0.1*sin(Time()));
		g.clear();
		g.gray(.5);
		g.FillRT();
		g.clear();
		g.M(cx,cy-R);
		g.a(R,R,0,0,0,0,2*R);
		g.a(R,R,0,0,0,0,-2*R);
		g.close();
		g.fin();
		g.gray(1);
		g.fill1();
		g.gray(0);

		g.width(1.5,1.5);
		g.stroke();


		g.clear();
		g.M(cx,cy-R);
		g.a(R,R,0,0,1,0,2*R);
		g.a(R/2,R/2,0,0,1,0,-R);
		g.a(R/2,R/2,0,0,0,0,-R);
		g.close();
		g.M(cx,cy-R/2-r);
		g.a(r,r,0,0,0,0,2*r);
		g.a(r,r,0,0,0,0,-2*r);
		g.close();
		g.M(cx,cy+R/2-r);
		g.a(r,r,0,0,0,0,2*r);
		g.a(r,r,0,0,0,0,-2*r);
		g.close();

		g.fin();

		g.gray(0);
		g.fill1();
		if(n==0)
		{
			char fn[64];
			snprintf(fn,64,"yy%05i.jpg",n);
			g.WriteImage(fn, "jpg", "");
			// g.WriteImage("yin_and_yang.png", "png", "");
			// g.WriteImage("yin_and_yang.jpg", "jpg", "");
		}
		n++;

		Present();
	}
	return 0;
}