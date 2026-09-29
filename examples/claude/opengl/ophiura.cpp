// ophiura.cpp - OpenGL port of ../../geometry/ophiura.cpp: a brittle star
// (Ophiura) whose arms wriggle over the sand.
//
// The arm motion and colours are computed exactly as in the original: every
// arm is a chain of ~150 beads whose heading swings with time and position
// along the arm, coloured by three drifting sine waves with segment bands.
// The original fakes the 3D look with a soft shadow blob and a white
// highlight per bead; here every bead is a real lit sphere resting on a
// procedurally textured sand floor, with soft blob shadows cast away from
// the light, so the camera can orbit it.
//
// Note: the original clears with g.rgb(.7,.6,.5), but Graph::rgb() swaps red
// and blue (packed 0xAARRGGBB colours are not affected), so on screen it is a
// blue-grey (.5,.6,.7). That is the default seabed here; B switches to the
// sand colour the code names.
//
//   ./build_so.sh           (once, builds scgl_dl.so)
//   Scinc ophiura.cpp
//
// Controls:
//   mouse drag  orbit the camera        wheel   zoom
//   - / +       number of arms (3..10)  space   pause / resume
//   H           home (top view, as the original)
//   B           seabed: blue-grey (as the original shows) / sand
//   F           fullscreen              S       save ophiura.bmp
//   Esc / Q     quit

#include <stdio.h>
#include "scgl.h"

#define M_PI 3.141592654

#define MAXARMS 10
#define MAXBEADS 1600          // MAXARMS * 151 steps along an arm, rounded up
#define SAND 256               // sand texture size
#define BLOB 64                // shadow texture size

float t;

// ---- the original's random generator and arm lengths ----
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

float ls[MAXARMS];             // arm lengths (in steps of 0.2)
int arms = 6;

// ---- beads of the current frame ----
float bx[MAXBEADS];            // position on the floor (x, z)
float bz[MAXBEADS];
float br[MAXBEADS];            // sphere radius
float bf[MAXBEADS];            // the original's thickness factor (shadow strength)
float bcr[MAXBEADS];           // colour
float bcg[MAXBEADS];
float bcb[MAXBEADS];
int nbeads = 0;

// Light comes from above, from the top of the original screen (-z), so the
// highlight sits on the upper side of each bead and the shadow falls below it.
float lightDir[4] = {0.30, 1.0, -0.55, 0.0};
float lightAmb[4] = {0.0, 0.0, 0.0, 1.0};
float lightDiff[4] = {0.85, 0.85, 0.85, 1.0};
float lightSpec[4] = {1.0, 1.0, 1.0, 1.0};
float ambient[4] = {0.42, 0.42, 0.42, 1.0};
float matSpec[4] = {0.62, 0.62, 0.62, 1.0};   // broad sheen: the original's big white highlight
float fogColor[4] = {0.50, 0.60, 0.70, 1.0};

// seabed colours: what the original shows, and what its code names
float bedBlue[3] = {0.50, 0.60, 0.70};
float bedSand[3] = {0.70, 0.60, 0.50};
int sandy = 0;

int sphereList = 0;
int bedTex[2];
int blobTex = 0;
int pixels[SAND * SAND];

// camera orbit around the star
#define HOME_YAW 0.0
#define HOME_PITCH 1.50
#define HOME_DIST 345.0
float yaw = HOME_YAW;
float pitch = HOME_PITCH;
float dist = HOME_DIST;

int Pack(float r, float g, float b, float a)
{
	if (r < 0.0) r = 0.0;
	if (r > 1.0) r = 1.0;
	if (g < 0.0) g = 0.0;
	if (g > 1.0) g = 1.0;
	if (b < 0.0) b = 0.0;
	if (b > 1.0) b = 1.0;
	if (a < 0.0) a = 0.0;
	if (a > 1.0) a = 1.0;
	return (int(a * 255.0) << 24) | (int(r * 255.0) << 16) | (int(g * 255.0) << 8) | int(b * 255.0);
}

// Tileable seabed: the base colour with wind ripples, blotches and
// per-pixel grain. All frequencies are whole periods over the tile.
int MakeBedTexture(float* base)
{
	int s = 12345;
	float k = 2.0 * M_PI / SAND;
	for (int y = 0; y < SAND; y++)
		for (int x = 0; x < SAND; x++)
		{
			float ripple = sin(x * k * 7.0 + 1.3 * sin(y * k * 2.0) + 0.6 * sin(y * k * 5.0 + x * k));
			float blotch = sin(x * k * 2.0 + 0.5) * sin(y * k * 3.0 + 1.1) + 0.5 * sin((x + y) * k * 3.0);
			int rnd = irand(s) % 1000;
			if (rnd < 0) rnd = -rnd;
			float grain = rnd / 1000.0 - 0.5;
			float v = 1.0 + 0.045 * ripple + 0.04 * blotch + 0.09 * grain;
			pixels[y * SAND + x] = Pack(base[0] * v, base[1] * v, base[2] * v, 1.0);
		}
	int tex = glwGenTexture();
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, 8);
	glwTexImage2D(SAND, SAND, pixels);
	return tex;
}

// Soft round shadow: black, alpha falling smoothly from the centre to 0 at
// the rim (the original's blurred black circle).
void MakeBlobTexture()
{
	for (int y = 0; y < BLOB; y++)
		for (int x = 0; x < BLOB; x++)
		{
			float dx = (x + 0.5) / BLOB * 2.0 - 1.0;
			float dy = (y + 0.5) / BLOB * 2.0 - 1.0;
			float d2 = dx * dx + dy * dy;
			float a = 0.0;
			if (d2 < 1.0)
				a = (1.0 - d2) * (1.0 - d2);
			pixels[y * BLOB + x] = Pack(0.0, 0.0, 0.0, a);
		}
	blobTex = glwGenTexture();
	glBindTexture(GL_TEXTURE_2D, blobTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glwTexImage2D(BLOB, BLOB, pixels);
}

// Unit sphere in a display list: latitude bands as quad strips, normal =
// position.
void MakeSphere(int slices, int stacks)
{
	sphereList = glGenLists(1);
	glNewList(sphereList, GL_COMPILE);
	for (int i = 0; i < stacks; i++)
	{
		float a0 = M_PI * i / stacks - M_PI / 2.0;
		float a1 = M_PI * (i + 1) / stacks - M_PI / 2.0;
		float y0 = sin(a0);
		float y1 = sin(a1);
		float r0 = cos(a0);
		float r1 = cos(a1);
		glBegin(GL_QUAD_STRIP);
		for (int j = 0; j <= slices; j++)
		{
			float b = 2.0 * M_PI * j / slices;
			float cb = cos(b);
			float sb = sin(b);
			// lower ring first: counter-clockwise seen from outside, so
			// back-face culling keeps the outer surface
			glNormal3f(r0 * cb, y0, r0 * sb);
			glVertex3f(r0 * cb, y0, r0 * sb);
			glNormal3f(r1 * cb, y1, r1 * sb);
			glVertex3f(r1 * cb, y1, r1 * sb);
		}
		glEnd();
	}
	glEndList();
}

// The original's arm walk, recording one bead per step instead of drawing
// three circles. Screen x -> world x, screen y (down) -> world z.
void ComputeArms()
{
	float xs[MAXARMS];
	float ys[MAXARMS];
	for (int j = 0; j < MAXARMS; j++)
	{
		xs[j] = 0.0;
		ys[j] = 0.0;
	}
	nbeads = 0;
	for (float i = 0; i < 30.; i += 0.2)
	{
		for (int j = 0; j < arms; j++)
		{
			if (i > ls[j])
				continue;
			float f = i / ls[j];
			float fw = cos(120. / (2. - f));
			fw = fw < 0. ? -fw : fw;
			fw = pow(fw, .2);
			// the original packs these as 0xAARRGGBB bytes: blue, green, red
			float cb = fw * (80. * sin(14 + t * .7 + f * 1.7 + j) + 100.) / 255.0;
			float cg = fw * (80. * sin(41 + t * .6 + f * 1.6 + j) + 100.) / 255.0;
			float cr = fw * (80. * sin(23 + t * .8 + f * 1.5 + j) + 100.) / 255.0;
			float af = t * .1 + j + sin(f * 12 * (11.7 + j * 1.71) * .05 - t * (26.3 + j * 1.87) * .02 + j * 3.1 + 117) * f * 5;
			xs[j] += sin(af) * 1.8;
			ys[j] += cos(af) * 1.8;
			f = (1. - f) + fw * fw * .5;

			if (nbeads < MAXBEADS)
			{
				bx[nbeads] = xs[j];
				bz[nbeads] = ys[j];
				br[nbeads] = 1.2 + 2.0 * f;
				bf[nbeads] = f;
				bcr[nbeads] = cr;
				bcg[nbeads] = cg;
				bcb[nbeads] = cb;
				nbeads++;
			}
		}
	}
}

// The floor is a grid of quads, not one big quad: fixed-function fog may be
// computed per vertex, and a single quad with far-away corners would be
// fogged all over.
int floorList = 0;

void MakeFloor()
{
	float e = 3000.0;          // floor half-size
	int cells = 60;
	float step = 2.0 * e / cells;
	float rep = 1.0 / 72.0;    // texture repeats every 72 units
	floorList = glGenLists(1);
	glNewList(floorList, GL_COMPILE);
	for (int i = 0; i < cells; i++)
	{
		float z0 = -e + i * step;
		float z1 = z0 + step;
		glBegin(GL_QUAD_STRIP);
		for (int j = 0; j <= cells; j++)
		{
			float x = -e + j * step;          // z0 first: faces up (+y)
			glTexCoord2f(x * rep, z0 * rep);
			glVertex3f(x, 0.0, z0);
			glTexCoord2f(x * rep, z1 * rep);
			glVertex3f(x, 0.0, z1);
		}
		glEnd();
	}
	glEndList();
}

void DrawFloor()
{
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, bedTex[sandy]);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glCallList(floorList);
	glDisable(GL_TEXTURE_2D);
}

// A bead at height r casts its shadow away from the light, stretched a bit;
// overlapping blobs darken each other just like the original's circles.
void DrawShadows()
{
	float ox = -lightDir[0] / lightDir[1];
	float oz = -lightDir[2] / lightDir[1];
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, blobTex);
	glDepthMask(0);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(-1.0, -4.0);
	glBegin(GL_QUADS);
	for (int k = 0; k < nbeads; k++)
	{
		float r = br[k];
		float cx = bx[k] + ox * r;
		float cz = bz[k] + oz * r;
		float s = r * 1.55;
		glColor4f(0.0, 0.0, 0.0, 0.20 * (0.5 + bf[k]));
		glTexCoord2f(0.0, 0.0);
		glVertex3f(cx - s, 0.0, cz - s);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(cx - s, 0.0, cz + s);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(cx + s, 0.0, cz + s);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(cx + s, 0.0, cz - s);
	}
	glEnd();
	glDisable(GL_POLYGON_OFFSET_FILL);
	glDepthMask(1);
	glDisable(GL_TEXTURE_2D);
}

void DrawBeads()
{
	glEnable(GL_LIGHTING);
	for (int k = 0; k < nbeads; k++)
	{
		float r = br[k];
		glColor3f(bcr[k], bcg[k], bcb[k]);
		glPushMatrix();
		glTranslatef(bx[k], r, bz[k]);     // resting on the sand
		glScalef(r, r, r);
		glCallList(sphereList);
		glPopMatrix();
	}
	glDisable(GL_LIGHTING);
}

void Reshape(int w, int h)
{
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(40.0, (1.0 * w) / h, 5.0, 6000.0);
	glMatrixMode(GL_MODELVIEW);
}

void InitGL()
{
	glClearColor(fogColor[0], fogColor[1], fogColor[2], 1.0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_NORMALIZE);
	glEnable(GL_MULTISAMPLE);
	glShadeModel(GL_SMOOTH);

	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiff);
	glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpec);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	glMaterialfv(GL_FRONT, GL_SPECULAR, matSpec);
	glMaterialf(GL_FRONT, GL_SHININESS, 14.0);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glEnable(GL_FOG);                  // the far sand fades into the background
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogfv(GL_FOG_COLOR, fogColor);
	glHint(GL_FOG_HINT, GL_NICEST);   // per-pixel fog where supported

	MakeSphere(20, 12);
	MakeFloor();
	bedTex[0] = MakeBedTexture(bedBlue);
	bedTex[1] = MakeBedTexture(bedSand);
	MakeBlobTexture();
}

int main()
{
	for (int j = 0; j < MAXARMS; j++)
	{
		ls[j] = 30 - frand(seed) * 10.;
		printf("%f ", ls[j]);
	}
	printf("\n");

	glwMultisample(4);
	if (!glwInit(1024, 768, "Scinc Ophiura"))
	{
		printf("Cannot open OpenGL window\n");
		return 1;
	}
	InitGL();

	float anim = 0.0;
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
			else if ((key == '+' || key == '=') && arms < MAXARMS)
				arms++;
			else if (key == '-' && arms > 3)
				arms--;
			else if (key == 'h' || key == 'H')
			{
				yaw = HOME_YAW;
				pitch = HOME_PITCH;
				dist = HOME_DIST;
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
				float* c = bedBlue;
				if (sandy)
					c = bedSand;
				fogColor[0] = c[0];
				fogColor[1] = c[1];
				fogColor[2] = c[2];
				glClearColor(c[0], c[1], c[2], 1.0);
				glFogfv(GL_FOG_COLOR, fogColor);
			}
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
				if (pitch < 0.12) pitch = 0.12;
				if (pitch > 1.50) pitch = 1.50;
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
			if (dist < 60.0) dist = 60.0;
			if (dist > 1500.0) dist = 1500.0;
		}

		if (!paused)
			anim = anim + dt;
		t = anim * 2;                      // the original's t = Time()*2
		ComputeArms();

		if (glwResized())
			Reshape(glwWidth(), glwHeight());

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glLoadIdentity();
		float cp = cos(pitch);
		gluLookAt(dist * cp * sin(yaw), dist * sin(pitch), dist * cp * cos(yaw),
			0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
		glLightfv(GL_LIGHT0, GL_POSITION, lightDir);
		glFogf(GL_FOG_START, dist * 1.3);
		glFogf(GL_FOG_END, dist * 4.5);

		DrawFloor();
		DrawShadows();
		DrawBeads();

		if (saveNext)
		{
			if (glwSaveBMP("ophiura.bmp"))
				printf("wrote ophiura.bmp\n");
			fflush(stdout);
			saveNext = 0;
		}
		glwSwap();

		frames++;
		if (now - tRate0 >= 1.0)
		{
			char title[96];
			snprintf(title, 96, "Scinc Ophiura - %d arms, %d beads - %.1f FPS", arms, nbeads, frames / (now - tRate0));
			glwSetTitle(title);
			tRate0 = now;
			frames = 0;
		}
	}

	glwDone();
	return 0;
}
