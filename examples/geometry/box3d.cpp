#include "graphics.h"

struct flt2
{
	float x;
	float y;
};

void rot(float& x, float& y, float a)
{
	float ox=x;
	x=x*cos(a)+y*sin(a);
	y=-ox*sin(a)+y*cos(a);
}

float t;

flt2 Animate(float x, float y, float z)
{
	rot(x,y,t*0.5);
	rot(y,z,t*0.31);
	rot(z,x,t*0.37);
	flt2 res;
	res.x=x/(y+4)*3;
	res.y=z/(y+4)*3;
	return res;
}

Graph g;

int main()
{
	float vb[24]={
		-1,-1,-1,
		 1,-1,-1,
		 1, 1,-1,
		-1, 1,-1,
		-1,-1, 1,
		 1,-1, 1,
		 1, 1, 1,
		-1, 1, 1
	};
	int lb[24]={
		0,1,1,2,2,3,3,0,
		4,5,5,6,6,7,7,4,
		0,4,1,5,2,6,3,7
	};
	int cols[6]=
	{0x000000ff,0x00ff0000,0x0000ff00,
	0x00ffff00,0x0000ffff,0x00ff00ff};
	int ib[24]={
		0,1,2,3,
		4,5,6,7,
		0,1,5,4,
		1,2,6,5,
		2,3,7,6,
		3,0,4,7
	};
	while(true)
	{
		t=Time();
		g.rgba(.4,.15,.1,1);
		g.FillRT();
		g.clear();
		g.alpha(1);
		for(int i=0;i<6;i++)
		{
			g.clear();
			int i0=ib[i*4];
			int i1=ib[i*4+1];
			int i2=ib[i*4+2];
			int i3=ib[i*4+3];
			flt2 p0=Animate(vb[i0*3],vb[i0*3+1],vb[i0*3+2]);
			flt2 p1=Animate(vb[i1*3],vb[i1*3+1],vb[i1*3+2]);
			flt2 p2=Animate(vb[i2*3],vb[i2*3+1],vb[i2*3+2]);
			flt2 p3=Animate(vb[i3*3],vb[i3*3+1],vb[i3*3+2]);
			g.M(p0.x*100+320,p0.y*100+240);
			g.L(p1.x*100+320,p1.y*100+240);
			g.L(p2.x*100+320,p2.y*100+240);
			g.L(p3.x*100+320,p3.y*100+240);
			g.fin();
			g.rgb((cols[i]&0xff)/255.,((cols[i]>>8)&0xff)/255.,((cols[i]>>16)&0xff)/255.);
			g.alpha(.5);
			g.fill1();
		}
		g.clear();
		for(int i=0;i<12;i++)
		{
			int i0=lb[i*2];
			int i1=lb[i*2+1];
			flt2 p0=Animate(vb[i0*3],vb[i0*3+1],vb[i0*3+2]);
			flt2 p1=Animate(vb[i1*3],vb[i1*3+1],vb[i1*3+2]);
			g.M(p0.x*100+320,p0.y*100+240);
			g.L(p1.x*100+320,p1.y*100+240);
		}
		g.fin();
		g.alpha(1);
		g.rgb(.4,.15,.1);
		g.width(30,1);
		g.stroke();
		g.gray(1);
		g.width(1.,1.);
		g.stroke();
		Present();
	}
	return 0;
}