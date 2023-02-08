#include "sound.h"
#include "graphics.h"

#pragma STACK_SIZE 4096

#define M_PI 3.141592654

int rseed;

int irand(int& seed)
{
	seed=(seed*1103515245+12345)%0x7ffffff;
	return seed;
}

char mel0[16];
char mel1[16];
char baz0[16];
char baz1[16];

// 	float i1=(t*("0867604356602121"[idx0]-'0')*("0120421012034200"[idx1]-'0'));
// 	float i2=(t*("0102030102030120"[idx0]-'0')*("2432342324323323"[idx2]-'0'));

void MusicInit()
{
	for(int i=0;i<16;i++)
	{
		// mel0[i]="0334566776503030"[i]-'0';
		// mel1[i]="0120303021040400"[i]-'0';
		// baz0[i]="0034231032423100"[i]-'0';
		// baz1[i]="1212131213212312"[i]-'0';
		// mel0[i]="0305423542305040"[i]-'0';
		// mel1[i]="0400605004005040"[i]-'0';
		// baz0[i]="0435430345340210"[i]-'0';
		// baz1[i]="2300230340320300"[i]-'0';
		mel0[i]="0867604356602121"[i]-'0';
		mel1[i]="0102030102030120"[i]-'0';
		baz0[i]="0120421012034200"[i]-'0';
		baz1[i]="2432342324323323"[i]-'0';
	}
}


float s0(float t)
{
	return sin(t*2.*M_PI)*.5;
}

float s1(float t)
{
	if(t<0.2)return sin(t/.2*2.*M_PI)*.5;
	return 0;
}

float mod(float a, float b)
{
	int r=a/b;
	return a-b*r;
}

float sndVal(float t)
{
	t*=0.75;
	int idx0=t/4096.;idx0=idx0&15;
	int idx1=t/65536.;idx1=idx1&15;
	int idx2=t/65536./2.;idx2=idx2&15;

	float i1=(t*mel0[idx0]*baz0[idx1]);
	float i2=(t*mel1[idx0]*baz1[idx2]);

	float w=t/4096.;
	w-=float(int(w));
	w=1.-w;

	return (s0(mod(i1,1024)/1024.)*.35*w-s0(mod(i1,256)/256.)*.17*w-s1(mod(i2,1024)/1024.)*.35*w);
}

Graph g;

void Rect(float x, float y ,float w, float h)
{
	g.M(x,y);g.l(w,0);g.l(0,h);g.l(-w,0);g.close();
}

void Circle(float x, float y, float r)
{
	g.M(x+r,y);
	float a=0.55;
	g.C(x+r,y+r*a,x+r*a,y+r,x,y+r);
	g.C(x-r*a,y+r,x-r,y+r*a,x-r,y);
	g.C(x-r,y-r*a,x-r*a,y-r,x,y-r);
	g.C(x+r*a,y-r,x+r,y-r*a,x+r,y);
	g.close();
}


int mx;
int my;
int mb;
int prevmb;
int prevmx;
int prevmy;

//int cursnd;

#define NOTESIZE 14

class Editor
{
public:
	char* mel;
	int x;
	int y;
	float cycle;
	int hoverx;
	int hovery;
	void Update()
	{
		hoverx=hovery=-1;
		int sx=(mx-x)/NOTESIZE;
		int sy=(my-y)/NOTESIZE;
		if((sx>=0)&&(sx<16)&&(sy>=0)&&(sy<10))
		{
			hoverx=sx;
			hovery=sy;
			if(
				(((mb&1)==1)&&((prevmb&1)==0))
				||(((mb&1)==1)&&((mx!=prevmx)||(my!=prevmy)))
				)
			{
				mel[sx]=sy;
			}
		}
	}
	void Draw()
	{
		g.clear();
		g.M(x,y);
		g.l(16*NOTESIZE,0);
		g.l(0,10*NOTESIZE);
		g.l(-16*NOTESIZE,0);
		g.close();
		g.fin();
		g.rgb(1,1,1);
		g.alpha(.5);
		g.fill1();
		g.width(1,1);
		g.rgb(.5,.8,1.0);
		g.stroke();

		g.alpha(1);
		g.clear();
		for(int i=0;i<16;i++)
		{
			//Rect(i*NOTESIZE+x+2,mel[i]*NOTESIZE+y+2,NOTESIZE-4,NOTESIZE-4);
			Circle(i*NOTESIZE+NOTESIZE/2+x,mel[i]*NOTESIZE+NOTESIZE/2+y,NOTESIZE/2-1);
		}
		g.fin();
		g.rgb(.4,.1,0);
		g.fill1();

		g.clear();
		for(int i=0;i<16;i++)
		{
			g.M(i*NOTESIZE+NOTESIZE/2+x-1,mel[i]*NOTESIZE+NOTESIZE/2+y-1);
			g.l(0,0);
		}
		g.fin();
		g.rgb(1.,.5,.3);
		g.width(NOTESIZE/2-1,1);
		g.stroke();

		g.clear();
		for(int i=0;i<16;i++)
		{
			g.M(i*NOTESIZE+NOTESIZE/2+x-2,mel[i]*NOTESIZE+NOTESIZE/2+y-2);
			g.l(0,0);
		}
		g.fin();
		g.rgb(1,1,1);
		g.width(2,1);
		g.stroke();

		g.clear();
		for(int i=1;i<10;i++)
		{
			g.M(x,y+i*NOTESIZE);
			g.l(16*NOTESIZE,0);
		}
		for(int i=1;i<16;i++)
		{
			g.M(x+i*NOTESIZE,y);
			g.l(0,10*NOTESIZE);
		}
		g.fin();
		g.width(1.,1.);
		g.rgb(.8,.0,0.3);
		g.stroke();

		g.clear();
		float tpos=0;//mod(snd.sample/48000.,cycle)/cycle;
		g.M(x+16*NOTESIZE*tpos,y);
		g.l(0,10*NOTESIZE);
		g.fin();
		g.width(2.,2.);
		g.rgba(1,1,1,1);
		g.alpha(1);
		g.stroke();

		if((hoverx>=0)&&(hovery>=0))
		{
			g.clear();
			Rect(hoverx*NOTESIZE+x,hovery*NOTESIZE+y,NOTESIZE,NOTESIZE);
			g.fin();
			g.width(1.,1.);
			g.rgba(1,1,1,1);
			g.alpha(1);
			g.stroke();
		}

	}
};


///////////////////////////////////////

bool vibratto;
bool cut;
bool fade;
double echoVal;

double freqs[12]={
	261.626, 277.183, 293.665, 311.127, 329.628, 349.228, 369.994, 391.995, 415.305, 440.000, 466.164, 493.883
};

struct Note
{
	int instr; // instrument
	double f;  // freq
	int t0; // starh
	int t1; // end
};



float ss(float x)
{
	//if(int(x/2./M_PI)%5!=0)return 0;
	x=sin(x);
	//if(x>.5)x=.5;else if(x<-.5)x=-.5;
	return x;
}

#define NN 64
class Polyphony
{
public:
	Note notes[NN];
	int ns;
	int Count()
	{
		int n=0;
		for(int i=0;i<NN;i++)
		{
			if(ns<notes[i].t1)
				n++;
		}
		return n;
	}
	void Init()
	{
		ns=0;
		for(int i=0;i<NN;i++)
		{
			notes[i].t0=-100;
			notes[i].t1=-100;
		}
	}
	void AddNote(int instr, double f, double len, float delay)
	{
		for(int i=0;i<NN;i++)
		{
			if(ns>notes[i].t1)
			{
				notes[i].instr=instr;
				notes[i].f=f;
				notes[i].t0=ns+delay*48000;
				notes[i].t1=ns+len*48000+delay*48000;
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
				if(n.f==0.)
					break;
				if(cs>n.t0)
				{
					double t=(cs-n.t0)/48000.;
					//double s=sin((t+sin(cs*.0005)*0.0005)*n.f*M_PI*2);
					if(vibratto)
						t+=sin(t*M_PI*5.)*0.0007*t;
					double s=0.;
					if(n.instr==2)
					{
						int tact=int(t*n.f*5.)%5;
						if(tact)
							s=0;
						else
						 	s=sin(t*n.f*M_PI*2.*5.)*.2;
						if(t<0.01)s*=t*100.;
					}
					else
					{
						s=(ss(t*n.f*M_PI*2.));//+ss(t*(n.f+2030.)*M_PI*2.)*.05+ss(t*(n.f+2730.)*M_PI*2.)*.126)*.5;
						if(cut)
						{
							if(s>0.4)s=0.4;else if(s<-0.4)s=-.4;
							s*=0.5;

							//s=s;
						}
						if(t<0.01)s*=t*100.;
					}

					if(fade)
					{
						s*=(1. -(cs-n.t0)/double(n.t1-n.t0));
					}
					else
					{
//						double te=(n.t1-cs)/48000.;if(te<0.1)s*=te*10.;
					}
					double b=.5;//+sin(cs*.001)*.45;
					int ep=((echoPos+i)%echoLen)*2;
					double l=s*b     +echo[ep  ];
					double r=s*(1.-b)+echo[ep+1];
					//if(l>.9)l=0.9;else if(l<-0.9)l=-.9;
					//if(r>.9)r=0.9;else if(r<-0.9)r=-.9;
					echo[ep  ]=l;
					echo[ep+1]=r;
				}
				cs++;
			}
		}
		ns+=nSamples;
	}
};

Polyphony notes;

#define EL 12000

#define ALMOST_ONE 0.9999999999999

double Fabs(double x)
{
	return x<0?-x:x;
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
		echoVal=0.75;
		vibratto=true;
		cut=true;
		fade=true;
		echo=(double*)malloc(sizeof(double)*2*EL);
		for(int i=0;i<EL*2;i++)echo[i]=0;
	}
	void GenerateSamples(int nSamples)
	{
		for(int i=0;i<nSamples;i++)
		{
			int ep=((echoPos+i)%EL)*2;
			double l=echo[ep  ];
			double r=echo[ep+1];
			echo[ep  ]=r*echoVal;
			echo[ep+1]=l*echoVal;
		}
		{
			float ns0=notes.ns;
			float ns1=notes.ns+nSamples;
			ns0/=48000.;
			ns1/=48000.;

			ns0*=(48000./4096./16./1.97)*16/.75;
			ns1*=(48000./4096./16./1.97)*16/.75;

			int t0=ns0;
			int t1=ns1;
			if(t0!=t1)
			{
				float i1=(mel0[t1%16]*baz0[(t1/16)%16]);
				float i2=(mel1[t1%16]*baz1[(t1/32)%16]);
				if(i1!=0.0)
				{
					notes.AddNote(1,i1*(48000./1024.),6./16.,0.0);
				}
				if(i2!=0.0)
				{
					notes.AddNote(2,i2*(48000./1024.),4./16.,0.0);
				}
				if(t1-t0>1)
					printf("Overflow\n");
			}
		}
		notes.RenderSpan(nSamples,&echo[0],echoPos,EL);
		//for(int i=0;i<nSamples;i++)
		//{
		//	int ep=(echoPos+i)%EL*2;
		//	snd_out(echo[ep],echo[ep+1]);
		//}
		//if(do_out)
		{
			int idx=echoPos-nSamples;
			int count=nSamples;
			if(idx<0)
			{
				count+=idx;
				snd_out_buf(&(echo[(EL+idx)*2]),-idx);
				idx=0;
			}
			snd_out_buf(&(echo[idx*2]),count);
		}
		echoPos=(echoPos+nSamples)%EL;
		sample+=nSamples;
	}
};

CSnd snd;

///////////////////////////////////////



int main()
{
	float tframe;
	tframe=Time();
	float dt;
	dt=0.;
	prevmb=0;
	rseed=21397862;
	MusicInit();
	snd.Init();
	echoVal=0.7;
	cut=1;
	fade=1;
	vibratto=1;

	Editor ed1;
	Editor ed2;
	Editor ed3;
	Editor ed4;
	ed1.mel=&mel0[0];ed1.x=100;ed1.y=100;ed1.cycle=.75*4096.*16./.75;
	ed2.mel=&mel1[0];ed2.x=350;ed2.y=100;ed2.cycle=.75*4096.*16./.75;
	ed3.mel=&baz0[0];ed3.x=100;ed3.y=260;ed3.cycle=.75*65536.*16./.75;
	ed4.mel=&baz1[0];ed4.x=350;ed4.y=260;ed4.cycle=.75*65536.*16.*2./.75;

	while(true)
	{
		g.t_0(0,0);
		g.t_x(1,0);
		g.t_y(0,1);
		g.gray(0);
		g.clear();
		g.M(0,0);g.l(640,0);g.l(0,480);g.l(-640,0);g.close();g.fin();
		g.fill1();
		g.clear();
		g.M(0,240.5);g.l(640,0);
		g.M(0,240.5+120);g.l(640,0);
		g.M(0,240.5-120);g.l(640,0);
		g.fin();
		g.width(1.,1.);
		g.rgb(0,.5,0);
		g.stroke();
		g.clear();

		float t1=Time();
		tframe=t1;

		g.M(-1,240);
		{
			while(snd_bufhealth()<(1024*3+1024))
			{
				for(int i=0;i<8;i++)
					snd.GenerateSamples(128);
			}
			for(int i=0;i<640;i++)
			{
				float lvl=snd.echo[(snd.echoPos+(i-640)*2+EL*2)%(EL*2)];
				g.L(i,lvl*120+240);
			}
			g.l(3,0);
			g.L(643,240);
			g.fin();
			g.rgb(0.0,.4,0.0);
			g.fill1();
			g.width(1.,1.);
			g.rgb(0.0,1.0,0.5);
			g.stroke();
		}

		char ss[64];
		snprintf(ss,64,"Time %f", Time());
		stext(ss,10,10,0xffffffff);
		
		snprintf(ss,64,"Poly %i", notes.Count());
		stext(ss,10,20,0xffffffff);



		g.t_0(0.5,0.5);
		g.t_x(1,0);
		g.t_y(0,1);


		GetMouseState(mx, my, mb);
		ed1.Draw();ed1.Update();
		ed2.Draw();ed2.Update();
		ed3.Draw();ed3.Update();
		ed4.Draw();ed4.Update();
		prevmx=mx;
		prevmy=my;
		prevmb=mb;

		if(KeyPressed('0'))
		{
			0==0;
		}

		if(KeyPressed('1'))
			notes.AddNote(2,440.,4./16.,0.0);
		if(KeyPressed('2'))
			notes.AddNote(2,880.,4./16.,0.0);

		if(KeyPressed('t'))
		{
			for(int i=0;i<16;i++)
			{
				int val;
				val=irand(rseed);if(val<0)val=-val;val=val%7;mel0[i]=val;
				val=irand(rseed);if(val<0)val=-val;val=val%7;mel1[i]=val;
				val=irand(rseed);if(val<0)val=-val;val=val%7;baz0[i]=val;
				val=irand(rseed);if(val<0)val=-val;val=val%7;baz1[i]=val;
			}
		}

		if(KeyPressed('s'))
		{
			printf("\n");
			for(int i=0;i<16;i++)printf("%c",mel0[i]+'0');printf("\n");
			for(int i=0;i<16;i++)printf("%c",mel1[i]+'0');printf("\n");
			for(int i=0;i<16;i++)printf("%c",baz0[i]+'0');printf("\n");
			for(int i=0;i<16;i++)printf("%c",baz1[i]+'0');printf("\n");
			printf("\n");
		}

		Present();
	}
	return 0;
}