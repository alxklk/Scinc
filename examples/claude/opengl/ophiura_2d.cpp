// ophiura_2d.cpp - faithful 2D OpenGL port of ../../geometry/ophiura.cpp.
//
// The original draws the brittle star with three soft round spots per bead
// (Graph::Circle): the coloured body, a dark shadow below it and a white
// highlight above it. Here every spot is one quad, and its round soft edge is
// computed per pixel by a small GLSL shader (embedded below) using the same
// profile as the native Circle, measured from the interpreter:
//
//   Circle(x, y, R, W, A, colour): ring of radius R and half-width W, fully
//   opaque, fading linearly to nothing over a further distance A.
//
// The arm walk, colours, spot sizes, opacities and drawing order are the
// original's; only the rasterisation moved to the GPU, so it stays smooth at
// any window size and zoom.
//
// Density: the GPU has room for more spots, so by default every arm is
// walked in 2x smaller steps (2x the spots along the same curve), which
// smooths the beaded outline. The translucent shadow and highlight spots
// then overlap k times as often, so their opacity a becomes
// 1 - (1 - a)^(1/k): the stack of k is exactly as opaque as one original
// spot. Density 1 reproduces the original frame exactly.
//
//   ./build_so.sh                  (once, builds scgl_dl.so)
//   Scinc ophiura_2d.cpp           animate
//   Scinc ophiura_2d.cpp 10        show the frame at t = 10, paused
//
// Controls:
//   - / +       number of arms (3..10)  space   pause / resume
//   [ / ]       spot density 1, 2, 4 (1 = the original's spacing)
//   wheel       zoom                    H       home (zoom 1)
//   B           background: blue-grey (as the original shows) / sand
//   F           fullscreen              S       save ophiura_2d.bmp
//   Esc / Q     quit
//
// The original clears with g.rgb(.7,.6,.5), but Graph::rgb() swaps red and
// blue (packed 0xAARRGGBB colours are not affected), so on screen it is a
// blue-grey (.5,.6,.7); that is the default here.

#include <stdio.h>
#include "scgl.h"
#include "../../algorithm/Token.h"   // SToken / MakeTokens: parse the CLI time
#include "../../algorithm/Lex.h"

typedef char* pchar;

#define M_PI 3.141592654
#define MAXARMS 10

// the original's logical screen
#define SCREEN_W 320.0
#define SCREEN_H 240.0

float t;

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

float ls[MAXARMS];
int arms = 6;
int density = 2;           // spots per original step: 1, 2 or 4
int spots = 0;

float bgBlue[3] = {0.50, 0.60, 0.70};
float bgSand[3] = {0.70, 0.60, 0.50};
int sandy = 0;
float zoom = 1.0;

int prog = 0;

// ---- the spot shader ----
// One soft round spot per quad. For all four corners the host sends the spot
// centre as the vertex, the corner (-1,-1)..(1,1) as the texture coordinate,
// the spot shape (R, W, A) as the normal and its colour; the vertex shader
// expands the quad to cover the whole spot.
char* spotVS =
	"#version 120\n"
	"varying vec2 vLocal;\n"           // position relative to the spot centre
	"varying vec3 vShape;\n"
	"varying vec4 vColor;\n"
	"void main()\n"
	"{\n"
	"	vShape = gl_Normal;\n"
	"	float extent = gl_Normal.x + gl_Normal.y + gl_Normal.z;\n"   // outermost visible radius
	"	vLocal = gl_MultiTexCoord0.xy * extent;\n"
	"	vColor = gl_Color;\n"
	"	gl_Position = gl_ModelViewProjectionMatrix * vec4(gl_Vertex.xy + vLocal, 0.0, 1.0);\n"
	"}\n";

// The profile of Graph::Circle(x, y, R, W, A, colour): a ring of radius R and
// half-width W, fully opaque, fading linearly to nothing over a further
// distance A. R = 0 makes it a disc of radius W, and W = 0 a cone of radius A.
char* spotFS =
	"#version 120\n"
	"varying vec2 vLocal;\n"
	"varying vec3 vShape;\n"           // R, W, A
	"varying vec4 vColor;\n"
	"void main()\n"
	"{\n"
	"	float d = length(vLocal);\n"
	"	float outside = max(abs(d - vShape.x) - vShape.y, 0.0);\n"
	"	float a = clamp(1.0 - outside / max(vShape.z, 1e-4), 0.0, 1.0);\n"
	"	gl_FragColor = vec4(vColor.rgb, vColor.a * a);\n"
	"}\n";

// Logical 320x240 coordinates (y down, as in the original), centred on the
// star and scaled uniformly to fit the window; zoom > 1 magnifies.
void SetView(int w, int h)
{
	glViewport(0, 0, w, h);
	float aspect = (1.0 * w) / h;
	float hh = SCREEN_H * 0.5;
	float hw = hh * aspect;
	if (aspect < SCREEN_W / SCREEN_H)
	{
		hw = SCREEN_W * 0.5;
		hh = hw / aspect;
	}
	hw = hw / zoom;
	hh = hh / zoom;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(160.0 - hw, 160.0 + hw, 120.0 + hh, 120.0 - hh, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

// One Circle(x, y, R, W, A, colour) of the original, as a quad: the vertex
// shader expands the four corners around the centre by R + W + A.
void Spot(float x, float y, float R, float W, float A, float r, float g, float b, float a)
{
	glColor4f(r, g, b, a);
	glNormal3f(R, W, A);
	glTexCoord2f(-1.0, -1.0);
	glVertex2f(x, y);
	glTexCoord2f(1.0, -1.0);
	glVertex2f(x, y);
	glTexCoord2f(1.0, 1.0);
	glVertex2f(x, y);
	glTexCoord2f(-1.0, 1.0);
	glVertex2f(x, y);
	spots++;
}

// The original's frame, line by line; each g.Circle became a Spot.
void DrawStar()
{
	float xs[MAXARMS];
	float ys[MAXARMS];
	for (int j = 0; j < MAXARMS; j++)
	{
		xs[j] = 0.0;
		ys[j] = 0.0;
	}
	spots = 0;
	glBegin(GL_QUADS);
	float step = 0.2 / density;
	float move = 1.8 / density;
	float invK = 1.0 / density;
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
			// packed as 0xAARRGGBB in the original: blue, green, red bytes
			float cb = int(fw * (80. * sin(14 + t * .7 + f * 1.7 + j) + 100.)) / 255.0;
			float cg = int(fw * (80. * sin(41 + t * .6 + f * 1.6 + j) + 100.)) / 255.0;
			float cr = int(fw * (80. * sin(23 + t * .8 + f * 1.5 + j) + 100.)) / 255.0;
			float af = t * .1 + j + sin(f * 12 * (11.7 + j * 1.71) * .05 - t * (26.3 + j * 1.87) * .02 + j * 3.1 + 117) * f * 5;
			xs[j] += sin(af) * move;
			ys[j] += cos(af) * move;
			float x1 = xs[j];
			float y1 = ys[j];
			f = (1. - f) + fw * fw * .5;
			// alpha bytes as the original computes them (an 8-bit field, so
			// values past 255 wrap around)
			float shadowA = (int(0x50 * (.5 + f)) & 0xff) / 255.0;
			float lightA = (int(0x80 * (.5 + f)) & 0xff) / 255.0;
			if (density > 1)
			{
				// k overlapping copies must add up to one original spot
				shadowA = 1.0 - pow(1.0 - shadowA, invK);
				lightA = 1.0 - pow(1.0 - lightA, invK);
			}
			Spot(160 + x1, 120 + y1,           0.0, f + 1.0,         2.0 * (f + .5), cr, cg, cb, 1.0);
			Spot(160 + x1, 120 + y1 + f * 2.0, 0.0, 0.0,             3.0 * f + 1.0,  0.0, 0.0, 0.0, shadowA);
			Spot(160 + x1, 120 + y1 - f * 2.5, 0.0, .5 * f + .5,     1.5,            1.0, 1.0, 1.0, lightA);
		}
	}
	glEnd();
}

int main(int argc, pchar* argv)
{
	for (int j = 0; j < MAXARMS; j++)
	{
		ls[j] = 30 - frand(seed) * 10.;
		printf("%f ", ls[j]);
	}
	printf("\n");

	float anim = 0.0;
	int paused = 0;
	if (argc >= 2)
	{
		SToken tk[16];
		MakeTokens(tk, argv[1]);
		if (tk[0].IsNumber())
		{
			anim = tk[0].GetNumber() / 2.0;
			paused = 1;
		}
	}

	glwMultisample(4);
	if (!glwInit(1280, 960, "Scinc Ophiura 2D"))   // the original: 320x240 at scale 4
	{
		printf("Cannot open OpenGL window\n");
		return 1;
	}
	prog = glwCreateProgram(spotVS, spotFS);
	if (prog == 0)
		return 1;
	glClearColor(bgBlue[0], bgBlue[1], bgBlue[2], 1.0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	int fullscreen = 0;
	int saveNext = 0;
	int running = 1;
	int viewDirty = 1;
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
			else if ((key == '+' || key == '=') && arms < MAXARMS)
				arms++;
			else if (key == '-' && arms > 3)
				arms--;
			else if (key == ']' && density < 4)
				density = density * 2;
			else if (key == '[' && density > 1)
				density = density / 2;
			else if (key == 'h' || key == 'H')
			{
				zoom = 1.0;
				viewDirty = 1;
			}
			else if (key == 'f' || key == 'F')
			{
				fullscreen = 1 - fullscreen;
				glwSetFullscreen(fullscreen);
			}
			else if (key == 's' || key == 'S')
				saveNext = 1;
			else if (key == 'b' || key == 'B')
			{
				sandy = 1 - sandy;
				if (sandy)
					glClearColor(bgSand[0], bgSand[1], bgSand[2], 1.0);
				else
					glClearColor(bgBlue[0], bgBlue[1], bgBlue[2], 1.0);
			}
			key = glwGetKey();
		}

		int wheel = glwMouseWheel();
		if (wheel != 0)
		{
			zoom = zoom * pow(1.15, wheel);
			if (zoom < 0.5) zoom = 0.5;
			if (zoom > 12.0) zoom = 12.0;
			viewDirty = 1;
		}
		if (glwResized())
			viewDirty = 1;
		if (viewDirty)
		{
			SetView(glwWidth(), glwHeight());
			viewDirty = 0;
		}

		if (!paused)
			anim = anim + dt;
		t = anim * 2;                      // the original's t = Time()*2

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(prog);
		DrawStar();
		glUseProgram(0);

		if (saveNext)
		{
			if (glwSaveBMP("ophiura_2d.bmp"))
				printf("wrote ophiura_2d.bmp\n");
			fflush(stdout);
			saveNext = 0;
		}
		glwSwap();

		frames++;
		if (now - tRate0 >= 1.0)
		{
			char title[96];
			snprintf(title, 96, "Scinc Ophiura 2D - %d arms, density %d, %d spots - %.1f FPS", arms, density, spots, frames / (now - tRate0));
			glwSetTitle(title);
			tRate0 = now;
			frames = 0;
		}
	}

	glDeleteProgram(prog);
	glwDone();
	return 0;
}
