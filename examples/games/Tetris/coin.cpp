#define G_SCREEN_WIDTH 320
#define G_SCREEN_HEIGHT 240
#define G_SCREEN_SCALE 5
#define G_SCREEN_MODE 1

#define C G_SCREEN_WIDTH/2

//#define WRITE_MPG

#include "graphics.h"

Graph g;

float Fabs(float x)
{
	return x<0?-x:x;
}

int main()
{
#ifdef WRITE_MPG
	int mpg=g.MpegCreate("Bear_And_Bees.mpg");
	int n=0;
#endif
	while(true)
	{
		float T=Time();
		int mx;
		int my;
		int mb;
		GetMouseState(mx, my, mb);
		float coinr=200;
		float coinh=coinr*.2;
		float coinb=0.05;
		g.clear();
		g.alpha(1);
		g.M(0,0);g.l(G_SCREEN_WIDTH,0);g.l(0,G_SCREEN_HEIGHT);g.l(-G_SCREEN_WIDTH,0);g.close();g.fin();g.rgb(.3,.15,.1);g.fill1();

		g.clear();

		float a=T*6;
		//a=n/60.*6.;
		//a=mx/25.;
		//coinr*=mx/100.;
		//coinh*=mx/100.;
		float s=cos(a)*coinh;

		if(sin(a)<0)s=-s;

		{
			g.clear();
			g.M(C-s/2,20+coinr*coinb);
			g.a(sin(a),1,0,0,1,0,coinr*(1.-coinb*2));
			g.a(sin(a),1,0,0,1,0,-coinr*(1.-coinb*2));
			g.close();
			g.fin();
			g.graddef(0);
			g.gradstop(1,.20,.30,.30,1);
			float l=.5-cos(a*2+1)*.5;
			g.gradstop(.3,l*.5+l*l*.5,l*.7+l*l*.3,l,1);
			g.gradstop(0,.5,.75,.75,1);
			g.gradtype(1);
			g.gradmethod(2);
			g.gradend();
			g.graduse(0);
			g.g_0(0,19);
			g.g_y(C,0);
			g.g_x(0,coinr+1);
			g.fill1();
			g.graduse(-1);
		}

		{
			g.clear();
			float r=coinr*(1.-coinb*2);
			g.M(C-s/2+s/4,20+coinr*coinb);
			g.a(sin(a),1,0,0,0,0, r);
			g.a(sin(a),1,0,0,0,0,-r);
			g.fin();
			float l=.65+sin(a*2)*.35;
			g.rgb(.1+l*.3+l*l*l*.5,.2+l*.6+l*l*l*.1,.2+l*.7);
			g.fill1();
			g.width(1,1);
			g.stroke();
		}

		{
			g.clear();
			g.M(C-s/2,20);
			g.a(sin(a),1,0,0,0,0,coinr);
			g.a(sin(a),1,0,0,0,0,-coinr);
			g.close();
			g.M(C-s/2,20+coinr*coinb);
			g.a(sin(a),1,0,0,1,0,coinr*(1.-coinb*2));
			g.a(sin(a),1,0,0,1,0,-coinr*(1.-coinb*2));
			g.close();
			g.fin();
			float l=.6+sin(a*2)*.4;
			g.rgb(.2+l*.3+l*l*l*.5,.3+l*.6+l*l*l*.1,.3+l*.7);
			g.fill1();
			g.width(1,1);
			g.stroke();
		}

		{
			g.clear();
			g.M(C-s/2,20);
			g.l(s,0);
			if(s<0)
				g.a(Fabs(sin(a)),1,0,0,0,0,coinr);
			else
				g.a(-Fabs(sin(a)),1,0,0,0,0,coinr);
			g.l(-s,0);
			if(s<0)
				g.a(Fabs(sin(a)),1,0,0,1,0,-coinr);
			else
				g.a(-Fabs(sin(a)),1,0,0,1,0,-coinr);
			g.fin();
			g.graddef(0);
			g.gradstop(0,.0,.2,.2,1);
			g.gradstop(.2,.0,.4,.5,1);
			float l=.75+cos(a*2+1)*.25;
			g.gradstop(.7,l*.5+l*l*.5,l*.9+l*l*.1,l,1);
			g.gradstop(1,.2,.8,.9,1);
			g.gradtype(1);
			g.gradend();
			g.graduse(0);
			g.g_0(0,19);
			g.g_y(C,0);
			g.g_x(0,coinr+1);
			g.fill1();
			g.graduse(-1);
		}
#ifdef WRITE_MPG
		if(n<1000)
		{
			printf("frame %i\n", n);
			g.MpegWriteFrame(mpg,0);
		}
		else if(n==1000)
		{
			g.MpegDone(mpg);
			puts("mpg finished\n");
		}
		else
		{
			Present();
		}
		n++;
#else 
		Present();
#endif	
	}
	return 0;
}