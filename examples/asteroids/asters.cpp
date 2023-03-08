#define G_SCREEN_MODE 0
#define G_SCREEN_SCALE 2

#include "graphics.h"

#define M_PI 3.141592654

#include "sound.h"
#include "scinc_utils.h"
#include "asters_sounds.h"
#include "asters_music.h"

Graph g;

void BG()
{
	float T=Time();
	int bgrand=12973460;
	g.graduse(-1);
	g.rgba32(0xff102050);
	g.clear();
	g.M(0,0);g.l(640,0);g.l(0,480);g.l(-640,0);g.close();g.fin();
	g.alpha(1);
	g.fill1();

	for(int i=0;i<200;i++)
	{
		g.clear();
		float x=frand(bgrand)*640;
		float y=frand(bgrand)*480;
		g.M(x,y);
		g.fin();
		g.rgb(1,.5,.2);
		float s=sin(i+T*6.*(.7+frand(bgrand)));
		g.width(3*(2+s),.5);
		g.stroke();
	}

	bgrand=12973460;
	g.clear();
	for(int i=0;i<200;i++)
	{
		float x=frand(bgrand)*640;
		float y=frand(bgrand)*480;
		float s=1+sin(i+T*6.*(.7+frand(bgrand)));
		float h=s*.1+.3;
		s=s*3+2;
		g.M(x-h,y+h);
		g.l(h,s);g.l(h,-s);
		g.l(s,-h);g.l(-s,-h);
		g.l(-h,-s);g.l(-h,s);
		g.l(-s,h);g.l(s,h);
		g.close();
	}
	g.fin();
	g.rgb(1,1,1);
	g.alpha(1);
	g.fill1();
}

void BGMENU()
{
	BG();
	g.graddef(1);
	g.gradstop(0,0,0,0,0);
	g.gradstop(1,0,0,0,1);
	g.alpha(1);
	g.gradtype(2);
	g.gradend();
	g.graduse(1);
	g.clear();
	g.M(0,0);
	g.l(640,0);
	g.l(0,480);
	g.l(-640,0);
	g.close();
	g.fin();
	g.g_0(320,240);
	g.g_x(320*1.4,0);
	g.g_y(0,240*1.4);
	g.fill1();
}

int eseed;
float T;
int sndExplode;
int sndRocket;
int sndShoot;
float rocketT;
int collisI=0;

#define N 12

void Aster(float x, float y, float size, int seed, float hitt, bool collis, int col)
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

	int oldSeed=aseed;
	g.clear();
	g.t_0(x,y);
	for(int i=0;i<N;i++)
	{
		float isize=size*(.95+.1*frand(aseed));
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

	g.graduse(-1);

	if(collis)
	{
		g.rgba32(0xff800000|(collisI+1));
		g.alpha(1);
		g.width(5,1000000);
		g.fill2();
		return;
	}

	g.rgb(.8,.75,.65);
	g.width(0,2);
	g.fill1();
	aseed=oldSeed;

	if((hitt<0)&&(hitt>-1))
	{
		g.alpha(1.+hitt);
		g.rgb(0,.5,1);
		g.fill1();
		g.alpha(1);
	}


	g.clear();
	g.t_0(x+4,y+4);
	g.t_x(ax*.9, ay*.9);
	g.t_y(ay*.9,-ax*.9);
	for(int i=0;i<N;i++)
	{
		float isize=size*(.95+.1*frand(aseed));
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

	g.rgb(.3,.15,.1);
	g.width(size/2,.65);
	g.fill2();

	//g.rgba32(col);

	g.Circle(x,y,0,0,1,0xff00ff00);
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

#define cellSize 40
// 640, 480 /cellSize
#define cellsW (640/cellSize)
#define cellsH (480/cellSize)

 // cellsW*cellsH
 #define nCells  (cellsW*cellsH)


int cells[nCells];
int cellcnt[nCells];

class CAsteroid
{
public:
	float x;
	float y;
	float dx;
	float dy;
	float size;
	int col;
	int seed;
	bool hit;
	float hitt;
	float hvx;
	float hvy;
	float health;
	int selfIndex;
	int nextInCell;
	void Set(float ix, float iy, float idx, float idy, float isize, int iseed)
	{
		x=ix;
		y=iy;
		dx=idx;
		dy=idy;
		size=isize;
		health=size;
		seed=iseed;
		col=(irand(iseed)&255)|((irand(iseed)&255)<<8)|((irand(iseed)&255)<<16)|0xff000000;
	}
	void Update(float dt)
	{
		hit=false;
		hitt-=dt;
		x+=dt*dx;
		y+=dt*dy;

		if(Len(fnear(x-ship.x,640),fnear(y-ship.y,640))<size+SSC*1.5)
		{
			shipHit++;
			ship.vx=-ship.vx;
			ship.vy=-ship.vy;
		}

		if(x<   -size)x=x+640;
		if(x>640+size)x=x-640;
		if(y<   -size)y=y+480;
		if(y>480+size)y=y-480;

		nextInCell=-1;
		int xindex=int((x+6400)/cellSize);
		int yindex=int((y+4800)/cellSize);
		xindex=xindex%cellsW;
		yindex=yindex%cellsH;
		int cellIndex=xindex+yindex*cellsW;
		if(cellIndex>=nCells)cellIndex=0;
		else if(cellIndex<0)cellIndex=nCells-1;

		int oldListHead=cells[cellIndex];
		nextInCell=oldListHead;
		cells[cellIndex]=selfIndex;
		cellcnt[cellIndex]++;
		
	}
	void Draw(bool collis)
	{
			if(x>640-size)Aster(x-640,y,size,seed,hitt,collis,col);
			if(x<  0+size)Aster(x+640,y,size,seed,hitt,collis,col);
			if(y>480-size)Aster(x,y-480,size,seed,hitt,collis,col);
			if(y<  0+size)Aster(x,y+480,size,seed,hitt,collis,col);
			Aster(x,y,size,seed,hitt,collis,col);
			//if(collis)
			//{
			//	char buf[16];
			//	snprintf(buf,16,"%i",collisI);
			//	stext(buf,x,y,0xffff0000);
			//}
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

		int xindex0=int((x+6400+32)/cellSize);
		int yindex0=int((y+4800+24)/cellSize);

		int realHit=-1;

		for(int i=-1;i<=1;i++)
		{
			for(int j=-1;j<=1;j++)
			{

				int xindex1=(xindex0+j);
				int yindex1=(yindex0+i);

				int xindex=xindex1%cellsW;
				int yindex=yindex1%cellsH;

				float x0=x;
				float y0=y;

				int cellIndex=xindex+yindex*cellsW;
				if(cellIndex>=nCells)cellIndex=0;
				else if(cellIndex<0)cellIndex=nCells-1;

				int hitI=cells[cellIndex];

				while(1)
				{
					if((hitI>=0)&&(hitI<nl))
					{
						float dx=asts[hitI].x-x0;
						float dy=asts[hitI].y-y0;
						if(dy>240.)dy-=480.;else if(dy<-240.)dy+=480.;
						if(dx>320.)dx-=640.;else if(dx<-320.)dx+=640.;

						float sz=asts[hitI].size;
						if(((dx*dx+dy*dy)<sz*sz))
						{
							hitx=x;
							hity=y;
							hitt=T;
							realHit=hitI;
							break;
						}
					}
					else break;
					hitI=asts[hitI].nextInCell;
				}
			}
		}

		if(realHit>=0)
		{
			snd_play(sndExplode);
			int i=realHit;
			asts[i].health-=8.;
			asts[i].hitt=0;
			asts[i].hvx=vx;
			asts[i].hvy=vy;
			asts[i].dx+=vx*.5/asts[i].size;
			asts[i].dy+=vy*.5/asts[i].size;
			asts[i].seed=asts[i].seed+irand(eseed);
			nHits++;
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

void AstersUpdateLife()
{
	int i=0;
	while(1)
	{
		if(i>=nl)
			break;
		if(asts[i].health<=0.)
		{
			asts[i].size*=0.6;
			nHits++;
			if(asts[i].size<6)
			{
				if(i<nl-1)
				{
					asts[i]=asts[nl-1];
				}
				nl--;
				continue;
			}
			else
			{
				asts[i].seed=asts[i].seed+irand(eseed);
				asts[i].health=asts[i].size;
				float aspeed=Len(asts[i].dx,asts[i].dy);
				float bspeed=Len(asts[i].hvx,asts[i].hvy);
				asts[i].dx=-asts[i].hvy/bspeed*aspeed+frand(eseed)*130;
				asts[i].dy= asts[i].hvx/bspeed*aspeed+frand(eseed)*130;
				if(nl<NA-1)
				{
					asts[nl]=asts[i];
					asts[nl].dx=-asts[nl].dx+frand(eseed)*30;
					asts[nl].dy=-asts[nl].dy+frand(eseed)*30;
					asts[nl].seed=asts[nl].seed+irand(eseed);
					nl++;
				}
			}
		}
		i++;
	}
}

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
	asts[0].Set(320,10,0,0,25,gseed);
	for(int i=1;i<nl;i++)
		asts[i].Set(640*frand(gseed),480*frand(gseed),frand(gseed)*3,frand(gseed)*3,25+frand(gseed)*12,gseed);

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
	//gtext("LOADING");
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
			//int nsndsamples=Time()*48000-tframe*48000;
			tframe=Time();
			/*if(nsndsamples>2000)nsndsamples=2000;
			music.GenerateSamples(nsndsamples);*/
			while(snd_bufhealth()<(2000+1024))
			{
				music.GenerateSamples(1024);
			}
			g.graduse(-1);

			g.t_0(320-200+sin(Time()*3.)*10,240);
			g.t_x(10,0);
			g.t_y(0,-10);
			g.clear();
			//gtext("Press ENTER");
			g.t_0(320-200+cos(Time()*3.)*10,280);
			//gtext("to start");
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
				puts("Enter!");
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

			for(int i=0;i<nCells;i++)
			{
				cells[i]=-1;
				cellcnt[i]=0;
			}

			for(int i=0;i<nl;i++)
			{
				asts[i].selfIndex=i;
				asts[i].Update(dt);
			}

			for(int i=0;i<NB;i++)
			{
				bullets[i].Update(dt);
			}

			AstersUpdateLife();

			for(int i=0;i<nl;i++)
			{
				asts[i].Draw(false);
			}
			//g.BitBlt(0,0,hitRT);

			g.t_0(0,0);
			g.t_x(1,0);
			g.t_y(0,1);


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
			//gtext(ss);

			snprintf(ss,64,"%f", Time());
			g.t_0(5,20);
			//gtext(ss);

			snprintf(ss,64,"%i asteroids", nl);
			g.t_0(5,35);
			//gtext(ss);

			snprintf(ss,64,"ship health %5.2f %%", ship.health*100);
			g.t_0(450,35);
			//gtext(ss);

			g.fin();
			g.width(1.25,1.25);
			g.rgb(.5,.8,1.0);
			g.stroke();

			g.t_0(0,0);
			g.t_x(1,0);
			g.t_y(0,1);
			g.clear();
			int index=0;
			int nic=cellSize/8;
			for(int i=0;i<cellsH;i++)
			{
				for(int j=0;j<cellsW;j++)
				{
					for(int indexInCell=0;indexInCell<cellcnt[index];indexInCell++)
					{
						int xi=indexInCell%nic;
						int yi=indexInCell/nic;
						g.M(j*cellSize+4+xi*8,i*cellSize+4+yi*8);
						g.l(.1,.1);
					}
					index++;
				}
			}
			g.fin();
			g.width(2.,2.);
			g.rgba(1,0,1,1);
			g.stroke();
			g.alpha(1);

			for(int i=0;i<cellsH;i++)
			{
				g.lineH(0,i*cellSize,640,0x80ffffff);
			}
			for(int j=0;j<cellsW;j++)
			{
				g.lineV(j*cellSize,0,480,0x80ffffff);
			}


			if(nl==0)
			{
				g.clear();
				g.t_0(50,200);
				g.t_x(5.5,0);
				g.t_y(0,-7.5);
				//gtext("All asteroids destroyed, press m for menu");
				g.fin();
				g.width(1.25,1.25);
				g.rgb(.5,.2,1.0);
				g.stroke();
			}


			Present();
		}
	}
	return 0;
}