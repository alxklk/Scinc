// raytrace_gl.cpp - the ../graphic/raytrace.cpp metal still life, ray traced
// in real time by a GLSL fragment shader on a fullscreen quad.
//
// Scinc sets up the window, owns the scene and camera, and feeds them to the
// shader as uniforms every frame; raytrace.frag traces every pixel.
//
//   ./build_so.sh                       (once, builds scgl_dl.so)
//   Scinc raytrace_gl.cpp [width height]      (default 640x480)
//   nvrun Scinc raytrace_gl.cpp               (on the NVIDIA GPU)
//
// Controls:
//   mouse drag  orbit the camera        wheel     zoom
//   space       pause/resume motion     H         home (the raytrace.cpp view)
//   1..4        anti-aliasing (N*N samples per pixel)
//   F           toggle fullscreen       S         save raytrace_gl.bmp
//   R           reload the shaders (edit raytrace.frag while it runs)
//   Esc / Q     quit

#include <stdio.h>
#include "scgl.h"
#include "../../algorithm/Token.h"   // SToken / MakeTokens: parse the CLI resolution
#include "../../algorithm/Lex.h"

typedef char* pchar;

// camera orbit around the target; the home values reproduce raytrace.cpp's
// eye (-2.5, 2.25, -5) looking at (0, 0.55, -0.15)
#define HOME_YAW   -2.0467384
#define HOME_PITCH  0.3020280
#define HOME_DIST   5.7151115

float yaw = HOME_YAW;
float pitch = HOME_PITCH;
float dist = HOME_DIST;
float cubeSpin = 0.0;       // extra rotation of the cube about its vertical diagonal

int prog = 0;
int uResolution;
int uEye;
int uTarget;
int uFov;
int uAA;
int uSphere;
int uCubeB;
int uCubeL;
int uCF0;
int uCF1;
int uCF2;
int uTorusC;
int uTorusR;

int LoadShaders()
{
	int p = glwLoadProgram("fullscreen.vert", "raytrace.frag");
	if (p == 0)
		return 0;
	if (prog != 0)
		glDeleteProgram(prog);
	prog = p;
	uResolution = glGetUniformLocation(prog, "uResolution");
	uEye        = glGetUniformLocation(prog, "uEye");
	uTarget     = glGetUniformLocation(prog, "uTarget");
	uFov        = glGetUniformLocation(prog, "uFov");
	uAA         = glGetUniformLocation(prog, "uAA");
	uSphere     = glGetUniformLocation(prog, "uSphere");
	uCubeB      = glGetUniformLocation(prog, "uCubeB");
	uCubeL      = glGetUniformLocation(prog, "uCubeL");
	uCF0        = glGetUniformLocation(prog, "uCF0");
	uCF1        = glGetUniformLocation(prog, "uCF1");
	uCF2        = glGetUniformLocation(prog, "uCF2");
	uTorusC     = glGetUniformLocation(prog, "uTorusC");
	uTorusR     = glGetUniformLocation(prog, "uTorusR");
	return 1;
}

// Scene uniforms: same objects as raytrace.cpp. The cube stands on a vertex
// with its space diagonal vertical; its three edge axes are spread 120
// degrees apart around the vertical at elevation 1/sqrt(3).
void SetScene(int aa)
{
	glUniform2f(uResolution, glwWidth(), glwHeight());

	float tx = 0.0;
	float ty = 0.55;
	float tz = -0.15;
	float cp = cos(pitch);
	glUniform3f(uEye, tx + dist * cp * cos(yaw), ty + dist * sin(pitch), tz + dist * cp * sin(yaw));
	glUniform3f(uTarget, tx, ty, tz);
	glUniform1f(uFov, 0.62);
	glUniform1i(uAA, aa);

	glUniform4f(uSphere, -1.70, 1.0, 0.60, 1.0);                 // silver

	float phi = 0.55 + cubeSpin;                                  // bronze
	float ch = sqrt(2.0 / 3.0);
	float cv = 1.0 / sqrt(3.0);
	float step = 2.0943951;                                       // 120 degrees
	glUniform3f(uCubeB, 1.50, 0.0, 0.60);
	glUniform1f(uCubeL, 1.40);
	glUniform3f(uCF0, ch * cos(phi), cv, ch * sin(phi));
	glUniform3f(uCF1, ch * cos(phi + step), cv, ch * sin(phi + step));
	glUniform3f(uCF2, ch * cos(phi + 2.0 * step), cv, ch * sin(phi + 2.0 * step));

	glUniform3f(uTorusC, 0.0, 0.30, -0.90);                       // gold, lying flat
	glUniform2f(uTorusR, 0.78, 0.30);
}

void DrawFullscreenQuad()
{
	glBegin(GL_QUADS);
	glVertex2f(-1.0, -1.0);
	glVertex2f( 1.0, -1.0);
	glVertex2f( 1.0,  1.0);
	glVertex2f(-1.0,  1.0);
	glEnd();
}

int main(int argc, pchar* argv)
{
	int W = 640;
	int H = 480;
	if (argc >= 3)
	{
		SToken tw[16];
		SToken th[16];
		MakeTokens(tw, argv[1]);
		MakeTokens(th, argv[2]);
		if (tw[0].IsNumber()) W = (int)tw[0].GetNumber();
		if (th[0].IsNumber()) H = (int)th[0].GetNumber();
	}
	if (W < 16) W = 16;
	if (H < 16) H = 16;

	if (!glwInit(W, H, "Scinc GLSL raytrace"))
	{
		printf("Cannot open OpenGL window\n");
		return 1;
	}
	char info[256];
	glwGetString(GL_RENDERER, info, 256);
	printf("GL_RENDERER = %s\n", info);
	fflush(stdout);

	if (!LoadShaders())
		return 1;

	int aa = 2;
	int moving = 1;
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
		double t = glwTime();
		float dt = t - tPrev;
		tPrev = t;

		int key = glwGetKey();
		while (key != 0)
		{
			if (key == GLW_KEY_ESC || key == 'q' || key == 'Q')
				running = 0;
			else if (key == ' ')
				moving = 1 - moving;
			else if (key >= '1' && key <= '4')
				aa = key - '0';
			else if (key == 'f' || key == 'F')
			{
				fullscreen = 1 - fullscreen;
				glwSetFullscreen(fullscreen);
			}
			else if (key == 's' || key == 'S')
				saveNext = 1;
			else if (key == 'r' || key == 'R')
			{
				if (LoadShaders())
					printf("shaders reloaded\n");
				fflush(stdout);
			}
			else if (key == 'h' || key == 'H')
			{
				yaw = HOME_YAW;
				pitch = HOME_PITCH;
				dist = HOME_DIST;
				cubeSpin = 0.0;
				moving = 0;
			}
			key = glwGetKey();
		}

		// mouse: drag to orbit, wheel to zoom
		int mx = glwMouseX();
		int my = glwMouseY();
		if (glwMouseButtons() & 1)
		{
			if (dragging)
			{
				yaw = yaw + 0.01 * (mx - lastX);
				pitch = pitch + 0.01 * (my - lastY);
				if (pitch < 0.03) pitch = 0.03;
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
			if (dist < 2.5) dist = 2.5;
			if (dist > 20.0) dist = 20.0;
		}

		if (moving)
		{
			yaw = yaw + 0.15 * dt;           // slow orbit
			cubeSpin = cubeSpin + 0.6 * dt;  // the cube pirouettes on its vertex
		}

		if (glwResized())
			glViewport(0, 0, glwWidth(), glwHeight());

		glUseProgram(prog);
		SetScene(aa);
		DrawFullscreenQuad();

		if (saveNext)
		{
			if (glwSaveBMP("raytrace_gl.bmp"))
				printf("wrote raytrace_gl.bmp\n");
			fflush(stdout);
			saveNext = 0;
		}
		glwSwap();

		frames++;
		if (t - tRate0 >= 1.0)
		{
			char title[96];
			snprintf(title, 96, "Scinc GLSL raytrace - %dx%d AA %dx%d - %.1f FPS",
				glwWidth(), glwHeight(), aa, aa, frames / (t - tRate0));
			glwSetTitle(title);
			tRate0 = t;
			frames = 0;
		}
	}

	glDeleteProgram(prog);
	glwDone();
	return 0;
}
