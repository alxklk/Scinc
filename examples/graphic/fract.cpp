#ifdef NOTDEF
Scinc "$0" "$*"
exit
#endif


#include "graphics.h"
#define G_SCREEN_MODE 1
#define G_SCREEN_HEIGHT 900
#define G_SCREEN_WIDTH 1500
#define G_SCREEN_SCALE 2
#define NO_GREEDY_EVENT
#include "../ws.h"
#include "flt2.h"
#include "../algorithm/Token.h"
#include "../algorithm/Lex.h"
#include "../ui/menu/menu.h"
#include "../ui/gui/GUI.h"
#include "CatmullRom.h"
#include "../include/strn.h"

#define M_PI 3.14159265358979323846

Graph g;

#define MAX_N 64

flt2 editxys[MAX_N];
int flags[MAX_N];

flt2 normxys[MAX_N];

flt2* render;
int nRender;

bool snap=true;
int domain=0;

struct SGridLine
{
	int col;
	int flags;
	flt2 pd;
};

SGridLine grid[4]={
	{0x2f00ffff,1,{5,0}},
	{0x2fff00ff,1,{0,10}},
	{0x3f808080,0,{10,10}},
	{0x3f808080,0,{10,-10}}
	};


#define FLAG_NODE 1
#define FLAG_SEGMENT 2
#define FLAG_START 4
#define FLAG_END 8
#define FLAG_NEGX 16
#define FLAG_NEGY 32

int nep=3;
int nnp;

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

float scale=10;
flt2 offset={100,00};

void InsertDot(int segment, float t)
{
	if(nep>=MAX_N-1)return;
	if(segment<nep-1)
	{
		printf("Insert @%i:%f\n", segment, t);
		flt2 d=(editxys[segment+1]-editxys[segment]);
		for(int i=nep+1;i>segment;i--)
		{
			editxys[i]=editxys[i-1];
			flags[i]=flags[i-1];
		}
		printf("d=%f %f\n", d.x,d.y);
		editxys[segment+1]=editxys[segment]+d*t;
		nep++;
	}
}

int drawCount;

flt2 globalP;

void Rec0(flt2 d, float signY, float signX, float depth, int flag)
{
	if(depth<=0)
	{
		globalP+=d;
		render[nRender++]=globalP;
		drawCount++;
	}
	else
	{
		flt2 dp=d.perp()*signY;
		if(depth<1)dp*=depth;
		if(flag&FLAG_NEGY){dp=-dp;signY=-signY;}
		if(flag&FLAG_NEGX){signX=-signX;}

		flt2 aux;
		if(depth<1.)
		{
			aux=d*(1.0-depth)*.5;
			globalP+=aux;
			render[nRender++]=globalP;
			drawCount++;
			d=d*depth;
		}

		if(signX<0)
		{
			for(int i0=0;i0<nnp;i0++)
			{
				int i=nnp-1-i0;
				Rec0(d*normxys[i].x+dp*normxys[i].y, signY, signX, depth-1, flags[i]);
			}
		}
		else
		{
			for(int i=0;i<nnp;i++)
			{
				Rec0(d*normxys[i].x-dp*normxys[i].y, signY, signX, depth-1, flags[i]);
			}
		}

		if(depth<1)
		{
			globalP+=aux;
			render[nRender++]=globalP;
			drawCount++;
		}

	}
}

void Rec1(flt2 d, float signY, float signX, float depth, int flag)
{
	if(depth<=0)
	{
		globalP+=d;
		render[nRender++]=globalP;
		drawCount++;
	}
	else
	{
		flt2 dp=d.perp()*signY;
		float fd=0.;
		if(depth<1)
		{
			fd=depth;
			if(fd<0)fd=0;
			dp*=fd;
		}
		if(flag&FLAG_NEGY){dp=-dp;signY=-signY;}
		if(flag&FLAG_NEGX){signX=-signX;}
		flt2 aux;
		if(depth<1)
		{
			aux=d*(1.0-fd)*.5;
			globalP+=aux;
			render[nRender++]=globalP;
			drawCount++;
			d=d*fd;
		}
		if(signX<0)
		{
			for(int i0=0;i0<nnp;i0++)
			{
				int i=nnp-1-i0;
				if(flags[i]&FLAG_NODE)
					Rec1(d*normxys[i].x+dp*normxys[i].y, signY, signX, depth-1, flags[i]);
				else
					Rec1(d*normxys[i].x+dp*normxys[i].y, signY, signX, 0, flags[i]);
			}
		}
		else
		{
			for(int i=0;i<nnp;i++)
			{
				if(flags[i]&FLAG_NODE)
					Rec1(d*normxys[i].x-dp*normxys[i].y, signY, signX, depth-1, flags[i]);
				else
					Rec1(d*normxys[i].x-dp*normxys[i].y, signY, signX, 0, flags[i]);
			}
		}
		if(depth<1)
		{
			globalP+=aux;
			render[nRender++]=globalP;
			drawCount++;
		}
	}
}



char* svgtemplate0=
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"
"<svg viewBox=\"0 0 1000 1000\" xmlns=\"http://www.w3.org/2000/svg\"\n"
"xmlns:svg=\"http://www.w3.org/2000/svg\">\n"
"<!--\n";
char* svgtemplate1=
"-->\n<g><path style=\"fill:black\" d=\"";
char* svgtemplate2="\"/></g></svg>\n";

int method=0;

struct SVGWriter
{
	FILE* f;
	void M(float x, float y)
	{
		char s[128];
		snprintf(s, 128, " M %f %f ", x, y);
		fputs(s, f);
	}
	void L(float x, float y)
	{
		char s[128];
		snprintf(s, 128, " L %f %f ", x, y);
		fputs(s, f);
	}
	void A(float rx, float ry, float angle, int large, int sweep, float x, float y)
	{
		char s[256];
		snprintf(s, 128, " A %f %f %f %i %i %f %f ", rx, ry, angle, large, sweep, x, y);
		fputs(s, f);
	}
};

void SVGA(SVGWriter& g, flt2* p, int cnt)
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
		g.L(p0.x,p0.y);

		float a=vdot(dp0,dp1);
		float ca=sqrt((1+a)/2.);
		float sa=sqrt((1-a)/2.);
		float ta=sa/ca;
		if((ca==0)||(ta>10000))
		{
			g.L(p1.x,p1.y);
		}
		else
		{
			g.A(l*ta,l*ta,0,0,(vcross(dp0,dp1)<0)?1:0,p1.x,p1.y);
		}
	}
	g.L(p[cnt-1].x,p[cnt-1].y);
}

void SVG()
{
	FILE*f=fopen("fract_out.svg","wb");
	fputs(svgtemplate0,f);
	char s[512];
	fputs("Number\n",f);
	snprintf(s,512,"%i\n",nep);fputs(s,f);
	fputs("Nodes\n",f);
	for(int i=0;i<nep;i++)
	{
		snprintf(s,512,"%f %f %i\n",editxys[i].x,editxys[i].y,flags[i]);fputs(s,f);
	}
	fputs(svgtemplate1,f);
	//snprintf(s,512,"M %f %f ", editxys[0].x, editxys[0].y);fputs(s,f);
	//snprintf(s,512,"l %f %f ", 0.,-500.);fputs(s,f);

	{
		drawCount=0;
		nRender=0;
		nnp=nep-1;
		flt2 delta=editxys[nep-1]-editxys[0];
		flt2 deltax=delta;
		flt2 deltay=delta.perp();
		for(int i=1;i<nep;i++)
		{
			normxys[i-1].x=vdot(deltax,editxys[i]-editxys[i-1])/delta.lengthSq();
			normxys[i-1].y=-vdot(deltay,editxys[i]-editxys[i-1])/delta.lengthSq();
		}
		delta=delta*scale;
		globalP=editxys[0]*scale+offset;
		render[0]=globalP;nRender++;
		switch(method)
		{
			case 0:
			{
				Rec0(delta,1.,1.,8, 0);
			}
			break;
			case 1:
			{
				Rec1(delta,1.,1.,8,0);
			}
			break;
		}
	}
	SVGWriter w={f};
	SVGA(w, render, nRender);
	//snprintf(s,512," %f %f z", 0., 500.);fputs(s,f);
	fputs(" z",f);
	fputs(svgtemplate2,f);
	fclose(f);
}

void Rec2(flt2 d, int depth)
{
	if(depth==0)
	{
		flt2 dp=d.perp();
		for(int i=0;i<nnp;i++)
		{
			flt2 di=d*normxys[i].x-d.perp()*normxys[i].y;
			g.l(di.x,di.y);
			drawCount++;
		}
	}
	else
	{
		flt2 p=flt2::New(0,0);
		flt2 dp=d.perp();
		for(int i=0;i<nnp;i++)
		{
			p+=normxys[i];
			if(flags[i])
			{
				Rec2(d*p.x-d.perp()*p.y,depth-1);
				p=flt2::New(0,0);
			}
		}
	}
}

float Abs(float x)
{
	if(x<0.)
		return -x;
	return x;
}

int showHelpers=1;
int curves=0;

void FindClosest(SScincEvent& ev, int& hp, int& lhp, float& lht)
{
	hp=-1;
	lhp=-1;
	for(int i=0;i<nep;i++)
	{
		flt2 pos=editxys[i]*scale+offset;
		if((Abs(pos.x-ev.x)<5)&&(Abs(pos.y-ev.y)<5))
		{
			hp=i;
			break;
		}
		else if(i)
		{
			flt2 pi=(editxys[i-1]*scale+offset);
			flt2 txl=pi-pos;
			float t=vdot(txl,flt2::New(pi.x-ev.x,pi.y-ev.y))/txl.lengthSq();
			if(t>0.0&&t<1.0)
			{
				float n=vdot(txl.normalized().perp(),flt2::New(pos.x-ev.x,pos.y-ev.y));
				//printf("%i: %f %f\n", i, t, n);
				if(Abs(n)<5)
				{
					lhp=i-1;
					lht=t;
					break;
				}
			}
		}
	}
}

void SaveFile()
{
	FILE* f=fopen("frac_0000.txt","wb");
	char s[512];
	snprintf(s,512,"%i\n",nep);fputs(s,f);
	for(int i=0;i<nep;i++)
	{
		snprintf(s,512,"%f %f %i\n",editxys[i].x,editxys[i].y,flags[i]);fputs(s,f);
	}
	fclose(f);
}

char filename[512]="frac_0000.txt";

void LoadFile()
{
	FILE* f=fopen(filename,"rb");
	char s[512];
	fgets(s,512,f);
	SToken toks[32];
	int ntok=MakeTokens(toks,s);
	if(ntok==1)
	{
		nep=toks[0].GetNumber();
	}
	for(int i=0;i<nep;i++)
	{
		fgets(s,512,f);
		int ntok=MakeTokens(toks,s);
		if(ntok>=3)
		{
			float numbers[3]={};
			int curNum=0;
			int curSign=1;
			// Lexer accepts negative numbers as the '-' operator and signless number. 
			// Need to consolidate them into a negative number. 
			for(int j=0;j<ntok;j++)
			{
				if(toks[j].IsOperator('-'))
				{
					curSign=-curSign;
				};
				if(toks[j].IsNumber())
				{
					numbers[curNum]=curSign*toks[j].GetNumber();
					curSign=1;
					curNum++;
				};

			}
			// If curNum!=3 report error
			editxys[i].x=numbers[0];
			editxys[i].y=numbers[1];
			flags[i]=numbers[2];
		}
	}
	fclose(f);
}

int depth=2;

void Reset()
{
	depth=2;
	nep=3;
	for(int i=0;i<nep;i++)
	{
		editxys[i]=flt2::New(10+i*50./(nep-1.),50);
		flags[i]=0;
	}
	editxys[nep-1].y+=0.0005;
	method=0;
}

void Koch()
{
	depth=4;
	nep=5;
	editxys[0]=flt2::New(0,0);
	editxys[1]=flt2::New(10.,0);
	editxys[2]=flt2::New(15.,-sqrt(10*10-5*5));
	editxys[3]=flt2::New(20.,0);
	editxys[4]=flt2::New(30.,0);
	flags[0]=0;
	flags[1]=0;
	flags[2]=0;
	flags[3]=0;
}

void Dragon()
{
	depth=1;
	nep=3;
	editxys[0]=flt2::New(0,0);
	editxys[1]=flt2::New(10,0);
	editxys[2]=flt2::New(10,-10);
	flags[0]=0;
	flags[1]=FLAG_NEGX|FLAG_NEGY;
}

void Triangle()
{
	depth=1;
	nep=3;
	editxys[0]=flt2::New(0,0);
	editxys[1]=flt2::New(10,0);
	editxys[2]=flt2::New(10,-10);
	flags[0]=FLAG_NEGY;
	flags[1]=FLAG_NEGX;
}

int style=0;

void DrawCR(flt2* p, int cnt)
{
	flt2 b0;
	flt2 b1;
	flt2 b2;
	flt2 b3;
	g.M(p[0].x,p[0].y);
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

void DrawLR(flt2* p, int cnt)
{
	g.M(p[0].x,p[0].y);
	for(int i=1;i<cnt;i++)
	{
		g.L(p[i].x, p[i].y);
	}
}

void DrawQ(flt2* p, int cnt)
{
	flt2 b1;
	flt2 b2;
	g.M(p[0].x,p[0].y);
	flt2 d=(p[1]-p[0]);
	b2=p[0]+d*.5f;
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
		g.L(p0.x,p0.y);

		float a=vdot(dp0,dp1);
		float ca=sqrt((1+a)/2.);
		float sa=sqrt((1-a)/2.);
		float ta=sa/ca;
		if((ca==0)||(ta>10000))
		{
			g.L(p1.x,p1.y);
		}
		else
		{
			g.A(l*ta,l*ta,0,0,(vcross(dp0,dp1)<0)?1:0,p1.x,p1.y);
		}
	}
	g.L(p[cnt-1].x,p[cnt-1].y);
}

void HelperGrid()
{
	for(int s=0;s<4;s++)
	{
		flt2 gp0=offset-grid[s].pd.perp()*scale*100.;
		flt2 gp1=offset+grid[s].pd.perp()*scale*100.;
		g.rgba32(grid[s].col);
		flt2 step=grid[s].pd*scale;
		for(int i=-10;i<11;i++)
		{
			flt2 gp0i=gp0-step*i;
			flt2 gp1i=gp1-step*i;
			g.hairline(gp0i.x, gp0i.y, gp1i.x, gp1i.y);
		}
	}
}

typedef char* charptr;

int main(int argc, charptr* argv)
{
	int optWin=wsys.CreateWindow(640,480,G_SCREEN_SCALE,G_SCREEN_SCALE,G_SCREEN_MODE);
#ifdef __SCINC_HOTRELOAD__

	{
		int x=GetHostInt("optWinPosX", -200000);
		int y=GetHostInt("optWinPosY", -200000);
		printf("Persistent pos %i %i\n", x, y);
		if((x>-200000)&&(y>-200000))
		{
			printf("opt windows pos %i %i\n", x, y);
			wsys.SetWindowPos(optWin,x,y);
		}
	}
#endif


	CGUI gui;
	gui.Init();
	int ctldy=20;
	int ctly=15;
	gui.AddFrame("",            5,5,105,135);
	gui.AddStatic("Render",     5,10,105,15);
	gui.AddCheck("Grid",        10,ctly+=ctldy,65,15,0,&snap);
	gui.AddSelect("Gaussian"  , 10,ctly+=ctldy,65,15,0,0,&domain).SetActionCB(
		[](SWidget* p)->int
		{
			grid[0].pd.Set(5,0);
			grid[1].pd.Set(0,10);
			grid[2].pd.Set(10,10);
			grid[3].pd.Set(10,-10);
		});
	gui.AddSelect("Eisenstein", 10,ctly+=ctldy,65,15,0,1,&domain).SetActionCB(
		[](SWidget* p)->int
		{
			float h=sqrt(10*10-5*5);
			grid[0].pd.Set(5,0);
			grid[1].pd.Set(0,h);
			grid[2].pd.Set( h,5);grid[2].pd.normalize();grid[2].pd=grid[2].pd*h;
			grid[3].pd.Set(-h,5);grid[3].pd.normalize();grid[3].pd=grid[3].pd*h;
		});
	gui.AddSelect("Line",       10,ctly+=ctldy,65,15,0,0,&curves);
	gui.AddSelect("Quadratic",  10,ctly+=ctldy,85,15,0,1,&curves);
	gui.AddSelect("Catmul-Rom", 10,ctly+=ctldy,85,15,0,2,&curves);
	gui.AddSelect("Circular",   10,ctly+=ctldy,85,15,0,3,&curves);
	gui.AddSelect("Bevel 1/3",  10,ctly+=ctldy,85,15,0,4,&curves);
	float fdepth=depth;
	gui.AddSlide("Depth",   10,ctly+=ctldy+10,200,15,0,&fdepth,0,0,10).SetActionCB(
		[](SWidget* p)->int
		{
			depth=*p->fDest;
		});
	gui.AddSlide("C-R alpha",   10,ctly+=ctldy+10,200,15,0,&alpha,0,0,1).SetActionCB(
		[](SWidget* p)->int
		{
			depth=*p->fDest;
		});

	alpha=0.5;
	render=(flt2*)malloc(sizeof(flt2)*1024*1024*8);
	SMenu menu;
	menu.Init();
	menu.cmdHandler=0;

	menu.Create()
	.P("=")
		.M("About","about")
		.M("Exit","exit").C([](SMenuItem&i)->int{exit(0);return 0;})
	.P("File")
		.M("Load ","file_load").C([](SMenuItem&i)->int{LoadFile();return 0;})
		.M("Save","file_save").C([](SMenuItem&i)->int{SaveFile();return 0;})
		.M("Export SVG","file_export").C([](SMenuItem&i)->int{SVG();return 0;})
	.P("Settings")
		.M("Snap to grid","set_snap").C([](SMenuItem&i)->int{snap=!snap;return 0;})
	.P("Operations")
		.M("Reset","oper_reset").C([](SMenuItem&i)->int{Reset();return 0;})
	.P("Examples")
		.M("Koch curve","ex_koch").C([](SMenuItem&i)->int{Koch();return 0;})
		.M("Dragon curve","ex_dragon").C([](SMenuItem&i)->int{Dragon();return 0;})
		.M("Triangle curve","ex_dragon").C([](SMenuItem&i)->int{Triangle();return 0;})
	.P("View")
		.M("Toggle helpers","view_helpers").C([](SMenuItem&i)->int{showHelpers=!showHelpers;return 0;})
		.M("Next stype","view_style").C([](SMenuItem&i)->int{style++;return 0;})
	.P("Render")
		.M("Linear segments",   "").C([](SMenuItem&i)->int{curves=0;return 0;})
		.M("Quadratic",         "").C([](SMenuItem&i)->int{curves=1;return 0;})
		.M("Catmull-Rom t=0",   "").C([](SMenuItem&i)->int{curves=2;alpha=0.0;return 0;})
		.M("Catmull-Rom t=0.5", "").C([](SMenuItem&i)->int{curves=2;alpha=0.5;return 0;})
		.M("Catmull-Rom t=1.0", "").C([](SMenuItem&i)->int{curves=2;alpha=1.0;return 0;})
		.M("Circular arc", "").C([](SMenuItem&i)->int{curves=3;return 0;})
		.M("Bevel 1/3", "").C([](SMenuItem&i)->int{curves=4;return 0;})
	.P("Method")
		.M("All recursive","methot_all").C([](SMenuItem&i)->int{method=0;return 0;})
		.M("Some recursive","method_some").C([](SMenuItem&i)->int{method=1;return 0;})
		.M("Recursive segments","method_segments").C([](SMenuItem&i)->int{method=2;return 0;})
	.P("Help")
		.M("Context","help_context")
		.M("Index","help_index")
	;

	sel.UnSelectAll();
	//sel.SelectAdd(3);
	//sel.SelectAdd(4);
	//sel.SelectAdd(5);
	int hp=-1;
	int lhp=-1;
	float lht=-1;
	int mx;
	int my;
	int dragStart=false;
	Reset();
	if(argc>1)
	{
		strncp(filename, argv[1], 512);
		printf("Loading data from file %s\n", filename);
		LoadFile();
	}

	while(true)
	{
		SScincEvent ev;
		WaitForScincEvent(.015);
		while(GetScincEvent(ev))
		{
			if(ev.w==optWin)
			{
				if(ev.type=='WMOV')
				{
#ifdef __SCINC_HOTRELOAD__
					SetHostInt("optWinPosX", ev.x);
					SetHostInt("optWinPosY", ev.y);
#endif
				}
				else
				{
					gui.Event(ev);
				}
				continue;
			}
			sel.List();
			if(ev.type=='MLUP')dragStart=false;
			if(menu.MenuHandleEvent(0,0,G_SCREEN_WIDTH,20,ev))
			{
				continue;
			}
			if(ev.type=='KBDN')
			{
				//printf("Key %i %i %i %i\n", ev.type, ev.x, ev.y, ev.z);
				if(ev.x==KEYCODE_GR_SUB){depth--;fdepth=int(fdepth-1);}
				else if(ev.x==KEYCODE_GR_ADD){depth++;fdepth=int(fdepth+1.01);}
				else if(ev.x=='0')method=0;
				else if(ev.x=='1')method=1;
				else if(ev.x=='2')method=2;
				else if(ev.x==KEYCODE_TAB)showHelpers=!showHelpers;
				else if(ev.x==KEYCODE_F2)style++;
				else if(ev.x==KEYCODE_V)
				{
					SVG();
				}
				else if(ev.x==KEYCODE_S)
				{
					//SaveFile();
				}
				else if(ev.x==KEYCODE_R)
				{
					LoadFile();
				}
				else if(ev.x==KEYCODE_X)
				{
					sel.List();
					for(int i=0;i<sel.ncp;i++)
					{
						if(sel.sp[i]>0)
						flags[sel.sp[i]-1]=flags[sel.sp[i]-1]^FLAG_NEGX;
					}
				}
				else if(ev.x==KEYCODE_Y)
				{
					sel.List();
					for(int i=0;i<sel.ncp;i++)
					{
						if(sel.sp[i]>0)
						flags[sel.sp[i]-1]=flags[sel.sp[i]-1]^FLAG_NEGY;
					}
				}
				else if(ev.x==KEYCODE_B)
				{
					// segment begin
				}
				else if(ev.x==KEYCODE_E)
				{
					// segment end
				}

				if(depth<0)depth=0;
				if(depth>18)depth=18;
			}
			if(((ev.type&0xff000000)>>24)=='M')
			{
				//if(ev.z)printf("z %i\n", ev.z);
				FindClosest(ev,hp,lhp,lht);
				if((ev.type=='MWUP')||ev.type=='MWDN')
				{
					float mu=.95;
					if(ev.type=='MWUP')mu=1./mu;
					float oldScale=scale;
					flt2 delta=-(flt2::New(ev.x,ev.y)-offset)/scale;
					scale*=mu;
					delta+=(flt2::New(ev.x,ev.y)-offset)/scale;
					offset+=delta*scale;
				}
				if(ev.type=='MMOV')
				{
					//if((sel.ncp==0)&&(hp!=-1))
					//{
					//	sel.Select(hp);
					//}

					if(ev.z&2)
					{
						float dx=(ev.x-mx);
						float dy=(ev.y-my);
						offset.x+=dx;
						offset.y+=dy;
					}

					if(dragStart&&(ev.z&1))
					{
						float dx=(ev.x-mx)/scale;
						float dy=(ev.y-my)/scale;
						if(sel.ncp==1)
						{
							float x=(mx-offset.x)/scale;
							float y=(my-offset.y)/scale;
							flt2 p={x,y};

							if(snap)
							{
								flt2 gp;
								gp.x=vdot(p, grid[0].pd)/grid[0].pd.lengthSq();
								gp.y=vdot(p, grid[1].pd)/grid[1].pd.lengthSq();
								if((Abs(gp.x-int(gp.x+.5))<.3)&&(Abs(gp.y-int(gp.y+.5))<.3))
								{
									gp.y=int(gp.y+.5);
									gp.x=int(gp.x+.5);
								}
								editxys[sel.sp[0]]=grid[0].pd*gp.x+grid[1].pd*gp.y;
							}
							else
							{
								editxys[sel.sp[0]].x=p.x;
								editxys[sel.sp[0]].y=p.y;
							}
						}
						else
						{
							for(int i=0;i<nep;i++)
							{
								if(sel.smap[i])
								{
									editxys[i].x+=dx;
									editxys[i].y+=dy;
								}
							}
						}
					}
				}
				if(ev.type=='MLDN')
				{
					if(hp!=-1)dragStart=true;
					if((ev.z&8)&&(lhp>=0))
					{
						InsertDot(lhp,lht);
						sel.Select(lhp+1);
						dragStart=true;
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
						sel.List();
					}
				}
				if(ev.type=='MRDN')
				{
					if(lhp>=0)
					{
						flags[lhp]=flags[lhp]^FLAG_NODE;
					}
				}
				if(ev.type=='MLUP')
				{
					dragStart=false;
				}
				mx=ev.x;
				my=ev.y;
			}
		}

		g.SetActiveRT(wsys.GetWindowRT(mainWin));

		drawCount=0;
		nRender=0;
		nnp=nep-1;
		flt2 delta=editxys[nep-1]-editxys[0];
		flt2 deltax=delta;
		flt2 deltay=delta.perp();
		for(int i=1;i<nep;i++)
		{
			normxys[i-1].x=vdot(deltax,editxys[i]-editxys[i-1])/delta.lengthSq();
			normxys[i-1].y=-vdot(deltay,editxys[i]-editxys[i-1])/delta.lengthSq();
		}
		delta=delta*scale;
		globalP=editxys[0]*scale+offset;
		render[0]=globalP;nRender++;
		switch(method)
		{
			case 0:
			{
				Rec0(delta,1.,1.,fdepth, 0);
			}
			break;
			case 1:
			{
				Rec1(delta,1.,1.,fdepth,0);
			}
			break;
			case 2: Rec2(flt2::New(scale,0),depth); break;
			default: Rec0(flt2::New(scale,0),1.,1.,depth, 0);
		}

		style=style%3;

		if((style==1)&&(curves==0))
		{
			g.rgba32(0xffffffff);
			g.FillRT();
			g.rgba32(0xff000000);
			for(int i=0;i<nRender-1;i++)
			{
				g.hairline(render[i].x,render[i].y,render[i+1].x,render[i+1].y);
			}
		}
		else
		{
			g.clear();
			g.M(render[0].x,render[0].y);
			if(curves==4)DrawB(render,nRender);
			else if(curves==3)DrawA(render,nRender);
			else if(curves==2)DrawCR(render,nRender);
			else if(curves==1)DrawQ(render,nRender);
			else DrawLR(render,nRender);
			if(style!=1)
			{
				float l=(render[0]-render[nRender-1]).length()*2;
				g.l(0,l);
				g.l(render[0].x-render[nRender-1].x,0);
				g.close();
			}
			g.fin();

			switch(style)
			{
				case 0:
					g.rgba32(0xff000000);
					g.FillRT();
					g.rgba32(0xff808080);
					g.fill1();
					g.width(1.5,1.5);
					g.rgba32(0xffc0c0c0);
					g.stroke();
				break;
				case 1:
					g.rgba32(0xffffffff);
					g.FillRT();
					g.rgba32(0xff50a060);
					g.width(1.5,1.5);
					g.stroke();
				break;
				case 2:
					g.rgba32(0xffffffff);
					g.FillRT();
					g.rgba32(0xff000000);
					g.fill1();
				break;
			}

		}


		{
			char s[128];
			snprintf(s,128,"Measured %i", drawCount);
			stext(s,10,30,0xffff00ff);
			int count=0;
			switch(method)
			{
				case 0: count=1;for(int i=0;i<depth;i++)count*=nnp;break;
				case 1:
				{
					int nr=0;for(int i=0;i<nnp;i++){if(flags[i]&1)nr++;}
					count=nnp;
					for(int i=1;i<depth;i++)count=count*nr+nnp-nr;
				}
				break;
				case 2: ; break;
				default: ;
			}
			snprintf(s,128,"Calculated %i", count);
			stext(s,10,50,0xff8000ff);
			snprintf(s,128,"nep=%i hp=%i lhp=%i", nep, hp, lhp);
			stext(s,10,70,0xff8000ff);
		}

		if(showHelpers)
		{
			if(snap)
				HelperGrid();
			flt2 zero=offset;
			g.rgba32(0xffffffff);
			g.hairline(zero.x-15,zero.y-1,zero.x+15,zero.y-1);
			g.hairline(zero.x-15,zero.y  ,zero.x+15,zero.y  );
			g.hairline(zero.x-15,zero.y+1,zero.x+15,zero.y+1);
			g.hairline(zero.x-1,zero.y-15,zero.x-1,zero.y+15);
			g.hairline(zero.x  ,zero.y-15,zero.x  ,zero.y+15);
			g.hairline(zero.x+1,zero.y-15,zero.x+1,zero.y+15);
			flt2 pos;
			//printf("scale =%f\n", scale);
			for(int i=0;i<nep;i++)
			{
				flt2 prevPos=pos;
				pos=editxys[i]*scale+offset;
				if(i)
				{
					g.rgba32(0x4000ff00);
					g.hairline(pos.x,pos.y,prevPos.x,prevPos.y);
					if(flags[i-1]&FLAG_NODE)
					{
						g.rgba32(0xff00ff00);
						g.hairline(pos.x,pos.y,prevPos.x,prevPos.y);
					}
					if(flags[i-1]&FLAG_NEGY)
					{
						g.rgba32(0xffff00ff);
						flt2 d=(pos-prevPos).perp().normalized()*10.;
						g.hairline(pos.x,  pos.y,prevPos.x,prevPos.y);
						g.hairline(pos.x,  pos.y,pos.x-d.x,pos.y-d.y);
					}
					if(flags[i-1]&FLAG_NEGX)
					{
						g.rgba32(0xffff00ff);
						flt2 x=(pos-prevPos).normalized()*10.;
						flt2 y=x.perp()*.5;
						flt2 pp=prevPos;
						g.hairline(pos.x,  pos.y,prevPos.x,prevPos.y);
						g.hairline(pp.x, pp.y,pp.x+x.x+y.x,pp.y+x.y+y.y);
						g.hairline(pp.x, pp.y,pp.x+x.x-y.x,pp.y+x.y-y.y);
					}
					if(i==lhp+1)
					{
						g.rgba32(0xffffff00);
						g.hairline(pos.x,pos.y,prevPos.x,prevPos.y);
						flt2 hov=prevPos+(pos-(prevPos))*lht;
						g.rgba32(0xffff0000);
						g.hairline(hov.x-5,hov.y-5,hov.x+5,hov.y+5);
						g.hairline(hov.x+5,hov.y-5,hov.x-5,hov.y+5);

						char s[128];
						snprintf(s,128,"#%i %i %f", i, lhp, lht);
						stext(s,hov.x+5,hov.y-10,0xffff00ff);
					}
				}
				//printf("p[%i]=%f %f \n", i, pos.x, pos.y);
				if(i==hp)
				{
					g.Circle(pos.x, pos.y,0,7,1,0x60ffff00);
					if(i)
					{
						char s[128];
						snprintf(s,128,"#%i%s%s", i,
							flags[i-1]&FLAG_NEGX?" -x":"",
							flags[i-1]&FLAG_NEGY?" -y":""
						);
						stext(s,pos.x+5,pos.y-10,0xffff00ff);
					}
				}
				if(sel.smap[i])
					g.Circle(pos.x, pos.y,0,3,1,0xff0080ff);
				else
					g.Circle(pos.x, pos.y,0,3,1,0xffff0000);
			}
		}
		menu.MenuDraw(g,0,0,G_SCREEN_WIDTH,20);
		Present();
		g.SetActiveRT(wsys.GetWindowRT(optWin));
		g.rgba32(0xff607080);
		g.FillRT();
		gui.Render(g);
		wsys.Present(optWin);
	}
	return 0;
}