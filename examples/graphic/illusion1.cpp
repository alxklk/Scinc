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
		g.gray(.95);
		g.fill1();

		stext("Fix eyes on the black cross", 10,10,0x40000000);
		stext("See moving green dot", 10,20,0x40000000);

		g.clear();
		int N=12;
		for(int i=0;i<N;i++)
		{
			if(i==int(T*8)%N)
				continue;
			float a=i/float(N);
			g.M(320+cos(a*M_PI*2)*180,240+sin(a*M_PI*2)*180);
		}
		g.fin();
		g.rgb(1,0,1);
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