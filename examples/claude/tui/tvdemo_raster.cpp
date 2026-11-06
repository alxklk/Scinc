// The Turbo Vision style desktop of tvdemo.cpp in a graphic window, with text drawn
// by the interpreter's bitmap font (stexts), which is much quicker than the stroke
// font of tvdemo_gfx.cpp.
//
//   ./build_so.sh                     once, builds the file and clock library
//   Scinc tvdemo_raster.cpp [file]    from this folder
#define TV_GFX
#define TV_GFX_RASTER
#define G_SCREEN_SCALE 3
#include "tvdemo.cpp"
