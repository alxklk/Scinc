#include <stdio.h>

#include "graphics.h"
#include "../ws.h"

Graph g;

#define M_PI 3.141592654

void wave(float& x, float& y, double T, float i, int octaves)
{
	float R=8.;
	float S=.025;
	float W=-.1;
	#define RRRRS R*=.51;S*=2.07;W*=-1.23;
	for(int s=0;s<octaves;s++)
	{
		x+=sin(T*W+i*S)*R;
		y+=cos(T*W+i*S)*R;
		RRRRS
	}
}

int main()
{
	g.miterlim(1.0);
	while(true)
	{
		double t=Time();
		g.rgba32(0xff6080ff);
		g.FillRT();

		float T=t*25;
		//g.clear();
		{
			float x0s[5]={50,190,240,400,540};
			for(int i=0;i<5;i++)
			{
				float x=0;
				float y=0;
				float x0=x0s[i];
				wave(x,y,T-3.,x0,4);
				//g.M(x0+x,240.+y);
				g.Circle((x0+x),(240.+y),0,9,1,0xffff8000);
				g.Circle((x0+x),(240.+y),10,0,1,0xff800000);
			}
		}
		//g.fin();
		//g.rgba32(0xff800000);
		//g.width(11.,11.);
		//g.stroke();
		//g.rgba32(0xffff8000);
		//g.width(10.,10.);
		//g.stroke();

		g.clear();
		g.L(-20,500);
		for(float i=-20;i<680;i+=1.5)
		{
			float x=0;
			float y=0;
			float x0=i;
			wave(x,y,T,x0,8);
			g.L(x0+x,240.+y);
		}
		g.L(660,500);
		g.close();
		g.fin();
		g.rgba32(0x80006080);
		g.fill1();

		g.rgba32(0xc0004080);
		g.width(2.,1.);
		g.stroke();

		Present();
	}
	return 0;
}