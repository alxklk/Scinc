// RTTTL ringtone player with a simple synth, stereo echo, oscilloscope and spectrum.
//
// Keys:  0-9 pick a melody   E echo   V vibrato   C clipped/clean tone
//        F fade/flat envelope   key code 4010 toggles the waterfall view
// Mouse: hold a button to replace the melody with a live sound
//        (left: sine following X, middle: bytebeat, right: noise)

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
#define SAMPLE_RATE 48000.

#define SCR_W 640
#define SCR_H 480

#define KEY_WATERFALL 4010

#define MAX_VOICES 16
#define NOTE_START_DELAY 4800 // samples between scheduling a note and its onset
#define MIN_NOTE_LEN .5       // seconds; short notes ring on past their slot

#define ECHO_LEN 16384        // stereo frames
#define ECHO_FEEDBACK .5

#define NUM_SONGS 11
#define ERROR_WAIT 10000      // samples to wait after a parse error

int mx=0;
int my=0;
int mb=0;

bool vibrato;
bool cut;
bool fade;
bool echoOn;

// ---------------------------------------------------------------- random

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

// ---------------------------------------------------------------- synth

double Clamp(double x, double lim)
{
	if(x>lim)return lim;
	if(x<-lim)return -lim;
	return x;
}

struct Note
{
	double f; // frequency, Hz
	int t0;   // start sample
	int t1;   // end sample
};

class Polyphony
{
public:
	Note notes[MAX_VOICES];
	int ns; // current sample
	void Init()
	{
		ns=0;
		for(int i=0;i<MAX_VOICES;i++)
		{
			notes[i].t0=-100;
			notes[i].t1=-100;
		}
	}
	// Takes the first free voice; the note is dropped if all of them are busy.
	void AddNote(double f, double len)
	{
		for(int i=0;i<MAX_VOICES;i++)
		{
			if(ns>notes[i].t1)
			{
				notes[i].f=f;
				notes[i].t0=ns+NOTE_START_DELAY;
				notes[i].t1=ns+NOTE_START_DELAY+len*SAMPLE_RATE;
				break;
			}
		}
	}
	double VoiceSample(Note& n, int cs)
	{
		double t=(cs-n.t0)/SAMPLE_RATE;
		if(vibrato)
			t+=sin(t*M_PI*5.)*0.0007*t;
		double s=sin(t*n.f*M_PI*2);
		if(cut)
			s=Clamp(s,0.4);
		else
			s*=0.35;
		if(fade)
		{
			s*=(1. -(cs-n.t0)/double(n.t1-n.t0));
		}
		else
		{
			double te=(n.t1-cs)/SAMPLE_RATE;
			if(te<0.1)s*=te*10.;
		}
		return s;
	}
	// Mixes all voices into the echo ring buffer, which doubles as the output buffer.
	void RenderSpan(int nSamples, double* echo, int echoPos, int echoLen)
	{
		for(int j=0;j<MAX_VOICES;j++)
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
					double s=VoiceSample(n,cs);
					double b=.5+sin(cs*.0001)*.25; // slow auto-pan
					int ep=((echoPos+i)%echoLen)*2;
					echo[ep  ]=Clamp(s*b     +echo[ep  ],0.9);
					echo[ep+1]=Clamp(s*(1.-b)+echo[ep+1],0.9);
				}
				cs++;
			}
		}
		ns+=nSamples;
	}
};

Polyphony synth;

class CSnd
{
	int EchoIndex(int i)
	{
		return (echoPos+i)%ECHO_LEN*2;
	}
	void AddMono(int i, double v)
	{
		int ep=EchoIndex(i);
		echo[ep  ]+=v;
		echo[ep+1]+=v;
	}
	// Reads back the old contents with channels swapped, for a ping-pong echo.
	void FeedbackEcho(int nSamples)
	{
		double gain=echoOn?ECHO_FEEDBACK:0.;
		for(int i=0;i<nSamples;i++)
		{
			int ep=EchoIndex(i);
			double l=echo[ep  ];
			double r=echo[ep+1];
			echo[ep  ]=r*gain;
			echo[ep+1]=l*gain;
		}
	}
	void ClearSpan(int nSamples)
	{
		for(int i=0;i<nSamples;i++)
		{
			int ep=EchoIndex(i);
			echo[ep  ]=0;
			echo[ep+1]=0;
		}
	}
	// Sine following the pointer. The phase is accumulated, so it stays continuous
	// however the frequency changes; the frequency is ramped across the block.
	void MixTone(int nSamples)
	{
		double target=mx*10;
		if(!toneOn)
		{
			toneOn=true;
			toneFreq=target;
			tonePhase=0;
		}
		double step=(target-toneFreq)/nSamples;
		for(int i=0;i<nSamples;i++)
		{
			toneFreq+=step;
			tonePhase+=toneFreq/SAMPLE_RATE;
			if(tonePhase>=1.)
				tonePhase-=1.;
			AddMono(i,sin(tonePhase*2*M_PI)*.3);
		}
	}
	void MixBytebeat(int nSamples)
	{
		for(int i=0;i<nSamples;i++)
		{
			int t=(sample+i);
			int il=((t*("16164289"[(t>>13)&7]&15))/12&128)+(((((t>>12)^(t>>12)-2)%11*t)/4|t>>13)&127);
			AddMono(i,(il&255)/255.-.5);
		}
	}
	void MixNoise(int nSamples)
	{
		for(int i=0;i<nSamples;i++)
			AddMono(i,.5*frand(seed));
	}
	void Output(int nSamples)
	{
		CSound s;
		for(int i=0;i<nSamples;i++)
		{
			int ep=EchoIndex(i);
			s.snd_out(echo[ep],echo[ep+1]);
		}
	}
public:
	int sample;
	int echoPos;
	bool toneOn;
	double toneFreq;
	double tonePhase;
	double* echo;
	void Init()
	{
		printf("Init echo\n");
		sample=0;
		echoPos=0;
		toneOn=false;
		toneFreq=0;
		tonePhase=0;
		echoOn=true;
		vibrato=true;
		cut=true;
		fade=true;
		echo=(double*)malloc(sizeof(double)*2*ECHO_LEN);
		for(int i=0;i<ECHO_LEN*2;i++)echo[i]=0;
	}
	void GenerateSamples(int nSamples)
	{
		if((mb&1)==0)
			toneOn=false;
		if(mb)
		{
			ClearSpan(nSamples);
			if(mb&1)MixTone(nSamples);
			if(mb&2)MixBytebeat(nSamples);
			if(mb&4)MixNoise(nSamples);
			Output(nSamples);
			echoPos=(echoPos+nSamples-1)%ECHO_LEN;
		}
		else
		{
			FeedbackEcho(nSamples);
			synth.RenderSpan(nSamples,&echo[0],echoPos,ECHO_LEN);
			Output(nSamples);
			echoPos=(echoPos+nSamples)%ECHO_LEN;
		}
		sample+=nSamples;
	}
};

CSnd snd;

// ---------------------------------------------------------------- melodies

struct Song
{
	const char* name;
	const char* text;
	double tempo;  // seconds per whole note
	double mul;    // frequency multiplier
	double deflen; // length of a note without a duration prefix
	int defoct;    // octave of a note without an octave suffix
};

Song songs[NUM_SONGS];

void SetSong(int i, const char* name, double tempo, double mul, double deflen, int defoct, const char* text)
{
	Song& s=songs[i];
	s.name=name;
	s.text=text;
	s.tempo=tempo;
	s.mul=mul;
	s.deflen=deflen;
	s.defoct=defoct;
}

void InitSongs()
{
	SetSong(0,"     Gamma",.5,.25,1./2.,7,
		"c c# d d# e f f# g g# a a# b p b a# a g# g f# f e d# d c# c p");
	SetSong(1,"   Nokia tune",2.,8.,1./4.,1,
		"2p 16e2 16d2 8#f 8#g 16#c2 16b 8d 8e 16b 16a 8#c 8e 2a 2p");
	SetSong(2,"     Nyan Cat",3.,.5,1./16.,5,
		"8p,16d#6,16e6,8f#6,8b6,16d#6,16e6,16f#6,16b6,16c#7,16d#7,16c#7,16a#6,8b6,8f#6,16d#6,16e6,8f#6,"
		"8b6,16c#7,16a#6,16b6,16c#7,16e7,16d#7,16e7,16c#7,8f#6,8g#6,16d#6,16d#6,16p,16b,16d6,16c#6,16b,16p,8b,8c#6,8d6,16d6,16c#6,16b,16c#6,16d#6,16f#6,16g#6,16d#6,16f#6,16c#6,16d#6,16b,16c#6,16b,8d#6,8f#6,16g#6,16d#6,16f#6,16c#6,16d#6,16b,16d6,16d#6,16d6,16c#6,16b,16c#6,8d6,16b,16c#6,16d#6,16f#6,16c#6,16d#6,16c#6,16b,8c#6,8b,8c#6,8f#6,8g#6,16d#6,16d#6,16p,16b,16d6,16c#6,16b,16p,8b,8c#6,8d6,16d6,16c#6,16b,16c#6,16d#6,16f#6,16g#6,16d#6,16f#6,16c#6,16d#6,16b,16c#6,16b,8d#6,8f#6,16g#6,16d#6,16f#6,16c#6,16d#6,16b,16d6,16d#6,16d6,16c#6,16b,16c#6,8d6,16b,16c#6,16d#6,16f#6,16c#6,16d#6,16c#6,16b,8c#6,8b,8c#6,8b,16f#,16g#,8b,16f#,16g#,16b,16c#6,16d#6,16b,16e6,16d#6,16e6,16f#6,8b,8b,16f#,16g#,16b,16f#,16e6,16d#6,16c#6,16b,16f#,16d#,16e,16f#,8b,16f#,16g#,8b,16f#,16g#,16b,16b,16c#6,16d#6,16b,16f#,16g#,16f#,8b,16b,16a#,16b,16f#,16g#,16b,16e6,16d#6,16e6,16f#6,8b,8a#,8b,16f#,16g#,8b,16f#,16g#,16b,16c#6,16d#6,16b,16e6,16d#6,16e6,16f#6,8b,8b,16f#,16g#,16b,16f#,16e6,16d#6,16c#6,16b,16f#,16d#,16e,16f#,8b,16f#,16g#,8b,16f#,16g#,16b,16b,16c#6,16d#6,16b,16f#,16g#,16f#,8b,16b,16a#,16b,16f#,16g#,16b,16e6,16d#6,16e6,16f#6,8b,8c#6");
	SetSong(3,"    Children",1.5,1.,1./4.,5,
		"8p, f.6, 1p, g#6, 8g6, d#.6, 1p, g#6, 8g6, c.6, 1p, g#6, 8g6, g#., 1p, 16f, 16g, 16g#, 16c6, f.6, 1p, g#6, 8g6, d#.6, 1p, 16c#6, 16c6, c#6, 8c6, g#, 2p, g., g#, 8c6, f.");
	SetSong(4,"  Greensleaves",1.5,.5,1./4.,5,
		"p, g, 2a#, c6, d.6, 8d#6, d6, 2c6, a, f., 8g, a, 2a#, g, g., 8f, g, 2a, f, 2d, g, 2a#, c6, d.6, 8e6, d6, 2c6, a, f., 8g, a, a#., 8a, g, f#., 8e, f#, 2g");
	SetSong(5,"Rondo alla turka",2.,8.,1./4.,5,
		"16#f1 16e1 16#d1 16e1 4g1 16a1 16g1 16#f1 16g1 4b1 16c2 16b1 16#a1 16b1 16#f2 16e2 16#d2 16e2 16#f2 16e2 16#d2 16e2 4g2 8e2 8g2 32d2 32e2 16#f2 8e2 8d2 8e2 32d2 32e2 16#f2 8e2 8d2 8e2 32d2 32e2 16#f2 8e2 8d2 8#c2 4b1 2p");
	SetSong(6,"Godfather theme",2.5,.5,1./8.,5,
		"8g,8c6,8d#6,8d6,8c6,8d#6,8c6,8d6,c6,8g#,8a#,2g,8p,8g,8c6,8d#6,8d6,8c6,8d#6,8c6,8d6,c6,8g,8f#,2f,8p,8f,8g#,8b,2d6,8p,8f,8g#,8b,2c6,8p,8c,8d#,8a#,8g#,g,8a#,8g#,8g#,8g,8g,8b4,2c,1p");
	SetSong(7,"    Bouree",1.25,.5,1./4.,5,
		"1p,8e,8f#,g,8f#,8e,d#,8e,8f#,b4,8c#,8d#,e,8d,8c,b4,8a4,8g4,f#4,8g4,8a4,8b4,8a4,8g4,8f#4,e4,8e,8f#,g,8f#,8e,d#,8e,8f#,b4,8c#,8d#,e,8d,8c,b4,8a4,8g4,f#.4,8g4,2g.4,8e,8f#,g,8f#,8e,d#,8e,8f#,b4,8c#,8d#,e,8d,8c,b4,8a4,8g4,f#4,8g4,8a4,8b4,8a4,8g4,8f#4,e4,8e,8f#,g,8f#,8e,d#,8e,8f#,b4,8c#,8d#,e,8d,8c,b4,8a4,8g4,f#.4,8g4,2g.4,8b4,8g4,d,8a4,8c,b4,8g,8d,e,8b4,8d,c,8b4,8a4,g#4,8a4,8b4,c,8b4,8a4,2a.4,8d,8a4,b4,8g,8d,e,8b4,8d,c,8a,8e,f#,8c#,8e,d,8c#,8b4,a#.4,8b4,2b.4,8b,8f#,g#,8f#,8e,a,8e,8g,f#,8e,8d,g,8d,8f,e,8a,8e,f#,8c#,8e,d#,2b4,8e,8b4,c,8d,8a4,b4,8c,8g4,a4,8b4,8f#4,g4,8f#4,8e4,d#4,8e4,8f#4,g4,8f#4,8e4,2e.4,8b4,8g4,d,8a4,8c,b4,8g,8d,e,8b4,8d,c,8b4,8a4,g#4,8a4,8b4,c,8b4,8a4,2a.4,8d,8a4,b4,8g,8d,e,8b4,8d,c,8a,8e,f#,8c#,8e,d,8c#,8b4,a#.4,8b4,2b.4,8b,8f#,g#,8f#,8e,a,8e,8g,f#,8e,8d,g,8d,8f,e,8a,8e,f#,8c#,8e,d#,2b4,8e,8b4,c,8d,8a4,b4,8c,8g4,a4,8b4,8f#4,g4,8f#4,8e4,d#4,8e4,8f#4,g4,8f#4,8e4,2e.4");
	SetSong(8,"  JingleBell",2.5,.5,1./8.,5,
		"32p,a,a,4a,a,a,4a,a,c6,f.,16g,2a,a#,a#,a#.,16a#,a#,a,a.,16a,a,g,g,a,4g,4c6");
	SetSong(9,"  Canon in D",2.,.5,1./4.,5,
		"8d, 8f#, 8a, 8d6, 8c#, 8e, 8a, 8c#6, 8d, 8f#, 8b, 8d6, 8a, 8c#, 8f#, 8a, 8b, 8d, 8g, 8b, 8a, 8d, 8f#, 8a, 8b, 8f#, 8g, 8b, 8c#, 8e, 8a, 8c#6, f#6, 8f#, 8a, e6, 8e, 8a, d6, 8f#, 8a, c#6, 8c#, 8e, b, 8d, 8g, a, 8f#, 8d, b, 8d, 8g, c#.6");
	SetSong(10,"     Bolero",2.7,.25,1./2.,5,
		"c6, 8c6, 16b, 16c6, 16d6, 16c6, 16b, 16a, 8c6, 16c6, 16a, c6, 8c6, 16b, 16c6, 16a, 16g, 16e, 16f, 2g, 16g, 16f, 16e, 16d, 16e, 16f, 16g, 16a, g, g, 16g, 16a, 16b, 16a, 16g, 16f, 16e, 16d, 16e, 16d, 8c, 8c, 16c, 16d, 8e, 8f, d, 2g");
}

double freqs[12]={
	261.626, 277.183, 293.665, 311.127, 329.628, 349.228, 369.994, 391.995, 415.305, 440.000, 466.164, 493.883
};

// Semitone above C for a note letter, or -1 if it is not one.
int NoteIndex(int c)
{
	if(c=='c')return 0;
	if(c=='d')return 2;
	if(c=='e')return 4;
	if(c=='f')return 5;
	if(c=='g')return 7;
	if(c=='a')return 9;
	if(c=='b')return 11;
	return -1;
}

double NoteFreq(int note, int octave)
{
	double f=freqs[note];
	for(int i=octave;i<4;i++)
		f*=.5;
	for(int i=4;i<octave;i++)
		f*=2.;
	return f;
}

// Plays a song one token at a time. Tokens are separated by spaces or commas:
//   [duration][#]note[#][.][octave]
// where note is one of c d e f g a b, or p for a pause.
class MelodyProcessor
{
	const char* text;
	int position;
	int wait;
	bool error;
	double deflen;
	int defoct;
	double mul;
	double tempo;

	bool ParseSharp()
	{
		if(text[position]!='#')
			return false;
		position++;
		return true;
	}
	void SkipSeparators()
	{
		while((text[position]==' ')||(text[position]==','))
			position++;
	}
	double ParseDuration()
	{
		char c=text[position];
		char d=text[position+1];
		if((c=='3')&&(d=='2')){position+=2;return 1./32;}
		if((c=='1')&&(d=='6')){position+=2;return 1./16;}
		if((c=='8')||(c=='4')||(c=='3')||(c=='2')||(c=='1'))
		{
			position++;
			return 1./(c-'0');
		}
		return deflen;
	}
	int ParseOctave()
	{
		char c=text[position];
		if((c<'1')||(c>'7'))
			return defoct;
		position++;
		return c-'0';
	}
	// Schedules the next note and returns the number of samples until the one after it.
	int Process()
	{
		if(error)
			return ERROR_WAIT;
		if(text[position]==0)
			position=0;
		SkipSeparators();

		double len=ParseDuration();
		bool sharp=ParseSharp();

		char c=text[position];
		position++;
		bool pause=(c=='p');
		int note=NoteIndex(c);
		if((!pause)&&(note<0))
		{
			error=true;
			return ERROR_WAIT;
		}

		if(ParseSharp())
			sharp=true;
		bool dot=(text[position]=='.');
		if(dot)
			position++;
		int octave=ParseOctave();

		if(dot)
			len*=1.5;
		if(sharp)
			note++;
		if(note>=12)
		{
			error=true;
			printf("Error\n");
			return ERROR_WAIT;
		}

		len*=tempo;
		if(!pause)
			synth.AddNote(NoteFreq(note,octave)*mul,len<MIN_NOTE_LEN?MIN_NOTE_LEN:len);
		return len*SAMPLE_RATE;
	}
public:
	const char* name;
	void Init(int n)
	{
		Song& s=songs[n];
		name=s.name;
		text=s.text;
		tempo=s.tempo;
		mul=s.mul;
		deflen=s.deflen;
		defoct=s.defoct;
		position=0;
		wait=0;
		error=false;
	}
	void Update(int ds)
	{
		wait-=ds;
		if(wait<0)
			wait+=Process();
	}
	// The melody text, with the part already played highlighted.
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
			char c=text[i];
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
};

MelodyProcessor melody;

// ---------------------------------------------------------------- spectrum

double fftout[NFFT];

// Magnitude spectrum of the left channel, decimated by 4, ending at frame j.
void FFT(double* in, double* o, int j)
{
	cplx buf[NFFT];
	cplx out[NFFT];
	for(int i=0;i<NFFT;i++)
	{
		int idx=((-i*4+j)+ECHO_LEN)%ECHO_LEN*2;
		buf[i].re=in[idx];
		buf[i].im=0;
	}

	fft(buf, out, NFFT);

	for(int i=0;i<NFFT;i++)
		o[i]=sqrt(out[i].im*out[i].im+out[i].re*out[i].re);
}

// ---------------------------------------------------------------- drawing

Graph g;

bool graph=false;
int frame=0;

void PathRect(int x, int y, int w, int h)
{
	g.M(x,y);
	g.l(w,0);
	g.l(0,h);
	g.l(-w,0);
	g.close();
}

int WaterfallChannel(double lvl, double gain)
{
	float v=lvl*gain;
	v=v>255.?255.:v;
	return int(v);
}

// One column of the waterfall, drawn straight to the screen as each block is generated.
void DrawWaterfallColumn()
{
	for(int i=0;i<NFFT;i++)
	{
		double lvl=fftout[i]/4.;
		int c=
			(WaterfallChannel(lvl,220.)<<16)|
			(WaterfallChannel(lvl,420.)<<8)|
			WaterfallChannel(lvl,960.);
		PutPixel(frame%SCR_W,SCR_H-112-i/2,c);
	}
	frame++;
}

void DrawWaterfallBands()
{
	g.clear();
	PathRect(0,0,SCR_W,110);
	PathRect(0,SCR_H,SCR_W,-110);
	g.fin();
	g.rgba(.2,0,0,1);
	g.fill2();
}

void DrawBackground()
{
	g.gray(0);
	g.clear();
	PathRect(0,0,SCR_W,SCR_H);
	g.fin();
	g.fill1();
}

void DrawScope()
{
	g.clear();
	for(int i=0;i<SCR_W;i++)
	{
		double lvl=snd.echo[(snd.echoPos+(SCR_W-i))%ECHO_LEN*2];
		g.L(i,lvl*200+SCR_H/2);
	}
	g.fin();
	g.width(2.,1.);
	g.rgb(0.5,1.0,0.0);
	g.stroke();
}

void DrawSpectrum()
{
	g.clear();
	for(int i=0;i<NFFT/2;i++)
	{
		double lvl=fftout[i*2]*2.;
		if(lvl>240)lvl=240;
		g.M(i+.5,SCR_H);
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
}

void DrawToggle(const char* label, int y, bool on)
{
	char ss[64];
	snprintf(ss,64,"[%c] %s"," x"[on],label);
	stext(ss,10,y,0xffffff00);
}

void DrawStatus(int bufHealth)
{
	char ss[64];
	snprintf(ss,64,"% 5i % 5i % 5i", NFFT, snd.echoPos, bufHealth);
	stext(ss,10,470,0xffffff00);
	DrawToggle("'V'ibratto",460,vibrato);
	DrawToggle("'E'cho",450,echoOn);
	DrawToggle("'C'ut",440,cut);
	DrawToggle("'F'ade",430,fade);
}

// ---------------------------------------------------------------- main

void HandleKey(int key, int press)
{
	if((key>='0')&&(key<='9'))
	{
		melody.Init(key-'0');
		printf("%s\n",melody.name);
	}
	else if(key=='e'){echoOn=!echoOn;}
	else if(key=='v'){vibrato=!vibrato;}
	else if(key=='c'){cut=!cut;}
	else if(key=='f'){fade=!fade;}
	else if(key==KEY_WATERFALL)
	{
		graph=!graph;
		if(graph)
			printf("Enable waterfall graph\n");
		else
			printf("Waterfall graph OFF\n");
	}
	printf("Key pressed %i %i\n", key, press);
}

void ReadInput()
{
	int key;
	int press;
	GetMouseState(mx, my, mb);
	while(GetKeyEvent(key,press))
	{
		if(press>0)
			HandleKey(key,press);
	}
}

int main()
{
	InitSongs();
	melody.Init(1);
	synth.Init();
	snd.Init();
	FFT_Init();
	for(int i=0;i<NFFT;i++)
		fftout[i]=0;

	CSound s;
	while(true)
	{
		s.Poll();
		ReadInput();

		while(s.snd_bufhealth()<(2000+NFFT))
		{
			melody.Update(NFFT);
			snd.GenerateSamples(NFFT);
			FFT(snd.echo,fftout,snd.echoPos);
			if(graph)
				DrawWaterfallColumn();
		}

		if(graph)
		{
			DrawWaterfallBands();
		}
		else
		{
			DrawBackground();
			DrawScope();
			DrawSpectrum();
			melody.Render();
			DrawStatus(s.snd_bufhealth());
		}
		Present();
	}
	return 0;
}
