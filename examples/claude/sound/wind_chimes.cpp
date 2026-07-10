#include <stdio.h>
#include "sound.h"        // intrinsic: CSound audio API (snd.*)
#include "graphics.h"     // intrinsic: the Graph drawing API (g.*)
#include "../../ws.h"     // windowing: Present(), Time() (real file at examples/ws.h)

// wind_chimes -- a little simulator of a set of tuned metal wind-chime tubes
// stirred by a turbulent breeze, with 2D animation and synthesized sound.
//
// A central striker hangs from the beam on a swinging rod, with a wind sail
// below it that the breeze pushes around.  As the striker swings it crosses
// the plane of the surrounding tubes and rings whichever ones it passes,
// harder the faster it is moving -- so a gentle breeze gives the odd single
// note and a strong gust sets off a flurry.
//
// SOUND -- each tube is modelled as a struck free-free metal tube: an additive
// stack of INHARMONIC partials (ratios ~ 1 : 2.76 : 5.40, the bending modes of
// a tube) each with its own exponential decay, plus a short metallic contact
// transient.  To keep the per-sample audio cheap enough for the interpreter,
// every partial is a coupled-form ("magic circle") oscillator: a unit rotation
// (cos/sin computed once at the strike) advanced by two multiplies + two adds
// per sample and scaled by a per-sample decay factor -- so the inner loop has
// NO sin()/pow() at all.  A soft stereo ping-pong delay adds air.  The tubes
// are tuned to an A-minor pentatonic so any combination is consonant.
//
// Controls:  SPACE = gust    - / + = less / more breeze    click a tube to ring
//            R = random gust   C = calm    Q / Esc = quit
//
// Scinc idioms (see track_editor.cpp / fractal_melody.cpp): a graphical sound
// loop must call snd.Poll() itself; no comma-initialised declarations; plain
// char*; no rand() (hand-rolled LCG); no exp() (use pow once, recurrence in the
// hot loop); struct array-members are fine; fill polygons with g.fill1().

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
#define NPART   3              // inharmonic partials per struck tube
#define NR      24             // ripple-ring pool
#define NW      46             // wind streaks

// ------------------------------- globals -----------------------------------
Graph  g;
CSound snd;

int    playSample;             // audio clock (samples)

// tube tuning + layout (filled in main)
float  tubeFreq[NT];
float  tubeX[NT];              // screen x of each tube
float  tubeLen[NT];            // visual length
float  tubePan[NT];            // 0=left .. 1=right
int    metalBase[NT];          // body colour
int    metalGlow[NT];          // ring / glow colour

// per-tube animation state
float  ring[NT];               // remaining ring energy 0..1 (glow + wobble)
float  ringT[NT];              // time since last strike (for wobble phase)
float  ringPh[NT];             // random wobble phase
float  cooldn[NT];             // strike debounce timer

// chime partial voice model (additive, coupled-form oscillators)
float  partRatio[NPART]  = { 1.0,  2.76, 5.40 };
float  partAmp[NPART]    = { 1.0,  0.5,  0.30 };
float  partTauMul[NPART] = { 1.0,  0.5,  0.30 };   // higher partials decay faster
float  baseTau = 1.15;         // fundamental e-fold time (s)

struct Chime
{
	int   active;
	int   t0;
	float pan;
	float strike;               // strike strength (transient scaling)
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

// striker pendulum
float  theta;
float  omega;
float  prevBallX;
#define PIVX  320.0
#define PIVY  56.0
#define BEAMY 52.0
#define LROD  176.0            // pivot -> striker
#define LSAIL 250.0           // pivot -> wind sail
#define TUBETOP 96.0

// wind
float  baseWind    = 0.45;     // steady breeze 0..1 (user adjustable)
float  gustImpulse = 0.0;      // transient signed gust
float  gustTimer   = 2.0;
float  windSigned  = 0.0;      // current signed wind
float  windSmooth  = 0.0;      // smoothed, for the visual lean

// ripple rings
float  rx[NR];
float  ry[NR];
float  rr[NR];
float  rlife[NR];
int    rcol[NR];

// wind streaks
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
	v.active = 1;
	v.t0     = playSample;
	v.pan    = pan;
	v.strike = amp;
	for (int k = 0; k < NPART; k++)
	{
		float fk = freq * partRatio[k];
		float w  = TWO_PI * fk / SR;
		v.cw[k]  = cos(w);
		v.sw[k]  = sin(w);
		v.cs[k]  = amp * partAmp[k];    // start vector (magnitude = partial amp)
		v.sn[k]  = 0.0;
		float tau = baseTau * partTauMul[k];
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
	if (age < 300)                      // ~6ms metallic contact transient
	{
		float te = 1.0 - age / 300.0;
		out += noiseSample() * te * te * v.strike * 0.45;
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
	cooldn[i] = 0.05;
	float lean = windSmooth * 13.0;
	SpawnRipple(tubeX[i] + lean * 0.5, TUBETOP + tubeLen[i] * 0.5, metalGlow[i]);
}

void UpdateWind(float dt)
{
	// smooth turbulent signed breeze from a few detuned sines
	float g1 = sin(tSim * 0.37);
	float g2 = sin(tSim * 0.91 + 1.3);
	float g3 = sin(tSim * 2.13 + 2.7);
	float turb = g1 * 0.5 + g2 * 0.32 + g3 * 0.18;

	// occasional random gusts, more frequent in stronger wind
	gustTimer -= dt;
	if (gustTimer <= 0.0)
	{
		float dir = signF(turb);
		if (dir == 0.0) dir = 1.0;
		gustImpulse += (0.4 + 0.6 * frand()) * dir;
		gustTimer = 1.4 + 4.5 * frand() * (1.3 - baseWind);
	}
	gustImpulse *= (1.0 - clampF(dt * 1.6, 0.0, 0.9));

	windSigned = baseWind * turb + gustImpulse;
	windSmooth += (windSigned - windSmooth) * clampF(dt * 6.0, 0.0, 1.0);
}

void UpdatePendulum(float dt)
{
	// damped driven pendulum, semi-implicit Euler in two substeps
	int sub = 2;
	float h = dt / sub;
	for (int s = 0; s < sub; s++)
	{
		float torque = windSigned * 6.0 - 9.0 * sin(theta) - 0.9 * omega;
		omega += torque * h;
		theta += omega * h;
		if (theta >  1.4) { theta =  1.4; omega *= -0.3; }
		if (theta < -1.4) { theta = -1.4; omega *= -0.3; }
	}
}

void CheckStrikes()
{
	float ballX = PIVX + LROD * sin(theta);
	float ballVX = LROD * cos(theta) * omega;    // px/s
	for (int i = 0; i < NT; i++)
	{
		float a = prevBallX - tubeX[i];
		float b = ballX - tubeX[i];
		if (a * b <= 0.0 && fabsF(ballVX) > 28.0)   // striker crossed this tube
		{
			float amp = 0.12 + clampF(fabsF(ballVX) / 700.0, 0.0, 1.0) * 0.72;
			StrikeTube(i, amp);
		}
	}
	prevBallX = ballX;
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
	float vx = windSigned * 150.0;
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
	// setting-sun glow near the horizon
	g.Circle(500.0, 348.0, 0.0, 130.0, 1, withAlpha(rgb3(255, 190, 120), 40));
	g.Circle(500.0, 348.0, 0.0,  62.0, 1, withAlpha(rgb3(255, 214, 150), 90));
	// two hill silhouettes for depth
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
	float d = signF(windSigned);
	if (d == 0.0) d = 1.0;
	int a = int((0.12 + clampF(fabsF(windSigned), 0.0, 1.0) * 0.5) * 120.0);
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

void DrawBeam()
{
	g.fillrect(104, 44, 432, 14, rgb3(74, 52, 34));     // wooden beam
	g.fillrect(104, 44, 432, 4,  rgb3(104, 76, 52));    // lit top edge
	g.fillrect(104, 56, 432, 2,  rgb3(38, 26, 18));     // shadow
	g.Circle(112.0, 51.0, 0.0, 7.0, 1, rgb3(52, 36, 24));
	g.Circle(528.0, 51.0, 0.0, 7.0, 1, rgb3(52, 36, 24));
}

void DrawTube(int i)
{
	float lean = windSmooth * 13.0 * (1.0 + i * 0.03)
	           + ring[i] * sin(ringT[i] * TWO_PI * 2.2 + ringPh[i]) * 11.0;
	float topX = tubeX[i];
	float y0   = TUBETOP;
	float y1   = TUBETOP + tubeLen[i];
	float hw   = 9.0;
	float midX = topX + lean * 0.5;
	float midY = (y0 + y1) * 0.5;

	// halo behind a ringing tube
	if (ring[i] > 0.01)
		g.Circle(midX, midY, 0.0, 20.0 + ring[i] * 28.0, 1,
			withAlpha(metalGlow[i], int(ring[i] * 55.0)));

	// suspension string
	g.clear(); g.M(topX, BEAMY + 4.0); g.L(topX, y0); g.fin();
	g.width(1.3, 1.3); g.rgba32(rgb3(120, 96, 70)); g.stroke();

	// soft glow edge
	if (ring[i] > 0.01)
	{
		float gw = hw + 5.0 + ring[i] * 9.0;
		g.clear();
		g.M(topX - gw, y0); g.L(topX + gw, y0);
		g.L(topX + gw + lean, y1); g.L(topX - gw + lean, y1);
		g.close(); g.fin();
		g.rgba32(withAlpha(metalGlow[i], int(ring[i] * 110.0)));
		g.fill1();
	}

	// tube body (a sheared parallelogram = leaning tube)
	int body = metalBase[i];
	if (ring[i] > 0.01) body = lerpCol(metalBase[i], metalGlow[i], ring[i] * 0.55);
	g.clear();
	g.M(topX - hw, y0); g.L(topX + hw, y0);
	g.L(topX + hw + lean, y1); g.L(topX - hw + lean, y1);
	g.close(); g.fin();
	g.rgba32(body); g.fill1();

	// specular highlight stripe (left) + shade (right)
	g.clear();
	g.M(topX - hw + 3.0, y0); g.L(topX - hw + 6.5, y0);
	g.L(topX - hw + 6.5 + lean, y1); g.L(topX - hw + 3.0 + lean, y1);
	g.close(); g.fin();
	g.rgba32(withAlpha(0xffffffff, 70)); g.fill1();
	g.clear();
	g.M(topX + hw - 4.0, y0); g.L(topX + hw, y0);
	g.L(topX + hw + lean, y1); g.L(topX + hw - 4.0 + lean, y1);
	g.close(); g.fin();
	g.rgba32(withAlpha(0xff000000, 70)); g.fill1();

	// rounded caps
	g.Circle(topX, y0, 0.0, hw, 1, darker(metalBase[i], 0.65));
	g.Circle(topX + lean, y1, 0.0, hw * 0.94, 1, darker(metalBase[i], 0.6));
}

void DrawClapper()
{
	float sinT = sin(theta);
	float cosT = cos(theta);
	float ballX = PIVX + LROD * sinT;
	float ballY = PIVY + LROD * cosT;
	float sailX = PIVX + LSAIL * sinT;
	float sailY = PIVY + LSAIL * cosT;

	// rod / strings
	g.clear();
	g.M(PIVX, PIVY); g.L(ballX, ballY); g.L(sailX, sailY);
	g.fin();
	g.width(1.6, 1.6); g.rgba32(rgb3(150, 130, 104)); g.stroke();

	// pivot hook
	g.Circle(PIVX, PIVY, 0.0, 4.0, 1, rgb3(40, 30, 22));

	// wind sail (the breeze-catcher)
	g.Circle(sailX, sailY, 0.0, 26.0, 1, rgb3(150, 96, 70));
	g.Circle(sailX, sailY, 0.0, 21.0, 1, rgb3(184, 126, 92));
	g.Circle(sailX - 5.0, sailY - 5.0, 0.0, 7.0, 1, withAlpha(0xffffffff, 60));

	// metal striker
	g.Circle(ballX, ballY, 0.0, 12.0, 1, rgb3(70, 74, 82));
	g.Circle(ballX, ballY, 0.0, 10.0, 1, rgb3(176, 184, 196));
	g.Circle(ballX - 3.0, ballY - 3.0, 0.0, 3.5, 1, 0xffffffff);
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
	snprintf(st, 80, "breeze %i%%     ringing %i", int(baseWind * 100.0), nActive);
	stext(st, 12, 24, 0xff9fd0ff);

	// wind strength meter
	g.fillrect(12, 452, 200, 10, rgb3(20, 28, 42));
	int bw = int(baseWind * 200.0);
	g.fillrect(12, 452, bw, 10, rgb3(80, 168, 224));
	int gustx = 12 + int(clampF(0.5 + windSigned * 0.5, 0.0, 1.0) * 200.0);
	g.fillrect(gustx - 1, 449, 2, 16, 0xffffffff);   // live gust marker

	stext("SPACE gust    -/+ breeze    R random gust    C calm    click tube    Q quit",
		12, 468, 0xff6c7d8c);
}

// ------------------------------- setup -------------------------------------
void InitTube(int i, float freq, float x, float len, float pan, int base, int glow)
{
	tubeFreq[i]  = freq;
	tubeX[i]     = x;
	tubeLen[i]   = len;
	tubePan[i]   = pan;
	metalBase[i] = base;
	metalGlow[i] = glow;
	ring[i] = 0.0; ringT[i] = 0.0; ringPh[i] = 0.0; cooldn[i] = 0.0;
}

int main()
{
	// tuning: A minor pentatonic (A C D E G A), longest/lowest on the left.
	// x spread across the beam; length shrinks as pitch rises.
	InitTube(0, 220.00, 175.0, 205.0, 0.16, rgb3(184, 115, 51),  0xffffb060);  // copper
	InitTube(1, 261.63, 233.0, 184.0, 0.30, rgb3(201, 162, 39),  0xffffe27a);  // brass
	InitTube(2, 293.66, 291.0, 171.0, 0.44, rgb3(207, 212, 218), 0xffffffff);  // silver
	InitTube(3, 329.63, 349.0, 160.0, 0.56, rgb3(212, 175, 55),  0xffffe98a);  // gold
	InitTube(4, 392.00, 407.0, 143.0, 0.70, rgb3(143, 179, 199), 0xffcfeeff);  // steel
	InitTube(5, 440.00, 465.0, 133.0, 0.84, rgb3(214, 160, 138), 0xffffd0c0);  // rose

	for (int i = 0; i < NV; i++)
		voices[i].active = 0;

	playSample = 0;
	delayLen = int(0.26 * SR);
	delayPos = 0;
	delayBuf = (float*)malloc(sizeof(float) * 2 * delayLen);
	for (int i = 0; i < 2 * delayLen; i++)
		delayBuf[i] = 0.0;

	theta = 0.12; omega = 0.0;
	prevBallX = PIVX + LROD * sin(theta);

	for (int i = 0; i < NR; i++) rlife[i] = 0.0;
	for (int i = 0; i < NW; i++)
	{
		wx[i]  = frand() * W;
		wy[i]  = 70.0 + frand() * 320.0;
		wln[i] = 6.0 + frand() * 14.0;
		wsp[i] = frand();
	}

	printf("wind_chimes: SPACE gust  -/+ breeze  R random gust  C calm  click tube  Q quit\n");
	fflush(stdout);

	Tprev = Time();

	while (true)
	{
		snd.Poll();

		// --- timing ---
		double T = Time();
		float dt = T - Tprev;
		if (dt > 0.05) dt = 0.05;
		if (dt < 0.0)  dt = 0.0;
		Tprev = T;
		tSim += dt;

		// --- discrete keys ---
		int key;
		int press;
		while (GetKeyEvent(key, press))
			if (press > 0)
			{
				if (key == ' ')
					gustImpulse += (frand() < 0.5 ? -1.0 : 1.0) * (0.8 + 0.5 * frand());
				else if (key == '-' || key == '_')
					baseWind = clampF(baseWind - 0.05, 0.0, 1.0);
				else if (key == '=' || key == '+')
					baseWind = clampF(baseWind + 0.05, 0.0, 1.0);
				else if (key == 'r' || key == 'R')
					gustImpulse += signF(windSigned + 0.001) * (1.0 + 0.6 * frand());
				else if (key == 'c' || key == 'C')
					baseWind = 0.10;
			}
		if (KeyPressed('q') || KeyPressed('Q') || KeyPressed(1000))
			break;

		// --- mouse: click a tube to ring it by hand ---
		GetMouseState(mx, my, mb);
		if ((mb & 1) && (prevmb & 1) == 0)
			for (int i = 0; i < NT; i++)
				if (mx >= tubeX[i] - 12.0 && mx <= tubeX[i] + 12.0
					&& my >= TUBETOP && my <= TUBETOP + tubeLen[i])
				{
					cooldn[i] = 0.0;
					StrikeTube(i, 0.6);
				}
		prevmb = mb;

		// --- simulation ---
		UpdateWind(dt);
		UpdatePendulum(dt);
		CheckStrikes();
		UpdateRipples(dt);
		UpdateStreaks(dt);
		for (int i = 0; i < NT; i++)
		{
			ring[i]  *= (1.0 - clampF(dt * 0.75, 0.0, 0.9));
			if (ring[i] < 0.002) ring[i] = 0.0;
			ringT[i] += dt;
			cooldn[i] -= dt;
			if (cooldn[i] < 0.0) cooldn[i] = 0.0;
		}

		// --- audio: keep the buffer topped up, capped so a flurry can't
		//     spin the loop and freeze the UI (it just underruns briefly) ---
		int gen = 0;
		while (snd.snd_bufhealth() < BUF_LOW && gen < 48)
		{
			for (int i = 0; i < CHUNK; i++)
				GenSample();
			gen++;
		}

		// --- draw (back to front) ---
		DrawSky();
		DrawStreaks();
		DrawBeam();
		for (int i = 0; i < NT; i++)
			DrawTube(i);
		DrawRipples();
		DrawClapper();
		DrawHUD();

		Present();
	}
	return 0;
}
