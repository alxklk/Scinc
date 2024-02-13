#include "sound.h"
#include "graphics.h"

#include "../ws.h"

#ifdef __SCINC__
#define const
#endif

#define NFFT 1024

#include "FFT_rc.h"

#pragma STACK_SIZE 655360

#define M_PI 3.141592654

int mx;
int my;
int mb;
int prevmx;
int prevmy;
int prevmb;

double freqs[12]={
	261.626, 277.183, 293.665, 311.127, 329.628, 349.228, 369.994, 391.995, 415.305, 440.000, 466.164, 493.883
};

struct Note
{
	double f;  // freq
	int t0; // starh
	int t1; // end
};

int seed=374693645;

int irand(int& seed)
{
	seed=(seed*1103515245+12345)%0x7ffffff;
	return seed;
}

float frand(int& seed)
{
	return (irand(seed)>>11)/65536.0;
}

bool vibratto;
bool cut;
bool fade;
double echoVal;

#define NN 16
class Polyphony
{
public:
	Note notes[NN];
	int ns;
	void Init()
	{
		ns=0;
		for(int i=0;i<NN;i++)
		{
			notes[i].t0=-100;
			notes[i].t1=-100;
		}
	}
	void AddNote(double f, double len)
	{
		for(int i=0;i<NN;i++)
		{
			if(ns>notes[i].t1)
			{
				notes[i].f=f;
				notes[i].t0=ns+4800;
				notes[i].t1=ns+4800+len*48000;
				break;
			}
		}
	}
	void RenderSpan(int nSamples, double* echo, int echoPos, int echoLen)
	{
		for(int j=0;j<NN;j++)
		{
			Note& n=notes[j];
			if(ns>n.t1)
				continue;
			int cs=ns;
			for(int i=0;i<nSamples;i++)
			{

				if(cs>n.t1)
					break;
				if(cs>n.t0)
				{
					double t=(cs-n.t0)/48000.;
					//double s=sin((t+sin(cs*.0005)*0.0005)*n.f*M_PI*2);
					if(vibratto)
						t+=sin(t*M_PI*5.)*0.0007*t;
					double s=sin(t*n.f*M_PI*2);
					if(cut)
					{
						if(s>0.4)s=0.4;else if(s<-0.4)s=-.4;
					}
					else
					{
						s*=0.35;
					}

					//if(t<0.01)s*=t*100.;
					if(fade)
					{
						s*=(1. -(cs-n.t0)/double(n.t1-n.t0));
					}
					else
					{
						double te=(n.t1-cs)/48000.;if(te<0.1)s*=te*10.;
					}
					double b=.5+sin(cs*.0001)*.25;
					int ep=((echoPos+i)%echoLen)*2;
					double l=s*b     +echo[ep  ];
					double r=s*(1.-b)+echo[ep+1];
					if(l>.9)l=0.9;else if(l<-0.9)l=-.9;
					if(r>.9)r=0.9;else if(r<-0.9)r=-.9;
					echo[ep  ]=l;
					echo[ep+1]=r;
				}
				cs++;
			}
		}
		ns+=nSamples;
	}
};

int cursnd;

Graph g;

Polyphony notes;

#define EL 16384

#define ALMOST_ONE 0.9999999999999

double mod(double a, double b)
{
	int r=a/b;
	return a-b*r;
}

double Fabs(double x)
{
	return x<0?-x:x;
}


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

float S_Curve(float x)
{
	return x*x*(-2.0*x+3.0);
}

class CSnd
{
public:
	int sample;
	int echoPos;
	double* echo;
	void Init()
	{
		printf("Init echo\n");
		sample=0;
		echoPos=0;
		echoVal=0.5;
		vibratto=true;
		cut=true;
		fade=true;
		echo=(double*)malloc(sizeof(double)*2*EL);
		for(int i=0;i<EL*2;i++)echo[i]=0;
	}
	void GenerateSamples(int nSamples)
	{
		if(!mb)
		for(int i=0;i<nSamples;i++)
		{
			int ep=((echoPos+i)%EL)*2;
			double l=echo[ep  ];
			double r=echo[ep+1];
			echo[ep  ]=r*echoVal;
			echo[ep+1]=l*echoVal;
		}

		if(mb)
		{
			for(int i=0;i<nSamples;i++)
			{
				int ep=(echoPos+i)%EL*2;
				echo[ep  ]=0;
				echo[ep+1]=0;
			}

			if(mb&1)
			{
				for(int i=0;i<nSamples;i++)
				{
					int s=sample+i;
					double l=lega(s/48000.,s/48000.,sample/48000.,(sample+nSamples)/48000.,prevmx*10,mx*10)*.3;
					int ep=((echoPos+i)%EL)*2;
					echo[ep  ]+=l;
					echo[ep+1]+=l;
				}
			}

			if(mb&2)
			{
				for(int i=0;i<nSamples;i++)
				{
					int t=(sample+i);
					int il=((t*("16164289"[(t>>13)&7]&15))/12&128)+(((((t>>12)^(t>>12)-2)%11*t)/4|t>>13)&127);
					double l=((il&255)/255.-.5);
					int ep=((echoPos+i)%EL)*2;
					echo[ep  ]+=l;
					echo[ep+1]+=l;
				}
			}

			if(mb&4)
			{
				double f=mx*4;
				for(int i=0;i<nSamples;i++)
				{
					int s=sample+i;
					int ep=(echoPos+i)%EL*2;
					double t=s/48000.;
					double l=0;
					//for(int k=0;k<20;k++)l+=sin(t*2000+2000*sin(t*(2+k*.05)))*.05;
					l+=.5*frand(seed);
					//for(int k=0;k<12;k++)
					//{
					//	float ti=t*.02+k/12.;
					//	ti=ti-int(ti);
					//	float w=.5-Fabs(ti-.5);
					//	w=S_Curve(w*2.);
					//	l+=sin(ti*3*440.*(3.+ti*600.))*.2*w;
					//}
					echo[ep  ]+=l;
					echo[ep+1]+=l;
				}
			}
			for(int i=0;i<nSamples;i++)
			{
				int ep=(echoPos+i)%EL*2;
				CSound s; s.snd_out(echo[ep],echo[ep+1]);
			}
			echoPos=(echoPos+nSamples-1)%EL;
			sample+=nSamples;
			return;
		}
		else notes.RenderSpan(nSamples,&echo[0],echoPos,EL);
		for(int i=0;i<nSamples;i++)
		{
			int ep=(echoPos+i)%EL*2;
			CSound s; s.snd_out(echo[ep],echo[ep+1]);
		}
		echoPos=(echoPos+nSamples)%EL;
		sample+=nSamples;
	}
};

CSnd snd;

int StartsWith(const char*s, const char* p)
{
	int i=0;
	while(true)
	{
		if((s[i]==0)||(p[i]==0))
			return i;
		else if(s[i]==p[i])
			i++;
		else
			return 0;
	}
}

class MelodyProcessor
{
	const char* melody;
	int position;
	int wait;
	bool error;
	double deflen;
	int defoct;
	double mul;
	double tempo;
public:
	const char* name;
	void Render()
	{
		stext(name,200,20,0xffffffff);
		stext("Press 1-9 for other melodies",160,30,0xff808080);
		char s[2];
		s[1]=0;
		int x=16;
		int y=50;
		int i=0;
		while(true)
		{
			char c=melody[i];
			if(c==0)
				break;
			s[0]=c;
			i++;
			stext(s,x,y,i<=position?0xffffff80:0xff80c0d0);
			x+=6;
			if((x>500)&&((c==' ')||(c==',')))
			{
				x=10;
				y+=10;
			}
		}
	}

	void Init(int n)
	{
		tempo=2;
		mul=1;
		if     (n==0){name="     Gamma";tempo=.5;mul=.25; deflen=1./2.;defoct=7;melody="c c# d d# e f f# g g# a a# b p b a# a g# g f# f e d# d c# c p";}
		else if(n==1){name="   Nokia tune"; mul=8;deflen=1./4.; defoct=1;melody="2p 16e2 16d2 8#f 8#g 16#c2 16b 8d 8e 16b 16a 8#c 8e 2a 2p";}
		else if(n==2){name="     Nyan Cat";tempo=3.0;mul=.5; deflen=1./16.;defoct=5;melody="8p,16d#6,16e6,8f#6,8b6,16d#6,16e6,16f#6,16b6,16c#7,16d#7,16c#7,16a#6,8b6,8f#6,16d#6,16e6,8f#6,"
		"8b6,16c#7,16a#6,16b6,16c#7,16e7,16d#7,16e7,16c#7,8f#6,8g#6,16d#6,16d#6,16p,16b,16d6,16c#6,16b,16p,8b,8c#6,8d6,16d6,16c#6,16b,16c#6,16d#6,16f#6,16g#6,16d#6,16f#6,16c#6,16d#6,16b,16c#6,16b,8d#6,8f#6,16g#6,16d#6,16f#6,16c#6,16d#6,16b,16d6,16d#6,16d6,16c#6,16b,16c#6,8d6,16b,16c#6,16d#6,16f#6,16c#6,16d#6,16c#6,16b,8c#6,8b,8c#6,8f#6,8g#6,16d#6,16d#6,16p,16b,16d6,16c#6,16b,16p,8b,8c#6,8d6,16d6,16c#6,16b,16c#6,16d#6,16f#6,16g#6,16d#6,16f#6,16c#6,16d#6,16b,16c#6,16b,8d#6,8f#6,16g#6,16d#6,16f#6,16c#6,16d#6,16b,16d6,16d#6,16d6,16c#6,16b,16c#6,8d6,16b,16c#6,16d#6,16f#6,16c#6,16d#6,16c#6,16b,8c#6,8b,8c#6,8b,16f#,16g#,8b,16f#,16g#,16b,16c#6,16d#6,16b,16e6,16d#6,16e6,16f#6,8b,8b,16f#,16g#,16b,16f#,16e6,16d#6,16c#6,16b,16f#,16d#,16e,16f#,8b,16f#,16g#,8b,16f#,16g#,16b,16b,16c#6,16d#6,16b,16f#,16g#,16f#,8b,16b,16a#,16b,16f#,16g#,16b,16e6,16d#6,16e6,16f#6,8b,8a#,8b,16f#,16g#,8b,16f#,16g#,16b,16c#6,16d#6,16b,16e6,16d#6,16e6,16f#6,8b,8b,16f#,16g#,16b,16f#,16e6,16d#6,16c#6,16b,16f#,16d#,16e,16f#,8b,16f#,16g#,8b,16f#,16g#,16b,16b,16c#6,16d#6,16b,16f#,16g#,16f#,8b,16b,16a#,16b,16f#,16g#,16b,16e6,16d#6,16e6,16f#6,8b,8c#6";}
		else if(n==3){name="    Children"; tempo=1.5; mul=1; deflen=1./4; defoct=5;melody="8p, f.6, 1p, g#6, 8g6, d#.6, 1p, g#6, 8g6, c.6, 1p,"" g#6, 8g6, g#., 1p, 16f, 16g, 16g#, 16c6, f.6, 1p, g#6, 8g6, d#.6, 1p, 16c#6, 16c6, c#6, 8c6, g#, 2p, g., g#, 8c6, f.";}
		else if(n==4){name="  Greensleaves";tempo=1.5; mul=.5; deflen=1./4.;defoct=5;melody="p, g, 2a#, c6, d.6, 8d#6, d6, 2c6, a, f., 8g, a, 2a#, g, g., 8f, g, 2a, f, 2d, g, 2a#, c6, d.6, 8e6, d6, 2c6, a, f., 8g, a, a#., 8a, g, f#., 8e, f#, 2g";}
		else if(n==5){name="Rondo alla turka";mul=8.;deflen=1./4.; defoct=5; melody="16#f1 16e1 16#d1 16e1 4g1 16a1 16g1 16#f1 16g1 4b1 16c2 16b1 16#a1 16b1 16#f2 16e2 16#d2 16e2 16#f2 16e2 16#d2 16e2 4g2 8e2 8g2 32d2 32e2 16#f2 8e2 8d2 8e2 32d2 32e2 16#f2 8e2 8d2 8e2 32d2 32e2 16#f2 8e2 8d2 8#c2 4b1 2p";}
		else if(n==6){name="Godfather theme";tempo=2.5;mul=.5;deflen=1./8.; defoct=5; melody="8g,8c6,8d#6,8d6,8c6,8d#6,8c6,8d6,c6,8g#,8a#,2g,8p,8g,8c6,8d#6,8d6,8c6,8d#6,8c6,8d6,c6,8g,8f#,2f,8p,8f,8g#,8b,2d6,8p,8f,8g#,8b,2c6,8p,8c,8d#,8a#,8g#,g,8a#,8g#,8g#,8g,8g,8b4,2c,1p";}
		else if(n==7){name="    Bouree";tempo=1.25;mul=.5;deflen=1./4.; defoct=5;melody="1p,8e,8f#,g,8f#,8e,d#,8e,8f#,b4,8c#,8d#,e,8d,8c,b4,8a4,8g4,f#4,8g4,8a4,8b4,8a4,8g4,8f#4,e4,8e,8f#,g,8f#,8e,d#,8e,8f#,b4,8c#,8d#,e,8d,8c,b4,8a4,8g4,f#.4,8g4,2g.4,8e,8f#,g,8f#,8e,d#,8e,8f#,b4,8c#,8d#,e,8d,8c,b4,8a4,8g4,f#4,8g4,8a4,8b4,8a4,8g4,8f#4,e4,8e,8f#,g,8f#,8e,d#,8e,8f#,b4,8c#,8d#,e,8d,8c,b4,8a4,8g4,f#.4,8g4,2g.4,8b4,8g4,d,8a4,8c,b4,8g,8d,e,8b4,8d,c,8b4,8a4,g#4,8a4,8b4,c,8b4,8a4,2a.4,8d,8a4,b4,8g,8d,e,8b4,8d,c,8a,8e,f#,8c#,8e,d,8c#,8b4,a#.4,8b4,2b.4,8b,8f#,g#,8f#,8e,a,8e,8g,f#,8e,8d,g,8d,8f,e,8a,8e,f#,8c#,8e,d#,2b4,8e,8b4,c,8d,8a4,b4,8c,8g4,a4,8b4,8f#4,g4,8f#4,8e4,d#4,8e4,8f#4,g4,8f#4,8e4,2e.4,8b4,8g4,d,8a4,8c,b4,8g,8d,e,8b4,8d,c,8b4,8a4,g#4,8a4,8b4,c,8b4,8a4,2a.4,8d,8a4,b4,8g,8d,e,8b4,8d,c,8a,8e,f#,8c#,8e,d,8c#,8b4,a#.4,8b4,2b.4,8b,8f#,g#,8f#,8e,a,8e,8g,f#,8e,8d,g,8d,8f,e,8a,8e,f#,8c#,8e,d#,2b4,8e,8b4,c,8d,8a4,b4,8c,8g4,a4,8b4,8f#4,g4,8f#4,8e4,d#4,8e4,8f#4,g4,8f#4,8e4,2e.4";}
		else if(n==8){name="  JingleBell";mul=.5;tempo=2.5; deflen=1./8; defoct=5; melody="32p,a,a,4a,a,a,4a,a,c6,f.,16g,2a,a#,a#,a#.,16a#,a#,a,a.,16a,a,g,g,a,4g,4c6";}
		else if(n==9){name="  Canon in D";tempo=2.;mul=.5;deflen=1./4.;defoct=5;melody="8d, 8f#, 8a, 8d6, 8c#, 8e, 8a, 8c#6, 8d, 8f#, 8b, 8d6, 8a, 8c#, 8f#, 8a, 8b, 8d, 8g, 8b, 8a, 8d, 8f#, 8a, 8b, 8f#, 8g, 8b, 8c#, 8e, 8a, 8c#6, f#6, 8f#, 8a, e6, 8e, 8a, d6, 8f#, 8a, c#6, 8c#, 8e, b, 8d, 8g, a, 8f#, 8d, b, 8d, 8g, c#.6";}
		else if(n==10){name="     Bolero";tempo=2.7;mul=.25; deflen=1./2.;defoct=5;melody="c6, 8c6, 16b, 16c6, 16d6, 16c6, 16b, 16a, 8c6, 16c6, 16a, c6, 8c6, 16b, 16c6, 16a, 16g, 16e, 16f, 2g, 16g, 16f, 16e, 16d, 16e, 16f, 16g, 16a, g, g, 16g, 16a, 16b, 16a, 16g, 16f, 16e, 16d, 16e, 16d, 8c, 8c, 16c, 16d, 8e, 8f, d, 2g";}
		position=0;
		wait=0;
		error=false;
	}
	int Process()
	{
		if(error)
			return 10000;
		if(melody[position]==0)
			position=0;
		double len=deflen;
		int note=-1;
		int octave=defoct;
		bool sharp=false;
		bool pause=false;
		bool dot=false;

		while(true)
		{
			if(StartsWith(&melody[position], " "))
			{position++;}
			else if(StartsWith(&melody[position], ","))
			{position++;}
			else break;
		}

		if     (StartsWith(&melody[position],"32")==2){len=1./32;position+=2;}
		else if(StartsWith(&melody[position],"16")==2){len=1./16;position+=2;}
		else if(StartsWith(&melody[position], "8")   ){len=1./ 8;position+=1;}
		else if(StartsWith(&melody[position], "4")   ){len=1./ 4;position+=1;}
		else if(StartsWith(&melody[position], "3")   ){len=1./ 3;position+=1;}
		else if(StartsWith(&melody[position], "2")   ){len=1./ 2;position+=1;}
		else if(StartsWith(&melody[position], "1")   ){len=1./ 1;position+=1;}

		if(StartsWith(&melody[position],"#")){sharp=true;position++;}

		if     (StartsWith(&melody[position],"p")){pause=true;position++;}
		else if(StartsWith(&melody[position],"c")){note=0;position++;}
		else if(StartsWith(&melody[position],"d")){note=2;position++;}
		else if(StartsWith(&melody[position],"e")){note=4;position++;}
		else if(StartsWith(&melody[position],"f")){note=5;position++;}
		else if(StartsWith(&melody[position],"g")){note=7;position++;}
		else if(StartsWith(&melody[position],"a")){note=9;position++;}
		else if(StartsWith(&melody[position],"b")){note=11;position++;}
		else
		{
			error=true;position++;
		}

		if(error)
		{
			return 10000;
		}

		if(StartsWith(&melody[position],"#")){sharp=true;position++;}
		if(StartsWith(&melody[position],".")){dot=true;position++;}

		if     (StartsWith(&melody[position],"1")){octave=1;position++;}
		else if(StartsWith(&melody[position],"2")){octave=2;position++;}
		else if(StartsWith(&melody[position],"3")){octave=3;position++;}
		else if(StartsWith(&melody[position],"4")){octave=4;position++;}
		else if(StartsWith(&melody[position],"5")){octave=5;position++;}
		else if(StartsWith(&melody[position],"6")){octave=6;position++;}
		else if(StartsWith(&melody[position],"7")){octave=7;position++;}

		if(dot)
			len*=1.5;
		if(sharp)
			note++;
		if(note>=12)
		{
			error=true;
		}

		if(error)
		{
			printf("Error\n");
			return 10000;
		}
		double f=freqs[note];

		if(octave>4)
		{
			for(int i=0;i<octave-4;i++)
				f*=2.;
		}
		if(octave<4)
		{
			for(int i=0;i<4-octave;i++)
				f*=.5;
		}
		f*=mul;
		len*=tempo;
		if(!pause)
		{
			notes.AddNote(f,len<.5?.5:len);
		}
		return len*48000;
	}
	void Update(int ds)
	{
		wait-=ds;
		if(wait<0)
		{
			wait+=Process();
		}
	}
};

MelodyProcessor melody;

void FFT(double* in, double* o, int j)
{
	I.re=0;
	I.im=1;

	cplx buf[NFFT];
	cplx out[NFFT];
	for(int i=0;i<NFFT;i++)
	{
		int idx=((-i*4+j)+EL)%EL*2;
		buf[i].re=in[idx];
		buf[i].im=0;
	}

	fft(buf, out, NFFT);

	for(int i=0;i<NFFT;i++)
	{
		o[i]=sqrt(out[i].im*out[i].im+out[i].re*out[i].re);
	}

}

double fftout[NFFT];

bool graph;
int frame;

int main()
{

//	StdFreqs();
	melody.Init(1);
	notes.Init();
	snd.Init();
	FFT_Init();
	double t0=Time();
	for(int i=0;i<NFFT;i++)
		fftout[i]=0;
	frame=0;
	graph=false;

	while(true)
	{
		{
			CSound s;
			s.Poll();
		}
		int key;
		int press;
		prevmx=mx;
		prevmy=my;
		prevmb=mb;
		GetMouseState(mx, my, mb);
		while(GetKeyEvent(key,press))
		{
			if(press>0)
			{
				if     (key==49){melody.Init(1);printf("%s\n",melody.name);}
				else if(key==50){melody.Init(2);printf("%s\n",melody.name);}
				else if(key==51){melody.Init(3);printf("%s\n",melody.name);}
				else if(key==52){melody.Init(4);printf("%s\n",melody.name);}
				else if(key==53){melody.Init(5);printf("%s\n",melody.name);}
				else if(key==54){melody.Init(6);printf("%s\n",melody.name);}
				else if(key==55){melody.Init(7);printf("%s\n",melody.name);}
				else if(key==56){melody.Init(8);printf("%s\n",melody.name);}
				else if(key==57){melody.Init(9);printf("%s\n",melody.name);}
				else if(key==48){melody.Init(0);printf("%s\n",melody.name);}
				else if(key==101){echoVal=Fabs(echoVal-.5);}
				else if(key==118){vibratto=!vibratto;}
				else if(key==99){cut=!cut;}
				else if(key==102){fade=!fade;}
				else if(key==4010)
				{
					graph=!graph;
					if(graph)
						printf("Enable waterfall graph\n");
					else
						printf("Waterfall graph OFF\n");
				}
				printf("Key pressed %i %i\n", key, press);
			}
		}

		//double t1=Time();
		//int nSamples=t1*48000-t0*48000+5;
		//t0=t1;
		//if(nSamples>2000)nSamples=2000;
		//if(snd_bufhealth()>2748)
		//{
		//	nSamples-=10;
		//}
		CSound s;
		while(s.snd_bufhealth()<(2000+NFFT))
		{
			melody.Update(NFFT);
			//notes.Update(snd.sample);
			snd.GenerateSamples(NFFT);
			FFT(snd.echo,fftout,snd.echoPos);

			if(graph)
			{
				for(int i=0;i<NFFT;i++)
				{
					double lvl=Fabs(fftout[i]);
					lvl/=4.;
					float lvlr=lvl*960.;lvlr=lvlr>255.?255.:lvlr;
					float lvlg=lvl*420.;lvlg=lvlg>255.?255.:lvlg;
					float lvlb=lvl*220.;lvlb=lvlb>255.?255.:lvlb;
					int c=
						((int(lvlb))<<16)|
						((int(lvlg))<<8)|
						int(lvlr);
					PutPixel((frame)%640,480-112-i/2,c);
				}
				frame++;
			}
		}

		if(graph)
		{
			g.clear();
			g.M(0,0);
			g.l(640,0);
			g.l(0,110);
			g.l(-640,0);
			g.close();
			g.M(0,480);
			g.l(640,0);
			g.l(0,-110);
			g.l(-640,0);
			g.close();
			g.fin();
			g.rgba(.2,0,0,1);
			g.fill2();

			Present();
			continue;
		}

		g.gray(0);
		g.clear();
		g.M(0,0);g.l(640,0);g.l(0,480);g.l(-640,0);g.close();g.fin();
		g.fill1();
		g.clear();
		//g.M(-1,240);
		for(int i=0;i<640;i++)
		{
			double lvl=snd.echo[(snd.echoPos+(640-i))%EL*2];
			g.L(i,lvl*200+240);
		}
		//g.L(641,240);
		//g.l(-641,0);
		g.fin();
		g.width(2.,1.);
		g.rgb(0.5,1.0,0.0);
		g.stroke();
		g.clear();

		g.clear();
		for(int i=0;i<NFFT/2;i++)
		{
			double lvl=0;
			lvl=fftout[i*2];
			lvl*=2.;
			if(lvl>240)lvl=240;
			g.M(i+.5,480);
			g.l(0,-lvl);
		}
		g.fin();
		g.rgb(1.,.7,.1);
		g.width(2.,1.);
		g.stroke();
		g.rgb(1.,.8,.5);
		g.width(1.,1.);
		g.stroke();
		g.clear();

		melody.Render();
		char ss[64];
		snprintf(ss,64,"% 5i % 5i % 5i", NFFT, snd.echoPos, s.snd_bufhealth());
		stext(ss,10,470,0xffffff00);
		snprintf(ss,64,"[%c] 'V'ibratto", " x"[vibratto]);
		stext(ss,10,460,0xffffff00);
		snprintf(ss,64,"[%c] 'E'cho", " x"[int(echoVal*10)!=0]);
		stext(ss,10,450,0xffffff00);
		snprintf(ss,64,"[%c] 'C'ut", " x"[cut]);
		stext(ss,10,440,0xffffff00);
		snprintf(ss,64,"[%c] 'F'ade", " x"[fade]);
		stext(ss,10,430,0xffffff00);
		Present();
	}
	return 0;
}