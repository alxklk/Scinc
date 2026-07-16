#include <stdio.h>
#include "sound.h"        // intrinsic: CSound audio API (snd.*)
#include "graphics.h"     // intrinsic: the Graph drawing API (g.*)
#include "../../ws.h"     // windowing: Present(), Time() (real file at examples/ws.h)

// wind_chimes -- a simulator of a set of tuned wind-chime tubes stirred by a
// turbulent breeze, with a 3D-projected animation and synthesized sound.
//
// The scene is genuinely 3D.  The suspension ring is a real circle lying in a
// horizontal plane; the tubes hang straight down from points spaced evenly
// around it (so some are near the camera, some far).  The central striker is a
// SPHERICAL PENDULUM swinging in that horizontal plane, with a wind sail below
// it.  Everything is perspective-projected to the screen and drawn back-to-front
// (painter's algorithm) by camera depth -- so the striker is correctly occluded
// by near tubes and draws over far ones, and never falsely intersects them.
// The striker rings a tube when its horizontal distance to that tube gets small,
// louder the faster it is moving -- a gentle breeze taps single notes, a gust
// sets off a flurry.
//
// SOUND -- each tube is a struck tube: an additive stack of INHARMONIC partials
// each with its own exponential decay + a contact transient.  To keep the audio
// cheap for the interpreter, every partial is a coupled-form ("magic circle")
// oscillator (cos/sin computed once at the strike, then two multiplies + two
// adds per sample), so the inner loop has NO sin()/pow().  A soft stereo
// ping-pong delay adds air.  Tubes are tuned to an A-minor pentatonic.
//
// Four materials, each a different synthesis model and look (M cycles, 1-4 pick):
//   Metal  - long inharmonic bending modes (1 : 2.76 : 5.40 : 8.93), shimmering
//   Glass  - brighter, near-harmonic, up an octave, tinkly and short (translucent)
//   Bamboo - hollow woody knock: few partials, fast decay, down a bit (joints)
//   Bell   - strong strike tone over a long low hum, very inharmonic
//
// Controls:  SPACE = gust    - / + = less / more breeze    M / 1-4 = material
//            click a tube to ring it    R = random gust   C = calm   Q/Esc = quit
//
// Scinc idioms (see track_editor.cpp / fractal_melody.cpp): a graphical sound
// loop must call snd.Poll() itself; no comma-initialised declarations; plain
// char*; no rand() (hand-rolled LCG); no exp() (pow once, recurrence in the hot
// loop); struct array-members / by-value struct returns are fine; fill polygons
// with g.fill1(); g.Circle(x,y,inner,outer,aa,col) for discs.

#define M_PI    3.14159265358979
#define TWO_PI  6.28318530717959
#define EULER   2.71828182845905
#define SR      48000
#define BUF_LOW 4000
#define CHUNK   512

#define W       640            // logical canvas (ws.h default 640x480, scale 2)
#define H       480

#define NT      6              // number of chime tubes
#define NV      14             // audio voice pool
#define NPART   4              // inharmonic partials per struck tube
#define NR      24             // ripple-ring pool
#define NW      46             // wind streaks
#define NITEMS  8              // depth-sorted drawables: NT tubes + striker + sail

// ---- 3D world geometry (world units ~ pixels at the ring's depth) ----------
#define RW       58.0          // suspension-ring radius (horizontal circle)
#define CORDLEN  14.0          // ring rim -> tube top
#define TUBER     5.0          // tube half-width
#define LROD     70.0          // pivot -> striker (sits at mid-tube height)
#define LSAIL   140.0          // pivot -> wind sail (hangs below the tubes)
#define BALLR    11.0          // striker radius
#define SAILR    20.0          // wind-sail radius
#define CEIL     82.0          // ceiling anchor height above the ring
#define WOBS      6.0          // struck-tube visual swing amplitude
#define LEANS    10.0          // wind lean amplitude

// ---- perspective camera ----------------------------------------------------
#define FOCAL   460.0
#define CAMDIST 300.0
#define PITCHA    0.40         // camera tilt (looking up under the ring)
#define PROJCX  320.0
#define PROJCY  125.0

// ---- pendulum / strike tuning ---------------------------------------------
#define WFORCE     5.0
#define KREST     12.0
#define DAMP       0.9
#define STRIKEBAND 32.0
#define MINSPEED  12.0

// ------------------------------- globals -----------------------------------
Graph  g;
CSound snd;

int    playSample;             // audio clock (samples)

float  cosP;                   // precomputed camera tilt
float  sinP;

// tube tuning + layout
float  tubeAng[NT];            // angle on the ring (radians)
float  tubeFreq[NT];
float  tubeLen[NT];            // world length
float  tubePan[NT];            // 0=left .. 1=right
int    metalBase[NT];          // body colour
int    metalGlow[NT];          // ring / glow colour

// per-tube animation state
float  ring[NT];               // remaining ring energy 0..1 (glow + wobble)
float  ringT[NT];              // time since last strike (for wobble phase)
float  ringPh[NT];             // random wobble phase
float  cooldn[NT];             // strike debounce timer
float  prevDist[NT];           // last frame's striker->tube horizontal distance

// depth-sort scratch
int    itemType[NITEMS];       // 0=tube 1=striker 2=sail
int    itemIdx[NITEMS];
float  itemDepth[NITEMS];

// ------------------------- chime materials ---------------------------------
#define METAL  0
#define GLASS  1
#define BAMBOO 2
#define BELL   3
#define NMAT   4
char* MAT_NAME[4] = { "Metal", "Glass", "Bamboo", "Bell" };

struct Material
{
	float ratio[NPART];
	float amp[NPART];
	float tauMul[NPART];        // per-partial decay multiplier (bigger = rings longer)
	float baseTau;              // fundamental e-fold time (s)
	float transAmp;             // contact-transient amount
	float transDur;             // contact-transient length (samples)
	float freqMul;              // overall tuning shift (octave)
};
Material mats[NMAT];
int      gMat = METAL;

struct Chime
{
	int   active;
	int   t0;
	float pan;
	float strike;               // strike strength (transient scaling)
	float trA;                  // captured transient amount
	int   trN;                  // captured transient length (samples)
	float cs[NPART];            // coupled-osc cosine state
	float sn[NPART];            // coupled-osc sine state  (this is the output)
	float cw[NPART];            // per-sample rotation cos
	float sw[NPART];            // per-sample rotation sin
	float dec[NPART];           // per-sample amplitude decay
};
Chime voices[NV];

// stereo ping-pong delay
float* delayBuf;
int    delayLen;
int    delayPos;

// striker spherical pendulum (two swing angles)
float  thx = 0.10;
float  thz = 0.0;
float  thxV = 0.0;
float  thzV = 0.0;
float  ox = 0.0;               // striker world offset (from ring centre)
float  oy = -70.0;
float  oz = 0.0;

// wind (3D horizontal)
float  baseWind    = 0.50;     // steady breeze 0..1 (user adjustable)
float  windX = 0.0;
float  windZ = 0.0;
float  windSmoothX = 0.0;
float  windSmoothZ = 0.0;
float  gustx = 0.0;
float  gustz = 0.0;
float  gustTimer = 2.0;

// ripple rings (screen space)
float  rx[NR];
float  ry[NR];
float  rr[NR];
float  rlife[NR];
int    rcol[NR];

// wind streaks (screen space)
float  wx[NW];
float  wy[NW];
float  wln[NW];
float  wsp[NW];

float  tSim = 0.0;
double Tprev;

int    mx;
int    my;
int    mb;
int    prevmb;

// ------------------------------- helpers -----------------------------------
int rndState = 987654321;
int irand()
{
	rndState = rndState * 1103515245 + 12345;
	return (rndState >> 16) & 0x7fff;
}
float frand() { return irand() / 32767.0; }                 // 0..1
float noiseSample() { return irand() / 16383.5 - 1.0; }     // -1..1

float fabsF(float x) { if (x < 0.0) return -x; return x; }
float signF(float x) { if (x > 0.0) return 1.0; if (x < 0.0) return -1.0; return 0.0; }
float clampF(float x, float lo, float hi) { if (x < lo) return lo; if (x > hi) return hi; return x; }
float fminF(float a, float b) { if (a < b) return a; return b; }
float fmaxF(float a, float b) { if (a > b) return a; return b; }

int clampi(int x) { if (x < 0) return 0; if (x > 255) return 255; return x; }
int rgb3(int rr3, int gg3, int bb3)
{
	return 0xff000000 | (clampi(rr3) << 16) | (clampi(gg3) << 8) | clampi(bb3);
}
int chan(int c, int sh) { return (c >> sh) & 0xff; }
int withAlpha(int base, int a) { return (base & 0x00ffffff) | (clampi(a) << 24); }
int lerpCol(int a, int b, float t)
{
	int ar = chan(a, 16); int ag = chan(a, 8); int ab = chan(a, 0);
	int br = chan(b, 16); int bg = chan(b, 8); int bb = chan(b, 0);
	return rgb3(int(ar + (br - ar) * t), int(ag + (bg - ag) * t), int(ab + (bb - ab) * t));
}
int darker(int c, float f)
{
	return rgb3(int(chan(c, 16) * f), int(chan(c, 8) * f), int(chan(c, 0) * f));
}

// body / glow colour of tube i under the current material
int matBody(int i)
{
	if (gMat == GLASS)  return withAlpha(lerpCol(metalBase[i], 0xff9fe8ff, 0.72), 175);
	if (gMat == BAMBOO) return lerpCol(rgb3(183, 145, 63), rgb3(138, 154, 58), i / 5.0);
	if (gMat == BELL)   return lerpCol(metalBase[i], rgb3(205, 127, 50), 0.58);
	return metalBase[i];            // METAL: distinct per-tube metals
}
int matGlowC(int i)
{
	if (gMat == GLASS)  return 0xffffffff;
	if (gMat == BAMBOO) return 0xffe8d08a;
	if (gMat == BELL)   return 0xffffcf80;
	return metalGlow[i];
}

void SetMat(int m, float b0, float b1, float b2, float b3,
                   float a0, float a1, float a2, float a3,
                   float d0, float d1, float d2, float d3,
            float baseTau, float trA, float trN, float fMul)
{
	mats[m].ratio[0]  = b0; mats[m].ratio[1]  = b1; mats[m].ratio[2]  = b2; mats[m].ratio[3]  = b3;
	mats[m].amp[0]    = a0; mats[m].amp[1]    = a1; mats[m].amp[2]    = a2; mats[m].amp[3]    = a3;
	mats[m].tauMul[0] = d0; mats[m].tauMul[1] = d1; mats[m].tauMul[2] = d2; mats[m].tauMul[3] = d3;
	mats[m].baseTau   = baseTau;
	mats[m].transAmp  = trA;
	mats[m].transDur  = trN;
	mats[m].freqMul   = fMul;
}

// ------------------------- 3D perspective projection ------------------------
struct P3 { float sx; float sy; float z; float sc; };

P3 project(float wx3, float wy3, float wz3)
{
	float yr = wy3 * cosP - wz3 * sinP;      // tilt about X
	float zr = wy3 * sinP + wz3 * cosP;
	float Zc = zr + CAMDIST;
	if (Zc < 20.0) Zc = 20.0;
	float sc = FOCAL / Zc;
	P3 p;
	p.sx = PROJCX + wx3 * sc;
	p.sy = PROJCY - yr * sc;                 // screen y grows downward
	p.z  = Zc;                               // larger = farther
	p.sc = sc;
	return p;
}

// ------------------------------- audio -------------------------------------
void AddChime(float freq, float amp, float pan)
{
	int slot = -1;
	for (int i = 0; i < NV; i++)
		if (!voices[i].active) { slot = i; break; }
	if (slot < 0)                       // steal the oldest
	{
		slot = 0;
		for (int i = 1; i < NV; i++)
			if (voices[i].t0 < voices[slot].t0) slot = i;
	}
	Chime& v = voices[slot];
	Material& mt = mats[gMat];
	v.active = 1;
	v.t0     = playSample;
	v.pan    = pan;
	v.strike = amp;
	v.trA    = mt.transAmp;
	v.trN    = int(mt.transDur);
	float f0 = freq * mt.freqMul;
	for (int k = 0; k < NPART; k++)
	{
		float fk = f0 * mt.ratio[k];
		float w  = TWO_PI * fk / SR;
		v.cw[k]  = cos(w);
		v.sw[k]  = sin(w);
		v.cs[k]  = amp * mt.amp[k];     // start vector (magnitude = partial amp)
		v.sn[k]  = 0.0;
		float tau = mt.baseTau * mt.tauMul[k];
		v.dec[k]  = pow(EULER, -1.0 / (tau * SR));
	}
}

float renderChime(Chime& v, int s)
{
	float out = 0.0;
	for (int k = 0; k < NPART; k++)
	{
		float nc = v.cs[k] * v.cw[k] - v.sn[k] * v.sw[k];   // rotate unit vector
		float ns = v.cs[k] * v.sw[k] + v.sn[k] * v.cw[k];
		v.cs[k]  = nc * v.dec[k];                           // and decay it
		v.sn[k]  = ns * v.dec[k];
		out += v.sn[k];
	}
	int age = s - v.t0;
	if (age < v.trN)                    // material-dependent contact transient
	{
		float te = 1.0 - age / float(v.trN);
		out += noiseSample() * te * te * v.strike * v.trA;
	}
	if (age > 7200)                     // give up after a long tail
	{
		if (v.cs[0] * v.cs[0] + v.sn[0] * v.sn[0] < 0.0000004)
			v.active = 0;
	}
	return out;
}

void GenSample()
{
	float l = 0.0;
	float r = 0.0;
	for (int i = 0; i < NV; i++)
	{
		if (!voices[i].active)
			continue;
		float m = renderChime(voices[i], playSample);
		l += m * (1.0 - voices[i].pan);
		r += m * voices[i].pan;
	}
	l *= 0.40;
	r *= 0.40;

	int   dp = delayPos * 2;
	float el = delayBuf[dp];
	float er = delayBuf[dp + 1];
	float outL = l + er * 0.30;         // ping-pong cross feedback
	float outR = r + el * 0.30;
	delayBuf[dp]     = outL * 0.72;
	delayBuf[dp + 1] = outR * 0.72;
	delayPos++;
	if (delayPos >= delayLen)
		delayPos = 0;

	if (outL >  0.95) outL =  0.95; else if (outL < -0.95) outL = -0.95;
	if (outR >  0.95) outR =  0.95; else if (outR < -0.95) outR = -0.95;
	snd.snd_out(outL, outR);
	playSample++;
}

// ------------------------------- gameplay ----------------------------------
void SpawnRipple(float x, float y, int col)
{
	for (int i = 0; i < NR; i++)
		if (rlife[i] <= 0.0)
		{
			rx[i] = x; ry[i] = y; rr[i] = 8.0; rlife[i] = 1.0; rcol[i] = col;
			return;
		}
}

void StrikeTube(int i, float amp)
{
	if (cooldn[i] > 0.0)
		return;
	AddChime(tubeFreq[i], amp, tubePan[i]);
	if (amp > ring[i]) ring[i] = amp;
	ringT[i]  = 0.0;
	ringPh[i] = frand() * TWO_PI;
	cooldn[i] = 0.06;
	float bx = RW * cos(tubeAng[i]);
	float bz = RW * sin(tubeAng[i]);
	P3 mp = project(bx, -CORDLEN - tubeLen[i] * 0.5, bz);
	SpawnRipple(mp.sx, mp.sy, matGlowC(i));
}

void UpdateWind(float dt)
{
	float t = tSim;
	windX = baseWind * (sin(t * 0.37) * 0.6 + sin(t * 0.90 + 1.3) * 0.4) + gustx;
	windZ = baseWind * (sin(t * 0.31 + 2.0) * 0.6 + sin(t * 0.80 + 0.5) * 0.4) + gustz;

	gustTimer -= dt;
	if (gustTimer <= 0.0)               // random gust in a random horizontal dir
	{
		float a = frand() * TWO_PI;
		float m = 0.5 + 0.7 * frand();
		gustx += cos(a) * m;
		gustz += sin(a) * m;
		gustTimer = 1.3 + 4.0 * frand() * (1.3 - baseWind);
	}
	float gd = 1.0 - clampF(dt * 1.4, 0.0, 0.9);
	gustx *= gd;
	gustz *= gd;

	windSmoothX += (windX - windSmoothX) * clampF(dt * 6.0, 0.0, 1.0);
	windSmoothZ += (windZ - windSmoothZ) * clampF(dt * 6.0, 0.0, 1.0);
}

void UpdatePendulum(float dt)
{
	int   sub = 2;
	float h = dt / sub;
	for (int s = 0; s < sub; s++)
	{
		float ax = windX * WFORCE - KREST * sin(thx) - DAMP * thxV;
		thxV += ax * h; thx += thxV * h;
		float az = windZ * WFORCE - KREST * sin(thz) - DAMP * thzV;
		thzV += az * h; thz += thzV * h;
		if (thx >  1.3) { thx =  1.3; thxV *= -0.3; }
		if (thx < -1.3) { thx = -1.3; thxV *= -0.3; }
		if (thz >  1.3) { thz =  1.3; thzV *= -0.3; }
		if (thz < -1.3) { thz = -1.3; thzV *= -0.3; }
	}
	ox = LROD * sin(thx);
	oz = LROD * sin(thz);
	float r2  = ox * ox + oz * oz;
	float lim = LROD * LROD * 0.90;
	if (r2 > lim) { float s = sqrt(lim / r2); ox *= s; oz *= s; r2 = lim; }
	oy = -sqrt(LROD * LROD - r2);
}

void CheckStrikes(float dt)
{
	float vx = LROD * cos(thx) * thxV;
	float vz = LROD * cos(thz) * thzV;
	float speed = sqrt(vx * vx + vz * vz);
	for (int i = 0; i < NT; i++)
	{
		float tx = RW * cos(tubeAng[i]);
		float tz = RW * sin(tubeAng[i]);
		float dx = tx - ox;
		float dz = tz - oz;
		float dist = sqrt(dx * dx + dz * dz);
		if (dist < STRIKEBAND && cooldn[i] <= 0.0 && dist < prevDist[i] && speed > MINSPEED)
		{
			float amp = 0.12 + clampF(speed / 130.0, 0.0, 1.0) * 0.72;
			StrikeTube(i, amp);
		}
		prevDist[i] = dist;
	}
}

void UpdateRipples(float dt)
{
	for (int i = 0; i < NR; i++)
		if (rlife[i] > 0.0)
		{
			rr[i]    += dt * 72.0;
			rlife[i] -= dt / 1.1;
		}
}

void UpdateStreaks(float dt)
{
	float vx = windX * 150.0;
	for (int i = 0; i < NW; i++)
	{
		wx[i] += vx * dt * (0.6 + wsp[i]);
		wy[i] += sin(tSim * 0.8 + i) * dt * 4.0;
		if (wx[i] >  W + 30.0) wx[i] -= (W + 60.0);
		if (wx[i] < -30.0)     wx[i] += (W + 60.0);
	}
}

// ------------------------------- drawing -----------------------------------
void DrawSky()
{
	int bands = 60;
	for (int i = 0; i < bands; i++)
	{
		float t = i / float(bands - 1);
		int col = lerpCol(rgb3(24, 32, 58), rgb3(92, 70, 96), t);
		g.fillrect(0, i * (H / bands), W, H / bands + 1, col);
	}
	g.Circle(500.0, 348.0, 0.0, 130.0, 1, withAlpha(rgb3(255, 190, 120), 40));
	g.Circle(500.0, 348.0, 0.0,  62.0, 1, withAlpha(rgb3(255, 214, 150), 90));
	g.clear();
	g.M(0, 388); g.L(120, 372); g.L(300, 392); g.L(470, 366); g.L(640, 386);
	g.L(640, H); g.L(0, H); g.close(); g.fin();
	g.rgba32(rgb3(38, 46, 70)); g.fill1();
	g.clear();
	g.M(0, 418); g.L(180, 404); g.L(360, 424); g.L(520, 408); g.L(640, 420);
	g.L(640, H); g.L(0, H); g.close(); g.fin();
	g.rgba32(rgb3(22, 28, 44)); g.fill1();
}

void DrawStreaks()
{
	float d = signF(windX);
	if (d == 0.0) d = 1.0;
	int a = int((0.12 + clampF(fabsF(windX), 0.0, 1.0) * 0.5) * 120.0);
	for (int i = 0; i < NW; i++)
	{
		g.clear();
		g.M(wx[i], wy[i]);
		g.L(wx[i] + d * wln[i], wy[i] - wln[i] * 0.12);
		g.fin();
		g.width(1.2, 1.2);
		g.rgba32(withAlpha(rgb3(191, 208, 224), a));
		g.stroke();
	}
}

// one arc of the suspension hoop (t0..t1 radians around the ring)
void DrawRingArc(float t0, float t1)
{
	int seg = 24;
	g.clear();
	for (int k = 0; k <= seg; k++)
	{
		float t = t0 + (t1 - t0) * k / seg;
		P3 p = project(RW * cos(t), 0.0, RW * sin(t));
		if (k == 0) g.M(p.sx, p.sy); else g.L(p.sx, p.sy);
	}
	g.fin();
	g.width(10.0, 10.0); g.rgba32(rgb3(72, 52, 34)); g.stroke();   // hoop body
	g.width(3.5, 3.5);   g.rgba32(rgb3(142, 106, 72)); g.stroke(); // lit edge
}

void DrawTube(int i)
{
	float a  = tubeAng[i];
	float bx = RW * cos(a);
	float bz = RW * sin(a);
	float wob = ring[i] * sin(ringT[i] * TWO_PI * 2.2 + ringPh[i]) * WOBS;
	float swx = windSmoothX * LEANS + wob * cos(a);   // bottom sway (world)
	float swz = windSmoothZ * LEANS + wob * sin(a);

	P3 rimp = project(bx, 0.0, bz);
	P3 topp = project(bx,       -CORDLEN,               bz);
	P3 botp = project(bx + swx, -CORDLEN - tubeLen[i],  bz + swz);
	float wt = TUBER * topp.sc;
	float wb = TUBER * botp.sc;

	int glow = matGlowC(i);

	// halo behind a ringing tube
	if (ring[i] > 0.01)
	{
		float mxs = (topp.sx + botp.sx) * 0.5;
		float mys = (topp.sy + botp.sy) * 0.5;
		g.Circle(mxs, mys, 0.0, (9.0 + ring[i] * 13.0) * botp.sc, 1,
			withAlpha(glow, int(ring[i] * 42.0)));
	}

	// cord from ring rim + knot
	g.clear(); g.M(rimp.sx, rimp.sy); g.L(topp.sx, topp.sy); g.fin();
	g.width(1.3, 1.3); g.rgba32(rgb3(120, 96, 70)); g.stroke();
	g.Circle(rimp.sx, rimp.sy, 0.0, 2.4, 1, rgb3(60, 44, 30));

	// far tubes dim toward the dusk sky
	int baseBody = matBody(i);
	if (bz > 0.0)
		baseBody = withAlpha(lerpCol(baseBody, rgb3(42, 46, 66), (bz / RW) * 0.4),
			chan(baseBody, 24));
	int body = baseBody;
	if (ring[i] > 0.01)
		body = withAlpha(lerpCol(baseBody, glow, ring[i] * 0.55), chan(baseBody, 24));

	// glow edge
	if (ring[i] > 0.01)
	{
		float gt = wt + 3.0 + ring[i] * 7.0;
		float gb = wb + 3.0 + ring[i] * 7.0;
		g.clear();
		g.M(topp.sx - gt, topp.sy); g.L(topp.sx + gt, topp.sy);
		g.L(botp.sx + gb, botp.sy); g.L(botp.sx - gb, botp.sy); g.close(); g.fin();
		g.rgba32(withAlpha(glow, int(ring[i] * 100.0))); g.fill1();
	}

	// tube body (a perspective quad)
	g.clear();
	g.M(topp.sx - wt, topp.sy); g.L(topp.sx + wt, topp.sy);
	g.L(botp.sx + wb, botp.sy); g.L(botp.sx - wb, botp.sy); g.close(); g.fin();
	g.rgba32(body); g.fill1();

	// highlight stripe (left) + shade (right)
	g.clear();
	g.M(topp.sx - wt * 0.7, topp.sy); g.L(topp.sx - wt * 0.25, topp.sy);
	g.L(botp.sx - wb * 0.25, botp.sy); g.L(botp.sx - wb * 0.7, botp.sy); g.close(); g.fin();
	g.rgba32(withAlpha(0xffffffff, 60)); g.fill1();
	g.clear();
	g.M(topp.sx + wt * 0.45, topp.sy); g.L(topp.sx + wt, topp.sy);
	g.L(botp.sx + wb, botp.sy); g.L(botp.sx + wb * 0.45, botp.sy); g.close(); g.fin();
	g.rgba32(withAlpha(0xff000000, 60)); g.fill1();

	// bamboo joints
	if (gMat == BAMBOO)
	{
		int ncol = darker(baseBody, 0.55);
		for (int n = 0; n < 2; n++)
		{
			float f  = 0.34 + n * 0.32;
			float jx = topp.sx + (botp.sx - topp.sx) * f;
			float jy = topp.sy + (botp.sy - topp.sy) * f;
			float jw = wt + (wb - wt) * f;
			g.clear();
			g.M(jx - jw, jy); g.L(jx + jw, jy);
			g.L(jx + jw, jy + 3.5); g.L(jx - jw, jy + 3.5); g.close(); g.fin();
			g.rgba32(ncol); g.fill1();
		}
	}

	// rounded caps
	g.Circle(topp.sx, topp.sy, 0.0, wt, 1, darker(baseBody, 0.62));
	g.Circle(botp.sx, botp.sy, 0.0, wb, 1, darker(baseBody, 0.6));
}

void DrawStriker()
{
	P3 piv = project(0.0, 0.0, 0.0);
	P3 st  = project(ox, oy, oz);
	g.clear(); g.M(piv.sx, piv.sy); g.L(st.sx, st.sy); g.fin();
	g.width(1.7, 1.7); g.rgba32(rgb3(150, 130, 104)); g.stroke();
	float br = BALLR * st.sc;
	g.Circle(st.sx, st.sy, 0.0, br + 2.0, 1, rgb3(58, 62, 70));
	g.Circle(st.sx, st.sy, 0.0, br, 1, rgb3(176, 184, 196));
	g.Circle(st.sx - br * 0.32, st.sy - br * 0.32, 0.0, br * 0.34, 1, 0xffffffff);
}

void DrawSail()
{
	P3 st = project(ox, oy, oz);
	float f = LSAIL / LROD;
	P3 sl = project(ox * f, oy * f, oz * f);
	g.clear(); g.M(st.sx, st.sy); g.L(sl.sx, sl.sy); g.fin();
	g.width(1.5, 1.5); g.rgba32(rgb3(150, 130, 104)); g.stroke();
	float sr = SAILR * sl.sc;
	g.Circle(sl.sx, sl.sy, 0.0, sr, 1, rgb3(150, 96, 70));
	g.Circle(sl.sx, sl.sy, 0.0, sr * 0.82, 1, rgb3(184, 126, 92));
	g.Circle(sl.sx - sr * 0.32, sl.sy - sr * 0.32, 0.0, sr * 0.28, 1, withAlpha(0xffffffff, 80));
}

void DrawCeilingCord()
{
	P3 anc = project(0.0, CEIL, 0.0);
	P3 top = project(0.0, 0.0, 0.0);
	g.clear(); g.M(anc.sx, anc.sy); g.L(top.sx, top.sy); g.fin();
	g.width(2.0, 2.0); g.rgba32(rgb3(92, 72, 52)); g.stroke();
	g.Circle(anc.sx, anc.sy, 0.0, 4.0, 1, rgb3(52, 36, 24));
}

void DrawItem(int ty, int idx)
{
	if (ty == 0) DrawTube(idx);
	else if (ty == 1) DrawStriker();
	else DrawSail();
}

void DrawRipples()
{
	for (int i = 0; i < NR; i++)
		if (rlife[i] > 0.0)
			g.Circle(rx[i], ry[i], rr[i] - 2.5, rr[i], 1,
				withAlpha(rcol[i], int(rlife[i] * 95.0)));
}

void DrawHUD()
{
	stext("wind chimes  --  a breeze-driven set of tuned tubes", 12, 10, 0xffffffff);

	int nActive = 0;
	for (int i = 0; i < NV; i++) if (voices[i].active) nActive++;
	char st[80];
	snprintf(st, 80, "%s     breeze %i%%     ringing %i",
		MAT_NAME[gMat], int(baseWind * 100.0), nActive);
	stext(st, 12, 24, 0xff9fd0ff);

	g.fillrect(12, 452, 200, 10, rgb3(20, 28, 42));
	g.fillrect(12, 452, int(baseWind * 200.0), 10, rgb3(80, 168, 224));
	float wm = sqrt(windX * windX + windZ * windZ);
	int gustx2 = 12 + int(clampF(wm, 0.0, 1.0) * 200.0);
	g.fillrect(gustx2 - 1, 449, 2, 16, 0xffffffff);

	stext("SPACE gust   -/+ breeze   M / 1-4 material   R gust   C calm   click tube   Q quit",
		12, 468, 0xff6c7d8c);
}

// ------------------------------- setup -------------------------------------
void InitTube(int i, float freq, float angDeg, float lenWorld, int base, int glow)
{
	float a = angDeg * M_PI / 180.0;
	tubeAng[i]  = a;
	tubeFreq[i] = freq;
	tubeLen[i]  = lenWorld;
	tubePan[i]  = 0.5 + 0.5 * cos(a);
	metalBase[i] = base;
	metalGlow[i] = glow;
	ring[i] = 0.0; ringT[i] = 0.0; ringPh[i] = 0.0; cooldn[i] = 0.0;
	prevDist[i] = 999.0;
}

int main()
{
	cosP = cos(PITCHA);
	sinP = sin(PITCHA);

	// materials:            ---- partial ratios ----   ---- amplitudes ----   ---- decay muls ----  bTau  trA   trN    oct
	SetMat(METAL,  1.00, 2.76, 5.40, 8.93,   1.0, 0.50, 0.30, 0.15,   1.00, 0.50, 0.30, 0.20,  1.15, 0.45, 300.0, 1.00);
	SetMat(GLASS,  1.00, 2.00, 3.01, 4.20,   1.0, 0.60, 0.40, 0.25,   1.00, 0.70, 0.50, 0.35,  0.80, 0.28, 200.0, 2.00);
	SetMat(BAMBOO, 1.00, 2.65, 4.80, 6.20,   1.0, 0.35, 0.12, 0.05,   1.00, 0.35, 0.20, 0.15,  0.34, 0.75, 520.0, 0.75);
	SetMat(BELL,   0.50, 1.00, 1.20, 2.50,   0.5, 1.00, 0.50, 0.35,   1.60, 1.00, 0.60, 0.40,  1.40, 0.50, 260.0, 1.00);

	// tubes 60 deg apart around the ring; angle sets screen x (cos) and depth (sin)
	InitTube(0, 220.00,   0.0, 108.0, rgb3(184, 115, 51),  0xffffb060);  // copper (right)
	InitTube(1, 261.63,  60.0,  98.0, rgb3(201, 162, 39),  0xffffe27a);  // brass  (front-right)
	InitTube(2, 293.66, 120.0,  90.0, rgb3(207, 212, 218), 0xffffffff);  // silver (front-left)
	InitTube(3, 329.63, 180.0,  82.0, rgb3(212, 175, 55),  0xffffe98a);  // gold   (left)
	InitTube(4, 392.00, 240.0,  72.0, rgb3(143, 179, 199), 0xffcfeeff);  // steel  (back-left)
	InitTube(5, 440.00, 300.0,  62.0, rgb3(214, 160, 138), 0xffffd0c0);  // rose   (back-right)

	for (int i = 0; i < NV; i++)
		voices[i].active = 0;

	playSample = 0;
	delayLen = int(0.26 * SR);
	delayPos = 0;
	delayBuf = (float*)malloc(sizeof(float) * 2 * delayLen);
	for (int i = 0; i < 2 * delayLen; i++)
		delayBuf[i] = 0.0;

	for (int i = 0; i < NR; i++) rlife[i] = 0.0;
	for (int i = 0; i < NW; i++)
	{
		wx[i]  = frand() * W;
		wy[i]  = 70.0 + frand() * 320.0;
		wln[i] = 6.0 + frand() * 14.0;
		wsp[i] = frand();
	}

	printf("wind_chimes: SPACE gust  -/+ breeze  M/1-4 material (metal/glass/bamboo/bell)  C calm  click tube  Q quit\n");
	fflush(stdout);

	Tprev = Time();

	while (true)
	{
		snd.Poll();

		double T = Time();
		float dt = T - Tprev;
		if (dt > 0.05) dt = 0.05;
		if (dt < 0.0)  dt = 0.0;
		Tprev = T;
		tSim += dt;

		int key;
		int press;
		while (GetKeyEvent(key, press))
			if (press > 0)
			{
				if (key == ' ')
				{
					float a = frand() * TWO_PI;
					float m = 0.8 + 0.6 * frand();
					gustx += cos(a) * m; gustz += sin(a) * m;
				}
				else if (key == '-' || key == '_')
					baseWind = clampF(baseWind - 0.05, 0.0, 1.0);
				else if (key == '=' || key == '+')
					baseWind = clampF(baseWind + 0.05, 0.0, 1.0);
				else if (key == 'r' || key == 'R')
				{
					float a = frand() * TWO_PI;
					gustx += cos(a) * 1.4; gustz += sin(a) * 1.4;
				}
				else if (key == 'c' || key == 'C')
					baseWind = 0.10;
				else if (key == 'm' || key == 'M')
					gMat = (gMat + 1) % NMAT;
				else if (key >= '1' && key <= '4')
					gMat = key - '1';
			}
		if (KeyPressed('q') || KeyPressed('Q') || KeyPressed(1000))
			break;

		// mouse: click the nearest tube to ring it by hand
		GetMouseState(mx, my, mb);
		if ((mb & 1) && (prevmb & 1) == 0)
		{
			int hit = -1;
			float hitZ = 1000000000.0;
			for (int i = 0; i < NT; i++)
			{
				float bx = RW * cos(tubeAng[i]);
				float bz = RW * sin(tubeAng[i]);
				P3 tp = project(bx, -CORDLEN, bz);
				P3 bp = project(bx, -CORDLEN - tubeLen[i], bz);
				float ww = TUBER * tp.sc + 3.0;
				float minx = fminF(tp.sx, bp.sx) - ww;
				float maxx = fmaxF(tp.sx, bp.sx) + ww;
				if (mx >= minx && mx <= maxx && my >= tp.sy - 4.0 && my <= bp.sy + 4.0)
					if (tp.z < hitZ) { hitZ = tp.z; hit = i; }
			}
			if (hit >= 0) { cooldn[hit] = 0.0; StrikeTube(hit, 0.6); }
		}
		prevmb = mb;

		// simulation
		UpdateWind(dt);
		UpdatePendulum(dt);
		CheckStrikes(dt);
		UpdateRipples(dt);
		UpdateStreaks(dt);
		for (int i = 0; i < NT; i++)
		{
			ring[i] *= (1.0 - clampF(dt * 0.75, 0.0, 0.9));
			if (ring[i] < 0.002) ring[i] = 0.0;
			ringT[i] += dt;
			cooldn[i] -= dt;
			if (cooldn[i] < 0.0) cooldn[i] = 0.0;
		}

		// audio: keep the buffer topped up, capped so a flurry can't freeze the UI
		int gen = 0;
		while (snd.snd_bufhealth() < BUF_LOW && gen < 48)
		{
			for (int i = 0; i < CHUNK; i++)
				GenSample();
			gen++;
		}

		// draw: sky, far hoop, depth-sorted tubes+striker+sail, near hoop, fx
		DrawSky();
		DrawStreaks();
		DrawCeilingCord();
		DrawRingArc(0.0, M_PI);        // far half of the hoop (behind)

		for (int i = 0; i < NT; i++)
		{
			float bx = RW * cos(tubeAng[i]);
			float bz = RW * sin(tubeAng[i]);
			itemType[i] = 0; itemIdx[i] = i;
			P3 dp = project(bx, -CORDLEN - tubeLen[i] * 0.5, bz);
			itemDepth[i] = dp.z;
		}
		itemType[NT] = 1; itemIdx[NT] = 0;
		P3 dstr = project(ox, oy, oz);
		itemDepth[NT] = dstr.z;
		float fs = LSAIL / LROD;
		itemType[NT + 1] = 2; itemIdx[NT + 1] = 0;
		P3 dsl = project(ox * fs, oy * fs, oz * fs);
		itemDepth[NT + 1] = dsl.z;

		for (int a = 0; a < NITEMS - 1; a++)         // bubble sort far -> near
			for (int b = a + 1; b < NITEMS; b++)
				if (itemDepth[b] > itemDepth[a])
				{
					float td = itemDepth[a]; itemDepth[a] = itemDepth[b]; itemDepth[b] = td;
					int tt = itemType[a]; itemType[a] = itemType[b]; itemType[b] = tt;
					int ti = itemIdx[a]; itemIdx[a] = itemIdx[b]; itemIdx[b] = ti;
				}
		for (int k = 0; k < NITEMS; k++)
			DrawItem(itemType[k], itemIdx[k]);

		DrawRingArc(M_PI, TWO_PI);     // near half of the hoop (in front)
		DrawRipples();
		DrawHUD();

		Present();
	}
	return 0;
}
