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
	float a=0.5522;
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

int main()
{
	int seed=348577;
	int mode=0;
	for(;;)
	{
		//g.M(0,0);g.l(640,0);g.l(0,480);g.l(-640,0);g.close();g.fin();
		g.clear();
		g.gray(0);
		g.M(0,0);g.l(640,0);g.l(0,480);g.l(-640,0);g.close();g.fin();
		g.fill1();
		g.clear();

		for(int i=0;i<500;i++)
		{
			g.clear();
			float x=20+frand(seed)*600.;
			float y=20+frand(seed)*440.;
			float R=(1.+frand(seed))*25.;
			Circle(x,y,R);
			g.fin();
			g.width(1.5,1.);
			float cr=frand(seed);
			float cg=frand(seed);
			float cb=frand(seed);
			g.rgb(cr,cg,cb);
			if((mode>>6)&1)
				g.fill1();
			else
				g.stroke();
		}
		mode++;
		Present();
	}
	return 0;
}