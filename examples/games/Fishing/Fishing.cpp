#include "PH_System2.h"

#define G_SCREEN_WIDTH 1280	
#define G_SCREEN_HEIGHT 720
#define G_SCREEN_SCALE 2

#include "graphics.h"

Graph g;
int main()
{
	puts("Fish there!\n");
	PH_System2 ph;
	ph.Reset();
	int floor_level=G_SCREEN_HEIGHT-10;
	while(true)
	{
		float T=Time();
		g.rgba32(0xff000040);
		g.FillRT();
		g.fillrect(0,floor_level,G_SCREEN_WIDTH,10,0xff606040);
		int mx;
		int my;
		int mb;
		GetMouseState(mx,my,mb);
		if(mb&1)
		{
			ph.nodes[0].xf.x=-(ph.nodes[0].p.x-mx)*150000;
			ph.nodes[0].xf.y=-(ph.nodes[0].p.y-(floor_level-my))*150000;
		}
		else
		{
			ph.nodes[0].xf.Zero();
		}
		for(int i=0;i<2;i++)
		{
			ph.Step(0.001);
		}
		g.rgba32(0xffffffff);
		g.clear();

		for(int i=0;i<ph.NLinks;i++)
		{
			PH_Node& n0=ph.nodes[ph.links[i].n0];
			PH_Node& n1=ph.nodes[ph.links[i].n1];
			PH_Num3 p0=n0.p;
			PH_Num3 p1=n1.p;

			//PH_Num3 p0=ph.nodes[i].p;
			//PH_Num3 p1=ph.nodes[i+1].p;
			//if((p1-p0).length()<100)
			{
				g.M(p0.x,floor_level-p0.y);
				g.L(p1.x,floor_level-p1.y);
			}
		}
		g.fin();
		g.width(2,2);
		g.stroke();
		float l=ph.Length();
		char s[64];
		snprintf(s,64,"%f",l);
		stext(s,10,10,0xff00ff00);
		Present();
	}
	return 0;
}




#ifdef UNDEFINED

#include "../src/Bezier.h"
#include "../src/UI.h"
#include "../src/GUI.h"
#include "../src/rand.h"

extern CUI* ui;
extern CSYS* sys;

CGUI gui;

rgba8 col=0x80808080;
CLehmerRand lrand;

rgba8 UI_COL(const rgba8& c)
{
	return c;
}

int MainConfigure(int &W, int &H, int &SX, int &SY, char const*&AppName)
{
	SX=2;
	SY=2;
	W=1920/SX;
	H=1080/SY;
	extern bool DoSound;
	DoSound=false;
	extern bool DoNetwork;
	DoNetwork=false;
	AppName="Fish test";
	return 0;
}

int ButtonPress(UIControl* ct)
{
	if(ct->tag==13)
		sys->Quit();
	else if(ct->tag==101)
		col=rgba8(lrand.F()*255,lrand.F()*255,lrand.F()*255,255);
	return 1;
}

int PushButtonEvent(UIControl* pctl, SEvent* event, void* pud, int hover)
{
	if((event->type==SEvent::ET_MOUSE)&&(event->me.t==SMouseEvent::CLICK))
		return ButtonPress(pctl);
	return 0;
}

PH_System2* ph=0;


typedef float3 Vector3;
typedef Bezier3Calculator<float3> BCF;

void DrawCurve(RT& r, 
	const float2& p0,
	const float2& p1,
	const float2& p2,
	const float2& p3,
	rgba8 col
	)
{
	Bezier3Calculator<float2> bc(p0,p1,p2,p3);
	float t=0;
	int i=0;
	for(;;)
	{
		i++;
		const float stp=1;
		float2 p=bc.Calc(t);
		float2 pdt=bc.Diff(t);
		float dt=stp/pdt.len;
		t+=dt;
		if(t>1.0f)
			break;
		r.putAArgba8F(p.x,p.y,col);
	}
}

inline float2 v3xz(const float3& p)
{
	return float2(p.x,p.z);
}

void DrawCurve(RT& r, 
	const float3& p0,
	const float3& p1,
	const float3& p2,
	const float3& p3,
	rgba8 col
	)
{
	DrawCurve(r,v3xz(p0),v3xz(p1),v3xz(p2),v3xz(p3),col);
}

class FishRender
{
public:
	Vector3 pos;
	Vector3 spd;
	Vector3 target;
	Vector3 rod0;
	Vector3 rod;
	Vector3 rodD;
	Vector3 rodEnd;
	Vector3 hook;
	bool hooked;
	float t;
	float rodLen;
public:
	FishRender()
	{
		Init();
	}
	void Init()
	{
		t = 0;
		pos = 100;
		spd.Set(90, 60, 0);
		target.Set(200, -50, 10);
		hooked = false;
		rodLen = 150;

		rod0.Set(450, 0, 450);
		rod.Set(100);
		rod.normalize();
		rod *= rodLen;
		rodD.Set(0);
		SetRod(100);
	}
	void Hook(const float3& hp)
	{
		hooked=true;
		hook=hp;
	}
	void UnHook()
	{
		hooked=false;
	}
	void SetTarget(const float2& newTarget)
	{
		target.x=newTarget.x;
		target.z=newTarget.y;
	}
	void SetRod(const float2& newRod)
	{
		float x=newRod.x-rod0.x;
		float z=newRod.y-rod0.z;
		float rl=Fsqrt(x*x+z*z);
		if(rl>rodLen)
			rod.Set(rod0.x+x/rl*rodLen,rod0.y,rod0.z+z/rl*rodLen);
		else
			rod.Set(rod0.x+x,rod0.y+Fsqrt(rodLen*rodLen-x*x-z*z),rod0.z+z);
	}
	void Render(RT& rt)
	{
		float2 p0(pos.x, pos.z);
		float2 s(-spd.x, -spd.z);
		s.normalize();
		float3 dd=target-pos;
		
		float sd=-vdot(float2(dd.x,dd.z).normalized(),s.purp());
		float tt=t*0.15;
//		float sl=S_Curve(Clamp01(spd.length()*0.2)*0.5f);
		float sl=1;
		p0              -=s.purp()*1.5*Fsin(tt)*(0.5+sl);
		float2 p1=p0+s* 7+s.purp()*2.5*Fsin(tt)*(0.5+sl);
		float2 p2=p0+s*25-s.purp()*2.5*Fsin(tt)*(0.5+sl);
		float2 p3=p0+s*40+s.purp()*5.0*Fsin(tt)*(0.5+sl);
		
		DrawCurve(rt,p0,p1+s.purp()*( 5+sd*10),p2+s.purp()*sd*10,p3,0xffffffff);
		DrawCurve(rt,p0,p1+s.purp()*(-5+sd*10),p2+s.purp()*sd*10,p3,0xffffffff);
		rt.drawcircle(target.x,target.z,5,0xffffffff);
		rt.hairline(rod0.x,rod0.z,rod.x,rod.z,0x60000000);	
//		rt.hairline(rod.x,rod.z,rod.x+rodD.x,rod.z+rodD.z,0x60ff0000);	
//		if(rodD.length()>10)rodD=rodD.normalized()*10;

		rodEnd=rod+rodD;
		rodEnd=rod0+(rodEnd-rod0).normalized()*rodLen;
		DrawCurve(rt,rod0,rod0*0.67f+rod*0.33f,rod0*0.33f+rod*0.67f,rodEnd,0xffffff00);
	}
	void Step(float dt)
	{
		pos+=spd*dt;
		float spdlen=spd.length();
		float3 dp=target-pos;
		spd+=(dp).normalized()*spdlen*60*0.15f*dt;
/*		if(dp.length()<10)
			spdlen=10;
		else
			spdlen=10+Clamp((dp.length()-10)*0.1,0,1)*10;*/
		spdlen=0.1+S_Curve(Clamp01((dp.length()-2)*0.05))*260;
		spd=spd.normalized()*spdlen;
		if(hooked)
			pos=hook;
		t+=dt*(100+spdlen);
	}
};

FishRender fr;

int MainMouseEvent(const SMouseEvent& me)
{
	gui.MouseEvent(me);
	if(((me.t==SMouseEvent::MOVE)&&(me.r))||(me.t==SMouseEvent::RDN))
	{
		fr.SetRod(float2(me.x-10,me.y-60));
	}
	if(((me.t==SMouseEvent::MOVE)&&(me.l))||(me.t==SMouseEvent::DN))
	{
		fr.SetTarget(float2(me.x-10,me.y-60));
	}
	if(me.t==SMouseEvent::WHEEL)
	{
		ph->seg_len-=0.01*me.w;
		ph->seg_len = Clamp(0.01,3,ph->seg_len);
	}
	
	return 0;
}

int MainInit(int, int)
{
	lrand.SetSeed(0x3475638);
	gui.Init(0, 0);
	gui.AddPanel(0,0,0,0,0);
	gui.SetButtonPressCB(&PushButtonEvent);
	gui.ncTitle=20;
	gui.ncFrame=5;
	gui.defaults.panelCol=0xFFE6EAED;
	gui.defaults.buttonCol=0xFFE6EAED;

	int scrW=ui->GetRT().W();
	gui.AddPanel(0,0,scrW,31," Fire");
	gui.LastPanel()->movable=0;

	gui.AddButton(&gui.LastPanel()->ncButton, scrW-18-5,0,17,13,"x",0,13);

	gui.AddButton(&gui.LastPanel()->firstChild,5,14,17,13,"btn",0,101);

	ph=new PH_System2;
	ph->Reset();

	ph->nodes[0].p.Set(fr.rod.x,fr.rod.y,fr.rod.z);

	fr.Init();

	return 0;
}

int MainRender(RT& rt)
{
	rt.Clear(col);
	RT rt1;
	rt.RTFromRect(rt1,10,60,rt.W()-20,rt.H()-70);
	rt1.fillrect(0,0,rt1.W(),rt1.H(),0xff507060);
	fr.Render(rt1);

	if(1)
	{

	//	rt1.fillrect(0,0,rt1.W(),rt1.H()/2,0xffc0d0e0);
	//	rt1.fillrect(0,rt1.H()/2,rt1.W(),rt1.H()/2,0xffb0c0d0);
	//	rt1.Fill(0xffc0d0e0);
	//	rt1.hairline(10,10,ui->GetLME().x, ui->GetLME().y,0xffffffff);	
		for(int i=1;i<ph->NNodes-1;i++)
		{
			double3 p0=ph->nodes[i].p;
			double3 p1=ph->nodes[i+1].p;
			float colx=Clamp(0,1,p0.y-0.5);
	//		p0.z=rt1.H()/2-p0.z;
	//		p1.z=rt1.H()/2-p1.z;
			if((p1-p0).length()<100)
			{
				rt1.hairline(p0.x,p0.z,p1.x,p1.z,blendSx(0x80000000,0x80ffffff,colx));	
			}
		}
		{
			fr.rodD=-ph->nodes[0].p+ph->nodes[1].p;
			rt1.hairline(ph->nodes[0].p.x,ph->nodes[0].p.z,ph->nodes[1].p.x,ph->nodes[1].p.z,0xffffffff);	
//			rt1.hairline(fr.rodEnd.x,fr.rodEnd.z,ph->nodes[1].p.x,ph->nodes[1].p.z,0xffff0000);	
		}
	/*	for(int i=0;i<ph->NLinks;i++)
		{
			double3 p0=ph->nodes[ph->links[i].n0].p;
			double3 p1=ph->nodes[ph->links[i].n1].p;
			float colx=Clamp(0,1,p0.y-0.5);
			p0.y=rt1.H()/2-p0.y;
			p1.y=rt1.H()/2-p1.y;
			if((p1-p0).length()<100)
			{
				rt1.hairline(p0.z,p0.y,p1.z,p1.y,blendSx(0xffd0e0f0,0xffffffff,colx));	
			}
		}*/

		float len;
		float dp;
		float maxLen;
		for(int i=0;i<20;i++)
		{
			fr.Step(1.0f / 60/20);
			float3 oldp = ph->nodes[0].p;

//			ph->nodes[0].p.Set(ui->GetLME().x-10,10,ui->GetLME().y-60);
			
//			ph->nodes[0].p.Set(fr.rod.x+fr.rodD.x,fr.rod.y+fr.rodD.y,fr.rod.z+fr.rodD.z);
			ph->nodes[0].p=fr.rodEnd;
			ph->nodes[0].p0=ph->nodes[0].p;
//			ph->tiePos=fr.rod+ph->tieForce;

			ph->Step(0.002);
//			fr.rodD=ph->tieForce;



			len=ph->Length();
			maxLen=ph->seg_len*ph->NLinks;
			dp=(ph->nodes[0].p-ph->nodes[ph->NNodes-1].p).length();

			if(dp>maxLen+1)
			{
				fr.Hook(float3(ph->nodes[ph->NNodes-1].p));
				SBitFont::outtext(rt1, font6x12, float2(30,30), "Hooked",-1,0);
			}
			else
			{
				ph->nodes[ph->NNodes-1].p.Set(fr.pos.x,fr.pos.y,fr.pos.z);
				ph->nodes[ph->NNodes-1].xf=fr.spd*10;
				fr.UnHook();
			}


	//	ph->nodes[0].p=100;
	//	ph->nodes[ph->NNodes-1].p.Set(ph->minx+dx*(ui->GetLME().x/(float)rt1.W()),ph->miny+dy*(ui->GetLME().y/(float)rt1.H()));
	//	ph->nodes[ph->NNodes-1].p.Set(ui->GetLME().x,ui->GetLME().y,0);
		}
		SBitFont::outint(rt1, font6x12, float2(30,80), len,-1);
		SBitFont::outint(rt1, font6x12, float2(30,90), dp,-1);
		SBitFont::outint(rt1, font6x12, float2(30,100), maxLen,-1);
	}



	gui.UpdateUI();
	gui.Render(rt);
	gui.DrawCursor(rt, ui->GetLME().x, ui->GetLME().y, arrowCursor);
	return 0;
}

int MainDone()
{
	return 0;
}

int MainKeyEvent(const SKeyEvent& ke)
{
	if(ke.code==SKeyCode::ESC)
		sys->Quit();
	return 1;
}

int MainCharEvent(const SCharEvent& ce)
{
	if (ce.ch)
		sys->LogF("Char: %c\n", ce.ch);
	else
		sys->LogF("Char: U%04i\n", ce.uch);
	return 0;
}

int MainEvent(const SWindowEvent& we)
{
	if(we.t==SWindowEvent::QUITREQ)
		sys->Quit();
	if(we.t==SWindowEvent::CHECKTITLE)
	{
		if((we.x<(ui->GetRT().W()-40))&&(we.y<(gui.ncTitle)))
			return 1;
		else
			return 0;
	}
	return 1;
}
#endif