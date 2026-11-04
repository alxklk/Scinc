// lavalamp.cpp - a lava lamp, after xscreensaver's "lavalite" by jwz
// (hacks/glx/lavalite.c).
//
// The lamp shapes (classic, giant, cone, rocket), the blob physics and the
// colours and lights follow the original: every blob of lava is a group of
// 4 metaballs that climb on slow, steep parabolic arcs (convection up,
// gravity down) with a leader keeping the group together; a big static
// pool sits at the bottom and a small blob under the cap. The field is the
// original's too: a linear fall-off between each ball's hard radius r and
// its radius of influence R, faded out near the glass, surface at 0.3.
//
// What changes is how the surface is found. The original polygonises the
// field on the CPU with marching cubes ("Like real lavalites, this program
// is very slow"). Here the glass is drawn with a small inline fragment
// shader that casts a ray per pixel into the same field and finds the
// surface exactly, with smooth normals - the Scinc side only moves 18
// metaballs and hands them over as uniforms. The same pass also puts the
// specular reflection on the front glass that the original's TODO list
// asks for.
//
//   ./build_so.sh           (once, builds scgl_dl.so)
//   Scinc lavalamp.cpp
//
// Keys: 1 classic, 2 giant, 3 cone, 4 rocket, B launch a blob now,
//       - / + blob frequency, G glass reflection, space pause,
//       mouse drag orbit, H home, F fullscreen, S save lavalamp.bmp,
//       Esc/Q quit.
//
// Original copyright and permission notice:
//
// lavalite --- 3D Simulation a Lava Lite, written by jwz.
// This software Copyright (c) 2002-2017 Jamie Zawinski <jwz@jwz.org>
//
// Permission to use, copy, modify, distribute, and sell this software and its
// documentation for any purpose is hereby granted without fee, provided that
// the above copyright notice appear in all copies and that both that
// copyright notice and this permission notice appear in supporting
// documentation.  No representations are made about the suitability of this
// software for any purpose.  It is provided "as is" without express or
// implied warranty.
//
// LAVA(R), LAVA LITE(R), LAVA WORLD INTERNATIONAL(R) and the configuration of
// the LAVA(R) brand motion lamp are registered trademarks of Haggerty
// Enterprises, Inc.

#include <stdio.h>
#include "scgl.h"

#define PI 3.14159265358979

// ---- lamp models: slices from top to bottom (part, elevation, radius) ----
#define CAP 1
#define BOTTLE 2
#define BASE 3
#define MAXSLICES 12

int slPart[MAXSLICES];
float slElev[MAXSLICES];
float slRad[MAXSLICES];
int nslices = 0;
int style = 0;                       // 0 classic, 1 giant, 2 cone, 3 rocket
char* styleName = "classic";

void Slice(int part, float elev, float rad)
{
	slPart[nslices] = part;
	slElev[nslices] = elev;
	slRad[nslices] = rad;
	nslices++;
}

void SetModel(int s)
{
	style = s;
	nslices = 0;
	if (s == 0)
	{
		styleName = "classic";
		Slice(CAP, 1.16, 0.089);
		Slice(BOTTLE, 0.97, 0.120);
		Slice(BOTTLE, 0.13, 0.300);
		Slice(BOTTLE, 0.07, 0.300);
		Slice(BASE, 0.00, 0.280);
		Slice(BASE, -0.40, 0.120);
		Slice(BASE, -0.80, 0.280);
	}
	else if (s == 1)
	{
		styleName = "giant";
		Slice(CAP, 1.12, 0.105);
		Slice(BOTTLE, 0.97, 0.130);
		Slice(BOTTLE, 0.20, 0.300);
		Slice(BOTTLE, 0.15, 0.300);
		Slice(BASE, 0.00, 0.230);
		Slice(BASE, -0.18, 0.140);
		Slice(BASE, -0.80, 0.280);
	}
	else if (s == 2)
	{
		styleName = "cone";
		Slice(CAP, 1.35, 0.001);
		Slice(CAP, 1.35, 0.020);
		Slice(CAP, 1.30, 0.055);
		Slice(BOTTLE, 0.97, 0.120);
		Slice(BOTTLE, 0.13, 0.300);
		Slice(BASE, 0.00, 0.300);
		Slice(BASE, -0.04, 0.320);
		Slice(BASE, -0.60, 0.420);
	}
	else
	{
		styleName = "rocket";
		Slice(CAP, 1.35, 0.001);
		Slice(CAP, 1.34, 0.020);
		Slice(CAP, 1.30, 0.055);
		Slice(BOTTLE, 0.97, 0.120);
		Slice(BOTTLE, 0.13, 0.300);
		Slice(BOTTLE, 0.07, 0.300);
		Slice(BASE, 0.00, 0.280);
		Slice(BASE, -0.50, 0.180);
		Slice(BASE, -0.75, 0.080);
		Slice(BASE, -0.80, 0.035);
		Slice(BASE, -0.90, 0.035);
	}
}

// radius of the lamp at elevation z (the original's bottle_radius_at)
float RadiusAt(float z)
{
	if (nslices < 2 || z > slElev[0])
		return 0.0;
	for (int i = 1; i < nslices; i++)
		if (z > slElev[i])
		{
			float t = (z - slElev[i]) / (slElev[i - 1] - slElev[i]);
			return slRad[i] + (slRad[i - 1] - slRad[i]) * t;
		}
	return 0.0;
}

// ---- colours (as the original parses them: component / 65536) ----
float lavaColor[4] = {0.99609, 0.0, 0.0, 1.0};         // #FF0000
float fluidColor[4] = {0.0, 0.66405, 0.99609, 1.0};    // #00AAFF
float baseColor[4] = {0.39843, 0.39843, 0.39843, 1.0}; // #666666
float tableColor[4] = {0.0, 0.0, 0.0, 1.0};            // #000000
float footColor[4] = {0.2, 0.2, 0.2, 1.0};
float lampSpec[4] = {1.0, 1.0, 1.0, 1.0};
// The original's lights (-0.6,0,1), (1,0,0.2), (0.6,0,1) are given after its
// "right side up" -90 degree turn about X, so they really shine from above:
// (-0.6,1,0), (1,0.2,0), (0.6,1,0) - x to the viewer's right, y up the lamp.
float light0Pos[4] = {0.0, 0.0, 0.0, 0.0};   // set per frame from the view
float light1Pos[4] = {0.0, 0.0, 0.0, 0.0};
float light2Pos[4] = {0.0, 0.0, 0.0, 0.0};
float amb[4] = {0.0, 0.0, 0.0, 1.0};
float dif[4] = {1.0, 1.0, 1.0, 1.0};
float spc0[4] = {0.0, 1.0, 1.0, 1.0};
float spc1[4] = {1.0, 0.0, 1.0, 1.0};

// ---- metaballs (the original's struct metaball, as parallel arrays) ----
#define BLOBS_PER_GROUP 4
#define COUNT 3                      // the original's *count
#define NBALLS ((COUNT + 1) * BLOBS_PER_GROUP + 2)
#define GRAVITY 0.000013
#define CONVECTION 0.005
#define TILT 0.00166666

int bAlive[NBALLS];
int bStatic[NBALLS];
float bR0[NBALLS];                   // hard radius r
float bR1[NBALLS];                   // radius of influence R
float bZ[NBALLS];                    // elevation
float bPosR[NBALLS];                 // position on a horizontal circle
float bPosTh[NBALLS];
float bDr[NBALLS];                   // velocity
float bDz[NBALLS];
float bX[NBALLS];                    // horizontal position (computed)
float bY[NBALLS];
int bLeader[NBALLS];                 // -1, or the ball to stay close to
float launchChance = 0.003;          // the original's -speed
int justStarted = 1;

int seed = 1;
float frand(float x)
{
	seed = seed * 1103515245 + 12345;
	return x * (((seed >> 8) & 0x7fffff) / 8388608.0);
}

float bellrand(float extent)
{
	return ((frand(extent) + frand(extent) + frand(extent)) / 3.0) - extent / 2.0;
}

void MoveBall(int i, float h)
{
	if (bStatic[i])
		return;
	bPosR[i] = bPosR[i] + bDr[i] * h;
	bZ[i] = bZ[i] + bDz[i] * h;
	bDz[i] = bDz[i] - GRAVITY * h;
	if (bPosR[i] > 0.9)
	{
		bPosR[i] = 0.9;
		bDr[i] = -bDr[i];
	}
	else if (bPosR[i] < 0.0)
	{
		bPosR[i] = -bPosR[i];
		bDr[i] = -bDr[i];
	}
	float realR = bPosR[i] * RadiusAt(bZ[i]);
	bX[i] = cos(bPosTh[i]) * realR;
	bY[i] = sin(bPosTh[i]) * realR;
	if (bZ[i] < -bR1[i])             // dropped below the glass: turn it off
		bAlive[i] = 0;
}

void ResetBall(int i)
{
	bR0[i] = 0.00001;
	bR1[i] = 0.12 + bellrand(0.10);
	bPosR[i] = bellrand(0.9);
	bPosTh[i] = frand(PI * 2.0);
	bZ[i] = 0.0;
	bDr[i] = bellrand(TILT);
	bDz[i] = CONVECTION;
	bLeader[i] = -1;
	bAlive[i] = 1;
	MoveBall(i, 1.0);
}

int GetBall()
{
	for (int i = 0; i < NBALLS; i++)
		if (!bAlive[i])
			return i;
	return -1;
}

void GenerateStaticBlobs()
{
	for (int i = 0; i < NBALLS; i++)
	{
		bAlive[i] = 0;
		bStatic[i] = 0;
		bLeader[i] = -1;
	}
	int b0 = GetBall();              // the giant blob at the bottom
	bStatic[b0] = 1;
	bAlive[b0] = 1;
	bR1[b0] = 0.6;
	bR0[b0] = 0.3;
	bPosR[b0] = 0.0;
	bPosTh[b0] = 0.0;
	bDr[b0] = 0.0;
	bDz[b0] = 0.0;
	bX[b0] = 0.0;
	bY[b0] = 0.0;
	bZ[b0] = -0.43;
	int b1 = GetBall();              // the small blob at the top
	bStatic[b1] = 1;
	bAlive[b1] = 1;
	bR1[b1] = 0.16;
	bR0[b1] = 0.135;
	bX[b1] = 0.0;
	bY[b1] = 0.0;
	bZ[b1] = 1.078;
	for (int i = 0; i < BLOBS_PER_GROUP; i++)  // jumble the bottom surface
	{
		int b = GetBall();
		ResetBall(b);
		bStatic[b] = 1;
		bZ[b] = frand(0.04);
		bDr[b] = 0.0;
		bDz[b] = 0.0;
	}
}

// Send a new blob upward: a leader and followers with slightly
// different velocities.
void LaunchBalls()
{
	int b0 = GetBall();
	if (b0 < 0)
		return;
	ResetBall(b0);
	for (int i = 0; i < BLOBS_PER_GROUP; i++)
	{
		int b1 = GetBall();
		if (b1 < 0)
			break;
		ResetBall(b1);
		bPosR[b1] = bPosR[b0];
		bPosTh[b1] = bPosTh[b0];
		bDr[b1] = bDr[b0] + bellrand(0.8) * bDr[b0];
		bDz[b1] = bDz[b0] + bellrand(0.6) * bDz[b0];
		bLeader[b1] = b0;
	}
}

// Keep followers within 0.1 (vertically) of their leader.
void ClampBalls()
{
	for (int i = 0; i < NBALLS; i++)
	{
		int l = bLeader[i];
		if (bAlive[i] && l >= 0)
		{
			float minz = bZ[l] - 0.1;
			float maxz = bZ[l] + 0.1;
			if (bZ[i] < minz)
			{
				if (bDz[i] < 0.0) bDz[i] = -bDz[i];
				bZ[i] = minz - bDz[i];
			}
			if (bZ[i] > maxz)
			{
				if (bDz[i] > 0.0) bDz[i] = -bDz[i];
				bZ[i] = maxz + bDz[i];
			}
		}
	}
}

// One step of 'h' original ticks (it runs one tick per 30 ms frame).
void AnimateLava(float h)
{
	if (justStarted || frand(1.0) < launchChance * h)
	{
		justStarted = 0;
		LaunchBalls();
	}
	for (int i = 0; i < NBALLS; i++)
		if (bAlive[i])
			MoveBall(i, h);
	ClampBalls();
}

// ---- lamp geometry (generate_bottle, draw_disc, draw_tube, draw_wing) ----
int faces = 60;                      // resolution 40 * 1.5, capped at 60

void DrawDisc(float r, float z, int up)
{
	float step = PI * 2.0 / faces;
	if (up) glFrontFace(GL_CW);
	else glFrontFace(GL_CCW);
	if (up) glNormal3f(0.0, 1.0, 0.0);
	else glNormal3f(0.0, -1.0, 0.0);
	glBegin(GL_TRIANGLES);
	float x = r;
	float y = 0.0;
	float th = 0.0;
	for (int j = 0; j <= faces; j++)
	{
		glVertex3f(0.0, z, 0.0);
		glVertex3f(x, z, y);
		th = th + step;
		x = r * cos(th);
		y = r * sin(th);
		glVertex3f(x, z, y);
	}
	glEnd();
}

// smooth tube or cone between two slices; inside-out for the glass, so we
// look at the far wall through the (culled) near one
void DrawTube(float r0, float r1, float z0, float z1, int insideOut)
{
	float step = PI * 2.0 / faces;
	float nsign = 1.0;
	if (insideOut)
	{
		glFrontFace(GL_CW);
		nsign = -1.0;
	}
	else
		glFrontFace(GL_CCW);
	glBegin(GL_QUAD_STRIP);
	for (int i = 0; i <= faces; i++)
	{
		float th = i * step;
		float x = cos(th);
		float y = sin(th);
		glNormal3f(x * nsign, z1, y * nsign);    // (sic: the original's normal)
		glVertex3f(x * r1, z1, y * r1);
		glVertex3f(x * r0, z0, y * r0);
	}
	glEnd();
}

void DrawTable(float z)
{
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, tableColor);
	glFrontFace(GL_CW);
	glNormal3f(0.0, 1.0, 0.0);
	glBegin(GL_TRIANGLE_FAN);
	glVertex3f(0.0, z, 0.0);
	for (int j = 0; j <= 6; j++)
	{
		float th = j * PI * 2.0 / 6.0;
		glVertex3f(cos(th) * 8.0, z, sin(th) * 8.0);
	}
	glEnd();
}

// the rocket's fins
int wingX0[8] = {0, 10, 20, 30, 40, 45, 50, 53};
int wingY0[8] = {0, 10, 23, 41, 64, 81, 103, 134};
int wingX1[8] = {0, 10, 20, 30, 40, 45, 50, 51};
int wingY1[8] = {54, 57, 64, 75, 92, 104, 127, 134};

void DrawWing(float w, float h, float d)
{
	float maxx = 53.0;
	float maxy = 134.0;
	float zz = d / 2.0;
	for (int x = 1; x < 8; x++)
	{
		float px0 = wingX0[x - 1] / maxx * w;
		float py0 = wingY0[x - 1] / maxy * h;
		float px1 = wingX1[x - 1] / maxx * w;
		float py1 = wingY1[x - 1] / maxy * h;
		float px2 = wingX0[x] / maxx * w;
		float py2 = wingY0[x] / maxy * h;
		float px3 = wingX1[x] / maxx * w;
		float py3 = wingY1[x] / maxy * h;
		glFrontFace(GL_CW);
		glNormal3f(0.0, 0.0, -1.0);
		glBegin(GL_QUADS);
		glVertex3f(px0, -py0, -zz);
		glVertex3f(px1, -py1, -zz);
		glVertex3f(px3, -py3, -zz);
		glVertex3f(px2, -py2, -zz);
		glEnd();
		glFrontFace(GL_CCW);
		glNormal3f(0.0, 0.0, 1.0);
		glBegin(GL_QUADS);
		glVertex3f(px0, -py0, zz);
		glVertex3f(px1, -py1, zz);
		glVertex3f(px3, -py3, zz);
		glVertex3f(px2, -py2, zz);
		glEnd();
		glFrontFace(GL_CCW);
		glNormal3f(1.0, -1.0, 0.0);
		glBegin(GL_QUADS);
		glVertex3f(px0, -py0, -zz);
		glVertex3f(px0, -py0, zz);
		glVertex3f(px2, -py2, zz);
		glVertex3f(px2, -py2, -zz);
		glEnd();
		glFrontFace(GL_CW);
		glNormal3f(-1.0, 1.0, 0.0);
		glBegin(GL_QUADS);
		glVertex3f(px1, -py1, -zz);
		glVertex3f(px1, -py1, zz);
		glVertex3f(px3, -py3, zz);
		glVertex3f(px3, -py3, -zz);
		glEnd();
	}
}

int lampList = 0;                    // cap, base, table, fins, back wall of the glass
int glassList = 0;                   // the glass's outer surface, for the lava pass
float bottleTop = 0.97;
float bottleBottom = 0.07;
float maxBottleR = 0.3;

void GenerateBottle()
{
	if (lampList == 0)
	{
		lampList = glGenLists(1);
		glassList = glGenLists(1);
	}
	bottleTop = -9.0;
	bottleBottom = 9.0;
	maxBottleR = 0.0;
	glNewList(lampList, GL_COMPILE);
	glMaterialfv(GL_FRONT, GL_SPECULAR, lampSpec);
	glMaterialf(GL_FRONT, GL_SHININESS, 128.0);
	int lastPart = 0;
	for (int i = 0; i < nslices; i++)
	{
		int part = slPart[i];
		int next = 0;
		if (i + 1 < nslices)
			next = slPart[i + 1];
		if (part == BOTTLE)
			glEnable(GL_LIGHT2);         // light2 affects only the fluid
		else
			glDisable(GL_LIGHT2);
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, footColor);  // darker discs
		if ((part == CAP && lastPart == 0) || (part == BASE && lastPart == BOTTLE))
			DrawDisc(slRad[i], slElev[i], 1);
		if ((part == CAP && next == BOTTLE) || (part == BASE && next == 0))
		{
			int k = i + 1;
			if (next == 0) k = i;
			DrawDisc(slRad[k], slElev[k], 0);
		}
		if (next == 0)
			break;
		if (part == BOTTLE)
		{
			glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, fluidColor);
			if (slElev[i] > bottleTop) bottleTop = slElev[i];
			if (slElev[i + 1] < bottleBottom) bottleBottom = slElev[i + 1];
			if (slRad[i] > maxBottleR) maxBottleR = slRad[i];
			if (slRad[i + 1] > maxBottleR) maxBottleR = slRad[i + 1];
		}
		else
			glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, baseColor);
		DrawTube(slRad[i], slRad[i + 1], slElev[i], slElev[i + 1], part == BOTTLE);
		lastPart = part;
	}
	glDisable(GL_LIGHT2);
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, baseColor);
	float tableZ = slElev[nslices - 1];
	if (style == 3)
	{
		for (int i = 0; i < 3; i++)
		{
			glPushMatrix();
			glRotatef(120.0 * i, 0.0, 1.0, 0.0);
			glTranslatef(0.14, -0.05, 0.0);
			DrawWing(0.4, 0.95, 0.02);
			glPopMatrix();
		}
		tableZ = tableZ - 0.1;
	}
	DrawTable(tableZ);
	glEndList();

	// the glass's outer surface, drawn by the lava shader
	glNewList(glassList, GL_COMPILE);
	for (int i = 0; i + 1 < nslices; i++)
		if (slPart[i] == BOTTLE && slPart[i + 1] == BOTTLE)
			DrawTube(slRad[i], slRad[i + 1], slElev[i], slElev[i + 1], 0);
	glEndList();
}

// ---- the lava shader: ray-cast the metaball field inside the glass ----
char* lavaVS =
	"#version 120\n"
	"varying vec3 vPos;\n"             // lamp coordinates: x, elevation, z
	"void main()\n"
	"{\n"
	"	vPos = gl_Vertex.xyz;\n"
	"	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
	"}\n";

char* lavaFS =
	"#version 120\n"
	"uniform vec3 uCam;\n"             // camera, lamp coordinates
	"uniform vec4 uBall[20];\n"        // x, elevation, z, alive
	"uniform vec4 uBallR[20];\n"       // r, R
	"uniform int uNBalls;\n"
	"uniform float uElev[12];\n"       // lamp profile
	"uniform float uRad[12];\n"
	"uniform int uNSlices;\n"
	"uniform vec2 uBottle;\n"          // bottom and top of the glass
	"uniform vec3 uL0;\n"              // lights, lamp coordinates
	"uniform vec3 uL1;\n"
	"uniform vec3 uLava;\n"
	"uniform float uGlass;\n"
	"varying vec3 vPos;\n"
	"\n"
	"float radiusAt(float z)\n"
	"{\n"
	"	if (z > uElev[0]) return 0.0;\n"
	"	for (int i = 1; i < 12; i++)\n"
	"	{\n"
	"		if (i >= uNSlices) break;\n"
	"		if (z > uElev[i])\n"
	"			return mix(uRad[i], uRad[i - 1], (z - uElev[i]) / (uElev[i - 1] - uElev[i]));\n"
	"	}\n"
	"	return 0.0;\n"
	"}\n"
	"\n"
	// the original's obj_compute: clip * sum of linear fall-offs
	"float field(vec3 p)\n"
	"{\n"
	"	if (p.y < uBottle.x || p.y > uBottle.y) return 0.0;\n"
	"	float or2 = radiusAt(p.y);\n"
	"	or2 *= or2;\n"
	"	float d2 = p.x * p.x + p.z * p.z;\n"
	"	if (d2 > or2) return 0.0;\n"
	"	float ir2 = or2 * 0.7;\n"
	"	float clip = d2 > ir2 ? 1.0 - (d2 - ir2) / (or2 - ir2) : 1.0;\n"
	"	float vv = 0.0;\n"
	"	for (int i = 0; i < 20; i++)\n"
	"	{\n"
	"		if (i >= uNBalls) break;\n"
	"		if (uBall[i].w < 0.5) continue;\n"
	"		vec3 d = p - uBall[i].xyz;\n"
	"		float R2 = uBallR[i].y * uBallR[i].y;\n"
	"		float dd = dot(d, d);\n"
	"		if (dd > R2) continue;\n"
	"		float r2 = uBallR[i].x * uBallR[i].x;\n"
	"		vv += dd <= r2 ? 1.0 : 1.0 - (dd - r2) / (R2 - r2);\n"
	"	}\n"
	"	return clip * vv;\n"
	"}\n"
	"\n"
	"void main()\n"
	"{\n"
	"	vec3 rd = normalize(vPos - uCam);\n"
	"	vec3 V = -rd;\n"
	// glass: specular highlight and a faint Fresnel rim on the front surface
	"	vec3 ng = normalize(vec3(vPos.x, 0.0, vPos.z));\n"
	"	float glass = pow(max(dot(ng, normalize(uL0 + V)), 0.0), 90.0) * 0.8\n"
	"	            + pow(max(dot(ng, normalize(uL1 + V)), 0.0), 90.0) * 0.5\n"
	"	            + pow(1.0 - max(dot(ng, V), 0.0), 4.0) * 0.18;\n"
	"	glass *= uGlass;\n"
	// march only where the ray passes through some ball's sphere of influence
	"	float t0 = length(vPos - uCam);\n"
	"	float tIn = 1e9;\n"
	"	float tOut = -1e9;\n"
	"	for (int i = 0; i < 20; i++)\n"
	"	{\n"
	"		if (i >= uNBalls) break;\n"
	"		if (uBall[i].w < 0.5) continue;\n"
	"		vec3 oc = uCam - uBall[i].xyz;\n"
	"		float b = dot(oc, rd);\n"
	"		float c = dot(oc, oc) - uBallR[i].y * uBallR[i].y;\n"
	"		float disc = b * b - c;\n"
	"		if (disc <= 0.0) continue;\n"
	"		float s = sqrt(disc);\n"
	"		tIn = min(tIn, -b - s);\n"
	"		tOut = max(tOut, -b + s);\n"
	"	}\n"
	"	tIn = max(tIn, t0);\n"
	"	bool hit = false;\n"
	"	float t = tIn;\n"
	"	if (tOut > tIn)\n"
	"	{\n"
	"		float step = 0.004;\n"
	"		float lastT = t;\n"
	"		for (int k = 0; k < 250; k++)\n"
	"		{\n"
	"			if (field(uCam + rd * t) >= 0.3) { hit = true; break; }\n"
	"			lastT = t;\n"
	"			t += step;\n"
	"			if (t > tOut) break;\n"
	"		}\n"
	"		if (hit && t > tIn)\n"              // refine the crossing
	"		{\n"
	"			float a = lastT;\n"
	"			float b = t;\n"
	"			for (int k = 0; k < 6; k++)\n"
	"			{\n"
	"				float m = 0.5 * (a + b);\n"
	"				if (field(uCam + rd * m) >= 0.3) b = m; else a = m;\n"
	"			}\n"
	"			t = b;\n"
	"		}\n"
	"	}\n"
	"	if (!hit)\n"
	"	{\n"
	"		gl_FragColor = vec4(vec3(1.0), clamp(glass, 0.0, 1.0));\n"
	"		return;\n"
	"	}\n"
	"	vec3 p = uCam + rd * t;\n"
	"	float e = 0.003;\n"
	"	vec3 g = vec3(field(p + vec3(e, 0.0, 0.0)) - field(p - vec3(e, 0.0, 0.0)),\n"
	"	              field(p + vec3(0.0, e, 0.0)) - field(p - vec3(0.0, e, 0.0)),\n"
	"	              field(p + vec3(0.0, 0.0, e)) - field(p - vec3(0.0, 0.0, e)));\n"
	"	vec3 n = length(g) > 1e-6 ? -normalize(g) : V;\n"
	// the original's lava material under its two lights: red, white
	// diffuse, cyan and magenta specular, shininess 128, GL's 0.2 ambient
	"	float d0 = max(dot(n, uL0), 0.0);\n"
	"	float d1 = max(dot(n, uL1), 0.0);\n"
	"	float s0 = d0 > 0.0 ? pow(max(dot(n, normalize(uL0 + V)), 0.0), 128.0) : 0.0;\n"
	"	float s1 = d1 > 0.0 ? pow(max(dot(n, normalize(uL1 + V)), 0.0), 128.0) : 0.0;\n"
	"	vec3 col = uLava * (0.2 + d0 + d1) + vec3(0.0, 1.0, 1.0) * s0 + vec3(1.0, 0.0, 1.0) * s1;\n"
	"	gl_FragColor = vec4(min(col + vec3(glass), 1.0), 1.0);\n"
	"}\n";

int prog = 0;
int uCam;
int uBall;
int uBallR;
int uNBalls;
int uElev;
int uRad;
int uNSlices;
int uBottle;
int uL0;
int uL1;
int uLava;
int uGlass;
float ballData[NBALLS * 4];
float ballRData[NBALLS * 4];
float elevData[MAXSLICES];
float radData[MAXSLICES];

// ---- camera and scene placement ----
float yaw = 0.0;
float pitch = 0.45;                 // the original leans the lamp toward the viewer
float dist = 4.0;
float lampOffset = 0.33;             // the original moves each style up a bit
float lampScale = 1.0;

void SetStyle(int s)
{
	SetModel(s);
	lampOffset = 0.33;
	lampScale = 1.0;
	if (s == 2) lampOffset = 0.16;
	if (s == 3)
	{
		lampOffset = 0.30;
		lampScale = 0.85;
	}
	GenerateBottle();
	GenerateStaticBlobs();
	justStarted = 1;
}

void Reshape(int w, int h)
{
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(30.0, (1.0 * w) / h, 1.0, 100.0);
	glMatrixMode(GL_MODELVIEW);
}

int main()
{
	glwMultisample(4);
	if (!glwInit(600, 900, "Scinc Lava Lamp"))     // the original's 600x900
	{
		printf("Cannot open OpenGL window\n");
		return 1;
	}
	seed = int(glwTime() * 1000.0) + 99;
	prog = glwCreateProgram(lavaVS, lavaFS);
	if (prog == 0)
		return 1;
	uCam = glGetUniformLocation(prog, "uCam");
	uBall = glGetUniformLocation(prog, "uBall");
	uBallR = glGetUniformLocation(prog, "uBallR");
	uNBalls = glGetUniformLocation(prog, "uNBalls");
	uElev = glGetUniformLocation(prog, "uElev");
	uRad = glGetUniformLocation(prog, "uRad");
	uNSlices = glGetUniformLocation(prog, "uNSlices");
	uBottle = glGetUniformLocation(prog, "uBottle");
	uL0 = glGetUniformLocation(prog, "uL0");
	uL1 = glGetUniformLocation(prog, "uL1");
	uLava = glGetUniformLocation(prog, "uLava");
	uGlass = glGetUniformLocation(prog, "uGlass");

	glClearColor(0.0, 0.0, 0.0, 1.0);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_NORMALIZE);
	glEnable(GL_MULTISAMPLE);
	glShadeModel(GL_SMOOTH);
	glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
	glLightfv(GL_LIGHT0, GL_SPECULAR, spc0);
	glLightfv(GL_LIGHT1, GL_AMBIENT, amb);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, dif);
	glLightfv(GL_LIGHT1, GL_SPECULAR, spc1);
	glLightfv(GL_LIGHT2, GL_AMBIENT, amb);
	glLightfv(GL_LIGHT2, GL_DIFFUSE, dif);
	glLightfv(GL_LIGHT2, GL_SPECULAR, spc0);

	int s = 0;                          // "random": classic half the time
	if (frand(1.0) >= 0.5)
		s = int(frand(4.0));
	SetStyle(s);

	int paused = 0;
	int glassOn = 1;
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
			else if (key >= '1' && key <= '4')
				SetStyle(key - '1');
			else if (key == 'b' || key == 'B')
				LaunchBalls();
			else if ((key == '+' || key == '=') && launchChance < 0.05)
				launchChance = launchChance * 1.5;
			else if (key == '-' && launchChance > 0.0005)
				launchChance = launchChance / 1.5;
			else if (key == 'g' || key == 'G')
				glassOn = 1 - glassOn;
			else if (key == ' ')
				paused = 1 - paused;
			else if (key == 'h' || key == 'H')
			{
				yaw = 0.0;
				pitch = 0.45;
				dist = 4.0;
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

		int mx = glwMouseX();
		int my = glwMouseY();
		if (glwMouseButtons() & 1)
		{
			if (dragging)
			{
				yaw = yaw - 0.01 * (mx - lastX);
				pitch = pitch + 0.01 * (my - lastY);
				if (pitch < -0.6) pitch = -0.6;
				if (pitch > 1.2) pitch = 1.2;
			}
			dragging = 1;
		}
		else
			dragging = 0;
		lastX = mx;
		lastY = my;
		int wheel = glwMouseWheel();
		if (wheel != 0)
		{
			dist = dist * pow(0.9, wheel);
			if (dist < 1.5) dist = 1.5;
			if (dist > 10.0) dist = 10.0;
		}

		if (!paused)
		{
			// the lava ticks as in the original (30 ms frames), in sub-steps
			float ticks = dt / 0.030;
			while (ticks > 0.0)
			{
				float h = ticks;
				if (h > 0.5) h = 0.5;
				AnimateLava(h);
				ticks = ticks - h;
			}
			if (!dragging)
				yaw = yaw + 0.12 * dt;        // the lamp turns slowly (spin)
		}

		if (glwResized())
			Reshape(glwWidth(), glwHeight());

		// camera orbiting the lamp's centre
		float cp = cos(pitch);
		float ex = dist * cp * sin(yaw);
		float ey = dist * sin(pitch);
		float ez = dist * cp * cos(yaw);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glLoadIdentity();
		gluLookAt(ex, ey, ez, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
		// lights: above and to the sides, turning with the viewer around
		// the lamp (the original spins the lamp under fixed lights)
		float rx = cos(yaw);                  // the viewer's right, horizontal
		float rz = -sin(yaw);
		light0Pos[0] = -0.6 * rx;
		light0Pos[1] = 1.0;
		light0Pos[2] = -0.6 * rz;
		light1Pos[0] = rx;
		light1Pos[1] = 0.2;
		light1Pos[2] = rz;
		light2Pos[0] = 0.6 * rx;
		light2Pos[1] = 1.0;
		light2Pos[2] = 0.6 * rz;
		glLightfv(GL_LIGHT0, GL_POSITION, light0Pos);
		glLightfv(GL_LIGHT1, GL_POSITION, light1Pos);
		glLightfv(GL_LIGHT2, GL_POSITION, light2Pos);
		// lamp coordinates: x, elevation, z
		glTranslatef(0.0, lampOffset, 0.0);
		glScalef(lampScale, lampScale, lampScale);
		glTranslatef(0.0, -0.5, 0.0);

		glEnable(GL_LIGHTING);
		glCallList(lampList);
		glDisable(GL_LIGHTING);

		// the same lights for the lava shader (lamp coordinates = world
		// coordinates here, up to the lamp's uniform scale)
		float l0x = light0Pos[0];
		float l0y = light0Pos[1];
		float l0z = light0Pos[2];
		float l1x = light1Pos[0];
		float l1y = light1Pos[1];
		float l1z = light1Pos[2];
		float n0 = sqrt(l0x * l0x + l0y * l0y + l0z * l0z);
		float n1 = sqrt(l1x * l1x + l1y * l1y + l1z * l1z);

		int nb = 0;
		for (int i = 0; i < NBALLS; i++)
		{
			// the original's metaball x, y are horizontal and z is up
			ballData[i * 4] = bX[i];
			ballData[i * 4 + 1] = bZ[i];
			ballData[i * 4 + 2] = bY[i];
			ballData[i * 4 + 3] = bAlive[i];
			ballRData[i * 4] = bR0[i];
			ballRData[i * 4 + 1] = bR1[i];
			ballRData[i * 4 + 2] = 0.0;
			ballRData[i * 4 + 3] = 0.0;
			nb++;
		}
		for (int i = 0; i < nslices; i++)
		{
			elevData[i] = slElev[i];
			radData[i] = slRad[i];
		}

		glUseProgram(prog);
		glUniform3f(uCam, ex / lampScale, (ey - lampOffset) / lampScale + 0.5, ez / lampScale);
		glUniform4fv(uBall, nb, ballData);
		glUniform4fv(uBallR, nb, ballRData);
		glUniform1i(uNBalls, nb);
		glUniform1fv(uElev, nslices, elevData);
		glUniform1fv(uRad, nslices, radData);
		glUniform1i(uNSlices, nslices);
		glUniform2f(uBottle, bottleBottom, bottleTop);
		glUniform3f(uL0, l0x / n0, l0y / n0, l0z / n0);
		glUniform3f(uL1, l1x / n1, l1y / n1, l1z / n1);
		glUniform3f(uLava, lavaColor[0], lavaColor[1], lavaColor[2]);
		glUniform1f(uGlass, glassOn);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(0);
		glCallList(glassList);
		glDepthMask(1);
		glDisable(GL_BLEND);
		glUseProgram(0);

		if (saveNext)
		{
			if (glwSaveBMP("lavalamp.bmp"))
				printf("wrote lavalamp.bmp\n");
			fflush(stdout);
			saveNext = 0;
		}
		glwSwap();

		frames++;
		if (now - tRate0 >= 1.0)
		{
			char title[96];
			snprintf(title, 96, "Scinc Lava Lamp - %s - %.1f FPS", styleName, frames / (now - tRate0));
			glwSetTitle(title);
			tRate0 = now;
			frames = 0;
		}
	}

	glDeleteProgram(prog);
	glwDone();
	return 0;
}
