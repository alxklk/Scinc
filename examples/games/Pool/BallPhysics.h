#ifndef __SCINC__
#include <match.h>
#endif

#define NBalls 8

typedef float PH_Num;

struct PH_Num2
{
	float x;
	float y;
};

struct PH_Num3
{
	float x;
	float y;
	float z;
	float length()
	{
		return sqrt(x*x+y*y+z*z);
	}
	static PH_Num3 New(float x, float y, float z)
	{
		PH_Num3 result;
		result.x=x;
		result.y=y;
		result.z=z;
		return result;
	}
	PH_Num3 normalized()
	{
		float r=1.0/length();
		PH_Num3 result;
		result.x=x*r;
		result.y=y*r;
		result.z=z*r;
		return result;
	}
	void normalize()
	{
		float r=1.0/length();
		x*=r;
		y*=r;
		z*=r;
	}
	void Zero()
	{
		x=0;
		y=0;
		z=0;
	}
	PH_Num3 operator- (PH_Num3 r)
	{
		PH_Num3 result;
		result.x=x-r.x;
		result.y=y-r.y;
		result.z=z-r.z;
		return result;
	}
	PH_Num3 operator+ (PH_Num3 r)
	{
		PH_Num3 result;
		result.x=x+r.x;
		result.y=y+r.y;
		result.z=z+r.z;
		return result;
	}
	PH_Num3 operator+= (PH_Num3 r)
	{
		PH_Num3 result;
		result.x=x=x+r.x;
		result.y=y=y+r.y;
		result.z=z=z+r.z;
		return result;
	}
	PH_Num3 operator-= (PH_Num3 r)
	{
		PH_Num3 result;
		result.x=x=x-r.x;
		result.y=y=y-r.y;
		result.z=z=z-r.z;
		return result;
	}
	PH_Num3 operator* (PH_Num r)
	{
		PH_Num3 result;
		result.x=x*r;
		result.y=y*r;
		result.z=z*r;
		return result;
	}
	PH_Num3 operator/ (PH_Num r)
	{
		float rr=1.0/r;
		PH_Num3 result;
		result.x=x*rr;
		result.y=y*rr;
		result.z=z*rr;
		return result;
	}

};

PH_Num vdot(PH_Num3& l, PH_Num3& r)
{
	return l.x*r.x+l.y*r.y+l.z*r.z;
}

struct PH_Node
{
	PH_Num  m;// mass
	PH_Num3 v;// speed
	PH_Num3 f;// force
	PH_Num3 xf;// force
	PH_Num3 p;// position
	PH_Num3 p0;// previous position
	PH_Num3 a;// acceleration
	bool uw;
	bool fix;
	int nl;
};

float Min(float x, float y){return x<y?x:y;}
float Max(float x, float y){return x<y?y:x;}
float Fabs(float x){return x<0?-x:x;}

class BallPhys
{
public:
	float ball_size;
	PH_Num dempf;

	PH_Node nodes[256];
	int NNodes;
	int nt;
	int tp;

	
	PH_Num area;
	PH_Num2 center;
	PH_Num pressure;
	PH_Num speed;
	bool force;
	PH_Num2 forcepos;
	PH_Num2 forcedir;

	PH_Num minx;
	PH_Num miny;
	PH_Num maxx;
	PH_Num maxy;

	PH_Num3 tiePos;
	PH_Num3 tieForce;


	void Reset()
	{
		ball_size=10;
		dempf=0.999998;
		NNodes=NBalls;
		for(int i=0;i<NNodes;i++)
		{
			PH_Node& n0=nodes[i];
			n0.v .Zero();
			n0.f .Zero();
			n0.xf.Zero();
			n0.fix=false;
			n0.a.Zero();
			n0.m=0.5;
			n0.nl=0;
			{
				n0.p=n0.p0=PH_Num3::New(100+i,100+i/3.0f,0);
			}
		}
	}

	void Extends()
	{
		PH_Node& n0=nodes[0];
		minx=n0.p.x;
		miny=n0.p.y;
		maxx=n0.p.x;
		maxy=n0.p.y;
		for(int i=1; i<NNodes; i++)
		{
			PH_Node& n=nodes[i];
			minx=Min(minx, n.p.x);
			miny=Min(miny, n.p.y);
			maxx=Max(maxx, n.p.x);
			maxy=Max(maxy, n.p.y);
		}
	}

	void CalcPositions(PH_Num dt)
	{
		for(int i=0;i<NNodes;i++)
		{
			PH_Node& n=nodes[i];
			if(n.p.y<ball_size)
			{
				n.p.y=ball_size-n.p.y;
			}
			if(n.p.y>500-ball_size)
			{
				n.p.y-=(ball_size-(500-n.p.y));
			}
			if(n.p.x<ball_size)
			{
				n.p.x=(ball_size+(ball_size-n.p.x)*dempf);
			}
			if(n.p.x>500-ball_size)
			{
				n.p.x=500-ball_size-(ball_size-(500-n.p.x));
			}
		}

		for(int j=0;j<NNodes;j++)
		{

			for(int i=0;i<NNodes;i++)
			{
				if(i==j)
					continue;
				PH_Node& n0=nodes[i];
				PH_Node& n1=nodes[j];
				PH_Num3 d=(n1.p-n0.p);
				PH_Num f=d.length()-ball_size*2;
				if(f<0)
				{
					PH_Num3 dn=d.normalized();
					d=dn*f*dempf;
					n0.p+=d;
					n1.p-=d;
				}
			}
		}

		for(int i=0;i<NNodes;i++)
		{
			PH_Node& n0=nodes[i];
			PH_Num3 np=n0.p+(n0.p-n0.p0)*dempf+n0.a*(dt*dt);
			n0.p0=n0.p;
			n0.p=np;
		}


	}

	void CollectForces()
	{


		// gravity
		if(1)
		{
			for(int i=0;i<NNodes;i++)
			{
				PH_Node& n0=nodes[i];
				n0.f.y-=n0.m*9.8*10000;
			}
		}

		// acceleration
		for(int i=0;i<NNodes;i++)
		{
			PH_Node& n0=nodes[i];
			n0.a=n0.f/n0.m;
		}
		//nodes[NNodes-1].a=nodes[NNodes-1].f/2;
		
	}
	void Clean()
	{
		for(int i=0;i<NNodes;i++)
		{
			PH_Node& n0=nodes[i];
			n0.f=n0.xf;
		}
	}


	void Step(PH_Num dt)
	{
		Clean();
		CollectForces();
		CalcPositions(dt);
	}
};

