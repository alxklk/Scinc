// gears_glsl.cpp - the glxgears scene with per-pixel GLSL lighting.
//
// Same three gears, sizes and motion as gears.cpp, but every pixel is shaded
// by a small inline shader instead of fixed-function lighting:
//   - a warm key light, a cool fill light and a rim light that outlines the
//     gears against the backdrop
//   - coloured metal: highlights and reflections tinted by the gear colour
//     through Schlick Fresnel, so grazing surfaces turn mirror-like
//   - lathe-turned front faces: fine concentric grooves with an anisotropic
//     highlight, the radial light streaks of machined watch gears
//   - reflections of a procedural photo studio (sky gradient, a big softbox
//     and a strip light), so the paint shows moving reflections as it turns
//   - lighting in linear colour, ACES-style tone mapping, gamma correction
//   - a gradient backdrop with vignette (its own tiny shader), 4x MSAA
// The gears are display lists as before; tooth faces carry their own face
// normals (shader varyings are always interpolated, so glShadeModel(GL_FLAT)
// no longer keeps them crisp).
//
//   ./build_so.sh           (once, builds scgl_dl.so)
//   Scinc gears_glsl.cpp
//
// Keys: arrows / mouse drag rotate the view, z/Z roll, wheel zoom,
//       L key light orbit on/off, space pause, H home, F fullscreen,
//       S save gears_glsl.bmp, Esc/Q quit.

#include <stdio.h>
#include "scgl.h"

#define M_PI 3.14159265358979

float view_rotx = 20.0;
float view_roty = 30.0;
float view_rotz = 0.0;
float angle = 0.0;
float zoom = 1.0;

int gear1;
int gear2;
int gear3;

int gearProg = 0;
int backProg = 0;
int uKey;
int uAspect;

// ---- shaders ----

// Positions and normals go to eye space; the object-space position and
// normal are passed too, for the lathe-turned pattern on the flat faces. The
// gear colour comes from glColor (set inside each display list) and is a
// linear-light albedo.
char* gearVS =
	"#version 120\n"
	"varying vec3 vN;\n"
	"varying vec3 vP;\n"
	"varying vec3 vObjP;\n"
	"varying vec3 vObjN;\n"
	"varying vec3 vAlbedo;\n"
	"void main()\n"
	"{\n"
	"	vec4 p = gl_ModelViewMatrix * gl_Vertex;\n"
	"	vP = p.xyz;\n"
	"	vN = gl_NormalMatrix * gl_Normal;\n"
	"	vObjP = gl_Vertex.xyz;\n"
	"	vObjN = gl_Normal;\n"
	"	vAlbedo = gl_Color.rgb;\n"
	"	gl_Position = gl_ProjectionMatrix * p;\n"
	"}\n";

// Coloured metal (anodised look): reflections and highlights are tinted by the
// albedo through a Schlick Fresnel term. The flat front and back faces are
// lathe-turned: fine concentric grooves whose tangent is the circle around the
// axle, lit with an anisotropic (Kajiya-Kay) highlight, which gives the
// radial light streaks of machined watch gears.
char* gearFS =
	"#version 120\n"
	"uniform vec3 uKey;\n"                       // direction to the key light (eye space)
	"varying vec3 vN;\n"
	"varying vec3 vP;\n"
	"varying vec3 vObjP;\n"
	"varying vec3 vObjN;\n"
	"varying vec3 vAlbedo;\n"
	"\n"
	// the photo studio the gears reflect, looked up by direction
	"vec3 studio(vec3 r)\n"
	"{\n"
	"	vec3 c = mix(vec3(0.015), vec3(0.16, 0.17, 0.20), smoothstep(-0.45, 0.25, r.y));\n"
	"	c = mix(c, vec3(0.55, 0.60, 0.70), smoothstep(0.25, 1.0, r.y));\n"
	"	c += vec3(3.2, 3.0, 2.7) * smoothstep(0.80, 0.95, dot(r, normalize(vec3(-0.50, 0.60, 0.62))));\n"  // big softbox
	"	c += vec3(0.9, 1.2, 1.7) * smoothstep(0.92, 0.99, dot(r, normalize(vec3(0.90, 0.20, 0.40))));\n"  // strip light
	"	return c;\n"
	"}\n"
	"\n"
	"float blinn(vec3 N, vec3 H, float n)\n"
	"{\n"
	"	return pow(max(dot(N, H), 0.0), n) * (n + 8.0) / 25.13;\n"
	"}\n"
	"\n"
	"float aniso(vec3 T, vec3 H, float n)\n"      // highlight across the grooves
	"{\n"
	"	float th = dot(T, H);\n"
	"	return pow(sqrt(max(1.0 - th * th, 0.0)), n) * (n + 8.0) / 25.13;\n"
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
	"	if (dot(N, V) < 0.0) N = -N;\n"            // never light a face from behind
	"	float NdV = max(dot(N, V), 1e-3);\n"
	"	vec3 albedo = vAlbedo;\n"
	"	float metal = 0.7;\n"
	"	vec3 F0 = mix(vec3(0.04), albedo * 1.6 + 0.04, metal);\n"
	"	vec3 F = F0 + (1.0 - F0) * pow(1.0 - NdV, 5.0);\n"
	"\n"
	"	vec3 L1 = normalize(uKey);\n"              // warm key
	"	vec3 C1 = vec3(1.00, 0.90, 0.78) * 1.7;\n"
	"	vec3 L2 = normalize(vec3(-0.80, -0.15, 0.45));\n"   // cool fill, low left
	"	vec3 C2 = vec3(0.45, 0.60, 1.00) * 0.6;\n"
	"	vec3 L3 = normalize(vec3(0.30, 0.55, -1.00));\n"    // rim, from behind
	"	vec3 C3 = vec3(1.00, 0.95, 0.90) * 2.0;\n"
	"	vec3 H1 = normalize(L1 + V);\n"
	"	vec3 H2 = normalize(L2 + V);\n"
	"	float d1 = max(dot(N, L1), 0.0);\n"
	"	float d2 = max(dot(N, L2), 0.0);\n"
	"	float d3 = max(dot(N, L3), 0.0);\n"
	"\n"
	// lathe-turned faces: circular groove tangent, fine ring pattern
	"	float turned = smoothstep(0.95, 0.999, abs(vObjN.z));\n"
	"	vec3 T = normalize(gl_NormalMatrix * vec3(-vObjP.y, vObjP.x, 0.0) + 1e-6);\n"
	"	float rad = length(vObjP.xy);\n"
	"	float phase = rad * 120.0;\n"
	"	float fade = clamp(1.5 - fwidth(phase) * 0.5, 0.0, 1.0);\n"   // fade grooves finer than a pixel (no moire)
	"	float rings = 0.5 + 0.5 * sin(phase) * sin(rad * 29.0 + 1.3) * fade;\n"
	"	float s1 = mix(blinn(N, H1, 140.0), aniso(T, H1, 160.0) * (0.7 + 0.6 * rings), turned);\n"
	"	float s2 = mix(blinn(N, H2, 50.0), aniso(T, H2, 60.0) * (0.7 + 0.6 * rings), turned);\n"
	"	vec3 spec = (C1 * d1 * s1 + C2 * d2 * s2) * F;\n"
	"\n"
	"	vec3 env = studio(reflect(-V, N)) * F * mix(1.0, 0.55 + 0.35 * rings, turned);\n"
	"	vec3 hemi = mix(vec3(0.03, 0.025, 0.02), vec3(0.16, 0.18, 0.22), N.y * 0.5 + 0.5);\n"
	"	vec3 diffuse = albedo * (1.0 - metal) * (hemi + C1 * d1 + C2 * d2);\n"
	"	vec3 ambientMetal = albedo * metal * hemi * 1.5;\n"
	"	float rim = pow(1.0 - NdV, 3.0) * d3;\n"   // edge light that outlines the silhouette
	"	vec3 col = diffuse + ambientMetal + spec + env + C3 * rim * mix(albedo, vec3(1.0), 0.4);\n"
	"\n"
	"	col = aces(col);\n"
	"	gl_FragColor = vec4(pow(col, vec3(1.0 / 2.2)), 1.0);\n"
	"}\n";

// Backdrop: drawn first as a fullscreen quad straight in clip space.
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
	"	vec3 top = vec3(0.070, 0.080, 0.105);\n"
	"	vec3 bottom = vec3(0.012, 0.012, 0.016);\n"
	"	vec3 c = mix(bottom, top, smoothstep(-1.0, 1.0, vUV.y));\n"
	"	c += vec3(0.10, 0.09, 0.08) * exp(-dot(p - vec2(0.0, 0.1), p - vec2(0.0, 0.1)) * 1.6);\n"  // glow behind the gears
	"	c *= 1.0 - 0.35 * dot(vUV, vUV) * 0.5;\n"   // vignette
	"	gl_FragColor = vec4(pow(c, vec3(1.0 / 2.2)), 1.0);\n"
	"}\n";

// ---- geometry ----

// One side face of a tooth: the edge p0 -> p1 (at radii ra, rb and angles
// aa, ab) swept through the gear's width, with its own face normal.
void flank(float ra, float aa, float rb, float ab, float hw)
{
	float x0 = ra * cos(aa);
	float y0 = ra * sin(aa);
	float x1 = rb * cos(ab);
	float y1 = rb * sin(ab);
	float u = x1 - x0;
	float v = y1 - y0;
	float len = sqrt(u * u + v * v);
	glNormal3f(v / len, -u / len, 0.0);
	glVertex3f(x0, y0, hw);
	glVertex3f(x0, y0, -hw);
	glVertex3f(x1, y1, -hw);
	glVertex3f(x1, y1, hw);
}

// Draw a gear wheel: inner radius (hole), outer radius (tooth middle),
// width, number of teeth and tooth depth - the glxgears gear.
void gear(float inner_radius, float outer_radius, float width, int teeth, float tooth_depth)
{
	float r0 = inner_radius;
	float r1 = outer_radius - tooth_depth / 2.0;
	float r2 = outer_radius + tooth_depth / 2.0;
	float da = 2.0 * M_PI / teeth / 4.0;
	float hw = width * 0.5;
	float a;

	// front face
	glNormal3f(0.0, 0.0, 1.0);
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

	// back face
	glNormal3f(0.0, 0.0, -1.0);
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

	// outward faces of the teeth, each quad with its own face normal:
	// rising flank, tooth top, falling flank, root land
	glBegin(GL_QUADS);
	for (int i = 0; i < teeth; i++)
	{
		a = i * 2.0 * M_PI / teeth;
		flank(r1, a, r2, a + da, hw);
		flank(r2, a + da, r2, a + 2.0 * da, hw);
		flank(r2, a + 2.0 * da, r1, a + 3.0 * da, hw);
		flank(r1, a + 3.0 * da, r1, a + 4.0 * da, hw);
	}
	glEnd();

	// inside radius cylinder, smooth
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

int MakeGear(float r, float g, float b, float inner, float outer, float width, int teeth, float depth)
{
	int list = glGenLists(1);
	glNewList(list, GL_COMPILE);
	glColor3f(r, g, b);                 // linear-light albedo, read by the shader
	gear(inner, outer, width, teeth, depth);
	glEndList();
	return list;
}

void reshape(int width, int height)
{
	float h = (1.0 * height) / width;
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-1.0 / zoom, 1.0 / zoom, -h / zoom, h / zoom, 5.0, 60.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0, 0.0, -40.0);
}

void drawBackdrop()
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
	glDepthMask(1);
	glEnable(GL_DEPTH_TEST);
}

void draw(float keyAngle)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	drawBackdrop();

	glUseProgram(gearProg);
	// key light: upper left, in front; with L on it swings slowly around
	glUniform3f(uKey, -0.55 + 0.9 * sin(keyAngle), 0.75 + 0.2 * cos(keyAngle * 0.7), 0.65);

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
	glUseProgram(0);
}

int main()
{
	glwMultisample(4);
	if (!glwInit(900, 720, "Scinc GLSL gears"))
	{
		printf("Cannot open OpenGL window\n");
		return 1;
	}
	gearProg = glwCreateProgram(gearVS, gearFS);
	backProg = glwCreateProgram(backVS, backFS);
	if (gearProg == 0 || backProg == 0)
		return 1;
	uKey = glGetUniformLocation(gearProg, "uKey");
	uAspect = glGetUniformLocation(backProg, "uAspect");

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);

	// the glxgears gears, in deeper linear-light colours
	gear1 = MakeGear(0.55, 0.035, 0.02, 1.0, 4.0, 1.0, 20, 0.7);   // red
	gear2 = MakeGear(0.025, 0.36, 0.09, 0.5, 2.0, 2.0, 10, 0.7);   // green
	gear3 = MakeGear(0.03, 0.08, 0.55, 1.3, 2.0, 0.5, 10, 0.7);    // blue

	int paused = 0;
	int orbitLight = 1;
	int fullscreen = 0;
	int saveNext = 0;
	int dragging = 0;
	int lastX = 0;
	int lastY = 0;
	int running = 1;
	int viewDirty = 1;
	float keyAngle = 0.0;
	int frames = 0;
	double tPrev = glwTime();
	double tRate0 = tPrev;

	while (running && glwPoll())
	{
		double t = glwTime();
		float dt = t - tPrev;
		tPrev = t;
		if (!paused)
		{
			angle = angle + 70.0 * dt;        // degrees per second, as glxgears
			if (angle > 3600.0)
				angle = angle - 3600.0;
			if (orbitLight)
				keyAngle = keyAngle + 0.35 * dt;
		}

		int key = glwGetKey();
		while (key != 0)
		{
			if (key == GLW_KEY_ESC || key == 'q' || key == 'Q')
				running = 0;
			else if (key == 'z')
				view_rotz = view_rotz + 5.0;
			else if (key == 'Z')
				view_rotz = view_rotz - 5.0;
			else if (key == ' ')
				paused = 1 - paused;
			else if (key == 'l' || key == 'L')
				orbitLight = 1 - orbitLight;
			else if (key == 'h' || key == 'H')
			{
				view_rotx = 20.0;
				view_roty = 30.0;
				view_rotz = 0.0;
				zoom = 1.0;
				keyAngle = 0.0;
				viewDirty = 1;
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
		if (glwKeyDown(GLW_KEY_UP))
			view_rotx = view_rotx + 90.0 * dt;
		if (glwKeyDown(GLW_KEY_DOWN))
			view_rotx = view_rotx - 90.0 * dt;
		if (glwKeyDown(GLW_KEY_LEFT))
			view_roty = view_roty + 90.0 * dt;
		if (glwKeyDown(GLW_KEY_RIGHT))
			view_roty = view_roty - 90.0 * dt;

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
		int wheel = glwMouseWheel();
		if (wheel != 0)
		{
			zoom = zoom * pow(1.1, wheel);
			if (zoom < 0.5) zoom = 0.5;
			if (zoom > 4.0) zoom = 4.0;
			viewDirty = 1;
		}

		if (glwResized())
			viewDirty = 1;
		if (viewDirty)
		{
			reshape(glwWidth(), glwHeight());
			viewDirty = 0;
		}

		draw(keyAngle);

		if (saveNext)
		{
			if (glwSaveBMP("gears_glsl.bmp"))
				printf("wrote gears_glsl.bmp\n");
			fflush(stdout);
			saveNext = 0;
		}
		glwSwap();

		frames++;
		if (t - tRate0 >= 1.0)
		{
			char title[80];
			snprintf(title, 80, "Scinc GLSL gears - %.1f FPS", frames / (t - tRate0));
			glwSetTitle(title);
			tRate0 = t;
			frames = 0;
		}
	}

	glDeleteProgram(gearProg);
	glDeleteProgram(backProg);
	glwDone();
	return 0;
}
