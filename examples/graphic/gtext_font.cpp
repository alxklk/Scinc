#ifdef __JS__
	#define G_SCREEN_SCALE 2
#else
	//#define G_SCREEN_WIDTH 1280
	//#define G_SCREEN_HEIGHT 960
	#define G_SCREEN_SCALE 2
#endif

#include "graphics.h"

Graph g;

#include "font.h"

CFont* pfont;

#include "letters.h"

void DrawGlyph(int c, float x, float y, float sx, float sy)
{
	CFont& font=*pfont;
	CGlyph& gl=font.g[c];
	for(int i=0;i<NSPANS;i++)
	{
		float* p=&(gl.s[i].p[0]);
		if(p[0]==0)
			break;
		if(p[0]==1)
			g.M(x+p[1]*sx,y-p[2]*sy);
		else if(p[0]==2)
			g.m(p[1]*sx,-p[2]*sy);
		else if(p[0]==3)
			g.L(x+p[1]*sx,y-p[2]*sy);
		else if(p[0]==4)
			g.l(p[1]*sx,-p[2]*sy);
		else if(p[0]==5)
		{
			g.M(x+p[1]*sx,y-p[2]*sy);
			g.C(
				x+p[3]*sx,y-p[4]*sy,
				x+p[5]*sx,y-p[6]*sy,
				x+p[7]*sx,y-p[8]*sy);
		}
	}
}

void DrawText(char* s, float x, float y, float sx, float sy)
{
	int i=0;
	while(true)
	{
		int c=s[i];
		if(c==0)
			break;
		DrawGlyph(c,x+i*6*sx,y,sx,sy);
		i++;
	}
}


int main()
{
	pfont=(CFont*)malloc(sizeof(CFont));
	CFont& font=*pfont;
	font.Init();
	MakeLetters();
	char st[32];
	//MLC_Typer t;
	//t.Init();
	int cx=-1;
	int cy=-1;
	cx=GetPersistentInt("cx",cx);
	cy=GetPersistentInt("cy",cy);

	while(true)
	{
		g.t_0(0,0);g.t_x(1,0);g.t_y(0,1);
		g.alpha(1);
		g.rgb(.7,.75,.9);
		g.clear();
		g.M(0,0);g.l(640,0);g.l(0,480);g.l(-640,0);g.close();g.fin();
		g.fill1();
		g.clear();

		g.clear();
		for(int i=0;i<=16;i++)
		{
			g.M(i*20+10.5,130.5);
			g.l(0,320);
			g.M(10.5,130.5+i*20);
			g.l(320,0);
		}
		for(int i=0;i<=32;i++)
		{
			g.M(i*7+400.5-42,400.5+42);
			g.l(0,-224);
			g.M(400.5-42,400.5+42-i*7);
			g.l(224,0);
		}
		g.fin();
		g.rgb(.6,.7,.8);
		g.width(1,1);
		g.stroke();

		g.clear();
		for(int i=0;i<=32;i+=2)
		{
			g.M(i*7+400.5-42,400.5+42);
			g.l(0,-224);
			g.M(400.5-42,400.5+42-i*7);
			g.l(224,0);
		}
		g.fin();
		g.rgb(.8,.85,.9);
		g.width(1,1);
		g.stroke();
		g.clear();

		g.clear();
		g.M(400.5,400.5+42);
		g.l(0,-224);
		g.M(400.5+7*10,400.5+42);
		g.l(0,-224);
		g.M(400.5-42,400.5);
		g.l(224,0);
		g.M(400.5-42,400.5-7*18);
		g.l(224,0);
		g.M(400.5-42,400.5-7*12);
		g.l(224,0);
		g.fin();
		g.rgba(0,0,0,0.2);
		g.width(2,2);
		g.stroke();
		g.clear();
		g.alpha(1);


		int mx;
		int my;
		int mb;
		GetMouseState(mx,my,mb);

		snprintf(st,32,"Time %f", Time());
		DrawText(st,10.5,30.5,1.5,1);

		DrawText("!\"#$%&\'()*+,-./:;<=>?@[\\]^_{|}~0123456789",10.5,60.5,1,1);
		DrawText("the quick brown fox jumps over the lazy dog",10.5,90.5,1,1);
		DrawText("THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG",10.5,120.5,1,1);

		g.fin();
		g.rgb(.3,.1,0);
		g.width(1.,1.);
		g.stroke();

		if(mb)
		{
			cx=(mx-10)/20;
			cy=(my-130)/20;
			SetPersistentInt("cx",cx);
			SetPersistentInt("cy",cy);
		}
		if((cx>=0)&&(cx<16)&&(cy>=0)&&(cy<16))
		{
			g.clear();
			char s[32];
			int code=cx+cy*16;
			snprintf(s,32,"%i %i code %i",cx,cy,code);
			DrawText(s,400.,160.,2,2);
			g.fin();
			g.rgb(.3,.1,0);
			g.width(1.5,1.5);
			g.stroke();

			g.clear();
			g.t_t(0,0,1,0,0,1);
			//t.b0=Flt2(400,400);
			//t.x=Flt2(14,0);
			//t.y=Flt2(0,-14);
			//t.Glyph(code);
			DrawGlyph(code,400,400,14,14);
			g.fin();
			g.rgba(.3,.1,0,.5);
			g.width(7.,7.);
			g.stroke();

			for(int i=0;i<10;i++)s[i]=code;
			s[10]=0;
			s[0]='2';
			s[4]='a';
			s[6]='d';

			g.clear();
			DrawText(s,420,130,2,2);
			g.fin();
			g.rgba(0,0,0,1);
			g.width(3.5,1.);
			g.stroke();
			g.rgb(.8,.9,1.);
			g.width(1.5,1.5);
			g.stroke();

			g.clear();
			DrawText(s,440.5,100.5,1,1);
			g.fin();
			g.rgba(.3,.1,0,1);
			g.width(1,1);
			g.stroke();

			{
				float x=400;
				float y=400;
				float sx=14;
				float sy=14;
				CGlyph& gl=font.g[code];
				for(int i=0;i<NSPANS;i++)
				{
					float* p=&(gl.s[i].p[0]);
					if(p[0]==0)
						break;
					if(p[0]==5)
					{
						g.clear();
						g.M(x+p[1]*sx,y-p[2]*sy);
						g.L(x+p[3]*sx,y-p[4]*sy);
						g.fin();
						g.rgba(1,0,1,1);
						g.width(1.,1.);
						g.stroke();
						g.clear();
						g.M(x+p[7]*sx,y-p[8]*sy);
						g.L(x+p[5]*sx,y-p[6]*sy);
						g.fin();
						g.rgba(1,1,0,1);
						g.width(1.,1.);
						g.stroke();
					}
				}
			}
		}

		g.alpha(1);


		for(int i=0;i<16;i++)
		{
			for(int j=0;j<16;j++)
			{
				char s[2];
				s[1]=0;
				s[0]=i+j*16;
				if((s[0]<127)&&(s[0]>32))
					stext(s,20+i*20,141+j*20,0xffffffff);
				//stext(s,11+i*20,142+j*20,0x40ffffff);
			}
		}

		g.clear();
		g.t_t(0,0,1,0,0,1);
		//t.x=Flt2(1,0);
		//t.y=Flt2(0,-1);
		for(int i=0;i<16;i++)
		{
			for(int j=0;j<16;j++)
			{
				//t.b0=Flt2(10.5+i*20,150.5+j*20);
				//t.Glyph(i+j*16);
				DrawGlyph(i+j*16,10.5+i*20,150.5+j*20,1,1);
			}
		}
		g.fin();
		g.rgb(.3,.1,0);
		g.width(1,1);
		g.stroke();



		Present();
	}
	return 0;
}