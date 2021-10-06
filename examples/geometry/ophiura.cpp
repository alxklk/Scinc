#include <stdio.h>

#define G_SCREEN_SCALE 2

#include "graphics.h"

#define M_PI 3.141592654

Graph g;

float t;

int main()
{
	printf("Start\n");
	int oldmx=-1;
	g.M(0,0);
	g.l(640,0);
	g.l(0,480);
	g.l(-640,0);
	g.close();
	g.fin();
	g.rgba(0,0,0,2./255.);
	for(;;)
	{
		t=Time()*2;
		g.fill1();

		for(float i=0;i<30;i+=0.2)
		{
			for(int j=0;j<10;j++)
			{
				float f=i/30.;
				float a=t*.08+sin(j*17.+t*.09)*2.+f*sin(j*13.7+t*.22+f*2.71);
				float ca=cos(a);
				float sa=sin(a);
				int c=0x00000000;
				c|=(int(0.1*255.*(1.-f))<<24);
				c|=(int(120.*sin(14+t*.7+f*1.7)+125));
				c|=(int(120.*sin(41+t*.6+f*1.6)+125)<<8);
				c|=(int(120.*sin(23+t*.8+f*1.5)+125)<<16);
				float x=sin((t*4.-f*16.+j*1.2))*10*f;
				float y=f*340;
				float x1=x*ca+y*sa;
				float y1=y*ca-x*sa;
				g.Circle(320+x1,240+y1,0,3,4*(1.-f)+4,c);
				g.Circle(320+x1,240+y1,0,2,6*(1.-f),c|0x00ffffff);
			}
		}

		Present();
	}
	return 0;
}