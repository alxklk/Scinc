#include <stdio.h>
#include "graphics.h"

Graph g;

#define M_PI 3.141592654

/*
	char s[64];
	snprintf(s,64,"%i",n);
	stext(s,100,200,0xff00ff00);
*/


void Sector(float cx, float cy, float r0, float r1, float a0, float a1)
{
	a0=a0/180*M_PI;
	a1=a1/180*M_PI;
	g.M(cx+r0*sin(a0),cy-r0*cos(a0));
	g.L(cx+r1*sin(a0),cy-r1*cos(a0));
	
	int n=int((a1-a0)*30);

	for(int i=1;i<n+1;i++)
	{
		float a=a0+(a1-a0)/n*i;
		g.L(cx+r1*sin(a),cy-r1*cos(a));
	}
	for(int i=0;i<n;i++)
	{
		float a=a1+(a0-a1)/n*i;
		g.L(cx+r0*sin(a),cy-r0*cos(a));
	}
	g.close();
}

void Arrow(float x, float y, float a, float l, float w, float wl, float ll)
{
	a=a/180*M_PI;
	float dx=cos(a);
	float dy=sin(a);
	g.M(x-dy*w,y+dx*w);
	g.l(dx*(l-w*ll),dy*(l-w*ll));
	g.l(-dy*w*wl, dx*wl*w);
	g.l( ( dx*ll+dy*(wl+1))*w, ( dy*ll-dx*(wl+1))*w);
	g.l( (-dx*ll+dy*(wl+1))*w, (-dy*ll-dx*(wl+1))*w);
	g.l(-dy*w*wl, dx*wl*w);
	g.L(x+dy*w,y-dx*w);
	g.close();
}

float r0=80;
float r1=120;

float dr0[8]={1,-1,1,-1,1,-1,1,-1};
float dr1[8]={1,-1,1,-1,1,-1,1,-1};

void Ring(float x, float y, float T)
{
	g.clear();
	Sector(x,y,r0+dr0[0],r1+dr1[0],T*90    ,T*90+45);
	Sector(x,y,r0+dr0[1],r1+dr1[1],T*90+45 ,T*90+90);
	Sector(x,y,r0+dr0[2],r1+dr1[2],T*90+180,T*90+225);
	Sector(x,y,r0+dr0[3],r1+dr1[3],T*90+225,T*90+270);
	g.fin();
	g.rgb(.8,.25,0);
	g.fill1();
	g.width(2.,2);
	g.stroke();

	g.clear();
	Sector(x,y,r0+dr0[4],r1+dr1[4],T*90+90 ,T*90+135);
	Sector(x,y,r0+dr0[5],r1+dr1[5],T*90+135,T*90+180);
	Sector(x,y,r0+dr0[6],r1+dr1[6],T*90+270,T*90+315);
	Sector(x,y,r0+dr0[7],r1+dr1[7],T*90+315,T*90+360);
	g.fin();
	g.rgb(0,.78,1.);
	g.fill1();
	g.width(2.,2);
	g.stroke();
}

int main()
{
	int i=320;
	while(true)
	{
		double T=Time()*7.5;
		//T=0.5;
		g.clear();
		g.gray(.5);
		g.fill1();

		stext("Left ring is growing bigger", 10,10,0x40000000);
		stext("Right ring is shrinking smaller", 10,20,0x40000000);

		float d=.75;

		for(int i=0;i<8;i++)
		{
			dr0[i]=-d;
			dr1[i]=d;
		}
		Ring(180, 240, T);
		for(int i=0;i<8;i++)
		{
			dr0[i]=(i&1)?-d:d;
			dr1[i]=(i&1)?-d:d;
		}
		Ring(180, 240, T+1);

		for(int i=0;i<8;i++)
		{
			dr0[i]=-d;
			dr1[i]=d;
		}
		Ring(460, 240, T);
		for(int i=0;i<8;i++)
		{
			dr0[i]=(i&1)?d:-d;
			dr1[i]=(i&1)?d:-d;
		}
		Ring(460, 240, T+1);


		g.clear();
		Arrow(195,240,  0,40,4,2.,4);
		Arrow(180,255, 90,40,4,2.,4);
		Arrow(165,240,180,40,4,2.,4);
		Arrow(180,225,270,40,4,2.,4);
		Arrow(410,240,  0,40,4,2.,4);
		Arrow(460,190, 90,40,4,2.,4);
		Arrow(510,240,180,40,4,2.,4);
		Arrow(460,290,270,40,4,2.,4);
		g.fin();
		g.rgba(1,1,1,.25);
		g.width(4,1);
		g.stroke();
		g.alpha(1);
		g.rgb(0,0,0);
		g.fill1();

		Present();
	}
	return 0;
}