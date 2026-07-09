#include "sound.h"
#include <stdio.h>

// fractal_melody -- a small 4-track generative synthesizer for Scinc.
//
// It endlessly composes a simple tune from ONE fractal generator (the
// Thue-Morse sequence) read at four different time scales, so every track is
// a self-similar variation of the same idea:
//
//   track 0  LEAD    - Thue-Morse random walk, 8th notes, 2-operator FM
//   track 1  HARMONY - a 3rd below the lead,   quarter notes, soft sine pad
//   track 2  BASS    - slow Thue-Morse walk,   half notes,  warm sine
//   track 3  DRUMS   - kick / snare / hi-hat groove on the 16th-note grid
//
// The Thue-Morse sequence t(n) = parity of the number of 1-bits in n
// (0 1 1 0 1 0 0 1 ...) is the textbook "fractal music" sequence: it is
// self-similar, and using it to nudge a walk up/down one step of a pentatonic
// scale yields a melody that wanders yet always stays in key.  A minor
// pentatonic scale is used throughout, so there are no wrong notes.
//
// Audio goes out through the intrinsic CSound API, exactly like the other
// examples in this folder (see whitenoise.cpp):
//   snd.Poll()           - service the audio device each pass
//   snd.snd_bufhealth()  - how many samples are queued; keep it topped up
//   snd.snd_out(l, r)    - push one stereo sample, each channel in [-1, 1]
//
// This is sound-only (no window), so it runs headless. Ctrl-C to stop.

#define SR       48000     // sample rate (Hz)
#define BPM      112       // tempo
#define BUF_LOW  4000      // keep at least this many samples queued
#define CHUNK    512       // samples generated per top-up pass
#define TWO_PI   6.28318530718

#define NDEG     5         // notes per octave in the scale (pentatonic)
#define ROOT_HZ  110.0     // frequency of scale-degree 0 (A2)

#define LEAD_RANGE 9       // lead walk spans degrees 0..9  (~two octaves)
#define BASS_RANGE 4       // bass walk spans degrees 0..4  (~one octave)

// A minor pentatonic, as semitone offsets from the root: A  C  D  E  G
int   scaleSemi[NDEG] = { 0, 3, 5, 7, 10 };

// Note names for the semitone classes we can land on (root = A).
char* NOTE[12] = { "A ", "A#", "B ", "C ", "C#", "D ", "D#",
                   "E ", "F ", "F#", "G ", "G#" };

// -------- one synthesizer voice (a monophonic oscillator + envelope) --------
struct Voice
{
	float freq;    // current note frequency in Hz (0 = silent)
	int   start;   // gSample at which the note was triggered
	float dur;     // seconds until the note fully decays
	float amp;     // track volume
	float atk;     // attack time in seconds
	float fm;      // FM modulation index (0 = a pure sine)
	float ratio;   // FM modulator : carrier frequency ratio
	float pan;     // stereo position, 0 = left .. 1 = right
};

Voice vLead;
Voice vHarm;
Voice vBass;

// drum one-shots: just a trigger time and a level each
int   kickStart;   float kickAmp;
int   snareStart;  float snareAmp;
int   hatStart;    float hatAmp;

// sequencer / transport state
int gSample;       // running sample counter
int gStep;         // running 16th-note step counter
int stepCounter;   // samples elapsed inside the current step
int stepSamples;   // samples per 16th note
int melPos;        // lead walk position (scale degrees)
int bassPos;       // bass walk position (scale degrees)

// stereo feedback delay (ping-pong) for a bit of space
float* delayBuf;
int    delayLen;
int    delayPos;

int rndState = 1;  // PRNG state (Scinc has no rand())

CSound snd;

// number of 1-bits in n
int popcount(int n)
{
	int c = 0;
	while (n > 0) { c += n & 1; n >>= 1; }
	return c;
}

// Thue-Morse bit: parity of the population count. This is the fractal source.
int thueMorse(int n)
{
	return popcount(n) & 1;
}

// fold x back into [lo, hi] by reflecting at the ends (a triangle wave)
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

// frequency of scale-degree d (degrees above NDEG wrap up by octaves)
float degFreq(int d)
{
	if (d < 0)
		d = 0;
	int oct  = d / NDEG;
	int step = d % NDEG;
	int semi = scaleSemi[step] + 12 * oct;
	return ROOT_HZ * pow(2.0, semi / 12.0);
}

// one white-noise sample in [-1, 1]
float noiseSample()
{
	rndState = rndState * 1103515245 + 12345;
	return ((rndState >> 16) & 0x7fff) / 16383.5 - 1.0;
}

// render one mono sample of a pitched voice
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

	// envelope: linear attack, then a smooth (1-u)^2 decay to the note end
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
		// 2-operator FM; the modulation index fades with the note (brightness
		// decays like a plucked/struck tone)
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
	// pitch drops quickly from ~145 Hz to ~45 Hz -> a punchy "boom"
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
	float tone = sin(TWO_PI * 185.0 * t) * 0.4;   // a little body under the noise
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
	return noiseSample() * env * hatAmp;   // short bright noise burst
}

// The sequencer: called once at the start of every 16th-note step. It reads
// the fractal generator at each track's own time scale and assigns new notes.
void onStep(int step)
{
	int bar = step / 16;
	int s16 = step % 16;

	// LEAD -- a new note every 2 steps (8th notes). Thue-Morse decides whether
	// the melody steps up or down one degree of the scale.
	if (s16 % 2 == 0)
	{
		int dir = thueMorse(step) ? 1 : -1;
		melPos = reflect(melPos + dir, 0, LEAD_RANGE);
		vLead.freq  = degFreq(melPos + 2 * NDEG);   // two octaves up
		vLead.start = gSample;
		vLead.dur   = 2.0 * stepSamples / float(SR) * 0.90;
	}

	// HARMONY -- every 4 steps (quarter notes), a 3rd below the lead. Sharing
	// the lead's walk keeps the two lines moving in parallel and in key.
	if (s16 % 4 == 0)
	{
		int hd = melPos - 2;
		if (hd < 0)
			hd += NDEG;
		vHarm.freq  = degFreq(hd + NDEG);           // one octave up (mid range)
		vHarm.start = gSample;
		vHarm.dur   = 4.0 * stepSamples / float(SR) * 0.95;
	}

	// BASS -- every 8 steps (half notes), its own slow Thue-Morse walk.
	if (s16 % 8 == 0)
	{
		int dir = thueMorse(bar) ? 1 : -1;
		bassPos = reflect(bassPos + dir, 0, BASS_RANGE);
		vBass.freq  = degFreq(bassPos);             // low octave
		vBass.start = gSample;
		vBass.dur   = 8.0 * stepSamples / float(SR) * 0.98;
	}

	// DRUMS -- a steady backbone with fractal ghost notes.
	if (s16 == 0 || s16 == 8)          // kick on the down-beats
	{
		kickStart = gSample; kickAmp = 0.60;
	}
	else if (s16 == 14 && thueMorse(step))   // syncopated ghost kick
	{
		kickStart = gSample; kickAmp = 0.32;
	}
	if (s16 == 4 || s16 == 12)         // snare on the back-beats
	{
		snareStart = gSample; snareAmp = 0.40;
	}
	// closed hi-hat every step, accented on the Thue-Morse ones
	hatStart = gSample;
	hatAmp = thueMorse(step) ? 0.18 : 0.09;

	// once per bar, print what the fractal generator is doing
	if (s16 == 0)
	{
		int deg  = melPos + 2 * NDEG;
		int semi = scaleSemi[deg % NDEG] + 12 * (deg / NDEG);
		printf("bar %3i | lead %s %6.1fHz  harm %6.1fHz  bass %6.1fHz\n",
			bar, NOTE[semi % 12], vLead.freq, vHarm.freq, vBass.freq);
		fflush(stdout);
	}
}

int main()
{
	// --- init voices ---
	vLead.freq = 0.0; vLead.amp = 0.26; vLead.atk = 0.003;
	vLead.fm = 2.2; vLead.ratio = 2.0; vLead.pan = 0.63; vLead.start = -1000000;
	vLead.dur = 0.2;

	vHarm.freq = 0.0; vHarm.amp = 0.14; vHarm.atk = 0.030;
	vHarm.fm = 0.0; vHarm.ratio = 1.0; vHarm.pan = 0.37; vHarm.start = -1000000;
	vHarm.dur = 0.4;

	vBass.freq = 0.0; vBass.amp = 0.30; vBass.atk = 0.006;
	vBass.fm = 0.8; vBass.ratio = 1.0; vBass.pan = 0.50; vBass.start = -1000000;
	vBass.dur = 0.4;

	kickAmp = 0.0;  kickStart  = -1000000;
	snareAmp = 0.0; snareStart = -1000000;
	hatAmp = 0.0;   hatStart   = -1000000;

	// --- init transport ---
	gSample = 0;
	gStep = 0;
	stepCounter = 0;
	melPos = 4;      // start the lead in the middle of its range
	bassPos = 0;
	stepSamples = int(15.0 / BPM * SR);   // a 16th note = quarter / 4 = 15/BPM s

	// --- init delay (a dotted-8th ping-pong echo) ---
	delayLen = stepSamples * 3;
	delayPos = 0;
	delayBuf = (float*)malloc(sizeof(float) * 2 * delayLen);
	for (int i = 0; i < 2 * delayLen; i++)
		delayBuf[i] = 0.0;

	printf("fractal_melody: a 4-track generative synth for Scinc\n");
	printf("  track 0  LEAD    - Thue-Morse fractal melody (8th notes, 2-op FM)\n");
	printf("  track 1  HARMONY - a 3rd below the lead      (quarter notes)\n");
	printf("  track 2  BASS    - slow fractal walk         (half notes)\n");
	printf("  track 3  DRUMS   - kick / snare / hi-hat      (16th grid)\n");
	printf("  scale: A minor pentatonic  |  tempo: %i BPM  |  Ctrl-C to stop\n\n", BPM);
	fflush(stdout);

	float duration = 0.0;   // seconds to play; 0 = run forever
	float t0 = Time();
	while (duration <= 0.0 || Time() - t0 < duration)
	{
		snd.Poll();
		int guard = 0;
		while (snd.snd_bufhealth() < BUF_LOW && guard < 64)
		{
			for (int i = 0; i < CHUNK; i++)
			{
				// advance the sequencer at each 16th-note boundary
				if (stepCounter == 0)
				{
					onStep(gStep);
					gStep++;
				}

				// mix the four tracks
				float lead = renderVoice(vLead);
				float harm = renderVoice(vHarm);
				float bass = renderVoice(vBass);
				float drum = renderKick() + renderSnare() + renderHat();

				float l = lead * (1.0 - vLead.pan) + harm * (1.0 - vHarm.pan)
						+ bass * 0.5 + drum * 0.5;
				float r = lead * vLead.pan + harm * vHarm.pan
						+ bass * 0.5 + drum * 0.5;

				// ping-pong feedback delay
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

				// keep the master within range
				if (outL > 0.95) outL = 0.95; else if (outL < -0.95) outL = -0.95;
				if (outR > 0.95) outR = 0.95; else if (outR < -0.95) outR = -0.95;

				snd.snd_out(outL, outR);

				gSample++;
				stepCounter++;
				if (stepCounter >= stepSamples)
					stepCounter = 0;
			}
			guard++;
		}
	}
	return 0;
}
