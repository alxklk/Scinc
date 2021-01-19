#include <stdio.h>
#include "graphics.h"

Graph g;

#define M_PI 3.141592654

void Dot(float x, float y, float r, float w, float a, float T, float alpha)
{
	int n=r/w;
	float ca=cos(a);
	float sa=sin(a);

	for(int i=-n;i<=n;i++)
	{
		float t=i+(T-int(T)+(T<0?1:0))*2-1;
		float s=sqrt(r*r-t*t*w*w);
		g.clear();
		g.M(x-s*ca+w*t*sa,y+s*sa+w*t*ca);
		g.l(s*2*ca,-s*2*sa);
		g.fin();
		g.width(w,1);
		if(i&1)
			g.rgb(1,1,1);
		else
			g.rgb(0,0,0);
		g.alpha(s/r*alpha);
		g.stroke();
	}
}

int main()
{
	while(true)
	{
		double T=Time();
		g.clear();
		g.gray(.5);
		g.FillRT();

		int scene=int(T/10)%3;
		//scene=2;
		if(scene==0)
		{
			stext("Watch the form of the ring", 10,10,0x40000000);
			int N=48;
			for(int i=0;i<N;i++)
			{
				float a=i/float(N)*M_PI*2;
				Dot(320+cos(a)*180,240+sin(a)*180,6,6,a*2+T*2,T*2+a*8,.5);
			}
		}
		else if(scene==1)
		{
			stext("Is the square rotating?", 10,10,0x40000000);
			for(int i=-5;i<6;i++)
			{
				Dot(320-i*20,240-120,4,4,     0,+sin(T/2.)*i/6.*8,.5);
				Dot(320+i*20,240+120,4,4,     0,-sin(T/2.)*i/6.*8,.5);
				Dot(320-120,240+i*20,4,4,M_PI/2,+sin(T/2.)*i/6.*8,.5);
				Dot(320+120,240-i*20,4,4,M_PI/2,-sin(T/2.)*i/6.*8,.5);
			}
		}
		else if(scene==2)
		{
			stext("Some movements", 10,10,0x40000000);
			for(int i=-14;i<15;i++)
			{
				for(int j=-14;j<15;j++)
				{
					Dot(320-j*12,240-i*12,8,2,(sin(i*.3+T*.8)+sin(j*.4-T))+T,T*.5,1.);
				}
			}
		}
		Present();
	}
	return 0;
}