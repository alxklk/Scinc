#include <stdio.h>
#include "graphics.h"

#define M_PI 3.141592654

float Fabs(float x)
{
	if(x<0)
		return -x;
	else
		return x;
}

Graph g;

void Circle(float x, float y, float r)
{
	g.M(x+r,y);
	float a=0.55;
	g.C(x+r,y+r*a,x+r*a,y+r,x,y+r);
	g.C(x-r*a,y+r,x-r,y+r*a,x-r,y);
	g.C(x-r,y-r*a,x-r*a,y-r,x,y-r);
	g.C(x+r*a,y-r,x+r,y-r*a,x+r,y);
}

int irand(int& seed)
{
	seed=(seed*1103515245+12345)&0x7ffffff;
	return seed;
}

float frand(int& seed)
{
	return (irand(seed))/134217727.0;
}

float mod(float a, float b)
{
	int r=a/b;
	return a-b*r;
}

int main()
{
	int mode=0;
	float t=10.;
	for(;;)
	{
		//alpha(.15);
		g.M(0,0);g.l(640,0);g.l(0,480);g.l(-640,0);g.close();g.fin();g.rgba(0.25,0.15,0.1,1.);g.fill1();
		int seed=348577;
		t+=1./60.;
		for(int i=0;i<100;i++)
		{
			float y=2000+frand(seed)*440.+t*(.1+frand(seed))*150.;
			float x=2000+frand(seed)*600.+t*(.1+frand(seed))*150.;

			float R=(1.+frand(seed))*10.;
			float dx=640.-R*2.;
			float dy=480.-R*2.;
			x=Fabs(mod(x,dx*2.)-dx);
			y=Fabs(mod(y,dy*2.)-dy);

			float cr=.5+.5*sin(t*frand(seed)*6.+15.);
			float cg=.5+.5*sin(t*frand(seed)*6.+15.);
			float cb=.5+.5*sin(t*frand(seed)*6.+15.);

			g.clear();
			Circle(x+R,y+R,R-5);
			g.fin();
			g.width(10.,1.);
			g.gray(0);
			g.fill2();

			g.clear();
			Circle(x+R,y+R,R);
			g.fin();
			g.width(1.,1.);
			g.rgb(cr*.3,cg*.3,cb*.3);
			g.fill2();

			g.clear();
			float d=0.85;
			Circle(x+R*d,y+R*d,R*.1);
			g.fin();
			g.width(R*.9,1.);
			//g.stroke();
			g.rgb(.5+cr*.5,.5+cg*.5,.5+cb*.5);
			g.fill2();
		}
		mode++;
		Present();
	}
	return 0;
}