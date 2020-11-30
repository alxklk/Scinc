#include "graphics.h"

#include "asters_music.h"
#include "../sound/FFT.h"

#define NFFT 1024

void FFT(float* in, float* o, int j)
{
	I.re=0;
	I.im=1;

	cplx buf[NFFT];
	cplx out[NFFT];
	for(int i=0;i<NFFT;i++)
	{
		int idx=((-i*8+j)*2+22500)%22500;
		buf[i].re=in[idx];
		buf[i].im=0;
	}

	fft(buf, out, NFFT);

	for(int i=0;i<NFFT;i++)
	{
		o[i]=sqrt(out[i].im*out[i].im+out[i].re*out[i].re);
	}

}

float fftout[NFFT];


int main()
{
	float tframe;
	tframe=Time();
	float dt;
	dt=0.;
	CMusic music;
	music.Init();
	Graph g;
	while(true)
	{
		g.rgb(.03,.1,.05);
		g.alpha(.15);
		g.clear();
		g.fill1();
		g.alpha(1);
		//g.clear();
		//g.M(0,240.5);g.l(640,0);
		//g.M(0,240.5+120);g.l(640,0);
		//g.M(0,240.5-120);g.l(640,0);
		//g.fin();
		//g.width(1.25,1.25);
		//g.rgb(0,.5,0);
		//g.stroke();
		//g.clear();


		int nSamples=Time()*44100-tframe*44100+1;
		tframe=Time();
		if(nSamples>2000)nSamples=2000;
		music.GenerateSamples(nSamples);

		FFT(music.echo,fftout,music.echoPos);

		for(int i=0;i<640;i+=4)
		{
			float lvl=music.echo[((music.echoPos+(i-640)*2+11150)%11150)];
			g.M(i+.5,240);
			g.l(0,lvl*120);
			g.l(2,0);
			g.l(0,-lvl*120);
		}
		g.fin();
		g.width(1.,1.);
		g.rgb(0.5,1.,0.3);
		g.stroke();

		g.clear();
		float l[4]={};
		for(int i=0;i<512;i+=4)
		{
			g.M(i+.5,480);
			float lvl=0;
			for(int j=0;j<4;j++)lvl+=fftout[i+j];
			l[i/128]+=lvl;
			lvl*=4.;
			if(lvl>240)lvl=240;
			g.l(0,-lvl);
		}
		g.fin();
		g.rgb(.8,.6,.0);
		g.width(2.,2.);
		g.stroke();

		float col[12]={0,0,1, 0,1,1, 0,1,0, 1,.5,0};

		for(int i=0;i<4;i++)
		{
			g.clear();
			g.M(170+100*i,120);
			g.l(0,0);
			g.fin();
			g.rgba(col[i*3],col[i*3+1],col[i*3+2],.5);
			//l[i]*=.08;
			float a=l[i]*.005;
			a=a>1.?1.:a;
			a*=a;
			g.alpha(a);
			g.width(50,1);
			g.stroke();
		}
		g.alpha(1);

		//char ss[64];
		//snprintf(ss,64,"Time %f %i %i %i", Time(), nSamples, music.echoPos, music.sample);
		//stext(ss,10,10,0xffffffff);

		Present();
	}
	return 0;
}