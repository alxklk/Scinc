// flurry.cpp - Scinc port of the "Flurry" screensaver (xscreensaver
// hacks/glx/flurry*.c, originally a Mac OS X screensaver by Calum Robinson).
//
// A "star" tumbles through space along a looping path; each of N streams is
// a "spark" orbiting on its own path. Every frame each stream releases a
// puff of smoke at the star, thrown towards its spark; every puff then feels
// the gravity of all sparks, is slowed by drag and widens as it ages. Puffs
// are drawn as quads stretched along their motion, textured with an animated
// 8x8 atlas of speckled smoke tiles and added (GL_SRC_ALPHA, GL_ONE) onto the
// previous frame, which fades a little each frame into long glowing trails.
//
// This follows the scalar code path of the original line by line (physics,
// colour modes, presets, texture generation). One addition: the original
// relies on the back buffer surviving the buffer swap, which modern drivers
// and compositors don't guarantee, so here every finished frame is copied
// into a texture (glCopyTexSubImage2D) and drawn back as the next frame's
// background before it is faded.
//
//   ./build_so.sh           (once, builds scgl_dl.so)
//   Scinc flurry.cpp
//
// Keys: 1 water, 2 fire, 3 psychedelic, 4 rgb, 5 binary, 6 classic,
//       7 insane (64 streams: heavy), R random preset, F fullscreen,
//       S save flurry.bmp, Esc/Q quit.
//
// Original copyright notice (the port keeps the original's license):
//
// Copyright (c) 2002, Calum Robinson
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the author nor the names of its contributors may be
//   used to endorse or promote products derived from this software without
//   specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <stdio.h>
#include "scgl.h"

#define PI 3.14159265358979

// ---- constants of the original (flurry.h, flurry-smoke.c) ----
#define NUMSMOKEPARTICLES 3600
#define MAX_SPARKS 64
#define MAXFLURRIES 9            // the "water" preset uses 9
#define MAXANGLES 16384.0
#define BIGMYSTERY 1800.0

#define GRAVITY 1500000.0
#define INCOHESION 0.07
#define COLORINCOHERENCE 0.15
#define STREAMSPEED 450.0
#define FIELDSPEED 12.0
#define SERAPHDISTANCE 2000.0
#define STREAMSIZE 25000.0
#define FIELDRANGE 1000.0
#define STREAMBIAS 7.0

// colour modes
#define RED_MODE 0
#define MAGENTA_MODE 1
#define BLUE_MODE 2
#define CYAN_MODE 3
#define GREEN_MODE 4
#define YELLOW_MODE 5
#define SLOWCYCLIC_MODE 6
#define CYCLIC_MODE 7
#define TIEDYE_MODE 8
#define RAINBOW_MODE 9
#define WHITE_MODE 10
#define MULTI_MODE 11
#define DARK_MODE 12

// ---- random numbers (the original's random() / frand()) ----
int seed = 1;
int irand()
{
	seed = seed * 1103515245 + 12345;
	return (seed >> 8) & 0x7fffff;
}
float frand(float x)                 // [0, x)
{
	return x * (irand() / 8388608.0);
}
float RandFlt(float lo, float hi)
{
	return lo + frand(hi - lo);
}
float RandBell(float scale)
{
	return scale * (-(frand(.5) + frand(.5) + frand(.5)));
}
float Max(float a, float b)
{
	if (a > b) return a;
	return b;
}

// Distance from (0,0) to (x,y) with ~3.5% error, as in the original.
float FastDistance2D(float x, float y)
{
	if (x < 0.0) x = -x;
	if (y < 0.0) y = -y;
	float mn = x;
	if (y < mn) mn = y;
	return x + y - (mn * 0.5) - (mn * 0.25) + (mn * 0.0625);
}

// ---- one flurry: settings, star, sparks, smoke ----
int nflurries = 0;
int fStreams[MAXFLURRIES];
int fColorMode[MAXFLURRIES];
float fExpansion[MAXFLURRIES];
float fBrite[MAXFLURRIES];
double fSeed[MAXFLURRIES];
double fTime[MAXFLURRIES];
double fOldTime[MAXFLURRIES];
double fDeltaTime[MAXFLURRIES];
float fDrag[MAXFLURRIES];
int fDframe[MAXFLURRIES];

// star
float stX[MAXFLURRIES];
float stY[MAXFLURRIES];
float stZ[MAXFLURRIES];
float stMystery[MAXFLURRIES];
float stRotSpeed[MAXFLURRIES];

// sparks, index f * MAX_SPARKS + i
float spX[MAXFLURRIES * MAX_SPARKS];
float spY[MAXFLURRIES * MAX_SPARKS];
float spZ[MAXFLURRIES * MAX_SPARKS];
float spR[MAXFLURRIES * MAX_SPARKS];
float spG[MAXFLURRIES * MAX_SPARKS];
float spB[MAXFLURRIES * MAX_SPARKS];
int spMystery[MAXFLURRIES * MAX_SPARKS];

// smoke emitter
int smNext[MAXFLURRIES];
float smLastTime[MAXFLURRIES];
int smFirst[MAXFLURRIES];
float smOldX[MAXFLURRIES];
float smOldY[MAXFLURRIES];
float smOldZ[MAXFLURRIES];

// smoke particles, index f * NUMSMOKEPARTICLES + p
#define NPALL (MAXFLURRIES * NUMSMOKEPARTICLES)
float pX[NPALL];
float pY[NPALL];
float pZ[NPALL];
float pOX[NPALL];                    // position last frame
float pOY[NPALL];
float pOZ[NPALL];
float pDX[NPALL];                    // velocity
float pDY[NPALL];
float pDZ[NPALL];
float pR[NPALL];
float pG[NPALL];
float pB[NPALL];
float pA[NPALL];
float pTime[NPALL];                  // birth time
int pDead[NPALL];
int pAnim[NPALL];                    // texture atlas frame 0..63

// quad arrays for one glDrawArrays per flurry
float qVert[NUMSMOKEPARTICLES * 8];
float qCol[NUMSMOKEPARTICLES * 16];
float qTex[NUMSMOKEPARTICLES * 8];

float glWidth = 1024.0;
float glHeight = 768.0;
double startTime = 0.0;

double TimeSinceStart()
{
	return glwTime() - startTime;
}

// ---- star (flurry-star.c) ----
void UpdateStar(int f)
{
	float rotationsPerSecond = (2.0 * PI * 12.0 / MAXANGLES) * stRotSpeed[f];
	double thisAngle = fTime[f] * rotationsPerSecond;
	float cf = cos(7.0 * thisAngle) + cos(3.0 * thisAngle) + cos(13.0 * thisAngle);
	cf = cf / 6.0 + 0.75;
	double p = 2.0 * PI * stMystery[f] / BIGMYSTERY;

	double x = 250.0 * cf * cos(11.0 * (p + 3.0 * thisAngle));
	double y = 250.0 * cf * sin(12.0 * (p + 4.0 * thisAngle));
	double z = 250.0 * cos(23.0 * (p + 12.0 * thisAngle));

	double rotation = thisAngle * 0.501 + 5.01 * stMystery[f] / BIGMYSTERY;
	double cr = cos(rotation);
	double sr = sin(rotation);
	double x1 = x * cr - y * sr;
	double y1 = y * cr + x * sr;
	double z1 = z;
	double x2 = x1 * cr - z1 * sr;
	double y2 = y1;
	double z2 = z1 * cr + x1 * sr;
	double x3 = x2;
	double y3 = y2 * cr - z2 * sr;
	double z3 = z2 * cr + y2 * sr + SERAPHDISTANCE;
	rotation = thisAngle * 2.501 + 85.01 * stMystery[f] / BIGMYSTERY;
	cr = cos(rotation);
	sr = sin(rotation);
	stX[f] = x3 * cr - y3 * sr;
	stY[f] = y3 * cr + x3 * sr;
	stZ[f] = z3;
}

// ---- sparks (flurry-spark.c) ----
void UpdateSpark(int f, int i)
{
	int k = f * MAX_SPARKS + i;
	float rotationsPerSecond = 2.0 * PI * FIELDSPEED / MAXANGLES;
	double thisAngle = fTime[f] * rotationsPerSecond;
	int mode = fColorMode[f];
	float cycleTime = 20.0;
	if (mode == RAINBOW_MODE) cycleTime = 1.5;
	else if (mode == TIEDYE_MODE) cycleTime = 4.5;
	else if (mode == CYCLIC_MODE) cycleTime = 20.0;
	else if (mode == SLOWCYCLIC_MODE) cycleTime = 120.0;
	float colorRot = 2.0 * PI / cycleTime;
	float greenPhaseShift = cycleTime / 3.0;
	float bluePhaseShift = cycleTime * 2.0 / 3.0;
	float colorTime = fTime[f];
	float baseRed;
	float baseGreen;
	float baseBlue;
	if (mode == WHITE_MODE)
	{
		baseRed = 0.1875;
		baseGreen = 0.1875;
		baseBlue = 0.1875;
	}
	else if (mode == MULTI_MODE)
	{
		baseRed = 0.0625;
		baseGreen = 0.0625;
		baseBlue = 0.0625;
	}
	else if (mode == DARK_MODE)
	{
		baseRed = 0.0;
		baseGreen = 0.0;
		baseBlue = 0.0;
	}
	else
	{
		if (mode < SLOWCYCLIC_MODE)
			colorTime = (mode / 6.0) * cycleTime;      // fixed hue modes
		else
			colorTime = fTime[f] + fSeed[f];
		baseRed = 0.109375 * (cos(colorTime * colorRot) + 1.0);
		baseGreen = 0.109375 * (cos((colorTime + greenPhaseShift) * colorRot) + 1.0);
		baseBlue = 0.109375 * (cos((colorTime + bluePhaseShift) * colorRot) + 1.0);
	}

	float oldX = spX[k];
	float oldY = spY[k];
	float oldZ = spZ[k];

	float cf = cos(7.0 * thisAngle) + cos(3.0 * thisAngle) + cos(13.0 * thisAngle);
	cf = cf / 6.0 + 2.0;
	double p = 2.0 * PI * spMystery[k] / BIGMYSTERY;

	spR[k] = baseRed + 0.0625 * (0.5 + cos(15.0 * (p + 3.0 * thisAngle)) + sin(7.0 * (p + thisAngle)));
	spG[k] = baseGreen + 0.0625 * (0.5 + sin(p + thisAngle));
	spB[k] = baseBlue + 0.0625 * (0.5 + cos(37.0 * (p + thisAngle)));

	double x = FIELDRANGE * cf * cos(11.0 * (p + 3.0 * thisAngle));
	double y = FIELDRANGE * cf * sin(12.0 * (p + 4.0 * thisAngle));
	double z = FIELDRANGE * cos(23.0 * (p + 12.0 * thisAngle));

	double rotation = thisAngle * 0.501 + 5.01 * spMystery[k] / BIGMYSTERY;
	double cr = cos(rotation);
	double sr = sin(rotation);
	double x1 = x * cr - y * sr;
	double y1 = y * cr + x * sr;
	double z1 = z;
	double x2 = x1 * cr - z1 * sr;
	double y2 = y1;
	double z2 = z1 * cr + x1 * sr;
	double x3 = x2;
	double y3 = y2 * cr - z2 * sr;
	double z3 = z2 * cr + y2 * sr + SERAPHDISTANCE;
	rotation = thisAngle * 2.501 + 85.01 * spMystery[k] / BIGMYSTERY;
	cr = cos(rotation);
	sr = sin(rotation);
	spX[k] = x3 * cr - y3 * sr;          // fieldCoherence is 0: no jitter
	spY[k] = y3 * cr + x3 * sr;
	spZ[k] = z3;
}

// ---- smoke (flurry-smoke.c, scalar path) ----
void UpdateSmoke(int f)
{
	int base = f * NUMSMOKEPARTICLES;
	int sb = f * MAX_SPARKS;
	int streams = fStreams[f];
	float sx = stX[f];
	float sy = stY[f];
	float sz = stZ[f];

	if (!smFirst[f])
	{
		// release one puff per stream (at most 121 times a second)
		if (fTime[f] - smLastTime[f] >= 1.0 / 121.0)
		{
			float deltax = (smOldX[f] - sx) * 5.0;
			float deltay = (smOldY[f] - sy) * 5.0;
			float deltaz = (smOldZ[f] - sz) * 5.0;
			for (int i = 0; i < streams; i++)
			{
				int n = base + smNext[f];
				int s = sb + i;
				pDX[n] = deltax;
				pDY[n] = deltay;
				pDZ[n] = deltaz;
				pX[n] = sx;
				pY[n] = sy;
				pZ[n] = sz;
				pOX[n] = sx;
				pOY[n] = sy;
				pOZ[n] = sz;
				float coherence = Max(0.0, 1.0 + RandBell(0.25 * INCOHESION));
				float dx = sx - spX[s];
				float dy = sy - spY[s];
				float dz = sz - spZ[s];
				float mag = STREAMSPEED * coherence / sqrt(dx * dx + dy * dy + dz * dz);
				pDX[n] = pDX[n] - dx * mag;
				pDY[n] = pDY[n] - dy * mag;
				pDZ[n] = pDZ[n] - dz * mag;
				pR[n] = spR[s] * (1.0 + RandBell(COLORINCOHERENCE));
				pG[n] = spG[s] * (1.0 + RandBell(COLORINCOHERENCE));
				pB[n] = spB[s] * (1.0 + RandBell(COLORINCOHERENCE));
				pA[n] = 0.85 * (1.0 + RandBell(0.5 * COLORINCOHERENCE));
				pTime[n] = fTime[f];
				pDead[n] = 0;
				pAnim[n] = irand() & 63;
				smNext[f] = smNext[f] + 1;
				if (smNext[f] >= NUMSMOKEPARTICLES)
					smNext[f] = 0;
			}
			smLastTime[f] = fTime[f];
		}
	}
	else
	{
		smLastTime[f] = fTime[f];
		smFirst[f] = 0;
	}
	smOldX[f] = sx;
	smOldY[f] = sy;
	smOldZ[f] = sz;

	float frameRate = fDframe[f] / fTime[f];
	float frameRateModifier = 42.5 / frameRate;
	float drag = fDrag[f];
	float dt = fDeltaTime[f];

	for (int p = 0; p < NUMSMOKEPARTICLES; p++)
	{
		int n = base + p;
		if (pDead[n])
			continue;
		float deltax = pDX[n];
		float deltay = pDY[n];
		float deltaz = pDZ[n];
		float px = pX[n];
		float py = pY[n];
		float pz = pZ[n];
		int own = p % streams;
		for (int j = 0; j < streams; j++)
		{
			int s = sb + j;
			float dx = px - spX[s];
			float dy = py - spY[s];
			float dz = pz - spZ[s];
			float rsquared = dx * dx + dy * dy + dz * dz;
			float g = (GRAVITY / rsquared) * frameRateModifier;
			if (j == own)
				g = g * (1.0 + STREAMBIAS);      // each puff favours its own spark
			float mag = g / sqrt(rsquared);
			deltax = deltax - dx * mag;
			deltay = deltay - dy * mag;
			deltaz = deltaz - dz * mag;
		}
		deltax = deltax * drag;
		deltay = deltay * drag;
		deltaz = deltaz * drag;
		if (deltax * deltax + deltay * deltay + deltaz * deltaz >= 25000000.0)
		{
			pDead[n] = 1;
			continue;
		}
		pDX[n] = deltax;
		pDY[n] = deltay;
		pDZ[n] = deltaz;
		pOX[n] = px;
		pOY[n] = py;
		pOZ[n] = pz;
		pX[n] = px + deltax * dt;
		pY[n] = py + deltay * dt;
		pZ[n] = pz + deltaz * dt;
	}
}

void DrawSmoke(int f, float brightness)
{
	int base = f * NUMSMOKEPARTICLES;
	float screenRatio = glWidth / 1024.0;
	float hslash2 = glHeight * 0.5;
	float wslash2 = glWidth * 0.5;
	float expansion = fExpansion[f];
	float width = (STREAMSIZE + 2.5 * expansion) * screenRatio;
	int si = 0;
	int vi = 0;
	int ci = 0;

	for (int p = 0; p < NUMSMOKEPARTICLES; p++)
	{
		int n = base + p;
		if (pDead[n])
			continue;
		float thisWidth = (STREAMSIZE + (fTime[f] - pTime[n]) * expansion) * screenRatio;
		if (thisWidth >= width)
		{
			pDead[n] = 1;
			continue;
		}
		float z = pZ[n];
		float sx = pX[n] * glWidth / z + wslash2;
		float sy = pY[n] * glWidth / z + hslash2;
		float oldz = pOZ[n];
		if (sx > glWidth + 50.0 || sx < -50.0 || sy > glHeight + 50.0 || sy < -50.0 || z < 25.0 || oldz < 25.0)
			continue;

		float w = Max(1.0, thisWidth / z);
		float oldscreenx = (pOX[n] * glWidth / oldz) + wslash2;
		float oldscreeny = (pOY[n] * glWidth / oldz) + hslash2;
		float dx = sx - oldscreenx;
		float dy = sy - oldscreeny;
		float d = FastDistance2D(dx, dy);
		float sm = 0.0;
		float os = 0.0;
		float ow = Max(1.0, thisWidth / oldz);
		if (d != 0.0)
		{
			sm = w / d;
			os = ow / d;
		}
		float m = 1.0 + sm;
		float dxs = dx * sm;
		float dys = dy * sm;
		float dxos = dx * os;
		float dyos = dy * os;
		float dxm = dx * m;
		float dym = dy * m;

		int anim = pAnim[n] + 1;
		if (anim >= 64)
			anim = 0;
		pAnim[n] = anim;
		float u0 = (anim & 7) * 0.125;
		float v0 = (anim >> 3) * 0.125;
		float u1 = u0 + 0.125;
		float v1 = v0 + 0.125;
		float cm = (1.375 - thisWidth / width) * brightness;
		float cr = pR[n] * cm;
		float cg = pG[n] * cm;
		float cb = pB[n] * cm;
		float ca = pA[n] * cm;
		for (int q = 0; q < 4; q++)
		{
			qCol[ci] = cr;
			qCol[ci + 1] = cg;
			qCol[ci + 2] = cb;
			qCol[ci + 3] = ca;
			ci = ci + 4;
		}
		qTex[vi] = u0;
		qTex[vi + 1] = v0;
		qTex[vi + 2] = u0;
		qTex[vi + 3] = v1;
		qTex[vi + 4] = u1;
		qTex[vi + 5] = v1;
		qTex[vi + 6] = u1;
		qTex[vi + 7] = v0;
		// a quad from the old screen position to the new one, as wide as the puff
		qVert[vi] = sx + dxm - dys;
		qVert[vi + 1] = sy + dym + dxs;
		qVert[vi + 2] = sx + dxm + dys;
		qVert[vi + 3] = sy + dym - dxs;
		qVert[vi + 4] = oldscreenx - dxm + dyos;
		qVert[vi + 5] = oldscreeny - dym - dxos;
		qVert[vi + 6] = oldscreenx - dxm - dyos;
		qVert[vi + 7] = oldscreeny - dym + dxos;
		vi = vi + 8;
		si++;
	}
	if (si > 0)
	{
		glwVertexArray(2, qVert);
		glwColorArray(4, qCol);
		glwTexCoordArray(2, qTex);
		glDrawArrays(GL_QUADS, 0, si * 4);
		glwDisableArrays();
	}
}

// ---- flurry setup and frame (flurry.c) ----
void NewFlurry(int streams, int colour, float thickness, float speed, float bf)
{
	if (nflurries >= MAXFLURRIES)
		return;
	int f = nflurries;
	nflurries++;
	fSeed[f] = RandFlt(0.0, 300.0);
	fOldTime[f] = 0.0;
	fDframe[f] = 0;
	fTime[f] = TimeSinceStart() + fSeed[f];
	fDeltaTime[f] = fTime[f] - fOldTime[f];
	fStreams[f] = streams;
	fExpansion[f] = thickness;
	fColorMode[f] = colour;
	fBrite[f] = bf;

	smNext[f] = 0;
	smLastTime[f] = 0.25;
	smFirst[f] = 1;
	smOldX[f] = RandFlt(-100.0, 100.0);
	smOldY[f] = RandFlt(-100.0, 100.0);
	smOldZ[f] = RandFlt(-100.0, 100.0);

	RandFlt(-10000.0, 10000.0);            // InitStar's (overwritten) position
	RandFlt(-10000.0, 10000.0);
	RandFlt(-10000.0, 10000.0);
	RandFlt(0.4, 0.9);                     // its rotSpeed, replaced by 'speed'
	stMystery[f] = RandFlt(0.0, 10.0);
	stRotSpeed[f] = speed;

	for (int i = 0; i < MAX_SPARKS; i++)
	{
		int k = f * MAX_SPARKS + i;
		spX[k] = RandFlt(-100.0, 100.0);
		spY[k] = RandFlt(-100.0, 100.0);
		spZ[k] = RandFlt(-100.0, 100.0);
		spMystery[k] = 1800 * (i + 1) / 13;
		UpdateSpark(f, i);
	}
	for (int p = 0; p < NUMSMOKEPARTICLES; p++)
		pDead[f * NUMSMOKEPARTICLES + p] = 1;
}

void RenderFlurry(int f, float b)
{
	fDframe[f] = fDframe[f] + 1;
	fOldTime[f] = fTime[f];
	fTime[f] = TimeSinceStart() + fSeed[f];
	fDeltaTime[f] = fTime[f] - fOldTime[f];
	fDrag[f] = pow(0.9965, fDeltaTime[f] * 85.0);
	UpdateStar(f);
	for (int i = 0; i < fStreams[f]; i++)
		UpdateSpark(f, i);
	UpdateSmoke(f);
	DrawSmoke(f, b);
}

char* presetName = "";

void SetPreset(int n)
{
	nflurries = 0;
	if (n == 0)
	{
		presetName = "water";
		for (int i = 0; i < 9; i++)
			NewFlurry(1, BLUE_MODE, 100.0, 2.0, 2.0);
	}
	else if (n == 1)
	{
		presetName = "fire";
		NewFlurry(12, SLOWCYCLIC_MODE, 10000.0, 0.2, 1.0);
	}
	else if (n == 2)
	{
		presetName = "psychedelic";
		NewFlurry(10, RAINBOW_MODE, 200.0, 2.0, 1.0);
	}
	else if (n == 3)
	{
		presetName = "rgb";
		NewFlurry(3, RED_MODE, 100.0, 0.8, 1.0);
		NewFlurry(3, GREEN_MODE, 100.0, 0.8, 1.0);
		NewFlurry(3, BLUE_MODE, 100.0, 0.8, 1.0);
	}
	else if (n == 4)
	{
		presetName = "binary";
		NewFlurry(16, TIEDYE_MODE, 1000.0, 0.5, 1.0);
		NewFlurry(16, TIEDYE_MODE, 1000.0, 1.5, 1.0);
	}
	else if (n == 5)
	{
		presetName = "classic";
		NewFlurry(5, TIEDYE_MODE, 10000.0, 1.0, 1.0);
	}
	else
	{
		presetName = "insane";
		NewFlurry(64, TIEDYE_MODE, 1000.0, 0.5, 0.5);
	}
}

#include "flurry_texture.h"

// ---- frame feedback: the previous frame lives in a screen-sized texture ----
int frameTex = 0;

void AllocFrameTexture()
{
	glBindTexture(GL_TEXTURE_2D, frameTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, int(glWidth), int(glHeight), 0);
}

void DrawPreviousFrame()
{
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, frameTex);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex2f(0.0, 0.0);
	glTexCoord2f(1.0, 0.0);
	glVertex2f(glWidth, 0.0);
	glTexCoord2f(1.0, 1.0);
	glVertex2f(glWidth, glHeight);
	glTexCoord2f(0.0, 1.0);
	glVertex2f(0.0, glHeight);
	glEnd();
	glDisable(GL_TEXTURE_2D);
}

void Reshape(int w, int h)
{
	glWidth = w;
	glHeight = h;
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, w, 0.0, h, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	AllocFrameTexture();
}

int main()
{
	if (!glwInit(1024, 768, "Scinc Flurry"))
	{
		printf("Cannot open OpenGL window\n");
		return 1;
	}
	seed = int(glwTime() * 1000.0) + 12345;
	startTime = glwTime();

	// GLSetupRC
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	int smokeTex = MakeFlurryTexture();
	frameTex = glwGenTexture();

	SetPreset(irand() % 6);                 // "random" chooses among the first six

	double oldFrameTime = -1.0;
	int fullscreen = 0;
	int saveNext = 0;
	int running = 1;
	int frames = 0;
	double tRate0 = glwTime();
	float fps = 0.0;

	while (running && glwPoll())
	{
		int key = glwGetKey();
		while (key != 0)
		{
			if (key == GLW_KEY_ESC || key == 'q' || key == 'Q')
				running = 0;
			else if (key >= '1' && key <= '7')
			{
				SetPreset(key - '1');
				AllocFrameTexture();            // start from black
			}
			else if (key == 'r' || key == 'R')
			{
				SetPreset(irand() % 6);
				AllocFrameTexture();
			}
			else if (key == 'f' || key == 'F')
			{
				fullscreen = 1 - fullscreen;
				glwSetFullscreen(fullscreen);
			}
			else if (key == 's' || key == 'S')
				saveNext = 1;
			key = glwGetKey();
		}
		if (glwResized())
			Reshape(glwWidth(), glwHeight());

		DrawPreviousFrame();

		// Flurry is designed for <= 60 fps: faster, the additive blending
		// saturates. On faster displays the in-between frames just show the
		// previous image again.
		double newFrameTime = glwTime();
		if (oldFrameTime < 0.0 || newFrameTime - oldFrameTime >= 1.0 / 60.0)
		{
			double deltaFrameTime = 0.0;
			float alpha = 1.0;                  // first frame: clear to black
			if (oldFrameTime >= 0.0)
			{
				deltaFrameTime = newFrameTime - oldFrameTime;
				alpha = 5.0 * deltaFrameTime;
			}
			oldFrameTime = newFrameTime;
			if (alpha > 0.2)
				alpha = 0.2;

			// fade the previous frame towards black
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glColor4f(0.0, 0.0, 0.0, alpha);
			glBegin(GL_QUADS);
			glVertex2f(0.0, 0.0);
			glVertex2f(glWidth, 0.0);
			glVertex2f(glWidth, glHeight);
			glVertex2f(0.0, glHeight);
			glEnd();

			// add the smoke
			float brite = pow(deltaFrameTime, 0.75) * 10.0;
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, smokeTex);
			for (int f = nflurries - 1; f >= 0; f--)   // the original's list is newest-first
				RenderFlurry(f, brite * fBrite[f]);
			glDisable(GL_TEXTURE_2D);
			glDisable(GL_BLEND);

			// keep this frame for the next one
			glBindTexture(GL_TEXTURE_2D, frameTex);
			glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, int(glWidth), int(glHeight));
			frames++;
		}

		if (saveNext)
		{
			if (glwSaveBMP("flurry.bmp"))
				printf("wrote flurry.bmp\n");
			fflush(stdout);
			saveNext = 0;
		}
		glwSwap();

		double now = glwTime();
		if (now - tRate0 >= 1.0)
		{
			fps = frames / (now - tRate0);
			char title[96];
			snprintf(title, 96, "Scinc Flurry - %s - %.1f FPS", presetName, fps);
			glwSetTitle(title);
			tRate0 = now;
			frames = 0;
		}
	}

	glwDone();
	return 0;
}
