// scgl_dl.cpp - native side of scgl.h: an SDL2 window with an OpenGL 1.x
// context, plus the exported wrappers generated from the WRAP list in scgl.h.
// Build with ./build_so.sh  ->  scgl_dl.so

#include "../../../include/ScincVM.h"
#include "../../../include/ScincBinder.h"

#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif

#include <SDL.h>
#include <SDL_opengl.h>

#include <cmath>
#include <cstdio>
#include <cstring>

namespace
{
SDL_Window* glw_window = nullptr;
SDL_GLContext glw_context = nullptr;
bool glw_quit = false;
bool glw_size_changed = false;
int glw_samples = 0;
Uint64 glw_t0 = 0;

int glw_mouse_x = 0;
int glw_mouse_y = 0;
int glw_mouse_buttons = 0;
int glw_mouse_wheel = 0;

const int KEYQ = 64;
int glw_keys[KEYQ];
int glw_key_head = 0;
int glw_key_tail = 0;

// SDL keycode <-> scgl key code (see GLW_KEY_* in scgl.h)
struct KeyMap { SDL_Keycode sdl; int glw; };
const KeyMap keyMap[] = {
	{SDLK_LEFT, 1001}, {SDLK_RIGHT, 1002}, {SDLK_UP, 1003}, {SDLK_DOWN, 1004},
	{SDLK_PAGEUP, 1005}, {SDLK_PAGEDOWN, 1006}, {SDLK_HOME, 1007}, {SDLK_END, 1008},
	{SDLK_F1, 1101}, {SDLK_F2, 1102}, {SDLK_F3, 1103}, {SDLK_F4, 1104},
	{SDLK_F5, 1105}, {SDLK_F6, 1106}, {SDLK_F7, 1107}, {SDLK_F8, 1108},
	{SDLK_F9, 1109}, {SDLK_F10, 1110}, {SDLK_F11, 1111}, {SDLK_F12, 1112},
};

int ToGlwKey(SDL_Keycode k, Uint16 mod)
{
	for(const KeyMap& m : keyMap)
		if(m.sdl == k)
			return m.glw;
	if(k >= 'a' && k <= 'z' && (mod & (KMOD_SHIFT | KMOD_CAPS)))
		return k - 'a' + 'A';
	return k < 128 ? int(k) : 0;
}

SDL_Keycode ToSdlKey(int key)
{
	for(const KeyMap& m : keyMap)
		if(m.glw == key)
			return m.sdl;
	if(key >= 'A' && key <= 'Z')
		return key - 'A' + 'a';
	return key;
}

void PushKey(int key)
{
	int next = (glw_key_tail + 1) % KEYQ;
	if(key == 0 || next == glw_key_head)
		return;
	glw_keys[glw_key_tail] = key;
	glw_key_tail = next;
}

// ---- GL 2.0 shader entry points, resolved at runtime after context creation
PFNGLCREATESHADERPROC        p_glCreateShader;
PFNGLSHADERSOURCEPROC        p_glShaderSource;
PFNGLCOMPILESHADERPROC       p_glCompileShader;
PFNGLGETSHADERIVPROC         p_glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC    p_glGetShaderInfoLog;
PFNGLDELETESHADERPROC        p_glDeleteShader;
PFNGLCREATEPROGRAMPROC       p_glCreateProgram;
PFNGLATTACHSHADERPROC        p_glAttachShader;
PFNGLLINKPROGRAMPROC         p_glLinkProgram;
PFNGLGETPROGRAMIVPROC        p_glGetProgramiv;
PFNGLGETPROGRAMINFOLOGPROC   p_glGetProgramInfoLog;
PFNGLDELETEPROGRAMPROC       p_glDeleteProgram;
PFNGLUSEPROGRAMPROC          p_glUseProgram;
PFNGLGETUNIFORMLOCATIONPROC  p_glGetUniformLocation;
PFNGLUNIFORM1IPROC           p_glUniform1i;
PFNGLUNIFORM1FPROC           p_glUniform1f;
PFNGLUNIFORM2FPROC           p_glUniform2f;
PFNGLUNIFORM3FPROC           p_glUniform3f;
PFNGLUNIFORM4FPROC           p_glUniform4f;
PFNGLUNIFORM1FVPROC          p_glUniform1fv;
PFNGLUNIFORM3FVPROC          p_glUniform3fv;
PFNGLUNIFORM4FVPROC          p_glUniform4fv;
PFNGLUNIFORMMATRIX4FVPROC    p_glUniformMatrix4fv;
PFNGLBLENDEQUATIONPROC       p_glBlendEquation;
bool glw_shaders_ok = false;

template<typename T> bool LoadProc(T& fn, const char* name)
{
	fn = (T)SDL_GL_GetProcAddress(name);
	return fn != nullptr;
}

bool LoadShaderProcs()
{
	bool ok = true;
	ok &= LoadProc(p_glCreateShader, "glCreateShader");
	ok &= LoadProc(p_glShaderSource, "glShaderSource");
	ok &= LoadProc(p_glCompileShader, "glCompileShader");
	ok &= LoadProc(p_glGetShaderiv, "glGetShaderiv");
	ok &= LoadProc(p_glGetShaderInfoLog, "glGetShaderInfoLog");
	ok &= LoadProc(p_glDeleteShader, "glDeleteShader");
	ok &= LoadProc(p_glCreateProgram, "glCreateProgram");
	ok &= LoadProc(p_glAttachShader, "glAttachShader");
	ok &= LoadProc(p_glLinkProgram, "glLinkProgram");
	ok &= LoadProc(p_glGetProgramiv, "glGetProgramiv");
	ok &= LoadProc(p_glGetProgramInfoLog, "glGetProgramInfoLog");
	ok &= LoadProc(p_glDeleteProgram, "glDeleteProgram");
	ok &= LoadProc(p_glUseProgram, "glUseProgram");
	ok &= LoadProc(p_glGetUniformLocation, "glGetUniformLocation");
	ok &= LoadProc(p_glUniform1i, "glUniform1i");
	ok &= LoadProc(p_glUniform1f, "glUniform1f");
	ok &= LoadProc(p_glUniform2f, "glUniform2f");
	ok &= LoadProc(p_glUniform3f, "glUniform3f");
	ok &= LoadProc(p_glUniform4f, "glUniform4f");
	ok &= LoadProc(p_glUniform1fv, "glUniform1fv");
	ok &= LoadProc(p_glUniform3fv, "glUniform3fv");
	ok &= LoadProc(p_glUniform4fv, "glUniform4fv");
	ok &= LoadProc(p_glUniformMatrix4fv, "glUniformMatrix4fv");
	ok &= LoadProc(p_glBlendEquation, "glBlendEquation");
	return ok;
}

int glw_init(int w, int h, const char* title)
{
	if(glw_window)
		return 1;
	if(SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		printf("glwInit: SDL_Init failed: %s\n", SDL_GetError());
		return 0;
	}
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	const Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
	if(glw_samples > 1)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, glw_samples);
	}
	glw_window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, flags);
	if(!glw_window && glw_samples > 1)
	{
		printf("glwInit: %d-sample anti-aliasing unavailable, continuing without\n", glw_samples);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
		glw_window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, flags);
	}
	if(!glw_window)
	{
		printf("glwInit: cannot create window: %s\n", SDL_GetError());
		return 0;
	}
	glw_context = SDL_GL_CreateContext(glw_window);
	if(!glw_context)
	{
		printf("glwInit: cannot create GL context: %s\n", SDL_GetError());
		SDL_DestroyWindow(glw_window);
		glw_window = nullptr;
		return 0;
	}
	SDL_GL_MakeCurrent(glw_window, glw_context);
	SDL_GL_SetSwapInterval(1);
	glw_shaders_ok = LoadShaderProcs();
	if(!glw_shaders_ok)
		printf("glwInit: OpenGL 2.0 shader functions are not available\n");
	glw_quit = false;
	glw_size_changed = true;
	glw_t0 = SDL_GetPerformanceCounter();
	return 1;
}

int glw_poll()
{
	SDL_Event e;
	while(SDL_PollEvent(&e))
	{
		switch(e.type)
		{
		case SDL_QUIT:
			glw_quit = true;
			break;
		case SDL_WINDOWEVENT:
			if(e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
				glw_size_changed = true;
			else if(e.window.event == SDL_WINDOWEVENT_CLOSE)
				glw_quit = true;
			break;
		case SDL_KEYDOWN:
			PushKey(ToGlwKey(e.key.keysym.sym, e.key.keysym.mod));
			break;
		case SDL_MOUSEWHEEL:
			glw_mouse_wheel += e.wheel.y;
			break;
		}
	}
	Uint32 b = SDL_GetMouseState(&glw_mouse_x, &glw_mouse_y);
	glw_mouse_buttons = ((b & SDL_BUTTON_LMASK) ? 1 : 0) | ((b & SDL_BUTTON_MMASK) ? 2 : 0) | ((b & SDL_BUTTON_RMASK) ? 4 : 0);
	return glw_quit ? 0 : 1;
}

void glw_swap()
{
	if(glw_window)
		SDL_GL_SwapWindow(glw_window);
}

void glw_done()
{
	if(glw_context)
		SDL_GL_DeleteContext(glw_context);
	if(glw_window)
		SDL_DestroyWindow(glw_window);
	glw_context = nullptr;
	glw_window = nullptr;
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

double glw_time()
{
	return double(SDL_GetPerformanceCounter() - glw_t0) / double(SDL_GetPerformanceFrequency());
}

void glw_set_title(const char* title)
{
	if(glw_window)
		SDL_SetWindowTitle(glw_window, title);
}

void glw_swap_interval(int n)
{
	SDL_GL_SetSwapInterval(n);
}

int glw_width()
{
	int w = 0, h = 0;
	if(glw_window)
		SDL_GL_GetDrawableSize(glw_window, &w, &h);
	return w;
}

int glw_height()
{
	int w = 0, h = 0;
	if(glw_window)
		SDL_GL_GetDrawableSize(glw_window, &w, &h);
	return h;
}

int glw_resized()
{
	int r = glw_size_changed ? 1 : 0;
	glw_size_changed = false;
	return r;
}

int glw_get_key()
{
	if(glw_key_head == glw_key_tail)
		return 0;
	int k = glw_keys[glw_key_head];
	glw_key_head = (glw_key_head + 1) % KEYQ;
	return k;
}

int glw_key_down(int key)
{
	const Uint8* state = SDL_GetKeyboardState(nullptr);
	SDL_Scancode sc = SDL_GetScancodeFromKey(ToSdlKey(key));
	return (sc != SDL_SCANCODE_UNKNOWN && state[sc]) ? 1 : 0;
}

int glw_get_string(int name, char* buf, int len)
{
	const char* s = (const char*)glGetString(name);
	if(!s || len <= 0)
		return 0;
	int n = (int)strlen(s);
	if(n > len - 1)
		n = len - 1;
	memcpy(buf, s, n);
	buf[n] = 0;
	return n;
}

// gluPerspective replacement: symmetric frustum from a vertical FOV in degrees.
void glw_perspective(double fovy, double aspect, double zNear, double zFar)
{
	double ymax = zNear * tan(fovy * M_PI / 360.0);
	double xmax = ymax * aspect;
	glFrustum(-xmax, xmax, -ymax, ymax, zNear, zFar);
}

// gluLookAt replacement: rotate so the eye looks down -Z at the center with
// 'up' along +Y, then move the eye to the origin.
void glw_look_at(double ex, double ey, double ez, double cx, double cy, double cz,
	double ux, double uy, double uz)
{
	double f[3] = {cx - ex, cy - ey, cz - ez};
	double fl = sqrt(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
	if(fl == 0.0)
		return;
	for(double& c : f)
		c /= fl;

	// s = f x up (normalized), u = s x f
	double s[3] = {f[1] * uz - f[2] * uy, f[2] * ux - f[0] * uz, f[0] * uy - f[1] * ux};
	double sl = sqrt(s[0] * s[0] + s[1] * s[1] + s[2] * s[2]);
	if(sl == 0.0)
		return;
	for(double& c : s)
		c /= sl;
	double u[3] = {s[1] * f[2] - s[2] * f[1], s[2] * f[0] - s[0] * f[2], s[0] * f[1] - s[1] * f[0]};

	// column-major: rows of the rotation are s, u, -f
	double m[16] = {
		s[0], u[0], -f[0], 0.0,
		s[1], u[1], -f[1], 0.0,
		s[2], u[2], -f[2], 0.0,
		0.0,  0.0,  0.0,   1.0};
	glMultMatrixd(m);
	glTranslated(-ex, -ey, -ez);
}

// Scinc floats arrive as doubles; GL's *fv calls want GLfloat[].
const int FV_MAX = 4096;
const GLfloat* glw_fvn(const double* v, int n)
{
	static GLfloat f[FV_MAX];
	if(n > FV_MAX)
		n = FV_MAX;
	for(int i = 0; i < n; ++i)
		f[i] = (GLfloat)v[i];
	return f;
}

const GLfloat* glw_fv4(const double* v)
{
	return glw_fvn(v, 4);
}

// Clamp a uniform array count so glw_fvn never truncates it.
int glw_fv_count(int count, int comps)
{
	return count * comps > FV_MAX ? FV_MAX / comps : count;
}

// ---- shaders

GLuint CompileShader(GLenum type, const char* src, const char* what)
{
	GLuint sh = p_glCreateShader(type);
	p_glShaderSource(sh, 1, &src, nullptr);
	p_glCompileShader(sh);
	GLint ok = 0;
	p_glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
	if(!ok)
	{
		char log[4096];
		p_glGetShaderInfoLog(sh, sizeof(log), nullptr, log);
		printf("%s shader (%s) failed to compile:\n%s\n",
			type == GL_VERTEX_SHADER ? "vertex" : "fragment", what, log);
		p_glDeleteShader(sh);
		return 0;
	}
	return sh;
}

// Returns a linked program id, or 0 (with the compile/link log printed).
int glw_create_program(const char* vs, const char* fs, const char* vsName, const char* fsName)
{
	if(!glw_shaders_ok)
	{
		printf("glwCreateProgram: shaders are not supported by this GL context\n");
		return 0;
	}
	GLuint v = CompileShader(GL_VERTEX_SHADER, vs, vsName);
	if(!v)
		return 0;
	GLuint f = CompileShader(GL_FRAGMENT_SHADER, fs, fsName);
	if(!f)
	{
		p_glDeleteShader(v);
		return 0;
	}
	GLuint prog = p_glCreateProgram();
	p_glAttachShader(prog, v);
	p_glAttachShader(prog, f);
	p_glLinkProgram(prog);
	p_glDeleteShader(v);  // flagged; freed with the program
	p_glDeleteShader(f);
	GLint ok = 0;
	p_glGetProgramiv(prog, GL_LINK_STATUS, &ok);
	if(!ok)
	{
		char log[4096];
		p_glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
		printf("shader program (%s + %s) failed to link:\n%s\n", vsName, fsName, log);
		p_glDeleteProgram(prog);
		return 0;
	}
	return (int)prog;
}

char* ReadFile(const char* path)
{
	FILE* f = fopen(path, "rb");
	if(!f)
	{
		printf("cannot open shader file '%s'\n", path);
		return nullptr;
	}
	fseek(f, 0, SEEK_END);
	long n = ftell(f);
	fseek(f, 0, SEEK_SET);
	char* s = new char[n + 1];
	size_t got = fread(s, 1, n, f);
	s[got] = 0;
	fclose(f);
	return s;
}

int glw_load_program(const char* vsPath, const char* fsPath)
{
	char* vs = ReadFile(vsPath);
	char* fs = ReadFile(fsPath);
	int prog = (vs && fs) ? glw_create_program(vs, fs, vsPath, fsPath) : 0;
	delete[] vs;
	delete[] fs;
	return prog;
}

void glw_set_fullscreen(int on)
{
	if(glw_window)
		SDL_SetWindowFullscreen(glw_window, on ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

// Saves the back buffer (call before glwSwap) as a 24-bit BMP.
int glw_save_bmp(const char* path)
{
	int w = glw_width();
	int h = glw_height();
	if(w <= 0 || h <= 0)
		return 0;
	SDL_Surface* s = SDL_CreateRGBSurfaceWithFormat(0, w, h, 24, SDL_PIXELFORMAT_RGB24);
	if(!s)
		return 0;
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	unsigned char* tmp = new unsigned char[w * h * 3];
	glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, tmp);
	for(int y = 0; y < h; ++y)  // GL rows are bottom-up
		memcpy((unsigned char*)s->pixels + y * s->pitch, tmp + (h - 1 - y) * w * 3, w * 3);
	delete[] tmp;
	int ok = SDL_SaveBMP(s, path) == 0 ? 1 : 0;
	SDL_FreeSurface(s);
	return ok;
}
}

#include "scgl.h"
