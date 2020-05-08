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
	M(x+r,y);
	float a=0.55;
	C(x+r,y+r*a,x+r*a,y+r,x,y+r);
	C(x-r*a,y+r,x-r,y+r*a,x-r,y);
	C(x-r,y-r*a,x-r*a,y-r,x,y-r);
	C(x+r*a,y-r,x+r,y-r*a,x+r,y);
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
		M(0,0);l(640,0);l(0,480);l(-640,0);close();fin();rgba(0,0,0,1);fill1();
		alpha(1);
		for(int i=0;i<500;i++)
		{
			clear();
			float x=20+frand(seed)*600.;
			float y=20+frand(seed)*440.;
			float R=(1.+frand(seed))*25.;
			Circle(x,y,R);
			fin();
			width(1.5,1.);
			float r=frand(seed);
			float g=frand(seed);
			float b=frand(seed);
			rgb(r,g,b);
			if((mode>>6)&1)
				fill1();
			else
				stroke();
		}
		mode++;
		Present();
	}
	return 0;
}