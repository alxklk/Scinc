#include <stdio.h>
#include <math.h>

// Ray-traced metal still life on a checkered plane:
//   - sphere: silver mirror, resting on the ground
//   - cube:   bronze mirror, balanced on one vertex (space diagonal vertical,
//             so the opposite vertex is exactly above the contact point)
//   - torus:  gold mirror, lying flat on the ground
//   - plane:  y = 0, checkerboard texture, reflectance 0.3
//   - sky:    vertical gradient with a soft sun glow
//
// Metals are coloured mirrors: reflection is carried as a per-channel colour
// "tint" (throughput) that each surface multiplies, so silver/gold/bronze tint
// what they reflect. The plane is a neutral 0.3 mirror.
//
// Geometry intersections:
//   - plane / sphere / cube (oriented box): analytic
//   - torus (a quartic surface): sphere-traced via its signed-distance field,
//     with a bounding-sphere early-out; the surface normal is analytic.
//
// Camera is deliberately off the principal axes -- a purely axial view looks
// unnatural.
//
// Usage:  Scinc raytrace.cpp [width height]     (defaults to 640x480)
// The window and the saved raytrace.png are created at that resolution.
//
// Scinc notes: no comma-initialised declarations; floor()/fabs() are not
// provided so we roll our own; structs are passed/returned by value; atoi() is
// absent so the CLI numbers are parsed with the lexer (MakeTokens/GetNumber).

#include "graphics.h"
#include "../../ws.h"
#include "../../algorithm/Token.h"   // SToken / MakeTokens: parse the CLI resolution
#include "../../algorithm/Lex.h"

Graph g;
typedef char* pchar;

#define MAXB        5       // max reflection bounces
#define AA          2       // anti-aliasing: AA*AA samples per pixel (1 = off)
#define TORUS_STEPS 128     // sphere-tracing iterations for the torus

struct Vec { float x; float y; float z; };

Vec  V(float x, float y, float z){ Vec r; r.x=x; r.y=y; r.z=z; return r; }
Vec  vadd(Vec a, Vec b){ return V(a.x+b.x, a.y+b.y, a.z+b.z); }
Vec  vsub(Vec a, Vec b){ return V(a.x-b.x, a.y-b.y, a.z-b.z); }
Vec  vmul(Vec a, float s){ return V(a.x*s, a.y*s, a.z*s); }
Vec  vcmul(Vec a, Vec b){ return V(a.x*b.x, a.y*b.y, a.z*b.z); }   // per-channel
float vdot(Vec a, Vec b){ return a.x*b.x + a.y*b.y + a.z*b.z; }
Vec  vcross(Vec a, Vec b){ return V(a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x); }
Vec  vnorm(Vec a){ float l=sqrt(vdot(a,a)); return vmul(a, 1.0/l); }
Vec  vreflect(Vec d, Vec n){ return vsub(d, vmul(n, 2.0*vdot(d,n))); }

float mx(float a, float b){ if(a>b) return a; return b; }
float maxc(Vec a){ float m=a.x; if(a.y>m) m=a.y; if(a.z>m) m=a.z; return m; }
float clamp01(float x){ if(x<0.0) return 0.0; if(x>1.0) return 1.0; return x; }
float myfloor(float x){ int i=(int)x; float f=(float)i; if(f>x) f=f-1.0; return f; }
float myabs(float x){ if(x<0.0) return -x; return x; }

struct Hit { int obj; float t; Vec n; };   // obj: 0 miss,1 plane,2 sphere,3 cube,4 torus

// scene globals (assigned in main)
Vec   SC;   float SR;              // sphere
Vec   CB;   float CL;              // cube: bottom vertex + edge length
Vec   CF0;  Vec CF1;  Vec CF2;     // cube edge axes (orthonormal tripod)
Vec   TC;   float TR;  float Tr;   // torus: center, major radius, minor radius
Vec   LDIR;                        // unit direction toward the light
float PLANE_REFL;
Vec   SILVER;  Vec GOLD;  Vec BRONZE;

Vec cubeAxis(int k){ if(k==0) return CF0; if(k==1) return CF1; return CF2; }

Vec metalColor(int obj)
{
	if(obj==2) return SILVER;      // sphere
	if(obj==4) return GOLD;        // torus
	return BRONZE;                 // cube (obj==3)
}

Vec skyColor(Vec d)
{
	float t = 0.5*(d.y+1.0);
	Vec horizon = V(0.85, 0.90, 1.00);
	Vec zenith  = V(0.25, 0.45, 0.85);
	Vec base = vadd(vmul(horizon, 1.0-t), vmul(zenith, t));
	float s = mx(0.0, vdot(d, LDIR));
	float sun = pow(s, 250.0);
	return vadd(base, vmul(V(1.0,0.95,0.8), sun));
}

int hitSphere(Vec o, Vec d)            // 1 if the sphere is hit ahead of o
{
	Vec oc = vsub(o, SC);
	float b = vdot(oc, d);
	float c = vdot(oc,oc) - SR*SR;
	float disc = b*b - c;
	if(disc <= 0.0) return 0;
	if(-b - sqrt(disc) > 1e-3) return 1;
	return 0;
}

Hit hitCube(Vec o, Vec d)              // oriented-box intersection (slab method)
{
	Hit h; h.obj=0; h.t=1e30; h.n=V(0.0,1.0,0.0);
	Vec rel = vsub(o, CB);
	float lo[3];
	float ld[3];
	for(int k=0;k<3;k++){ Vec ax=cubeAxis(k); lo[k]=vdot(rel,ax); ld[k]=vdot(d,ax); }

	float tmin = -1e30;
	float tmax =  1e30;
	int   hitAxis = 0;
	float hitSign = -1.0;
	for(int k=0;k<3;k++)
	{
		if(myabs(ld[k]) < 1e-9)
		{
			if(lo[k] < 0.0 || lo[k] > CL) return h;
		}
		else
		{
			float inv = 1.0/ld[k];
			float ta = (0.0 - lo[k])*inv;
			float tb = (CL  - lo[k])*inv;
			float sgn = -1.0;
			if(ta > tb){ float tmp=ta; ta=tb; tb=tmp; sgn=1.0; }
			if(ta > tmin){ tmin=ta; hitAxis=k; hitSign=sgn; }
			if(tb < tmax) tmax=tb;
		}
	}
	if(tmin > tmax) return h;
	if(tmax < 1e-3) return h;
	float t = tmin;
	if(t < 1e-3) t = tmax;
	if(t < 1e-3) return h;

	Vec n = vmul(cubeAxis(hitAxis), hitSign);
	if(vdot(n,d) > 0.0) n = vmul(n,-1.0);
	h.obj=3; h.t=t; h.n=n;
	return h;
}

float torusSDF(Vec p)                  // p in torus-local coords, axis = y
{
	float a = sqrt(p.x*p.x + p.z*p.z) - TR;
	return sqrt(a*a + p.y*p.y) - Tr;
}

Hit hitTorus(Vec o, Vec d)             // sphere-traced quartic surface
{
	Hit h; h.obj=0; h.t=1e30; h.n=V(0.0,1.0,0.0);
	Vec oc = vsub(o, TC);
	float Rb = TR + Tr + 1e-2;          // bounding sphere radius
	float b = vdot(oc, d);
	float c = vdot(oc,oc) - Rb*Rb;
	float disc = b*b - c;
	if(disc <= 0.0) return h;
	float sq = sqrt(disc);
	float t0 = -b - sq;
	float t1 = -b + sq;
	if(t1 < 1e-3) return h;
	if(t0 < 1e-3) t0 = 1e-3;

	float t = t0;
	for(int i=0;i<TORUS_STEPS;i++)
	{
		Vec p = vsub(vadd(o, vmul(d, t)), TC);
		float dist = torusSDF(p);
		if(dist < 1e-4)
		{
			float a = sqrt(p.x*p.x + p.z*p.z);
			float f = 1.0 - TR/a;
			Vec n = vnorm(V(p.x*f, p.y, p.z*f));
			if(vdot(n,d) > 0.0) n = vmul(n,-1.0);
			h.obj=4; h.t=t; h.n=n;
			return h;
		}
		t = t + dist;
		if(t > t1) return h;
	}
	return h;
}

int inShadow(Vec p)                    // is p shadowed from the light by any object?
{
	Vec so = V(p.x, p.y + 1e-3, p.z);
	if(hitSphere(so, LDIR)) return 1;
	if(hitCube(so, LDIR).obj != 0) return 1;
	if(hitTorus(so, LDIR).obj != 0) return 1;
	return 0;
}

Vec planeColor(Vec p, Vec d, float dist)
{
	int ix = (int)myfloor(p.x);
	int iz = (int)myfloor(p.z);
	int parity = (ix + iz) & 1;
	Vec base;
	if(parity) base = V(0.95, 0.95, 0.95);
	else       base = V(0.08, 0.09, 0.12);

	float diff = mx(0.0, LDIR.y);
	float sh = 1.0;
	if(inShadow(p)) sh = 0.30;
	float lit = 0.25 + 0.75*diff*sh;
	base = vmul(base, lit);

	float f = 1.0/(1.0 + dist*dist*0.0015);
	return vadd(vmul(base, f), vmul(skyColor(d), 1.0-f));
}

Hit intersect(Vec o, Vec d)
{
	Hit best; best.obj=0; best.t=1e30; best.n=V(0.0,1.0,0.0);

	if(myabs(d.y) > 1e-6)
	{
		float t = -o.y / d.y;
		if(t > 1e-3 && t < best.t){ best.obj=1; best.t=t; best.n=V(0.0,1.0,0.0); }
	}

	Vec oc = vsub(o, SC);
	float b = vdot(oc, d);
	float c = vdot(oc,oc) - SR*SR;
	float disc = b*b - c;
	if(disc > 0.0)
	{
		float t = -b - sqrt(disc);
		if(t <= 1e-3) t = -b + sqrt(disc);
		if(t > 1e-3 && t < best.t)
		{
			best.obj=2; best.t=t;
			best.n = vnorm(vsub(vadd(o, vmul(d,t)), SC));
		}
	}

	Hit hc = hitCube(o, d);
	if(hc.obj != 0 && hc.t < best.t) best = hc;

	Hit ht = hitTorus(o, d);
	if(ht.obj != 0 && ht.t < best.t) best = ht;

	return best;
}

Vec trace(Vec o, Vec d)
{
	Vec col  = V(0.0, 0.0, 0.0);
	Vec tint = V(1.0, 1.0, 1.0);       // per-channel reflected-light throughput
	for(int bounce=0; bounce<MAXB; bounce++)
	{
		Hit h = intersect(o, d);
		if(h.obj == 0){ col = vadd(col, vcmul(tint, skyColor(d))); break; }

		Vec hit = vadd(o, vmul(d, h.t));
		Vec n = h.n;
		if(h.obj == 1)
		{
			Vec local = planeColor(hit, d, h.t);
			col = vadd(col, vcmul(tint, vmul(local, 1.0-PLANE_REFL)));
			tint = vmul(tint, PLANE_REFL);          // neutral mirror
		}
		else
		{
			Vec mcol = metalColor(h.obj);
			Vec viewd = vmul(d, -1.0);
			Vec hlf = vnorm(vadd(LDIR, viewd));
			float spec = pow(mx(0.0, vdot(n, hlf)), 60.0);
			col = vadd(col, vcmul(tint, vmul(mcol, spec)));   // tinted highlight
			tint = vcmul(tint, mcol);                         // tint reflection
		}
		o = vadd(hit, vmul(n, 1e-3));
		d = vreflect(d, n);
		if(maxc(tint) < 0.01) break;
	}
	return col;
}

int packColor(Vec c)
{
	int r  = (int)(clamp01(c.x)*255.0 + 0.5);
	int gc = (int)(clamp01(c.y)*255.0 + 0.5);
	int bl = (int)(clamp01(c.z)*255.0 + 0.5);
	return 0xff000000 | (r<<16) | (gc<<8) | bl;
}

int main(int argc, pchar* argv)
{
	// resolution from the command line: "Scinc raytrace.cpp <width> <height>"
	// (defaults to 640x480 when omitted). Numbers are parsed with the Scinc
	// lexer, since atoi() is not available.
	int W = 640;
	int H = 480;
	if(argc >= 3)
	{
		SToken tw[16];
		SToken th[16];
		MakeTokens(tw, argv[1]);
		MakeTokens(th, argv[2]);
		if(tw[0].IsNumber()) W = (int)tw[0].GetNumber();
		if(th[0].IsNumber()) H = (int)th[0].GetNumber();
	}
	if(W < 16) W = 16;
	if(H < 16) H = 16;

	// ws.h auto-created a default window; replace it with one at our resolution.
	wsys.DeleteWindow(mainWin);
	mainWin = wsys.CreateWindow(W, H, 1, 1, 0);
	g.SetActiveRT(wsys.GetWindowRT(mainWin));

	LDIR = vnorm(V(0.6, 1.0, -0.5));
	PLANE_REFL = 0.3;

	// metal reflectances (per channel)
	SILVER = V(0.95, 0.94, 0.90);
	GOLD   = V(1.00, 0.77, 0.34);
	BRONZE = V(0.80, 0.50, 0.28);

	// sphere -- silver (back-left)
	SC = V(-1.70, 1.0, 0.60);
	SR = 1.0;

	// cube -- bronze, on a vertex with a vertical space diagonal (back-right)
	CB = V(1.50, 0.0, 0.60);
	CL = 1.40;
	float phi = 0.55;
	float ch = sqrt(2.0/3.0);
	float cv = 1.0/sqrt(3.0);
	float step = 2.0943951;            // 120 degrees
	CF0 = V(ch*cos(phi),        cv, ch*sin(phi));
	CF1 = V(ch*cos(phi+step),   cv, ch*sin(phi+step));
	CF2 = V(ch*cos(phi+2*step), cv, ch*sin(phi+2*step));

	// torus -- gold, lying flat (axis vertical) resting on the plane (front-center)
	TR = 0.78;
	Tr = 0.30;
	TC = V(0.00, 0.30, -0.90);         // center height = Tr so it sits on y=0

	// camera -- deliberately off the principal axes for a natural 3/4 view
	Vec eye    = V(-2.50, 2.25, -5.00);
	Vec target = V( 0.00, 0.55, -0.15);
	Vec wup    = V( 0.00, 1.00,  0.00);
	Vec fwd = vnorm(vsub(target, eye));
	Vec rgt = vnorm(vcross(fwd, wup));
	Vec up  = vcross(rgt, fwd);
	float aspect = (float)W/(float)H;
	float fov = 0.62;

	g.rgba32(0xff000000);
	g.FillRT();

	float t0 = Time();
	for(int y=0; y<H; y++)
	{
		for(int x=0; x<W; x++)
		{
			Vec acc = V(0.0, 0.0, 0.0);
			for(int jy=0; jy<AA; jy++)
				for(int jx=0; jx<AA; jx++)
				{
					float px = x + (jx + 0.5)/AA;
					float py = y + (jy + 0.5)/AA;
					float sx = (2.0*px/W - 1.0) * aspect * fov;
					float sy = (1.0 - 2.0*py/H) * fov;
					Vec dir = vnorm(vadd(fwd, vadd(vmul(rgt, sx), vmul(up, sy))));
					acc = vadd(acc, trace(eye, dir));
				}
			Vec col = vmul(acc, 1.0/(AA*AA));
			g.Rect(x, y, 1, 1, packColor(col));
		}
		if((y%16)==0){ Poll(); Present(); }
	}
	Present();
	float t1 = Time();
	printf("render done in %.2f s (%dx%d, AA=%d)\n", t1-t0, W, H, AA);
	fflush(stdout);

	g.WriteImage("raytrace.png", "png", "");
	printf("wrote raytrace.png\n");
	fflush(stdout);

	while(true){ Present(); Poll(); }
	return 0;
}
