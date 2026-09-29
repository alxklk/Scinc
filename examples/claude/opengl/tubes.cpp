// tubes.cpp - "3D pipes": glossy tubes growing through a 3D grid.
//
// Several pipes grow at once, one grid cell per step. Each keeps going
// straight most of the time and now and then turns into a random free
// neighbouring cell; turns get a ball joint. Pipes never cross: a pipe that
// is boxed in ends with a cap and a new one starts in a free cell, in a new
// colour. When the grid is crowded the scene pauses and starts over.
// The newest segment of every pipe grows smoothly between steps.
//
// Drawing: a unit cylinder and a unit sphere are display lists; every tube
// segment and joint is one glCallList with a translate / rotate / scale.
// Lighting is per pixel in an inline GLSL shader: warm key, cool fill and
// rim lights, glossy Fresnel reflections of a procedural studio, ACES tone
// mapping; plus a gradient backdrop and 4x MSAA.
//
//   ./build_so.sh           (once, builds scgl_dl.so)
//   Scinc tubes.cpp
//
// Controls:
//   mouse drag  orbit                  wheel   zoom
//   [ / ]       pipes growing at once (1..8)
//   - / +       growth speed           N       start over
//   space       pause                  H       home view
//   F           fullscreen             S       save tubes.bmp
//   Esc / Q     quit

#include <stdio.h>
#include "scgl.h"

#define M_PI 3.14159265358979

#define G 14                   // grid cells per side
#define CELLS (G * G * G)
#define MAXSEG 2400            // tube segments per scene
#define MAXJ 1600              // joints and caps per scene
#define MAXPIPES 8
#define RADIUS 0.17            // tube radius (grid spacing 1)
#define JOINT 0.25             // ball joint radius

// ---- random numbers ----
int seed = 20260928;
int irand()
{
	seed = seed * 1103515245 + 12345;
	int r = (seed >> 8) & 0x7fffff;
	return r;
}
float frand()
{
	return irand() / 8388608.0;
}

// ---- scene ----
int grid[CELLS];               // 1 = occupied

int segX[MAXSEG];              // start cell
int segY[MAXSEG];
int segZ[MAXSEG];
int segDir[MAXSEG];            // 0 +x, 1 -x, 2 +y, 3 -y, 4 +z, 5 -z
int segStep[MAXSEG];           // growth step it was created in
float segR[MAXSEG];
float segG[MAXSEG];
float segB[MAXSEG];
int nseg = 0;

int jX[MAXJ];
int jY[MAXJ];
int jZ[MAXJ];
float jS[MAXJ];                // radius
float jR[MAXJ];
float jG[MAXJ];
float jB[MAXJ];
int nj = 0;

int pX[MAXPIPES];              // head cell of each pipe
int pY[MAXPIPES];
int pZ[MAXPIPES];
int pDir[MAXPIPES];
int pAlive[MAXPIPES];
float pR[MAXPIPES];
float pG[MAXPIPES];
float pB[MAXPIPES];

int pipes = 5;
int step = 0;                  // growth steps so far
float stepsPerSec = 9.0;
float progress = 0.0;          // 0..1 through the current step
int full = 0;                  // scene finished, waiting to start over
float fullTimer = 0.0;

// palette, linear-light colours
float palR[9] = {0.70, 0.80, 0.75, 0.10, 0.02, 0.05, 0.30, 0.75, 0.55};
float palG[9] = {0.04, 0.25, 0.60, 0.50, 0.40, 0.12, 0.05, 0.08, 0.55};
float palB[9] = {0.02, 0.02, 0.03, 0.08, 0.40, 0.70, 0.65, 0.30, 0.58};
int lastColor = -1;

int dirX[6] = {1, -1, 0, 0, 0, 0};
int dirY[6] = {0, 0, 1, -1, 0, 0};
int dirZ[6] = {0, 0, 0, 0, 1, -1};

int Cell(int x, int y, int z)
{
	return (z * G + y) * G + x;
}

int Free(int x, int y, int z)
{
	if (x < 0 || y < 0 || z < 0 || x >= G || y >= G || z >= G)
		return 0;
	return grid[Cell(x, y, z)] == 0;
}

void AddJoint(int x, int y, int z, float s, int k)
{
	if (nj >= MAXJ)
		return;
	jX[nj] = x;
	jY[nj] = y;
	jZ[nj] = z;
	jS[nj] = s;
	jR[nj] = pR[k];
	jG[nj] = pG[k];
	jB[nj] = pB[k];
	nj++;
}

// Start pipe k in a random free cell; returns 0 when none could be found.
int Spawn(int k)
{
	for (int tries = 0; tries < 200; tries++)
	{
		int x = irand() % G;
		int y = irand() % G;
		int z = irand() % G;
		if (Free(x, y, z))
		{
			int c = irand() % 9;
			if (c == lastColor)
				c = (c + 1) % 9;
			lastColor = c;
			pR[k] = palR[c];
			pG[k] = palG[c];
			pB[k] = palB[c];
			pX[k] = x;
			pY[k] = y;
			pZ[k] = z;
			pDir[k] = irand() % 6;
			pAlive[k] = 1;
			grid[Cell(x, y, z)] = 1;
			AddJoint(x, y, z, RADIUS * 1.05, k);    // round start cap
			return 1;
		}
	}
	pAlive[k] = 0;
	return 0;
}

void NewScene()
{
	for (int i = 0; i < CELLS; i++)
		grid[i] = 0;
	nseg = 0;
	nj = 0;
	step = 0;
	progress = 0.0;
	full = 0;
	for (int k = 0; k < MAXPIPES; k++)
		pAlive[k] = 0;
	for (int k = 0; k < pipes; k++)
		Spawn(k);
}

// Advance pipe k by one cell.
void Grow(int k)
{
	int x = pX[k];
	int y = pY[k];
	int z = pZ[k];
	int d = pDir[k];
	int nd = -1;
	if (frand() < 0.78 && Free(x + dirX[d], y + dirY[d], z + dirZ[d]))
		nd = d;                            // keep going straight
	else
	{
		int opts[6];
		int n = 0;
		for (int i = 0; i < 6; i++)
			if (i != (d ^ 1) && Free(x + dirX[i], y + dirY[i], z + dirZ[i]))
			{
				opts[n] = i;
				n++;
			}
		if (n > 0)
			nd = opts[irand() % n];
	}
	if (nd < 0 || nseg >= MAXSEG)
	{
		AddJoint(x, y, z, RADIUS * 1.05, k);    // boxed in: cap it, start another
		Spawn(k);
		return;
	}
	if (nd != d && nseg > 0)
		AddJoint(x, y, z, JOINT, k);             // ball joint at the turn

	segX[nseg] = x;
	segY[nseg] = y;
	segZ[nseg] = z;
	segDir[nseg] = nd;
	segStep[nseg] = step;
	segR[nseg] = pR[k];
	segG[nseg] = pG[k];
	segB[nseg] = pB[k];
	nseg++;

	pX[k] = x + dirX[nd];
	pY[k] = y + dirY[nd];
	pZ[k] = z + dirZ[nd];
	pDir[k] = nd;
	grid[Cell(pX[k], pY[k], pZ[k])] = 1;
}

void GrowAll()
{
	step++;
	for (int k = 0; k < pipes; k++)
	{
		if (!pAlive[k])
			Spawn(k);
		if (pAlive[k])
			Grow(k);
	}
	if (nseg >= MAXSEG - MAXPIPES || nj >= MAXJ - 2 * MAXPIPES || nseg >= CELLS * 0.42)
	{
		full = 1;
		fullTimer = 0.0;
	}
}

// ---- shaders ----
char* tubeVS =
	"#version 120\n"
	"varying vec3 vN;\n"
	"varying vec3 vP;\n"
	"varying vec3 vAlbedo;\n"
	"void main()\n"
	"{\n"
	"	vec4 p = gl_ModelViewMatrix * gl_Vertex;\n"
	"	vP = p.xyz;\n"
	"	vN = gl_NormalMatrix * gl_Normal;\n"   // the normal matrix undoes the scaling
	"	vAlbedo = gl_Color.rgb;\n"
	"	gl_Position = gl_ProjectionMatrix * p;\n"
	"}\n";

char* tubeFS =
	"#version 120\n"
	"varying vec3 vN;\n"
	"varying vec3 vP;\n"
	"varying vec3 vAlbedo;\n"
	"\n"
	"vec3 studio(vec3 r)\n"                     // the room the tubes reflect
	"{\n"
	"	vec3 c = mix(vec3(0.015), vec3(0.16, 0.17, 0.20), smoothstep(-0.45, 0.25, r.y));\n"
	"	c = mix(c, vec3(0.55, 0.60, 0.70), smoothstep(0.25, 1.0, r.y));\n"
	"	c += vec3(3.2, 3.0, 2.7) * smoothstep(0.80, 0.95, dot(r, normalize(vec3(-0.50, 0.60, 0.62))));\n"
	"	c += vec3(0.9, 1.2, 1.7) * smoothstep(0.92, 0.99, dot(r, normalize(vec3(0.90, 0.20, 0.40))));\n"
	"	return c;\n"
	"}\n"
	"\n"
	"vec3 aces(vec3 x)\n"
	"{\n"
	"	return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);\n"
	"}\n"
	"\n"
	"void main()\n"
	"{\n"
	"	vec3 N = normalize(vN);\n"
	"	vec3 V = normalize(-vP);\n"
	"	if (dot(N, V) < 0.0) N = -N;\n"
	"	float NdV = max(dot(N, V), 1e-3);\n"
	"	vec3 albedo = vAlbedo;\n"
	"	float metal = 0.45;\n"                  // glossy lacquered metal
	"	vec3 F0 = mix(vec3(0.04), albedo * 1.5 + 0.04, metal);\n"
	"	vec3 F = F0 + (1.0 - F0) * pow(1.0 - NdV, 5.0);\n"
	"\n"
	"	vec3 L1 = normalize(vec3(-0.55, 0.75, 0.65));\n"   // warm key
	"	vec3 C1 = vec3(1.00, 0.90, 0.78) * 1.8;\n"
	"	vec3 L2 = normalize(vec3(-0.80, -0.15, 0.45));\n"  // cool fill
	"	vec3 C2 = vec3(0.45, 0.60, 1.00) * 0.6;\n"
	"	vec3 L3 = normalize(vec3(0.30, 0.55, -1.00));\n"   // rim
	"	vec3 C3 = vec3(1.00, 0.95, 0.90) * 1.8;\n"
	"	float d1 = max(dot(N, L1), 0.0);\n"
	"	float d2 = max(dot(N, L2), 0.0);\n"
	"	float d3 = max(dot(N, L3), 0.0);\n"
	"	vec3 H1 = normalize(L1 + V);\n"
	"	vec3 H2 = normalize(L2 + V);\n"
	"	float n1 = 120.0;\n"
	"	float n2 = 40.0;\n"
	"	vec3 spec = (C1 * d1 * pow(max(dot(N, H1), 0.0), n1) * (n1 + 8.0) / 25.13\n"
	"	           + C2 * d2 * pow(max(dot(N, H2), 0.0), n2) * (n2 + 8.0) / 25.13) * F;\n"
	"	vec3 env = studio(reflect(-V, N)) * F;\n"
	"	vec3 hemi = mix(vec3(0.03, 0.025, 0.02), vec3(0.16, 0.18, 0.22), N.y * 0.5 + 0.5);\n"
	"	vec3 diffuse = albedo * (1.0 - metal) * (hemi + C1 * d1 + C2 * d2) + albedo * metal * hemi * 1.5;\n"
	"	float rim = pow(1.0 - NdV, 3.0) * d3;\n"
	"	vec3 col = diffuse + spec + env + C3 * rim * mix(albedo, vec3(1.0), 0.4);\n"
	"	col = aces(col);\n"
	"	gl_FragColor = vec4(pow(col, vec3(1.0 / 2.2)), 1.0);\n"
	"}\n";

char* backVS =
	"#version 120\n"
	"varying vec2 vUV;\n"
	"void main()\n"
	"{\n"
	"	vUV = gl_Vertex.xy;\n"
	"	gl_Position = vec4(gl_Vertex.xy, 0.0, 1.0);\n"
	"}\n";

char* backFS =
	"#version 120\n"
	"uniform float uAspect;\n"
	"varying vec2 vUV;\n"
	"void main()\n"
	"{\n"
	"	vec2 p = vec2(vUV.x * uAspect, vUV.y);\n"
	"	vec3 c = mix(vec3(0.012, 0.012, 0.016), vec3(0.070, 0.080, 0.105), smoothstep(-1.0, 1.0, vUV.y));\n"
	"	c += vec3(0.08, 0.075, 0.07) * exp(-dot(p, p) * 1.4);\n"
	"	c *= 1.0 - 0.18 * dot(vUV, vUV);\n"
	"	gl_FragColor = vec4(pow(c, vec3(1.0 / 2.2)), 1.0);\n"
	"}\n";

int tubeProg = 0;
int backProg = 0;
int uAspect;
int cylList = 0;
int sphList = 0;

// Unit cylinder along +z from 0 to 1, radius 1, no caps (joints cover them).
void MakeCylinder(int slices)
{
	cylList = glGenLists(1);
	glNewList(cylList, GL_COMPILE);
	glBegin(GL_QUAD_STRIP);
	for (int i = 0; i <= slices; i++)
	{
		float a = 2.0 * M_PI * i / slices;
		float c = cos(a);
		float s = sin(a);
		glNormal3f(c, s, 0.0);
		glVertex3f(c, s, 1.0);
		glVertex3f(c, s, 0.0);
	}
	glEnd();
	glEndList();
}

void MakeSphere(int slices, int stacks)
{
	sphList = glGenLists(1);
	glNewList(sphList, GL_COMPILE);
	for (int i = 0; i < stacks; i++)
	{
		float a0 = M_PI * i / stacks - M_PI / 2.0;
		float a1 = M_PI * (i + 1) / stacks - M_PI / 2.0;
		glBegin(GL_QUAD_STRIP);
		for (int j = 0; j <= slices; j++)
		{
			float b = 2.0 * M_PI * j / slices;
			glNormal3f(cos(a0) * cos(b), sin(a0), cos(a0) * sin(b));
			glVertex3f(cos(a0) * cos(b), sin(a0), cos(a0) * sin(b));
			glNormal3f(cos(a1) * cos(b), sin(a1), cos(a1) * sin(b));
			glVertex3f(cos(a1) * cos(b), sin(a1), cos(a1) * sin(b));
		}
		glEnd();
	}
	glEndList();
}

float Coord(int i)
{
	return i - (G - 1) * 0.5;
}

// Rotate the unit cylinder's +z onto grid direction d.
void AlignZ(int d)
{
	if (d == 0) glRotatef(90.0, 0.0, 1.0, 0.0);
	else if (d == 1) glRotatef(-90.0, 0.0, 1.0, 0.0);
	else if (d == 2) glRotatef(-90.0, 1.0, 0.0, 0.0);
	else if (d == 3) glRotatef(90.0, 1.0, 0.0, 0.0);
	else if (d == 5) glRotatef(180.0, 0.0, 1.0, 0.0);
}

void DrawScene()
{
	glUseProgram(tubeProg);
	for (int i = 0; i < nseg; i++)
	{
		float len = 1.0;
		if (segStep[i] == step && !full)
			len = progress;                   // the newest segments are still growing
		if (len <= 0.001)
			continue;
		glColor3f(segR[i], segG[i], segB[i]);
		glPushMatrix();
		glTranslatef(Coord(segX[i]), Coord(segY[i]), Coord(segZ[i]));
		AlignZ(segDir[i]);
		glScalef(RADIUS, RADIUS, len);
		glCallList(cylList);
		glPopMatrix();
	}
	for (int i = 0; i < nj; i++)
	{
		float s = jS[i];
		glColor3f(jR[i], jG[i], jB[i]);
		glPushMatrix();
		glTranslatef(Coord(jX[i]), Coord(jY[i]), Coord(jZ[i]));
		glScalef(s, s, s);
		glCallList(sphList);
		glPopMatrix();
	}
	glUseProgram(0);
}

void DrawBackdrop()
{
	glDisable(GL_DEPTH_TEST);
	glDepthMask(0);
	glUseProgram(backProg);
	glUniform1f(uAspect, (1.0 * glwWidth()) / glwHeight());
	glBegin(GL_QUADS);
	glVertex2f(-1.0, -1.0);
	glVertex2f(1.0, -1.0);
	glVertex2f(1.0, 1.0);
	glVertex2f(-1.0, 1.0);
	glEnd();
	glUseProgram(0);
	glDepthMask(1);
	glEnable(GL_DEPTH_TEST);
}

void Reshape(int w, int h)
{
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(42.0, (1.0 * w) / h, 1.0, 200.0);
	glMatrixMode(GL_MODELVIEW);
}

int main()
{
	glwMultisample(4);
	if (!glwInit(1024, 768, "Scinc tubes"))
	{
		printf("Cannot open OpenGL window\n");
		return 1;
	}
	tubeProg = glwCreateProgram(tubeVS, tubeFS);
	backProg = glwCreateProgram(backVS, backFS);
	if (tubeProg == 0 || backProg == 0)
		return 1;
	uAspect = glGetUniformLocation(backProg, "uAspect");
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_MULTISAMPLE);
	MakeCylinder(24);
	MakeSphere(24, 16);
	NewScene();

	float yaw = 0.6;
	float pitch = 0.45;
	float dist = 30.0;
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

		int key = glwGetKey();
		while (key != 0)
		{
			if (key == GLW_KEY_ESC || key == 'q' || key == 'Q')
				running = 0;
			else if (key == ' ')
				paused = 1 - paused;
			else if (key == 'n' || key == 'N')
				NewScene();
			else if (key == ']' && pipes < MAXPIPES)
			{
				pipes++;
				Spawn(pipes - 1);
			}
			else if (key == '[' && pipes > 1)
			{
				pAlive[pipes - 1] = 0;
				pipes--;
			}
			else if ((key == '+' || key == '=') && stepsPerSec < 60.0)
				stepsPerSec = stepsPerSec * 1.5;
			else if (key == '-' && stepsPerSec > 1.0)
				stepsPerSec = stepsPerSec / 1.5;
			else if (key == 'h' || key == 'H')
			{
				yaw = 0.6;
				pitch = 0.45;
				dist = 30.0;
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
				if (pitch < -1.45) pitch = -1.45;
				if (pitch > 1.45) pitch = 1.45;
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
			if (dist < 8.0) dist = 8.0;
			if (dist > 80.0) dist = 80.0;
		}

		if (!paused)
		{
			if (!dragging)
				yaw = yaw + 0.08 * dt;        // slow turntable
			if (full)
			{
				fullTimer = fullTimer + dt;
				if (fullTimer > 2.5)
					NewScene();
			}
			else
			{
				progress = progress + dt * stepsPerSec;
				while (progress >= 1.0 && !full)
				{
					progress = progress - 1.0;
					GrowAll();
				}
			}
		}

		if (glwResized())
			Reshape(glwWidth(), glwHeight());

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		DrawBackdrop();
		glLoadIdentity();
		float cp = cos(pitch);
		gluLookAt(dist * cp * sin(yaw), dist * sin(pitch), dist * cp * cos(yaw),
			0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
		DrawScene();

		if (saveNext)
		{
			if (glwSaveBMP("tubes.bmp"))
				printf("wrote tubes.bmp\n");
			fflush(stdout);
			saveNext = 0;
		}
		glwSwap();

		frames++;
		if (now - tRate0 >= 1.0)
		{
			char title[96];
			snprintf(title, 96, "Scinc tubes - %d pipes, %d segments - %.1f FPS", pipes, nseg, frames / (now - tRate0));
			glwSetTitle(title);
			tRate0 = now;
			frames = 0;
		}
	}

	glDeleteProgram(tubeProg);
	glDeleteProgram(backProg);
	glwDone();
	return 0;
}
