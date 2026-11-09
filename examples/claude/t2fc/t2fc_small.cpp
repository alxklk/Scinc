// t2fc in a graphic window, with small text in the interpreter's 6x12 bitmap font
// (150x42 cells in the window of t2fc_raster.cpp).
//
//   (cd ../tui && ./build_so.sh)       once, builds the file and clock library
//   Scinc t2fc_small.cpp [left [right]]
#define TV_GFX
#define TV_GFX_SMALL
#define G_SCREEN_SCALE 2
#include "t2fc.cpp"
