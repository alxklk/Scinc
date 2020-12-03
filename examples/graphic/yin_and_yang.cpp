#include <stdio.h>
#include "graphics.h"

Graph g;

#define M_PI 3.141592654

int main()
{
	while(true)
	{
		double cx=320;
		double cy=240;
		double R=100;
		double r=20;
		g.clear();
		g.gray(.5);
		g.fill1();
		g.M(cx,cy-R);
		//int N=128;
		//for(int i=1;i<N;i++)
		//{
		//	float a=2*M_PI*i/N;
		//	g.L(cx+sin(a)*R, cy-cos(a)*R);
		//}
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

		Present();
	}
	return 0;
}