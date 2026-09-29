// ophiura_flurry.cpp - the brittle star of ../../geometry/ophiura.cpp drawn
// in the style of Flurry (see flurry.cpp).
//
// The arms wriggle exactly as in the original (same arm walk, lengths and
// colours), but every bead is a glowing Flurry smoke puff: a quad textured
// with Flurry's animated smoke atlas, stretched from where the bead was last
// frame to where it is now, and added (GL_SRC_ALPHA, GL_ONE) onto the
// previous frame, which fades a little every frame - so the moving arms
// leave light trails. A brighter, narrower core runs along each arm, and
// the arm tips shed smoke puffs that drift off with the tip's motion,
// widen and fade like Flurry's streams.
//
//   ./build_so.sh           (once, builds scgl_dl.so)
//   Scinc ophiura_flurry.cpp
//
// Keys: - / + arms (3..10), [ / ] trail length, T tip smoke on/off,
//       space pause, C clear, F fullscreen, S save ophiura_flurry.bmp,
//       Esc/Q quit.

#include <stdio.h>
#include "scgl.h"
#include "flurry_texture.h"

#define MAXARMS 10
#define DENSITY 2                  // beads per step of the original
#define MAXBEADS 310               // per arm: 30 / (0.2 / DENSITY) + 1, rounded up
#define MAXPUFFS 2400              // tip smoke particles
#define PUFFLIFE 1.8

// ---- the original's arm lengths ----
int seed = 374693645;

int irand(int& seed)
{
	seed = (seed * 1103515245 + 12345) % 0x7ffffff;
	return seed;
}

double frand(int& seed)
{
	return (irand(seed) >> 11) / 65536.0;
}

int rs = 99991;                    // random numbers for the smoke
float rnd()
{
	rs = rs * 1103515245 + 12345;
	return ((rs >> 8) & 0x7fffff) / 8388608.0;
}

float ls[MAXARMS];
int arms = 6;
float t = 0.0;                     // the original's time (t = Time() * 2)

// ---- bead positions, now and last frame (logical 320x240 coordinates) ----
float bx[MAXARMS * MAXBEADS];
float by[MAXARMS * MAXBEADS];
float oldBx[MAXARMS * MAXBEADS];
float oldBy[MAXARMS * MAXBEADS];
float bw[MAXARMS * MAXBEADS];      // puff half-width
float bfw[MAXARMS * MAXBEADS];     // the original's band factor fw
float bcr[MAXARMS * MAXBEADS];
float bcg[MAXARMS * MAXBEADS];
float bcb[MAXARMS * MAXBEADS];
int bAnim[MAXARMS * MAXBEADS];
int armBeads[MAXARMS];
int haveOld = 0;

// ---- tip smoke ----
float sX[MAXPUFFS];
float sY[MAXPUFFS];
float sOX[MAXPUFFS];
float sOY[MAXPUFFS];
float sVX[MAXPUFFS];
float sVY[MAXPUFFS];
float sAge[MAXPUFFS];
float sW[MAXPUFFS];
float sR[MAXPUFFS];
float sG[MAXPUFFS];
float sB[MAXPUFFS];
int sAnim[MAXPUFFS];
int sAlive[MAXPUFFS];
int nextPuff = 0;
int tipSmoke = 1;

// ---- quads for one glDrawArrays ----
#define MAXQUADS (MAXARMS * MAXBEADS * 2 + MAXPUFFS)
float qVert[MAXQUADS * 8];
float qCol[MAXQUADS * 16];
float qTex[MAXQUADS * 8];
int nq = 0;

// screen mapping of the original's 320x240 screen (y down) into the window
float winW = 1024.0;
float winH = 768.0;
float scale = 3.2;
float cx = 512.0;
float cy = 384.0;

float fadeRate = 5.0;              // Flurry's: alpha = 5 * dt per frame

// The original's arm walk (at DENSITY x its bead count), in logical coords.
void ComputeArms()
{
	float xs[MAXARMS];
	float ys[MAXARMS];
	for (int j = 0; j < MAXARMS; j++)
	{
		xs[j] = 0.0;
		ys[j] = 0.0;
		armBeads[j] = 0;
	}
	float step = 0.2 / DENSITY;
	float move = 1.8 / DENSITY;
	for (float i = 0; i < 30.; i += step)
	{
		for (int j = 0; j < arms; j++)
		{
			if (i > ls[j])
				continue;
			float f = i / ls[j];
			float fw = cos(120. / (2. - f));
			fw = fw < 0. ? -fw : fw;
			fw = pow(fw, .2);
			float af = t * .1 + j + sin(f * 12 * (11.7 + j * 1.71) * .05 - t * (26.3 + j * 1.87) * .02 + j * 3.1 + 117) * f * 5;
			xs[j] += sin(af) * move;
			ys[j] += cos(af) * move;
			int n = armBeads[j];
			if (n < MAXBEADS)
			{
				int k = j * MAXBEADS + n;
				float thick = (1. - f) + fw * fw * .5;
				bx[k] = 160 + xs[j];
				by[k] = 120 + ys[j];
				bw[k] = 1.6 * thick + 1.3;
				bfw[k] = fw;
				// the original's colours (bytes packed as blue, green, red)
				bcb[k] = fw * (80. * sin(14 + t * .7 + f * 1.7 + j) + 100.) / 255.0;
				bcg[k] = fw * (80. * sin(41 + t * .6 + f * 1.6 + j) + 100.) / 255.0;
				bcr[k] = fw * (80. * sin(23 + t * .8 + f * 1.5 + j) + 100.) / 255.0;
				armBeads[j] = n + 1;
			}
		}
	}
}

// Flurry's puff quad: from the old screen position (ox, oy) to the new one
// (x, y), 'w' wide at both ends, textured with atlas tile 'anim'.
void AddQuad(float x, float y, float ox, float oy, float w, int anim, float r, float g, float b, float a)
{
	if (nq >= MAXQUADS)
		return;
	float dx = x - ox;
	float dy = y - oy;
	float d = sqrt(dx * dx + dy * dy);
	if (d < 0.5)                      // hardly moved: a round puff pointing anywhere
	{
		dx = 0.5;
		dy = 0.0;
		d = 0.5;
	}
	float sm = w / d;
	float m = 1.0 + sm;
	float dxs = dx * sm;
	float dys = dy * sm;
	float dxm = dx * m;
	float dym = dy * m;
	int vi = nq * 8;
	int ci = nq * 16;
	for (int q = 0; q < 4; q++)
	{
		qCol[ci + q * 4] = r;
		qCol[ci + q * 4 + 1] = g;
		qCol[ci + q * 4 + 2] = b;
		qCol[ci + q * 4 + 3] = a;
	}
	float u0 = (anim & 7) * 0.125;
	float v0 = (anim >> 3) * 0.125;
	qTex[vi] = u0;
	qTex[vi + 1] = v0;
	qTex[vi + 2] = u0;
	qTex[vi + 3] = v0 + 0.125;
	qTex[vi + 4] = u0 + 0.125;
	qTex[vi + 5] = v0 + 0.125;
	qTex[vi + 6] = u0 + 0.125;
	qTex[vi + 7] = v0;
	qVert[vi] = x + dxm - dys;
	qVert[vi + 1] = y + dym + dxs;
	qVert[vi + 2] = x + dxm + dys;
	qVert[vi + 3] = y + dym - dxs;
	qVert[vi + 4] = ox - dxm + dys;
	qVert[vi + 5] = oy - dym - dxs;
	qVert[vi + 6] = ox - dxm - dys;
	qVert[vi + 7] = oy - dym + dxs;
	nq++;
}

float SX(float x)
{
	return cx + (x - 160.0) * scale;
}

float SY(float y)
{
	return cy - (y - 120.0) * scale;  // window y grows upwards
}

// Every bead as a soft coloured puff, plus a narrow bright core where the
// segment bands are strong (the original's white highlight streak).
void AddArmQuads(float brite)
{
	for (int j = 0; j < arms; j++)
		for (int n = 0; n < armBeads[j]; n++)
		{
			int k = j * MAXBEADS + n;
			float x = SX(bx[k]);
			float y = SY(by[k]);
			float ox = x;
			float oy = y;
			if (haveOld)
			{
				ox = SX(oldBx[k]);
				oy = SY(oldBy[k]);
			}
			int anim = bAnim[k] + 1;
			if (anim >= 64)
				anim = 0;
			bAnim[k] = anim;
			float w = bw[k] * scale;
			AddQuad(x, y, ox, oy, w, anim, bcr[k] * brite, bcg[k] * brite, bcb[k] * brite, 0.85);
			float core = bfw[k] * bfw[k] * brite * 0.55;
			AddQuad(x, y, ox, oy, w * 0.38, (anim + 32) & 63,
				(bcr[k] + 0.35) * core, (bcg[k] + 0.35) * core, (bcb[k] + 0.35) * core, 0.85);
		}
}

// Arm tips shed puffs that keep part of the tip's motion, slow down,
// widen and fade over PUFFLIFE seconds.
void UpdateTipSmoke(float dt)
{
	if (tipSmoke && haveOld && dt > 0.0)
		for (int j = 0; j < arms; j++)
		{
			int n = armBeads[j];
			if (n < 2)
				continue;
			int k = j * MAXBEADS + n - 1;
			float vx = (bx[k] - oldBx[k]) / dt;
			float vy = (by[k] - oldBy[k]) / dt;
			for (int e = 0; e < 2; e++)
			{
				int p = nextPuff;
				nextPuff = (nextPuff + 1) % MAXPUFFS;
				sX[p] = bx[k];
				sY[p] = by[k];
				sOX[p] = bx[k];
				sOY[p] = by[k];
				sVX[p] = vx * 0.6 + (rnd() - 0.5) * 30.0;
				sVY[p] = vy * 0.6 + (rnd() - 0.5) * 30.0;
				sAge[p] = rnd() * dt;
				sW[p] = bw[k];
				float lift = 0.25;          // tips glow a little whiter
				sR[p] = bcr[k] + lift;
				sG[p] = bcg[k] + lift;
				sB[p] = bcb[k] + lift;
				sAnim[p] = int(rnd() * 64.0) & 63;
				sAlive[p] = 1;
			}
		}
	float drag = pow(0.2, dt);           // keeps 20% of its speed after a second
	for (int p = 0; p < MAXPUFFS; p++)
	{
		if (!sAlive[p])
			continue;
		sAge[p] = sAge[p] + dt;
		if (sAge[p] >= PUFFLIFE)
		{
			sAlive[p] = 0;
			continue;
		}
		sOX[p] = sX[p];
		sOY[p] = sY[p];
		sVX[p] = sVX[p] * drag;
		sVY[p] = sVY[p] * drag;
		sX[p] = sX[p] + sVX[p] * dt;
		sY[p] = sY[p] + sVY[p] * dt;
	}
}

void AddSmokeQuads(float brite)
{
	for (int p = 0; p < MAXPUFFS; p++)
	{
		if (!sAlive[p])
			continue;
		float life = sAge[p] / PUFFLIFE;
		float w = sW[p] * (1.0 + 3.5 * life) * scale;   // widens as it ages
		float c = (1.0 - life) * brite * 0.5;
		int anim = sAnim[p] + 1;
		if (anim >= 64)
			anim = 0;
		sAnim[p] = anim;
		AddQuad(SX(sX[p]), SY(sY[p]), SX(sOX[p]), SY(sOY[p]), w, anim, sR[p] * c, sG[p] * c, sB[p] * c, 0.85);
	}
}

// ---- frame feedback (as in flurry.cpp) ----
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
	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, int(winW), int(winH), 0);
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
	glVertex2f(winW, 0.0);
	glTexCoord2f(1.0, 1.0);
	glVertex2f(winW, winH);
	glTexCoord2f(0.0, 1.0);
	glVertex2f(0.0, winH);
	glEnd();
	glDisable(GL_TEXTURE_2D);
}

void Reshape(int w, int h)
{
	winW = w;
	winH = h;
	scale = winW / 320.0;
	if (winH / 240.0 < scale)
		scale = winH / 240.0;
	cx = winW * 0.5;
	cy = winH * 0.5;
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, w, 0.0, h, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	AllocFrameTexture();
	haveOld = 0;
}

int main()
{
	for (int j = 0; j < MAXARMS; j++)
		ls[j] = 30 - frand(seed) * 10.;

	if (!glwInit(1024, 768, "Scinc Ophiura Flurry"))
	{
		printf("Cannot open OpenGL window\n");
		return 1;
	}
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	int smokeTex = MakeFlurryTexture();
	frameTex = glwGenTexture();
	// Scinc does not zero global arrays: initialise everything read before
	// it is first written
	for (int j = 0; j < MAXARMS; j++)
		armBeads[j] = 0;
	for (int k = 0; k < MAXARMS * MAXBEADS; k++)
		bAnim[k] = int(rnd() * 64.0) & 63;
	for (int p = 0; p < MAXPUFFS; p++)
		sAlive[p] = 0;

	float anim = 0.0;
	int paused = 0;
	int fullscreen = 0;
	int saveNext = 0;
	int running = 1;
	int frames = 0;
	double oldFrameTime = -1.0;
	double tRate0 = glwTime();

	while (running && glwPoll())
	{
		int key = glwGetKey();
		while (key != 0)
		{
			if (key == GLW_KEY_ESC || key == 'q' || key == 'Q')
				running = 0;
			else if (key == ' ')
				paused = 1 - paused;
			else if ((key == '+' || key == '=') && arms < MAXARMS)
			{
				arms++;
				haveOld = 0;
			}
			else if (key == '-' && arms > 3)
				arms--;
			else if (key == ']' && fadeRate > 0.8)
				fadeRate = fadeRate / 1.5;          // longer trails
			else if (key == '[' && fadeRate < 25.0)
				fadeRate = fadeRate * 1.5;          // shorter trails
			else if (key == 't' || key == 'T')
				tipSmoke = 1 - tipSmoke;
			else if (key == 'c' || key == 'C')
				AllocFrameTexture();
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

		// like Flurry, step at most 60 times a second; faster displays just
		// show the previous image again in between
		double now = glwTime();
		if (oldFrameTime < 0.0 || now - oldFrameTime >= 1.0 / 60.0)
		{
			float dt = 0.0;
			float alpha = 1.0;
			if (oldFrameTime >= 0.0)
			{
				dt = now - oldFrameTime;
				alpha = fadeRate * dt;
			}
			oldFrameTime = now;
			if (alpha > 1.0)
				alpha = 1.0;
			if (paused)
				dt = 0.0;

			// keep last frame's beads for the motion stretch
			for (int j = 0; j < arms; j++)
				for (int n = 0; n < armBeads[j]; n++)
				{
					int k = j * MAXBEADS + n;
					oldBx[k] = bx[k];
					oldBy[k] = by[k];
				}
			anim = anim + dt;
			t = anim * 2;
			ComputeArms();
			UpdateTipSmoke(dt);

			if (!paused)
			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glColor4f(0.0, 0.0, 0.0, alpha);
				glBegin(GL_QUADS);
				glVertex2f(0.0, 0.0);
				glVertex2f(winW, 0.0);
				glVertex2f(winW, winH);
				glVertex2f(0.0, winH);
				glEnd();
				// In 8 bits the fade alone never reaches black (v * 0.92 rounds
				// back to v for dim pixels), leaving a grey haze; subtract one
				// more step per frame so the trails die out completely.
				glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
				glBlendFunc(GL_ONE, GL_ONE);
				glColor4f(1.0 / 255.0, 1.0 / 255.0, 1.0 / 255.0, 0.0);
				glBegin(GL_QUADS);
				glVertex2f(0.0, 0.0);
				glVertex2f(winW, 0.0);
				glVertex2f(winW, winH);
				glVertex2f(0.0, winH);
				glEnd();
				glBlendEquation(GL_FUNC_ADD);

				// glow per frame scaled with the fade, so the trail's steady
				// brightness does not depend on the frame rate or trail length
				float brite = alpha * 0.9;
				nq = 0;
				AddSmokeQuads(brite);
				AddArmQuads(brite);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, smokeTex);
				glwVertexArray(2, qVert);
				glwColorArray(4, qCol);
				glwTexCoordArray(2, qTex);
				glDrawArrays(GL_QUADS, 0, nq * 4);
				glwDisableArrays();
				glDisable(GL_TEXTURE_2D);
				glDisable(GL_BLEND);

				glBindTexture(GL_TEXTURE_2D, frameTex);
				glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, int(winW), int(winH));
			}
			haveOld = 1;
			frames++;
		}

		if (saveNext)
		{
			if (glwSaveBMP("ophiura_flurry.bmp"))
				printf("wrote ophiura_flurry.bmp\n");
			fflush(stdout);
			saveNext = 0;
		}
		glwSwap();

		if (now - tRate0 >= 1.0)
		{
			char title[96];
			snprintf(title, 96, "Scinc Ophiura Flurry - %d arms, %d puffs - %.1f FPS", arms, nq, frames / (now - tRate0));
			glwSetTitle(title);
			tRate0 = now;
			frames = 0;
		}
	}

	glwDone();
	return 0;
}
