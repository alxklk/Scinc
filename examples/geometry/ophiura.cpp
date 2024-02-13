#include <stdio.h>

#define G_SCREEN_SCALE 4
#define G_SCREEN_WIDTH 320
#define G_SCREEN_HEIGHT 240

#include "graphics.h"

#define M_PI 3.141592654

Graph g;

float t;

int seed=374693645;

int irand(int& seed)
{
	seed=(seed*1103515245+12345)%0x7ffffff;
	return seed;
}

double frand(int& seed)
{
	return (irand(seed)>>11)/65536.0;
}

int main()
{
	int oldmx=-1;
	float ls[10];
	for(int j=0;j<10;j++)
	{
		ls[j]=30-frand(seed)*10.;
		printf("%f ", ls[j]);
	}
	printf("\n");

	for(;;)
	{
		t=Time()*2;
		g.rgb(.7,.6,.5);
		g.FillRT();

		float xs[6]={0,0,0,0,0,0};
		float ys[6]={0,0,0,0,0,0};

		for(float i=0;i<30.;i+=0.2)
		{
			for(int j=0;j<6;j++)
			{
				if(i>ls[j])
					continue;
				float f=i/ls[j];
				int c=0x00000000;
				float fw=cos(120./(2.-f));
				fw=fw<0.?-fw:fw;
				fw=pow(fw,.2);
				c|=(int(255)<<24);
				c|=(int(fw*(80.*sin(14+t*.7+f*1.7+j)+100.)));
				c|=(int(fw*(80.*sin(41+t*.6+f*1.6+j)+100.))<<8);
				c|=(int(fw*(80.*sin(23+t*.8+f*1.5+j)+100.))<<16);
				//float fc=.5+fw*.5;
				//c=0xff000000|(int(0xf0*fc)<<16)|(int(0x80*fc)<<8)|(int(0x60*fc));
//				float af=t*.0+j*.6+sin(f*f*9.+t*.5+j*13.1)*28.*sin(t*.1+j)*(1.2+sin(t*0.3))*.2;
				float af=t*.1+j+sin(f*12*(11.7+j*1.71)*.05-t*(26.3+j*1.87)*.02+j*3.1+117)*f*5;
				xs[j]+=sin(af)*1.8;
				ys[j]+=cos(af)*1.8;
				float x1=xs[j];
				float y1=ys[j];
				f=(1.-f)+fw*fw*.5;
				g.Circle(160+x1,      120+y1,0,    f+1,2*(f+.5),                                c);
				g.Circle(160+x1,  120+y1+f*2,0,      0,  3.*f+1,           (int(0x50*(.5+f))<<24));
				g.Circle(160+x1,120+y1-f*2.5,0,.5*f+.5,     1.5,(int(0x80*(.5+f))<<24)|0x00ffffff);
			}
		}

		Present();
	}
	return 0;
}