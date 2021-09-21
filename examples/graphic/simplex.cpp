#include "../math/simplex_noise.h"

#define G_SCREEN_WIDTH 320
#define G_SCREEN_HEIGHT 240
#define G_SCREEN_SCALE 2
#define G_SCREEN_MODE 1

#include "graphics.h"

Graph g;

struct sline{int l[320];};

int main()
{
	sline *buf=(sline*)malloc(320*240*sizeof(int));
	while(true)
	{
		float T=Time();
		for(int i=0;i<240;i++)
		{
			for(int j=0;j<320;j++)
			{
				int c=(noise(j/30.,i/30.,T)+1)/2*255;
				c=c|(c<<8)|(c<<16);
				buf[i].l[j]=c;
			}
		}
		g.BitBlt(buf,320,240,0,0);
		Present();
	}
	return 0;
}