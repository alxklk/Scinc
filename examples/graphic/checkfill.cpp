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
		#define N 2000
		for(int i=0;i<N;i++)
		{
			float x=(cos(i*M_PI/N*2.*5+T*1.1)*.8+sin(i*M_PI/N*2.*29-T*0.12)*.2)*300+320;
			float y=(sin(i*M_PI/N*2.*4+T*.93)*.8+cos(i*M_PI/N*2.*27-T*0.37)*.2)*220+240;
			g.L(x,y);
		}
		g.close();
		g.fin();
		g.alpha(1);

		g.graddef(0);
		g.gradstop(0,sin(T*4.3+17)*.25+.75,sin(T*3.3+11)*.25+.75,sin(T*5.1+25)*.25+.75,1);
		g.gradstop(1,sin(T*2.3+71)*.25+.75,sin(T*1.3+31)*.25+.75,sin(T*2.1+65)*.25+.75,1);
		g.gradtype(2);
		g.gradmethod(0);
		g.gradend();
		g.graduse(0);
		g.g_t(320,240,240,0,0,240);
		g.width(1,1);
		g.fill2();
		Present();
	}
	return 0;
}