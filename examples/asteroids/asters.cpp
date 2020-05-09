#include "graphics.h"

#define M_PI 3.141592654

#include "scinc_utils.h"
#include "asters_sounds.h"
#include "asters_music.h"

Graph g;

void BG()
{
	g.graduse(-1);
	g.gray(0);
	g.clear();
	g.fill1();
}

void BGMENU()
{
	float t=-Time();
	g.graddef(0);
	for(int i=0;i<256;i+=16)
	{
		float s=i*1./256.0;
		g.gradstop(s,
			.5+.15*sin(s*3.35*3.-t*2.85+17.8)+.15*sin(-s*14.85*1.6+(t+1.8)*2.85*1.13+7.8),
			.15+.15*sin(s*2.81*3.-t*3.58+12.2)+.15*sin(-s*16.11*1.6+(t+5.7)*3.58*1.12+2.2),
			.15+.15*sin(s*3.37*3.-t*2.59+27.7)+.15*sin(-s*15.37*1.6+(t+8.7)*2.59*1.11+7.7),
			1
		);
	}
	g.alpha(1);
	g.gradend();
	g.graduse(0);
	g.clear();
	g.M(0,0);
	g.l(640,0);
	g.l(0,480);
	g.l(-640,0);
	g.close();
	g.fin();
	g.g_0(320,240);
	float r=240*1.7;
	g.g_x(r,0);
	g.g_y(0,r);
	g.fill1();
}

int eseed;
float T;
int sndExplode;
int sndRocket;
int sndShoot;
float rocketT;

#define N 10

void Aster(float x, float y, float size, int seed, int hit)
{
	int aseed=seed;

	float a=T*frand(aseed)*3.;
	float ax=sin(a);
	float ay=cos(a);

	g.t_0(x,y);
	g.t_x(ax,ay);
	g.t_y(ay,-ax);

	/*
	g.clear();
	for(int i=0;i<32;i++)
	{
		float a=i/31.*2.*M_PI;
		g.L(size*sin(a),size*cos(a));
	}
	g.fin();
	g.rgb(0,.2,.5);
	g.width(2,1);
	g.stroke();
	*/

	float xp;
	float yp;

	g.clear();
	for(int i=0;i<N;i++)
	{
		float isize=size*(.7+.3*frand(aseed));
		float a=i*2.*M_PI/N;
		float xi=sin(a)*isize;
		float yi=cos(a)*isize;
		if(i==0)
		{
			g.M(xi,yi);
		}
		else
		{
			g.L(xi,yi);
		}
		xp=xi;
		yp=yi;
	}
	g.close();
	g.fin();
	g.rgb(.7,1.,.2);

	g.alpha(.3);
	g.width(8.,1);
	//g.stroke();
	g.width(1.5,1);
	g.alpha(1);
	g.stroke();
	g.alpha(.3);
	if(hit)
		g.rgb(0,0,1);
	g.fill1();
	g.alpha(1);
}

#define SSC 10
int shipHit;
class CSpaceShip
{
public:
	int fire;
	float x;
	float y;
	float vx;
	float vy;
	float ax;
	float ay;
	float health;
	void Update(float dt)
	{
		x=x+vx*dt;
		y=y+vy*dt;
		if(x<0)x+=640.;
		if(x>640)x-=640.;
		if(y<0)y+=480.;
		if(y>480)y-=480.;
		if(fire&&(T-rocketT>0.1))
		{
			snd_play(sndRocket);
			rocketT=T;
		}
	}
	void Draw()
	{
		/*
		clear();
		for(int i=0;i<32;i++)
		{
			float a=i/31.*2.*M_PI;
			L(x+SSC*1.5*sin(a),y+SSC*1.5*cos(a));
		}
		fin();
		rgb(0,.2,.5);
		width(2,1);
		stroke();
		*/
	
		g.clear();
		g.M(x,y);
		g.L(x-ax*.5*SSC,y-ay*.5*SSC);
		g.L(x-(ax+ay)*SSC,y-(ay-ax)*SSC);
		g.L(x+ax*1.5*SSC,y+ay*1.5*SSC);
		g.L(x-(ax-ay)*SSC,y-(ay+ax)*SSC);
		g.L(x-ax*.5*SSC,y-ay*.5*SSC);
		g.fin();
		g.alpha(.5);
		g.rgb(1,.7,.5);
		if(shipHit)
		{
			g.rgb(0,0,1);
			health-=0.05*shipHit;
			shipHit=0;
		}
		g.fill1();
		g.width(2,2);
		g.stroke();
		g.alpha(1);
		g.rgb(1,.9,.2);
		g.width(1,1);
		g.stroke();

		if(fire)
		{
			int seed=Time()*187;
			g.clear();
			for(int i=0;i<5;i++)
			{
				g.M(x-ax*.5*SSC,y-ay*.5*SSC);
				g.l(-ax*SSC+frand(seed)*6,-ay*SSC+frand(seed)*6);
			}
			g.fin();
			g.alpha(.5);
			g.rgb(0.1,.7,1);
			g.width(14,1.5);
			g.stroke();
			g.alpha(1);
			g.rgb(.5,1,1);
			g.width(3,1);
			g.stroke();
		}
	}
};

CSpaceShip ship;
class CAsteroid
{
public:
	float x;
	float y;
	float dx;
	float dy;
	float size;
	int seed;
	int hit;
	void Set(float ix, float iy, float idx, float idy, float isize, int iseed)
	{
		x=ix;
		y=iy;
		dx=idx;
		dy=idy;
		size=isize;
		seed=iseed;
	}
	void Update(float dt)
	{
		hit=0;
		x+=dt*dx;
		y+=dt*dy;

		if(Len(fnear(x-ship.x,640),fnear(y-ship.y,640))<size+SSC*1.5)
		{
			shipHit++;
			hit=1;
			ship.vx=-ship.vx;
			ship.vy=-ship.vy;
		}

		if(x<   -size)x=x+640;
		if(x>640+size)x=x-640;
		if(y<   -size)y=y+480;
		if(y>480+size)y=y-480;
	}
	void Draw()
	{
			if(x>640-size)Aster(x-640,y,size,seed,hit);
			if(x<  0+size)Aster(x+640,y,size,seed,hit);
			if(y>480-size)Aster(x,y-480,size,seed,hit);
			if(y<  0+size)Aster(x,y+480,size,seed,hit);
			Aster(x,y,size,seed,hit);
	}
};

#define NA 60
int nl;
CAsteroid asts[NA];

void Explode(float x, float y, float t)
{
	if(t>.15)
		return;
	g.clear();
	for(int i=0;i<18;i++)
	{
		g.M(x,y);
		g.l(frand(eseed)*150*t,frand(eseed)*150*t);
	}
	g.fin();
	g.alpha(1.-t*3.);
	g.rgb(0.1,.6,1);
	g.width(10.+20.*t,1.);
	g.stroke();
	if(t<0.1)
	{
		g.alpha(.7);
		g.rgb(.5,1,1);
		g.width(3,2);
		g.stroke();
		g.alpha(1);
		g.rgb(1,.9,.7);
		g.width(1,1);
		g.stroke();
	}
}


float hitx;
float hity;
float hitt;

int nHits;
int nShots;

class CBullet
{
public:
	float t;
	float x;
	float y;
	float vx;
	float vy;
	void Update(float dt)
	{
		x=x+vx*dt;
		y=y+vy*dt;
		if(x<0)x+=640.;
		if(x>640)x-=640.;
		if(y<0)y+=480.;
		if(y>480)y-=480.;
		int c=GetPixel(x+vx*0.015,y+vy*0.015);
		if(((c&0x0000ff00)>>8)>((c&0x00ff0000)>>16))
		{
			hitx=x;
			hity=y;
			hitt=T;
			snd_play(sndExplode);
			for(int i=0;i<nl;i++)
			{
				float d=Len(fnear(asts[i].x-hitx,640),fnear(asts[i].y-hity,480));
				if(d<(asts[i].size)+6)
				{
					asts[i].size*=0.6;
					asts[i].seed=asts[i].seed+irand(eseed);
					nHits++;
					if(asts[i].size<6)
					{
						if(i<nl-1)
						{
							asts[i]=asts[nl-1];
						}
						nl--;
						break;
					}
					else
					{
						float aspeed=Len(asts[i].dx,asts[i].dy);
						float bspeed=Len(vx,vy);
						asts[i].dx=-vy/bspeed*aspeed+frand(eseed)*30;
						asts[i].dy= vx/bspeed*aspeed+frand(eseed)*30;
						if(nl<NA-1)
						{
							asts[nl]=asts[i];
							asts[nl].dx=-asts[nl].dx+frand(eseed)*30;
							asts[nl].dy=-asts[nl].dy+frand(eseed)*30;
							asts[nl].seed=asts[nl].seed+irand(eseed);
							nl++;
						}
					}
					break;
				}
			}
			t-=10.;
		}
	}
	void Draw()
	{
		g.clear();
		g.M(x+vx*0.015,y+vy*0.015);
		g.L(x-vx*0.015,y-vy*0.015);
		g.fin();
		g.alpha(.6);
		g.rgb(.5,0,1);
		g.width(8,1);
		g.stroke();
		g.alpha(1);
		g.rgb(0,.5,1);
		g.width(2,1);
		g.stroke();
	}
};

#define MAXB 40
CBullet bullets[MAXB];
int NB;

float tframe;

#define MENU 1
#define GAME 2

float t0;
float fireT;
int sndsample;
int gseed;

void ResetGame()
{
	NB=0;
	nl=7;
	asts[0].Set(640,280,0,0,25,gseed);
	for(int i=1;i<nl;i++)
		asts[i].Set(640*frand(gseed),480*frand(gseed),frand(gseed)*30,frand(gseed)*30,25+frand(gseed)*12,gseed);

	t0=Time();
	ship.x=320;
	ship.y=240;
	ship.vx=0;
	ship.vy=0;
	ship.health=1.;
	ship.fire=0;
	fireT=-100;
	rocketT=-100;
	hitx=-100;
	hity=-100;
	nHits=0;
	nShots=0;
	shipHit=0;
}

int main()
{
	BG();
	g.t_0(320-7/2.*2.5*15,240);
	g.t_x(15,0);
	g.t_y(5,-20);
	g.clear();
	gtext("LOADING");
	g.fin();
	g.width(5,5);
	g.rgb(1.0,.5,0.0);
	g.stroke();
	g.width(2,2);
	g.rgb(1,1,1);
	g.stroke();
	Present();

	gseed=341199578;
	eseed=7243456;

	sndExplode=MakeExplodeSound(20000,eseed);
	sndShoot=MakeShootSound(12000);
	sndRocket=MakeEngineSound(12000,eseed);

	int state=MENU;
	int menuPos=0;
	CMusic music;
	music.Init();

	while(true)
	{
		g.t_0(0,0);
		g.t_x(1,0);
		g.t_y(0,1);
		if(state==MENU)
		{
			BGMENU();
			int nsndsamples=Time()*44100-tframe*44100;
			tframe=Time();
			if(nsndsamples>2000)nsndsamples=2000;
			music.GenerateSamples(nsndsamples);

			g.t_0(320-200+sin(Time()*3.)*10,240);
			g.t_x(10,0);
			g.t_y(0,-10);
			g.clear();
			gtext("Press ENTER");
			g.t_0(320-200+cos(Time()*3.)*10,280);
			gtext("to start");
			g.fin();
			g.width(3,2);
			g.alpha(1);
			g.rgb(.6,.2,0.0);
			g.stroke();
			g.width(2,1);
			g.rgb(1,1,1);
			g.stroke();
			Present();

			if(KeyPressed(4013))
			{
				state=GAME;
				ResetGame();
			}
			if(KeyPressed('q'))
			{
				exit(0);
			}
		}
		else if(state==GAME)
		{
			BG();
			if(ship.health<0)
				state=MENU;
			if(KeyPressed('m'))
			{
				state=MENU;
				sndsample=0;
			}
			T=Time();
			float dt=T-t0;
			t0=T;

			int mx;
			int my;
			int mb;
			GetMouseState(mx,my,mb);

			if((mb&1)&&(NB<MAXB))
			{
				if(T-fireT>1./8.)
				{
					snd_play(sndShoot);
					nShots++;
					bullets[NB].t=T;
					bullets[NB].x=ship.x+ship.ax*SSC*2.;
					bullets[NB].y=ship.y+ship.ay*SSC*2.;
					bullets[NB].vx=ship.ax*400;
					bullets[NB].vy=ship.ay*400;
					NB++;
					fireT=T;
				}
			}

			if(T-fireT<1./20)
			{
				float x=ship.x+ship.ax*SSC*2.;
				float y=ship.y+ship.ay*SSC*2.;
				g.clear();
				g.M(x,y);
				g.L(x+ship.ax,y+ship.ay);
				g.fin();
				g.alpha(1-(T-fireT)*20);
				g.rgb(0,0,1);
				g.width(18,1.);
				g.stroke();
			}


			for(int i=0;i<nl;i++)
			{
				asts[i].Update(dt);
			}

			for(int i=0;i<nl;i++)
			{
				asts[i].Draw();
			}
			g.t_0(0,0);
			g.t_x(1,0);
			g.t_y(0,1);

			for(int i=0;i<NB;i++)
			{
				bullets[i].Update(dt);
			}

			{
				int i=0;
				while(true)
				{
					if(i>=NB)
						break;
					if((T-bullets[i].t)>.65)
					{
						bullets[i]=bullets[NB-1];
						NB--;
					}
					i++;
				}
			}

			for(int i=0;i<NB;i++)
			{
				bullets[i].Draw();
			}

			if((hitx>0)&&(hity>0))
			{
				Explode(hitx, hity, Time()-hitt);
			}

			ship.ax=mx-ship.x;
			ship.ay=my-ship.y;
			float l=sqrt((ship.ax*ship.ax)+(ship.ay*ship.ay));
			ship.ax=ship.ax/l;
			ship.ay=ship.ay/l;
			ship.Update(dt);
			ship.Draw();
			if(KeyPressed('s')||(mb&4))
			{
				ship.vx+=ship.ax*2.;
				ship.vy+=ship.ay*2.;
				ship.fire=1;
			}
			else
			{
				ship.fire=0;
			}

			char ss[64];
			int len=snprintf(ss,64,"Hits %i of %i", nHits, nShots);
			g.t_0(320-len/2.*2.5*6,20);
			g.t_x(3.5,0);
			g.t_y(0,-4.5);
			g.clear();
			gtext(ss);

			snprintf(ss,64,"%f", Time());
			g.t_0(5,20);
			gtext(ss);

			snprintf(ss,64,"%i asteroids", nl);
			g.t_0(5,35);
			gtext(ss);

			snprintf(ss,64,"ship health %5.2f %%", ship.health*100);
			g.t_0(450,35);
			gtext(ss);

			g.fin();
			g.width(1.25,1.25);
			g.rgb(.5,.8,1.0);
			g.stroke();

			Present();
		}
	}
	return 0;
}