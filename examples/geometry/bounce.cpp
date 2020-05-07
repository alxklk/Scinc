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
		M(0,0);l(640,0);l(0,480);l(-640,0);close();fin();rgba(0.25,0.15,0.1,1.);fill1();
		int seed=348577;
		t+=1./60.;
		for(int i=0;i<100;i++)
		{
			float y=2000+frand(seed)*440.+t*(.1+frand(seed))*150.;
			float x=2000+frand(seed)*600.+t*(.1+frand(seed))*150.;

			float R=(1.+frand(seed))*10.;
			float dx=640.-R*2.;
			float dy=480.-R*2.;
			x=abs(mod(x,dx*2.)-dx);
			y=abs(mod(y,dy*2.)-dy);

			float r=.5+.5*sin(t*frand(seed)*6.+15.);
			float g=.5+.5*sin(t*frand(seed)*6.+15.);
			float b=.5+.5*sin(t*frand(seed)*6.+15.);

			clear();
			Circle(x+R,y+R,R-5);
			fin();
			width(10.,1.);
			gray(0);
			fill2();

			clear();
			Circle(x+R,y+R,R);
			fin();
			width(1.,1.);
			rgb(r*.3,g*.3,b*.3);
			fill2();

			clear();
			float d=0.85;
			Circle(x+R*d,y+R*d,R*.3);
			fin();
			width(R*.5,1.);
			//stroke();
			rgb(.5+r*.5,.5+g*.5,.5+b*.5);
			fill2();
		}
		mode++;
		Present();
	}
	return 0;
}