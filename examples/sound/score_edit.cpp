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

void MusicInit()
{
	for(int i=0;i<16;i++)
	{
		//mel0[i]="0334566776503030"[i]-'0';
		//mel1[i]="0120303021040400"[i]-'0';
		//baz0[i]="0034231032423100"[i]-'0';
		//baz1[i]="1212131213212312"[i]-'0';
		mel0[i]="0305423542305040"[i]-'0';
		mel1[i]="0400605004005040"[i]-'0';
		baz0[i]="0435430345340210"[i]-'0';
		baz1[i]="2300230340320300"[i]-'0';
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
	return (s0(mod(i1,1024)/1024.)*.35-s0(mod(i1,256)/256.)*.17-s1(mod(i2,1024)/1024.)*.35);
}

class Echo
{
public:
	float* data;
	int pos;
	void Init()
	{
		data=(float*)malloc(16384*sizeof(float));
	}
	void Clear()
	{
		for(int i=0;i<16384;i++)data[i]=0;
		pos=0;
	}
	float Voice(float l)
	{
			l=(l+data[pos]*0.9)*.7;
			data[pos]=l;
			pos++;
			pos=pos%11150;
			return l;
	}
};

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

int cursnd;

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
		float tpos=mod(cursnd,cycle)/cycle;
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

class CMusic
{
public:
	int sample;
	int echoPos;
	float* echo;
	void Init()
	{
		sample=0;
		echoPos=0;
		echo=(float*)malloc(22500*sizeof(float));
		for(int i=0;i<22500;i++)echo[i]=0;
	}
	void GenerateSamples(int nSamples)
	{
		for(int i=0;i<nSamples;i++)
		{
			float ts=sample;
			float l=sndVal(ts+sin(sample/20000.)*150);
			float r=sndVal(ts-sin(sample/20000.)*150);
//			float l=sndVal(sample*.375+sin(sample/5000.)*500);
			sample++;
			int ep=echoPos*2;
			l=(l+echo[ep  ]*0.9)*.7;
			r=(r+echo[ep+1]*0.9)*.7;
			echo[ep  ]=l;
			echo[ep+1]=r;
			echoPos++;
			echoPos=echoPos%11150;
			snd_out(l,r);
		}
	}	
};

CMusic music;

int main()
{
	float tframe;
	tframe=Time();
	float dt;
	dt=0.;
	cursnd=0;
	prevmb=0;
	rseed=21397862;
	MusicInit();

	Echo echo;
	echo.Init();
	echo.Clear();
	music.Init();
	Editor ed1;
	Editor ed2;
	Editor ed3;
	Editor ed4;
	ed1.mel=&mel0[0];ed1.x=100;ed1.y=100;ed1.cycle=4096.*16./.75;
	ed2.mel=&mel1[0];ed2.x=350;ed2.y=100;ed2.cycle=4096.*16./.75;
	ed3.mel=&baz0[0];ed3.x=100;ed3.y=260;ed3.cycle=65536.*16./.75;
	ed4.mel=&baz1[0];ed4.x=350;ed4.y=260;ed4.cycle=65536.*16.*2./.75;

	while(true)
	{
		g.gray(0);
		g.clear();
		g.fill1();
		g.t_0(0,0);
		g.t_x(1,0);
		g.t_y(0,1);
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
		int nSamples=t1*44100-tframe*44100+.5;
		tframe=t1;
		if(nSamples>2000)nSamples=2000;

		g.M(-1,240);
		{
			music.GenerateSamples(nSamples);
			cursnd+=nSamples;
			for(int i=0;i<640;i+=2)
			{
				float lvl=music.echo[((music.echoPos+(i-640)*2+11150)%11150)];
				g.L(i,lvl*120+240);
			}
			g.L(641,240);
			g.fin();
			g.rgb(0.0,.4,0.0);
			g.fill1();
			g.width(1.,1.);
			g.rgb(0.0,1.0,0.5);
			g.stroke();
		}

		char ss[64];
		snprintf(ss,64,"Time %f %i %i", Time(),nSamples, cursnd);
		stext(ss,10,10,0xffffffff);



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
			cursnd=0;
			echo.Clear();
		}

		if(KeyPressed('t'))
		{
			echo.Clear();
			for(int i=0;i<16;i++)
			{
				int val;
				val=irand(rseed);if(val<0)val=-val;val=val%4;mel0[i]=val;
				val=irand(rseed);if(val<0)val=-val;val=val%4;mel1[i]=val;
				val=irand(rseed);if(val<0)val=-val;val=val%4;baz0[i]=val;
				val=irand(rseed);if(val<0)val=-val;val=val%4;baz1[i]=val;
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