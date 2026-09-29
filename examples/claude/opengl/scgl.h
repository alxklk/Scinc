// scgl.h - OpenGL (fixed-function, GL 1.x) + window wrapper for Scinc.
//
// One list of WRAP(...) entries serves both sides:
//  - included from a Scinc program (__SCINC__ defined) every entry becomes a
//    [[scinc::dynlink]] declaration resolved from scgl_dl.so;
//  - included from scgl_dl.cpp (native build) every entry becomes an exported
//    "<name>_wrapper" that unpacks the Scinc arguments via ScincBoundCall.
//
// Type mapping: Scinc 'float' is a native double, so float arrays are passed
// as FLOATV (Scinc float* == native double*) and converted to GLfloat[] here.
// Pixel buffers are int* of 0xAARRGGBB (same layout as Graph::BitBlt).
//
// Build the library first: ./build_so.sh

#ifndef SCGL_H
#define SCGL_H

#ifdef __SCINC__

typedef float* FLOATV;
#define NAME(name) #name"_wrapper"
#define WRAP(rettype, name, args, ...) [[scinc::dynlink("scgl_dl", NAME(name) )]]rettype name args;

// ---- key codes returned by glwGetKey()/accepted by glwKeyDown() ----
// Printable keys are their ASCII code ('a', '1', ' ' ...).
#define GLW_KEY_ESC       27
#define GLW_KEY_ENTER     13
#define GLW_KEY_TAB        9
#define GLW_KEY_BACKSPACE  8
#define GLW_KEY_LEFT    1001
#define GLW_KEY_RIGHT   1002
#define GLW_KEY_UP      1003
#define GLW_KEY_DOWN    1004
#define GLW_KEY_PGUP    1005
#define GLW_KEY_PGDN    1006
#define GLW_KEY_HOME    1007
#define GLW_KEY_END     1008
#define GLW_KEY_F1      1101

// ---- OpenGL constants (values from GL/gl.h) ----
#define GL_FALSE                      0
#define GL_TRUE                       1
#define GL_NO_ERROR                   0
#define GL_ZERO                       0
#define GL_ONE                        1
// primitives
#define GL_POINTS                0x0000
#define GL_LINES                 0x0001
#define GL_LINE_LOOP             0x0002
#define GL_LINE_STRIP            0x0003
#define GL_TRIANGLES             0x0004
#define GL_TRIANGLE_STRIP        0x0005
#define GL_TRIANGLE_FAN          0x0006
#define GL_QUADS                 0x0007
#define GL_QUAD_STRIP            0x0008
#define GL_POLYGON               0x0009
// clear bits
#define GL_DEPTH_BUFFER_BIT      0x0100
#define GL_STENCIL_BUFFER_BIT    0x0400
#define GL_COLOR_BUFFER_BIT      0x4000
// capabilities
#define GL_LINE_SMOOTH           0x0B20
#define GL_CULL_FACE             0x0B44
#define GL_LIGHTING              0x0B50
#define GL_LIGHT_MODEL_TWO_SIDE  0x0B52
#define GL_LIGHT_MODEL_AMBIENT   0x0B53
#define GL_COLOR_MATERIAL        0x0B57
#define GL_FOG                   0x0B60
#define GL_DEPTH_TEST            0x0B71
#define GL_NORMALIZE             0x0BA1
#define GL_BLEND                 0x0BE2
#define GL_TEXTURE_2D            0x0DE1
#define GL_MULTISAMPLE           0x809D
#define GL_POLYGON_OFFSET_LINE   0x2A02
#define GL_POLYGON_OFFSET_FILL   0x8037
// lights & materials
#define GL_LIGHT0                0x4000
#define GL_LIGHT1                0x4001
#define GL_LIGHT2                0x4002
#define GL_LIGHT3                0x4003
#define GL_AMBIENT               0x1200
#define GL_DIFFUSE               0x1201
#define GL_SPECULAR              0x1202
#define GL_POSITION              0x1203
#define GL_SPOT_DIRECTION        0x1204
#define GL_SPOT_EXPONENT         0x1205
#define GL_SPOT_CUTOFF           0x1206
#define GL_CONSTANT_ATTENUATION  0x1207
#define GL_LINEAR_ATTENUATION    0x1208
#define GL_QUADRATIC_ATTENUATION 0x1209
#define GL_EMISSION              0x1600
#define GL_SHININESS             0x1601
#define GL_AMBIENT_AND_DIFFUSE   0x1602
// faces, shading, polygon mode
#define GL_FRONT                 0x0404
#define GL_BACK                  0x0405
#define GL_FRONT_AND_BACK        0x0408
#define GL_CW                    0x0900
#define GL_CCW                   0x0901
#define GL_FLAT                  0x1D00
#define GL_SMOOTH                0x1D01
#define GL_POINT                 0x1B00
#define GL_LINE                  0x1B01
#define GL_FILL                  0x1B02
// depth / blend functions
#define GL_NEVER                 0x0200
#define GL_LESS                  0x0201
#define GL_EQUAL                 0x0202
#define GL_LEQUAL                0x0203
#define GL_GREATER               0x0204
#define GL_ALWAYS                0x0207
#define GL_SRC_COLOR             0x0300
#define GL_ONE_MINUS_SRC_COLOR   0x0301
#define GL_SRC_ALPHA             0x0302
#define GL_ONE_MINUS_SRC_ALPHA   0x0303
#define GL_FUNC_ADD              0x8006
#define GL_FUNC_SUBTRACT         0x800A
#define GL_FUNC_REVERSE_SUBTRACT 0x800B
// fog
#define GL_EXP                   0x0800
#define GL_EXP2                  0x0801
#define GL_FOG_DENSITY           0x0B62
#define GL_FOG_START             0x0B63
#define GL_FOG_END               0x0B64
#define GL_FOG_MODE              0x0B65
#define GL_FOG_COLOR             0x0B66
// hints
#define GL_PERSPECTIVE_CORRECTION_HINT 0x0C50
#define GL_FOG_HINT              0x0C54
#define GL_DONT_CARE             0x1100
#define GL_FASTEST               0x1101
#define GL_NICEST                0x1102
// display lists, matrices
#define GL_COMPILE               0x1300
#define GL_COMPILE_AND_EXECUTE   0x1301
#define GL_MODELVIEW             0x1700
#define GL_PROJECTION            0x1701
#define GL_TEXTURE               0x1702
// strings
#define GL_VENDOR                0x1F00
#define GL_RENDERER              0x1F01
#define GL_VERSION               0x1F02
#define GL_EXTENSIONS            0x1F03
// textures
#define GL_MODULATE              0x2100
#define GL_DECAL                 0x2101
#define GL_REPLACE               0x1E01
#define GL_TEXTURE_ENV_MODE      0x2200
#define GL_TEXTURE_ENV           0x2300
#define GL_NEAREST               0x2600
#define GL_LINEAR                0x2601
#define GL_TEXTURE_MAG_FILTER    0x2800
#define GL_TEXTURE_MIN_FILTER    0x2801
#define GL_TEXTURE_WRAP_S        0x2802
#define GL_TEXTURE_WRAP_T        0x2803
#define GL_REPEAT                0x2901
#define GL_CLAMP_TO_EDGE         0x812F
#define GL_RGB                   0x1907
#define GL_RGBA                  0x1908
#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#define GL_LINEAR_MIPMAP_NEAREST 0x2701
#define GL_NEAREST_MIPMAP_LINEAR 0x2702
#define GL_LINEAR_MIPMAP_LINEAR  0x2703
#define GL_GENERATE_MIPMAP       0x8191   // glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, 1) before glwTexImage2D
#define GL_TEXTURE_MAX_ANISOTROPY 0x84FE  // sharper textures at grazing angles (EXT/GL 4.6)

#else  // native side (compiled into scgl_dl.so by scgl_dl.cpp)

typedef const double* FLOATV;
#define WRAP(rettype, name, args, ...) extern "C" EXPORT int name##_wrapper(ScincVM* s, void*){ScincBoundCall(s,+[]args ->rettype __VA_ARGS__);return 0;}

#endif

// ============================ window / input ============================
// glwMultisample(n) before glwInit asks for n-sample anti-aliasing (falls
// back to none if unsupported).
WRAP(void,   glwMultisample, (int samples), {glw_samples = samples;})
// glwInit opens a resizable double-buffered window with a GL 1.x
// (compatibility) context and depth buffer; returns 1 on success.
WRAP(int,    glwInit,        (int w, int h, char* title), {return glw_init(w, h, title);})
// glwPoll pumps window events; returns 0 once the window was closed.
WRAP(int,    glwPoll,        (), {return glw_poll();})
WRAP(void,   glwSwap,        (), {glw_swap();})
WRAP(void,   glwDone,        (), {glw_done();})
WRAP(double, glwTime,        (), {return glw_time();})
WRAP(void,   glwSetTitle,    (char* title), {glw_set_title(title);})
WRAP(void,   glwSwapInterval,(int n), {glw_swap_interval(n);})
// Drawable size in pixels; glwResized() returns 1 once after every resize
// (and once after glwInit), so call reshape code when it fires.
WRAP(int,    glwWidth,       (), {return glw_width();})
WRAP(int,    glwHeight,      (), {return glw_height();})
WRAP(int,    glwResized,     (), {return glw_resized();})
// Keyboard: glwGetKey() pops the next pressed key (0 when the queue is
// empty); glwKeyDown(key) tells whether a key is currently held.
WRAP(int,    glwGetKey,      (), {return glw_get_key();})
WRAP(int,    glwKeyDown,     (int key), {return glw_key_down(key);})
// Mouse: position in window pixels, button mask (1 left, 2 middle, 4 right),
// and accumulated wheel steps since the previous glwMouseWheel() call.
WRAP(int,    glwMouseX,      (), {return glw_mouse_x;})
WRAP(int,    glwMouseY,      (), {return glw_mouse_y;})
WRAP(int,    glwMouseButtons,(), {return glw_mouse_buttons;})
WRAP(int,    glwMouseWheel,  (), {int w = glw_mouse_wheel; glw_mouse_wheel = 0; return w;})
WRAP(void,   glwSetFullscreen,(int on), {glw_set_fullscreen(on);})
// Saves the current back buffer (call before glwSwap) as a .bmp file.
WRAP(int,    glwSaveBMP,     (char* path), {return glw_save_bmp(path);})

// ============================ OpenGL 1.x ================================
// state
WRAP(void, glEnable,      (int cap), {glEnable(cap);})
WRAP(void, glDisable,     (int cap), {glDisable(cap);})
WRAP(void, glClear,       (int mask), {glClear(mask);})
WRAP(void, glClearColor,  (float r, float g, float b, float a), {glClearColor(r, g, b, a);})
WRAP(void, glClearDepth,  (double d), {glClearDepth(d);})
WRAP(void, glViewport,    (int x, int y, int w, int h), {glViewport(x, y, w, h);})
WRAP(void, glShadeModel,  (int mode), {glShadeModel(mode);})
WRAP(void, glCullFace,    (int mode), {glCullFace(mode);})
WRAP(void, glFrontFace,   (int mode), {glFrontFace(mode);})
WRAP(void, glPolygonMode, (int face, int mode), {glPolygonMode(face, mode);})
WRAP(void, glDepthFunc,   (int func), {glDepthFunc(func);})
WRAP(void, glDepthMask,   (int flag), {glDepthMask(flag);})
WRAP(void, glBlendFunc,   (int sf, int df), {glBlendFunc(sf, df);})
WRAP(void, glBlendEquation,(int mode), {p_glBlendEquation(mode);})
WRAP(void, glHint,        (int target, int mode), {glHint(target, mode);})
WRAP(void, glPolygonOffset, (float factor, float units), {glPolygonOffset(factor, units);})
WRAP(void, glLineWidth,   (float w), {glLineWidth(w);})
WRAP(void, glPointSize,   (float s), {glPointSize(s);})
WRAP(void, glFlush,       (), {glFlush();})
WRAP(void, glFinish,      (), {glFinish();})
WRAP(int,  glGetError,    (), {return glGetError();})
// Copies glGetString(name) into buf (at most len bytes incl. the zero).
WRAP(int,  glwGetString,  (int name, char* buf, int len), {return glw_get_string(name, buf, len);})

// matrices
WRAP(void, glMatrixMode,  (int mode), {glMatrixMode(mode);})
WRAP(void, glLoadIdentity,(), {glLoadIdentity();})
WRAP(void, glPushMatrix,  (), {glPushMatrix();})
WRAP(void, glPopMatrix,   (), {glPopMatrix();})
WRAP(void, glTranslatef,  (float x, float y, float z), {glTranslated(x, y, z);})
WRAP(void, glRotatef,     (float a, float x, float y, float z), {glRotated(a, x, y, z);})
WRAP(void, glScalef,      (float x, float y, float z), {glScaled(x, y, z);})
WRAP(void, glFrustum,     (double l, double r, double b, double t, double n, double f), {glFrustum(l, r, b, t, n, f);})
WRAP(void, glOrtho,       (double l, double r, double b, double t, double n, double f), {glOrtho(l, r, b, t, n, f);})
WRAP(void, gluPerspective,(double fovy, double aspect, double n, double f), {glw_perspective(fovy, aspect, n, f);})
WRAP(void, gluLookAt,     (double ex, double ey, double ez, double cx, double cy, double cz, double ux, double uy, double uz), {glw_look_at(ex, ey, ez, cx, cy, cz, ux, uy, uz);})
// m = 16 floats, column-major as in OpenGL
WRAP(void, glLoadMatrixf, (FLOATV m), {glLoadMatrixd(m);})
WRAP(void, glMultMatrixf, (FLOATV m), {glMultMatrixd(m);})

// immediate mode
WRAP(void, glBegin,       (int mode), {glBegin(mode);})
WRAP(void, glEnd,         (), {glEnd();})
WRAP(void, glVertex2f,    (float x, float y), {glVertex2d(x, y);})
WRAP(void, glVertex3f,    (float x, float y, float z), {glVertex3d(x, y, z);})
WRAP(void, glNormal3f,    (float x, float y, float z), {glNormal3d(x, y, z);})
WRAP(void, glColor3f,     (float r, float g, float b), {glColor3d(r, g, b);})
WRAP(void, glColor4f,     (float r, float g, float b, float a), {glColor4d(r, g, b, a);})
WRAP(void, glTexCoord2f,  (float s, float t), {glTexCoord2d(s, t);})

// lighting & materials (v = up to 4 floats)
WRAP(void, glLightf,      (int light, int pname, float v), {glLightf(light, pname, v);})
WRAP(void, glLightfv,     (int light, int pname, FLOATV v), {glLightfv(light, pname, glw_fv4(v));})
WRAP(void, glMaterialf,   (int face, int pname, float v), {glMaterialf(face, pname, v);})
WRAP(void, glMaterialfv,  (int face, int pname, FLOATV v), {glMaterialfv(face, pname, glw_fv4(v));})
WRAP(void, glLightModelfv,(int pname, FLOATV v), {glLightModelfv(pname, glw_fv4(v));})
WRAP(void, glLightModeli, (int pname, int v), {glLightModeli(pname, v);})
WRAP(void, glColorMaterial,(int face, int mode), {glColorMaterial(face, mode);})
WRAP(void, glFogf,        (int pname, float v), {glFogf(pname, v);})
WRAP(void, glFogi,        (int pname, int v), {glFogi(pname, v);})
WRAP(void, glFogfv,       (int pname, FLOATV v), {glFogfv(pname, glw_fv4(v));})

// display lists
WRAP(int,  glGenLists,    (int range), {return glGenLists(range);})
WRAP(void, glNewList,     (int list, int mode), {glNewList(list, mode);})
WRAP(void, glEndList,     (), {glEndList();})
WRAP(void, glCallList,    (int list), {glCallList(list);})
WRAP(void, glDeleteLists, (int list, int range), {glDeleteLists(list, range);})

// vertex arrays: one draw call for a whole mesh. The arrays are Scinc
// float arrays (native doubles, read by GL in place as GL_DOUBLE), size =
// components per vertex. GL keeps only the pointer, so set the arrays right
// before drawing each frame; glwDisableArrays() turns them all off again.
WRAP(void, glwVertexArray,   (int size, FLOATV v), {glEnableClientState(GL_VERTEX_ARRAY); glVertexPointer(size, GL_DOUBLE, 0, v);})
WRAP(void, glwNormalArray,   (FLOATV v), {glEnableClientState(GL_NORMAL_ARRAY); glNormalPointer(GL_DOUBLE, 0, v);})
WRAP(void, glwColorArray,    (int size, FLOATV v), {glEnableClientState(GL_COLOR_ARRAY); glColorPointer(size, GL_DOUBLE, 0, v);})
WRAP(void, glwTexCoordArray, (int size, FLOATV v), {glEnableClientState(GL_TEXTURE_COORD_ARRAY); glTexCoordPointer(size, GL_DOUBLE, 0, v);})
WRAP(void, glwDisableArrays, (), {glDisableClientState(GL_VERTEX_ARRAY); glDisableClientState(GL_NORMAL_ARRAY); glDisableClientState(GL_COLOR_ARRAY); glDisableClientState(GL_TEXTURE_COORD_ARRAY);})
WRAP(void, glDrawArrays,     (int mode, int first, int count), {glDrawArrays(mode, first, count);})
// indices: count ints (vertex numbers), e.g. 3 per triangle for GL_TRIANGLES
WRAP(void, glDrawElements,   (int mode, int count, int* indices), {glDrawElements(mode, count, GL_UNSIGNED_INT, indices);})

// textures (pixels = w*h ints of 0xAARRGGBB, row 0 = first row uploaded)
WRAP(int,  glwGenTexture, (), {GLuint t = 0; glGenTextures(1, &t); return (int)t;})
WRAP(void, glwDeleteTexture,(int tex), {GLuint t = tex; glDeleteTextures(1, &t);})
WRAP(void, glBindTexture, (int target, int tex), {glBindTexture(target, tex);})
WRAP(void, glTexParameteri,(int target, int pname, int v), {glTexParameteri(target, pname, v);})
WRAP(void, glTexEnvi,     (int target, int pname, int v), {glTexEnvi(target, pname, v);})
// copy a rectangle of the framebuffer into the bound texture: allocate with
// glCopyTexImage2D, then refresh with glCopyTexSubImage2D (frame feedback)
WRAP(void, glCopyTexImage2D, (int target, int level, int internalFormat, int x, int y, int w, int h, int border), {glCopyTexImage2D(target, level, internalFormat, x, y, w, h, border);})
WRAP(void, glCopyTexSubImage2D, (int target, int level, int xoff, int yoff, int x, int y, int w, int h), {glCopyTexSubImage2D(target, level, xoff, yoff, x, y, w, h);})
WRAP(void, glwTexImage2D, (int w, int h, int* pixels), {glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, pixels);})

// ============================ shaders (GLSL, OpenGL 2.0+) ===============
// Build a program from GLSL source strings or from two files; returns the
// program id, or 0 after printing the compile/link log.
WRAP(int,  glwCreateProgram,   (char* vsSource, char* fsSource), {return glw_create_program(vsSource, fsSource, "vs", "fs");})
WRAP(int,  glwLoadProgram,     (char* vsPath, char* fsPath), {return glw_load_program(vsPath, fsPath);})
WRAP(void, glUseProgram,       (int prog), {p_glUseProgram(prog);})
WRAP(void, glDeleteProgram,    (int prog), {p_glDeleteProgram(prog);})
WRAP(int,  glGetUniformLocation,(int prog, char* name), {return p_glGetUniformLocation(prog, name);})
WRAP(void, glUniform1i,        (int loc, int v), {p_glUniform1i(loc, v);})
WRAP(void, glUniform1f,        (int loc, float x), {p_glUniform1f(loc, x);})
WRAP(void, glUniform2f,        (int loc, float x, float y), {p_glUniform2f(loc, x, y);})
WRAP(void, glUniform3f,        (int loc, float x, float y, float z), {p_glUniform3f(loc, x, y, z);})
WRAP(void, glUniform4f,        (int loc, float x, float y, float z, float w), {p_glUniform4f(loc, x, y, z, w);})
// arrays: v holds count elements of 1/3/4 floats; matrix = 16 floats column-major
WRAP(void, glUniform1fv,       (int loc, int count, FLOATV v), {count = glw_fv_count(count, 1); p_glUniform1fv(loc, count, glw_fvn(v, count));})
WRAP(void, glUniform3fv,       (int loc, int count, FLOATV v), {count = glw_fv_count(count, 3); p_glUniform3fv(loc, count, glw_fvn(v, count * 3));})
WRAP(void, glUniform4fv,       (int loc, int count, FLOATV v), {count = glw_fv_count(count, 4); p_glUniform4fv(loc, count, glw_fvn(v, count * 4));})
WRAP(void, glUniformMatrix4fv, (int loc, FLOATV m), {p_glUniformMatrix4fv(loc, 1, GL_FALSE, glw_fvn(m, 16));})

#endif // SCGL_H
