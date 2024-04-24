#include <stdio.h>
#define G_SCREEN_WIDTH 640
#define G_SCREEN_HEIGHT 480
#define G_SCREEN_SCALE 3
#define G_SCREEN_MODE 1
#include "graphics.h"
#include "../ws.h"

Graph g;

int main()
{
	int n=0;
	float t0=Time();
	SetPresentWait(0);
	while(true)
	{
		n++;
		if(n%2)
		{
			g.rgba32(0xff000000);
		}
		else
		{
			g.rgba32(0xffffffff);
		}
		if((n%60)==0)
		{
			printf("fps=%f\n", n/(Time()-t0));
		}
		g.FillRT();
		Present();
	}
	return 0;
}