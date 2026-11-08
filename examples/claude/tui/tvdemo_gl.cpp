// The Turbo Vision style desktop of tvdemo.cpp in an OpenGL window (tvgl.h): every
// glyph of the interpreter's 9x16 font sits in one texture atlas, and the screen is
// one quad drawn by a shader from a small texture of glyph slots and colours.
// The window resizes freely; Ctrl+= and Ctrl+- zoom, Ctrl+0 resets.
//
//   ./build_so.sh                        once, builds the file and clock library
//   (cd ../opengl && ./build_so.sh)      once, builds the OpenGL wrapper
//   Scinc tvdemo_gl.cpp [file]           from this folder
#define TV_GFX
#define TV_GL
#define TV_GL_SCALE 3
#define TV_GL_TITLE "tvdemo (OpenGL)"
#include "tvdemo.cpp"
