#include <stdio.h>
#include "graphics.h"
#include "../ws.h"

#define M_PI 3.141592654

float Fabs(float x)
{
	if(x<0)
		return -x;
	else
		return x;
}

Graph g;

void Circle(float x, float y, float r)
{
	g.M(x,y-r);
	g.a(1,1,0,0,0,0,r*2);
	g.a(1,1,0,0,0,0,-r*2);
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
		g.gray(0);
		g.FillRT();
		g.clear();
		g.width(1.,1.);

		float t0=Time();
		//seed=23423448;
		for(int i=0;i<500;i++)
		{
			float x=20+frand(seed)*600.;
			float y=20+frand(seed)*440.;
			float R=(1.+frand(seed))*25.;
			int cr=irand(seed)&0xff;
			int cg=irand(seed)&0xff;
			int cb=irand(seed)&0xff;
			int c=0xff000000|(cb<<16)|(cg<<8)|cr;
#define VECT
#ifdef VECT
			g.clear();
			Circle(x,y,R);
			g.fin();
			g.rgba32(c);
			if((mode>>6)&1)
				g.fill1();
			else
				g.stroke();
#else
			if((mode>>6)&1)
				g.Circle(x,y,0,R-.5,1,c);
			else
				g.Circle(x,y,R,0,1,c);
#endif
		}
		float t1=Time();
		char s[32];
		snprintf(s, 32, "%10.8f", t1-t0);
		stext(s,10,11,0xffffffff);
		stext(s,11,10,0xffffffff);
		stext(s,10,9,0xffffffff);
		stext(s,9,10,0xffffffff);
		stext(s,10,10,0xff000000);
		mode++;
		Present();
	}
	return 0;
}