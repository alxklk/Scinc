#include "sound.h"                    // intrinsic: CSound audio API
#include "graphics.h"                  // intrinsic: the Graph drawing API (g.*)
#include "../../ws.h"                  // windowing: Present(), Time() (real file)

// track_editor -- a 4-track visual melody editor, in the spirit of
// examples/sound/melody_matrix.cpp but rebuilt so each of the four 16-step
// grids is an INDEPENDENT instrument track (not the mel*baz multiplication
// scheme of the original), each playable with its own synthesized instrument.
//
// Paint notes with the mouse; a playhead sweeps the 16 steps in a loop and
// triggers each track's instrument. Click a track's header to cycle its
// instrument through the palette:
//
//   EPiano  - 2-op FM electric piano (bright attack, mellow tail)
//   Bell    - 2-op FM with an inharmonic ratio (metallic, long decay)
//   Pluck   - Karplus-Strong plucked string (noise burst + damped delay line)
//   Square  - hollow square-wave lead (chiptune)
//   Pad     - three detuned saws, slow attack (lush)
//   Organ   - additive drawbar harmonics (sustained)
//   SubBass - saturated sine, low
//   Drums   - percussion; the row picks kick / snare / hat / clap
//
// Controls:  paint = drag on a grid    click header = next instrument
//            T random   C clear   - / + tempo   Q quit
//
// Grid rows go high (top) to low, with the very bottom row = rest, so painting
// the bottom cell clears a step.
//
// Scinc notes (see fractal_melody.cpp): a graphical sound loop must call
// snd.Poll() itself (Present()'s Poll() is the windowing poll, not the audio
// device); no comma-initialised declarations; plain char*; no rand() (LCG);
// struct references and array members are fine.

#define M_PI     3.14159265358979
#define TWO_PI   6.28318530718
#define SR       48000
#define BUF_LOW  4000
#define CHUNK    512
#define ROOT_HZ  220.0
#define NOTESIZE 14
#define NCOL     16
#define NROW     10

// instruments
#define EPIANO 0
#define BELL   1
#define PLUCK  2
#define SQUARE 3
#define PAD    4
#define ORGAN  5
#define SUB    6
#define DRUMS  7
#define NINSTR 8

char* INSTR_NAME[8] = { "EPiano", "Bell", "Pluck", "Square",
                        "Pad", "Organ", "SubBass", "Drums" };

// A minor pentatonic
#define NDEG 5
int scaleSemi[NDEG] = { 0, 3, 5, 7, 10 };

Graph g;
CSound snd;

int rndState = 12345;
int irand()
{
	rndState = rndState * 1103515245 + 12345;
	return (rndState >> 16) & 0x7fff;
}
float noiseSample() { return irand() / 16383.5 - 1.0; }
float fabsF(float x) { if (x < 0.0) return -x; return x; }
float sawWave(float x) { x = x - int(x); return 2.0 * x - 1.0; }

// ----------------------------- voice pool ----------------------------------
#define NV    16
#define KSMAX 1024

struct Voice
{
	int   active;
	int   instr;
	int   param;    // drum type for DRUMS
	float freq;
	int   t0;
	float amp;
	float pan;
	int   ksBase;   // Karplus-Strong delay line: offset into ksPool (PLUCK)
	int   kslen;
	int   kspos;
};

Voice  voices[NV];
float* ksPool;

int playSample;
int stepSamples;
int lastStep;
int gBPM;

// stereo ping-pong delay
float* delayBuf;
int    delayLen;
int    delayPos;

// ----------------------------- tracks --------------------------------------
struct Track
{
	int   note[NCOL];   // 0 = rest, 1..9 = pitch (row)
	int   instr;
	int   octave;       // semitone shift
	float pan;
	int   gx;
	int   gy;
	int   color;
	char* name;
	int   hoverc;
	int   hoverr;
};

Track tracks[4];

int mx;
int my;
int mb;
int prevmx;
int prevmy;
int prevmb;

float lifeOf(int instr)
{
	if (instr == BELL)  return 1.5;
	if (instr == PLUCK) return 1.0;
	if (instr == PAD)   return 1.7;
	if (instr == ORGAN) return 1.3;
	if (instr == DRUMS) return 0.35;
	return 0.55;   // EPiano, Square, Sub
}
float atkOf(int instr)
{
	if (instr == PAD)   return 0.060;
	if (instr == ORGAN) return 0.012;
	return 0.003;
}
float ampOf(int instr)
{
	if (instr == SQUARE) return 0.32;
	if (instr == PAD)    return 0.26;
	if (instr == ORGAN)  return 0.30;
	if (instr == BELL)   return 0.42;
	if (instr == DRUMS)  return 0.60;
	return 0.48;   // EPiano, Pluck, Sub
}

float degToFreq(int degree, int octaveSemi)
{
	int oct  = degree / NDEG;
	int st   = degree % NDEG;
	int semi = scaleSemi[st] + 12 * oct + octaveSemi;
	return ROOT_HZ * pow(2.0, semi / 12.0);
}

float renderDrum(int param, float t)
{
	if (param == 0)                     // kick
	{
		if (t > 0.30) return 0.0;
		float u = t / 0.30; float e = 1.0 - u; e *= e;
		float pu = t / 0.07; if (pu > 1.0) pu = 1.0;
		float f = 45.0 + 100.0 * (1.0 - pu) * (1.0 - pu);
		return sin(TWO_PI * f * t) * e;
	}
	if (param == 1)                     // snare
	{
		if (t > 0.18) return 0.0;
		float u = t / 0.18; float e = 1.0 - u; e *= e;
		return (noiseSample() * 0.7 + sin(TWO_PI * 185.0 * t) * 0.4) * e;
	}
	if (param == 2)                     // hi-hat
	{
		if (t > 0.05) return 0.0;
		float u = t / 0.05; float e = 1.0 - u; e *= e;
		return noiseSample() * e * 0.8;
	}
	if (t > 0.13) return 0.0;            // clap
	float u = t / 0.13; float e = 1.0 - u; e *= e;
	return noiseSample() * e * 0.9;
}

float renderVoice(Voice& v, int s)
{
	float t = (s - v.t0) / float(SR);
	if (t < 0.0)
		return 0.0;
	float dur = lifeOf(v.instr);
	float u = t / dur;
	if (u >= 1.0)
	{
		v.active = 0;
		return 0.0;
	}

	if (v.instr == DRUMS)
		return renderDrum(v.param, t) * v.amp;

	float atk = atkOf(v.instr);
	float env;
	if (t < atk) env = t / atk; else env = 1.0;
	if (v.instr == PAD || v.instr == ORGAN)
		env *= (1.0 - u);               // sustained-ish linear decay
	else
	{
		float d = 1.0 - u; env *= d * d;    // percussive
	}

	float ph = TWO_PI * v.freq * t;
	float s0;
	if (v.instr == EPIANO)
		s0 = sin(ph + 3.0 * (1.0 - u) * sin(ph * 2.0));
	else if (v.instr == BELL)
		s0 = sin(ph + 4.0 * (1.0 - u) * sin(ph * 3.5));
	else if (v.instr == PLUCK)
	{
		int b = v.ksBase;               // Karplus-Strong: read, then low-pass feedback
		float out = ksPool[b + v.kspos];
		int nx = v.kspos + 1;
		if (nx >= v.kslen) nx = 0;
		ksPool[b + v.kspos] = 0.5 * (ksPool[b + v.kspos] + ksPool[b + nx]) * 0.996;
		v.kspos = nx;
		s0 = out;
	}
	else if (v.instr == SQUARE)
		s0 = (sin(ph) > 0.0 ? 0.5 : -0.5);
	else if (v.instr == PAD)
		s0 = (sawWave(v.freq * t) + sawWave(v.freq * 1.006 * t)
			+ sawWave(v.freq * 0.994 * t)) * 0.24;
	else if (v.instr == ORGAN)
		s0 = (sin(ph) + 0.5 * sin(ph * 2.0) + 0.35 * sin(ph * 3.0)
			+ 0.2 * sin(ph * 4.0)) * 0.5;
	else                                // SUB: saturated sine
	{
		float d = sin(ph) * 1.8;
		s0 = d / (1.0 + fabsF(d));
	}
	return s0 * env * v.amp;
}

void AddVoice(int instr, float freq, float amp, int param, float pan)
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
	Voice& v = voices[slot];
	v.active = 1; v.instr = instr; v.param = param;
	v.freq = freq; v.amp = amp; v.pan = pan; v.t0 = playSample;
	if (instr == PLUCK)
	{
		int L = int(SR / freq);
		if (L < 8) L = 8;
		if (L > KSMAX) L = KSMAX;
		v.kslen = L; v.kspos = 0;
		for (int i = 0; i < L; i++)
			ksPool[v.ksBase + i] = noiseSample();
	}
}

void TriggerStep(int step)
{
	for (int tr = 0; tr < 4; tr++)
	{
		int nv = tracks[tr].note[step];
		if (nv <= 0)
			continue;                   // rest
		int instr = tracks[tr].instr;
		float amp = ampOf(instr);
		if (instr == DRUMS)
			AddVoice(instr, 220.0, amp, (nv - 1) % 4, tracks[tr].pan);
		else
			AddVoice(instr, degToFreq(nv - 1, tracks[tr].octave), amp, 0, tracks[tr].pan);
	}
}

void GenSample()
{
	int step = (playSample / stepSamples) % NCOL;
	if (step != lastStep)
	{
		lastStep = step;
		TriggerStep(step);
	}

	float l = 0.0;
	float r = 0.0;
	for (int i = 0; i < NV; i++)
	{
		if (!voices[i].active)
			continue;
		float m = renderVoice(voices[i], playSample);
		l += m * (1.0 - voices[i].pan);
		r += m * voices[i].pan;
	}
	l *= 0.9;
	r *= 0.9;

	int dp = delayPos * 2;
	float el = delayBuf[dp];
	float er = delayBuf[dp + 1];
	float outL = l + er * 0.28;
	float outR = r + el * 0.28;
	delayBuf[dp]     = outL * 0.9;
	delayBuf[dp + 1] = outR * 0.9;
	delayPos++;
	if (delayPos >= delayLen)
		delayPos = 0;

	if (outL > 0.95) outL = 0.95; else if (outL < -0.95) outL = -0.95;
	if (outR > 0.95) outR = 0.95; else if (outR < -0.95) outR = -0.95;
	snd.snd_out(outL, outR);
	playSample++;
}

// -------------------------------- editing ----------------------------------
void TrackUpdate(Track& t)
{
	t.hoverc = -1;
	t.hoverr = -1;
	int sc = (mx - t.gx) / NOTESIZE;
	int sr = (my - t.gy) / NOTESIZE;
	if (mx >= t.gx && my >= t.gy && sc >= 0 && sc < NCOL && sr >= 0 && sr < NROW)
	{
		t.hoverc = sc;
		t.hoverr = sr;
		int painting = 0;
		if ((mb & 1) && (prevmb & 1) == 0) painting = 1;
		if ((mb & 1) && (mx != prevmx || my != prevmy)) painting = 1;
		if (painting)
			t.note[sc] = (NROW - 1) - sr;   // top row = highest, bottom = rest(0)
	}
}

void DrawTrack(Track& t, int curStep)
{
	int W = NCOL * NOTESIZE;
	int H = NROW * NOTESIZE;

	g.fillrect(t.gx, t.gy, W, H, 0xff0e1420);                       // grid bg
	g.fillrect(t.gx + curStep * NOTESIZE, t.gy, NOTESIZE, H, 0x30ffffff);   // playhead

	for (int c = 0; c < NCOL; c++)
	{
		int nv = t.note[c];
		if (nv <= 0)
			continue;
		int sr = (NROW - 1) - nv;
		int col = t.color;
		if (c == curStep)
			col = 0xffffffff;
		g.fillrect(t.gx + c * NOTESIZE + 1, t.gy + sr * NOTESIZE + 1,
			NOTESIZE - 2, NOTESIZE - 2, col);
	}

	g.clear();
	for (int i = 0; i <= NCOL; i++) { g.M(t.gx + i * NOTESIZE, t.gy); g.l(0, H); }
	for (int i = 0; i <= NROW; i++) { g.M(t.gx, t.gy + i * NOTESIZE); g.l(W, 0); }
	g.fin();
	g.rgba32(0xff203040); g.width(1.0, 1.0); g.stroke();

	if (t.hoverc >= 0)
	{
		g.clear();
		g.M(t.gx + t.hoverc * NOTESIZE, t.gy + t.hoverr * NOTESIZE);
		g.l(NOTESIZE, 0); g.l(0, NOTESIZE); g.l(-NOTESIZE, 0); g.close();
		g.fin();
		g.rgba32(0xffffffff); g.width(1.0, 1.0); g.stroke();
	}

	g.fillrect(t.gx, t.gy - 18, W, 15, 0xff1b2636);                 // header button
	char hb[48];
	snprintf(hb, 48, "%s:  %s", t.name, INSTR_NAME[t.instr]);
	stext(hb, t.gx + 5, t.gy - 16, t.color);
}

void Randomize()
{
	for (int tr = 0; tr < 4; tr++)
		for (int c = 0; c < NCOL; c++)
		{
			if (irand() % 3 == 0)
				tracks[tr].note[c] = 1 + irand() % 9;
			else
				tracks[tr].note[c] = 0;
		}
}

void ClearAll()
{
	for (int tr = 0; tr < 4; tr++)
		for (int c = 0; c < NCOL; c++)
			tracks[tr].note[c] = 0;
}

void SetTempo(int bpm)
{
	if (bpm < 50)  bpm = 50;
	if (bpm > 220) bpm = 220;
	gBPM = bpm;
	stepSamples = int(15.0 / bpm * SR);
}

void InitTrack(int i, char* name, int instr, int octave, float pan,
               int gx, int gy, int color)
{
	tracks[i].name = name;
	tracks[i].instr = instr;
	tracks[i].octave = octave;
	tracks[i].pan = pan;
	tracks[i].gx = gx;
	tracks[i].gy = gy;
	tracks[i].color = color;
	tracks[i].hoverc = -1;
	tracks[i].hoverr = -1;
	for (int c = 0; c < NCOL; c++)
		tracks[i].note[c] = 0;
}

void LoadPattern(int i, char* p)   // p is 16 chars, '0' = rest, '1'..'9' = row
{
	for (int c = 0; c < NCOL; c++)
		tracks[i].note[c] = p[c] - '0';
}

int main()
{
	// voices + KS pool
	ksPool = (float*)malloc(sizeof(float) * NV * KSMAX);
	for (int i = 0; i < NV; i++)
	{
		voices[i].active = 0;
		voices[i].t0 = 0;
		voices[i].ksBase = i * KSMAX;
	}

	// transport / fx
	playSample = 0;
	lastStep = -1;
	SetTempo(120);
	delayLen = int(0.22 * SR);
	delayPos = 0;
	delayBuf = (float*)malloc(sizeof(float) * 2 * delayLen);
	for (int i = 0; i < 2 * delayLen; i++)
		delayBuf[i] = 0.0;

	// four tracks in a 2x2 layout
	InitTrack(0, "Lead",  EPIANO, 12, 0.62,  60,  70, 0xffffcc44);
	InitTrack(1, "Keys",  ORGAN,   0, 0.40, 340,  70, 0xff55c0ff);
	InitTrack(2, "Bass",  SUB,   -12, 0.50,  60, 290, 0xff55e0a0);
	InitTrack(3, "Drums", DRUMS,   0, 0.50, 340, 290, 0xffff7a5a);

	// a little starter groove so it plays immediately
	LoadPattern(0, "0607080706070805");
	LoadPattern(1, "4004005030050300");
	LoadPattern(2, "2020303010104040");
	LoadPattern(3, "1333213313332133");

	printf("track_editor: 4-track melody editor with synth instruments\n");
	printf("  paint=drag  click header=next instrument  T random  C clear  -/+ tempo  Q quit\n");
	fflush(stdout);

	while (true)
	{
		snd.Poll();

		int key;
		int press;
		while (GetKeyEvent(key, press))
			if (press > 0)
			{
				if (key == 't' || key == 'T') Randomize();
				else if (key == 'c' || key == 'C') ClearAll();
				else if (key == '-') SetTempo(gBPM - 5);
				else if (key == '=' || key == '+') SetTempo(gBPM + 5);
			}

		GetMouseState(mx, my, mb);
		// header clicks cycle a track's instrument
		if ((mb & 1) && (prevmb & 1) == 0)
			for (int tr = 0; tr < 4; tr++)
			{
				Track& t = tracks[tr];
				if (mx >= t.gx && mx < t.gx + NCOL * NOTESIZE
					&& my >= t.gy - 18 && my < t.gy - 3)
					t.instr = (t.instr + 1) % NINSTR;
			}
		for (int tr = 0; tr < 4; tr++)
			TrackUpdate(tracks[tr]);
		prevmx = mx; prevmy = my; prevmb = mb;

		// audio: top up the buffer, but cap the work per frame so a heavy
		// patch can never spin this loop and freeze the UI (it just underruns)
		int gen = 0;
		while (snd.snd_bufhealth() < BUF_LOW && gen < 48)
		{
			for (int i = 0; i < CHUNK; i++)
				GenSample();
			gen++;
		}

		// draw
		g.rgba32(0xff0a0e18);
		g.FillRT();
		stext("track_editor  --  4-track instrument sequencer", 12, 10, 0xffffffff);
		char st[80];
		int nActive = 0;
		for (int i = 0; i < NV; i++) if (voices[i].active) nActive++;
		snprintf(st, 80, "tempo %i BPM    voices %i", gBPM, nActive);
		stext(st, 12, 24, 0xff90c8ff);

		int curStep = (playSample / stepSamples) % NCOL;
		for (int tr = 0; tr < 4; tr++)
			DrawTrack(tracks[tr], curStep);

		stext("paint=drag   click header=instrument   T random   C clear   -/+ tempo   Q quit",
			12, 464, 0xff5a6b7a);

		Present();
	}
	return 0;
}
