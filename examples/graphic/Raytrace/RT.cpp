#include <stdio.h>
#include <math.h>

#define SW 320
#define SH 240
#ifdef __SCINC__
#warning "Scinc defined"
#define NL 5
#else
#warning "Scinc not defined"
#define NL SH
#endif

#define G_SCREEN_WIDTH 320
#define G_SCREEN_HEIGHT 240
#define G_SCREEN_MODE 1
#define G_SCREEN_SCALE 4
#define USE_AA 3

#ifdef __SCINC__
#define const
#endif

#define GRAPH

#ifdef GRAPH
#include "graphics.h"
Graph g;
#endif

#define float3 flt3

class float3
{
public:
	float x;
	float y;
	float z;
	static float3 New(float newx, float newy, float newz)
	{
		float3 retval;
		retval.x=newx;
		retval.y=newy;
		retval.z=newz;
		return retval;
	}
	float3 Normalized() const
	{
		float3 ret=*this;
		ret.Normalize();
		return ret;
	}
	void Normalize()
	{
		float l=1./sqrt(x*x+y*y+z*z);
		x*=l;
		y*=l;
		z*=l;
	}
	float3 operator-(float r) const
	{
		return New(x-r,y-r,z-r);
	}
	float3 operator-(const float3& r) const
	{
		return New(x-r.x,y-r.y,z-r.z);
	}
	void operator-=(const float3& r)
	{
		x-=r.x;
		y-=r.y;
		z-=r.z;
	}
	float3 operator-() const
	{
		return New(-x,-y,-z);
	}
	float3 operator+(const float3& r) const
	{
		return New(x+r.x,y+r.y,z+r.z);
	}
	void operator+=(const float3& r)
	{
		x+=r.x;
		y+=r.y;
		z+=r.z;
	}
	float3 operator*(float r) const
	{
		return New(x*r,y*r,z*r);
	}
	void operator*=(float r)
	{
		x*=r;
		y*=r;
		z*=r;
	}
	float3 operator/(float r) const
	{
		return New(x/r,y/r,z/r);
	}
};

float3 cross(const float3& u, const float3& v)
{
	return float3::New(
		u.y*v.z-u.z*v.y,
		u.z*v.x-u.x*v.z,
		u.x*v.y-u.y*v.x
		);
}

float dot(const float3& v1, const float3& v2)
{
	return v1.x*v2.x+v1.y*v2.y+v1.z*v2.z;
}

float clamp(float x, float min, float max)
{
	if(x<min)return min;
	else if(x>max)return max;
	return x;
}

struct Intersection
{
	int id;
	float t;
	float3 p;
	float3 n;
	float3 uv;
};

struct Reflection
{
	int id;
	float3 dir;
	float fresn;
};

struct Plane
{
	int id;
	float3 p0;
	float3 dp1;
	float3 dp2;
};

struct Sphere
{
	int id;
	float3 p;
	float r;
};

struct Ray
{
	float3 p;
	float3 d;
};

struct Trace
{
	float3 rgb;
	float val;
};

struct Quad
{
	int id;
	float3 p0;
	float3 p1;
	float3 p2;
};

struct Cylinder
{
	int id;
	float3 p0;
	float3 p1;
	float r;
};

void RayCylinder(const Cylinder& C, const Ray& R, Intersection& i)
{
	float r2=C.r*C.r;
	float3 dp=C.p1-C.p0;
	float3 dpt=dp/dot(dp,dp);
	float3 ao=R.p-C.p0;
	float3 aoxab=cross(ao,dpt);
	float3 vxab=cross(R.d,dpt);
	float ab2=dot(dpt,dpt);
	float a=2.0*dot(vxab,vxab);
	float b=2.0*dot(vxab,aoxab);
	float c=dot(aoxab,aoxab)-r2*ab2;
	float det=b*b-2.0*a*c;

 
	if(det<0.0)
	{
		return;
	}

	float ra=1.0/a;
	det=sqrt(det);

	float t0=(-b+det)*ra;
	float t1=(-b-det)*ra;

	bool inside=false;
	if(t0>t1)
	{
		float temp=t1;
		t1=t0;
		t0=temp;
	}

	float d=t0;
	if(d<0.0)
	{
		return;
	}

	float3 ip=R.p+R.d*d;
	float3 lp=ip-C.p0;
	float ct=dot(lp,dpt);
	if((ct<0.0)||(ct>1.0))
	{	
		d=t1;
		if(d<0.0)
		{
			return;
		}

		inside=true;
		ip=R.p+R.d*d;
		float3 lp=ip-C.p0;
		float ct=dot(lp,dpt);
		if((ct<0.0)||(ct>1.0))
		{
			return;
		}
	}

	if((i.id!=0)&&(i.t<d))
		return;

	i.id=C.id;
	i.p=ip;
	i.t=d;
	i.n=(ip-(C.p0+dp*ct)).Normalized();
	if(inside)i.n=-i.n;
	i.uv.y=ct;
	i.uv.x=atan2(i.n.x,i.n.y);
	i.uv.z=0.0;
}

void RayPlane(const Plane& p, const Ray& r,  Intersection& i)
{
	float3 dp0=r.p-p.p0;

	float3 dett=cross(p.dp1,p.dp2);
	float3 detuv=cross(dp0,r.d);

	float det=-1.0/dot(dett,r.d);

	float t=(dot(dett ,dp0))*det;

	if(t>0.)
	{
		if((i.id!=0)&&(i.t<t))
			return;
		float u=(dot(detuv,p.dp2))*det;
		float v=(dot(detuv,p.dp1))*det;
		i.id=p.id;
		i.t=t;
		i.uv=float3::New(u,v,0.);
		i.p=r.p+r.d*t;
		i.n=-dett.Normalized();
	}
}

void RaySphere(const Sphere& s, const Ray& r, Intersection& i)
{
	float3 l=s.p-r.p;
	float tc=dot(l,r.d);
	if(tc<0.0)
	{
		return;
	};

	float d2=s.r*s.r+tc*tc-dot(l,l);

	if(d2<0.0)
	{
		return;
	};

	float thc=sqrt(d2);
	float t=tc-thc;
	if((i.id!=0)&&(i.t<t))
		return;

	i.t=tc-thc;
	i.p=r.p+r.d*i.t;
	i.n=(i.p-s.p).Normalized();
	i.uv=i.p;
	i.id=s.id;
}

void F3ToCol(const float3& col, char* c)
{
	c[0]=int(clamp(col.z,0.,1.)*255);
	c[1]=int(clamp(col.y,0.,1.)*255);
	c[2]=int(clamp(col.x,0.,1.)*255);
}


void RayQuad(const Quad& q, const Ray& r, Intersection& i)
{
	float3 dp0=r.p-q.p0;
	float3 dp1=q.p1-q.p0;
	float3 dp2=q.p0-q.p2;

	float3 dett=cross(dp1,dp2);
	float3 detuv=cross(dp0,r.d);

	float det=-1.0/dot(dett,r.d);

	float t=(dot(dett ,dp0))*det;

	if(t>0.)
	{
		if((i.id!=0)&&(i.t<t))
			return;
		float u=(dot(detuv,dp2))*det;
		if((u<0.)||(u>1.))
			return;
		float v=(dot(detuv,dp1))*det;
		if((v<0.)||(v>1.))
			return;
		i.id=q.id;
		i.t=t;
		i.uv=float3::New(u,v,0.);
		i.p=r.p+r.d*t;
		i.n=dett.Normalized();
	}
}

Sphere sphere;
Sphere sphere1;
Sphere sphere2;
Plane plane;
Cylinder cyl1;
float3 tolight;
Quad quad[6];

void RayScene(const Ray& ray, Intersection& is)
{
	RaySphere(sphere,ray,is);
	RaySphere(sphere1,ray,is);
	RaySphere(sphere2,ray,is);
	//RayCylinder(cyl1,ray,is);
	RayPlane(plane,ray,is);
	//for(int i=0;i<6;i++)RayQuad(quad[i],ray,is);
}

float3 HorzColor(float y)
{
	if(y<0.)y*=-.2;
	y=pow(1.-y,6);
	return float3::New(.9,.75,.7)*y+float3::New(.4,.6,.9)*(1.-y);
}

float3 VReflect(const float3& ray, const float3& norm)
{
	return (ray+norm*dot(norm,-ray)*2.0).Normalized();
}

void SceneCol(const Intersection& is, float3& col, float& a)
{
	col=float3::New(.5,.5,.5);
	if(is.id==1)
	{
		int ip=(int(is.uv.x+1000)^int(is.uv.y-1000))&1;
		col=float3::New(ip,ip,ip);
	}
	else if(is.id==2)
		col=float3::New(1.,.2,.1);
	else if(is.id==3)
		col=float3::New(.2,.5,1.);
	else if(is.id==4)
		col=float3::New(1.,.8,.2);
	else if(is.id==5)
		col=float3::New(is.uv.x,is.uv.y,.5);

	a=1.;
}


void SceneDirectionalLight(
	const Intersection& is, 
	const float3& ldir,
	const Ray& ray,
	const Reflection& refl,
	float3& col,
	float& a
	)
{
	float lamb=dot(is.n, tolight)*1.5;
	if(lamb<0)lamb=0;

	float3 halfn=-(tolight-ray.d).Normalized();

	//float spec1=clamp(dot(halfn,-is.n),0.0,1.0);
	float spec2=dot(tolight,refl.dir);

	float light=lamb;
	float shadow=1.;
	{
		Intersection iss;
		iss.id=0;
		float3 tol=ldir;
		Ray rays;
		rays.p=is.p+tol*0.002;
		rays.d=tol;

		RayScene(rays,iss);
		if(iss.id!=0)
			shadow=0.;
	}
	col=col*(.25+.125+.125*is.n.y+light*shadow*.5);

	float fresn=refl.fresn;
	float frw=pow(spec2,120.)*shadow*fresn*2.;
	
	col=col*(1.-frw)+float3::New(5,4,2)*frw;
}


void TraceScene(
	const Ray& ray,
	float3& ocol,
	Intersection& is,
	Reflection& refl,
	float& a)
{
	ocol=HorzColor(ray.d.z);

	is.id=0;
	refl.id=0;
	RayScene(ray, is);
	if(is.id!=0)
	{
		refl.id=is.id;
		refl.dir=VReflect(ray.d,is.n);
		refl.fresn=.3+.7*pow(clamp(1.0-dot(ray.d,-is.n),0.0,1.0),2.);

		a=0;
		SceneCol(is,ocol,a);
		SceneDirectionalLight(is, tolight, ray, refl, ocol,a);
	}
	else
	{
		refl.fresn=1.0;
	}

}


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

int ib[18]={
	0,1,2,
	4,5,6,
	0,1,5,
	1,2,6,
	2,3,7,
	3,0,4
};

void rot(float& x, float& y, float a)
{
	float ox=x;
	x=x*cos(a)+y*sin(a);
	y=-ox*sin(a)+y*cos(a);
}

void rotrot(float3& p, float a)
{
	rot(p.x,p.y,.3*a);
	rot(p.y,p.z,.6*a);
	rot(p.z,p.x,.2*a);
}

float3 TraceRay(const Ray& ray, int depth)
{
	Reflection refl;
	Intersection is;
	is.id=0;
	float3 ocol;
	float a;
	TraceScene(ray,ocol,is,refl,a);
	float fresn=refl.fresn;
	Trace hits[16];
	hits[0].rgb=ocol;
	hits[0].val=refl.fresn*a;

	
	int maxdepth=0;
	for(int r=1;r<depth;r++)
	{
		if(refl.id!=0)
		{
			Ray rray;
			Intersection ris;
			ris.id=0;
			refl.id=0;
			rray.p=is.p+refl.dir*0.001;
			rray.d=refl.dir;
			float3 rcol;
			float ra;
			TraceScene(rray,rcol,ris,refl,ra);
			hits[r].rgb=rcol;
			hits[r].val=refl.fresn;
			maxdepth=r;
			is=ris;
		}
		else
		{
			break;
		}
	}

	ocol=hits[maxdepth].rgb;
	for(int r=maxdepth-1;r>=0;r--)
		ocol=hits[r].rgb*(1.-hits[r].val)+ocol*hits[r].val;
	return ocol;
}


CWinSys ws;

int main()
{
	int x0;
	int y0;
	int win0=ws.CreateWindow(G_SCREEN_WIDTH,G_SCREEN_HEIGHT,4,4,1);
	printf("NL=%i\n", NL);
	if(0)while(true)
	{
		g.rgba32(0xff404050);
		g.FillRT();
		g.clear();
		g.M(160,70);
		g.a(50,50,0,0,0,0,100);
		g.a(50,50,0,0,0,0,-100);
		g.fin();
		g.rgba32(0xffffffff);
		g.stroke();
		Sphere s;
		s.p=float3::New(160,120,0);
		s.r=50;
		Ray r;
		r.p=float3::New(x0,y0,0);
		int mx;
		int my;
		int mb;
		GetMouseState(mx,my,mb);
		if(mb){x0=mx;y0=my;}
		r.d=float3::New(mx-r.p.x,my-r.p.y,0).Normalized();
		Intersection in;
		in.id=0;
		RaySphere(s,r,in);
		g.clear();
		g.M(r.p.x,r.p.y);
		if(in.id)
		{
			g.L(in.p.x, in.p.y);
			float3 refl=VReflect(r.d,in.n);
			g.l(refl.x*1000,refl.y*1000);
			g.fin();
			g.rgba32(0xff008000);
			g.stroke();
			g.clear();

			//float3 refr;
			//if(dot(r.d,in.n)<0)
			//	refr=-VRefract(r.d,in.n,.8);
			//else
			//	refr=-VRefract(r.d,-in.n,1./.8);
			//g.M(in.p.x, in.p.y);
			//g.l(refr.x*1000,refr.y*1000);
			g.fin();
			g.rgba32(0xff000080);
			g.stroke();
		}
		else
		{
			g.l(r.d.x*1000,r.d.y*1000);
			g.fin();
			g.rgba32(0xff008000);
			g.stroke();

		}


		ws.Present(win0);
	}


	float3 p0=float3::New(1,1,1);
	int n=0;
	char* buf=(char*)malloc(SW*SH*4);
	tolight=float3::New(1,1,1).Normalized();
	sphere.p=float3::New(0,0,0);
	sphere.r=0.6;
	sphere.id=3;
	sphere1.p=float3::New(0.,0.8,.5);
	sphere1.r=0.5;
	sphere1.id=2;
	sphere2.p=float3::New(0.,-0.7,.4);
	sphere2.r=0.4;
	sphere2.id=4;
	plane.id=1;
	plane.p0=float3::New(0,0,-.7);
	plane.dp1=float3::New(-1,0,0);
	plane.dp2=float3::New(0,1,0);
	for(int i=0;i<6;i++)
	{
		quad[i].id=4;
		int i0=ib[i*3+1];
		int i1=ib[i*3+0];
		int i2=ib[i*3+2];
		quad[i].p0=float3::New(vb[i0*3],vb[i0*3+1],vb[i0*3+2])*.5;
		quad[i].p1=float3::New(vb[i1*3],vb[i1*3+1],vb[i1*3+2])*.5;
		quad[i].p2=float3::New(vb[i2*3],vb[i2*3+1],vb[i2*3+2])*.5;
		rotrot(quad[i].p0,3.7);
		rotrot(quad[i].p1,3.7);
		rotrot(quad[i].p2,3.7);
	}
	cyl1.id=4;
	cyl1.p0=float3::New(0.5,-0.7,0);
	cyl1.p1=float3::New(0.,-0.7,.4);;
	cyl1.r=.4;

	float3 Z=float3::New(0,0,1);
	int fn=0;
	float t=Time();
	bool changed=false;
	int mx;
	int my;
	int mb;
	int oldmx;
	int oldmy;
	int oldmb;
	GetMouseState(mx,my,mb);
	oldmb=mb;
	oldmx=mx;
	oldmy=my;

	bool upPressed=false;
	bool downPressed=false;
	bool leftPressed=false;
	bool rightPressed=false;
	bool forwardPressed=false;
	bool backPressed=false;


	float3 campos=float3::New(2,2,1);
	float3 forward=-float3::New(2,2,1).Normalized();

	for(;;)
	{
		oldmb=mb;
		oldmx=mx;
		oldmy=my;
		GetMouseState(mx,my,mb);
		if(mb)
		{
			float3 right=cross(forward,Z).Normalized();
			float3 up=cross(forward,right).Normalized();
			changed=true;
			campos-=right*(mx-oldmx)*.01;
			campos-=up*(my-oldmy)*.01;
			forward+=right*(mx-oldmx)*.01;
			forward+=up*(my-oldmy)*.01;
			forward.Normalize();
		}

		forwardPressed  = KeyPressed(119);
		backPressed     = KeyPressed(115);
		leftPressed     = KeyPressed(97 );
		rightPressed    = KeyPressed(100);
		downPressed     = KeyPressed(113);
		upPressed       = KeyPressed(101);


		//if(forwardPressed||backPressed||leftPressed||rightPressed||downPressed||upPressed)
		//	changed=true;

		{
			float3 right=cross(forward,Z).Normalized();
			float3 up=cross(forward,right).Normalized();
			if(forwardPressed){campos+=forward*0.1;changed=true;}
			if(   backPressed){campos-=forward*0.1;changed=true;}
			if(   leftPressed){campos-=right*0.1;changed=true;}
			if(  rightPressed){campos+=right*0.1;changed=true;}
			if(   downPressed){campos-=up*0.1;changed=true;}
			if(     upPressed){campos+=up*0.1;changed=true;}
		}

		if(changed)
		{
			SetPersistentFloat("campos.x",campos.x);
			SetPersistentFloat("campos.y",campos.y);
			SetPersistentFloat("campos.z",campos.z);
			SetPersistentFloat("forward.x",forward.x);
			SetPersistentFloat("forward.y",forward.y);
			SetPersistentFloat("forward.z",forward.z);
		}


		campos.x=GetPersistentFloat("campos.x",campos.x);
		campos.y=GetPersistentFloat("campos.y",campos.y);
		campos.z=GetPersistentFloat("campos.z",campos.z);
		forward.x=GetPersistentFloat("forward.x",forward.x);
		forward.y=GetPersistentFloat("forward.y",forward.y);
		forward.z=GetPersistentFloat("forward.z",forward.z);
		
		float3 right=cross(forward,Z).Normalized();
		float3 up=cross(forward,right).Normalized();
		Ray ray;
		ray.p=campos;


		if(changed)
		{
			#define RS 8
			int nx=SW/RS;
			int ny=SH/RS;
			for(int i=0;i<ny;i++)
			{
				for(int j=0;j<nx;j++)
				{
					float3 ocol=float3::New(0,0,0);
					ray.d=(forward*2+up*(i/((ny-.5)*.5)-1.)+right*(j/((ny-.5)*.5)-1.333)).Normalized();
					ocol+=TraceRay(ray,2);
					ray.d=(forward*2+up*((i+.5)/((ny-.5)*.5)-1.)+right*(j/((ny-.5)*.5)-1.333)).Normalized();
					ocol+=TraceRay(ray,2);
					ray.d=(forward*2+up*(i/((ny-.5)*.5)-1.)+right*((j+.5)/((ny-.5)*.5)-1.333)).Normalized();
					ocol+=TraceRay(ray,2);
					ray.d=(forward*2+up*((i+.5)/((ny-.5)*.5)-1.)+right*((j+.5)/((ny-.5)*.5)-1.333)).Normalized();
					ocol+=TraceRay(ray,2);
					ocol*=0.25;

					ocol.z=clamp(ocol.z,0.,1.);
					ocol.y=clamp(ocol.y,0.,1.);
					ocol.x=clamp(ocol.x,0.,1.);

					g.rgb(ocol.z, ocol.y, ocol.x);
					g.fillrect(j*RS,i*RS,RS,RS);
					{
						char s[64];
						snprintf(s,64,"\033[%i;%iH", i, j*2);
						printf(s);
						//snprintf(s,64,"\e[48;5;%im  ", 16+int(ocol.z*5.99)+int(ocol.y*5.99)*6+int(ocol.x*5.99)*36);
						snprintf(s,64,"\e[48;2;%i;%i;%im  ",int(ocol.x*255.99),int(ocol.y*255.99),int(ocol.z*255.99));
						printf(s);
					}

				}
			}
			ws.Present(win0);
			changed=false;
			fn=0;
			continue;
		}

		int last=fn+NL;
		if(last>SH)
			last=SH;
		for(int y=fn;y<last;y++)
		{
			for(int x=0;x<SW;x++)
			{
				float3 c0=float3::New(0,0,0);
				char* p=&buf[(y*SW+x)*4];
#ifdef USE_AA
				for(int i=0;i<USE_AA;i++)
				{
					for(int j=0;j<USE_AA;j++)
					{
						ray.d=(forward*2+up*((y+i*(1./USE_AA))/(SH*.5)-1.)+right*((x+j*(1./USE_AA))/(SH*.5)-1.333)).Normalized();
#else
						ray.d=(forward*2+up*((y)/(SH*.5)-1.)+right*((x)/(SH*.5)-1.333)).Normalized();
#endif

					c0+=TraceRay(ray,8);

#ifdef USE_AA
					}
				}


				F3ToCol(c0*(1./(USE_AA*USE_AA)),p);
#else
				F3ToCol(c0,p);
#endif
				p[3]=255;
			}
		}

		int n=NL;
		if(fn+NL>SH)
			n=SH-fn;
#ifdef GRAPH
		g.BitBlt(&buf[fn*SW*4],SW,NL,0,fn);
#ifdef __SCINC__
		g.rgba32(0xffffffff);
		g.lineH(0,fn+NL,SW);
		//printf("Line %i\n",fn+NL);
#endif
		//SetPresentWait(0);
		ws.Present(win0);
#ifdef __SCINC__
		// prevent CPU overheating
		//Wait(0.01);
#endif

#endif
		fn+=NL;
		if(fn>SH-1)
		{
			//fn=0;
			//break;
		}
	}
	return 0;
}