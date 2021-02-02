#include <stdio.h>

#define G_SCREEN_WIDTH 320
#define G_SCREEN_HEIGHT 240
#define G_SCREEN_SCALE 4

#include "graphics.h"

#define __SCINC_STEPS__ 500

int main()
{
	Graph g;
	int n=0;
	for(;;)
	{
		g.gray(0);g.fill1();
		float cy=-120.;
		for(int y=0;y<240;y++)
		{
			float ddy=cy*cy;
			cy=cy+1.;
			float cx=-160;
			for(int x=0;x<320;x++)
			{
				float R=sqrt(cx*cx+ddy);
				cx=cx+1.;
				int l=sin((R+n)*.03)*127.+127.;
				PutPixel(x,y,(l&0xff));
			}
		}
		Present();
		n++;
	}
	return 0;
}