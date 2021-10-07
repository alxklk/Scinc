#include <stdio.h>

#define G_SCREEN_SCALE 2

#include "graphics.h"

#define M_PI 3.141592654

Graph g;

float t;

int main()
{
	int oldmx=-1;
	g.M(0,0);
	g.l(640,0);
	g.l(0,480);
	g.l(-640,0);
	g.close();
	g.fin();
	g.rgb(.7,.6,.5);
	for(;;)
	{
		t=Time()*2;
		g.fill1();

		float xs[10]={};
		float ys[10]={};
		for(float i=0;i<30;i+=0.1)
		{
			for(int j=0;j<10;j++)
			{
				float f=i/30.;
				float a=0;//t*.08+sin(j*17.+t*.09)*2.+f*sin(j*13.7+t*.22+f*2.71);
				float ca=cos(a);
				float sa=sin(a);
				int c=0x00000000;
				float fw=sin(f*50.);
				fw=fw<0.?-fw:fw;
				c|=(int(255)<<24);
				c|=(int(fw*(80.*sin(14+t*.7+f*1.7+j)+100.)));
				c|=(int(fw*(80.*sin(41+t*.6+f*1.6+j)+100.))<<8);
				c|=(int(fw*(80.*sin(23+t*.8+f*1.5+j)+100.))<<16);
//				float af=t*.0+j*.6+sin(f*f*9.+t*.5+j*13.1)*28.*sin(t*.1+j)*(1.2+sin(t*0.3))*.2;
				float af=t*.1+j+sin(f*12*(11.7+j*1.71)*.05-t*(26.3+j*1.87)*.02+j*3.1+117)*f*5;
				xs[j]+=sin(af)*1.5;
				ys[j]+=cos(af)*1.5;
				float x=xs[j];
				float y=ys[j];
				float x1=x*ca+y*sa;
				float y1=y*ca-x*sa;
				//if(i-int(i)<.5)g.Circle(320+x1,240+y1,0,4*(1.-f)+1,1,-1);
				//else
				f-=fw*fw*.5;
				g.Circle(320+x1,240+y1,0,2*(1.-f)+1,2*(1.5-f),c);
				g.Circle(320+x1,240+y1+(1.-f)*2,0,0,3*(1.-f)+2,(int(0x80*(1.5-f))<<24));
				g.Circle(320+x1,240+y1-(1.-f)*2.5,0,.5*(1.-f)+.5,1.5,(int(0x80*(1.5-f))<<24)|0x00ffffff);
			}
		}

		Present();
	}
	return 0;
}