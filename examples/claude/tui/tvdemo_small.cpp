// The Turbo Vision style desktop of tvdemo.cpp in a graphic window, with small text
// in the interpreter's 6x12 bitmap font (150x42 cells in the same window as
// tvdemo_raster.cpp, which uses the 9x16 font).
//
//   ./build_so.sh                    once, builds the file and clock library
//   Scinc tvdemo_small.cpp [file]    from this folder
#define TV_GFX
#define TV_GFX_SMALL
#define G_SCREEN_SCALE 2
#include "tvdemo.cpp"
