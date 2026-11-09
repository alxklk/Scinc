// t2fc in an OpenGL window (../tui/tvgl.h) with the 9x16 font from a glyph atlas.
// The window resizes freely; Ctrl+= and Ctrl+- zoom, Ctrl+0 resets.
//
//   (cd ../tui && ./build_so.sh)           once, builds the file and clock library
//   (cd ../opengl && ./build_so.sh)        once, builds the OpenGL wrapper
//   Scinc t2fc_gl.cpp [left [right]]
#define TV_GFX
#define TV_GL
#define TV_GL_SCALE 3
#define TV_GL_TITLE "t2fc (OpenGL)"
#include "t2fc.cpp"
