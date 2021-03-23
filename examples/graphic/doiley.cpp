#include <stdio.h>

#define G_SCREEN_WIDTH 1024
#define G_SCREEN_HEIGHT 1024
#define G_SCREEN_SCALE 1

#include "graphics.h"


Graph g;

#define M_PI 3.141592654

float Fsign(float x){return x<0?-1:1;}
float Fabs(float x){return x<0?-x:x;}

void doiley(float x, float y, float a, float b, float c, int N)
{
	int i=0;

	while(true)
	{
		g.clear();
		for(int j=0;j<10000;j++)
		{
			float x0=x;
			float y0=y;
			x=y0-Fsign(x0)*sqrt(Fabs(b*x0-c));
			y=a-x0;
			g.M(512+x*3,512+y*3);
			i++;
		}
		g.fin();
		g.width(2,1);
		float t=i/(float)N;
		g.rgb(t,t*t,1-t);
		g.alpha(.5);
		g.stroke();
		if(i>N)
			break;
		Present();
	}
}

void CLS()
{
	g.rgba(.1,.05,0,1);g.FillRT();
}

int main()
{
	while(1)
	{
		CLS();
		doiley(5, 5, 30.5, 2.5, 2.5, 500000);
		g.WriteImage("doiley1.png", "png", "");
		CLS();
		doiley(5, 6, 30, 3, 3, 500000);
		g.WriteImage("doiley2.png", "png", "");
		CLS();
		doiley(3, 4, 5, 6, 7, 500000);
		g.WriteImage("doiley3.png", "png", "");
	}
	return 0;
}