#include <stdio.h>

#include "graphics.h"

Graph g;

#define M_PI 3.141592654

float dot(float x0, float y0, float x1, float y1)
{
	return x0*x1+y0*y1;
}

float datable[32];

void CalcTable(float T)
{
	for(int s=0;s<32;s++)
		datable[s]=1.8+sin(T*0.021)*0.1+.41*sin(float(s)*.71+T*0.02);
}

void wave(float& x, float& y, float& z, double T, int octaves)
{
	float R=8.;
	float S=.03;
	float W=-.05;
	#define RRRRS R*=.72;S*=1.27;W*=-1.21;
	for(int s=0;s<octaves;s++)
	{
		float da=datable[s];
		float dx=cos(da);
		float dy=sin(da);
		float t=dot(x-320.,y-240,dx,dy);
		float a=T*W+t*S;
		float sa=sin(a)*R;
		float ca=cos(a)*R;

		x-=ca*dx;
		y-=ca*dy;
		z-=sa;
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
		CalcTable(T);
		{
			for(int i=0;i<480;i+=8)
			{
				for(int j=0;j<640;j+=8)
				{
					float x=j;
					float y=i;
					float z=0.;
					wave(x,y,z,T-3.,12);
					float r=(-z+16.)*.023;
					//if(r<1.)
					{
						int h=r*191+64;
						int c=0xff000000|int(h)|(int(h*.5)<<8)|(int(h*.5+30)<<16);
						g.Rect(j,i,8,8,c);
					}
				}
			}
		}
		Present();
	}
	return 0;
}