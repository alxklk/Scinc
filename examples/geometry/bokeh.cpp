#include <stdio.h>

#define G_SCREEN_SCALE 2

#include "graphics.h"

#define M_PI 3.141592654

Graph g;

float t;

float f(float x, float y)
{
	return sin(sqrt(x*x+y*y)*.8+t);
}

#define N 10

float S=1.;
float SZ=30;

float h[(N*2+1)*(N*2+1)];

int main()
{
	printf("Start\n");
	int oldmx=-1;
	for(;;)
	{
		t=Time();

		int mx;
		int my;
		int mb;
		GetMouseState(mx,my,mb);
		//if(oldmx==mx)
		//{
		//	Present();
		//	continue;
		//}
		oldmx=mx;
		float a=mx/60.;
		a=t*.04;
		//a=2;
		float SXX= 7*S*cos(.5)*6;
		float SYX= 7*S*sin(.5)*6;
		float SXY= 3*S*sin(.5)*6;
		float SYY=-3*S*cos(.5)*6;

		g.rgb(0.3,.15,.1);
		g.FillRT();

		g.clear();
		g.gray(1);
		g.width(1.,.5);
		for(int i=-N;i<=N;i++)
		{
			for(int j=-N;j<=N;j++)
			{
				h[(i+N)*(N*2+1)+j+N]=f(j,i);
			}
		}
		for(int ii=-N+1;ii<=N;ii++)
		{
			int i=ii;
			int i0=ii-1;
			int i1=ii  ;
			if(SYY<0)
			{
				i=-i+1;
				i0=-ii;i1=-ii+1;
			}
			for(int jj=-N+1;jj<=N;jj++)
			{
				int j=jj;
				int j0=jj-1;
				int j1=jj  ;
				if(SXY<0)
				{
					j=-j+1;
					j0=-jj;j1=-jj+1;
				}
				g.clear();
				float h0=h[(i0+N)*(N*2+1)+j1+N];
				//float h1=h[(i1+N)*(N*2+1)+j1+N];
				//float h2=h[(i1+N)*(N*2+1)+j0+N];
				//float h3=h[(i0+N)*(N*2+1)+j0+N];

				g.L(320+(i-1)*SYX+(j  )*SXX,240+(i-1)*SYY+(j  )*SXY-h0*SZ);
				//g.L(320+(i  )*SYX+(j  )*SXX,240+(i  )*SYY+(j  )*SXY-h1*SZ);
				//g.L(320+(i  )*SYX+(j-1)*SXX,240+(i  )*SYY+(j-1)*SXY-h2*SZ);
				//g.L(320+(i-1)*SYX+(j-1)*SXX,240+(i-1)*SYY+(j-1)*SXY-h3*SZ);
				//g.close();
				g.fin();
				//float l=(h0+h1+h2+h3)/4.*.5+.5;
				//g.gray(0);
				//g.rgb(0.3,.15,.1);
				//g.alpha(.65);
				//g.rgb((h0-h1)*.15+.15,(h0-h2)*.15+.15,.0);
				//g.fill1();
				//g.rgb((h0-h1)*.5+.5,(h0-h2)*.5+.5,0);
				//g.alpha(1);
				float w=(i-1)*SYY+(j  )*SXY;
				if(w<0)w=-w;
				float w1=150.-w;
				if(w1<0)w1=0;
				w1*=.006;
				g.alpha(w1*w1*w1*4.);
				g.width(w*.2+3,(w*.1+2)*.5);
				g.stroke();
			}
		}
		g.clear();
		g.M(sin(t*.31+13)*320+320,0);
		g.L(sin(t*.12-15)*320+320,480);
		g.fin();
		g.alpha(.1);
		g.width(350,1);
		g.stroke();
		Present();
	}
	return 0;
}