#define G_SCREEN_SCALE 4
#include "graphics.h"
#include "GUI.h"

float interp(float y0, float y1, float t)
{
	t=(3.-2.*t)*t*t;
	return y0+(y1-y0)*t;
}

int main()
{
	CGUI gui;
	gui.Init();
	Graph g;

	SScincEvent event;
	float val[8]={.5,.3,.7,0.,1.,1.,1.,1.};

	int cx0=10;
	int dx=25;
	gui.AddSlide("", cx0+=dx,10,15,101,'val1',&val[0],true,2,0);
	gui.AddSlide("", cx0+=dx,10,15,101,'val1',&val[1],true,2,0);
	gui.AddSlide("", cx0+=dx,10,15,101,'val2',&val[2],true,2,0);
	gui.AddSlide("", cx0+=dx,10,15,101,'val3',&val[3],true,2,0);
	gui.AddSlide("", cx0+=dx,10,15,101,'val4',&val[4],true,2,0);
	gui.AddSlide("", cx0+=dx,10,15,101,'val4',&val[5],true,2,0);
	gui.AddSlide("", cx0+=dx,10,15,101,'val4',&val[6],true,2,0);
	gui.AddSlide("", cx0+=dx,10,15,101,'val4',&val[7],true,2,0);

	SetPresentWait(true);

	while(1)
	{
		while(GetScincEvent(event))
		{
			if(gui.Event(event))
				continue;
		}
		g.rgba32(0xff505050);
		g.FillRT();
		gui.Render(g);
		for(int i=0;i<8;i++)
		{
			char s[32];
			snprintf(s,32,"%3.1f",val[i]);
			stext(s,33+i*25,120,0xff000000);
		}
		g.rgba32(0xffffffff);
		for(int i=1;i<8;i++)
		{
			float y1=-val[i  ]*50+110;
			float y0=-val[i-1]*50+110;
			float x0=(i-1)*25+33+8;
			float dj=1./10.;
			float y=y0;
			float x=x0;
			for(float j=dj;j<1.001;j=j+dj)
			{
				float yn=interp(y0,y1,j);
				float xn=x+dj*25.;
				g.hairline(x,y,xn,yn);
				x=xn;
				y=yn;
			}
		}

		Present();

	}	
	return 0;
}