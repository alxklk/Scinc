// boing.cpp - Scinc port of xscreensaver's "boing" (hacks/glx/boing.c):
// a clone of the Amiga 1000 "Boing" demo - the red and white checkered ball
// bouncing in a purple grid room, with its shadow on the back wall and CRT
// scanlines.
//
// Physics, geometry, colours, projection and scanlines follow the original.
// Its physics advances one tick per frame at a 30 ms delay; here the same
// integrator runs in small sub-steps at 33.3 ticks per second, so the speed
// matches on any display. And one addition the original apologises for not
// having: the Amiga version made noise when the ball hit the walls, so this
// one does too - a synthesized "boing" thump, panned to the side it hits.
//
//   ./build_so.sh           (once, builds scgl_dl.so)
//   Scinc boing.cpp
//
// Keys: O smooth ball (the original's -smooth), L lighting (-lighting),
//       N scanlines on/off, P spin on/off, A sound on/off, - / + speed,
//       mouse drag rotates the room, H reset view, space pause,
//       F fullscreen, S save boing.bmp, Esc/Q quit.
//
// Original copyright and permission notice:
//
// boing, Copyright (c) 2005-2014 Jamie Zawinski <jwz@jwz.org>
//
// Permission to use, copy, modify, distribute, and sell this software and its
// documentation for any purpose is hereby granted without fee, provided that
// the above copyright notice appear in all copies and that both that
// copyright notice and this permission notice appear in supporting
// documentation.  No representations are made about the suitability of this
// software for any purpose.  It is provided "as is" without express or
// implied warranty.

#include <stdio.h>
#include "sound.h"
#include "scgl.h"

#define PI 3.14159265358979

// ---- the original's defaults ----
int spin = 1;
int lighting = 0;
int smooth = 0;
int scanlines = 1;
float speedArg = 1.0;
float angle = 15.0;              // ball tilt, degrees
float ballSize = 0.5;
int meridians = 16;
int parallels = 8;
int tiles = 12;
float thickness = 0.05;

// colours: #CC1919, #F2F2F2, #991999, #303030 (alpha 0.9), #8C8C8C,
// converted as the original does (component / 65536)
float ballColor1[4] = {0.79686, 0.09764, 0.09764, 1.0};
float ballColor2[4] = {0.94529, 0.94529, 0.94529, 1.0};
float gridColor[4] = {0.59764, 0.09764, 0.59764, 1.0};
float shadowColor[4] = {0.18749, 0.18749, 0.18749, 0.9};
float bgColor[4] = {0.54686, 0.54686, 0.54686, 1.0};
float lightPos[4] = {0.5, 0.5, -1.0, 0.0};
float lightAmb[4] = {0.0, 0.0, 0.0, 1.0};
float lightDif[4] = {1.0, 1.0, 1.0, 1.0};
float lightSpc[4] = {1.0, 1.0, 1.0, 1.0};

// ---- ball state ----
float speed = 0.0;
double ballX = 0.0;
double ballY = 0.0;
double ballZ = 0.0;
double ballTh = 0.0;
double ballDx = 0.0;
double ballDy = 0.0;
double ballDz = 0.0;
double ballDth = 0.0;
double ballDdx = 0.0;
double ballDdy = 0.0;
double ballDdz = 0.0;

int winW = 800;
int winH = 600;

int seed = 1;
float frand(float x)
{
	seed = seed * 1103515245 + 12345;
	return x * (((seed >> 8) & 0x7fffff) / 8388608.0);
}

// ---- sound: a few "boing" voices ----
#define RATE 48000.0
#define NV 4
CSound snd;
int soundOn = 1;
int vOn[NV];
float vPhase[NV];
float vFreq[NV];                 // current pitch
float vFreqEnd[NV];              // pitch it glides down to
float vGlide[NV];                // per-sample glide factor
float vEnv[NV];
float vDecay[NV];                // per-sample decay factor
float vClick[NV];                // attack click envelope
float vPanL[NV];
float vPanR[NV];
int noise = 12345;

// A bounce: a low pitch-dropping sine thump with a knock at the start,
// like the Amiga sample. Walls sound higher than the floor; x pans it.
void Boing(float strength, float pan, float pitch)
{
	if (!soundOn)
		return;
	int v = 0;
	for (int i = 0; i < NV; i++)
		if (!vOn[i])
			v = i;
	vOn[v] = 1;
	vPhase[v] = 0.0;
	vFreq[v] = 190.0 * pitch;
	vFreqEnd[v] = 62.0 * pitch;
	vGlide[v] = pow(0.5, 1.0 / (0.045 * RATE));   // pitch halves its distance every 45 ms
	vEnv[v] = 0.55 * strength;
	vDecay[v] = pow(0.5, 1.0 / (0.07 * RATE));    // loudness halves every 70 ms
	vClick[v] = 0.35 * strength;
	if (pan < -1.0) pan = -1.0;
	if (pan > 1.0) pan = 1.0;
	vPanL[v] = sqrt(0.5 * (1.0 - pan));
	vPanR[v] = sqrt(0.5 * (1.0 + pan));
}

void FillAudio()
{
	snd.Poll();
	int blocks = 0;
	while (snd.snd_bufhealth() < 2400 && blocks < 8)
	{
		for (int s = 0; s < 256; s++)
		{
			float l = 0.0;
			float r = 0.0;
			for (int v = 0; v < NV; v++)
			{
				if (!vOn[v])
					continue;
				float f = vFreqEnd[v] + (vFreq[v] - vFreqEnd[v]) * vGlide[v];
				vFreq[v] = f;
				vPhase[v] = vPhase[v] + 2.0 * PI * f / RATE;
				if (vPhase[v] > 2.0 * PI)
					vPhase[v] = vPhase[v] - 2.0 * PI;
				float x = sin(vPhase[v]);
				x = x + 0.25 * sin(2.0 * vPhase[v]);        // a little body
				noise = noise * 1103515245 + 12345;
				float n = ((noise >> 8) & 0xffff) / 32768.0 - 1.0;
				float y = x * vEnv[v] + n * vClick[v];
				vEnv[v] = vEnv[v] * vDecay[v];
				vClick[v] = vClick[v] * 0.994;              // ~4 ms knock
				l = l + y * vPanL[v];
				r = r + y * vPanR[v];
				if (vEnv[v] < 0.0005)
					vOn[v] = 0;
			}
			snd.snd_out(l, r);
		}
		blocks++;
	}
}

// ---- drawing (draw_grid, draw_box, draw_ball, draw_shadow, draw_scanlines) ----
void DrawGrid()
{
	float t2 = tiles / 2.0;
	float s = 1.0 / (tiles + thickness);
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, gridColor);
	glColor3f(gridColor[0], gridColor[1], gridColor[2]);
	glPushMatrix();
	glScalef(s, s, s);
	glTranslatef(-t2, -t2, 0.0);
	glLineWidth(winH * 0.06 * thickness);
	glBegin(GL_LINES);
	for (int y = 0; y <= tiles; y++)
	{
		glVertex3f(0.0, y, 0.0);
		glVertex3f(tiles, y, 0.0);
	}
	for (int x = 0; x <= tiles; x++)
	{
		glVertex3f(x, tiles, 0.0);
		glVertex3f(x, 0.0, 0.0);
	}
	glEnd();
	glPopMatrix();
}

void DrawBox()
{
	glPushMatrix();                    // back wall
	glTranslatef(0.0, 0.0, -0.5);
	DrawGrid();
	glPopMatrix();
	glPushMatrix();                    // floor
	glRotatef(90.0, 1.0, 0.0, 0.0);
	glTranslatef(0.0, 0.0, 0.5);
	DrawGrid();
	glPopMatrix();
}

void BallPoint(float thy, float thx)
{
	float x = cos(thy) * cos(thx) / 2.0;
	float y = sin(thy) / 2.0;
	float z = cos(thy) * sin(thx) / 2.0;
	if (smooth)
		glNormal3f(-x, -y, -z);
	glVertex3f(x, y, z);
}

void DrawBall()
{
	int scale = 1;
	if (smooth)
		scale = 5;
	if (parallels < 3)
		scale = scale * 2;
	int xx = meridians * scale;
	int yy = parallels * scale;
	if (lighting)
		glEnable(GL_LIGHTING);
	glFrontFace(GL_CW);
	glPushMatrix();
	glTranslatef(ballX, ballY, ballZ);
	glScalef(ballSize, ballSize, ballSize);
	glRotatef(-angle, 0.0, 0.0, 1.0);
	glRotatef(ballTh, 0.0, 1.0, 0.0);
	for (int y = 0; y < yy; y++)
	{
		float thy0 = y * (PI * 2.0) / (yy * 2) + PI / 2.0;
		float thy1 = (y + 1) * (PI * 2.0) / (yy * 2) + PI / 2.0;
		for (int x = 0; x < xx; x++)
		{
			float thx0 = x * (PI * 2.0) / xx;
			float thx1 = (x + 1) * (PI * 2.0) / xx;
			int bgp = ((x / scale) & 1) ^ ((y / scale) & 1);
			if (bgp)
			{
				glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, ballColor2);
				glColor3f(ballColor2[0], ballColor2[1], ballColor2[2]);
			}
			else
			{
				glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, ballColor1);
				glColor3f(ballColor1[0], ballColor1[1], ballColor1[2]);
			}
			glBegin(GL_QUADS);
			if (!smooth)
			{
				float my = (thy0 + thy1) / 2.0;
				float mx = (thx0 + thx1) / 2.0;
				glNormal3f(-cos(my) * cos(mx), -sin(my), -cos(my) * sin(mx));
			}
			BallPoint(thy0, thx0);
			BallPoint(thy1, thx0);
			BallPoint(thy1, thx1);
			BallPoint(thy0, thx1);
			glEnd();
		}
	}
	glPopMatrix();
	if (lighting)
		glDisable(GL_LIGHTING);
}

void DrawShadow()
{
	int scale = 1;
	if (smooth)
		scale = 5;
	if (parallels < 3)
		scale = scale * 2;
	int yy = parallels * scale;
	glPushMatrix();
	glTranslatef(ballX + 0.14, ballY + 0.07, -0.49);
	glScalef(ballSize, ballSize, ballSize);
	glRotatef(-angle, 0.0, 0.0, 1.0);
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, shadowColor);
	glColor4f(shadowColor[0], shadowColor[1], shadowColor[2], shadowColor[3]);
	glFrontFace(GL_CCW);
	glNormal3f(0.0, 0.0, 1.0);
	glBegin(GL_TRIANGLE_FAN);
	glVertex3f(0.0, 0.0, 0.0);
	for (int y = 0; y < yy * 2 + 1; y++)
	{
		float thy0 = y * (PI * 2.0) / (yy * 2) + PI / 2.0;
		glVertex3f(cos(thy0) / 2.0, sin(thy0) / 2.0, 0.0);
	}
	glEnd();
	glPopMatrix();
}

void DrawScanlines()
{
	int w = winW;
	int h = winH;
	if (h <= 300)
		return;
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.0, w, 0.0, h, -1.0, 1.0);
	int lh = 2;
	int ls = 1;
	if (h > 500)
	{
		lh = 4;
		ls = 4;
	}
	glLineWidth(lh);
	glColor4f(0.0, 0.0, 0.0, 0.3);
	glBegin(GL_LINES);
	for (int y = 0; y < h; y += lh + ls)
	{
		glVertex3f(0.0, y, 0.0);
		glVertex3f(w, y, 0.0);
	}
	glEnd();
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}

// ---- physics (tick_physics), in sub-steps: h = fraction of an original tick ----
void TickPhysics(float h)
{
	float s2 = ballSize / 2.0;
	float mx = 0.5 - s2;
	float mn = -mx;

	ballTh = ballTh + ballDth * h;
	while (ballTh > 360.0) ballTh = ballTh - 360.0;
	while (ballTh < 0.0) ballTh = ballTh + 360.0;

	ballDx = ballDx + ballDdx * h;
	ballX = ballX + ballDx * h;
	if (ballX < mn || ballX > mx)
	{
		float side = -1.0;
		if (ballX > mx) side = 1.0;
		float hit = ballDx / (speed * 7.0);
		if (hit < 0.0) hit = -hit;
		if (ballX < mn) ballX = mn;
		else ballX = mx;
		ballDx = -ballDx;
		ballDth = -ballDth;
		ballDx = ballDx + (frand(speed / 2.0) - speed);
		Boing(0.6 + 0.4 * hit, side, 1.25);
	}

	ballDy = ballDy + ballDdy * h;
	ballY = ballY + ballDy * h;
	if (ballY < mn)
	{
		ballY = mn;
		ballDy = -ballDy;
		Boing(1.0, ballX / mx * 0.8, 1.0);            // the floor: the big boing
	}
	else if (ballY > mx)
	{
		ballY = mx;
		ballDy = -ballDy;
	}

	ballDz = ballDz + ballDdz * h;
	ballZ = ballZ + ballDz * h;
	if (ballZ < mn)
	{
		ballZ = mn;
		ballDz = -ballDz;
		Boing(0.5, ballX / mx * 0.8, 1.4);            // back wall: a softer knock
	}
	else if (ballZ > mx)
	{
		ballZ = mx;
		ballDz = -ballDz;
	}
}

float viewRotX = 0.0;
float viewRotY = 0.0;

void Reshape(int width, int height)
{
	winW = width;
	winH = height;
	float h = (1.0 * height) / width;
	int y = 0;
	h = h * 4.0 / 3.0;      // "Back in the caveman days we couldn't even afford square pixels!"
	if (width > height * 5)
	{
		height = width * 3 / 4;
		y = -height / 2;
		h = (1.0 * height) / width;
	}
	glViewport(0, y, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (height > width)
	{
		float s = (1.0 * width) / height;
		glScalef(s, s, s);
	}
	gluPerspective(8.0, 1.0 / h, 1.0, 10.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0.0, 0.0, 8.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
}

void InitBall()
{
	speed = speedArg / 800.0;
	ballDth = 0.0;
	if (spin)
		ballDth = -speed * 7.0 * 360.0;
	ballX = 0.5 - ((ballSize / 2.0) + frand(1.0 - ballSize));
	ballY = 0.2;
	ballZ = 0.0;
	ballDx = speed * 6.0 + frand(speed);
	ballDy = 0.0;
	ballDdx = 0.0;
	ballDdy = -speed;
	ballDdz = 0.0;
	ballDz = speed * 6.0 + frand(speed);
}

int main()
{
	for (int v = 0; v < NV; v++)
		vOn[v] = 0;

	glwMultisample(4);
	if (!glwInit(1024, 768, "Scinc Boing"))
	{
		printf("Cannot open OpenGL window\n");
		return 1;
	}
	seed = int(glwTime() * 1000.0) + 777;

	glClearColor(bgColor[0], bgColor[1], bgColor[2], 1.0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDif);
	glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpc);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	InitBall();

	int paused = 0;
	int fullscreen = 0;
	int saveNext = 0;
	int dragging = 0;
	int lastX = 0;
	int lastY = 0;
	int running = 1;
	int frames = 0;
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
			else if (key == 'o' || key == 'O')
				smooth = 1 - smooth;
			else if (key == 'l' || key == 'L')
				lighting = 1 - lighting;
			else if (key == 'n' || key == 'N')
				scanlines = 1 - scanlines;
			else if (key == 'p' || key == 'P')
			{
				spin = 1 - spin;
				if (spin)
					ballDth = -speed * 7.0 * 360.0;
				else
					ballDth = 0.0;
			}
			else if (key == 'a' || key == 'A')
				soundOn = 1 - soundOn;
			else if ((key == '+' || key == '=') && speedArg < 4.0)
			{
				speedArg = speedArg * 1.25;
				InitBall();
			}
			else if (key == '-' && speedArg > 0.25)
			{
				speedArg = speedArg / 1.25;
				InitBall();
			}
			else if (key == 'h' || key == 'H')
			{
				viewRotX = 0.0;
				viewRotY = 0.0;
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

		// mouse drag rotates the room (the original's trackball); the ball
		// holds still while dragging, as in the original
		int mx = glwMouseX();
		int my = glwMouseY();
		if (glwMouseButtons() & 1)
		{
			if (dragging)
			{
				viewRotY = viewRotY + 0.4 * (mx - lastX);
				viewRotX = viewRotX + 0.4 * (my - lastY);
			}
			dragging = 1;
		}
		else
			dragging = 0;
		lastX = mx;
		lastY = my;

		if (!paused && !dragging)
		{
			// the original ticks once per 30 ms frame
			float ticks = dt / 0.030;
			while (ticks > 0.0)
			{
				float h = ticks;
				if (h > 0.25) h = 0.25;
				TickPhysics(h);
				ticks = ticks - h;
			}
		}
		FillAudio();

		if (glwResized())
			Reshape(glwWidth(), glwHeight());

		glShadeModel(GL_SMOOTH);
		glEnable(GL_NORMALIZE);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glPushMatrix();
		glRotatef(viewRotX, 1.0, 0.0, 0.0);
		glRotatef(viewRotY, 0.0, 1.0, 0.0);
		glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_LINE_SMOOTH);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		DrawBox();
		DrawShadow();
		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		DrawBall();
		if (scanlines)
			DrawScanlines();
		glPopMatrix();

		if (saveNext)
		{
			if (glwSaveBMP("boing.bmp"))
				printf("wrote boing.bmp\n");
			fflush(stdout);
			saveNext = 0;
		}
		glwSwap();

		frames++;
		if (now - tRate0 >= 1.0)
		{
			char* sm = "";
			char* li = "";
			char* so = "";
			if (smooth) sm = " - smooth";
			if (lighting) li = " - lighting";
			if (soundOn) so = " - sound";
			char title[96];
			snprintf(title, 96, "Scinc Boing - %.1f FPS%s%s%s", frames / (now - tRate0), sm, li, so);
			glwSetTitle(title);
			tRate0 = now;
			frames = 0;
		}
	}

	glwDone();
	return 0;
}
