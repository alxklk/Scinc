#include <stdio.h>
#include "graphics.h"

Graph g;

#define M_PI 3.141592654

int main()
{
	while(true)
	{
		double T=Time();
		g.clear();
		g.gray(.5);
		g.fill1();

		g.clear();
		for(int i=0;i<24;i++)
		{
			if(i==int(T*8)%24)
				continue;
			float a=i/24.;
			g.M(320+cos(a*M_PI*2)*180,240+sin(a*M_PI*2)*180);
		}
		g.fin();
		g.rgb(0,1,0);
		g.width(20,2);
		g.stroke();

		g.clear();
		g.M(320,230);
		g.l(0,20);
		g.M(310,240);
		g.l(20,0);
		g.fin();
		g.gray(0);
		g.width(1.5,1.5);
		g.stroke();

		Present();
	}
	return 0;
}