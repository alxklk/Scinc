#include <stdio.h>

#define G_SCREEN_WIDTH 512
#define G_SCREEN_HEIGHT 512
#define G_SCREEN_SCALE 2
#define G_SCREEN_MODE 1

#include "graphics.h"

Graph g;

int gseed=12736347;

int irand(int& seed)
{
	seed=(seed*1103515245+12345)&0x7ffffff;
	return seed;
}

float Fabs(float x)
{
	return x<0?-x:x;
}

void Wave(float x, float y, int col, float t, float w)
{
	g.clear();
	g.M(x,y-cos(t)*3.);
	for(int i=0;i<50;i++)
	{
		float dy=sin(i*.25-t);
		//dy*=Fabs(dy);
		dy*=(1.2+i*0.05);
		g.l(-5,dy);
	}
	g.fin();
	g.width(w,w);
	g.rgba32(col);
	g.stroke();
}

void RoundRect(float x, float y, float w, float h, float r)
{
	g.M(x,y+r);
	if(r>0)g.a(r,r,0,0,1,r,-r);
	g.l(w-r*2,0);
	if(r>0)g.a(r,r,0,0,1,r,r);
	g.l(0,h-r*2);
	if(r>0)g.a(r,r,0,0,1,-r,r);
	g.l(-w+r*2,0);
	if(r>0)g.a(r,r,0,0,1,-r,-r);
	g.close();
}

float Fmod(float x,float y)
{
	return x-int(x/y)*y;
}

float Fmax(float x, float y)
{
	return x>y?x:y;
}

float Fmin(float x, float y)
{
	return x<y?x:y;
}

void Star(float x, float y, float t)
{
	t=Fmod(t,1)*30.;
	x-=t*100;

	if(t<1)
	{
		float r1=Fmin(t*t*30,25);// Fmax(t*12,t*55-10);
		float r2=Fmin(t*50,25);
		g.M(x+r1,y);g.L(x+r2,y);
		g.M(x-r1,y);g.L(x-r2,y);
		g.M(x,y+r1);g.L(x,y+r2);
		g.M(x,y-r1);g.L(x,y-r2);
		if(t>.5)
		{
			g.M(x+17,y+17);
			g.M(x-17,y+17);
			g.M(x+17,y-17);
			g.M(x-17,y-17);
		}
	}
	if((t<1.25)&&(t>.5))
	{
			g.M(x,y);
	}
}

int main()
{
	while(true)
	{
		float T=Time();
		g.rgba32(0xff000040);
		g.FillRT();
		g.clear();
		g.M(256,256);
		g.fin();
		g.width(256*1.7,1);
		g.rgba32(0xff1060a0);
		g.stroke();
		int y=0;

		g.clear();
		gseed=73661561;
		for(int i=0;i<200;i++)
		{
			Star(irand(gseed)%512+100,irand(gseed)%512,T*.05+(irand(gseed)%200)*.005);
		}
		g.fin();
		g.rgba32(0xffffffff);
		g.width(3,3);
		g.stroke();

		Wave(200,226+y,0xffff0000,T*12,11);y+=14;
		Wave(200,226+y,0xffff8000,T*12,11);y+=14;
		Wave(200,226+y,0xffffff00,T*12,11);y+=14;
		Wave(200,226+y,0xff00ff00,T*12,11);y+=14;
		Wave(200,226+y,0xff20a0ff,T*12,11);y+=12;
		Wave(200,226+y,0xff6000ff,T*12,8);
		g.clear();
		float bdx=-sin(T*12)*5;
		float bdy=cos(T*12)*3;
		g.M(200+bdx,270+bdy);
		g.c(-10,-5,-20,bdy*2-5,-40+bdy,-bdy*4-10);

		g.M(210,305-bdy);
		g.l(-bdx*2.5-5,15);
		g.M(235,305+bdx);
		g.l(-bdy*2.5-5,15);

		g.M(290,305-bdx*1.4);
		g.l(bdy*3.,15);
		g.M(315,305-bdy);
		g.l(-bdx*2.,15);

		g.fin();
		g.rgba32(0xff000000);
		g.width(13,13);
		g.stroke();
		g.rgba32(0xffa0a0a0);
		g.width(8,8);
		g.stroke();

		g.clear();
		RoundRect(200+bdx,210+bdy,135,100,15);
		g.fin();
		g.rgba32(0xffe0c0a0);
		g.fill1();
		g.rgba32(0xff000000);
		g.width(3,3);
		g.stroke();
		g.clear();
		g.rgba32(0xffff90f0);
		RoundRect(210+bdx,220+bdy,115,80,20);
		g.fin();
		g.fill1();

		g.clear();
		gseed=16799381;
		g.M(215+bdx+ 0*10,220+bdy+5*10);
		g.M(215+bdx+ 1*10,220+bdy+1*10);
		g.M(215+bdx+ 2*10,220+bdy+7*10);
		g.M(215+bdx+ 3*10,220+bdy+4*10);
		g.M(215+bdx+ 4*10,220+bdy+2*10);
		g.M(215+bdx+ 5*10,220+bdy+5*10);
		g.M(215+bdx+ 6*10,220+bdy+1*10);
		g.M(215+bdx+ 7*10,220+bdy+6*10);
		g.M(215+bdx+ 8*10,220+bdy+1*10);
		g.M(215+bdx+ 9*10,220+bdy+2*10);
		g.fin();
		g.width(4,4);
		g.rgba32(0xffff30d0);
		g.stroke();


		g.clear();
		g.M(285+bdx*.75,260+bdy*.75-bdx*.8);
		g.a(42,37,0,1,0,75,0);
		g.l(-5,-22);
		g.l(-7,0);
		g.l(-16,15);
		g.l(-22,0);
		g.l(-16,-15);
		g.l(-7,0);
		g.close();
		g.fin();

		g.rgba32(0xffa0a0a0);
		g.fill1();
		g.rgba32(0xff000000);
		g.width(3,3);
		g.stroke();

		g.clear();
		g.M(295+bdx*.75,290+bdy*.75-bdx*.8);
		g.M(355+bdx*.75,290+bdy*.75-bdx*.8);
		g.fin();
		g.width(7,7);
		g.rgba32(0xfff0a0a0);
		g.stroke();

		g.clear();
		g.M(305+bdx*.75,278+bdy*.75-bdx*.8);
		g.M(348+bdx*.75,278+bdy*.75-bdx*.8);
		g.fin();
		g.width(8,8);
		g.rgba32(0xff000000);
		g.stroke();

		g.clear();
		g.M(302+bdx*.75,276+bdy*.75-bdx*.8);
		g.M(345+bdx*.75,276+bdy*.75-bdx*.8);
		g.fin();
		g.rgba32(0xffffffff);
		g.width(3,3);
		g.stroke();


		g.clear();
		g.M(307+bdx*.75,290+bdy*.75-bdx*.8);
		g.a(1,1,0,0,0,20,0);
		g.a(1,1,0,0,0,20,0);
		g.M(330+bdx*.75,282+bdy*.75-bdx*.8);
		g.fin();
		g.width(3,3);
		g.rgba32(0xff000000);
		g.stroke();

		Present();
	}
	return 0;
}