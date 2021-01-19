#define G_SCREEN_WIDTH 512
#define G_SCREEN_HEIGHT 512

#include "graphics.h"

Graph g;

#define M_PI 3.141592654

int main()
{
	double T=0;
	while(true)
	{
		T+=0.03;
		g.rgba32(0xff000000);
		g.FillRT();
		
		int N=24;
		float size=160;
		for(int i=0;i<N;i++)
		{
			g.clear();
			g.M(256,256);
			float i0=(i)/(float)N;
			float i1=(i+1)/(float)N;
			//g.L(320+cos(i0*2*M_PI)*200,240+sin(i0*2*M_PI)*200);
			g.A(size/3,size/2,i0*360-70,0,1,256+cos(i0*2*M_PI)*size,256+sin(i0*2*M_PI)*size);
			//g.L(320+cos(i1*2*M_PI)*200,240+sin(i1*2*M_PI)*200);
			g.A(size,size,0,0,1,256+cos(i1*2*M_PI)*size,256+sin(i1*2*M_PI)*size);
			g.A(size/3,size/2,i1*360-70,0,0,256,256);
			//g.close();
			g.fin();
			g.alpha(1);
			float a=i0*M_PI*2.;
			g.rgb(sin(a*4+T*2.1+17)*.33+.67,sin(a*3+T*3.3+11)*.33+.67,sin(a*5+T*3.1+25)*.33+.67);
			g.fill1();
			g.alpha(.75);
			//g.rgb(1,1,1);
			g.width(1,1);
			g.stroke();
		}
		Present();
	}
	return 0;
}