#include "graphics.h"

#define M_PI 3.141592654

#include "scinc_utils.h"
#include "asters_sounds.h"
#include "asters_music.h"

void BG()
{
	graduse(-1);
	gray(0);
	clear();
	fill1();
}

void BGMENU()
{
	float t=-Time();
	graddef(0);
	for(int i=0;i<256;i+=16)
	{
		float s=i*1./256.0;
		gradstop(s,
			.5+.15*sin(s*3.35*3.-t*2.85+17.8)+.15*sin(-s*14.85*1.6+(t+1.8)*2.85*1.13+7.8),
			.15+.15*sin(s*2.81*3.-t*3.58+12.2)+.15*sin(-s*16.11*1.6+(t+5.7)*3.58*1.12+2.2),
			.15+.15*sin(s*3.37*3.-t*2.59+27.7)+.15*sin(-s*15.37*1.6+(t+8.7)*2.59*1.11+7.7),
			1
		);
	}
	alpha(1);
	gradend();
	graduse(0);
	clear();
	M(0,0);
	l(640,0);
	l(0,480);
	l(-640,0);
	close();
	fin();
	g_0(320,240);
	float r=240*1.7;
	g_x(r,0);
	g_y(0,r);
	fill1();
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

	t_0(x,y);
	t_x(ax,ay);
	t_y(ay,-ax);

	/*
	clear();
	for(int i=0;i<32;i++)
	{
		float a=i/31.*2.*M_PI;
		L(size*sin(a),size*cos(a));
	}
	fin();
	rgb(0,.2,.5);
	width(2,1);
	stroke();
	*/

	float xp;
	float yp;

	clear();
	for(int i=0;i<N;i++)
	{
		float isize=size*(.7+.3*frand(aseed));
		float a=i*2.*M_PI/N;
		float xi=sin(a)*isize;
		float yi=cos(a)*isize;
		if(i==0)
		{
			M(xi,yi);
		}
		else
		{
			L(xi,yi);
		}
		xp=xi;
		yp=yi;
	}
	close();
	fin();
	rgb(.7,1.,.2);

	alpha(.3);
	width(8.,1);
	//stroke();
	width(1.5,1);
	alpha(1);
	stroke();
	alpha(.3);
	if(hit)
		rgb(0,0,1);
	fill1();
	alpha(1);
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
	
		clear();
		M(x,y);
		L(x-ax*.5*SSC,y-ay*.5*SSC);
		L(x-(ax+ay)*SSC,y-(ay-ax)*SSC);
		L(x+ax*1.5*SSC,y+ay*1.5*SSC);
		L(x-(ax-ay)*SSC,y-(ay+ax)*SSC);
		L(x-ax*.5*SSC,y-ay*.5*SSC);
		fin();
		alpha(.5);
		rgb(1,.7,.5);
		if(shipHit)
		{
			rgb(0,0,1);
			health-=0.05*shipHit;
			shipHit=0;
		}
		fill1();
		width(2,2);
		stroke();
		alpha(1);
		rgb(1,.9,.2);
		width(1,1);
		stroke();

		if(fire)
		{
			int seed=Time()*187;
			clear();
			for(int i=0;i<5;i++)
			{
				M(x-ax*.5*SSC,y-ay*.5*SSC);
				l(-ax*SSC+frand(seed)*6,-ay*SSC+frand(seed)*6);
			}
			fin();
			alpha(.5);
			rgb(0.1,.7,1);
			width(14,1.5);
			stroke();
			alpha(1);
			rgb(.5,1,1);
			width(3,1);
			stroke();
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
	clear();
	for(int i=0;i<18;i++)
	{
		M(x,y);
		l(frand(eseed)*150*t,frand(eseed)*150*t);
	}
	fin();
	alpha(1.-t*3.);
	rgb(0.1,.6,1);
	width(10.+20.*t,1.);
	stroke();
	if(t<0.1)
	{
		alpha(.7);
		rgb(.5,1,1);
		width(3,2);
		stroke();
		alpha(1);
		rgb(1,.9,.7);
		width(1,1);
		stroke();
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
		clear();
		M(x+vx*0.015,y+vy*0.015);
		L(x-vx*0.015,y-vy*0.015);
		fin();
		alpha(.6);
		rgb(.5,0,1);
		width(8,1);
		stroke();
		alpha(1);
		rgb(0,.5,1);
		width(2,1);
		stroke();
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
	t_0(320-7/2.*2.5*15,240);
	t_x(15,0);
	t_y(5,-20);
	clear();
	gtext("LOADING");
	fin();
	width(5,5);
	rgb(1.0,.5,0.0);
	stroke();
	width(2,2);
	rgb(1,1,1);
	stroke();
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
		t_0(0,0);
		t_x(1,0);
		t_y(0,1);
		if(state==MENU)
		{
			BGMENU();
			int nsndsamples=Time()*44100-tframe*44100;
			tframe=Time();
			if(nsndsamples>2000)nsndsamples=2000;
			music.GenerateSamples(nsndsamples);

			t_0(320-200+sin(Time()*3.)*10,240);
			t_x(10,0);
			t_y(0,-10);
			clear();
			gtext("Press ENTER");
			t_0(320-200+cos(Time()*3.)*10,280);
			gtext("to start");
			fin();
			width(3,2);
			alpha(1);
			rgb(.6,.2,0.0);
			stroke();
			width(2,1);
			rgb(1,1,1);
			stroke();
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
				clear();
				M(x,y);
				L(x+ship.ax,y+ship.ay);
				fin();
				alpha(1-(T-fireT)*20);
				rgb(0,0,1);
				width(18,1.);
				stroke();
			}


			for(int i=0;i<nl;i++)
			{
				asts[i].Update(dt);
			}

			for(int i=0;i<nl;i++)
			{
				asts[i].Draw();
			}
			t_0(0,0);
			t_x(1,0);
			t_y(0,1);

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
			t_0(320-len/2.*2.5*6,20);
			t_x(3.5,0);
			t_y(0,-4.5);
			clear();
			gtext(ss);

			snprintf(ss,64,"%f", Time());
			t_0(5,20);
			gtext(ss);

			snprintf(ss,64,"%i asteroids", nl);
			t_0(5,35);
			gtext(ss);

			snprintf(ss,64,"ship health %5.2f %%", ship.health*100);
			t_0(450,35);
			gtext(ss);

			fin();
			width(1.25,1.25);
			rgb(.5,.8,1.0);
			stroke();

			Present();
		}
	}
	return 0;
}