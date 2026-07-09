#include "sound.h"
#include <stdio.h>

// Simple white-noise generator for Scinc.
//
// Scinc has no rand(), so we use a small glibc-style linear congruential
// generator (LCG) for the pseudo-random samples. Audio goes out through the
// intrinsic CSound API, exactly like the other examples in this folder:
//   snd.Poll()           - service the audio device
//   snd.snd_bufhealth()  - how many samples are queued; keep it topped up
//   snd.snd_out(l, r)    - push one stereo sample, each channel in [-1, 1]

int rndState = 1;

int nextRand()                 // pseudo-random int in [0, 32767]
{
	rndState = rndState * 1103515245 + 12345;
	return (rndState >> 16) & 0x7fff;
}

float noiseSample(float amp)   // one white-noise sample in [-amp, amp]
{
	return (nextRand() / 16383.5 - 1.0) * amp;
}

CSound snd;

#define BUF_LOW 3000           // keep at least this many samples queued
#define CHUNK   1024           // samples generated per top-up

int main()
{
	float amp = 0.25;          // 0..1 loudness; kept modest so it isn't harsh
	float duration = 3.0;      // seconds; set to 0 to run until interrupted

	float t0 = Time();
	while (duration <= 0.0 || Time() - t0 < duration)
	{
		snd.Poll();
		while (snd.snd_bufhealth() < BUF_LOW)
			for (int i = 0; i < CHUNK; i++)
			{
				float l = noiseSample(amp);   // independent left / right
				float r = noiseSample(amp);   // channels -> wide stereo hiss
				snd.snd_out(l, r);
			}
	}
	printf("white noise: played %.1f s\n", duration);
	return 0;
}
