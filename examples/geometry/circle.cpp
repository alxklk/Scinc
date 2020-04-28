#include <stdio.h>
#include "graphics.h"

#define M_PI 3.141592654

float abs(float x)
{
	if(x<0)
		return -x;
	else
		return x;
}

void Circle(float x, float y, float r)
{
	int steps=10+r;
	float step=1./steps;
	M(x+r,y);
	for(int i=1;i<=steps;i++)
	{
		float angle=i*step*2.*M_PI;
		L(x+cos(angle)*r,y+sin(angle)*r);
	}
}

int irand(int& seed)
{
	seed=(seed*1103515245+12345)%0x7ffffff;
	return seed;
}

float frand(int& seed)
{
	return (irand(seed)>>11)/65536.0;
}

int main()
{
	int seed=32348577;
	for(;;)
	{
		M(0,0);l(640,0);l(0,480);l(-640,0);close();fin();rgba(0,0,0,.025);fill1();
		alpha(.5);
		for(int i=0;i<50;i++)
		{
			clear();
			float x=320+frand(seed)*300.;
			float y=240+frand(seed)*220.;
			float R=(1.+frand(seed))*10.;
			Circle(x,y,R);
			fin();
			width(4.,4.);
			float r=frand(seed);
			float g=frand(seed);
			float b=frand(seed);
			rgb(r,g,b);
			fill2();
		}
		Present();
	}
	return 0;
}