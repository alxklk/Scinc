#define G_SCREEN_SCALE 2
#define G_SCREEN_WIDTH 1024
#define G_SCREEN_HEIGHT 768

#include "graphics.h"

Graph g;

#define float double

float w=180;
float sw=G_SCREEN_WIDTH;
float sh=G_SCREEN_HEIGHT;

#define M_PI 3.141592654

struct flt2
{
	float x;
	float y;
	float length()
	{
		return sqrt(x*x+y*y);
	}
	static flt2 New(float x, float y)
	{
		flt2 result;
		result.x=x;
		result.y=y;
		return result;
	}
	flt2 normalized()
	{
		float r=1.0/length();
		flt2 result;
		result.x=x*r;
		result.y=y*r;
		return result;
	}
	flt2 perp()
	{
		flt2 result;
		result.x=y;
		result.y=-x;
		return result;
	}
	void normalize()
	{
		float r=1.0/length();
		x*=r;
		y*=r;
	}
	void Zero()
	{
		x=0;
		y=0;
	}
	flt2 operator- ()
	{
		flt2 result;
		result.x=-x;
		result.y=-y;
		return result;
	}
	flt2 operator- (flt2 r)
	{
		flt2 result;
		result.x=x-r.x;
		result.y=y-r.y;
		return result;
	}
	flt2 operator+ (flt2 r)
	{
		flt2 result;
		result.x=x+r.x;
		result.y=y+r.y;
		return result;
	}
	flt2 operator+= (flt2 r)
	{
		flt2 result;
		result.x=x=x+r.x;
		result.y=y=y+r.y;
		return result;
	}
	flt2 operator-= (flt2 r)
	{
		flt2 result;
		result.x=x=x-r.x;
		result.y=y=y-r.y;
		return result;
	}
	flt2 operator* (float r)
	{
		flt2 result;
		result.x=x*r;
		result.y=y*r;
		return result;
	}
	flt2 operator/ (float r)
	{
		float rr=1.0/r;
		flt2 result;
		result.x=x*rr;
		result.y=y*rr;
		return result;
	}
	flt2 operator/= (float r)
	{
		float rr=1.0/r;
		flt2 result;
		result.x=x*rr;
		result.y=y*rr;
		*this=result;
		return result;
	}

};

struct STriangle
{
	int type;
	int l;
	flt2 a;
	flt2 b;
	flt2 c;
	void Set(int t, flt2& newa, flt2& newb, flt2& newc)
	{
		type=t;
		a=newa;
		b=newb;
		c=newc;
	}
};

double gr;

int Subdivide(STriangle& t, STriangle* t0, STriangle* t1, STriangle* t2)
{
	if(t.type==0)
	{
		flt2 p=t.a+(t.b-t.a)/gr;
		t0->Set(0,t.c,p,t.b);
		t1->Set(1,p,t.c,t.a);
		return 2;
	}
	else if(t.type==1)
	{
		flt2 q=t.b+(t.a-t.b)/gr;
		flt2 r=t.b+(t.c-t.b)/gr;
		t0->Set(1, r, t.c, t.a);
		t1->Set(1, q,   r, t.b);
		t2->Set(0, r,   q, t.a);
		return 3;
	}
	return 0;
}

void RenderTriangle(STriangle& t)
{
	g.clear();
	if(t.type==0)
	{
		g.rgba32(0xffffd000);
	}
	else if(t.type==1)
	{
		g.rgba32(0xff4060ff);
	}
	g.M(t.b.x,t.b.y);
	g.L(t.a.x,t.a.y);
	g.L(t.c.x,t.c.y);
	g.close();
	g.fin();
	g.fill1();
	g.width(.5,1.);
	g.stroke();

	g.clear();
	g.M(t.b.x,t.b.y);
	g.L(t.a.x,t.a.y);
	g.L(t.c.x,t.c.y);
	g.fin();
	g.width(1.,1.);
	g.rgba32(0x80000000);
	g.stroke();

}

float tan(float x)
{
	return sin(x)/cos(x);
}

class STriangleStorage
{
public:
	STriangle* s;
	int n;
	void Add(STriangle& src)
	{
		s[n]=src;
		n++;
	}
};

void RecSubdiv(STriangle& t, STriangleStorage& s0, int level, int endLevel)
{
	STriangle t0;
	STriangle t1;
	STriangle t2;
	int nt=Subdivide(t,&t0,&t1,&t2);
	t0.l=level;
	t1.l=level;
	t2.l=level;
	if(nt==2)
	{
		s0.Add(t0);
		s0.Add(t1);
		if(level<endLevel)
		{
			RecSubdiv(t0, s0, level+1, endLevel);
			RecSubdiv(t1, s0, level+1, endLevel);
		}
	}
	else if(nt==3)
	{
		s0.Add(t0);
		s0.Add(t1);
		s0.Add(t2);
		if(level<endLevel)
		{
			RecSubdiv(t0, s0, level+1, endLevel);
			RecSubdiv(t1, s0, level+1, endLevel);
			RecSubdiv(t2, s0, level+1, endLevel);
		}
	}
}

void RecRender(STriangle& t, int level, int endLevel)
{
	STriangle t0;
	STriangle t1;
	STriangle t2;
	int nt=Subdivide(t,&t0,&t1,&t2);
	if(nt==2)
	{
		if(level<endLevel)
		{
			RecRender(t0, level+1, endLevel);
			RecRender(t1, level+1, endLevel);
		}
		else
		{
			RenderTriangle(t0);
			RenderTriangle(t1);
		}
	}
	else if(nt==3)
	{
		if(level<endLevel)
		{
			RecRender(t0, level+1, endLevel);
			RecRender(t1, level+1, endLevel);
			RecRender(t2, level+1, endLevel);
		}
		else
		{
			RenderTriangle(t0);
			RenderTriangle(t1);
			RenderTriangle(t2);
		}
	}
}


int main()
{
	gr=(1.+sqrt(5.))/2.;
	printf("gr: %f %f\n", gr, gr*gr);
	double T=0;
	g.width(1.25,1.25);
	STriangle t0;
	t0.type=0;
	t0.a=flt2::New(512,-1570);
	float l=2470;
	t0.b=flt2::New(t0.a.x-sin(M_PI/10.)*l,t0.a.y+cos(M_PI/10.)*l);
	t0.c=flt2::New(t0.a.x+sin(M_PI/10.)*l,t0.a.y+cos(M_PI/10.)*l);
	//STriangleStorage ta0;ta0.n=0;ta0.s=(STriangle*)malloc(sizeof(STriangle)*7000);
	//RecSubdiv(t0,ta0,0,8);
	//printf("%i triangles\n", ta0.n);
	while(true)
	{
		T=Time();
		//g.rgba32(0xffc0c0c0);
		//g.FillRT();

		//RenderTriangle(t0);
		//RenderTriangle(t1);

		RecRender(t0,0,9);

		//for(int i=0;i<ta0.n;i++)
		//{
		//	if(ta0.s[i].l==8)RenderTriangle(ta0.s[i]);
		//}

		Present();
		Wait(.1);
	}
	return 0;
}