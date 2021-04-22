#include "graphics.h"
#include "svg.h"

Graph g;

int irand(int& seed)
{
	seed=(seed*1103515245+12345)&0x7ffffff;
	return seed;
}

float frand(int& seed)
{
	return (irand(seed))/134217727.0;
}

int main()
{
	int ssat=SVGLoad("satellite.svg");
	int sship=SVGLoad("spaceship.svg");
	int dog=SVGLoad("dog.svg");
	int heart=SVGLoad("Heart0.svg","Heart0.cen");
	int heartAnim=SVGLoadAnimation(heart,"Heart0.ami");
	int hips=SVGLoad("Hips.svg","Hips.cen");
	int hipsAnim=SVGLoadAnimation(hips,"Hips.ami");
	while(true)
	{
		g.rgb(.3,.1,.0);
		g.FillRT();
		float t=Time()*1.2;
		for(int i=0;i<260;i++)
		{
			float x=300+300*sin((t+i*7.301)*.311);
			float y=220+220*cos((t+i*5.24)*.137);
			SVGSetTransform(x,y,.1,0,0,.1);
			int idx=i%8;
			if(idx==0)
				SVGDrawAnimated(heart, heartAnim, t+i*.1);
			else if(idx==1)
				SVGDraw(sship);
			else if(idx==2)
				SVGDraw(ssat);
			else if(idx==3)
			{
				SVGSetTransform(x,y,.5,0,0,.5);
				SVGDraw(dog);
			}
			else
			{
				SVGSetTransform(x,y,2,0,0,2);
				SVGDrawAnimated(hips, hipsAnim, x*.01);
			}
		}
		Present();
	}
	return 0;
}