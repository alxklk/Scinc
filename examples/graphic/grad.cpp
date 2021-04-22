#include "graphics.h"

Graph g;

void Gradient()
{
	float t=-Time();
	g.graddef(0);
	for(int i=0;i<256;i+=2)
	{
		float s=i*1./256.0;
		g.gradstop(s,
			.5+.15*sin(s*3.35*3.-t*2.85+17.8)+.15*sin(-s*14.85*1.6+(t+1.8)*2.85*1.13+7.8),
			.15+.15*sin(s*2.81*3.-t*3.58+12.2)+.15*sin(-s*16.11*1.6+(t+5.7)*3.58*1.12+2.2),
			.15+.15*sin(s*3.37*3.-t*2.59+27.7)+.15*sin(-s*15.37*1.6+(t+8.7)*2.59*1.11+7.7),
			1
		);
	}
	g.alpha(1);
	g.gradtype(2);
	g.gradend();
	g.graduse(0);
	g.clear();
	g.M(0,0);
	g.l(640,0);
	g.l(0,480);
	g.l(-640,0);
	g.close();
	g.fin();
	g.g_0(0,0);
	g.g_x(640,480);
	g.g_y(-480,640);
	g.fill1();
}


int main()
{

	while(true)
	{
		Gradient();
		Present();
	}
	return 0;
}