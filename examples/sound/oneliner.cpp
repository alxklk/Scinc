#include "graphics.h"

#define M_PI 3.141592654

float sndVal(int t)
{
	int il=((t*("16164289"[(t>>13)&7]&15))/12&128)+(((((t>>12)^(t>>12)-2)%11*t)/4|t>>13)&127);
	return (il&255)/255.;
}

Graph g;

int sndSample;

void GenerateSamples(int nSamples)
{
	for(int i=0;i<nSamples;i++)
	{
		float l=sndVal(sndSample);
		sndSample++;
		snd_out(l,l);
	}
}	

int main()
{
	float t0=Time();
	float tframe;
	tframe=Time();
	sndSample=0;
	while(true)
	{
		g.t_0(0,0);
		g.t_x(1,0);
		g.t_y(0,1);
		g.rgb(.01,.15,.05);
		g.clear();
		g.FillRT();
		g.clear();

		int nSamples=Time()*44100-tframe*44100+1;
		tframe=Time();
		if(nSamples>2000)nSamples=2000;
		GenerateSamples(nSamples);
		g.M(0.,120+240.*sndVal(sndSample-640));
		for(int i=1;i<640;i++)
		{
			g.L(i,120+sndVal(sndSample+i-640)*240.);
		}
		g.fin();
		g.width(8.,1.);
		g.rgb(.5,.8,0);
		g.stroke();
		g.width(2.,1.);
		g.rgb(.9,1,.8);
		g.stroke();

		char ss[64];
		g.t_x(3.5,0);
		g.t_y(0,-4.5);
		g.clear();
		snprintf(ss,64,"Time %f", Time());
		g.t_0(5,20);
		gtext(ss);

		stext("((t*(\"16164289\"[(t>>13)&7]&15))/12&128)+(((((t>>12)^(t>>12)-2)%11*t)/4|t>>13)&127)",10,30,0xff80ffe0);

		g.fin();
		g.width(1.25,1.25);
		g.rgb(.5,.8,1.0);
		g.stroke();
		Present();
	}
	return 0;
}