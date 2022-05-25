#define G_SCREEN_WIDTH 320
#define G_SCREEN_HEIGHT 240
#define G_SCREEN_SCALE 4
#define G_SCREEN_MODE 3

#define M_PI 3.141592654

#include "sound.h"
#include "graphics.h"

Graph g;
float t;

int iseed=3453245;

int irand(int& seed)
{
	seed=(seed*1103515245+12345)&0x7ffffff;
	return seed;
}

#define MAXFIELD 2048

float cell=16;


int fieldW=17;
int fieldH=11;

int fieldX=30;
int fieldY=30;

int patha_[MAXFIELD];
int pathhome[MAXFIELD];
#define Path(pa, x, y) pa[((x)+fieldW)%fieldW+(((y)+fieldH)%fieldH)*fieldW]

void Background()
{
	float t=Time()*5;
	for(int i=0;i<G_SCREEN_HEIGHT;i+=32)
	{
		int gr=int(sin(t*.147+i*.011-54)*40+48);
		int gg=int(sin(t*.149-i*.017+13)*40+48);
		int gb=int(sin(t*.141+i*.013-77)*40+48);
		g.Rect(0,i,G_SCREEN_WIDTH,32,0xff000000|(gr<<16)|(gg<<8)|gb);
	}
}

int w;
int h;
bool put=false;

int MonsterBornX=8;
int MonsterBornY=5;

int PacBornX=1;
int PacBornY=1;

char* wallBuf=
"******** ********"
"*               *"
"* * ********* * *"
"* *           * *"
"* * **** **** * *"
"* * *       * * *"
"* * ********* * *"
"* *           * *"
"  * ********* *  "
"*               *"
"******** ********"
;


char* wallBuf0=
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

char objs[MAXFIELD];

struct SWall
{
	float x;
	float y;
	float dx;
	float dy;
};

SWall wallList[MAXFIELD];
int nWalls=0;

#define Wall(x, y) wallBuf[((x)+fieldW)%fieldW+((y)+fieldH)%fieldH*fieldW]

float pacAnger=-1;

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
	bool triggered;
	bool dead;

	static bool MonsterPathStep(GameActor& mon, int* pathBuf, int dir)
	{
		int dist=Path(pathBuf, mon.x, mon.y);
		if(dist>0)
		{
			dist+=dir;
			/* */if(Path(pathBuf, mon.x-1, mon.y)==dist){mon.CommandMove(-1,0);return true;}
			else if(Path(pathBuf, mon.x+1, mon.y)==dist){mon.CommandMove( 1,0);return true;}
			else if(Path(pathBuf, mon.x, mon.y-1)==dist){mon.CommandMove(0,-1);return true;}
			else if(Path(pathBuf, mon.x, mon.y+1)==dist){mon.CommandMove(0, 1);return true;}
		}
		return false;
	}

	static void MonsterMove(GameActor& mon)
	{
		bool dirChosen=false;

		mon.triggered=false;

		if(mon.dead)
		{
			mon.triggered=MonsterPathStep(mon, pathhome, -1);
			if(mon.triggered)
				return;
		}

		if(t-pacAnger<0)
			dirChosen|=MonsterPathStep(mon, patha_,1);
		else
			dirChosen|=MonsterPathStep(mon, patha_,-1);
		mon.triggered=dirChosen;

		if(mon.triggered)
			return;

		while(true)
		{
			int ir=irand(iseed)%4;
			int dirX=0;
			int dirY=0;
			/* */if(ir==0){dirX= 1;}
			else if(ir==1){dirY= 1;}
			else if(ir==2){dirX=-1;}
			else if(ir==3){dirY=-1;}
			if(Wall(mon.x+dirX,mon.y+dirY)==' ')
			{
				mon.CommandMove(dirX,dirY);
				break;
			}
			break;
		}
	}

	void Tick(float idt)
	{
		if(dead)
		{
			if(Path(pathhome,x,y)<2)
			{
				dead=false;
				Born(MonsterBornX,MonsterBornY,1);
			}
		}
		progress+=idt*speed*(dead?2:1);
		if(progress>1)
		{
			progress-=1.;
			//if(Wall(x+dx,y+dy)==' ')
			{
				x+=dx;
				y+=dy;
				x=(x+fieldW)%fieldW;
				y=(y+fieldH)%fieldH;
				if(type==1)
				{
					MonsterMove(*this);
					if(triggered)
					{
						dx=nextdx;
						dy=nextdy;
					}
				}
				if(Wall(x+dx,y+dy)!=' ')	
				{
					dx=0;
					dy=0;
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
		if((dx!=0)&&(nextdx==-dx))
		{
			moveblocked=false;
			dx=nextdx;
			progress=1.-progress;
			x=x-dx;
		}
		if((dy!=0)&&(nextdy==-dy))
		{
			moveblocked=false;
			dy=nextdy;
			progress=1.-progress;
			y=y-dy;
		}

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
		//speed=3.5;
		type=itype;
		dead=false;
	}
};

GameActor pac;
GameActor mon[5];


void DrawPac()
{
	g.clear();
	float a=pac.progress;//(sin(t*M_PI*10)*.5+.5)*M_PI/3;
	float r=cell*2./3.;
	float progress=pac.progress;
	float cx=fieldX+pac.x*cell+cell*pac.dx*pac.progress;
	float cy=fieldY+pac.y*cell+cell*pac.dy*pac.progress;
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
	float x=fieldX+(mon.x+mon.progress*mon.dx)*cell;
	float y=fieldY+(mon.y+mon.progress*mon.dy)*cell;
	float r=cell*2./3.;
	if(!mon.dead)
	{
		int col=mon.col;
		if(t-pacAnger<0)
			col=0xff0000ff;
		int dx=mon.dx;
		int dy=mon.dy;
		g.clear();
		g.M(x-r,y);
		g.a(r,r,0,0,1,r*2,0);
		//g.l(0,r*2);
		/*
		for(int i=0;i<=8;i++)
		{
			float fi=i/8.;
			float tl=sin(fi*M_PI*4)*(.8+cos(Time()*21+fi*9)*.2);
			if(tl<0)tl=-tl;
			g.L(x+r-r*2*fi+r*.3*tl*sin(Time()*9+fi*9),y+r/2+tl*r/2*1.5);
		}
		*/
		float dirx=1;
		float diry=1;
		float ampx=0;
		float ampy=0;
		if(mon.dx)
		{
			diry=dirx=mon.dx*mon.speed/3;
			ampx=1;
			ampy=1;
		}
		else if(mon.dy)
		{
			diry=mon.dy*mon.speed/3;
			dirx=0;
			ampx=0;
			ampy=1;
		}
		for(int i=0;i<=64;i++)
		{
			float fi=i/64.;
			int tn=i/16;
			float tl=fi*4;tl=tl-int(tl);if(tl>.5)tl=1-tl;tl*=2.;tl=sqrt(tl);
			tl*=(.8-cos(Time()*13*diry+tn*15)*.3*ampy);
			if((mon.dx==0)&&(mon.dy==0))tl/=.8;

			g.L(x+r-r*2*fi+ampx*r*.5*tl*tl*sin(Time()*13*dirx+tn*15),y+r/2+tl*r/2*1.8);
		}
		g.close();
		g.rgba32(col);
		g.fin();
		if(t-pacAnger<0)
		{
			g.rgba32(0x400080ff);
			g.fill1();
			g.rgba32(0xff0040ff);
			g.width(2,1);
			g.stroke();
		}
		else
			g.fill1();
	}
	g.clear();
	g.rgba32(0xffffffff);
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


struct CellPos
{
	int x;
	int y;
};
struct CellFront
{
	CellPos f[256];
	int count;
};

CellFront fronts[2];
int chainLen=0;

void BuildPaths(int x0, int y0, int* pa, int l)
{
	for(int i=0;i<1024;i++)
		pa[i]=-1;
	chainLen=0;
	if(Wall(x0,y0)==' ')
	{
		pa[x0+fieldW*y0]=0;
		fronts[0].f[0].x=x0;
		fronts[0].f[0].y=y0;
		fronts[0].count=1;
		int niter=1;
		while(1)
		{
			CellPos* front=&fronts[niter%2].f[0];
			int & frontCount=fronts[niter%2].count;
			CellPos* oldFront=&fronts[1-niter%2].f[0];
			int & oldFrontCount=fronts[1-niter%2].count;
			frontCount=0;
			for(int i=0;i<oldFrontCount;i++)
			{
				int px=oldFront[i].x;
				int py=oldFront[i].y;
				if((Wall(px+1,py)==' ')&&(Path(pa,px+1,py)==-1))
				{
					Path(pa,px+1,py)=niter;
					int fc=frontCount;
					front[fc].x=px+1;
					front[fc].y=py;
					frontCount++;
				}
				if((Wall(px-1,py)==' ')&&(Path(pa,px-1,py)==-1))
				{
					Path(pa,px-1,py)=niter;
					int fc=frontCount;
					front[fc].x=px-1;
					front[fc].y=py;
					frontCount++;
				}
				if((Wall(px,py-1)==' ')&&(Path(pa,px,py-1)==-1))
				{
					Path(pa,px,py-1)=niter;
					int fc=frontCount;
					front[fc].x=px;
					front[fc].y=py-1;
					frontCount++;
				}
				if((Wall(px,py+1)==' ')&&(Path(pa,px,py+1)==-1))
				{
					Path(pa,px,py+1)=niter;
					int fc=frontCount;
					front[fc].x=px;
					front[fc].y=py+1;
					frontCount++;
				}
			}
			if(frontCount==0)
				break;
			//if(niter>l)
			//	break;
			niter++;
		}
	}
}
int cols[5]={
	0xffff0080,
	0xffff8000,
	0xffffc000,
	0xff80ff00,
	0xff0080ff
};

void Restart()
{
	pacAnger=-10;
	pac.Born(PacBornX,PacBornY,0);
	pac.speed=5.5;
	for(int i=0;i<5;i++)
		mon[i].Born(MonsterBornX,MonsterBornY,1);
	for(int i=0;i<5;i++)
	{
		mon[i].speed=.5+.3*i;
		mon[i].col=cols[i];
	}
	for(int i=0;i<fieldH;i++)
	{
		for(int j=0;j<fieldW;j++)
		{
			objs[j+i*fieldW]=0;
			if(wallBuf[j+i*fieldW]==' ')
			{
				if(i<10)objs[j+i*fieldW]=1;
				if(i>20)objs[j+i*fieldW]=1;
			}
			else
			{
				objs[j+i*fieldW]=0;
			}
		}
	}
	for(int i=1;i<fieldH;i+=4)
	{
		for(int j=1;j<fieldW;j+=14)
		{
			if(wallBuf[j+i*fieldW]==' ')
			{
				if(i<10)objs[j+i*fieldW]=2;
				if(i>20)objs[j+i*fieldW]=2;
			}
		}
	}
}

void AddWall(float x, float y, float dx, float dy)
{
	wallList[nWalls].x =fieldX+x*cell;
	wallList[nWalls].y =fieldY+y*cell;
	wallList[nWalls].dx=dx*cell;
	wallList[nWalls].dy=dy*cell;
	nWalls++;
}

void BuildWalls()
{
	nWalls=0;

	for(int i=0;i<fieldH;i++)
	{
		int lx=-10;
		for(int j=0;j<=fieldW;j++)
		{
			if((Wall(j,i)!=' ')&&(j<fieldW))
			{
				if(lx<0)
				{
					lx=j;
				}
			}
			else
			{
				if(lx>=0)
				{
					if(j-lx-1!=0)
					{
						AddWall(lx,i,j-lx-1,0);
					}
					lx=-10;
				}
			}
		}
	}

	for(int j=0;j<fieldW;j++)
	{
		int ly=-10;
		for(int i=0;i<=fieldH;i++)
		{
			if((Wall(j,i)!=' ')&&(i<fieldH))
			{
				if(ly<0)
				{
					ly=i;
				}
			}
			else
			{
				if(ly>=0)
				{
					AddWall(j,ly,0,i-ly-1);
					ly=-10;
				}
			}
		}
	}
}


int main()
{
	Restart();
	pac.col=0xffffe000;
	float prevt=Time();
	BuildPaths(MonsterBornX,MonsterBornY,pathhome,128);
	BuildWalls();

	while(1)
	{
		t=Time();
		g.gray(0);
		g.FillRT();
		//Background();

		if(0)
		for(int i=0;i<fieldH;i++)
		{
			for(int j=0;j<fieldW;j++)
			{
				if(wallBuf[fieldW*i+j]!=' ')
				{
					g.fillrect(fieldX+j*cell-cell/2,fieldY+i*cell-cell/2,cell,cell,0xff008000);
				}
			}
		}

		g.fillrect(fieldX+mx*cell-cell/2,fieldY+my*cell-cell/2,cell,cell,0xff004080);
		g.clear();
		for(int i=0;i<fieldH;i++)
		{
			for(int j=0;j<fieldW;j++)
			{
				if(objs[j+i*fieldW]==1)
				{
					g.M(fieldX+j*cell, fieldY+i*cell);
					g.l(.1,.1);
				}
			}
		}
		g.fin();
		g.width(cell/8.,cell/8.);
		g.rgba32(0xffffffff80);
		g.stroke();
		g.clear();
		for(int i=0;i<fieldH;i++)
		{
			for(int j=0;j<fieldW;j++)
			{
				if(objs[j+i*fieldW]==2)
				{
					g.M(fieldX+j*cell, fieldY+i*cell);
					g.l(.1,.1);
				}
			}
		}
		g.fin();
		g.width(cell/4.,cell/4.);
		g.rgba32(0xffffff8000);
		g.stroke();


		g.clear();

		/*
		for(int i=0;i<nWalls;i++)
		{
 			if(wallList[i].dy!=0)
			{
				g.M(wallList[i].x,wallList[i].y);
				g.l(wallList[i].dx,wallList[i].dy);
			}
		}

		g.fin();
		g.width(4,2);
		g.rgba32(0xffffff00);
		g.stroke();
		*/

		g.clear();

		for(int i=0;i<nWalls;i++)
		{
 			if((wallList[i].dx!=0)||(wallList[i].dy!=0))
			{
				g.M(wallList[i].x,wallList[i].y);
				g.l(wallList[i].dx,wallList[i].dy);
			}
			else
			{
				g.M(wallList[i].x,wallList[i].y);
			}
		}

		g.fin();
		g.width(cell/4.,cell/8.);
		g.rgba32(0xff0080ff);
		g.stroke();
		g.width(cell/8.,cell/8.);
		g.rgba32(0xff000040);
		g.stroke();


		// g.clear();
		// for(int i=0;i<fieldH;i++)
		// {
 		// 	if(ly[i]>=0)
		// 	{
		// 		g.M(i*cell,ly[i]);
		// 		g.l(0,31*cell-ly[i]-cell);
		// 	}			
		// }
		// g.fin();
		// g.width(4,1);
		// g.rgba32(0xff00ff80);
		// g.stroke();


		char s[60];
		snprintf(s,60,"%f",pacAnger-t);
		stext(s,200,10,0xff00ffff);

		for(int i=0;i<5;i++)
			mon[i].Tick(t-prevt);
		///*if(int(t*15+27)%35==0)*/GameActor::MonsterRandomMove(mon0);
		///*if(int(t*16+17)%33==0)*/GameActor::MonsterRandomMove(mon1);
		///*if(int(t*17+57)%36==0)*/GameActor::MonsterRandomMove(mon2);
		//DrawMonster(200,200,50,0x80ff8000,1,0);
		for(int i=0;i<5;i++)
			DrawMonster(mon[i]);

		if(0)
		{
			int phx=mx;
			int phy=my;
			for(int i=0;i<64;i++)
			{
				int a=Path(pathhome,phx,phy);
				if(a>0)
				{
					g.fillrect(((phx+fieldW)%fieldW)*cell,((phy+fieldH)%fieldH)*cell,8,8,0x800080ff);
					/* */if(Path(pathhome,phx-1,phy)==(a-1))phx--;
					else if(Path(pathhome,phx+1,phy)==(a-1))phx++;
					else if(Path(pathhome,phx,phy-1)==(a-1))phy--;
					else if(Path(pathhome,phx,phy+1)==(a-1))phy++;
				}
				else break;
			}
		}

		pac.Tick(t-prevt);

		if(objs[pac.x+pac.y*fieldW]==2)
			pacAnger=t+5;
		objs[pac.x+pac.y*fieldW]=0;
		DrawPac();
		//DrawMonster(pac);

		for(int i=0;i<5;i++)
		{
			float dx=(pac.x+pac.progress*pac.dx)-(mon[i].x+mon[i].progress*mon[i].dx);
			float dy=(pac.y+pac.progress*pac.dy)-(mon[i].y+mon[i].progress*mon[i].dy);
			if(((dx<0.5)&&(dx>-0.5))&&((dy<0.5)&&(dy>-0.5)))
			{
				if(t-pacAnger<0)
				{
					mon[i].dead=true;
				}
				else if(!mon[i].dead)
				{
					Restart();
				}
			}

		}

		BuildPaths(pac.x,pac.y,patha_,60);
		int ipath=int(t*20)%30;
		if(0)
		for(int i=0;i<fieldW*fieldH;i++)
		{
			if(wallBuf[i]==' ')
			{
				snprintf(s,60,"%i",pathhome[i]);
				stext(s,cell*(i%fieldW)+fieldX,cell*(i/fieldW)+fieldY,0x4000ffff);
			}
		}

		Present();


		SScincEvent ev;
		while(GetScincEvent(ev))
		{
			if(ev.type=='MLDN')
			{
				mx=float(ev._1-fieldX)/cell+.5;
				my=float(ev._2-fieldY)/cell+.5;
				if((mx>=0)&&(my>=0)&&(mx<fieldW)&&(my<fieldH))
				{
					if(wallBuf[mx+my*fieldW]==' ')
					{
						wallBuf[mx+my*fieldW]='*';
						put=true;
					}
					else
					{
						wallBuf[mx+my*fieldW]=' ';
						put=false;
					}
					BuildWalls();
					BuildPaths(MonsterBornX,MonsterBornY,pathhome,128);
				}
			}
			if(ev.type=='MMOV')
			{
				mx=float(ev._1-fieldX)/cell+.5;
				my=float(ev._2-fieldY)/cell+.5;
				if(ev._0&1)
				{
					if((mx>=0)&&(my>=0)&&(mx<fieldW)&&(my<fieldH))
					{
						if(put)
							wallBuf[mx+my*fieldW]='*';
						else
							wallBuf[mx+my*fieldW]=' ';
						BuildWalls();
						BuildPaths(MonsterBornX,MonsterBornY,pathhome,128);
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
