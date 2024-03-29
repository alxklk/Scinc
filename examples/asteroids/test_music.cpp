#include "sound.h"

#define G_SCREEN_MODE 1
#include "graphics.h"

#ifndef __SCINC__
#define float double
#endif

#include "asters_music.h"

#define NFFT 1024

#include "../sound/FFT_rc.h"
#include "../ws.h"

void FFT(float* in, float* o, int j)
{
	cplx buf[NFFT];
	for(int i=0;i<NFFT;i++)
	{
		int idx=(-i*8+j+12000*2)%12000;
		buf[i].re=in[idx];
		buf[i].im=in[idx];
	}
	fft((cplx*)&(buf[0]), (cplx*)(void*)o, NFFT);

/*	I.re=0;
	I.im=1;

	cplx buf[NFFT];
	cplx out[NFFT];
	for(int i=0;i<NFFT;i++)
	{
		int idx=((-i+j)*2+22500)%22500;
		buf[i].re=in[idx*2];
		buf[i].im=0;
	}

	fft(buf, out, NFFT);

	for(int i=0;i<NFFT;i++)
	{
		o[i]=sqrt(out[i].im*out[i].im+out[i].re*out[i].re);
	}
*/
}

float fftout[NFFT*2];


int main()
{
	//SetPresentWait(0);
	float tframe;
	tframe=Time();
	float dt;
	dt=0.;
	CMusic music;
	music.Init();
	FFT_Init();
	Graph g;
	while(true)
	{
		g.rgb(.03,.1,.05);
		g.FillRT();
		g.clear();
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


		tframe=Time();
		CSound s;
		while(s.snd_bufhealth()<(3000+NFFT))
		{
			music.GenerateSamples(NFFT);
			FFT(music.echo,fftout,music.echoPos);
		}

		//g.M(0,240.5);
		for(int i=0;i<640;i++)
		{
			float lvl=music.echo[((music.echoPos+(i-640)*2+12000)%12000)];
			g.L(i,240.5+lvl*120);
			//g.l(2,0);
			//g.l(0,-lvl*120);
		}
		//g.L(640,240.5);
		//g.close();
		g.fin();
		g.width(1.,1.);
		g.rgb(0.2,.4,0.);
		//g.fill2();
		g.rgb(.7,1.,.5);
		g.stroke();
		g.rgb(.3,1.,.5);
		g.width(8.,.5);
		g.stroke();

		g.clear();
		float l[4]={0.,0.,0.,0.};
		int N=NFFT/2;
		for(int i=0;i<N;i++)
		{
			g.M(i+.5,480);
			float lvl;
			double re=fftout[i*2  ];
			double im=fftout[i*2+1];
			lvl=(sqrt(re*re+im*im));
			l[i/128]+=lvl;
			lvl*=4.;
			if(lvl>240)lvl=240;
			g.l(0,-lvl);
		}
		g.fin();
		g.rgb(1.,.3,.0);
		g.width(4.,1.);
		g.stroke();
		g.rgb(.8,.6,.0);
		g.width(1.,1.);
		g.stroke();

		float col[12]={0,0,1, 0,1,1, 0,1,0, 1,.5,0};
		int coli[4]={0xffff0000,0xffffff00,0xff00ff00,0xff4000ff};

		for(int i=0;i<4;i++)
		{
			//g.clear();
			//g.M(170+100*i,120);
			//g.l(0,0);
			//g.fin();
			//g.rgba(col[i*3],col[i*3+1],col[i*3+2],.5);
			//l[i]*=.08;
			float a=l[i]*0.01;
			a=a>1.?1.:a;
			//a*=a;
			//g.alpha(a);
			g.Circle(170+100*i,120,0,1,50*a+10,coli[i]);
			//g.stroke();
		}
		g.alpha(1);

		char ss[64];
		snprintf(ss,64,"Time % 8.4f % 8i % 8i % 8i", Time(), s.snd_bufhealth(), music.echoPos, music.sample);
		stext(ss,10,10,0xffffffff);

		Present();
		s.Poll();
	}
	return 0;
}