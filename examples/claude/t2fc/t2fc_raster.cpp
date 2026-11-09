// t2fc in a graphic window, with text in the interpreter's 9x16 bitmap font.
//
//   (cd ../tui && ./build_so.sh)        once, builds the file and clock library
//   Scinc t2fc_raster.cpp [left [right]]
#define TV_GFX
#define TV_GFX_RASTER
#define G_SCREEN_SCALE 3
#include "t2fc.cpp"
