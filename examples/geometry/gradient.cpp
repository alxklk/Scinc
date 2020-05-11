#include <stdio.h>
#include "graphics.h"

#define __SCINC_STEPS__ 100000000

int main()
{
	Graph g;
	int n=0;
	for(;;)
	{
		g.gray(0);g.fill1();
		for(int y=0;y<480;y++)
		{
			float dy=y-240;
			for(int x=0;x<640;x++)
			{
				float dx=x-320;
				float R=sqrt(dx*dx+dy*dy);
				int l=R;
				PutPixel(x,y,(l+n)&0xff);
			}
		}
		Present();
		n++;
	}
	return 0;
}