// gears.cpp - the classic glxgears demo, in Scinc, through the scgl wrapper.
// Three lit, flat-shaded gears built once into display lists and spun
// every frame; prints the frame rate every 5 seconds like glxgears.
//
//   ./build_so.sh           (once, builds scgl_dl.so)
//   Scinc gears.cpp
//
// Keys: arrows rotate the view, z/Z spin around the view axis,
//       mouse drag rotates too, Esc or Q quits.

#include <stdio.h>
#include "scgl.h"

#define M_PI 3.14159265358979

float view_rotx = 20.0;
float view_roty = 30.0;
float view_rotz = 0.0;
float angle = 0.0;

int gear1;
int gear2;
int gear3;

float pos[4]   = {5.0, 5.0, 10.0, 0.0};
float red[4]   = {0.8, 0.1, 0.0, 1.0};
float green[4] = {0.0, 0.8, 0.2, 1.0};
float blue[4]  = {0.2, 0.2, 1.0, 1.0};

// Draw a gear wheel. Input: inner radius (hole), outer radius (tooth
// middle), width, number of teeth and tooth depth.
void gear(float inner_radius, float outer_radius, float width, int teeth, float tooth_depth)
{
	float r0 = inner_radius;
	float r1 = outer_radius - tooth_depth / 2.0;
	float r2 = outer_radius + tooth_depth / 2.0;
	float da = 2.0 * M_PI / teeth / 4.0;
	float hw = width * 0.5;
	float a;
	float u;
	float v;
	float len;

	glShadeModel(GL_FLAT);
	glNormal3f(0.0, 0.0, 1.0);

	// front face
	glBegin(GL_QUAD_STRIP);
	for (int i = 0; i <= teeth; i++)
	{
		a = i * 2.0 * M_PI / teeth;
		glVertex3f(r0 * cos(a), r0 * sin(a), hw);
		glVertex3f(r1 * cos(a), r1 * sin(a), hw);
		if (i < teeth)
		{
			glVertex3f(r0 * cos(a), r0 * sin(a), hw);
			glVertex3f(r1 * cos(a + 3.0 * da), r1 * sin(a + 3.0 * da), hw);
		}
	}
	glEnd();

	// front sides of teeth
	glBegin(GL_QUADS);
	for (int i = 0; i < teeth; i++)
	{
		a = i * 2.0 * M_PI / teeth;
		glVertex3f(r1 * cos(a), r1 * sin(a), hw);
		glVertex3f(r2 * cos(a + da), r2 * sin(a + da), hw);
		glVertex3f(r2 * cos(a + 2.0 * da), r2 * sin(a + 2.0 * da), hw);
		glVertex3f(r1 * cos(a + 3.0 * da), r1 * sin(a + 3.0 * da), hw);
	}
	glEnd();

	glNormal3f(0.0, 0.0, -1.0);

	// back face
	glBegin(GL_QUAD_STRIP);
	for (int i = 0; i <= teeth; i++)
	{
		a = i * 2.0 * M_PI / teeth;
		glVertex3f(r1 * cos(a), r1 * sin(a), -hw);
		glVertex3f(r0 * cos(a), r0 * sin(a), -hw);
		if (i < teeth)
		{
			glVertex3f(r1 * cos(a + 3.0 * da), r1 * sin(a + 3.0 * da), -hw);
			glVertex3f(r0 * cos(a), r0 * sin(a), -hw);
		}
	}
	glEnd();

	// back sides of teeth
	glBegin(GL_QUADS);
	for (int i = 0; i < teeth; i++)
	{
		a = i * 2.0 * M_PI / teeth;
		glVertex3f(r1 * cos(a + 3.0 * da), r1 * sin(a + 3.0 * da), -hw);
		glVertex3f(r2 * cos(a + 2.0 * da), r2 * sin(a + 2.0 * da), -hw);
		glVertex3f(r2 * cos(a + da), r2 * sin(a + da), -hw);
		glVertex3f(r1 * cos(a), r1 * sin(a), -hw);
	}
	glEnd();

	// outward faces of teeth
	glBegin(GL_QUAD_STRIP);
	for (int i = 0; i < teeth; i++)
	{
		a = i * 2.0 * M_PI / teeth;

		glVertex3f(r1 * cos(a), r1 * sin(a), hw);
		glVertex3f(r1 * cos(a), r1 * sin(a), -hw);
		u = r2 * cos(a + da) - r1 * cos(a);
		v = r2 * sin(a + da) - r1 * sin(a);
		len = sqrt(u * u + v * v);
		u = u / len;
		v = v / len;
		glNormal3f(v, -u, 0.0);
		glVertex3f(r2 * cos(a + da), r2 * sin(a + da), hw);
		glVertex3f(r2 * cos(a + da), r2 * sin(a + da), -hw);
		glNormal3f(cos(a), sin(a), 0.0);
		glVertex3f(r2 * cos(a + 2.0 * da), r2 * sin(a + 2.0 * da), hw);
		glVertex3f(r2 * cos(a + 2.0 * da), r2 * sin(a + 2.0 * da), -hw);
		u = r1 * cos(a + 3.0 * da) - r2 * cos(a + 2.0 * da);
		v = r1 * sin(a + 3.0 * da) - r2 * sin(a + 2.0 * da);
		glNormal3f(v, -u, 0.0);
		glVertex3f(r1 * cos(a + 3.0 * da), r1 * sin(a + 3.0 * da), hw);
		glVertex3f(r1 * cos(a + 3.0 * da), r1 * sin(a + 3.0 * da), -hw);
		glNormal3f(cos(a), sin(a), 0.0);
	}
	glVertex3f(r1, 0.0, hw);
	glVertex3f(r1, 0.0, -hw);
	glEnd();

	glShadeModel(GL_SMOOTH);

	// inside radius cylinder
	glBegin(GL_QUAD_STRIP);
	for (int i = 0; i <= teeth; i++)
	{
		a = i * 2.0 * M_PI / teeth;
		glNormal3f(-cos(a), -sin(a), 0.0);
		glVertex3f(r0 * cos(a), r0 * sin(a), -hw);
		glVertex3f(r0 * cos(a), r0 * sin(a), hw);
	}
	glEnd();
}

void draw()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPushMatrix();
	glRotatef(view_rotx, 1.0, 0.0, 0.0);
	glRotatef(view_roty, 0.0, 1.0, 0.0);
	glRotatef(view_rotz, 0.0, 0.0, 1.0);

	glPushMatrix();
	glTranslatef(-3.0, -2.0, 0.0);
	glRotatef(angle, 0.0, 0.0, 1.0);
	glCallList(gear1);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(3.1, -2.0, 0.0);
	glRotatef(-2.0 * angle - 9.0, 0.0, 0.0, 1.0);
	glCallList(gear2);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-3.1, 4.2, 0.0);
	glRotatef(-2.0 * angle - 25.0, 0.0, 0.0, 1.0);
	glCallList(gear3);
	glPopMatrix();

	glPopMatrix();
}

// new window size or exposure
void reshape(int width, int height)
{
	float h = (1.0 * height) / width;

	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-1.0, 1.0, -h, h, 5.0, 60.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0, 0.0, -40.0);
}

void init()
{
	glLightfv(GL_LIGHT0, GL_POSITION, pos);
	glEnable(GL_CULL_FACE);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_DEPTH_TEST);

	// make the gears
	gear1 = glGenLists(1);
	glNewList(gear1, GL_COMPILE);
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, red);
	gear(1.0, 4.0, 1.0, 20, 0.7);
	glEndList();

	gear2 = glGenLists(1);
	glNewList(gear2, GL_COMPILE);
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, green);
	gear(0.5, 2.0, 2.0, 10, 0.7);
	glEndList();

	gear3 = glGenLists(1);
	glNewList(gear3, GL_COMPILE);
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, blue);
	gear(1.3, 2.0, 0.5, 10, 0.7);
	glEndList();

	glEnable(GL_NORMALIZE);
}

int main()
{
	if (!glwInit(300, 300, "Scinc gears"))
	{
		printf("Cannot open OpenGL window\n");
		return 1;
	}

	char info[256];
	glwGetString(GL_RENDERER, info, 256);
	printf("GL_RENDERER   = %s\n", info);
	glwGetString(GL_VERSION, info, 256);
	printf("GL_VERSION    = %s\n", info);
	glwGetString(GL_VENDOR, info, 256);
	printf("GL_VENDOR     = %s\n", info);

	init();

	int frames = 0;
	double tRate0 = glwTime();
	double tPrev = glwTime();
	int dragging = 0;
	int lastX = 0;
	int lastY = 0;
	int running = 1;

	while (running && glwPoll())
	{
		// advance rotation for next frame: 70 degrees per second, as glxgears
		double t = glwTime();
		float dt = t - tPrev;
		angle = angle + 70.0 * dt;
		if (angle > 3600.0)
			angle = angle - 3600.0;
		tPrev = t;

		int key = glwGetKey();
		while (key != 0)
		{
			if (key == GLW_KEY_ESC || key == 'q' || key == 'Q')
				running = 0;
			else if (key == 'z')
				view_rotz = view_rotz + 5.0;
			else if (key == 'Z')
				view_rotz = view_rotz - 5.0;
			key = glwGetKey();
		}
		// arrows are polled so holding one rotates smoothly (90 deg/s)
		if (glwKeyDown(GLW_KEY_UP))
			view_rotx = view_rotx + 90.0 * dt;
		if (glwKeyDown(GLW_KEY_DOWN))
			view_rotx = view_rotx - 90.0 * dt;
		if (glwKeyDown(GLW_KEY_LEFT))
			view_roty = view_roty + 90.0 * dt;
		if (glwKeyDown(GLW_KEY_RIGHT))
			view_roty = view_roty - 90.0 * dt;

		// left-drag rotates the view
		int mx = glwMouseX();
		int my = glwMouseY();
		if (glwMouseButtons() & 1)
		{
			if (dragging)
			{
				view_roty = view_roty + 0.5 * (mx - lastX);
				view_rotx = view_rotx + 0.5 * (my - lastY);
			}
			dragging = 1;
		}
		else
			dragging = 0;
		lastX = mx;
		lastY = my;

		if (glwResized())
			reshape(glwWidth(), glwHeight());

		draw();
		glwSwap();

		frames++;
		if (t - tRate0 >= 5.0)
		{
			float seconds = t - tRate0;
			float fps = frames / seconds;
			printf("%d frames in %3.1f seconds = %6.3f FPS\n", frames, seconds, fps);
			fflush(stdout);
			char title[64];
			snprintf(title, 64, "Scinc gears - %.1f FPS", fps);
			glwSetTitle(title);
			tRate0 = t;
			frames = 0;
		}
	}

	glwDone();
	return 0;
}
