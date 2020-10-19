#include "graphics.h"

#pragma STACK_SIZE 25600000

#define M_PI 3.141592654

#ifdef __SCINC__
#define const
#endif


float freqs[12];

void StdFreqs()
{
	freqs[ 0]=261.626;
	freqs[ 1]=277.183;
	freqs[ 2]=293.665;
	freqs[ 3]=311.127;
	freqs[ 4]=329.628;
	freqs[ 5]=349.228;
	freqs[ 6]=369.994;
	freqs[ 7]=391.995;
	freqs[ 8]=415.305;
	freqs[ 9]=440.000;
	freqs[10]=466.164;
	freqs[11]=493.883;
}

struct Note
{
	float f;  // freq
	int t0; // starh
	int t1; // end
};

#define NN 6
class Polyphony
{
public:
	Note n[NN];
	int ns;
	void Update(int T)
	{
		ns=T;
	}
	void Init()
	{
		for(int i=0;i<NN;i++)
		{
			n[i].t0=-100;
			n[i].t1=-100;
		}
	}
	void AddNote(float f, float len)
	{
		for(int i=0;i<NN;i++)
		{
			if(ns>n[i].t1)
			{
				n[i].f=f;
				n[i].t0=ns+4410;
				n[i].t1=ns+4410+len*44100;
				break;
			}
		}
	}
};

int cursnd;

Graph g;

Polyphony notes;

#define EL 8150

class CSnd
{
public:
	int sample;
	int echoPos;
	float echo[EL*3];
	void Init()
	{
		printf("Init echo\n");
		sample=0;
		echoPos=0;
		for(int i=0;i<EL*3;i++)echo[i]=0;
	}
	void GenerateSamples(int nSamples)
	{
		for(int i=0;i<nSamples;i++)
		{
			int ep=((echoPos+i)%EL)*2;
			float l=echo[ep  ];
			float r=echo[ep+1];
			echo[ep  ]=r*.4;
			echo[ep+1]=l*.4;
			//echo[ep  ]=0;
			//echo[ep+1]=0;
		}
		for(int j=0;j<NN;j++)
		{
			Note& n=notes.n[j];
			if(sample>n.t1)
				continue;
			int cs=sample;
			for(int i=0;i<nSamples;i++)
			{
				if(cs>n.t1)
					break;
				if(cs>n.t0)
				{
					float t=(cs-n.t0)/44100.;
					float s=sin((t+sin(t*24)*0.0005)*n.f*M_PI*2);
					if(s>0.3)s=0.3;else if(s<-0.3)s=-.3;
					s*=(1.-(cs-n.t0)/float(n.t1-n.t0));
					float tb=(cs-n.t0)/44100.;if(tb<0.01)s*=tb*100.;
					//float te=(n.t1-cs)/44100.;if(te<0.1)s*=te*10.;
					int ep=(echoPos+i)%EL*2;
					echo[ep  ]+=s;
					echo[ep+1]+=s;
				}
				cs++;
			}
		}
		for(int i=0;i<nSamples;i++)
		{
			int ep=(echoPos+i)%EL*2;
			snd_out(echo[ep],echo[ep+1]);
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
	float deflen;
	int defoct;
	float mul;
public:
	void Render()
	{
		char s[2];
		s[1]=0;
		int x=10;
		int y=20;
		int i=0;
		while(true)
		{
			char c=melody[i];
			if(c==0)
				break;
			s[0]=c;
			i++;
			stext(s,x,y,i<=position?0xffffff80:0xff808080);
			x+=6;
			if((x>400)&&((c==' ')||(c==',')))
			{
				x=10;
				y+=10;
			}
		}
	}

	void Init()
	{
		//mul=8;deflen=1./4.; defoct=1;melody="2p 16e2 16d2 8#f 8#g 16#c2 16b 8d 8e 16b 16a 8#c 8e 2a 2p";
		//mul=8.;deflen=1./4.; defoct=5; melody="16#f1 16e1 16#d1 16e1 4g1 16a1 16g1 16#f1 16g1 4b1 16c2 16b1 16#a1 16b1 16#f2 16e2 16#d2 16e2 16#f2 16e2 16#d2 16e2 4g2 8e2 8g2 32d2 32e2 16#f2 8e2 8d2 8e2 32d2 32e2 16#f2 8e2 8d2 8e2 32d2 32e2 16#f2 8e2 8d2 8#c2 4b1 2p";
		//Godfather:d=4,o=5,b=160:
		mul=1.;deflen=1./8.; defoct=5; melody="8g,8c6,8d#6,8d6,8c6,8d#6,8c6,8d6,c6,8g#,8a#,2g,8p,8g,8c6,8d#6,8d6,8c6,8d#6,8c6,8d6,c6,8g,8f#,2f,8p,8f,8g#,8b,2d6,8p,8f,8g#,8b,2c6,8p,8c,8d#,8a#,8g#,g,8a#,8g#,8g#,8g,8g,8b4,2c";
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
		float len=deflen;
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

		if   (StartsWith(&melody[position],"32")==2){len=1./32;position+=2;}
		else if(StartsWith(&melody[position],"16")==2){len=1./16;position+=2;}
		else if(StartsWith(&melody[position], "8")   ){len=1./ 8;position+=1;}
		else if(StartsWith(&melody[position], "4")   ){len=1./ 4;position+=1;}
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
		float f=freqs[note];

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
		len*=2.;
		if(!pause)
		{
			notes.AddNote(f,len<.25?.25:len);
		}
		return len*44100;
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

int main()
{
	StdFreqs();
	melody.Init();
	notes.Init();
	snd.Init();
	float t0=Time();

	while(true)
	{
		g.gray(0);
		g.clear();
		g.fill1();
		float t1=Time();
		int nSamples=t1*44100-t0*44100+1;
		if(t1<1)
			nSamples+=100;
		t0=t1;
		if(nSamples>2000)nSamples=2000;
		melody.Update(nSamples);
		notes.Update(snd.sample);
		snd.GenerateSamples(nSamples);
		g.clear();
		g.M(-1,240);
		for(int i=0;i<640;i++)
		{
			float lvl=snd.echo[(snd.echoPos+(i-640)+EL)%EL];
			g.L(i,lvl*120+240);
		}
		g.L(641,240);
		g.l(-641,0);
		g.fin();
		g.rgb(0.0,.4,0.0);
		g.fill1();
		g.width(2.,1.);
		g.rgb(0.0,1.0,0.5);
		g.stroke();
		g.clear();
		melody.Render();
		Present();
	}
	return 0;
}