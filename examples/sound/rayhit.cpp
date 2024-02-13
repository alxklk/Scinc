#include "sound.h"
#include "graphics.h"
#include "../ws.h"

#pragma STACK_SIZE 25600000

#define M_PI 3.141592654

#ifdef __SCINC__
#define const
#endif

CSound snd;

class flt2
{
public:
	float x;
	float y;
	flt2 operator+=(const flt2& r){x+=r.x;y+=r.y;return *this;}
	flt2 operator*(float r)const{flt2 res;res.x=x*r;res.y=y*r;return res;}
	flt2 operator-(const flt2& r)const{flt2 res;res.x=x-r.x;res.y=y-r.y;return res;}
	flt2 operator+(const flt2& r) {flt2 res;res.x=x+r.x;res.y=y+r.y;return res;}
	flt2 perp()const{flt2 res;res.x=y;res.y=-x;return res;}
	float length()const{return sqrt(x*x+y*y);}
	flt2 norm()const
	{
		flt2 res;
		res.x=0;res.y=0;
		float l=length();
		if(l!=0)
		{
			res.x=x/l;
			res.y=y/l;
		}
		return res;
	}
};

flt2 Flt2(float x, float y){flt2 res;res.x=x;res.y=y;return res;}

float vdot(const flt2& l,const flt2& r){return l.x*r.x+l.y*r.y;}

flt2 reflect(const flt2& r, const flt2& n)
{
	return n*vdot(n,r)*2.0f-r;
}

class Wall
{
public:
	flt2 p0;
	flt2 p1;
	void Set(float x0, float y0, float x1, float y1)
	{
		p0.x=x0;
		p0.y=y0;
		p1.x=x1;
		p1.y=y1;
	}
};

#define NW 8

Wall walls[NW];

class Bullet
{
public:
	flt2 p;
	flt2 v;
	void Set(float px, float py, float vx, float vy)
	{
		p.x=px;
		p.y=py;
		v.x=vx;
		v.y=vy;
	}
};

#define NB 3

Bullet bullets[NB];

Graph g;

void Circle(float x, float y, float r)
{
	g.M(x+r,y);
	float a=0.55;
	g.C(x+r,y+r*a,x+r*a,y+r,x,y+r);
	g.C(x-r*a,y+r,x-r,y+r*a,x-r,y);
	g.C(x-r,y-r*a,x-r*a,y-r,x,y-r);
	g.C(x+r*a,y-r,x+r,y-r*a,x+r,y);
}


struct Hit
{
	float x;
	float y;
	float t;
	float f;
	int s;
};

#define NH 20
#define hitl 1.
class Hits
{
public:
	Hit h[NH];
	int nh;
	float t;
	int ns;
	void Update(float T)
	{
		t=T;
	}
	void Init()
	{
		nh=0;
		for(int i=0;i<NH;i++)
		{
			h[i].t=-100;
			h[i].x=0;
			h[i].y=0;
			h[i].s=-100000;
		}
	}
	void AddHit(float x, float y)
	{
		for(int i=0;i<NH;i++)
		{
			if((t-h[i].t)>hitl)
			{
				h[i].x=x;
				h[i].y=y;
				h[i].t=t;
				h[i].s=ns;
				h[i].f=(40+x*4+y*5)*4;
				break;
			}
		}
	}
	void Render()
	{
		int nh=0;
		for(int i=0;i<NH;i++)
		{
			float r=(t-h[i].t)/hitl;
			if((r>0.)&&(r<1.))
			{
				g.clear();
				Circle(h[i].x,h[i].y,r*20);
				g.fin();
				g.alpha(1-r);
				g.width(8,1);
				g.rgb(1,.5,0);
				g.stroke();
				g.width(2,1);
				g.rgb(1,1,.5);
				g.stroke();
				nh++;
				//char s[128];
				//snprintf(s,128,"%.2f Hz",h[i].f/M_PI);
				//stext(s,h[i].x,h[i].y,0x8000ffff);
			}
		}
		g.alpha(1.);
		char s[128];
		snprintf(s,128,"%i sounds",nh);
		stext(s,320,10,0xffffffff);
	}
};

int mx;
int my;
int mb;
int prevmb;
int prevmx;
int prevmy;

int cursnd;


bool IntersectRaySection(const flt2& r00, const flt2& r01,
    const flt2& s10, const flt2& s11, float& t)
{
	flt2 d0=r01-r00;
	flt2 d1=s11-s10;

	float a1=-d0.y;
	float b1= d0.x;
	float c1=-(a1*r00.x+b1*r00.y);

	float a2=-d1.y;
	float b2= d1.x;
	float c2=-(a2*s10.x+b2*s10.y);

	float d10=a2*r00.x+b2*r00.y+c2;
	float d11=a2*r01.x+b2*r01.y+c2;

	float d00=a1*s10.x+b1*s10.y+c1;
	float d01=a1*s11.x+b1*s11.y+c1;

	if((d10*d11>=0)||(d00*d01>=0))
			return false;

	t=d10/(d10-d11);

	return true;
}

void print(flt2& p)
{
	printf("%f %f\n", p.x, p.y);
}

Hits hits;

#define EL 15150

class CHitSound
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
			echo[ep  ]=r*.65;
			echo[ep+1]=l*.65;
		}
		for(int j=0;j<NH;j++)
		{
			float x=hits.h[j].x;
			float y=hits.h[j].y;
			float l=(sample-hits.h[j].s)/44100.;
			if(((l> -nSamples/44100.))&&(l<(hitl+nSamples/44100.)))
			{
				float f=hits.h[j].f;
				for(int i=0;i<nSamples;i++)
				{
					if((l>0)&&(l<hitl))
					{
						float v=(1.-l/hitl);
						v*=v*v;
						float s=sin(l*f+sin(l*42.)*6*(1-v))*v;
						s=s<-0.6?-0.6:s>0.6?0.6:s;
						s*=.3;

						float b=(x-30)/300.;
						int ep=(echoPos+i)%EL*2;
						echo[ep  ]+=s*(1.-b);
						echo[ep+1]+=s*b;
					}
					l+=1./44100;
				}
			}
		}
		for(int i=0;i<nSamples;i++)
		{
			int ep=(echoPos+i)%EL*2;
			snd.snd_out(echo[ep],echo[ep+1]);
		}
		echoPos=(echoPos+nSamples)%EL;
		sample+=nSamples;
	}
};


int main()
{
	InitWS();
	float dt;
	dt=0.;
	cursnd=0;
	prevmb=0;

	float prevt=Time();

	walls[0].Set(60,60,250,50);
	walls[1].Set(250,50,260,280);
	walls[2].Set(260,280,50,250);
	walls[3].Set(50,250,60,60);
	walls[4].Set(100,105,200,80);
	walls[5].Set(100,105,80,200);
	walls[6].Set(200,80,205,265);
	walls[7].Set(205,265,80,200);

	float bs=.6;
	bullets[0].Set(170,60,55*bs,21*bs);
	bullets[1].Set(120,60,36*bs,38*bs);
	bullets[2].Set(60,120,21*bs,56*bs);

	hits.Init();
	CHitSound hs;
	hs.Init();

	float tf=Time();
	while(true)
	{
		float t=Time()*2.;
		hits.Update(t);
		float dt=t-prevt;
		hits.ns=hs.sample;

		g.clear();
		g.M(0,0);g.l(640,0);g.l(0,480);g.l(-640,0);g.close();g.fin();g.rgb(0.35,0.1,0.0);g.alpha(1);g.fill1();
		g.clear();
		g.alpha(1);

		for(int i=0;i<NB;i++)
		{
			float bt=prevt;
			float bdt=dt;
			Bullet& b=bullets[i];
			while(true)
			{
				float nearestT=10;
				int nearestW=-1;
				for(int j=0;j<NW;j++)
				{
					Wall& w=walls[j];
					float tsect;
					if(IntersectRaySection(b.p,b.p+b.v*bdt,w.p0,w.p1,tsect))
					{
						if(tsect<nearestT)
						{
							nearestT=tsect;
							nearestW=j;
						}
					}
				}
				if(nearestW>=0)
				{
					Wall& w=walls[nearestW];
					b.p+=b.v*bdt*nearestT;
					bt+=nearestT*bdt;
					bdt-=nearestT*bdt;
					b.v=reflect(b.v,(w.p0-w.p1).norm());
					b.p+=b.v*0.00001;
					hits.AddHit(b.p.x,b.p.y);
				}
				else
				{
					b.p+=b.v*bdt;
					break;
				}
			}
		}
		prevt=t;


		snd.Poll();
		while(snd.snd_bufhealth()<10000)
		{
			hs.GenerateSamples(512);
		}

		//if(0)
		{
			g.clear();
			g.M(-1,240);
			for(int i=0;i<640;i+=2)
			{
				float lvl=hs.echo[((hs.echoPos+(i-640)*2+EL)%EL)*2];
				g.L(i,lvl*120+240);
			}
			g.L(641,240);
			g.l(-641,0);
			g.fin();
			g.rgb(0.0,.4,0.0);
			g.fill1();
			g.width(1.,1.);
			g.rgb(0.0,1.0,0.5);
			g.stroke();
			g.clear();
		}

		for(int i=0;i<NW;i++)
		{
			Wall& w=walls[i];
			g.M(w.p0.x,w.p0.y);
			g.L(w.p1.x,w.p1.y);
		}
		g.fin();
		g.width(1.5,1);
		g.rgb(1,1,1);
		g.stroke();

		hits.Render();

		g.clear();
		for(int i=0;i<NB;i++)
		{
			Bullet& b=bullets[i];
			g.M(b.p.x,b.p.y);
			float l=b.v.length()/10.;
			g.l(0-b.v.x/l,0-b.v.y/l);
		}
		g.fin();
		g.width(5,1);
		g.rgb(0,0,1);
		g.stroke();
		g.width(3,1);
		g.rgb(0,1,1);
		g.stroke();

		g.clear();
		for(int i=0;i<NB;i++)
		{
			Bullet& b=bullets[i];
			g.M(b.p.x,b.p.y);
			g.l(0,0);
			//Circle(b.p.x,b.p.y,4);
		}
		g.fin();
		g.rgb(0,0,1);
		g.width(10,1);
		g.stroke();
		g.rgb(0,1,1);
		g.width(5,1);
		g.stroke();
		//g.fill1();

		Present();
		Wait(0.002);
	}
	return 0;
}