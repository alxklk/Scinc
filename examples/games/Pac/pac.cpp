#define G_SCREEN_WIDTH 640
#define G_SCREEN_HEIGHT 480
#define G_SCREEN_SCALE 3
#define G_SCREEN_MODE 2

#define M_PI 3.141592654

#include "sound.h"
#include "graphics.h"

Graph g;

int iseed=3453245;

int irand(int& seed)
{
	seed=(seed*1103515245+12345)&0x7ffffff;
	return seed;
}

void Background()
{
	float t=Time();
	for(int i=0;i<G_SCREEN_HEIGHT;i+=16)
	{
		int gr=int(sin(t*.147+i*.011-54)*40+48);
		int gg=int(sin(t*.149-i*.017+13)*40+48);
		int gb=int(sin(t*.141+i*.013-77)*40+48);
		g.Rect(0,i,G_SCREEN_WIDTH,16,0xff000000|(gr<<16)|(gg<<8)|gb);
	}
}

void Cell(int s, int u, int d, int l, int r)
{
	if(!s)
	{
		return;
	}
}

int w;
int h;
bool put=false;

char* walls=
"********************************"
"*                              *"
"* ************** ************* *"
"*       *               *      *"
"* **************************** *"
"*               *              *"
"* **************************** *"
"*                              *"
"* ************** ************* *"
"* *                          * *"
"* ****** *************** ***** *"
"*               *              *"
"* **************************** *"
"*                              *"
"* *************  ************* *"
"         *            *         "
"* ****** *  *  **  *  * ****** *"
"*        *            *        *"
"* **************************** *"
"*                              *"
"* ************** ************* *"
"*                              *"
"* ******* ************ ******* *"
"* *                          * *"
"* * ************************ * *"
"* *      *           *       * *"
"* ************* ************** *"
"*                              *"
"* **************************** *"
"*                              *"
"********************************"
"                                ";

char objs[1024];

#define Wall(x, y) walls[((x)+32)%32+((y)+32)%32*32]


struct GameActor
{
	int x;
	int y;

	int dx;
	int dy;
	int nextdx;
	int nextdy;
	bool moveblocked;
	float progress;
	float speed;
	int type;
	int col;

	static void MonsterRandomMove(GameActor& mon, GameActor& pac)
	{
		bool dirNotChosen=false;

		while(true)
		{
			int ir=irand(iseed)%7;
			int dirX=0;
			int dirY=0;
			/* */if(ir==0){dirX= 1;}
			else if(ir==1){dirY= 1;}
			else if(ir==2){dirX=-1;}
			else if(ir==3){dirY=-1;}
			//else if(ir==14){dirX=pac.nextdx;dirY=pac.nextdy;}
			else if(ir==5){dirX=(mon.x<pac.x)?1:-1;}
			else if(ir==6){dirY=(mon.y<pac.y)?1:-1;}
			else {dirX=-1;}
			if(Wall(mon.x+dirX,mon.y+dirY)==' ')
			{
				mon.CommandMove(dirX,dirY);
				break;
			}
		}
	}

	void Tick(float idt)
	{
		progress+=idt*speed;
		if(progress>1)
		{
			progress-=1.;
			if(Wall(x+dx,y+dy)==' ')
			{
				x+=dx;
				y+=dy;
				x=(x+32)%32;
				y=(y+32)%32;
				if(Wall(x+dx,y+dy)!=' ')	
				{
					dx=0;
					dy=0;
					if(type==1)
					{
						MonsterRandomMove(*this,*this);
					}
				}
				moveblocked=false;
			}
		}

		if(!moveblocked)
		{
			if(nextdx==-1)
			{
				if(Wall(x-1,y)==' ')
				{
					dx=-1;
					dy=0;
					nextdx==0;
				}
			}
			else if(nextdx==1)
			{
				if(Wall(x+1,y)==' ')
				{
					dx=1;
					dy=0;
					nextdx==0;
				}
			}
			else if(nextdy==-1)
			{
				if(Wall(x,y-1)==' ')
				{
					dx=0;
					dy=-1;
					nextdy==0;
				}
			}
			else if(nextdy==1)
			{
				if(Wall(x,y+1)==' ')
				{
					dx=0;
					dy=1;
					nextdy==0;
				}
			}
		}
	}
	void CommandMove(int idx, int idy)
	{
		nextdx=idx;
		nextdy=idy;
		moveblocked=true;
		//if((dx!=0)&&(nextdx==-dx))
		//{
		//	moveblocked=false;
		//	dx=nextdx;
		//	progress=1.-progress;
		//	x=x-dx;
		//}

	}
	void Born(int ix, int iy, int itype)
	{
		x=ix;
		y=iy;
		dx=0;
		dy=0;
		nextdx=0;
		nextdy=0;
		progress=0;
		speed=3.5;
		type=itype;
	}
};

GameActor pac;
GameActor mon0;
GameActor mon1;
GameActor mon2;

int cell=15;

void DrawPac()
{
	g.clear();
	float a=pac.progress;//(sin(t*M_PI*10)*.5+.5)*M_PI/3;
	float r=10;
	float progress=pac.progress;
	float cx=pac.x*cell+cell*pac.dx*pac.progress;
	float cy=pac.y*cell+cell*pac.dy*pac.progress;
	if(pac.dx==1)
	{
		g.M(cx+r-r*(1.-cos(a)),cy-sin(a)*r);
		g.a(r,r,0,1,0,0,sin(a)*r*2+.01);
		g.l(-r*cos(a),-sin(a)*r);
		g.l( r*cos(a),-sin(a)*r);
	}
	else if(pac.dx==-1)
	{
		g.M(cx-r+r*(1.-cos(a)),cy-sin(a)*r);
		g.a(r,r,0,1,1,0,sin(a)*r*2+.01);
		g.l( r*cos(a),-sin(a)*r);
		g.l(-r*cos(a),-sin(a)*r);
	}
	else if(pac.dy==-1)
	{
		g.M(cx-sin(a)*r,cy-r+r*(1.-cos(a)));
		g.a(r,r,0,1,0,sin(a)*r*2+.01,0);
		g.l(-sin(a)*r, r*cos(a));
		g.l(-sin(a)*r,-r*cos(a));
	}
	else if(pac.dy==1)
	{
		g.M(cx-sin(a)*r,cy+r-r*(1.-cos(a)));
		g.a(r,r,0,1,1,sin(a)*r*2+.01,0);
		g.l(-sin(a)*r,-r*cos(a));
		g.l(-sin(a)*r, r*cos(a));
	}
	else
	{
		g.M(cx,cy-r);
		g.a(r,r,0,1,0,0,r*2);
		g.a(r,r,0,1,0,0,-r*2);
		g.close();
		g.M(cx-r/2,cy+r/4);
		g.a(r,r*.5,0,0,0, r,0);
		g.a(r,r*3,0,0,1,-r,0);
	}
	g.close();
	g.fin();
	g.rgba32(0xffffff00);
	g.fill1();
}

int mx=-1;
int my=-1;

void DrawMonster(GameActor& mon)
{
	float x=(mon.x+mon.progress*mon.dx)*cell;
	float y=(mon.y+mon.progress*mon.dy)*cell;
	float r=10;
	int col=mon.col;
	int dx=mon.dx;
	int dy=mon.dy;
	g.clear();
	g.M(x-r,y);
	g.a(r,r,0,0,1,r*2,0);
	g.l(0,r*2);
	/*
	for(int i=0;i<=8;i++)
	{
		float fi=i/8.;
		float tl=sin(fi*M_PI*4)*(.8+cos(Time()*21+fi*9)*.2);
		if(tl<0)tl=-tl;
		g.L(x+r-r*2*fi+r*.3*tl*sin(Time()*9+fi*9),y+r/2+tl*r/2*1.5);
	}
	*/
	for(int i=0;i<=64;i++)
	{
		float fi=i/64.;
		int tn=i/16;
		float tl=fi*4;tl=tl-int(tl);if(tl>.5)tl=1-tl;tl*=2.;tl=sqrt(tl);
		tl*=(.9+cos(Time()*13+tn/4.*5)*.3);
		g.L(x+r-r*2*fi+r*.5*tl*tl*sin(Time()*12+tn/4.*29+tl*3),y+r/2+tl*r/2*1.8);
	}
	g.close();
	g.rgba32(col);
	g.fin();
	g.fill1();
	g.rgba32(0xffffffff);
	g.clear();
	float ew=r*.35;
	float eh=r*.4;
	g.M(x-r*.4-ew,y-eh*.8);
	g.a(ew,eh,0,1,1,0,0.1);
	g.close();
	g.M(x+r*.4-ew,y-eh*.8);
	g.a(ew,eh,0,1,1,0,0.1);
	g.close();
	g.fin();
	g.fill1();
	//g.Circle(x-r*.4+sin(Time()*4)*r*.1,y-eh+sin(Time()*2.3-12.)*r*.05,0,r*.08,1,0xff000000);
	//g.Circle(x+r*.4+sin(Time()*4)*r*.1,y-eh+sin(Time()*2.3-12.)*r*.05,0,r*.08,1,0xff000000);
	g.Circle(x-r*.4+mon.dx*r*.2,y-eh+mon.dy*r*.2,0,r*.08,1,0xff000000);
	g.Circle(x+r*.4+mon.dx*r*.2,y-eh+mon.dy*r*.2,0,r*.08,1,0xff000000);
	//g.stroke();

}

int main()
{
	pac.Born(5,5,0);
	mon0.Born(12,17,1);
	mon0.CommandMove(1,0);
	mon1.Born(16,17,1);
	mon1.CommandMove(1,0);
	mon2.Born(20,17,1);
	mon2.CommandMove(1,0);
	pac.speed=5.;
	mon0.speed=5.1;
	mon1.speed=5.2;
	mon2.speed=5.3;
	mon0.col=0x80ff00ff;
	mon1.col=0x80c0ff00;
	mon2.col=0x8000ffff;
	float prevt=Time();

	for(int i=0;i<32;i++)
	{
		for(int j=0;j<32;j++)
		{
			if(walls[j+i*32]==' ')
			{
				objs[j+i*32]=1;
			}
			else
			{
				objs[j+i*32]=0;
			}
		}
	}

	while(1)
	{
		float t=Time();
		g.gray(0);
		g.FillRT();
		g.fillrect(mx*cell-cell/2,my*cell-cell/2,cell,cell,0xff004080);
		for(int i=0;i<32;i++)
		{
			for(int j=0;j<32;j++)
			{
				if(objs[j+i*32]!=0)
				{
					g.Circle(j*cell, i*cell, 0, 1, 1, 0xffc0c040);
				}
			}
		}


		for(int i=0;i<32;i++)
		{
			for(int j=0;j<32;j++)
			{
				if(walls[j+i*32]!=' ')
				{
					//g.fillrect(j*cell-cell/2,i*cell-cell/2,cell,cell,0x40ffff00);
					bool chained=false;
					if(j&&(walls[j-1+i*32]!=' '))
					{
						chained=true;
						g.fillrect(j*cell-8,i*cell,8,1,0xff0080ff);
					}
					if((j<31)&&(walls[j+1+i*32]!=' '))
					{
						chained=true;
						g.fillrect(j*cell,i*cell,8,1,0xff0080ff);
					}
					if(i&&(walls[j+i*32-32]!=' '))
					{
						chained=true;
						g.fillrect(j*cell,i*cell-8,1,9,0xff0080ff);
					}
					if((i<31)&&(walls[j+i*32+32]!=' '))
					{
						chained=true;
						g.fillrect(j*cell,i*cell,1,8,0xff0080ff);
					}
					if(!chained)
						g.Circle(j*cell,i*cell,1.5,.5,.5,0xff0080ff);
				}
			}
		}
		g.clear();
		g.width(1,1);
		g.rgba32(0xffffff00);
		char s[60];
		snprintf(s,60,"%i %i",mx, my);
		stext(s,200,10,0xff00ffff);

		mon0.Tick(t-prevt);
		mon1.Tick(t-prevt);
		mon2.Tick(t-prevt);
		if(int(t*10+27)%25==0)GameActor::MonsterRandomMove(mon0,pac);
		if(int(t*12+17)%23==0)GameActor::MonsterRandomMove(mon1,pac);
		if(int(t*17+57)%26==0)GameActor::MonsterRandomMove(mon2,pac);
		//DrawMonster(200,200,50,0x80ff8000,1,0);
		DrawMonster(mon0);
		DrawMonster(mon1);
		DrawMonster(mon2);

		pac.Tick(t-prevt);
		objs[pac.x+pac.y*32]=0;
		DrawPac();

		Present();


		SScincEvent ev;
		while(GetScincEvent(ev))
		{
			if(ev.type=='MLDN')
			{
				mx=float(ev._1)/cell+.5;
				my=float(ev._2)/cell+.5;
				if((mx>=0)&&(my>=0)&&(mx<32)&&(my<32))
				{
					if(walls[mx+my*32]==' ')
					{
						walls[mx+my*32]='*';
						put=true;
					}
					else
					{
						walls[mx+my*32]=' ';
						put=false;
					}
				}
			}
			if(ev.type=='MMOV')
			{
				mx=float(ev._1)/cell+.5;
				my=float(ev._2)/cell+.5;
				if(ev._0&1)
				{
					if((mx>=0)&&(my>=0)&&(mx<32)&&(my<32))
					{
						if(put)
							walls[mx+my*32]='*';
						else
							walls[mx+my*32]=' ';
					}
				}
			}
			if(ev.type=='KBDN')
			{
				int key=ev._0;
				{
					//printf("Key %i\n",key);
					if(key==4002)
					{
						pac.CommandMove(-1,0);
					}
					else if(key==4003)
					{
						pac.CommandMove(1,0);
					}
					else if(key==4000)
					{
						pac.CommandMove(0,-1);
					}
					else if(key==4001)
					{
						pac.CommandMove(0,1);
					}
				}
			}
		}
		prevt=t;
	}
	return 0;
}
