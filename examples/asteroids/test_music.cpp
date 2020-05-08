#include "graphics.h"

#include "asters_music.h"

int main()
{
	float tframe;
	tframe=Time();
	float dt;
	dt=0.;
	CMusic music;
	music.Init();
	while(true)
	{
		rgb(.03,.1,.05);
		clear();
		fill1();
		clear();
		M(0,240.5);l(640,0);
		M(0,240.5+120);l(640,0);
		M(0,240.5-120);l(640,0);
		fin();
		width(1.25,1.25);
		rgb(0,.5,0);
		stroke();
		clear();

		int nSamples=Time()*44100-tframe*44100+1;
		tframe=Time();
		if(nSamples>2000)nSamples=2000;
		music.GenerateSamples(nSamples);
		for(int i=0;i<640;i+=4)
		{
			float lvl=music.echo[((music.echoPos+(i-640)*2+11150)%11150)];
			M(i+.5,240);
			l(0,lvl*120);
			l(2,0);
			l(0,-lvl*120);
		}
		fin();
		width(1.,1.);
		rgb(1.0,.75,0.3);
		stroke();

		char ss[64];
		snprintf(ss,64,"Time %f %i %i %i", Time(), nSamples, music.echoPos, music.sample);
		stext(ss,10,10,0xffffffff);

		Present();
	}
	return 0;
}