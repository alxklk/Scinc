// func_graph_3d.cpp - OpenGL version of ../../geometry/func_graph_3d.cpp:
// an animated surface z = f(x, y) drawn as a lit, filled height field with a
// wireframe on top, turning slowly. The function is evaluated in Scinc every
// frame (just like the original); OpenGL does the 3D: perspective, depth
// buffer, lighting and smooth shading.
//
//   ./build_so.sh           (once, builds scgl_dl.so)
//   Scinc func_graph_3d.cpp
//
// Controls:
//   1..5        function: ripple (the original), egg crate, sinc, two-source
//               interference, saddle
//   M           mode: fill + wire / fill / wire
//   C           colouring: slope (as the original) / height
//   - / +       grid resolution            space   pause / resume
//   mouse drag  orbit the camera           wheel   zoom
//   H           home view                  F       fullscreen
//   Esc / Q     quit

#include <stdio.h>
#include "scgl.h"

#define M_PI 3.14159265358979

#define NMAX 80                    // max grid half-size (cells per side = 2*N)
#define WMAX (2 * NMAX + 1)
#define DOMAIN 40.0                // x, y range is [-DOMAIN, DOMAIN]
#define HSCALE 4.3                 // world height per unit of f (30px/7px in the original)

#define NODES (WMAX * WMAX)
#define CELLS (2 * NMAX * 2 * NMAX)

// Mesh in vertex-array form, drawn with one glDrawElements call each for the
// fill and the wire. Grid x -> world X, grid y -> world Z, f -> world Y (up).
float h[NODES];                    // f at the grid nodes
float pos[NODES * 3];              // x, y, z per node (x, z fixed per resolution)
float nrm[NODES * 3];              // normal per node
float col[NODES * 3];              // colour per node
int fillIdx[CELLS * 6];            // two triangles per cell
int wireIdx[CELLS * 4 + WMAX * 4]; // grid-line segments
int fillCount = 0;
int wireCount = 0;

int N = 40;                        // grid half-size (the original uses 40)
int W = 81;                        // nodes per side = 2*N+1
float t = 0.0;                     // animation time
int func = 1;
int mode = 0;                      // 0 fill+wire, 1 fill, 2 wire
int heightColors = 0;

float yaw = 0.0;
float pitch = 0.62;
float dist = 118.0;

float lightPos[4]  = {-0.4, 1.0, 0.3, 0.0};    // directional, fixed in the world
float lightDiff[4] = {0.62, 0.62, 0.62, 1.0};
float lightSpec[4] = {0.6, 0.6, 0.6, 1.0};
float ambient[4]   = {0.18, 0.18, 0.18, 1.0};
float matSpec[4]   = {0.35, 0.35, 0.35, 1.0};

char* funcName(int k)
{
	if (k == 1) return "ripple  sin(0.4 r + t)";
	if (k == 2) return "egg crate  sin(x/4 + t) cos(y/4 + 0.7t)";
	if (k == 3) return "sinc  sin(r)/r";
	if (k == 4) return "interference of two point sources";
	return "saddle  (x^2 - y^2) cos t";
}

float f(float x, float y)
{
	if (func == 1)
		return sin(sqrt(x * x + y * y) * 0.4 + t);
	if (func == 2)
		return sin(x * 0.25 + t) * cos(y * 0.25 + t * 0.7);
	if (func == 3)
	{
		float r = sqrt(x * x + y * y) * 0.45 + 1e-4;
		return 1.6 * sin(r) / r * (0.75 + 0.25 * cos(t * 1.3)) - 0.3;   // breathing
	}
	if (func == 4)
	{
		float sx = 13.0 * cos(t * 0.3);
		float sy = 13.0 * sin(t * 0.3);
		float d1 = sqrt((x - sx) * (x - sx) + (y - sy) * (y - sy));
		float d2 = sqrt((x + sx) * (x + sx) + (y + sy) * (y + sy));
		return 0.5 * (sin(d1 * 0.6 - t * 3.0) + sin(d2 * 0.6 - t * 3.0));
	}
	return (x * x - y * y) / (DOMAIN * DOMAIN) * 1.5 * cos(t * 0.7);
}

// Build the static part of the mesh for the current resolution: x/z of the
// nodes and the triangle / line index lists.
void BuildMesh()
{
	float s = DOMAIN / N;
	for (int i = 0; i < W; i++)
		for (int j = 0; j < W; j++)
		{
			int k = (i * W + j) * 3;
			pos[k] = (j - N) * s;
			pos[k + 2] = (i - N) * s;
			nrm[k + 1] = 1.0;
		}

	fillCount = 0;
	for (int i = 0; i < W - 1; i++)
		for (int j = 0; j < W - 1; j++)
		{
			int a = i * W + j;
			fillIdx[fillCount] = a;
			fillIdx[fillCount + 1] = a + W;
			fillIdx[fillCount + 2] = a + 1;
			fillIdx[fillCount + 3] = a + 1;
			fillIdx[fillCount + 4] = a + W;
			fillIdx[fillCount + 5] = a + W + 1;
			fillCount = fillCount + 6;
		}

	wireCount = 0;
	for (int i = 0; i < W; i++)
		for (int j = 0; j < W - 1; j++)
		{
			wireIdx[wireCount] = i * W + j;          // along x
			wireIdx[wireCount + 1] = i * W + j + 1;
			wireIdx[wireCount + 2] = j * W + i;      // along y
			wireIdx[wireCount + 3] = (j + 1) * W + i;
			wireCount = wireCount + 4;
		}
}

// Sample f on the grid, then per node: height, normal from the slopes
// (central differences, one-sided at the border) and colour. The original
// tints red by the x-slope and green by the y-slope; the alternative is a
// blue-cyan-yellow-red height map.
void Evaluate()
{
	float s = DOMAIN / N;
	for (int i = 0; i < W; i++)
	{
		float y = (i - N) * s;
		int row = i * W;
		for (int j = 0; j < W; j++)
			h[row + j] = f((j - N) * s, y);
	}

	for (int i = 0; i < W; i++)
	{
		int i0 = i - 1;
		int i1 = i + 1;
		if (i0 < 0) i0 = 0;
		if (i1 > W - 1) i1 = W - 1;
		float sy = 1.0 / ((i1 - i0) * s);
		for (int j = 0; j < W; j++)
		{
			int j0 = j - 1;
			int j1 = j + 1;
			if (j0 < 0) j0 = 0;
			if (j1 > W - 1) j1 = W - 1;
			int k = i * W + j;
			float v = h[k];
			float dx = (h[i * W + j1] - h[i * W + j0]) / ((j1 - j0) * s);
			float dy = (h[i1 * W + j] - h[i0 * W + j]) * sy;
			int k3 = k * 3;
			pos[k3 + 1] = v * HSCALE;
			nrm[k3] = -dx * HSCALE;
			nrm[k3 + 2] = -dy * HSCALE;

			float r;
			float g;
			float b;
			if (heightColors)
			{
				float u = (v + 1.0) * 0.5;
				if (u < 0.0) u = 0.0;
				if (u > 1.0) u = 1.0;
				if (u < 0.33)      { u = u / 0.33;          r = 0.10;            g = 0.20 + 0.60 * u; b = 0.85; }
				else if (u < 0.66) { u = (u - 0.33) / 0.33; r = 0.10 + 0.85 * u; g = 0.80 + 0.15 * u; b = 0.85 - 0.75 * u; }
				else               { u = (u - 0.66) / 0.34; r = 0.95;            g = 0.95 - 0.75 * u; b = 0.10; }
			}
			else
			{
				r = 0.5 + 1.1 * dx;
				g = 0.5 + 1.1 * dy;
				b = 0.08;
				if (r < 0.0) r = 0.0;
				if (r > 1.0) r = 1.0;
				if (g < 0.0) g = 0.0;
				if (g > 1.0) g = 1.0;
			}
			col[k3] = r;
			col[k3 + 1] = g;
			col[k3 + 2] = b;
		}
	}
}

// The fill is lit (at most ~80% of the node colour), the wire is drawn unlit
// at full colour, so the lines stand out from the surface as in the original.
void DrawSurface()
{
	glwVertexArray(3, pos);
	glwNormalArray(nrm);
	glwColorArray(3, col);
	if (mode != 2)
	{
		glEnable(GL_LIGHTING);
		glEnable(GL_POLYGON_OFFSET_FILL);    // push the fill back so the wire sits on top
		glPolygonOffset(1.0, 1.0);
		glDrawElements(GL_TRIANGLES, fillCount, fillIdx);
		glDisable(GL_POLYGON_OFFSET_FILL);
		glDisable(GL_LIGHTING);
	}
	if (mode != 1)
		glDrawElements(GL_LINES, wireCount, wireIdx);
	glwDisableArrays();
}

void Reshape(int w, int hgt)
{
	glViewport(0, 0, w, hgt);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(40.0, (1.0 * w) / hgt, 2.0, 600.0);
	glMatrixMode(GL_MODELVIEW);
}

void InitGL()
{
	glClearColor(0.3, 0.15, 0.1, 1.0);      // the original's background
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	glShadeModel(GL_SMOOTH);

	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiff);
	glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpec);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);   // the underside is visible too

	glEnable(GL_COLOR_MATERIAL);                 // glColor drives ambient+diffuse
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpec);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 40.0);

	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glLineWidth(1.0);
}

void SetTitle(float fps)
{
	char title[160];
	snprintf(title, 160, "Scinc 3D graph - %s - grid %dx%d - %.1f FPS", funcName(func), 2 * N, 2 * N, fps);
	glwSetTitle(title);
}

int main()
{
	if (!glwInit(900, 675, "Scinc 3D function graph"))
	{
		printf("Cannot open OpenGL window\n");
		return 1;
	}
	InitGL();
	BuildMesh();

	int paused = 0;
	int fullscreen = 0;
	int dragging = 0;
	int lastX = 0;
	int lastY = 0;
	int running = 1;
	int frames = 0;
	double tPrev = glwTime();
	double tRate0 = tPrev;
	float fps = 0.0;
	SetTitle(fps);

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
			else if (key >= '1' && key <= '5')
				func = key - '0';
			else if (key == 'm' || key == 'M')
				mode = (mode + 1) % 3;
			else if (key == 'c' || key == 'C')
				heightColors = 1 - heightColors;
			else if (key == ' ')
				paused = 1 - paused;
			else if ((key == '+' || key == '=') && N < NMAX)
				N = N + 10;
			else if (key == '-' && N > 10)
				N = N - 10;
			else if (key == 'h' || key == 'H')
			{
				yaw = 0.0;
				pitch = 0.62;
				dist = 118.0;
			}
			else if (key == 'f' || key == 'F')
			{
				fullscreen = 1 - fullscreen;
				glwSetFullscreen(fullscreen);
			}
			if (2 * N + 1 != W)
			{
				W = 2 * N + 1;
				BuildMesh();
			}
			SetTitle(fps);
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
				if (pitch < -1.4) pitch = -1.4;
				if (pitch > 1.5) pitch = 1.5;
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
			if (dist < 30.0) dist = 30.0;
			if (dist > 400.0) dist = 400.0;
		}

		if (!paused)
		{
			t = t + dt;
			if (!dragging)
				yaw = yaw + 0.1 * dt;          // slow turn
		}

		if (glwResized())
			Reshape(glwWidth(), glwHeight());

		Evaluate();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glLoadIdentity();
		float cp = cos(pitch);
		gluLookAt(dist * cp * sin(yaw), dist * sin(pitch), dist * cp * cos(yaw),
			0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
		glLightfv(GL_LIGHT0, GL_POSITION, lightPos);   // after the view: fixed in the world

		DrawSurface();

		glwSwap();

		frames++;
		if (now - tRate0 >= 1.0)
		{
			fps = frames / (now - tRate0);
			SetTitle(fps);
			tRate0 = now;
			frames = 0;
		}
	}

	glwDone();
	return 0;
}
