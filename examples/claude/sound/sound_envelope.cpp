#include "sound.h"                    // intrinsic: CSound audio API
#include "graphics.h"                  // intrinsic: Graph drawing API
#define G_SCREEN_WIDTH  1800
#define G_SCREEN_HEIGHT 1000
#define G_SCREEN_SCALE  1
#define G_SCREEN_MODE   1
#define NO_GREEDY_EVENT                // we read GetScincEvent ourselves
#include "../../ws.h"

// sound_envelope -- a Scinc port of Lowland/SoundEnvelope (C++).
//
// An instrument is a stack of oscillators. Each oscillator has a hand-drawn
// single-cycle waveform (keyframes joined by none/linear/scurve/spline/sin
// interpolation, or "mixed" = smooth per-cycle noise), a delay-ADSR envelope,
// a frequency multiplier/offset and a volume. Play it from the computer
// keyboard, record and replay what you played, save it as JSON.
//
//   Piano keys    Q 2 W 3 E R 5 T 6 Y 7 U | I 9 O 0 P  (upper part = +1 octave)
//   Waveform      drag points; Ctrl+click or right-click empty space adds one,
//                 right-click a point or Del removes the selected one
//   Oscillators   click a row to edit it, MUTE / SOLO buttons in the row,
//                 PgUp/PgDn next/previous, Ins adds one, M mute, S solo
//   F5            start / stop recording       F7 play recording (restart)
//   F2            save to <file>.new           Esc / close window: quit
//
//   Scinc sound_envelope.cpp [instr.json]
//
// Differences from the C++ original are listed at the end of this file.

#define M_PI    3.14159265358979
#define SRATE   48000
#define DT      (1.0/48000.0)
#define CHUNK   512
#define BLK     64              // envelope/LFO are evaluated per 64 samples
#define BUF_LOW 3200

#define MAXOSC  16
#define MAXPTS  64
#define WT      1024            // wavetable size per oscillator
#define WTM     1023
#define WTS     1025            // WT + guard sample
#define NNOISE  4096
#define NMASK   4095
#define MAXN    64              // simultaneous notes
#define MAXREC  2048            // recorded notes
#define SCOPE   2048
#define JMAX    262144

#define IM_NONE   0
#define IM_LINEAR 1
#define IM_SCURVE 2
#define IM_SPLINE 3
#define IM_SIN    4
#define IM_MIXED  5
#define IM_TOTAL  6

#define MAXFMUL 10.0
#define MAXFADD 2500.0

// screen layout (same as the original 1800x1000 window)
#define SCRW 1800
#define SCRH 1000
#define EPX  1390               // "Envelope" panel
#define EPY  0
#define EPW  410
#define EPH  460
#define WPX  790                // "Waveform" panel
#define WPY  0
#define WPW  600
#define WPH  760
#define EDX  795                // waveform editor control
#define EDY  20
#define EDW  590
#define EDH  590

char* IM_NAME[6] = { "none", "linear", "scurve", "spline", "sin", "mixed" };

float stdFreq[12] = { 261.626, 277.183, 293.665, 311.127, 329.628, 349.228,
                      369.994, 391.995, 415.305, 440.000, 466.164, 493.883 };

Graph g;
CSound snd;

// --------------------------------------------------------------- oscillators
struct Osc
{
	float d;      // delay
	float a;      // attack
	float dc;     // decay
	float s;      // sustain time
	float r;      // release
	float sl;     // sustain level
	float vol;
	float fmul;
	float fadd;
	int sus;      // sustainable (affects the envelope drawing, as in the original)
	int interp;
	int mute;
	int npts;
	float t0;     // keyframe time range covered by the wavetable
	float dt;
};

Osc oscs[MAXOSC];
int nosc = 0;
float kfT[1024];                // MAXOSC*MAXPTS keyframes, sorted by time
float kfV[1024];
float* wt = 0;                  // MAXOSC*WTS wavetable samples
float noiseTab[NNOISE];

int curanim = 0;
int hoveranim = -1;
int hovermute = 0;
int hoversolo = 0;
int solo = -1;

// UI copy of the current oscillator's points (unsorted, like PointList)
float plX[MAXPTS];
float plY[MAXPTS];
int npl = 0;
int plHover = -1;
int plSel = -1;
int pointschanged = 0;
int actualize = 1;

// --------------------------------------------------------------- notes
struct Note
{
	float t0;
	float freq;
	float l;
	float ml;
	float vol;
	int active;
	int held;     // sustain mode: still held down, length unknown
};

Note notes[MAXN];

float sndT = 0.0;
int octave = 0;
float allVolume = 0.5;
float length = 0.0;
int sustain = 0;

float pnT0[MAXREC];
float pnL[MAXREC];
int pnNote[MAXREC];
int pnOct[MAXREC];
float pnVol[MAXREC];
int nPlay = 0;
float curPlayTime = 0.0;
int curPlayNote = 0;
int playing = 0;
int recording = 0;
float recordStart = 0.0;

float mixBuf[CHUNK];
float scope[SCOPE];
int scopePos = 0;
float mixProcTime = 0.0;
float mixAll = 0.0;

// piano keyboard mapping (original SKeyCode switch)
int kmCode[17] = { 'q', '2', 'w', '3', 'e', 'r', '5', 't', '6', 'y', '7', 'u', 'i', '9', 'o', '0', 'p' };
int kmNote[17] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0, 1, 2, 3, 4 };
int kmW[17]    = { 0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6, 7, -1, 8, -1, 9 };
int kmB[17]    = { -1, 1, -1, 2, -1, -1, 4, -1, 5, -1, 6, -1, -1, 8, -1, 9, -1 };
int kmOct[17]  = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1 };
int keyDown[17];
int keyNote[17];
float keyFreq[17];
int relPending[17];

// GUI: scroll bars and check boxes
#define NSB 13
#define SB_DELAY   0
#define SB_ATTACK  1
#define SB_DECAY   2
#define SB_SUSTIME 3
#define SB_SUSLVL  4
#define SB_RELEASE 5
#define SB_FMUL    6
#define SB_FADD    7
#define SB_VOL     8
#define SB_INTERP  9
#define SB_OCTAVE  10
#define SB_ALLVOL  11
#define SB_LENGTH  12
float sbPos[NSB];
int sbX[NSB];
int sbY[NSB];
int sbW[NSB];
int sbDrag = -1;
#define CB_SUSTAINABLE 0
#define CB_SUSTAIN     1
int cbOn[2];
int cbX[2];
int cbY[2];

int ctrlDown = 0;
int edCapture = 0;
int lastMX = 0;
int lastMY = 0;

char filename[256];
char message[256];
float messageTime = -100.0;

// --------------------------------------------------------------- helpers
int rndState = 20190302;
int irand()
{
	rndState = rndState * 1103515245 + 12345;
	return (rndState >> 16) & 0x7fff;
}

int floorI(float x)
{
	int i = int(x);
	if (x < i) i--;
	return i;
}

float absF(float x) { if (x < 0.0) return -x; return x; }

int streq(char* a, char* b)
{
	int i = 0;
	while (true)
	{
		int ca = a[i];
		int cb = b[i];
		if (ca != cb) return 0;
		if (ca == 0) return 1;
		i++;
	}
	return 0;
}

void strcopy(char* dst, char* src, int max)
{
	int i = 0;
	while (i < max - 1)
	{
		int c = src[i];
		if (c == 0) break;
		dst[i] = c;
		i++;
	}
	dst[i] = 0;
}

int inRect(int x, int y, int w, int h, int px, int py)
{
	if (px < x) return 0;
	if (py < y) return 0;
	if (px >= x + w) return 0;
	if (py >= y + h) return 0;
	return 1;
}

float freqByNote(int note, int oct)
{
	float f = stdFreq[note];
	int i = 0;
	while (i < oct) { f = f * 2.0; i++; }
	while (i > oct) { f = f * 0.5; i--; }
	return f;
}

// --------------------------------------------------------------- envelope
float ADSR(float time, Osc& e, float l)
{
	if (time < 0.0) return 0.0;
	float xp = e.d;
	if (time < xp) return 0.0;
	float xp0 = xp;
	xp = xp + e.a;
	if (time < xp) return (time - xp0) / e.a;
	xp0 = xp;
	xp = xp + e.dc;
	if (time < xp)
	{
		float t = (time - xp0) / e.dc;
		return 1.0 - (1.0 - e.sl) * t;
	}
	xp = xp + e.s;
	if (xp < l) xp = l;
	if (time < xp) return e.sl;
	xp0 = xp;
	xp = xp + e.r;
	if (time < xp)
	{
		float t = (time - xp0) / e.r;
		return e.sl * (1.0 - t);
	}
	return 0.0;
}

float MaxLen(float l)
{
	float maxl = l;
	for (int i = 0; i < nosc; i++)
	{
		Osc& o = oscs[i];
		float tot = o.d + o.a + o.dc + o.s + o.r;
		if (tot > maxl) maxl = tot;
		// the original added R only for sustainable oscillators, which cut
		// the release of the others off with a click after a long hold
		if (l + o.r > maxl) maxl = l + o.r;
	}
	return maxl;
}

// --------------------------------------------------------------- waveform
// InterpolateInside() of Cumulus/SVG/Interpolate.h, loop mode (EM_REPEAT)
float EvalInside(int oi, float t)
{
	Osc& o = oscs[oi];
	int n = o.npts;
	int b = oi * MAXPTS;
	int k = -1;
	for (int i = 0; i < n - 1; i++)
	{
		if (t >= kfT[b + i])
			if (t <= kfT[b + i + 1]) { k = i; break; }
	}
	if (k == -1)
	{
		if (t <= kfT[b]) return kfV[b];
		if (t >= kfT[b + n - 1]) return kfV[b + n - 1];
		return 0.0;
	}
	int mode = o.interp;
	if (mode == IM_MIXED) mode = IM_LINEAR;
	if (mode == IM_SPLINE)
	{
		if (n != 2)
		{
			int n0 = k - 1;
			int n1 = k;
			int n2 = k + 1;
			int n3 = k + 2;
			if (n0 < 0) n0 = n - 2;
			if (n3 > n - 1) n3 = 1;
			float x0 = kfV[b + n0];
			float x1 = kfV[b + n1];
			float x2 = kfV[b + n2];
			float x3 = kfV[b + n3];
			float t0 = kfT[b + n0];
			float t1 = kfT[b + n1];
			float t2 = kfT[b + n2];
			float t3 = kfT[b + n3];
			float dt12 = t1 - t2;
			float dt20 = t2 - t0;
			float dt31 = t3 - t1;
			float wrap = kfT[b + 1] - kfT[b] + kfT[b + n - 1] - kfT[b + n - 2];
			if (n2 <= n0) dt20 = wrap;
			if (n3 <= n1) dt31 = wrap;
			dt12 = absF(dt12);
			dt20 = absF(dt20);
			dt31 = absF(dt31);
			if (dt12 < 0.000001) return x1;
			float c1 = x1;
			float c2 = x2;
			if (dt20 > 0.000001) c1 = x1 + (x2 - x0) / (3.0 * dt20) * dt12;
			if (dt31 > 0.000001) c2 = x2 + (x1 - x3) / (3.0 * dt31) * dt12;
			float it = (t - t1) / dt12;
			// Bezier3Calculator::Instant(x1, c1, c2, x2, it)
			float d1 = (c1 - x1) * 3.0;
			float d2 = (c1 - c2) * 3.0;
			return (((d2 - x1 + x2) * it - d1 - d2) * it + d1) * it + x1;
		}
		mode = IM_LINEAR;
	}
	float ta = kfT[b + k];
	float tb = kfT[b + k + 1];
	float va = kfV[b + k];
	float vb = kfV[b + k + 1];
	if (tb - ta < 0.000001) return va;
	float u = (t - ta) / (tb - ta);
	if (mode == IM_NONE) return va;
	if (mode == IM_SCURVE) u = u * u * (3.0 - 2.0 * u);
	if (mode == IM_SIN) u = 0.5 * (1.0 + sin((u - 0.5) * M_PI));
	return va + u * (vb - va);
}

// Interpolate1() with EM_REPEAT, rendered into a wavetable over one period
void BuildTable(int oi)
{
	Osc& o = oscs[oi];
	int base = oi * WTS;
	int b = oi * MAXPTS;
	int n = o.npts;
	o.t0 = 0.0;
	o.dt = 1.0;
	if (n < 2)
	{
		float c = 0.0;
		if (n == 1) c = kfV[b];
		for (int i = 0; i < WTS; i++) wt[base + i] = c;
		return;
	}
	float t0 = kfT[b];
	float dt = kfT[b + n - 1] - t0;
	if (dt < 0.000001)
	{
		for (int i = 0; i < WTS; i++) wt[base + i] = kfV[b];
		return;
	}
	o.t0 = t0;
	o.dt = dt;
	for (int i = 0; i < WT; i++)
		wt[base + i] = EvalInside(oi, t0 + dt * i / WT);
	wt[base + WT] = wt[base];
}

// waveform value at keyframe time t (for drawing)
float WaveAt(int oi, float t)
{
	Osc& o = oscs[oi];
	float p = (t - o.t0) / o.dt;
	p = p - floorI(p);
	p = p * WT;
	int i = int(p);
	if (i > WTM) i = WTM;
	float f = p - i;
	int k = oi * WTS + i;
	float v0 = wt[k];
	return v0 + (wt[k + 1] - v0) * f;
}

void SetOscDefaults(int oi)
{
	Osc& o = oscs[oi];
	o.d = 0.0;
	o.a = 0.05;
	o.dc = 0.1;
	o.s = 0.2;
	o.r = 0.2;
	o.sl = 0.8;
	o.vol = 1.0;
	o.fmul = 1.0;
	o.fadd = 0.0;
	o.sus = 0;
	o.interp = IM_SIN;
	o.mute = 0;
	o.npts = 3;
	int b = oi * MAXPTS;
	kfT[b] = 0.0;     kfV[b] = -1.0;
	kfT[b + 1] = 0.5; kfV[b + 1] = 1.0;
	kfT[b + 2] = 1.0; kfV[b + 2] = -1.0;
	BuildTable(oi);
}

// --------------------------------------------------------------- mixing
void MixOsc(int oi, Note& nt, float T0)
{
	Osc& o = oscs[oi];
	float freq = nt.freq * o.fmul + o.fadd;
	float vol = o.vol * nt.vol;
	float len = nt.l;            // as in tune.h: every oscillator holds
	int base = oi * WTS;
	float invdt = 1.0 / o.dt;
	float t0 = o.t0;
	int noise = 0;
	if (o.interp == IM_MIXED) noise = 1;
	for (int b0 = 0; b0 < CHUNK; b0 += BLK)
	{
		float Ta = T0 + b0 * DT;
		float Tb = Ta + BLK * DT;
		float aA = ADSR(Ta, o, len) * vol;
		float aB = ADSR(Tb, o, len) * vol;
		if (aA <= 0.0)
			if (aB <= 0.0) continue;
		// vibrato of the original: sin(T*30)*(T+0.5)*0.02 cycles
		float pa = Ta * freq + sin(Ta * 30.0) * (Ta + 0.5) * 0.02;
		float pb = Tb * freq + sin(Tb * 30.0) * (Tb + 0.5) * 0.02;
		float a = aA;
		float da = (aB - aA) / BLK;
		int jend = b0 + BLK;
		if (noise == 0)
		{
			float ph = (pa - t0) * invdt;
			ph = (ph - floorI(ph)) * WT;
			float dph = (pb - pa) * invdt * WT / BLK;
			if (dph < 0.0) ph = ph + WT * (int(-dph * BLK / WT) + 2);
			for (int j = b0; j < jend; j++)
			{
				int i = int(ph);
				int k = base + (i & WTM);
				float v0 = wt[k];
				mixBuf[j] += a * (v0 + (wt[k + 1] - v0) * (ph - i));
				ph += dph;
				a += da;
			}
		}
		else
		{
			// sn0(): a smooth bump per cycle with a random height
			float ph = pa;
			float dph = (pb - pa) / BLK;
			for (int j = b0; j < jend; j++)
			{
				int i = int(ph);
				if (ph < i) i--;
				float x0 = ph - i;
				float x1 = 1.0 - x0;
				float h0 = noiseTab[i & NMASK];
				float h1 = noiseTab[(i + 1) & NMASK];
				mixBuf[j] += a * (h0 * x1 * x1 * x0 - h1 * x1 * x0 * x0) * 6.0;
				ph += dph;
				a += da;
			}
		}
	}
}

int AddNote(float freq, float l, float vol, int held)
{
	int slot = -1;
	for (int i = 0; i < MAXN; i++)
		if (notes[i].active == 0) { slot = i; break; }
	if (slot == -1)
	{
		float oldest = 1e30;
		for (int i = 0; i < MAXN; i++)
			if (notes[i].held == 0)
				if (notes[i].t0 < oldest) { oldest = notes[i].t0; slot = i; }
	}
	if (slot == -1) return -1;
	Note& n = notes[slot];
	n.t0 = sndT;
	n.freq = freq;
	n.l = l;
	n.ml = MaxLen(l);
	n.vol = vol;
	n.active = 1;
	n.held = held;
	return slot;
}

void GenChunk()
{
	float tStart = Time();
	if (playing != 0)
	{
		curPlayTime += CHUNK * DT;
		while (curPlayNote < nPlay)
		{
			if (pnT0[curPlayNote] >= curPlayTime) break;
			int p = curPlayNote;
			AddNote(freqByNote(pnNote[p], pnOct[p]), pnL[p], pnVol[p], 0);
			curPlayNote++;
		}
	}
	for (int j = 0; j < CHUNK; j++) mixBuf[j] = 0.0;
	for (int i = 0; i < MAXN; i++)
	{
		Note& nt = notes[i];
		if (nt.active == 0) continue;
		float T0 = sndT - nt.t0;
		for (int a = 0; a < nosc; a++)
		{
			int play = 0;
			if (solo == -1)
			{
				if (oscs[a].mute == 0) play = 1;
			}
			else if (a == solo) play = 1;
			if (play != 0) MixOsc(a, nt, T0);
		}
	}
	for (int j = 0; j < CHUNK; j++)
	{
		float x = mixBuf[j];
		float v = x * 0.5;
		if (v > 1.0) v = 1.0;
		if (v < -1.0) v = -1.0;
		snd.snd_out(v, v);
		scope[scopePos] = x;
		scopePos = (scopePos + 1) & (SCOPE - 1);
	}
	sndT += CHUNK * DT;
	for (int i = 0; i < MAXN; i++)
	{
		Note& nt = notes[i];
		if (nt.active != 0)
			if (nt.held == 0)
				if (sndT - nt.t0 > nt.ml) nt.active = 0;
	}
	mixProcTime = Time() - tStart;
	mixAll = CHUNK * DT;
}

void FillAudio()
{
	snd.Poll();
	int gen = 0;
	while (snd.snd_bufhealth() < BUF_LOW && gen < 16)
	{
		GenChunk();
		gen++;
	}
}

// --------------------------------------------------------------- recording
void Record(int note, int oct, float l, float t0, float vol)
{
	if (nPlay >= MAXREC) return;
	pnNote[nPlay] = note;
	pnOct[nPlay] = oct;
	pnL[nPlay] = l;
	pnT0[nPlay] = t0;
	pnVol[nPlay] = vol;
	nPlay++;
}

void SortRecording()
{
	for (int i = 1; i < nPlay; i++)
	{
		int j = i;
		while (j > 0)
		{
			if (pnT0[j - 1] <= pnT0[j]) break;
			float tf = pnT0[j]; pnT0[j] = pnT0[j - 1]; pnT0[j - 1] = tf;
			tf = pnL[j]; pnL[j] = pnL[j - 1]; pnL[j - 1] = tf;
			tf = pnVol[j]; pnVol[j] = pnVol[j - 1]; pnVol[j - 1] = tf;
			int ti = pnNote[j]; pnNote[j] = pnNote[j - 1]; pnNote[j - 1] = ti;
			ti = pnOct[j]; pnOct[j] = pnOct[j - 1]; pnOct[j - 1] = ti;
			j--;
		}
	}
}

// --------------------------------------------------------------- piano keys
void PianoDown(int k)
{
	if (relPending[k] != 0) { relPending[k] = 0; return; }   // auto-repeat
	if (keyDown[k] != 0) return;
	keyDown[k] = 1;
	int note = kmNote[k];
	int o = octave + kmOct[k];
	float f = freqByNote(note, o);
	keyFreq[k] = f;
	if (sustain == 0)
	{
		keyNote[k] = -1;
		AddNote(f, length, allVolume, 0);
		if (recording != 0) Record(note, o, length, sndT - recordStart, allVolume);
	}
	else
	{
		keyNote[k] = AddNote(f, 10.0, allVolume, 1);
	}
}

void PianoRelease(int k)
{
	keyDown[k] = 0;
	int ni = keyNote[k];
	keyNote[k] = -1;
	if (ni < 0) return;
	Note& n = notes[ni];
	if (n.held == 0) return;
	n.held = 0;
	n.l = sndT - n.t0;
	n.ml = MaxLen(n.l);
	if (recording != 0)
		Record(kmNote[k], kmOct[k] + octave, n.l, n.t0 - recordStart, n.vol);
}

// --------------------------------------------------------------- JSON
#define CTX_OTHER  0
#define CTX_ROOT   1
#define CTX_OSCARR 2
#define CTX_OSC    3
#define CTX_ADSR   4
#define CTX_POINTS 5
#define CTX_PAIR   6

char* jb = 0;
int jn = 0;
int jp = 0;
int jctx[32];
int jdepth = 0;
char jkey[32];
char jstr[32];

void JReadString()
{
	jp++;
	int i = 0;
	while (jp < jn)
	{
		int c = jb[jp];
		if (c == '"') break;
		if (c == '\\') { jp++; c = jb[jp]; }
		if (i < 31) { jstr[i] = c; i++; }
		jp++;
	}
	jstr[i] = 0;
	jp++;
}

float JReadNumber()
{
	float sign = 1.0;
	if (jb[jp] == '-') { sign = -1.0; jp++; }
	float v = 0.0;
	while (jp < jn)
	{
		int c = jb[jp];
		if (c < '0') break;
		if (c > '9') break;
		v = v * 10.0 + (c - '0') * 1.0;
		jp++;
	}
	if (jb[jp] == '.')
	{
		jp++;
		float sc = 0.1;
		while (jp < jn)
		{
			int c = jb[jp];
			if (c < '0') break;
			if (c > '9') break;
			v = v + (c - '0') * sc;
			sc = sc * 0.1;
			jp++;
		}
	}
	int c = jb[jp];
	if (c == 'e' || c == 'E')
	{
		jp++;
		int es = 1;
		if (jb[jp] == '-') { es = -1; jp++; }
		else if (jb[jp] == '+') jp++;
		int e = 0;
		while (jp < jn)
		{
			int d = jb[jp];
			if (d < '0') break;
			if (d > '9') break;
			e = e * 10 + d - '0';
			jp++;
		}
		while (e > 0) { if (es > 0) v = v * 10.0; else v = v * 0.1; e--; }
	}
	return v * sign;
}

int LoadInstrument(char* fname)
{
	FILE* f = fopen(fname, "rb");
	if (f == 0) return 0;
	jn = 0;
	while (true)
	{
		int c = fgetc(f);
		if (c < 0) break;
		if (jn < JMAX) { jb[jn] = c; jn++; }
	}
	fclose(f);
	jb[jn] = 0;
	jb[jn + 1] = 0;

	int count = 0;
	int cur = -1;
	int pairIdx = 0;
	jdepth = 0;
	jkey[0] = 0;
	jp = 0;
	while (jp < jn)
	{
		int c = jb[jp];
		int top = CTX_OTHER;
		if (jdepth > 0) top = jctx[jdepth - 1];
		if (c == '"')
		{
			JReadString();
			while (jp < jn)
			{
				int w = jb[jp];
				if (w != ' ' && w != '\t' && w != '\n' && w != '\r') break;
				jp++;
			}
			if (jb[jp] == ':') { strcopy(jkey, jstr, 32); jp++; }
			else
			{
				if (top == CTX_OSC)
					if (streq(jkey, "interp"))
						for (int m = 0; m < IM_TOTAL; m++)
							if (streq(jstr, IM_NAME[m])) oscs[cur].interp = m;
				jkey[0] = 0;
			}
			continue;
		}
		if (c == '{' || c == '[')
		{
			int ctx = CTX_OTHER;
			if (jdepth == 0)
			{
				if (c == '{') ctx = CTX_ROOT;
			}
			else if (c == '{')
			{
				if (top == CTX_OSCARR)
				{
					if (count < MAXOSC)
					{
						cur = count;
						count++;
						Osc& o = oscs[cur];
						// LoadOscs() defaults
						o.d = 0.0; o.a = 0.02; o.dc = 0.05; o.s = 0.1; o.r = 0.3; o.sl = 0.8;
						o.vol = 1.0; o.fmul = 1.0; o.fadd = 0.0; o.sus = 0;
						o.interp = IM_NONE; o.mute = 0; o.npts = 0;
						ctx = CTX_OSC;
					}
				}
				else if (top == CTX_OSC)
				{
					if (streq(jkey, "ADSR")) ctx = CTX_ADSR;
				}
			}
			else
			{
				if (top == CTX_ROOT)
				{
					if (streq(jkey, "oscillators")) ctx = CTX_OSCARR;
				}
				else if (top == CTX_OSC)
				{
					if (streq(jkey, "points")) ctx = CTX_POINTS;
				}
				else if (top == CTX_POINTS)
				{
					Osc& o = oscs[cur];
					if (o.npts < MAXPTS)
					{
						kfT[cur * MAXPTS + o.npts] = 0.0;
						kfV[cur * MAXPTS + o.npts] = 0.0;
						o.npts++;
						ctx = CTX_PAIR;
						pairIdx = 0;
					}
				}
			}
			if (jdepth < 32) { jctx[jdepth] = ctx; jdepth++; }
			jkey[0] = 0;
			jp++;
			continue;
		}
		if (c == '}' || c == ']')
		{
			if (jdepth > 0) jdepth--;
			jp++;
			continue;
		}
		if (c == '-' || (c >= '0' && c <= '9'))
		{
			float v = JReadNumber();
			if (top == CTX_PAIR)
			{
				int pi = cur * MAXPTS + oscs[cur].npts - 1;
				if (pairIdx == 0) kfT[pi] = v;
				if (pairIdx == 1) kfV[pi] = v;
				pairIdx++;
			}
			else if (top == CTX_ADSR)
			{
				Osc& o = oscs[cur];
				if (streq(jkey, "_")) o.d = v;
				if (streq(jkey, "A")) o.a = v;
				if (streq(jkey, "D")) o.dc = v;
				if (streq(jkey, "S")) o.s = v;
				if (streq(jkey, "R")) o.r = v;
			}
			else if (top == CTX_OSC)
			{
				Osc& o = oscs[cur];
				if (streq(jkey, "SL")) o.sl = v;
				if (streq(jkey, "vol")) o.vol = v;
				if (streq(jkey, "freqMul")) o.fmul = v;
				if (streq(jkey, "freqAdd")) o.fadd = v;
			}
			jkey[0] = 0;
			continue;
		}
		if (c >= 'a' && c <= 'z')
		{
			int isTrue = 0;
			if (c == 't') isTrue = 1;
			while (jp < jn)
			{
				int w = jb[jp];
				if (w < 'a' || w > 'z') break;
				jp++;
			}
			if (top == CTX_OSC)
				if (streq(jkey, "sustainable")) oscs[cur].sus = isTrue;
			jkey[0] = 0;
			continue;
		}
		jp++;
	}
	if (count == 0) return 0;
	nosc = count;
	for (int i = 0; i < nosc; i++) BuildTable(i);
	return 1;
}

void SaveInstrument()
{
	char fn[300];
	snprintf(fn, 299, "%s.new", filename);
	FILE* f = fopen(fn, "wb");
	if (f == 0)
	{
		snprintf(message, 255, "Could not write %s", fn);
		messageTime = Time();
		return;
	}
	char s[256];
	fputs("{\n\t\"oscillators\":[\n", f);
	for (int i = 0; i < nosc; i++)
	{
		Osc& o = oscs[i];
		if (i != 0) fputs("\t\t,\n", f);
		fputs("\t\t{\n", f);
		snprintf(s, 255, "\t\t\t\"ADSR\":{\"_\":%f,\"A\":%f,\"D\":%f,\"S\":%f,\"R\":%f},\n", o.d, o.a, o.dc, o.s, o.r);
		fputs(s, f);
		snprintf(s, 255, "\t\t\t\"SL\":%f,\n", o.sl);
		fputs(s, f);
		snprintf(s, 255, "\t\t\t\"vol\":%f,\n", o.vol);
		fputs(s, f);
		if (o.sus != 0) fputs("\t\t\t\"sustainable\":true,\n", f);
		else fputs("\t\t\t\"sustainable\":false,\n", f);
		snprintf(s, 255, "\t\t\t\"freqMul\":%f,\n", o.fmul);
		fputs(s, f);
		snprintf(s, 255, "\t\t\t\"freqAdd\":%f,\n", o.fadd);
		fputs(s, f);
		snprintf(s, 255, "\t\t\t\"interp\":\"%s\",\n", IM_NAME[o.interp]);
		fputs(s, f);
		fputs("\t\t\t\"points\":[", f);
		int b = i * MAXPTS;
		for (int j = 0; j < o.npts; j++)
		{
			if (j != 0) fputs(",", f);
			snprintf(s, 255, "[%f,%f]", kfT[b + j], kfV[b + j]);
			fputs(s, f);
		}
		fputs("]\n", f);
		fputs("\t\t}\n", f);
	}
	fputs("\t]\n}\n", f);
	fclose(f);
	snprintf(message, 255, "Saved %s", fn);
	messageTime = Time();
}

// --------------------------------------------------------------- GUI state
void LoadUIFromOsc()
{
	Osc& o = oscs[curanim];
	int b = curanim * MAXPTS;
	npl = o.npts;
	for (int i = 0; i < npl; i++) { plX[i] = kfT[b + i]; plY[i] = kfV[b + i]; }
	plHover = -1;
	plSel = -1;
	sbPos[SB_DELAY] = o.d;
	sbPos[SB_ATTACK] = o.a;
	sbPos[SB_DECAY] = o.dc;
	sbPos[SB_SUSTIME] = o.s;
	sbPos[SB_RELEASE] = o.r;
	sbPos[SB_SUSLVL] = o.sl;
	sbPos[SB_VOL] = o.vol;
	sbPos[SB_FMUL] = o.fmul / MAXFMUL;
	sbPos[SB_FADD] = o.fadd / MAXFADD / 2.0 + 0.5;
	sbPos[SB_INTERP] = o.interp * 1.0 / IM_TOTAL;
	cbOn[CB_SUSTAINABLE] = o.sus;
	for (int k = 0; k < SB_OCTAVE; k++)
	{
		if (sbPos[k] < 0.0) sbPos[k] = 0.0;
		if (sbPos[k] > 1.0) sbPos[k] = 1.0;
	}
}

// pl (UI points) -> sorted keyframes, ends pinned to t=0 / t=1
void PointsToOsc()
{
	Osc& o = oscs[curanim];
	int b = curanim * MAXPTS;
	o.npts = npl;
	for (int i = 0; i < npl; i++) { kfT[b + i] = plX[i]; kfV[b + i] = plY[i]; }
	for (int i = 1; i < npl; i++)
	{
		int j = i;
		while (j > 0)
		{
			if (kfT[b + j - 1] <= kfT[b + j]) break;
			float tf = kfT[b + j]; kfT[b + j] = kfT[b + j - 1]; kfT[b + j - 1] = tf;
			tf = kfV[b + j]; kfV[b + j] = kfV[b + j - 1]; kfV[b + j - 1] = tf;
			j--;
		}
	}
	if (npl > 0)
	{
		kfT[b + npl - 1] = 1.0;
		kfV[b + npl - 1] = kfV[b];
		kfT[b] = 0.0;
	}
	BuildTable(curanim);
}

// once per frame, as in the original main loop
void UpdateState()
{
	if (actualize != 0)
	{
		LoadUIFromOsc();
		actualize = 0;
	}
	else if (pointschanged != 0)
	{
		PointsToOsc();
		pointschanged = 0;
	}
	Osc& o = oscs[curanim];
	o.d = sbPos[SB_DELAY];
	o.a = sbPos[SB_ATTACK];
	o.dc = sbPos[SB_DECAY];
	o.s = sbPos[SB_SUSTIME];
	o.sl = sbPos[SB_SUSLVL];
	o.r = sbPos[SB_RELEASE];
	o.vol = sbPos[SB_VOL];
	o.fmul = sbPos[SB_FMUL] * MAXFMUL;
	o.fadd = (sbPos[SB_FADD] - 0.5) * 2.0 * MAXFADD;
	o.sus = cbOn[CB_SUSTAINABLE];
	int im = int(sbPos[SB_INTERP] * IM_TOTAL);
	if (im >= IM_TOTAL) im = IM_TOTAL - 1;
	if (im != o.interp)
	{
		o.interp = im;
		BuildTable(curanim);
	}
	octave = floorI(sbPos[SB_OCTAVE] * 6.0 - 3.0 + 0.5);
	allVolume = sbPos[SB_ALLVOL];
	sustain = cbOn[CB_SUSTAIN];
	length = sbPos[SB_LENGTH] * 5.0;
}

void LayoutGUI()
{
	int y = EPY + 20;
	for (int k = 0; k < 6; k++)
	{
		sbX[k] = EPX + 5; sbY[k] = y + 17; sbW[k] = 400;
		y += 34;
	}
	cbX[CB_SUSTAINABLE] = EPX + 5; cbY[CB_SUSTAINABLE] = y;
	y += 17;
	for (int k = 6; k < 10; k++)
	{
		sbX[k] = EPX + 5; sbY[k] = y + 17; sbW[k] = 400;
		y += 34;
	}
	y = EDY + EDH + 3;
	for (int k = 10; k < 13; k++)
	{
		sbX[k] = WPX + 5; sbY[k] = y + 17; sbW[k] = 400;
		y += 34;
	}
	cbX[CB_SUSTAIN] = WPX + 5; cbY[CB_SUSTAIN] = y;
}

void SliderLabel(int k, char* s)
{
	Osc& o = oscs[curanim];
	if (k == SB_DELAY) snprintf(s, 127, "Delay Time     %.3f s", sbPos[k]);
	if (k == SB_ATTACK) snprintf(s, 127, "Attack Time    %.3f s", sbPos[k]);
	if (k == SB_DECAY) snprintf(s, 127, "Decay Time     %.3f s", sbPos[k]);
	if (k == SB_SUSTIME) snprintf(s, 127, "Sustain Time   %.3f s", sbPos[k]);
	if (k == SB_SUSLVL) snprintf(s, 127, "Sustain Level  %.3f", sbPos[k]);
	if (k == SB_RELEASE) snprintf(s, 127, "Release Time   %.3f s", sbPos[k]);
	if (k == SB_FMUL) snprintf(s, 127, "mul freq %f", o.fmul);
	if (k == SB_FADD) snprintf(s, 127, "add freq %f", o.fadd);
	if (k == SB_VOL) snprintf(s, 127, "Volume %.3f", sbPos[k]);
	if (k == SB_INTERP) snprintf(s, 127, "Interpolation: %s", IM_NAME[o.interp]);
	if (k == SB_OCTAVE) snprintf(s, 127, "octave %i", octave);
	if (k == SB_ALLVOL) snprintf(s, 127, "Volume %.3f", sbPos[k]);
	if (k == SB_LENGTH) snprintf(s, 127, "Length %f", length);
}

void SetSlider(int k, int mx)
{
	float p = (mx - sbX[k] - 8) * 1.0 / (sbW[k] - 16);
	if (p < 0.0) p = 0.0;
	if (p > 1.0) p = 1.0;
	sbPos[k] = p;
}

// --------------------------------------------------------------- waveform editor input
float PtSX(int i) { return EDX + 5 + (EDW - 10) * plX[i]; }
float PtSY(int i) { return EDY + 5 + (EDH - 10) * (1.0 - plY[i]) / 2.0; }

void EdHover(int mx, int my)
{
	plHover = -1;
	for (int i = 0; i < npl; i++)
	{
		float dx = mx - PtSX(i);
		float dy = my - PtSY(i);
		if (dx * dx + dy * dy < 25.0) plHover = i;
	}
}

void EdAddPoint(int mx, int my)
{
	if (npl >= MAXPTS) return;
	plX[npl] = (mx - EDX - 5) * 1.0 / (EDW - 10);
	plY[npl] = -(my - EDY - 5) * 1.0 / (EDH - 10) * 2.0 + 1.0;
	plHover = npl;
	npl++;
	pointschanged = 1;
}

void EdDeleteSelected()
{
	if (plSel < 0) return;
	for (int i = plSel; i < npl - 1; i++) { plX[i] = plX[i + 1]; plY[i] = plY[i + 1]; }
	npl--;
	plSel = -1;
	plHover = -1;
	pointschanged = 1;
}

void EdClamp()
{
	for (int i = 0; i < npl; i++)
	{
		if (plX[i] < 0.0) plX[i] = 0.0;
		if (plX[i] > 1.0) plX[i] = 1.0;
		if (plY[i] < -1.0) plY[i] = -1.0;
		if (plY[i] > 1.0) plY[i] = 1.0;
	}
	pointschanged = 1;
}

void OscListHover(int mx, int my)
{
	hoveranim = -1;
	hovermute = 0;
	hoversolo = 0;
	if (my > 100)
	{
		hoveranim = (my - 100) / 50;
		if (hoveranim > nosc - 1) hoveranim = -1;
		if (hoveranim != -1)
		{
			int lx = mx - 10;
			int ly = my - 100 - 50 * hoveranim;
			hovermute = inRect(0, 40, 24, 10, lx, ly);
			hoversolo = inRect(25, 40, 24, 10, lx, ly);
		}
	}
}

int InPanels(int x, int y)
{
	if (inRect(EPX, EPY, EPW, EPH, x, y)) return 1;
	if (inRect(WPX, WPY, WPW, WPH, x, y)) return 1;
	return 0;
}

void MouseDown(int x, int y, int z, int right)
{
	int ctrl = ctrlDown;
	if ((z & 8) != 0) ctrl = 1;
	if (InPanels(x, y))
	{
		if (right == 0)
		{
			for (int k = 0; k < NSB; k++)
				if (inRect(sbX[k], sbY[k], sbW[k], 14, x, y)) { sbDrag = k; SetSlider(k, x); return; }
			for (int k = 0; k < 2; k++)
				if (inRect(cbX[k], cbY[k], 140, 14, x, y)) { cbOn[k] = 1 - cbOn[k]; return; }
		}
		if (inRect(EDX, EDY, EDW, EDH, x, y))
		{
			EdHover(x, y);
			if (right != 0)
			{
				if (plHover == -1) EdAddPoint(x, y);
				else { plSel = plHover; EdDeleteSelected(); }
				return;
			}
			if (ctrl != 0)
				if (plHover == -1) EdAddPoint(x, y);
			plSel = plHover;
			edCapture = 1;
		}
		return;
	}
	if (right != 0) return;
	OscListHover(x, y);
	if (hoveranim != -1)
	{
		curanim = hoveranim;
		if (hovermute != 0) oscs[curanim].mute = 1 - oscs[curanim].mute;
		if (hoversolo != 0)
		{
			if (solo != curanim) solo = curanim;
			else solo = -1;
		}
		actualize = 1;
	}
}

void MouseMove(int x, int y)
{
	int dx = x - lastMX;
	int dy = y - lastMY;
	if (sbDrag >= 0) SetSlider(sbDrag, x);
	if (edCapture != 0)
		if (plSel != -1)
		{
			plX[plSel] += dx * 1.0 / (EDW - 10);
			plY[plSel] -= dy * 2.0 / (EDH - 10);
			pointschanged = 1;
		}
	EdHover(x, y);
	if (InPanels(x, y) == 0) OscListHover(x, y);
	else hoveranim = -1;
}

void KeyDown(int code)
{
	for (int k = 0; k < 17; k++)
		if (kmCode[k] == code) { PianoDown(k); return; }
	if (code == 3000 || code == 3001) ctrlDown = 1;
	if (code == KEYCODE_F7)
	{
		SortRecording();
		recording = 0;
		playing = 1;
		curPlayNote = 0;
		curPlayTime = 0.0;
	}
	if (code == KEYCODE_F5)
	{
		if (recording != 0)
		{
			SortRecording();
			recording = 0;
		}
		else
		{
			recordStart = sndT;
			recording = 1;
			playing = 0;
			nPlay = 0;
		}
	}
	if (code == KEYCODE_F2) SaveInstrument();
	if (code == 's')
	{
		if (solo == -1) solo = curanim;
		else solo = -1;
	}
	if (code == 'm') oscs[curanim].mute = 1 - oscs[curanim].mute;
	if (code == KEYCODE_PGUP)
	{
		curanim++;
		if (curanim > nosc - 1) curanim = nosc - 1;
		actualize = 1;
	}
	if (code == KEYCODE_PGDN)
	{
		curanim--;
		if (curanim < 0) curanim = 0;
		actualize = 1;
	}
	if (code == KEYCODE_INS)
	{
		if (nosc < MAXOSC)
		{
			SetOscDefaults(nosc);
			curanim = nosc;
			nosc++;
			actualize = 1;
		}
	}
	if (code == KEYCODE_DEL) EdDeleteSelected();
	if (code == 1000) exit(0);
}

void KeyUp(int code)
{
	for (int k = 0; k < 17; k++)
		if (kmCode[k] == code) { relPending[k] = 1; return; }
	if (code == 3000 || code == 3001) ctrlDown = 0;
}

void HandleEvents()
{
	SScincEvent ev;
	while (GetScincEvent(ev))
	{
		if (ev.type == 'KBDN') KeyDown(ev.x);
		else if (ev.type == 'KBUP') KeyUp(ev.x);
		else if (ev.type == 'MLDN') { MouseDown(ev.x, ev.y, ev.z, 0); lastMX = ev.x; lastMY = ev.y; }
		else if (ev.type == 'MRDN') { MouseDown(ev.x, ev.y, ev.z, 1); lastMX = ev.x; lastMY = ev.y; }
		else if (ev.type == 'MMOV') { MouseMove(ev.x, ev.y); lastMX = ev.x; lastMY = ev.y; }
		else if (ev.type == 'MLUP')
		{
			sbDrag = -1;
			if (edCapture != 0) { EdClamp(); edCapture = 0; }
		}
	}
	// a release followed by a press in the same batch is X auto-repeat
	for (int k = 0; k < 17; k++)
		if (relPending[k] != 0) { relPending[k] = 0; PianoRelease(k); }
}

// --------------------------------------------------------------- drawing
void LineC(float x0, float y0, float x1, float y1, int col)
{
	g.clear();
	g.M(x0, y0);
	g.L(x1, y1);
	g.fin();
	g.rgba32(col);
	g.width(1, 2);
	g.stroke();
}

void RectO(int x, int y, int w, int h, int col)
{
	g.fillrect(x, y, w, 1, col);
	g.fillrect(x, y + h - 1, w, 1, col);
	g.fillrect(x, y, 1, h, col);
	g.fillrect(x + w - 1, y, 1, h, col);
}

void DrawOscRow(int oi, int X, int Y, float W, float H, int selected, int hover, int isSolo)
{
	Osc& e = oscs[oi];
	float L = (e.d + e.a + e.dc + e.s + e.r) * W;
	if (selected != 0)
	{
		g.fillrect(X, Y, int(L) + 50, 1, 0xff004060);
		g.fillrect(X, Y, 1, int(H), 0xff004060);
		g.fillrect(X, Y + int(H), int(L) + 50, 1, 0xff004060);
	}
	if (hover != 0) g.fillrect(X, Y, 1, int(H), 0xffffffff);
	g.fillrect(X, Y, 33, 33, 0xff002040);
	RectO(X - 1, Y - 1, 35, 35, 0xff000000);
	g.fillrect(X, Y + 16, 33, 1, 0xff004060);
	g.clear();
	for (int i = 0; i <= 64; i++)
	{
		float px = X + i * 0.5;
		float py = Y + 16 - WaveAt(oi, i / 64.0) * 16.0;
		if (i == 0) g.M(px, py);
		else g.L(px, py);
	}
	g.fin();
	g.rgba32(0xff00ff00);
	g.width(1, 2);
	g.stroke();

	if (hovermute != 0)
		if (hover != 0) g.fillrect(X, Y + 40, 24, 10, 0xff608090);
	if (hoversolo != 0)
		if (hover != 0) g.fillrect(X + 25, Y + 40, 24, 10, 0xff608090);
	int col = 0xff404040;
	if (e.mute != 0) col = 0xffffffff;
	stext("MUTE", X, Y + int(H) - 10, col);
	col = 0xff404040;
	if (isSolo != 0) col = 0xffffffff;
	stext("SOLO", X + 25, Y + int(H) - 10, col);

	X += 50;
	float vol = e.vol;
	float xA = X + W * e.d;
	float xD = xA + W * e.a;
	float xS = xD + W * e.dc;
	float xp1 = e.d + e.a + e.dc + e.s;
	if (e.sus != 0)
		if (length > xp1) xp1 = length;
	float xR = X + W * xp1;
	float xE = xR + W * e.r;
	float yTop = Y + H - H * vol;
	float ySus = Y + H - e.sl * H * vol;
	float yBot = Y + H;
	// filled envelope (the original hatches it with dotted lines)
	g.clear();
	g.M(xA, yBot);
	g.L(xD, yTop);
	g.L(xS, ySus);
	g.L(xR, ySus);
	g.L(xE, yBot);
	g.close();
	g.fin();
	if (selected != 0) g.rgba32(0x60ffffff);
	else g.rgba32(0x40000000);
	g.fill1();
	g.fillrect(X, Y, 1, int(H), 0xff404040);
	g.fillrect(X, Y, int(L), 1, 0xff404040);
	g.fillrect(int(xD), Y, 1, int(H), 0xff404040);
	g.fillrect(int(xS), Y, 1, int(H), 0xff404040);
	g.fillrect(int(xR), Y, 1, int(H), 0xff404040);
	g.clear();
	g.M(xA, yBot);
	g.L(xD, yTop);
	g.L(xS, ySus);
	g.L(xR, ySus);
	g.L(xE, yBot);
	g.fin();
	g.rgba32(0xff000000);
	g.width(1, 2);
	g.stroke();
}

void DrawPanel(int x, int y, int w, int h, char* title)
{
	g.fillrect(x, y, w, h, 0xff5a5a5a);
	g.fillrect(x, y, w, 16, 0xff303848);
	RectO(x, y, w, h, 0xff202020);
	stext(title, x + 6, y + 2, 0xffffffff);
}

void DrawSlider(int k)
{
	char s[128];
	SliderLabel(k, s);
	stext(s, sbX[k], sbY[k] - 16, 0xffffffff);
	g.fillrect(sbX[k], sbY[k], sbW[k], 14, 0xff303030);
	RectO(sbX[k], sbY[k], sbW[k], 14, 0xff202020);
	int tx = sbX[k] + int(sbPos[k] * (sbW[k] - 16));
	int col = 0xffb0b0b0;
	if (sbDrag == k) col = 0xffe0e0ff;
	g.fillrect(tx, sbY[k] + 1, 16, 12, col);
}

void DrawCheck(int k, char* label)
{
	g.fillrect(cbX[k], cbY[k] + 1, 12, 12, 0xff303030);
	RectO(cbX[k], cbY[k] + 1, 12, 12, 0xffb0b0b0);
	if (cbOn[k] != 0) g.fillrect(cbX[k] + 3, cbY[k] + 4, 6, 6, 0xffffff60);
	stext(label, cbX[k] + 18, cbY[k] + 1, 0xffffffff);
}

void DrawWaveEditor()
{
	int X = EDX;
	int Y = EDY;
	g.fillrect(X, Y, EDW, EDH, 0xff202020);
	RectO(X + 5, Y + 5, EDW - 10, EDH - 10, 0xff808080);
	float h2 = (EDH - 10) / 2.0;
	float yc = Y + 5 + h2;
	for (int i = 5; i < EDW - 5; i += 8)
	{
		float t = (i - 5.0) / (EDW - 10.0);
		float v = WaveAt(curanim, t) * h2;
		if (v >= 0.0) g.fillrect(X + i, int(yc - v), 1, int(v) + 1, 0xff505000);
		else g.fillrect(X + i, int(yc), 1, int(-v) + 1, 0xff505000);
	}
	for (int i = 1; i < 8; i++)
	{
		g.fillrect(X + 5 + (EDW - 10) * i / 8, Y, 1, EDH, 0x600080ff);
		g.fillrect(X, Y + 5 + (EDH - 10) * i / 8, EDW, 1, 0x600080ff);
	}
	g.fillrect(X, Y + 5 + (EDH - 10) / 2, EDW, 1, 0xffc0c0c0);
	g.clear();
	for (int i = 5; i < EDW - 5; i++)
	{
		float t = (i - 5.0) / (EDW - 10.0);
		float py = yc - h2 * WaveAt(curanim, t);
		if (i == 5) g.M(X + i, py);
		else g.L(X + i, py);
	}
	g.fin();
	g.rgba32(0xffffff00);
	g.width(1, 2);
	g.stroke();
	for (int i = 0; i < npl; i++)
	{
		float px = PtSX(i);
		float py = PtSY(i);
		g.Circle(px, py, 0, 2, 1, 0x80ffffff);
		if (i == plSel) g.Circle(px, py, 5, 0.5, 1, 0xffffffff);
		if (i == plHover) g.Circle(px, py, 5, 0.5, 1, 0x80ffffff);
	}
}

void DrawPiano()
{
	int keyX = 100;
	int keyY = 800;
	int keyW = 20;
	int keyH = 60;
	int pressed = 0xff0080ff;
	for (int i = 0; i < 10; i++)
	{
		int x0 = keyX + i * keyW;
		g.fillrect(x0, keyY, keyW, keyH, 0xffffffff);
		RectO(x0, keyY, keyW, keyH, 0xffc0c0c0);
	}
	for (int k = 0; k < 17; k++)
		if (keyDown[k] != 0)
			if (kmW[k] >= 0) g.fillrect(keyX + kmW[k] * keyW + 1, keyY + 1, keyW - 2, keyH - 2, pressed);
	int bw = keyW * 3 / 4;
	int bh = keyH * 2 / 3;
	for (int i = 1; i < 10; i++)
	{
		if (i == 3 || i == 7) continue;
		int x0 = keyX + keyW * i - keyW / 3;
		g.fillrect(x0, keyY, bw, bh, 0xff404040);
		RectO(x0, keyY, bw, bh, 0xffc0c0c0);
	}
	for (int k = 0; k < 17; k++)
		if (keyDown[k] != 0)
			if (kmB[k] >= 0) g.fillrect(keyX + keyW * kmB[k] - keyW / 3 + 1, keyY + 1, bw - 2, bh - 2, pressed);
	stext("Q 2 W 3 E R 5 T 6 Y 7 U I 9 O 0 P", keyX, keyY + keyH + 6, 0xff000000);
}

void Draw()
{
	g.rgba32(0xff808080);
	g.FillRT();

	for (int i = 0; i < nosc; i++)
	{
		int sel = 0;
		if (curanim == i) sel = 1;
		int hov = 0;
		if (hoveranim == i) hov = 1;
		int so = 0;
		if (solo == i) so = 1;
		DrawOscRow(i, 10, 100 + i * 50, 100.0, 50.0, sel, hov, so);
	}

	// oscilloscope: the most recent SCRW mixed samples
	g.fillrect(0, 400, SCRW, 1, 0xffc0c0c0);
	g.fillrect(0, 600, SCRW, 1, 0xffc0c0c0);
	g.fillrect(0, 500, SCRW, 1, 0xff0000ff);
	g.clear();
	int sp = scopePos - SCRW;
	for (int i = 0; i < SCRW; i++)
	{
		float v = scope[(sp + i) & (SCOPE - 1)];
		if (v > 4.0) v = 4.0;
		if (v < -4.0) v = -4.0;
		if (i == 0) g.M(i, 500 + 100 * v);
		else g.L(i, 500 + 100 * v);
	}
	g.fin();
	g.rgba32(0xff00ff00);
	g.width(1, 2);
	g.stroke();

	DrawPiano();

	char text[160];
	int top = 5;
	float pct = 0.0;
	if (mixAll > 0.0) pct = mixProcTime / mixAll * 100.0;
	snprintf(text, 159, "CPU time: %f/%f %f%%", mixProcTime, mixAll, pct);
	stext(text, 5, top, 0xff000000);
	int nheld = 0;
	int nfree = 0;
	for (int i = 0; i < MAXN; i++)
		if (notes[i].active != 0)
		{
			if (notes[i].held != 0) nheld++;
			else nfree++;
		}
	snprintf(text, 159, "notes: %i+%i", nheld, nfree);
	top += 11;
	stext(text, 5, top, 0xff000000);
	int lines = 0;
	for (int i = 0; i < MAXN; i++)
	{
		if (notes[i].active == 0) continue;
		if (lines >= 6) break;
		snprintf(text, 159, "%f Hz", notes[i].freq);
		int col = 0xff000000;
		if (notes[i].held != 0) col = 0xff000080;
		top += 11;
		stext(text, 5, top, col);
		lines++;
	}
	snprintf(text, 159, "osc: %i/%i", curanim, nosc);
	stext(text, 350, 5, 0xff000000);
	snprintf(text, 159, "file: %s", filename);
	stext(text, 350, 16, 0xff000000);

	DrawPanel(EPX, EPY, EPW, EPH, "Envelope");
	for (int k = 0; k < 10; k++) DrawSlider(k);
	DrawCheck(CB_SUSTAINABLE, "Sustainable");
	DrawPanel(WPX, WPY, WPW, WPH, "Waveform");
	DrawWaveEditor();
	for (int k = 10; k < 13; k++) DrawSlider(k);
	DrawCheck(CB_SUSTAIN, "Sustain");
	stext("drag points; Ctrl+click / right-click adds, right-click / Del removes",
	      WPX + 5, WPY + WPH - 16, 0xffd0d0d0);

	if (recording != 0)
	{
		snprintf(text, 159, "Rec: %i %f", nPlay, sndT - recordStart);
		stext(text, SCRW - 150, 5, 0xffff0000);
	}
	else stext("F5 to record/stop record", SCRW - 150, 5, 0xffff0000);
	if (playing != 0)
	{
		snprintf(text, 159, "Play: %i/%i %f", curPlayNote, nPlay, curPlayTime);
		stext(text, SCRW - 150, 16, 0xffff0000);
	}
	else stext("F7 to play/restart", SCRW - 150, 16, 0xffff0000);
	stext("F2 save   PgUp/PgDn osc   Ins add osc   M mute   S solo", 5, 880, 0xff000000);
	if (Time() - messageTime < 3.0) stext(message, 5, 892, 0xff000080);
}

// --------------------------------------------------------------- main
typedef char* pchar;

int main(int argc, pchar* argv)
{
	wt = (float*)malloc(MAXOSC * WTS * sizeof(float));
	jb = (char*)malloc(JMAX + 4);
	for (int i = 0; i < NNOISE; i++) noiseTab[i] = irand() / 16383.5 - 1.0;
	for (int i = 0; i < CHUNK; i++) mixBuf[i] = 0.0;
	for (int i = 0; i < SCOPE; i++) scope[i] = 0.0;
	for (int i = 0; i < MAXN; i++) { notes[i].active = 0; notes[i].held = 0; }
	for (int k = 0; k < 17; k++) { keyDown[k] = 0; keyNote[k] = -1; relPending[k] = 0; keyFreq[k] = 0.0; }
	for (int k = 0; k < NSB; k++) sbPos[k] = 0.0;
	cbOn[0] = 0;
	cbOn[1] = 0;
	message[0] = 0;
	LayoutGUI();

	// default instrument: one sine-ish oscillator
	nosc = 1;
	SetOscDefaults(0);
	// default recording, as in the original
	nPlay = 0;
	Record(0, 0, 0.1, 0.5, 1.0);
	Record(1, 0, 0.5, 0.55, 1.0);

	sbPos[SB_OCTAVE] = 0.25;
	sbPos[SB_ALLVOL] = 0.5;
	sbPos[SB_LENGTH] = 0.0;

	strcopy(filename, "instr.json", 256);
	if (argc > 1)
	{
		strcopy(filename, argv[1], 256);
		if (LoadInstrument(filename)) snprintf(message, 255, "Loaded %s: %i oscillators", filename, nosc);
		else snprintf(message, 255, "Could not load %s, using the default instrument", filename);
		messageTime = Time();
	}
	actualize = 1;

	float lastDraw = -1.0;
	while (true)
	{
		FillAudio();
		HandleEvents();
		float now = Time();
		if (now - lastDraw >= 1.0 / 40.0)
		{
			lastDraw = now;
			UpdateState();
			Draw();
			FillAudio();
			Present();
		}
		else
		{
			Poll();
			Wait(0.002);
		}
	}
	return 0;
}

// Differences from the C++ original (Lowland/SoundEnvelope/main.cpp):
// - Oscillators play from a 1024-sample wavetable rebuilt whenever the
//   waveform or interpolation changes, instead of evaluating the keyframes
//   per sample; envelope and vibrato are evaluated every 64 samples and
//   interpolated linearly. Both keep the interpreter real-time.
// - "mixed" noise uses a 4096-entry random table instead of the integer hash.
// - As in the original, "Sustainable" only changes the envelope drawing
//   (tune.h computes the per-oscillator length but never uses it); a held key
//   in Sustain mode holds every oscillator. Release is never cut short.
// - F5 stops a running recording (the original only re-sorted it).
// - Ins selects the new oscillator; Esc quits; right-click edits points.
// - Recorded notes replay at the volume they were played with.
// - At most 16 oscillators, 64 points each, 64 notes, 2048 recorded notes.
