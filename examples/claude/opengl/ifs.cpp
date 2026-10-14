// ifs.cpp - Scinc port of xscreensaver's "ifs" (hacks/ifs.c): an iterated
// function system of a few "lenses" - each a rotation, scale and
// translation - drawn by the chaos game, while every lens slowly mutates,
// so the fractal swirls and morphs.
//
// Lenses, their mutation, the chaos-game iteration and the multi-colour mode
// follow the original (its fixed-point maths done in floats). What changes
// is the point: the original plots single pixels; here every point is a
// soft round spot like in ophiura_2d.cpp, with the same Circle profile
// (radius R, half-width W, linear fade over A) computed per pixel in a
// small inline shader. The spots are point sprites - the GPU expands each
// vertex into a screen-aligned square and the shader shapes it with
// gl_PointCoord - so Scinc writes just x, y per spot for ~20000 spots a
// frame. With additive blending (the default "glow" mode) spots add up
// where the fractal is dense, like a flame fractal; G switches to the
// original's flat look.
//
//   ./build_so.sh           (once, builds scgl_dl.so)
//   Scinc ifs.cpp
//
// Keys: N / click new lenses, - / + functions (2..6), [ / ] detail,
//       , / . spot size, G glow / flat, M multi-colour, C new colours,
//       T trails, space pause, F fullscreen, S save ifs.bmp, Esc/Q quit.
//
// Original copyright and permission notice (MIT style):
//
// Copyright (c) Chris Le Sueur and Robby Griffin, 2005-2006
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// (Original hack by Massimino Pascal; this version by Chris Le Sueur 2005,
// improvements by Robby Griffin 2006, multi-coloured mode by Jack Grahl
// 2007.)

#include <stdio.h>
#include "scgl.h"

#define PI 3.14159265358979
#define MAXL 6                     // lenses
#define MAXPTS 60000               // spots per lens per frame
#define NCOL 200                   // the original's *colors: 200

// ---- options (the original's defaults) ----
int lensnum = 3;
int length = 9;                    // detail: lensnum^(length-1) points per lens
int multi = 1;
int rotate = 1;
int scaleOn = 1;
int translate = 1;

// ---- lenses ----
float lR[MAXL];                    // rotation
float lS[MAXL];                    // scale
float lTx[MAXL];                   // translation
float lTy[MAXL];
float lRo[MAXL];                   // old / target / counter for the rotation
float lRt[MAXL];
float lRc[MAXL];
float lSo[MAXL];                   // ... and for the scale
float lSt[MAXL];
float lSc[MAXL];
float lTxa[MAXL];                  // translation drift
float lTya[MAXL];
float mA[MAXL];                    // the lens as x' = a x + b y + ux, y' = c x + d y + uy
float mB[MAXL];
float mC[MAXL];
float mD[MAXL];
float mUx[MAXL];
float mUy[MAXL];

float W = 1024.0;                  // window size in pixels
float H = 768.0;
float chaosX = 0.0;                // the chaos game's running point
float chaosY = 0.0;

float pts[MAXPTS * 2];

float colR[NCOL];
float colG[NCOL];
float colB[NCOL];
int ccolour = 0;

int seed = 1;
int irand()
{
	seed = seed * 1103515245 + 12345;
	return (seed >> 8) & 0x7fffff;
}
float myrandom(float up)           // [0, up)
{
	return irand() / 8388608.0 * up;
}

// Precompute the lens matrix: the original's integer lensmatrix() with its
// 2^10 and 2^8 fixed-point factors taken out (coordinates in pixels).
void LensMatrix(int i)
{
	float s = lS[i];
	float c = cos(lR[i]);
	float n = sin(lR[i]);
	mA[i] = s * c;
	mB[i] = -s * n;
	mC[i] = s * n;
	mD[i] = s * c;
	mUx[i] = W * 0.5 * (s * (n - c) + lTx[i] / 16.0 + 1.0);
	mUy[i] = -H * 0.5 * (s * (n + c) + lTy[i] / 16.0 - 1.0);
}

void CreateLens(int i, float nr, float ns, float nx, float ny)
{
	lTxa[i] = 0.0;
	lTya[i] = 0.0;
	if (rotate)
	{
		lR[i] = nr;
		lRo[i] = nr;
		lRt[i] = nr;
		lRc[i] = 1.0;
	}
	else
		lR[i] = 0.0;
	if (scaleOn)
	{
		lS[i] = ns;
		lSo[i] = ns;
		lSt[i] = ns;
		lSc[i] = 1.0;
	}
	else
		lS[i] = 0.5;
	lTx[i] = nx;
	lTy[i] = ny;
	LensMatrix(i);
}

void NewLenses()
{
	for (int i = 0; i < MAXL; i++)
		CreateLens(i, myrandom(1.0) - 0.5, myrandom(1.0), myrandom(4.0) - 2.0, myrandom(4.0) + 2.0);
}

// Glide rotation and scale sinusoidally to new random targets, and let the
// translation wander (the original's mutate(), 50 times a second).
void Mutate(int i)
{
	if (rotate)
	{
		if (lRc[i] >= 1.0)
		{
			lRc[i] = 0.0;
			lRo[i] = lRt[i];
			lRt[i] = myrandom(4.0) - 2.0;
		}
		float factor = (sin(-PI / 2.0 + PI * lRc[i]) + 1.0) / 2.0;
		lR[i] = lRo[i] + (lRt[i] - lRo[i]) * factor;
		lRc[i] = lRc[i] + 0.01;
	}
	if (scaleOn)
	{
		if (lSc[i] >= 1.0)
		{
			lSc[i] = 0.0;
			lSo[i] = lSt[i];
			lSt[i] = myrandom(2.0) - 1.0;
		}
		float factor = (sin(-PI / 2.0 + PI * lSc[i]) + 1.0) / 2.0;
		lS[i] = lSo[i] + (lSt[i] - lSo[i]) * factor;
		lSc[i] = lSc[i] + 0.01;
	}
	if (translate)
	{
		lTxa[i] = lTxa[i] + myrandom(0.004) - 0.002;
		lTya[i] = lTya[i] + myrandom(0.004) - 0.002;
		lTx[i] = lTx[i] + lTxa[i];
		lTy[i] = lTy[i] + lTya[i];
		if (lTx[i] > 6.0) lTxa[i] = lTxa[i] - 0.004;
		if (lTy[i] > 6.0) lTya[i] = lTya[i] - 0.004;
		if (lTx[i] < -6.0) lTxa[i] = lTxa[i] + 0.004;
		if (lTy[i] < -6.0) lTya[i] = lTya[i] + 0.004;
		if (lTxa[i] > 0.05 || lTxa[i] < -0.05) lTxa[i] = lTxa[i] / 1.7;
		if (lTya[i] > 0.05 || lTya[i] < -0.05) lTya[i] = lTya[i] / 1.7;
	}
	LensMatrix(i);
}

// The chaos game (the original's iterate()): 'count' random lens steps from
// the running point, a spot at every step after the first 10. With p > 0
// each spot is moved once more by lens p (multi-colour mode). Returns the
// number of spots written to pts[].
int Iterate(int count, int p)
{
	float x = chaosX;
	float y = chaosY;
	int n = 0;
	int nl = lensnum;
	float pa = mA[p];
	float pb = mB[p];
	float pc = mC[p];
	float pd = mD[p];
	float pux = mUx[p];
	float puy = mUy[p];
	for (int i = 0; i < count; i++)
	{
		int l = irand() % nl;
		float tx = mA[l] * x + mB[l] * y + mUx[l];
		y = mC[l] * x + mD[l] * y + mUy[l];
		x = tx;
		if (i < 10)
			continue;
		if (p == 0)
		{
			pts[n] = x;
			pts[n + 1] = y;
		}
		else
		{
			pts[n] = pa * x + pb * y + pux;
			pts[n + 1] = pc * x + pd * y + puy;
		}
		n = n + 2;
	}
	chaosX = x;
	chaosY = y;
	return n / 2;
}

// ---- colours: a smooth cyclic colour map through a few random HSV points
// (like xscreensaver's make_smooth_colormap) ----
void HsvToRgb(float h, float s, float v, int k)
{
	while (h >= 360.0) h = h - 360.0;
	while (h < 0.0) h = h + 360.0;
	float c = v * s;
	float hh = h / 60.0;
	float m = hh - 2.0 * int(hh / 2.0) - 1.0;   // position within a 120 degree pair
	if (m < 0.0) m = -m;
	float x = c * (1.0 - m);
	float r = 0.0;
	float g = 0.0;
	float b = 0.0;
	if (hh < 1.0) { r = c; g = x; }
	else if (hh < 2.0) { r = x; g = c; }
	else if (hh < 3.0) { g = c; b = x; }
	else if (hh < 4.0) { g = x; b = c; }
	else if (hh < 5.0) { r = x; b = c; }
	else { r = c; b = x; }
	colR[k] = r + v - c;
	colG[k] = g + v - c;
	colB[k] = b + v - c;
}

void MakeColormap()
{
	float ph[4];
	float ps[4];
	float pv[4];
	int np = 2 + irand() % 3;
	for (int i = 0; i < np; i++)
	{
		ph[i] = myrandom(360.0);
		ps[i] = 0.55 + myrandom(0.45);
		pv[i] = 0.75 + myrandom(0.25);
	}
	for (int k = 0; k < NCOL; k++)
	{
		float f = (1.0 * k * np) / NCOL;
		int i0 = int(f);
		int i1 = (i0 + 1) % np;
		float u = f - i0;
		float dh = ph[i1] - ph[i0];       // shortest way round the hue circle
		if (dh > 180.0) dh = dh - 360.0;
		if (dh < -180.0) dh = dh + 360.0;
		HsvToRgb(ph[i0] + dh * u, ps[i0] + (ps[i1] - ps[i0]) * u, pv[i0] + (pv[i1] - pv[i0]) * u, k);
	}
}

// ---- the soft spot shader: point sprites with the Circle profile ----
char* spotVS =
	"#version 120\n"
	"varying vec4 vColor;\n"
	"void main()\n"
	"{\n"
	"	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
	"	vColor = gl_Color;\n"
	"}\n";

// Graph::Circle(x, y, R, W, A) profile: a ring of radius R and half-width W,
// opaque, fading linearly over a further distance A (all in pixels).
char* spotFS =
	"#version 120\n"
	"uniform vec3 uShape;\n"          // R, W, A
	"uniform float uExtent;\n"        // half the sprite size, pixels
	"varying vec4 vColor;\n"
	"void main()\n"
	"{\n"
	"	float d = length(gl_PointCoord * 2.0 - 1.0) * uExtent;\n"
	"	float outside = max(abs(d - uShape.x) - uShape.y, 0.0);\n"
	"	float a = clamp(1.0 - outside / max(uShape.z, 1e-4), 0.0, 1.0);\n"
	"	gl_FragColor = vec4(vColor.rgb, vColor.a * a);\n"
	"}\n";

int prog = 0;
int uShape;
int uExtent;

// ---- optional trails: previous frame kept in a texture, faded ----
int frameTex = 0;

void AllocFrameTexture()
{
	glBindTexture(GL_TEXTURE_2D, frameTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glClear(GL_COLOR_BUFFER_BIT);
	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, int(W), int(H), 0);
}

void FullQuad()
{
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 1.0);
	glVertex2f(0.0, 0.0);
	glTexCoord2f(1.0, 1.0);
	glVertex2f(W, 0.0);
	glTexCoord2f(1.0, 0.0);
	glVertex2f(W, H);
	glTexCoord2f(0.0, 0.0);
	glVertex2f(0.0, H);
	glEnd();
}

void Reshape(int w, int h)
{
	W = w;
	H = h;
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, w, h, 0.0, -1.0, 1.0);     // pixels, y down as in X11
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	chaosX = W * 0.5;
	chaosY = H * 0.5;
	for (int i = 0; i < MAXL; i++)
		LensMatrix(i);
	AllocFrameTexture();
}

int Power(int b, int e)
{
	int r = 1;
	for (int i = 0; i < e; i++)
	{
		r = r * b;
		if (r > MAXPTS)
			return MAXPTS;
	}
	return r;
}

int main()
{
	if (!glwInit(1024, 768, "Scinc IFS"))
	{
		printf("Cannot open OpenGL window\n");
		return 1;
	}
	seed = int(glwTime() * 1000.0) + 4242;
	prog = glwCreateProgram(spotVS, spotFS);
	if (prog == 0)
		return 1;
	uShape = glGetUniformLocation(prog, "uShape");
	uExtent = glGetUniformLocation(prog, "uExtent");
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_POINT_SPRITE);
	glClearColor(0.0, 0.0, 0.0, 1.0);
	frameTex = glwGenTexture();
	NewLenses();
	MakeColormap();

	int glow = 1;
	int trails = 0;
	float spot = 1.0;                  // spot size factor
	int paused = 0;
	int fullscreen = 0;
	int saveNext = 0;
	int running = 1;
	int wasDown = 0;
	float mutateClock = 0.0;
	int frames = 0;
	int spots = 0;
	double tPrev = glwTime();
	double tRate0 = tPrev;

	while (running && glwPoll())
	{
		double now = glwTime();
		float dt = now - tPrev;
		tPrev = now;
		if (dt > 0.1)
			dt = 0.1;

		int key = glwGetKey();
		while (key != 0)
		{
			if (key == GLW_KEY_ESC || key == 'q' || key == 'Q')
				running = 0;
			else if (key == 'n' || key == 'N')
				NewLenses();
			else if ((key == '+' || key == '=') && lensnum < MAXL)
				lensnum++;
			else if (key == '-' && lensnum > 2)
				lensnum--;
			else if (key == ']' && length < 12)
				length++;
			else if (key == '[' && length > 3)
				length--;
			else if (key == '.' && spot < 4.0)
				spot = spot * 1.25;
			else if (key == ',' && spot > 0.4)
				spot = spot / 1.25;
			else if (key == 'g' || key == 'G')
				glow = 1 - glow;
			else if (key == 'm' || key == 'M')
				multi = 1 - multi;
			else if (key == 'c' || key == 'C')
				MakeColormap();
			else if (key == 't' || key == 'T')
			{
				trails = 1 - trails;
				AllocFrameTexture();
			}
			else if (key == ' ')
				paused = 1 - paused;
			else if (key == 'f' || key == 'F')
			{
				fullscreen = 1 - fullscreen;
				glwSetFullscreen(fullscreen);
			}
			else if (key == 's' || key == 'S')
				saveNext = 1;
			key = glwGetKey();
		}
		int down = glwMouseButtons() & 1;     // a click makes new lenses, as in the original
		if (down && !wasDown)
			NewLenses();
		wasDown = down;

		if (glwResized())
			Reshape(glwWidth(), glwHeight());

		// the original mutates once per 20 ms frame
		if (!paused)
		{
			mutateClock = mutateClock + dt;
			while (mutateClock >= 0.02)
			{
				for (int i = 0; i < lensnum; i++)
					Mutate(i);
				ccolour = (ccolour + 1) % NCOL;
				mutateClock = mutateClock - 0.02;
			}
		}

		// background: black, or the faded previous frame
		if (trails)
		{
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, frameTex);
			glColor4f(1.0, 1.0, 1.0, 1.0);
			glDisable(GL_BLEND);
			FullQuad();
			glDisable(GL_TEXTURE_2D);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glColor4f(0.0, 0.0, 0.0, 0.18);
			FullQuad();
			glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);   // no 8-bit grey residue
			glBlendFunc(GL_ONE, GL_ONE);
			glColor4f(1.0 / 255.0, 1.0 / 255.0, 1.0 / 255.0, 0.0);
			FullQuad();
			glBlendEquation(GL_FUNC_ADD);
		}
		else
			glClear(GL_COLOR_BUFFER_BIT);

		// spot shape, scaled with the window: a small solid core, soft edge
		float k = spot * H / 768.0;
		float sW = 0.45 * k;
		float sA = 1.6 * k;
		float extent = sW + sA + 0.5;
		glUseProgram(prog);
		glUniform3f(uShape, 0.0, sW, sA);
		glUniform1f(uExtent, extent);
		glPointSize(2.0 * extent);
		glEnable(GL_BLEND);
		float alpha = 1.0;
		if (glow)
		{
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);          // density adds up
			alpha = 0.32;
		}
		else
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		spots = 0;
		if (multi)
		{
			int count = Power(lensnum, length - 1);
			for (int i = 0; i < lensnum; i++)
			{
				int c = (ccolour * (i + 1)) % NCOL;
				int n = Iterate(count, i);
				glColor4f(colR[c], colG[c], colB[c], alpha);
				glwVertexArray(2, pts);
				glDrawArrays(GL_POINTS, 0, n);
				glwDisableArrays();
				spots = spots + n;
			}
		}
		else
		{
			int n = Iterate(Power(lensnum, length), 0);
			glColor4f(colR[ccolour], colG[ccolour], colB[ccolour], alpha);
			glwVertexArray(2, pts);
			glDrawArrays(GL_POINTS, 0, n);
			glwDisableArrays();
			spots = n;
		}
		glUseProgram(0);
		glDisable(GL_BLEND);

		if (trails)
		{
			glBindTexture(GL_TEXTURE_2D, frameTex);
			glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, int(W), int(H));
		}
		if (saveNext)
		{
			if (glwSaveBMP("ifs.bmp"))
				printf("wrote ifs.bmp\n");
			fflush(stdout);
			saveNext = 0;
		}
		glwSwap();

		frames++;
		if (now - tRate0 >= 1.0)
		{
			char title[100];
			snprintf(title, 100, "Scinc IFS - %d functions, detail %d, %d spots - %.1f FPS", lensnum, length, spots, frames / (now - tRate0));
			glwSetTitle(title);
			tRate0 = now;
			frames = 0;
		}
	}

	glDeleteProgram(prog);
	glwDone();
	return 0;
}
