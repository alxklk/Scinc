#include "graphics.h"

Graph g;

#define M_PI 3.141592654

int main()
{
	double T=0;
	while(true)
	{
		T+=0.003;
		g.rgba32(0xff000000);
		g.FillRT();
		
		g.clear();
		for(int i=0;i<2000;i++)
		{
			float x=(cos(i*M_PI/2000.*2.*7+T*1.1)*.8+sin(i*M_PI/2000.*2.*29-T*0.12)*.2)*300+320;
			float y=(sin(i*M_PI/2000.*2.*8+T*.93)*.8+cos(i*M_PI/2000.*2.*27-T*0.37)*.2)*220+240;
			if(i==0)
				g.M(x,y);
			else
				g.L(x,y);
		}
		g.close();
		g.fin();
		g.alpha(1);
		g.rgb(sin(T*4.3+17)*.25+.75,sin(T*3.3+11)*.25+.75,sin(T*5.1+25)*.25+.75);
		g.width(1,1);
		g.fill2();
		Present();
	}
	return 0;
}