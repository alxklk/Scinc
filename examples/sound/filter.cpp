#define SW 1024
#define G_SCREEN_WIDTH SW
#define G_SCREEN_SCALE 1

#define NFFT 1024

#include "sound.h"
#include "graphics.h"
#include "../asteroids/asters_music.h"

#ifdef __SCINC__
#define const
#endif

#include "FFT_rc.h"

#include "mp3.h"
#include "mic_in.h"

#pragma STACK_SIZE 655360

#define M_PI 3.141592654

int mx;
int my;
int mb;
int prevmx;
int prevmy;
int prevmb;


int seed=374693645;

int irand(int& seed)
{
	seed=(seed*1103515245+12345)%0x7ffffff;
	return seed;
}

double frand(int& seed)
{
	return (irand(seed)>>11)/65536.0;
}

#define ALMOST_ONE 0.9999999999999

/*double mod(double a, double b)
{
	int r=a/b;
	return a-b*r;
}*/

double lega(double globalTime, double time, double t0, double t1, double f0, double f1)
{
	if(time<t0)
		time=t0;
	else if(time>t1)
		time=t1;
	double dt=t1-t0;

	double t=(time-t0)/dt;


	double gt0=(globalTime-time+t0);
	double gt1=(globalTime+t1-time);

	double p0=mod(gt0*f0,1.);
	double p1=mod(gt1*f1,1.);

	double y0=t0*f0;
	y0+=-mod(y0,1.)+p0;
	double y1=y0+(f1+f0)/2.*dt;
	y1+=-mod(y1,1.)+p1;

	double invt=1.-t;

	double y=y0*invt*invt*invt
			+3.*(y0+dt/3.*f0)*invt*invt*t
			+3.*(y1-dt/3.*f1)*invt*t*t
			+y1*t*t*t;

	double v=sin(y*2*M_PI);

	return v;
}


int cursnd;

Graph g;

double Fabs(double x)
{
	return x<0?-x:x;
}


double S_Curve(double x)
{
	return x*x*(-2.0*x+3.0);
}

void FFT(double* in, double* o)
{
	//cplx buf[NFFT];
	//cplx out[NFFT];
	//for(int i=0;i<NFFT;i++)
	//{
	//	buf[i].re=in[i+i  ];
	//	buf[i].im=in[i+i+1];
	//}
	//memcpy(&buf[0],&in[0],sizeof(cplx)*NFFT);
	fft((cplx*)(void*)in, (cplx*)(void*)o, NFFT);
	//memcpy(&o[0],&out[0],sizeof(cplx)*NFFT);

	//for(int i=0;i<NFFT;i++)
	//{
	//	//o[i]=sqrt(out[i].im*out[i].im+out[i].re*out[i].re);
	//	o[i+i  ]=out[i].re;
	//	o[i+i+1]=out[i].im;
	//}

}

double fftout[NFFT*2];
double filtered[NFFT*2];
double sliding[NFFT*2];
double fftlast[NFFT*2];

int curSample=0;
double echo[NFFT*2];

CMusic music;
bool music_on=false;
bool noise_on=false;
bool saw_on=false;
bool square_on=false;
bool oneliner_on=false;
bool uprising_on=false;
bool rawfile_on=false;
bool mic_in=false;

FILE* rawf;

void GenerateSamples()
{
	memcpy(&echo[0], &echo[NFFT], NFFT*sizeof(double));
	for(int i=NFFT;i<NFFT*2;i++)
	{
		echo[i]=0;
	}
	{
		if(mb&1)
		{
			for(int i=NFFT/2;i<NFFT;i++)
			{
				int s=curSample+i;
				double l=lega(s/48000.,s/48000.,(curSample+NFFT/2)/48000.,(curSample+NFFT)/48000.,prevmx*10,mx*10)*.3;
				//double l=lega(s/48000.,s/48000.,(curSample+NFFT/2)/48000.,(curSample+NFFT)/48000.,
				//	3000.+2500.*sin((curSample+NFFT/2)/48000.*M_PI),3000.+2500.*sin((curSample+NFFT)/48000.*M_PI)
				//)*.3;
				//double l=sin(s/48000.*M_PI*880.);
				int idx=i+i;
				echo[idx  ]+=l;
				echo[idx+1]+=l;
			}
		}
		if(mb&4)
		{
			double freq=1./48000.*mx*10.*2.*M_PI;
			for(int i=NFFT/2;i<NFFT;i++)
			{
				int s=curSample+i;
				double l=sin(s*freq)*.3;
				int idx=i+i;
				echo[idx  ]+=l;
				echo[idx+1]+=l;
			}
		}
		if(oneliner_on)
		{
			for(int i=NFFT/2;i<NFFT;i++)
			{
				int t=curSample+i-NFFT/2;
				int il=((t*("16164289"[(t>>13)&7]&15))/12&128)+(((((t>>12)^(t>>12)-2)%11*t)/4|t>>13)&127);
				double l=((il&255)/255.-.5);
				int idx=i+i;
				echo[idx  ]+=l;
				echo[idx+1]+=l;
			}
		}
		if(noise_on)
		{
			for(int i=NFFT/2;i<NFFT;i++)
			{
				int s=curSample+i;
				double t=s/48000.;
				double l=0;
				int idx=i+i;
				seed+=s;l+=.5*frand(seed);
				echo[idx  ]+=l;
				echo[idx+1]+=l;
			}
		}
		if(saw_on)
		{
			for(int i=NFFT/2;i<NFFT;i++)
			{
				int s=curSample+i;
				double l=0;
				int idx=i+i;
				l=s/102.45;
				l=l-int(l)-.5;
				echo[idx  ]+=l;
				echo[idx+1]+=l;
			}
		}
		if(rawfile_on)
		{
			char buf[NFFT*2];
			// int res=fread(&buf[0],1,NFFT*2,rawf);
			// if(res<NFFT*2)fseek(rawf,0,SEEK_SET);
			int bp=0;
			decode_samples(&buf[0],NFFT*2);

			for(int i=NFFT/2;i<NFFT;i++)
			{
				int idx=i+i;
				int val=((buf[bp+1]<<8)+(buf[bp]));
				if(val>32768)
					val=-65535+val;
				float l=val/(65536.);
				bp+=4;
				echo[idx  ]+=l;
				echo[idx+1]+=l;
			}
		}
		if(square_on)
		{
			for(int i=NFFT/2;i<NFFT;i++)
			{
				int s=curSample+i;
				double l=0;
				int idx=i+i;
				l=(int(s/102.45)&1)-.5;
				echo[idx  ]+=l;
				echo[idx+1]+=l;
			}
		}
		if(uprising_on)
		{
			for(int i=NFFT/2;i<NFFT;i++)
			{
				int s=curSample+i;
				double t=s/48000.;
				double l=0;
				int idx=i+i;
				for(int k=0;k<12;k++)
				{
					double ti=t*.02+k/12.;
					ti=ti-int(ti);
					double w=.5-Fabs(ti-.5);
					w=S_Curve(w*2.);
					l+=sin(ti*3*440.*(3.+ti*600.))*.2*w;
				}
				echo[idx  ]+=l;
				echo[idx+1]+=l;
			}
		}
		if(music_on)
		{
			music.GenerateSamples(NFFT/2);
			for(int i=0;i<NFFT/2;i++)
			{
				float l=music.echo[(music.echoPos-NFFT/2+i)%11250*2];
				int idx=i+i+NFFT;
				echo[idx  ]+=l;
				echo[idx+1]+=l;
			}
			
			/*
			for(int i=NFFT/2;i<NFFT;i++)
			{
				int s=curSample+i;
				double t=s/48000.;
				double l=0;
				int idx=i+i;
				//for(int k=0;k<20;k++)l+=sin(t*18000+18000*sin(t*(2+k*.05)))*.05;
				seed+=s;l+=.5*frand(seed);
				//for(int k=0;k<12;k++)
				//{
				//	double ti=t*.02+k/12.;
				//	ti=ti-int(ti);
				//	double w=.5-Fabs(ti-.5);
				//	w=S_Curve(w*2.);
				//	l+=sin(ti*3*440.*(3.+ti*600.))*.2*w;
				//}
				echo[idx  ]+=l;
				echo[idx+1]+=l;
			}*/
		}
		for(int i=NFFT+1;i<NFFT*2;i+=2)
		{
			echo[i]=0;
		}
		return;
	}
}
void Out(double* buf, int count)
{
	int count2=count*2;
	for(int i=0;i<count2;i+=2)
		buf[i+1]=buf[i];
	curSample+=count;
	snd_out_buf(buf, count);
	return;
	for(int i=0;i<count2;i+=2)
	{
		snd_out(buf[i],buf[i]);
	}
}

bool graph=false;
int frame;
bool filter=false;
bool hipass=false;
bool inv=false;

int main()
{
	SetPresentWait(0);
	open_dec("bensound-funnysong.mp3");
	FFT_Init();
	MIC_In_Init(NFFT/2);	
//	StdFreqs();
	//for(int i=0;i<NFFT*2;i++)
	//{
	//	fftout[i]=0;
	//	filtered[i]=0;
	//}
	double t0=Time();
	music.Init();
	music.do_out=false;
	frame=0;
	//graph=true;
	rawf=fopen("out.raw","wb");
	printf("File opened: %i\n", rawf);

	while(true)
	{
		int key;
		int press;
		if(GetKeyEvent(key,press))
		{
			if(press>0)
			{
				if(key==101){}
				else if(key=='f')
				{
					filter=!filter;
				}
				else if(key=='h')
				{
					hipass=!hipass;
				}
				else if(key=='i')
				{
					inv=!inv;
				}
				else if(key=='s')
				{
					saw_on=!saw_on;
				}
				else if(key=='q')
				{
					square_on=!square_on;
				}
				else if(key=='m')
				{
					music_on=!music_on;
				}
				else if(key=='r')
				{
					rawfile_on=!rawfile_on;
				}
				else if(key=='n')
				{
					noise_on=!noise_on;
				}
				else if(key=='o')
				{
					oneliner_on=!oneliner_on;
				}
				else if(key=='u')
				{
					uprising_on=!uprising_on;
				}
				else if(key==4010)
				{
					graph=!graph;
					if(graph)
						printf("Enable waterfall graph\n");
					else
						printf("Waterfall graph OFF\n");
				}
				printf("Key pressed %i %c %i\n", key, key, press);
			}
		}

		double T0=Time();
		bool didOut=false;
		while(snd_bufhealth()<2000+NFFT)
		{
			didOut=true;
			prevmx=mx;
			prevmy=my;
			prevmb=mb;
			GetMouseState(mx, my, mb);
			GenerateSamples();
			if(mic_in)
			{
				float mic_buf[NFFT/2];
				int res=MIC_In_Record((float*)&(mic_buf[0]));
				//fwrite((void*)&(mic_buf[0]), 1, sizeof(double)*NFFT/2,rawf);
				//printf("Mic in: %i\n", res);
				for(int i=0;i<NFFT/2;i++)
				{
					echo[NFFT+i*2]+=mic_buf[i];
					echo[NFFT+i*2+1]+=mic_buf[i];
				}
			}
			if(filter)
			{
				//for(int i=0;i<NFFT*2;i+=2)
				//{
				//	echo[i+1]=0;
				//}
				FFT(echo,fftout);
				if(mb&2)
				{
					int mp=int(mx/2.5)&0xfffffffe;
					if(mp<NFFT)
					{
						fftout[mp]=NFFT/4.f;
						fftout[mp+1]=1.;
						//fftout[NFFT*2-mp]=NFFT/81f;
						//fftout[NFFT*2-mp-1]=1.;
					}
				}
				for(int i=0;i<NFFT*2;i+=2)
				{
					double tmp=fftout[i];
					fftout[i]=fftout[i+1];
					fftout[i+1]=tmp;
					//fftout[i+1]=-fftout[i+1];
				}

				
				int cut=mx;
				if(cut>NFFT/2)cut=NFFT/2;
				if(cut<0)cut=0;
				
				// if(inv)
				// {
				// 	for(int i=0;i<cut;i++)
				// 	{
				// 		fftout[i*2]=0.;
				// 		fftout[i*2+1]=0.;
				// 		fftout[NFFT*2-i*2]=0.;
				// 		fftout[NFFT*2-i*2-1]=0.;
				// 	}
				// }
				// else
				// {
				// 	for(int i=cut;i<NFFT/2;i++)
				// 	{
				// 		fftout[i*2]=0.;
				// 		fftout[i*2+1]=0.;
				// 		fftout[NFFT*2-i*2]=0.;
				// 		fftout[NFFT*2-i*2-1]=0.;
				// 	}
				// }
				
				float R=NFFT*.02;
				int cut0=cut-R;if(cut0<0)cut0=0;
				int cut1=cut+R;if(cut1>NFFT/2)cut1=NFFT/2;
				for(int i=0;i<cut0;i++)
				{
					fftout[i*2]=0.;
					fftout[i*2+1]=0.;
					fftout[NFFT*2-i*2]=0.;
					fftout[NFFT*2-i*2-1]=0.;
				}
				for(int i=cut1;i<=NFFT/2;i++)
				{
					fftout[i*2]=0.;
					fftout[i*2+1]=0.;
					fftout[NFFT*2-i*2]=0.;
					fftout[NFFT*2-i*2-1]=0.;
				}
				for(int i=cut0;i<=cut1;i++)
				{
					float sc=S_Curve(1.-Fabs((i-cut)/R));
					fftout[i*2]*=sc;
					fftout[i*2+1]*=sc;
					fftout[NFFT*2-i*2]*=sc;
					fftout[NFFT*2-i*2+1]*=sc;
				}
				
				/*
				int m=mx*2;
				if(m<0)m=0;
				for(int i=NFFT;i>m;i--)
				{
					fftout[i]=fftout[i-m];
					fftout[(NFFT-1)*2-(i)]=fftout[(NFFT-1)*2-(i-m)];
				}
				for(int i=0;i<m;i++)
				{
					fftout[i]=0;
					fftout[(NFFT-1)*2-(i)]=0;
				}*/
				
				//for(int i=0;i<200;i++)
				//{
				//	fftout[i]=0.;
				//	fftout[NFFT*2-i]=0.;
				//}
				FFT(fftout,filtered);
				double mul=1./(NFFT-1);
				for(int i=0;i<NFFT*2;i+=2)
				{
					//double tmp=filtered[i];
					double res=filtered[i+1]*mul;
					filtered[i]=res;
					//filtered[i+1]=res;
					//double res=filtered[i]*mul;
					//filtered[i]  =res;
					////filtered[i+1]=res;
				}
				if(hipass)
				for(int i=0;i<NFFT;i++)
				{
					filtered[i+i]=echo[i+i]-filtered[i+i];
					//filtered[i+i+1]=filtered[i+i];
				}
				memmove(&sliding[0],&sliding[NFFT],sizeof(double)*NFFT);

				double t=1.;
				double dt=1./double((NFFT-1)/2);
				for(int i=NFFT/2;i<NFFT;i++)
				{
					double w=S_Curve(t);
					t-=dt;
					sliding[i+i  ]=filtered[i+i]*w;
					//sliding[i+i+1]=filtered[i+i+1]*w;
				}
				t=0.;
				for(int i=0;i<NFFT/2;i++)
				{
					double w=S_Curve(t);
					t+=dt;
					sliding[i+i  ]+=filtered[i+i  ]*w;
					//sliding[i+	i+1]+=filtered[i+i+1]*w;
				}
				Out(sliding,NFFT/2);
				for(int i=0;i<NFFT*2;i+=2)
				{
					filtered[i+1]=0;
				}
				FFT(filtered,fftlast);
			}
			else
			{
				FFT(echo,fftlast);
				Out(echo,NFFT/2);
			}

			if(graph)
			{
				for(int i=0;i<NFFT;i+=2)
				{
					g.clear();
					double re=fftlast[i];
					double lvl;
					double im=fftlast[i+1];
					lvl=sqrt(sqrt(re*re+im*im))/4.;
					//if(lvl<0)lvl=-lvl;
					//lvl=lvl>1?1.:lvl;
					float lvlr=lvl*960.;lvlr=lvlr>255.?255.:lvlr;
					float lvlg=lvl*420.;lvlg=lvlg>255.?255.:lvlg;
					float lvlb=lvl*220.;lvlb=lvlb>255.?255.:lvlb;
					int c=
						((int(lvlb))<<16)|
						((int(lvlg))<<8)|
						int(lvlr);
					PutPixel((frame)%SW,480-112-i/2,c);
				}
				frame++;
			}
		}

		if(graph)
		{
			g.clear();
			g.M(0,0);
			g.l(SW,0);
			g.l(0,110);
			g.l(-SW,0);
			g.close();
			g.M(0,480);
			g.l(SW,0);
			g.l(0,-110);
			g.l(-SW,0);
			g.close();
			g.fin();
			g.rgba(.2,0,.5,1);
			g.fill2();
			g.clear();

			Present();
			continue;
		}

		g.gray(0);
		g.clear();
		g.M(0,0);g.l(SW,0);g.l(0,480);g.l(-SW,0);g.close();g.fin();
		g.fill1();


		double dT=Time()-T0;
		char s[128];
		snprintf(s,128,"Processing time: %f",dT);
		stext(s,10,10,0xffffffff);

		g.clear();
		g.miterlim(1.01);
		int nG=SW;
		if(nG>NFFT)nG=NFFT;
		for(int i=0;i<nG/2;i++)
		{
			double lvl=echo[i*2];
			g.L(i*2,lvl*200+240);
		}
		g.fin();
		g.width(2.,1.);
		g.rgb(1.0,0.5,0.0);
		g.stroke();

		g.clear();
		for(int i=0;i<nG/2;i++)
		{
			double lvl=sliding[i*2];
			g.L(i*2,lvl*100+240);
		}
		g.fin();
		g.width(1.,1.);
		g.rgb(0.0,1.0,1.0);
		g.stroke();
		g.clear();

		g.clear();
		for(int i=0;i<NFFT;i++)
		{
			double lvl=0;
			double re=fftlast[i*2  ];
			double im=fftlast[i*2+1];
			lvl=sqrt(sqrt(re*re+im*im));
			lvl*=16.;
			if(lvl>240)lvl=240;
			g.M(i+.5,480);
			g.l(0,-lvl);
		}
		g.fin();
		g.rgb(1.,.2,.1);
		g.width(6.,1.);
		g.stroke();
		g.rgb(1.,.8,.5);
		g.width(1.,1.);
		g.stroke();
		g.clear();

		char ss[64];
		snprintf(ss,64,"Buffer health: % 5i", snd_bufhealth());
		stext(ss,10,470,0xffffff00);
		Present();
	}
	fclose(rawf);
	MIC_In_Done();	
	close_dec();
	return 0;
}