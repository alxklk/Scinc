#include "graphics.h"
#define G_SCREEN_MODE 1
#define G_SCREEN_HEIGHT 900
#define G_SCREEN_WIDTH 1600
#define NO_GREEDY_EVENT
#include "../ws.h"
#include "../ui/menu/menu.h"
#include "CatmullRom.h"

#define M_PI 3.14159265358979323846

Graph g;

#define MAX_N 48

flt2 xys[MAX_N];

int np=3;

class Sel
{
public:
	int smap[MAX_N];
	int sp[MAX_N];
	int ncp;
	void UnSelectAll()
	{
		for(int i=0;i<MAX_N;i++)
		{
			smap[i]=0;
		}
		ncp=0;
	}
	void Select(int n)
	{
		UnSelectAll();
		if(n>=0)
		{
			smap[n]=1;
			ncp=1;
		}
	}
	void List()
	{
		int j=0;
		for(int i=0;i<MAX_N;i++)
		{
			if(smap[i])sp[j++]=i;
		}
		ncp=j;
		printf("selected %i nodes\n", ncp);
	}
	void SelectAdd(int n)
	{
		if(!smap[n])
		{
			smap[n]=1;
			ncp++;
		}
	}
};

Sel sel;

void InsertDot(int segment, float t)
{
	if(np>=MAX_N-1)return;
	if(segment<np)
	{
		printf("Insert @%i:%f\n", segment, t);
		for(int i=np+1;i>segment;i--)
		{
			xys[i]=xys[i-1];
		}
		xys[segment]=xys[segment]*(1.0-t);
		xys[segment+1]=xys[segment+1]*t;
		np++;
	}
}

int showHelpers=1;

float Abs(float x)
{
	if(x<0.)
		return -x;
	return x;
}

void FindClosest(SScincEvent& ev, int& hp, int& lhp, float& lht)
{
	hp=-1;
	lhp=-1;
	flt2 prevPos={};
	flt2 pos={};
	for(int i=0;i<np;i++)
	{
		prevPos=pos;
		pos=xys[i];
		flt2 delta=pos-prevPos;
		if((Abs(pos.x-ev.x)<8)&&(Abs(pos.y-ev.y)<5))
		{
			hp=i;
			break;
		}
		else if(i)
		{
			float t=vdot(delta,flt2::New(pos.x-ev.x,pos.y-ev.y))/delta.lengthSq();
			if(t>0.0&&t<1.0)
			{
				float n=vdot(delta.normalized().perp(),flt2::New(pos.x-ev.x,pos.y-ev.y));
				if(Abs(n)<5)
				{
					lhp=i;
					lht=t;
				}
			}
		}
	}
}

void Reset()
{
	np=12;
	float dx=float(G_SCREEN_WIDTH)/(np+1);
	for(int i=0;i<np;i++)
	{
		xys[i]=flt2::New(dx+dx*i,G_SCREEN_HEIGHT/2.);
	}
	xys[np-1].y+=0.0005;
	xys[1].y+=dx;
	xys[2].y-=dx/2;
	
}

void DrawCR0()
{
	flt2 b0;
	flt2 b1;
	flt2 b2;
	flt2 b3;
	for(int i=0;i<np-1;i++)
	{
		if(i==0)
		{
			g.M(xys[0].x,xys[0].y);
			CR2Bez(xys[i],xys[i],xys[i+1],xys[i+2], b0, b1, b2, b3);
		}
		else if(i==np-2)
		{
			CR2Bez(xys[i-1],xys[i],xys[i+1],xys[i+1], b0, b1, b2, b3);
		}
		else
		{
			CR2Bez(xys[i-1],xys[i],xys[i+1],xys[i+2], b0, b1, b2, b3);
		}
		g.C(b1.x, b1.y, b2.x, b2.y, b3.x, b3.y);
	}
}

void DrawQ(flt2* p, int cnt)
{
	flt2 b1;
	flt2 b2;
	g.M(p[0].x,p[0].y);
	flt2 d=(p[1]-p[0]);
	//b1=p[0]+d*.25;
	b2=p[0]+d*.5f;
	//g.Q(b1.x, b1.y, b2.x, b2.y);
	g.L(b2.x, b2.y);
	for(int i=1;i<cnt-1;i++)
	{
		b1=p[i];
		b2=(p[i+1]+p[i])*.5f;
		g.Q(b1.x, b1.y, b2.x, b2.y);
	}
	b2=p[cnt-1];
	g.L(b2.x, b2.y);
}

void DrawA(flt2* p, int cnt)
{
	g.M(p[0].x,p[0].y);
	for(int i=1;i<cnt-1;i++)
	{
		flt2 dp0=p[i-1]-p[i];
		flt2 dp1=p[i+1]-p[i];
		float dpl0=dp0.length()/2.;
		float dpl1=dp1.length()/2.;
		dp0.normalize();
		dp1.normalize();
		float l=dpl0;
		if(dpl1<l)l=dpl1;
		flt2 p0=p[i]+dp0*l;
		flt2 p1=p[i]+dp1*l;
		float a=vdot(dp0,dp1);
		g.L(p0.x,p0.y);
		//a=sqrt(1.-a*a);
		float ca=sqrt((1+a)/2.);
		if(ca!=0)
		{
			float sa=sqrt((1-a)/2.);
			float ta=sa/ca;
			int sweep=0;
			if(vcross(dp0,dp1)<0)sweep=1;
			g.A(l*ta,l*ta,0,0,sweep,p1.x,p1.y);
		}
		else
		{
			g.L(p1.x,p1.y);
		}
	}
	g.L(p[cnt-1].x,p[cnt-1].y);
}

void DrawB(flt2* p, int cnt)
{
	g.M(p[0].x,p[0].y);
	for(int i=1;i<cnt-1;i++)
	{
		flt2 dp0=p[i-1]-p[i];
		flt2 dp1=p[i+1]-p[i];
		float dpl0=dp0.length()/3.;
		float dpl1=dp1.length()/3.;
		dp0.normalize();
		dp1.normalize();
		float l=dpl0;
		if(dpl1<l)l=dpl1;
		flt2 p0=p[i]+dp0*l;
		flt2 p1=p[i]+dp1*l;
		float a=vdot(dp0,dp1);
		g.L(p0.x,p0.y);
		g.L(p1.x,p1.y);
	}
	g.L(p[cnt-1].x,p[cnt-1].y);
}

void DrawCR(flt2* p, int cnt)
{
	flt2 b0;
	flt2 b1;
	flt2 b2;
	flt2 b3;
	CR2Bez(p[0], p[0], p[1], p[2], b0, b1, b2, b3);
	g.C(b1.x, b1.y, b2.x, b2.y, b3.x, b3.y);
	for(int i=1;i<cnt-2;i++)
	{
		CR2Bez(p[i-1],p[i],p[i+1],p[i+2], b0, b1, b2, b3);
		g.C(b1.x, b1.y, b2.x, b2.y, b3.x, b3.y);
	}
	CR2Bez(p[cnt-3],p[cnt-2],p[cnt-1],p[cnt-1], b0, b1, b2, b3);
	g.C(b1.x, b1.y, b2.x, b2.y, b3.x, b3.y);
}

int method=0;

int main()
{
	SMenu menu;
	menu.Init();
	menu.cmdHandler=0;


	menu.Create()
	.P("=")
		.M("About","about")
		.M("Exit","exit").C([](SMenuItem&i)->int{exit(0);return 0;})
	.P("Method")
		.M("Arcs","").C([](SMenuItem&i)->int{method=0;return 0;})
		.M("Q Bez","").C([](SMenuItem&i)->int{method=1;return 0;})
		.M("Cat-Rom","").C([](SMenuItem&i)->int{method=2;return 0;})
		.M("Cat-Rom 0","").C([](SMenuItem&i)->int{method=3;return 0;})
		.M("Bevel","").C([](SMenuItem&i)->int{method=4;return 0;})
	.P("ALpha")
		.M("0","").C([](SMenuItem&i)->int{alpha=0;return 0;})
		.M("0.5","").C([](SMenuItem&i)->int{alpha=0.5;return 0;})
		.M("1.0","").C([](SMenuItem&i)->int{alpha=1.0;return 0;})
	.P("Help")
		.M("Context","help_context")
		.M("Index","help_index")
	;

	sel.UnSelectAll();
	int hp=-1;
	int lhp=-1;
	float lht=-1;
	int mx;
	int my;
	Reset();

	while(true)
	{
		SScincEvent ev;
		WaitForScincEvent(.015);
		while(GetScincEvent(ev))
		{
			sel.List();
			if(menu.MenuHandleEvent(0,0,G_SCREEN_WIDTH,20,ev))
			{
				continue;
			}
			if(ev.type=='KBDN')
			{
				/**/ if(ev.x==KEYCODE_TAB)showHelpers=!showHelpers;
			}
			if(((ev.type&0xff000000)>>24)=='M')
			{
				//if(ev.z)printf("z %i\n", ev.z);
				FindClosest(ev,hp,lhp,lht);
				if(ev.type=='MWUP')
				{
				}
				if(ev.type=='MWDN')
				{
				}
				if(ev.type=='MMOV')
				{
					if((sel.ncp==0)&&(hp!=-1))
					{
						sel.Select(hp);
					}
					if(ev.z&2)
					{
						float dx=(ev.x-mx);
						float dy=(ev.y-my);
					}

					if(ev.z&1)
					{
						float dx=(ev.x-mx);
						float dy=(ev.y-my);
						for(int i=0;i<np;i++)
						{
							if(sel.smap[i])
							{
								xys[i].x+=dx;
								xys[i].y+=dy;
							}
						}
					}
				}
				if(ev.type=='MLDN')
				{
					if((ev.z&8)&&(lhp>=0))
					{
						InsertDot(lhp,lht);
						sel.Select(lhp);
					}
					else
					{
						if(hp==-1)
						{
							sel.UnSelectAll();
						}
						else if(!sel.smap[hp])
						{
							if(ev.z&32)
								sel.SelectAdd(hp);
							else
								sel.Select(hp);
						}
					}
				}
				if(ev.type=='MRDN')
				{
				}
				mx=ev.x;
				my=ev.y;
			}
		}

		g.rgba32(0xff000000);
		g.FillRT();

		if(showHelpers)
		{
			g.clear();
			g.M(xys[0].x,xys[0].y);
			for(int i=0;i<np;i++)g.L(xys[i].x,xys[i].y);
			g.fin();
			g.rgba32(0x80ffffff);
			g.width(1,1);
			g.stroke();
		}

		g.clear();
		g.M(100,100);
		g.Q(100,200,200,200);
		g.M(xys[0].x,xys[0].y);
		switch(method)
		{
			case 0: DrawA (xys,np);break;
			case 1: DrawQ (xys,np);break;
			case 2: DrawCR(xys,np);break;
			case 3: DrawCR0();     break;
			case 4: DrawB (xys,np);break;
		}
		g.fin();
		g.rgba32(0x8000ff00);
		g.width(3,3);
		g.stroke();

		//g.clear();
		//g.rgba32(0xffff00ff);
		//DrawCR(1);
		//g.fin();
		//g.width(2,2);
		//g.stroke();
		//g.width(1,1);

		if(showHelpers)
		{
			flt2 prevPos={};
			flt2 pos={};
			for(int i=0;i<np;i++)
			{
				prevPos=pos;
				pos=xys[i];
				g.rgba32(0x4000ff00);
				if(i)g.hairline(pos.x,pos.y,prevPos.x,prevPos.y);

				if(i==hp)g.Circle(pos.x, pos.y,0,7,1,0x60ffff00);
				if(i==lhp)
				{
					flt2 delta=pos-prevPos;
					g.rgba32(0xffffff00);
					g.hairline(pos.x,pos.y,prevPos.x,prevPos.y);
					flt2 hov=prevPos+delta*(1.0-lht);
					g.rgba32(0xffff0000);
					g.hairline(hov.x-5,hov.y-5,hov.x+5,hov.y+5);
					g.hairline(hov.x+5,hov.y-5,hov.x-5,hov.y+5);
				}
				if(sel.smap[i])
					g.Circle(pos.x, pos.y,0,3,1,0xff0080ff);
				else
					g.Circle(pos.x, pos.y,0,3,1,0xffff0000);
			}
		}
		menu.MenuDraw(g,0,0,G_SCREEN_WIDTH,20);
		Present();
	}
	return 0;
}