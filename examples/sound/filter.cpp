#define SW 1024
#define G_SCREEN_WIDTH SW
#define G_SCREEN_SCALE 1
#define G_SCREEN_MODE 1

#define NFFT 2048

#include <stdio.h>
#include "sound.h"
#include "graphics.h"
#include "../asteroids/asters_music.h"
#include "../ui/gui/GUI.h"
#include "../ws.h"

#ifdef __SCINC__
#define const
#endif

#include "FFT_rc.h"

#include "mp3.h"
#include "mic_in.h"

#pragma STACK_SIZE 655360

#define M_PI 3.141592654

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

//#define ALMOST_ONE 0.9999999999999

/*double mod(double a, double b)
{
	int r=a/b;
	return a-b*r;
}*/

float fract(float a)
{
	return a-int(a);
}

double lega(double globalTime, double time, double t0, double t1, double f0, double f1)
{
	if(time<t0)
		time=t0;
	else if(time>t1)
		time=t1;
	double dt=t1-t0;

	double t=(time-t0)/dt;

	double p0=fract(t0*f0);
	double p1=fract(t1*f1);

	double y0=t0*f0;
	y0+=-fract(y0)+p0;
	double y1=y0+(f1+f0)/2.*dt;
	y1+=-fract(y1)+p1;

	double invt=1.-t;

	double y=y0*invt*invt*invt
			+3.*(y0+dt/3.*f0)*invt*invt*t
			+3.*(y1-dt/3.*f1)*invt*t*t
			+y1*t*t*t;

	y=y-double(int(y));
	double v=sin(y*2.*M_PI);

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

float Bell_Curve(float t)
{
	float t2=t*t;
	return t2*16.*(t2-2.*t+1.);
}

void FFT(double* in, double* o)
{
	cplx buf[NFFT];
	//cplx out[NFFT];
	for(int i=0;i<NFFT;i++)
	{
		buf[i].re=in[i+i  ];
		buf[i].im=0;//in[i+i+1];
	}
	//memcpy(&buf[0],&in[0],sizeof(cplx)*NFFT);
	fft((cplx*)(&buf[0]), (cplx*)(void*)o, NFFT);
	//memcpy(&o[0],&out[0],sizeof(cplx)*NFFT);

	//for(int i=0;i<NFFT;i++)
	//{
	//	//o[i]=sqrt(out[i].im*out[i].im+out[i].re*out[i].re);
	//	o[i+i  ]=out[i].re;
	//	o[i+i+1]=out[i].im;
	//}

}

void revFFT(double* in, double* o)
{
	fft((cplx*)(void*)in, (cplx*)(void*)o, NFFT);
}

double fftout[NFFT*2];
double filtered[NFFT*2];
double sliding[NFFT*2];
double fftlast[NFFT*2];

int curSample=0;
double echo[NFFT*2];

CMusic music;
int music_on=false;
int eq_on=false;
int shift_on=false;
int noise_on=false;
int saw_on=false;
int square_on=false;
int sine_on=false;
int sine_lega_on=false;
int oneliner_on=false;
int uprising_on=false;
double upT[16]={};
int rawfile_on=false;
int mic_in=false;
float shift=0.;

FILE* rawf;

#define HASH_K 1103515245

int Hash(int x)
{
	x=((x>>8)^x)*HASH_K;
	x=((x>>8)^x)*HASH_K;
	x=((x>>8)^x)*HASH_K;
	return x;
}

float GradNoise(float x)
{
	int x0=x;
	int x1=x0+1;
	float h0=float(Hash(x0))/float(0x7fffffff);
	float h1=float(Hash(x1))/float(0x7fffffff);
	float w0=(x-x0);
	float w1=1.-w0;
	return w0*w0*w1*h1-w1*w1*w0*h0;
}

float SndNoise(float t)
{
	return GradNoise(t/10.);
}

float mfreq=0.;
float cfreq=0.;
float prevmfreq=0.;

void GenerateSamples()
{
	memcpy(&echo[0], &echo[NFFT], NFFT*sizeof(double));
	for(int i=NFFT;i<NFFT*2;i++)
	{
		echo[i]=0;
	}
	{
		if(sine_lega_on)
		{
			for(int i=NFFT/2;i<NFFT;i++)
			{
				int s=curSample+i;
				float res=48000./NFFT;
				double l=lega(s/48000.,s/48000.,(curSample+NFFT/2)/48000.,(curSample+NFFT)/48000.,prevmfreq,mfreq)*.3;
				//double l=lega(s/48000.,s/48000.,(curSample+NFFT/2)/48000.,(curSample+NFFT)/48000.,
				//	3000.+2500.*sin((curSample+NFFT/2)/48000.*M_PI),3000.+2500.*sin((curSample+NFFT)/48000.*M_PI)
				//)*.3;
				//double l=sin(s/48000.*M_PI*880.);
				int idx=i+i;
				echo[idx  ]+=l;
				echo[idx+1]+=l;
			}
			prevmfreq=mfreq;
		}
		if(sine_on)
		{
			float res=48000./NFFT;
			for(int i=NFFT/2;i<NFFT;i++)
			{
				int s=curSample+i;
				double a=s/48000.*mfreq;
				a=a-int(a);
				double l=sin(a*2*M_PI)*.3;
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
				//seed=s;l+=.5*frand(seed);
				l=SndNoise(s);
				echo[idx  ]+=l;
				echo[idx+1]+=l;
			}
		}
		if(saw_on)
		{
			for(int i=NFFT/2;i<NFFT;i++)
			{
				int s=curSample+i;
				double l=float(s)*mfreq/24000.;
				l=(l-float(int(l))-.5)*.3;
				int idx=i+i;
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
				double l=float(s)*mfreq/24000.;
				l=((l-float(int(l))<.5)-.5)*.3;
				int idx=i+i;
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

				for(int fi=0;fi<8;fi++)
				{				
					float t0=t*.01+fi/8.;
					float s0=(t0-int(t0));
					float f0=220.+220.*16.*s0;
					float w0=Bell_Curve(s0);
					upT[fi]+=1./48000.*2.*M_PI*f0;
					l+=sin(upT[fi])*.1*w0;
				}

				echo[idx  ]+=l;
				echo[idx+1]+=l;
			}
		}
		if(music_on)
		{
			int nSamples=NFFT/2;
			music.GenerateSamples(nSamples);

			if(0)
			{
				int start=music.echoPos-nSamples;
				int count=nSamples;
				int start1=0;
				if(start<0)
				{
					count+=start;
					for(int i=0;i< -start;i++)
					{
						float l=music.echo[(12000+start+i)*2];
						int idx=i+i+NFFT;
						echo[idx  ]+=l;
						echo[idx+1]+=l;
					}
					start1=-start;
				}
				for(int i=0;i<count;i++)
				{
					float l=music.echo[(start+i)*2];
					int idx=start1*2+i+i+NFFT;
					echo[idx  ]+=l;
					echo[idx+1]+=l;
				}
			}

			int idx=NFFT;
			for(int i=-NFFT/2;i<0;i++)
			{
				float l=music.echo[(music.echoPos+i+12000)%12000*2];
				//int idx=i+i+NFFT*2;
				echo[idx  ]+=l;
				echo[idx+1]+=l;
				idx+=2;
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
	CSound s;
	s.snd_out_buf(buf, count);
	return;
	for(int i=0;i<count2;i+=2)
	{
		s.snd_out(buf[i],buf[i]);
	}
}

bool graph=false;
int frame;
int filter=false;
int hipass=false;
int inv=false;
int cut_on=false;

float interp(float y0, float y1, float t)
{
	t=(3.-2.*t)*t*t;
	return y0+(y1-y0)*t;
}

float EQ[11]={1,1,1,1,1,1,1,1,1,1,1};
int EQF[11] ={0,0,0,0,0,0,0,0,0,0,NFFT/2-1};

CWinSys wsys;
int mainWin;

int InitWS()
{
	//SetEventCallback([](SScincEvent e)->int
	//{
	//	if(e.type=='WMOV')
	//	{
	//		printf("%i,%i %ix%i\n",e.x,e.y,e.z,e.h);
	//	}
	//	if(e.type=='WKIL')
	//	{
	//		printf("Window killed\n");
	//		exit(0);
	//	}
	//	return 0;
	//});
	mainWin=wsys.CreateWindow(640,480,3,3,1);
	wsys.SetWindowPos(mainWin,200,200);
	return 0;
}

//int dummy=InitWS();

void Present()
{
	wsys.Present(mainWin);
}

int main()
{
	InitWS();
	CGUI gui;
	gui.Init();
	int ctldy=18;
	int ctly=5-ctldy;
	gui.AddCheck("1lineR",  10,ctly+=ctldy,65,15,0,&oneliner_on);
	gui.AddCheck("Music",   10,ctly+=ctldy,65,15,0,&music_on);
	gui.AddCheck("Square",  10,ctly+=ctldy,65,15,0,&square_on);
	gui.AddCheck("Sawtooth",10,ctly+=ctldy,65,15,0,&saw_on);
	gui.AddCheck("Sine",    10,ctly+=ctldy,65,15,0,&sine_on);
	gui.AddCheck("SineL",   10,ctly+=ctldy,65,15,0,&sine_lega_on);
	gui.AddCheck("MicIn",   10,ctly+=ctldy,65,15,0,&mic_in);
	gui.AddCheck("MP3",     10,ctly+=ctldy,65,15,0,&rawfile_on);

	ctly=5-ctldy;
	gui.AddCheck("Filter",80,ctly+=ctldy,65,15,0,&filter);
	gui.AddCheck("EQ"    ,80,ctly+=ctldy,65,15,0,&eq_on);
	gui.AddCheck("Shift" ,80,ctly+=ctldy,65,15,0,&shift_on);
	gui.AddCheck("Cut"   ,80,ctly+=ctldy,65,15,0,&cut_on);
	gui.AddCheck("InvC"  ,80,ctly+=ctldy,65,15,0,&inv);

	gui.AddSlide("Shift",   200,10,401,15,0,&shift, false,0,200);
	gui.AddSlide("Freq",    200,30,401,15,0,&mfreq, false,0,48000./NFFT*100);
	gui.AddSlide("Cut Freq",200,50,401,15,0,&cfreq, false,0,NFFT/2);

	{
		int cx0=710;
		int dx=20;
		gui.AddSlide("", cx0+=dx,10,15,101,'EQ[0]',&EQ[ 0],true,8,0);
		gui.AddSlide("", cx0+=dx,10,15,101,'EQ[1]',&EQ[ 1],true,8,0);
		gui.AddSlide("", cx0+=dx,10,15,101,'EQ[2]',&EQ[ 2],true,8,0);
		gui.AddSlide("", cx0+=dx,10,15,101,'EQ[3]',&EQ[ 3],true,8,0);
		gui.AddSlide("", cx0+=dx,10,15,101,'EQ[4]',&EQ[ 4],true,8,0);
		gui.AddSlide("", cx0+=dx,10,15,101,'EQ[5]',&EQ[ 5],true,8,0);
		gui.AddSlide("", cx0+=dx,10,15,101,'EQ[6]',&EQ[ 6],true,8,0);
		gui.AddSlide("", cx0+=dx,10,15,101,'EQ[7]',&EQ[ 7],true,8,0);
		gui.AddSlide("", cx0+=dx,10,15,101,'EQ[8]',&EQ[ 8],true,8,0);
		gui.AddSlide("", cx0+=dx,10,15,101,'EQ[9]',&EQ[ 9],true,8,0);
		gui.AddSlide("", cx0+=dx,10,15,101,'EQ[A]',&EQ[10],true,8,0);
	}

	printf("\nEQ bands calc:\n");
	float eq0=128;
	for(int i=1;i<10;i++)
	{
		printf(" %f %i", eq0, int(eq0/(48000./NFFT)));
		EQF[i]=int(eq0/(48000./NFFT)*1.5)+1;
		eq0+=eq0*.75;
	}
	printf("\n");

	printf("\nEQ bands:\n");
	for(int i=0;i<11;i++)
	{
		printf(" %i", EQF[i]);
	}
	printf("\n");

	{
		printf("-----------\n");
		int i0=EQF[0];
		for(int i=1;i<11;i++)
		{
			int i1=EQF[i];
			printf("%i: %i %i \n", i, i0, i1);
			for(int j=i0;j<i1;j++)
			{
				float t=(j-i0)/float(i1-i0);
				float val=interp(EQ[i-1],EQ[i],t);
				fftout[i*2         ]*=val;
				fftout[i*2+1       ]*=val;
				fftout[NFFT*2-i*2  ]*=val;
				fftout[NFFT*2-i*2-1]*=val;
			}
			i0=i1;
		}
		printf("\n");
	}

	SetPresentWait(0);
	//open_dec("bensound-funnysong.mp3");
	//open_dec("money.mp3");
	open_dec("bear.mp3");
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
		SScincEvent event;
		while(GetScincEvent(event))
		{
			if(gui.Event(event))
				continue;
			if(IsMouseEvent(event.type))
			{
				// mx=event._1;
				// my=event._2;
				// mb=event._0;
			}
		}
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
				else if(key=='a')
				{
					mic_in=!mic_in;
				}
				else if(key=='s')
				{
					saw_on=!saw_on;
				}
				else if(key=='0')
				{
					close_dec();
					open_dec("money.mp3");
				}
				else if(key=='d')
				{
					shift_on=!shift_on;
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
		int gscount=0;
		CSound s;
		while(s.snd_bufhealth()<1024*2+NFFT)
		{
			gscount++;
			didOut=true;
			GenerateSamples();
			{
				float mic_buf[NFFT/2];
				int res=MIC_In_Record((float*)&(mic_buf[0]));
				//fwrite((void*)&(mic_buf[0]), 1, sizeof(double)*NFFT/2,rawf);
				//printf("Mic in: %i\n", res);
				if(mic_in)
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
				//if(mb&2)
				//{
				//	int mp=int(mx/2.5)&0xfffffffe;
				//	if(mp<NFFT)
				//	{
				//		fftout[mp]=NFFT/4.f;
				//		fftout[mp+1]=1.;
				//		//fftout[NFFT*2-mp]=NFFT/81f;
				//		//fftout[NFFT*2-mp-1]=1.;
				//	}
				//}
				for(int i=0;i<NFFT*2;i+=2)
				{
					double tmp=fftout[i];
					fftout[i]=fftout[i+1];
					fftout[i+1]=tmp;
					//fftout[i+1]=-fftout[i+1];
				}

				if(eq_on)
				{
					int i0=EQF[0];
					for(int i=1;i<11;i++)
					{
						int i1=EQF[i];
						float dt=float(i1-i0);
						for(int j=i0;j<i1;j++)
						{
							float t=(j-i0)/dt;
							float val=interp(EQ[i-1],EQ[i],t);
							fftout[j*2         ]*=val;
							fftout[j*2+1       ]*=val;
							fftout[NFFT*2-j*2  ]*=val;
							fftout[NFFT*2-j*2-1]*=val;
						}
						i0=i1;
					}
					//for(int i=0;i<NFFT/2;i++)
					//{
					//	int idx=i/(NFFT/2/8);
					//	float t=(i-idx*(NFFT/2/8))/float(NFFT/2/8);
					//	float val=interp(EQ[idx],EQ[idx+1],t);
					//	fftout[i*2         ]*=val;
					//	fftout[i*2+1       ]*=val;
					//	fftout[NFFT*2-i*2  ]*=val;
					//	fftout[NFFT*2-i*2-1]*=val;
					//}
				}

				int cut=100000;
				if(cut>NFFT/2)cut=NFFT/2;
				if(cut<0)cut=0;
				cut=cfreq;

				if(cut_on)
				{
				if(1)// low or high pass
				{
					if(inv)
					{
						for(int i=0;i<cut;i++)
						{
							fftout[i*2]=0.;
							fftout[i*2+1]=0.;
							fftout[NFFT*2-i*2]=0.;
							fftout[NFFT*2-i*2-1]=0.;
						}
					}
					else
					{
						for(int i=cut;i<NFFT/2;i++)
						{
							fftout[i*2]=0.;
							fftout[i*2+1]=0.;
							fftout[NFFT*2-i*2]=0.;
							fftout[NFFT*2-i*2-1]=0.;
						}
					}
				}
				}

				if(0) // Band
				{
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
				}

				if(shift_on)
				{
					int m=((int)(shift/4))*4;
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
					}
				}
				
				//for(int i=0;i<200;i++)
				//{
				//	fftout[i]=0.;
				//	fftout[NFFT*2-i]=0.;
				//}
				revFFT(fftout,filtered);
				double mul=1./(NFFT);
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
					filtered[i+1]=filtered[i];
				}
				revFFT(filtered,fftlast);
			}
			else
			{
				Out(echo,NFFT/2);
				FFT(echo,fftlast);
			}

			if(graph)
			{
				for(int i=0;i<NFFT;i+=2)
				{
					g.clear();
					double re=fftlast[i];
					double lvl;
					double im=fftlast[i+1];
					lvl=(sqrt(re*re+im*im))/16.;
					if(lvl<0)lvl=-lvl;
					lvl=lvl>1?1.:lvl;
					float lvlr=lvl*960.;lvlr=lvlr>255.?255.:lvlr;
					float lvlg=lvl*420.;lvlg=lvlg>255.?255.:lvlg;
					float lvlb=lvl*220.;lvlb=lvlb>255.?255.:lvlb;
					int c=
						((int(lvlb))<<16)|
						((int(lvlg))<<8)|
						int(lvlr)|
						0xff000000;
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


		//double dT=Time()-T0;
		//char s[128];
		//snprintf(s,128,"Processing time: %f",dT);
		//stext(s,10,10,0xffffffff);

		g.clear();
		g.miterlim(1.01);
		int nG=SW;
		if(nG>NFFT)nG=NFFT;
		for(int i=0;i<nG;i++)
		{
			double lvl=echo[i+i];
			g.L(i,lvl*200+240);
		}
		g.fin();
		g.width(2.,2.);
		if(filter)
			g.rgb(1.0,0.4,0.4);
		else
			g.rgb(1.0,0.7,0.0);
		g.stroke();

		if(filter)
		{
			g.clear();
			for(int i=0;i<nG;i++)
			{
				double lvl=sliding[i+i];
				g.L(i,lvl*200+240);
			}
			g.fin();
			g.width(2.,2.);
			g.rgb(0.0,1.0,1.0);
			g.stroke();
			g.clear();
		}

		//g.clear();
		g.rgb(1.,.8,.5);
		for(int i=0;i<nG;i++)
		{
			double lvl=0;
			double re=fftlast[i*2  ];
			double im=fftlast[i*2+1];
			lvl=sqrt(sqrt(re*re+im*im));
			lvl*=16.;
			if(lvl>240)lvl=240;
			//g.M(i+.5,480);
			//g.l(0,-lvl);
			g.hairline(i,480,i,480-lvl);
		}
		//g.fin();
		//g.rgb(1.,.2,.1);
		//g.width(2.,1.);
		//g.stroke();
		//g.rgb(1.,.8,.5);
		//g.width(1.,1.);
		//g.stroke();
		//g.clear();



		char ss[64];
		snprintf(ss,64,"Buffer health: % 5i  gs:%i", s.snd_bufhealth(), gscount);
		stext(ss,10,470,0xffffff00);
		snprintf(ss,64,"mfreq: %f", mfreq);
		stext(ss,10,460,0xffffff00);
		gui.Render(g);

		g.rgba32(0xffffffff);
		g.clear();
		for(int i=1;i<11;i++)
		{
			float y1=-EQ[i  ]*50+110;
			float y0=-EQ[i-1]*50+110;
			float x0=(i-1)*20+740;
			float dj=1./10.;
			float y=y0;
			float x=x0;
			g.M(x,y);
			for(float j=dj;j<1.001;j=j+dj)
			{
				float yn=interp(y0,y1,j);
				float xn=x+dj*20.;
				g.L(xn,yn);
				x=xn;
				y=yn;
			}
		}
		g.fin();
		g.width(2.,2.);
		g.stroke();

		Present();
		{
			CSound s;
			s.Poll();
		}

	}
	fclose(rawf);
	MIC_In_Done();	
	close_dec();
	return 0;
}