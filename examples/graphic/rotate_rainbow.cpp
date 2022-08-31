#ifdef NOTDEF
Scinc "$0"
exit
#endif

#include "graphics.h"

Graph g;

#define M_PI 3.141592654

int main()
{
	double T=0;
	while(true)
	{
		T=Time();
		g.rgba32(0xffc0c0c0);
		g.FillRT();
		g.miterlim(.1);
		
		int N=22;
		float size=220;
		float r0=size/N;
		for(int i=1;i<N;i++)
		{
			float dx=cos(T+i*T*.2);
			float dy=sin(T+i*T*.2);
			g.clear();
			g.M(320-dx*r0*i,240-dy*r0*i);
			g.a(1,1,0,0,1,dx*r0*i*2,dy*r0*i*2);
			g.l(r0*dx,r0*dy);
			g.a(1,1,0,0,0,-dx*(r0*(i*2+2)),-dy*(r0*(i*2+2)));
			g.close();
			g.fin();
			g.width(5,1);
			g.rgba32(0x60000000);
			g.stroke();
			g.alpha(1);
			g.rgb(1,1./N*i,1-1./N*i);
			g.fill1();
			g.width(1,1);
			//g.stroke();
		}
		Present();
	}
	return 0;
}