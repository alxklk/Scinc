#include "sound.h"                    // intrinsic: CSound audio API
#include "graphics.h"                  // intrinsic: the Graph drawing API (g.*)
#include "../../ws.h"                  // windowing: Present(), Time() (real file)
#include "../../algorithm/Token.h"     // SToken / MakeTokens: parse the CLI args
#include "../../algorithm/Lex.h"

// fractal_melody_scope -- the visual companion to fractal_melody.cpp.
//
// Same 4-track generative synth (a Thue-Morse fractal read at four time scales
// drives LEAD / HARMONY / BASS over a DRUM track), but this one opens a window
// and shows what it is doing:
//
//   * a stereo oscilloscope of the mixed output
//   * the 16-step drum grid with the current step highlighted
//   * a "fractal roll" plotting the lead & bass walk history -- you can see the
//     self-similar Thue-Morse contour scroll by
//   * per-track VU meters
//
// Tempo and scale are selectable, both from the command line and live:
//   Scinc fractal_melody_scope.cpp [bpm] [scale]
//     bpm   : tempo, e.g. 128   (default 112)
//     scale : 0 minor-pentatonic 1 major-pentatonic 2 natural-minor
//             3 dorian 4 major   (default 0)
//   live keys:  1-5 pick scale   -/+ tempo   space skip ahead   Q quit
//
// Scinc notes (see fractal_melody.cpp for the audio details): no atoi(), so the
// CLI numbers are tokenised with the lexer (MakeTokens/GetNumber, as in
// claude/graphic/raytrace.cpp); no comma-initialised declarations; plain
// char* (const can misbehave); struct references are fine.

typedef char* pchar;

#define SR       48000
#define BUF_LOW  4000
#define CHUNK    512
#define TWO_PI   6.28318530718
#define ROOT_HZ  110.0                 // frequency of scale-degree 0 (A2)

#define SCOPE_N  1024                  // oscilloscope ring buffer
#define HIST_N   128                   // fractal-roll history length

Graph g;
CSound snd;

// ----------------------------- scale table ---------------------------------
char* scaleName;
int   nDeg;                            // notes per octave in the current scale
int   scaleSemi[7];                    // semitone offsets from the root
int   leadRange;                       // lead walk spans 0..leadRange
int   bassRange;                       // bass walk spans 0..bassRange
int   curScale;

char* NOTE[12] = { "A ", "A#", "B ", "C ", "C#", "D ", "D#",
                   "E ", "F ", "F#", "G ", "G#" };

// ----------------------------- synth voices --------------------------------
struct Voice
{
	float freq;    // current note frequency (Hz); 0 = silent
	int   start;   // gSample at trigger
	float dur;     // seconds until the note fully decays
	float amp;     // track volume
	float atk;     // attack time (s)
	float fm;      // FM modulation index (0 = pure sine)
	float ratio;   // FM modulator : carrier ratio
	float pan;     // 0 = left .. 1 = right
};

Voice vLead;
Voice vHarm;
Voice vBass;

int   kickStart;   float kickAmp;
int   snareStart;  float snareAmp;
int   hatStart;    float hatAmp;

// ----------------------------- transport -----------------------------------
int gSample;
int gStep;
int stepCounter;
int stepSamples;
int gBPM;
int melPos;
int bassPos;

// ----------------------------- fx / visuals --------------------------------
float* delayBuf;
int    delayLen;
int    delayPos;

float scopeBuf[SCOPE_N];
int   scopePos;

int   histLead[HIST_N];                // lead walk history (for the roll)
int   histBass[HIST_N];
int   histWrite;                       // total notes pushed (monotonic)

float lvlLead;                         // smoothed peak levels (VU meters)
float lvlHarm;
float lvlBass;
float lvlDrum;

int rndState = 1;

float fabsF(float x) { if (x < 0.0) return -x; return x; }

int popcount(int n)
{
	int c = 0;
	while (n > 0) { c += n & 1; n >>= 1; }
	return c;
}

int thueMorse(int n) { return popcount(n) & 1; }   // the fractal source

int reflect(int x, int lo, int hi)
{
	int range = hi - lo;
	if (range <= 0)
		return lo;
	int span = 2 * range;
	int m = (x - lo) % span;
	if (m < 0)
		m += span;
	if (m > range)
		m = span - m;
	return lo + m;
}

float degFreq(int d)
{
	if (d < 0)
		d = 0;
	int oct  = d / nDeg;
	int step = d % nDeg;
	int semi = scaleSemi[step] + 12 * oct;
	return ROOT_HZ * pow(2.0, semi / 12.0);
}

float noiseSample()
{
	rndState = rndState * 1103515245 + 12345;
	return ((rndState >> 16) & 0x7fff) / 16383.5 - 1.0;
}

void LoadScale(int idx)
{
	idx = idx % 5;
	if (idx < 0)
		idx += 5;
	if (idx == 0)
	{
		scaleName = "minor pentatonic"; nDeg = 5;
		scaleSemi[0]=0; scaleSemi[1]=3; scaleSemi[2]=5; scaleSemi[3]=7; scaleSemi[4]=10;
	}
	else if (idx == 1)
	{
		scaleName = "major pentatonic"; nDeg = 5;
		scaleSemi[0]=0; scaleSemi[1]=2; scaleSemi[2]=4; scaleSemi[3]=7; scaleSemi[4]=9;
	}
	else if (idx == 2)
	{
		scaleName = "natural minor"; nDeg = 7;
		scaleSemi[0]=0; scaleSemi[1]=2; scaleSemi[2]=3; scaleSemi[3]=5;
		scaleSemi[4]=7; scaleSemi[5]=8; scaleSemi[6]=10;
	}
	else if (idx == 3)
	{
		scaleName = "dorian"; nDeg = 7;
		scaleSemi[0]=0; scaleSemi[1]=2; scaleSemi[2]=3; scaleSemi[3]=5;
		scaleSemi[4]=7; scaleSemi[5]=9; scaleSemi[6]=10;
	}
	else
	{
		scaleName = "major"; nDeg = 7;
		scaleSemi[0]=0; scaleSemi[1]=2; scaleSemi[2]=4; scaleSemi[3]=5;
		scaleSemi[4]=7; scaleSemi[5]=9; scaleSemi[6]=11;
	}
	curScale  = idx;
	leadRange = 2 * nDeg;      // ~two octaves
	bassRange = nDeg;          // ~one octave
	if (melPos > leadRange) melPos = leadRange;
	if (bassPos > bassRange) bassPos = bassRange;
}

void SetTempo(int bpm)
{
	if (bpm < 40)  bpm = 40;
	if (bpm > 240) bpm = 240;
	gBPM = bpm;
	stepSamples = int(15.0 / bpm * SR);   // a 16th note = quarter/4 = 15/BPM s
}

float renderVoice(Voice& v)
{
	if (v.freq <= 0.0)
		return 0.0;
	float t = (gSample - v.start) / float(SR);
	if (t < 0.0)
		return 0.0;
	float u = t / v.dur;
	if (u >= 1.0)
		return 0.0;
	float env;
	if (t < v.atk)
		env = t / v.atk;
	else
		env = 1.0;
	float dec = 1.0 - u;
	env *= dec * dec;
	float ph = TWO_PI * v.freq * t;
	float s;
	if (v.fm > 0.0)
	{
		float mph = TWO_PI * v.freq * v.ratio * t;
		s = sin(ph + v.fm * (1.0 - u) * sin(mph));
	}
	else
	{
		s = sin(ph);
	}
	return s * env * v.amp;
}

float renderKick()
{
	if (kickAmp <= 0.0)
		return 0.0;
	float t = (gSample - kickStart) / float(SR);
	if (t < 0.0 || t > 0.30)
		return 0.0;
	float u = t / 0.30;
	float env = 1.0 - u;
	env *= env;
	float pu = t / 0.07;
	if (pu > 1.0)
		pu = 1.0;
	float f = 45.0 + 100.0 * (1.0 - pu) * (1.0 - pu);
	return sin(TWO_PI * f * t) * env * kickAmp;
}

float renderSnare()
{
	if (snareAmp <= 0.0)
		return 0.0;
	float t = (gSample - snareStart) / float(SR);
	if (t < 0.0 || t > 0.18)
		return 0.0;
	float u = t / 0.18;
	float env = 1.0 - u;
	env *= env;
	float tone = sin(TWO_PI * 185.0 * t) * 0.4;
	return (noiseSample() * 0.7 + tone) * env * snareAmp;
}

float renderHat()
{
	if (hatAmp <= 0.0)
		return 0.0;
	float t = (gSample - hatStart) / float(SR);
	if (t < 0.0 || t > 0.05)
		return 0.0;
	float u = t / 0.05;
	float env = 1.0 - u;
	env *= env;
	return noiseSample() * env * hatAmp;
}

// the sequencer, called at every 16th-note boundary
void onStep(int step)
{
	int s16 = step % 16;
	int bar = step / 16;

	if (s16 % 2 == 0)   // LEAD: 8th notes, Thue-Morse walk
	{
		int dir = thueMorse(step) ? 1 : -1;
		melPos = reflect(melPos + dir, 0, leadRange);
		vLead.freq  = degFreq(melPos + 2 * nDeg);
		vLead.start = gSample;
		vLead.dur   = 2.0 * stepSamples / float(SR) * 0.90;
		histLead[histWrite % HIST_N] = melPos;   // record for the fractal roll
		histBass[histWrite % HIST_N] = bassPos;
		histWrite++;
	}
	if (s16 % 4 == 0)   // HARMONY: quarter notes, a 3rd below the lead
	{
		int hd = melPos - 2;
		if (hd < 0)
			hd += nDeg;
		vHarm.freq  = degFreq(hd + nDeg);
		vHarm.start = gSample;
		vHarm.dur   = 4.0 * stepSamples / float(SR) * 0.95;
	}
	if (s16 % 8 == 0)   // BASS: half notes, slow Thue-Morse walk
	{
		int dir = thueMorse(bar) ? 1 : -1;
		bassPos = reflect(bassPos + dir, 0, bassRange);
		vBass.freq  = degFreq(bassPos);
		vBass.start = gSample;
		vBass.dur   = 8.0 * stepSamples / float(SR) * 0.98;
	}

	if (s16 == 0 || s16 == 8)              { kickStart = gSample; kickAmp = 0.60; }
	else if (s16 == 14 && thueMorse(step)) { kickStart = gSample; kickAmp = 0.32; }
	if (s16 == 4 || s16 == 12)             { snareStart = gSample; snareAmp = 0.40; }
	hatStart = gSample;
	hatAmp = thueMorse(step) ? 0.18 : 0.09;
}

// generate exactly one output sample (advances the sequencer & transport)
void GenSample()
{
	if (stepCounter == 0)
	{
		onStep(gStep);
		gStep++;
	}

	float lead = renderVoice(vLead);
	float harm = renderVoice(vHarm);
	float bass = renderVoice(vBass);
	float drum = renderKick() + renderSnare() + renderHat();

	float l = lead * (1.0 - vLead.pan) + harm * (1.0 - vHarm.pan) + bass * 0.5 + drum * 0.5;
	float r = lead * vLead.pan       + harm * vHarm.pan       + bass * 0.5 + drum * 0.5;

	int dp = delayPos * 2;
	float el = delayBuf[dp];
	float er = delayBuf[dp + 1];
	float outL = l + er * 0.33;
	float outR = r + el * 0.33;
	delayBuf[dp]     = outL * 0.9;
	delayBuf[dp + 1] = outR * 0.9;
	delayPos++;
	if (delayPos >= delayLen)
		delayPos = 0;

	if (outL > 0.95) outL = 0.95; else if (outL < -0.95) outL = -0.95;
	if (outR > 0.95) outR = 0.95; else if (outR < -0.95) outR = -0.95;

	snd.snd_out(outL, outR);

	// feed the visualisers
	scopeBuf[scopePos] = outL;
	scopePos++;
	if (scopePos >= SCOPE_N)
		scopePos = 0;

	float a;
	lvlLead = lvlLead * 0.9996; a = fabsF(lead); if (a > lvlLead) lvlLead = a;
	lvlHarm = lvlHarm * 0.9996; a = fabsF(harm); if (a > lvlHarm) lvlHarm = a;
	lvlBass = lvlBass * 0.9996; a = fabsF(bass); if (a > lvlBass) lvlBass = a;
	lvlDrum = lvlDrum * 0.9996; a = fabsF(drum); if (a > lvlDrum) lvlDrum = a;

	gSample++;
	stepCounter++;
	if (stepCounter >= stepSamples)
		stepCounter = 0;
}

// -------------------------------- drawing ----------------------------------
float scopeGet(int col)   // scope sample for screen column col (0..639)
{
	int idx = scopePos - 640 + col + SCOPE_N;
	idx = idx % SCOPE_N;
	return scopeBuf[idx];
}

// plot one walk history as a polyline; the available history is spread across
// the full width, so the roll looks alive from the first notes and settles
// into a steady scroll once the ring buffer fills
void DrawWalk(int* hist, int range, int rx, int ry, int rw, int rh,
              float cr, float cg, float cb, float w)
{
	int n = histWrite;
	if (n > HIST_N)
		n = HIST_N;
	if (n < 2)
		return;
	g.clear();
	for (int j = 0; j < n; j++)
	{
		int k = histWrite - n + j;
		int d = hist[((k % HIST_N) + HIST_N) % HIST_N];
		float x = rx + 2.0 + float(j) / (n - 1) * (rw - 4.0);
		float y = ry + rh - 2.0 - (d + 0.5) / (range + 1.0) * (rh - 4.0);
		if (j == 0)
			g.M(x, y);
		else
			g.L(x, y);
	}
	g.fin();
	g.rgb(cr, cg, cb);
	g.width(w, 1.0);
	g.stroke();
}

void Meter(int x, int y, int w, int h, float lvl, char* label, int color)
{
	float v = lvl * 1.6;
	if (v > 1.0) v = 1.0;
	int bh = int(v * h);
	g.fillrect(x, y, w, h, 0xff10161f);            // track
	if (bh > 0)
		g.fillrect(x, y + h - bh, w, bh, color);   // level
	stext(label, x, y + h + 4, 0xff8fa2b5);
}

void DrawFrame()
{
	char buf[96];
	int curBar = gStep / 16;
	int cur16  = gStep % 16;

	g.rgba32(0xff0a0e18);
	g.FillRT();

	stext("fractal_melody  --  4-track generative synth", 10, 8, 0xffffffff);
	snprintf(buf, 96, "scale %i: %-16s   tempo: %i BPM   bar %i",
		curScale, scaleName, gBPM, curBar);
	stext(buf, 10, 20, 0xff90c8ff);

	// --- oscilloscope ---
	stext("output", 10, 40, 0xff5a6b7a);
	float scy = 92.0;
	float amp = 46.0;
	g.clear();
	g.M(0, scy + scopeGet(0) * amp);
	for (int i = 1; i < 640; i++)
		g.L(i, scy + scopeGet(i) * amp);
	g.fin();
	g.rgb(0.10, 0.45, 0.24); g.width(6.0, 1.0); g.stroke();   // glow
	g.rgb(0.42, 1.00, 0.56); g.width(1.5, 1.0); g.stroke();   // core

	// --- 16-step drum grid ---
	stext("beat", 10, 150, 0xff5a6b7a);
	int bx = 10;
	int by = 162;
	int bw = 34;
	int gp = 4;
	for (int s = 0; s < 16; s++)
	{
		int step = curBar * 16 + s;
		int col;
		if (s == 0 || s == 8 || (s == 14 && thueMorse(step)))
			col = 0xffff5442;                      // kick
		else if (s == 4 || s == 12)
			col = 0xfff0c040;                      // snare
		else
			col = thueMorse(step) ? 0xff44586a : 0xff283240;   // hat accent / normal
		int x = bx + s * (bw + gp);
		g.fillrect(x, by, bw, 16, col);
		if (s == cur16)                            // current step: white outline
		{
			g.clear();
			g.M(x, by); g.l(bw, 0); g.l(0, 16); g.l(-bw, 0); g.close();
			g.fin();
			g.rgb(1.0, 1.0, 1.0); g.width(2.0, 1.0); g.stroke();
		}
	}

	// --- fractal roll: the lead & bass walk history ---
	stext("fractal roll  (lead / bass walk)", 10, 196, 0xff5a6b7a);
	int rx = 10;
	int ry = 208;
	int rw = 620;
	int rh = 150;
	g.fillrect(rx, ry, rw, rh, 0xff0f1420);
	// octave gridlines for the lead range
	g.clear();
	for (int oc = 0; oc <= leadRange; oc += nDeg)
	{
		float y = ry + rh - 2.0 - (oc + 0.5) / (leadRange + 1.0) * (rh - 4.0);
		g.M(rx, y); g.l(rw, 0);
	}
	g.fin();
	g.rgb(0.12, 0.18, 0.28); g.width(1.0, 1.0); g.stroke();

	DrawWalk(histBass, bassRange, rx, ry, rw, rh, 0.30, 0.55, 1.00, 1.5);  // dim
	DrawWalk(histLead, leadRange, rx, ry, rw, rh, 1.00, 0.80, 0.30, 2.0);  // bright

	// --- VU meters ---
	Meter( 20, 376, 40, 70, lvlLead, "LEAD", 0xffffcc4d);
	Meter(120, 376, 40, 70, lvlHarm, "HARM", 0xff4d9bff);
	Meter(220, 376, 40, 70, lvlBass, "BASS", 0xff4dd0a0);
	Meter(320, 376, 40, 70, lvlDrum, "DRUM", 0xffff6a5a);

	stext("keys:  1-5 scale    -/+ tempo    space skip    Q quit", 10, 466, 0xff5a6b7a);
}

int main(int argc, pchar* argv)
{
	gBPM = 112;
	int scaleIdx = 0;
	melPos = 4;
	bassPos = 0;

	if (argc >= 2)
	{
		SToken t[16];
		MakeTokens(t, argv[1]);
		if (t[0].IsNumber()) gBPM = int(t[0].GetNumber());
	}
	if (argc >= 3)
	{
		SToken t2[16];
		MakeTokens(t2, argv[2]);
		if (t2[0].IsNumber()) scaleIdx = int(t2[0].GetNumber());
	}

	LoadScale(scaleIdx);
	SetTempo(gBPM);

	// voices
	vLead.freq = 0.0; vLead.amp = 0.26; vLead.atk = 0.003;
	vLead.fm = 2.2; vLead.ratio = 2.0; vLead.pan = 0.63; vLead.start = -1000000; vLead.dur = 0.2;
	vHarm.freq = 0.0; vHarm.amp = 0.14; vHarm.atk = 0.030;
	vHarm.fm = 0.0; vHarm.ratio = 1.0; vHarm.pan = 0.37; vHarm.start = -1000000; vHarm.dur = 0.4;
	vBass.freq = 0.0; vBass.amp = 0.30; vBass.atk = 0.006;
	vBass.fm = 0.8; vBass.ratio = 1.0; vBass.pan = 0.50; vBass.start = -1000000; vBass.dur = 0.4;
	kickAmp = 0.0;  kickStart  = -1000000;
	snareAmp = 0.0; snareStart = -1000000;
	hatAmp = 0.0;   hatStart   = -1000000;

	// transport / fx / visuals
	gSample = 0;
	gStep = 0;
	stepCounter = 0;
	delayLen = int(0.22 * SR);
	delayPos = 0;
	delayBuf = (float*)malloc(sizeof(float) * 2 * delayLen);
	for (int i = 0; i < 2 * delayLen; i++)
		delayBuf[i] = 0.0;
	for (int i = 0; i < SCOPE_N; i++)
		scopeBuf[i] = 0.0;
	scopePos = 0;
	histWrite = 0;
	lvlLead = 0.0; lvlHarm = 0.0; lvlBass = 0.0; lvlDrum = 0.0;

	printf("fractal_melody_scope: scale %i (%s), %i BPM\n", curScale, scaleName, gBPM);
	printf("  keys: 1-5 scale, -/+ tempo, space skip, Q quit\n");
	fflush(stdout);

	while (true)
	{
		snd.Poll();   // service the audio device (drains the queue as it plays)

		int key;
		int press;
		while (GetKeyEvent(key, press))
		{
			if (press > 0)
			{
				if (key >= '1' && key <= '5')       LoadScale(key - '1');
				else if (key == '-')                SetTempo(gBPM - 4);
				else if (key == '=' || key == '+')  SetTempo(gBPM + 4);
				else if (key == ' ')                gStep += 137;   // jump the fractal
			}
		}

		while (snd.snd_bufhealth() < BUF_LOW)
			for (int i = 0; i < CHUNK; i++)
				GenSample();

		DrawFrame();
		Present();
	}
	return 0;
}
