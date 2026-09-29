/**
 * @file lanczos_zoom.cpp
 * @brief Image upscaling filters compared side by side.
 *
 *  Left  : a live 64x64 source bitmap, rendered here with a hand written
 *          analytic-coverage antialiaser (discs, rings and thick/thin lines
 *          drifting on Lissajous curves or bouncing off the borders).
 *  Right : the very same bitmap magnified 8x (512x512).
 *
 *  Five buttons pick the reconstruction filter.  All five run through the
 *  identical separable two-pass resampler and differ only in their weight
 *  table, so what you see really is the kernel and nothing else:
 *
 *      nearest    1 tap    blocky, no interpolation at all
 *      bilinear   2 taps   smooth but soft, and visibly faceted
 *      a = 2      4 taps   Lanczos, soft
 *      a = 4      8 taps   Lanczos, sharp
 *      a = 6     12 taps   Lanczos, sharpest, and it rings - look for the
 *                          dark/bright haloes beside the green line and the
 *                          thin magenta ring
 *
 *  The measured filter time next to the buttons shows what the taps cost.
 *
 *  Keys:  N B 2 4 6  select filter     SPACE  pause      Q or ESC  quit
 *
 *  Run:   Scinc lanczos_zoom.cpp
 */

#define G_SCREEN_WIDTH  736
#define G_SCREEN_HEIGHT 580
#define G_SCREEN_SCALE  2
#define G_SCREEN_MODE   2

#include "graphics.h"

#define NO_GREEDY_EVENT   /* keep Present() from eating our queued events */

#include "../../ws.h"
#include "../../graphic/font.h"
#include "../../graphic/letters.h"
#include "../../graphic/gtext.h"

/* ---------- geometry ------------------------------------------------- */

#define SRCW  64
#define SRCH  64
#define ZOOM  8
#define DSTW  512           /* SRCW * ZOOM */
#define DSTH  512           /* SRCH * ZOOM */

#define SRCSTRIDE 192       /* SRCW * 3 floats per row */
#define DSTSTRIDE 1536      /* DSTW * 3 floats per row */

#define TAPS  12            /* widest kernel (Lanczos a=6), the table stride */

/* ---------- layout --------------------------------------------------- */

#define PANX  24            /* 1:1 source panel */
#define PANY  74
#define BIGX  200           /* magnified panel */
#define BIGY  44

#define NBTN  5             /* filter buttons */
#define BTNX  24
#define BTNY  176
#define BTNW  124
#define BTNH  30
#define BTNSTEP 38

#define PBY   470           /* pause button, same x/w as the filter buttons */

#define PI 3.14159265358979

/* ---------- palette -------------------------------------------------- */

#define COL_BG    0xff11151c
#define COL_FRAME 0xff39485c
#define COL_TEXT  0xffc8d4e2
#define COL_DIM   0xff6f8096
#define COL_HI    0xff7fd6ff
#define COL_WARN  0xffffc060

Graph g;
CFont* pfont;

float* src;                 /* SRCW*SRCH*3, channel range 0..255 */
float* tmp;                 /* DSTW*SRCH*3, horizontal pass output */
int*   dst;                 /* DSTW*DSTH packed 0xAARRGGBB */
int*   spv;                 /* SRCW*SRCH packed, the 1:1 preview */

float* colW;                /* DSTW*TAPS horizontal weights */
int*   colI;                /* DSTW*TAPS source x, pre-scaled by 3 */
float* rowW;                /* DSTH*TAPS vertical weights */
int*   rowI;                /* DSTH*TAPS source y, pre-scaled by DSTSTRIDE */

int gMode;                  /* 0 = nearest, 1 = bilinear, 2 = Lanczos */
int gRadius;                /* Lanczos radius a, when gMode == 2 */
int gTaps;                  /* 1, 2, 4, 8 or 12 */

/* ---------- small helpers -------------------------------------------- */

float Fl(float x)
{
    int i = int(x);
    float f = i;
    if (x < f)
        i = i - 1;
    return i;
}

/* period 1 ping-pong ramp: exact reflection off the borders, and being a
   closed form of time it stays correct however uneven the frame rate is. */
float Tri(float x)
{
    float f = x - Fl(x);
    if (f < 0.5)
        return f*2;
    return 2 - f*2;
}

int TextW(char* s, float sc)
{
    int n = 0;
    while (s[n] != 0)
        n++;
    return int(n*6*sc);
}

void Text(char* s, float x, float y, float sc, int col)
{
    g.clear();
    DrawText(g, *pfont, s, 0, x, y, sc, sc);
    g.fin();
    g.rgba32(col);
    g.width(sc, sc);
    g.stroke();
}

void Frame(int x, int y, int w, int h, int col)
{
    g.fillrect(x-2, y-2, w+4, 2, col);
    g.fillrect(x-2, y+h, w+4, 2, col);
    g.fillrect(x-2, y,   2,   h, col);
    g.fillrect(x+w, y,   2,   h, col);
}

/* ---------- source bitmap: analytic coverage antialiasing ------------- */

void Blend(int x, int y, float cov, float cr, float cg, float cb)
{
    if (cov <= 0)
        return;
    if (cov > 1)
        cov = 1;
    int o = (y*SRCW + x)*3;
    float k = 1.0 - cov;
    src[o]   = src[o]  *k + cr*cov;
    src[o+1] = src[o+1]*k + cg*cov;
    src[o+2] = src[o+2]*k + cb*cov;
}

/* rin = 0 gives a filled disc, rin > 0 an annulus */
void Disc(float cx, float cy, float rin, float rout, float cr, float cg, float cb)
{
    int x0 = int(cx - rout - 1);
    int x1 = int(cx + rout + 2);
    int y0 = int(cy - rout - 1);
    int y1 = int(cy + rout + 2);
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > SRCW) x1 = SRCW;
    if (y1 > SRCH) y1 = SRCH;

    for (int y = y0; y < y1; y++)
    {
        float dy = y + 0.5 - cy;
        for (int x = x0; x < x1; x++)
        {
            float dx = x + 0.5 - cx;
            float d = sqrt(dx*dx + dy*dy);
            float cov = rout + 0.5 - d;      /* outer edge ramp */
            if (rin > 0)
            {
                float ci = d - (rin - 0.5);  /* inner edge ramp */
                if (ci < cov)
                    cov = ci;
            }
            Blend(x, y, cov, cr, cg, cb);
        }
    }
}

/* hw is the half width, so hw = 0.4 draws a 0.8 pixel wide line */
void Line(float ax, float ay, float bx, float by, float hw,
          float cr, float cg, float cb)
{
    float ex = bx - ax;
    float ey = by - ay;
    float el = ex*ex + ey*ey;
    if (el < 0.000001)
        el = 0.000001;

    float lo = ax;
    float hi = bx;
    if (hi < lo) { lo = bx; hi = ax; }
    int x0 = int(lo - hw - 1);
    int x1 = int(hi + hw + 2);
    lo = ay;
    hi = by;
    if (hi < lo) { lo = by; hi = ay; }
    int y0 = int(lo - hw - 1);
    int y1 = int(hi + hw + 2);
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > SRCW) x1 = SRCW;
    if (y1 > SRCH) y1 = SRCH;

    for (int y = y0; y < y1; y++)
    {
        float py = y + 0.5 - ay;
        for (int x = x0; x < x1; x++)
        {
            float px = x + 0.5 - ax;
            float t = (px*ex + py*ey)/el;    /* projection onto the segment */
            if (t < 0) t = 0;
            if (t > 1) t = 1;
            float qx = px - t*ex;
            float qy = py - t*ey;
            float d = sqrt(qx*qx + qy*qy);
            Blend(x, y, hw + 0.5 - d, cr, cg, cb);
        }
    }
}

/* ---------- the animated scene --------------------------------------- */

#define NSHAPE 7

struct Shape
{
    int kind;        /* 0 = disc or ring, 1 = line */
    int mot;         /* 0 = bounce off the borders, 1 = Lissajous */
    float ox;        /* motion origin ... */
    float oy;
    float ax;        /* ... and extent */
    float ay;
    float fx;        /* motion frequency, deliberately incommensurate */
    float fy;
    float px;        /* phase */
    float py;
    float rin;       /* ring inner radius, 0 for a filled disc */
    float rout;
    float len;       /* line half length */
    float hw;        /* line half width */
    float aspd;      /* line spin rate */
    float aph;
    float cr;
    float cg;
    float cb;
};

Shape shape[NSHAPE];

void SetShape(int i, int kind, int mot,
              float ox, float oy, float ax, float ay,
              float fx, float fy, float px, float py,
              float cr, float cg, float cb)
{
    shape[i].kind = kind;
    shape[i].mot  = mot;
    shape[i].ox = ox;
    shape[i].oy = oy;
    shape[i].ax = ax;
    shape[i].ay = ay;
    shape[i].fx = fx;
    shape[i].fy = fy;
    shape[i].px = px;
    shape[i].py = py;
    shape[i].rin  = 0;
    shape[i].rout = 0;
    shape[i].len  = 0;
    shape[i].hw   = 0;
    shape[i].aspd = 0;
    shape[i].aph  = 0;
    shape[i].cr = cr;
    shape[i].cg = cg;
    shape[i].cb = cb;
}

void InitShapes()
{
    /* A spread of feature sizes, because that is what a reconstruction
       filter is judged on: fat edges survive anything, hairlines do not. */

    /* 0: big filled disc, drifting on a Lissajous curve */
    SetShape(0, 0, 1, 32, 32, 20, 19, 0.31, 0.23, 0.0, 1.7, 255, 148, 46);
    shape[0].rout = 8;

    /* 1: fat ring bouncing corner to corner */
    SetShape(1, 0, 0, 12, 12, 40, 40, 0.19, 0.27, 0.35, 0.8, 62, 214, 232);
    shape[1].rin  = 7.5;
    shape[1].rout = 10;

    /* 2: hairline ring, 1 px thick, the worst case for ringing */
    SetShape(2, 0, 1, 32, 32, 22, 17, 0.43, 0.53, 2.1, 0.4, 240, 92, 200);
    shape[2].rin  = 5;
    shape[2].rout = 6;

    /* 3: small fast bouncing dot */
    SetShape(3, 0, 0, 5, 5, 54, 54, 0.71, 0.57, 0.13, 0.62, 255, 242, 158);
    shape[3].rout = 3;

    /* 4: thick 2.4 px line, slow spin */
    SetShape(4, 1, 0, 17, 17, 30, 30, 0.17, 0.13, 0.9, 0.2, 116, 232, 126);
    shape[4].len  = 16;
    shape[4].hw   = 1.2;
    shape[4].aspd = 0.37;

    /* 5: 1.4 px line */
    SetShape(5, 1, 1, 32, 32, 11, 12, 0.29, 0.37, 1.2, 2.6, 236, 240, 255);
    shape[5].len  = 20;
    shape[5].hw   = 0.7;
    shape[5].aspd = -0.61;

    /* 6: sub-pixel 0.8 px line */
    SetShape(6, 1, 1, 32, 32, 15, 12, 0.47, 0.19, 3.0, 1.1, 150, 190, 255);
    shape[6].len  = 26;
    shape[6].hw   = 0.4;
    shape[6].aspd = 0.23;
}

void BuildSource(float T)
{
    /* background: a faint vertical gradient, dark enough that the negative
       lobes of the kernel show up as visible haloes */
    int o = 0;
    for (int y = 0; y < SRCH; y++)
    {
        float f = y/63.0;
        float br = 13 + 9*f;
        float bg = 17 + 13*f;
        float bb = 25 + 21*f;
        for (int x = 0; x < SRCW; x++)
        {
            src[o]   = br;
            src[o+1] = bg;
            src[o+2] = bb;
            o = o + 3;
        }
    }

    for (int i = 0; i < NSHAPE; i++)
    {
        float cx;
        float cy;
        if (shape[i].mot == 0)
        {
            cx = shape[i].ox + shape[i].ax*Tri(T*shape[i].fx + shape[i].px);
            cy = shape[i].oy + shape[i].ay*Tri(T*shape[i].fy + shape[i].py);
        }
        else
        {
            cx = shape[i].ox + shape[i].ax*sin(T*shape[i].fx + shape[i].px);
            cy = shape[i].oy + shape[i].ay*sin(T*shape[i].fy + shape[i].py);
        }

        if (shape[i].kind == 0)
        {
            Disc(cx, cy, shape[i].rin, shape[i].rout,
                 shape[i].cr, shape[i].cg, shape[i].cb);
        }
        else
        {
            float a = shape[i].aph + T*shape[i].aspd;
            float dx = cos(a)*shape[i].len;
            float dy = sin(a)*shape[i].len;
            Line(cx-dx, cy-dy, cx+dx, cy+dy, shape[i].hw,
                 shape[i].cr, shape[i].cg, shape[i].cb);
        }
    }
}

void PackSource()
{
    int o = 0;
    for (int i = 0; i < SRCW*SRCH; i++)
    {
        int r = int(src[o]   + 0.5);
        int q = int(src[o+1] + 0.5);
        int b = int(src[o+2] + 0.5);
        if (r > 255) r = 255;
        if (q > 255) q = 255;
        if (b > 255) b = 255;
        spv[i] = 0xff000000 | (r<<16) | (q<<8) | b;
        o = o + 3;
    }
}

/* ---------- weight tables --------------------------------------------
   Nearest, bilinear and Lanczos are all just weight tables for the same
   separable resampler, which is the point of the demo: the two passes
   below never learn which filter they are running. */

float Lanczos(float x, int a)
{
    float af = a;
    if (x < 0)
        x = -x;
    if (x < 0.000001)
        return 1.0;
    if (x > af - 0.000001)
        return 0.0;
    float px = PI*x;
    return af*sin(px)*sin(px/af)/(px*px);
}

/* One axis.  The map is dst -> src with pixel centres at half integers, so
   src = (d + 0.5)/ZOOM - 0.5.  Weights are normalised (a truncated Lanczos
   does not sum to 1 by itself) and source indices are clamped at the
   borders and pre-multiplied by the buffer stride. */
void BuildAxis(float* W, int* I, int n, int srcN, int mode, int a, int stride)
{
    int nt = 1;
    if (mode == 1)
        nt = 2;
    else if (mode == 2)
        nt = 2*a;

    for (int d = 0; d < n; d++)
    {
        float s = (d + 0.5)/ZOOM - 0.5;
        int o = d*TAPS;
        int i0;

        if (mode == 0)
        {
            i0 = int(Fl(s + 0.5));           /* round to the closest sample */
            W[o] = 1.0;
        }
        else if (mode == 1)
        {
            i0 = int(Fl(s));
            float f = s - i0;
            W[o]   = 1.0 - f;
            W[o+1] = f;
        }
        else
        {
            i0 = int(Fl(s)) - a + 1;         /* 2a taps centred on s */
            for (int k = 0; k < nt; k++)
                W[o+k] = Lanczos(s - (i0 + k), a);
        }

        float sum = 0;
        for (int k = 0; k < nt; k++)
            sum = sum + W[o+k];
        for (int k = 0; k < nt; k++)
        {
            W[o+k] = W[o+k]/sum;
            int i = i0 + k;
            if (i < 0)
                i = 0;
            if (i > srcN-1)
                i = srcN-1;
            I[o+k] = i*stride;
        }
        for (int k = nt; k < TAPS; k++)
        {
            W[o+k] = 0;
            I[o+k] = 0;
        }
    }
}

void SetFilter(int mode, int a)
{
    gMode = mode;
    gRadius = a;
    if (mode == 0)
        gTaps = 1;
    else if (mode == 1)
        gTaps = 2;
    else
        gTaps = 2*a;
    BuildAxis(colW, colI, DSTW, SRCW, mode, a, 3);
    BuildAxis(rowW, rowI, DSTH, SRCH, mode, a, DSTSTRIDE);
}

/* ---------- the two filter passes ------------------------------------
   Both are unrolled per tap count: with a tap loop the loop overhead alone
   costs more than the multiply-adds it carries.  Weights and source
   offsets are hoisted out of the pixel loop, which is why the column loop
   is the outer one here.  The nearest-neighbour path keeps its (always
   1.0) weight multiply so that every filter runs identical code. */

void HPass()
{
    if (gTaps == 1)
    {
        for (int d = 0; d < DSTW; d++)
        {
            int o = d*TAPS;
            int c0 = colI[o];
            float w0 = colW[o];
            int rb = 0;
            int tb = d*3;
            for (int y = 0; y < SRCH; y++)
            {
                int a0 = rb+c0;
                tmp[tb]   = w0*src[a0];
                tmp[tb+1] = w0*src[a0+1];
                tmp[tb+2] = w0*src[a0+2];
                rb = rb + SRCSTRIDE;
                tb = tb + DSTSTRIDE;
            }
        }
    }
    else if (gTaps == 2)
    {
        for (int d = 0; d < DSTW; d++)
        {
            int o = d*TAPS;
            int c0 = colI[o];
            int c1 = colI[o+1];
            float w0 = colW[o];
            float w1 = colW[o+1];
            int rb = 0;
            int tb = d*3;
            for (int y = 0; y < SRCH; y++)
            {
                int a0 = rb+c0;
                int a1 = rb+c1;
                tmp[tb]   = w0*src[a0]   + w1*src[a1];
                tmp[tb+1] = w0*src[a0+1] + w1*src[a1+1];
                tmp[tb+2] = w0*src[a0+2] + w1*src[a1+2];
                rb = rb + SRCSTRIDE;
                tb = tb + DSTSTRIDE;
            }
        }
    }
    else if (gTaps == 4)
    {
        for (int d = 0; d < DSTW; d++)
        {
            int o = d*TAPS;
            int c0 = colI[o];
            int c1 = colI[o+1];
            int c2 = colI[o+2];
            int c3 = colI[o+3];
            float w0 = colW[o];
            float w1 = colW[o+1];
            float w2 = colW[o+2];
            float w3 = colW[o+3];
            int rb = 0;
            int tb = d*3;
            for (int y = 0; y < SRCH; y++)
            {
                int a0 = rb+c0;
                int a1 = rb+c1;
                int a2 = rb+c2;
                int a3 = rb+c3;
                tmp[tb]   = w0*src[a0]   + w1*src[a1]   + w2*src[a2]   + w3*src[a3];
                tmp[tb+1] = w0*src[a0+1] + w1*src[a1+1] + w2*src[a2+1] + w3*src[a3+1];
                tmp[tb+2] = w0*src[a0+2] + w1*src[a1+2] + w2*src[a2+2] + w3*src[a3+2];
                rb = rb + SRCSTRIDE;
                tb = tb + DSTSTRIDE;
            }
        }
    }
    else if (gTaps == 8)
    {
        for (int d = 0; d < DSTW; d++)
        {
            int o = d*TAPS;
            int c0 = colI[o];
            int c1 = colI[o+1];
            int c2 = colI[o+2];
            int c3 = colI[o+3];
            int c4 = colI[o+4];
            int c5 = colI[o+5];
            int c6 = colI[o+6];
            int c7 = colI[o+7];
            float w0 = colW[o];
            float w1 = colW[o+1];
            float w2 = colW[o+2];
            float w3 = colW[o+3];
            float w4 = colW[o+4];
            float w5 = colW[o+5];
            float w6 = colW[o+6];
            float w7 = colW[o+7];
            int rb = 0;
            int tb = d*3;
            for (int y = 0; y < SRCH; y++)
            {
                int a0 = rb+c0;
                int a1 = rb+c1;
                int a2 = rb+c2;
                int a3 = rb+c3;
                int a4 = rb+c4;
                int a5 = rb+c5;
                int a6 = rb+c6;
                int a7 = rb+c7;
                tmp[tb]   = w0*src[a0]   + w1*src[a1]   + w2*src[a2]   + w3*src[a3]
                          + w4*src[a4]   + w5*src[a5]   + w6*src[a6]   + w7*src[a7];
                tmp[tb+1] = w0*src[a0+1] + w1*src[a1+1] + w2*src[a2+1] + w3*src[a3+1]
                          + w4*src[a4+1] + w5*src[a5+1] + w6*src[a6+1] + w7*src[a7+1];
                tmp[tb+2] = w0*src[a0+2] + w1*src[a1+2] + w2*src[a2+2] + w3*src[a3+2]
                          + w4*src[a4+2] + w5*src[a5+2] + w6*src[a6+2] + w7*src[a7+2];
                rb = rb + SRCSTRIDE;
                tb = tb + DSTSTRIDE;
            }
        }
    }
    else
    {
        for (int d = 0; d < DSTW; d++)
        {
            int o = d*TAPS;
            int c0 = colI[o];
            int c1 = colI[o+1];
            int c2 = colI[o+2];
            int c3 = colI[o+3];
            int c4 = colI[o+4];
            int c5 = colI[o+5];
            int c6 = colI[o+6];
            int c7 = colI[o+7];
            int c8 = colI[o+8];
            int c9 = colI[o+9];
            int c10 = colI[o+10];
            int c11 = colI[o+11];
            float w0 = colW[o];
            float w1 = colW[o+1];
            float w2 = colW[o+2];
            float w3 = colW[o+3];
            float w4 = colW[o+4];
            float w5 = colW[o+5];
            float w6 = colW[o+6];
            float w7 = colW[o+7];
            float w8 = colW[o+8];
            float w9 = colW[o+9];
            float w10 = colW[o+10];
            float w11 = colW[o+11];
            int rb = 0;
            int tb = d*3;
            for (int y = 0; y < SRCH; y++)
            {
                int a0 = rb+c0;
                int a1 = rb+c1;
                int a2 = rb+c2;
                int a3 = rb+c3;
                int a4 = rb+c4;
                int a5 = rb+c5;
                int a6 = rb+c6;
                int a7 = rb+c7;
                int a8 = rb+c8;
                int a9 = rb+c9;
                int a10 = rb+c10;
                int a11 = rb+c11;
                tmp[tb]   = w0*src[a0]   + w1*src[a1]   + w2*src[a2]   + w3*src[a3]
                          + w4*src[a4]   + w5*src[a5]   + w6*src[a6]   + w7*src[a7]
                          + w8*src[a8]   + w9*src[a9]   + w10*src[a10] + w11*src[a11];
                tmp[tb+1] = w0*src[a0+1] + w1*src[a1+1] + w2*src[a2+1] + w3*src[a3+1]
                          + w4*src[a4+1] + w5*src[a5+1] + w6*src[a6+1] + w7*src[a7+1]
                          + w8*src[a8+1] + w9*src[a9+1] + w10*src[a10+1] + w11*src[a11+1];
                tmp[tb+2] = w0*src[a0+2] + w1*src[a1+2] + w2*src[a2+2] + w3*src[a3+2]
                          + w4*src[a4+2] + w5*src[a5+2] + w6*src[a6+2] + w7*src[a7+2]
                          + w8*src[a8+2] + w9*src[a9+2] + w10*src[a10+2] + w11*src[a11+2];
                rb = rb + SRCSTRIDE;
                tb = tb + DSTSTRIDE;
            }
        }
    }
}

void VPass()
{
    if (gTaps == 1)
    {
        for (int dy = 0; dy < DSTH; dy++)
        {
            int o = dy*TAPS;
            int r0 = rowI[o];
            float w0 = rowW[o];
            int ob = dy*DSTW;
            int b = 0;
            for (int dx = 0; dx < DSTW; dx++)
            {
                float R = w0*tmp[r0+b];
                int q = b+1;
                float G = w0*tmp[r0+q];
                q = b+2;
                float B = w0*tmp[r0+q];
                int ir = int(R + 0.5);
                int ig = int(G + 0.5);
                int ib = int(B + 0.5);
                if (ir < 0) ir = 0;
                if (ig < 0) ig = 0;
                if (ib < 0) ib = 0;
                if (ir > 255) ir = 255;
                if (ig > 255) ig = 255;
                if (ib > 255) ib = 255;
                dst[ob+dx] = 0xff000000 | (ir<<16) | (ig<<8) | ib;
                b = b + 3;
            }
            if ((dy & 63) == 0)
                Poll();
        }
    }
    else if (gTaps == 2)
    {
        for (int dy = 0; dy < DSTH; dy++)
        {
            int o = dy*TAPS;
            int r0 = rowI[o];
            int r1 = rowI[o+1];
            float w0 = rowW[o];
            float w1 = rowW[o+1];
            int ob = dy*DSTW;
            int b = 0;
            for (int dx = 0; dx < DSTW; dx++)
            {
                float R = w0*tmp[r0+b] + w1*tmp[r1+b];
                int q = b+1;
                float G = w0*tmp[r0+q] + w1*tmp[r1+q];
                q = b+2;
                float B = w0*tmp[r0+q] + w1*tmp[r1+q];
                int ir = int(R + 0.5);
                int ig = int(G + 0.5);
                int ib = int(B + 0.5);
                if (ir < 0) ir = 0;
                if (ig < 0) ig = 0;
                if (ib < 0) ib = 0;
                if (ir > 255) ir = 255;
                if (ig > 255) ig = 255;
                if (ib > 255) ib = 255;
                dst[ob+dx] = 0xff000000 | (ir<<16) | (ig<<8) | ib;
                b = b + 3;
            }
            if ((dy & 63) == 0)
                Poll();
        }
    }
    else if (gTaps == 4)
    {
        for (int dy = 0; dy < DSTH; dy++)
        {
            int o = dy*TAPS;
            int r0 = rowI[o];
            int r1 = rowI[o+1];
            int r2 = rowI[o+2];
            int r3 = rowI[o+3];
            float w0 = rowW[o];
            float w1 = rowW[o+1];
            float w2 = rowW[o+2];
            float w3 = rowW[o+3];
            int ob = dy*DSTW;
            int b = 0;
            for (int dx = 0; dx < DSTW; dx++)
            {
                float R = w0*tmp[r0+b] + w1*tmp[r1+b] + w2*tmp[r2+b] + w3*tmp[r3+b];
                int q = b+1;
                float G = w0*tmp[r0+q] + w1*tmp[r1+q] + w2*tmp[r2+q] + w3*tmp[r3+q];
                q = b+2;
                float B = w0*tmp[r0+q] + w1*tmp[r1+q] + w2*tmp[r2+q] + w3*tmp[r3+q];
                int ir = int(R + 0.5);
                int ig = int(G + 0.5);
                int ib = int(B + 0.5);
                if (ir < 0) ir = 0;
                if (ig < 0) ig = 0;
                if (ib < 0) ib = 0;
                if (ir > 255) ir = 255;
                if (ig > 255) ig = 255;
                if (ib > 255) ib = 255;
                dst[ob+dx] = 0xff000000 | (ir<<16) | (ig<<8) | ib;
                b = b + 3;
            }
            if ((dy & 63) == 0)
                Poll();
        }
    }
    else if (gTaps == 8)
    {
        for (int dy = 0; dy < DSTH; dy++)
        {
            int o = dy*TAPS;
            int r0 = rowI[o];
            int r1 = rowI[o+1];
            int r2 = rowI[o+2];
            int r3 = rowI[o+3];
            int r4 = rowI[o+4];
            int r5 = rowI[o+5];
            int r6 = rowI[o+6];
            int r7 = rowI[o+7];
            float w0 = rowW[o];
            float w1 = rowW[o+1];
            float w2 = rowW[o+2];
            float w3 = rowW[o+3];
            float w4 = rowW[o+4];
            float w5 = rowW[o+5];
            float w6 = rowW[o+6];
            float w7 = rowW[o+7];
            int ob = dy*DSTW;
            int b = 0;
            for (int dx = 0; dx < DSTW; dx++)
            {
                float R = w0*tmp[r0+b] + w1*tmp[r1+b] + w2*tmp[r2+b] + w3*tmp[r3+b]
                        + w4*tmp[r4+b] + w5*tmp[r5+b] + w6*tmp[r6+b] + w7*tmp[r7+b];
                int q = b+1;
                float G = w0*tmp[r0+q] + w1*tmp[r1+q] + w2*tmp[r2+q] + w3*tmp[r3+q]
                        + w4*tmp[r4+q] + w5*tmp[r5+q] + w6*tmp[r6+q] + w7*tmp[r7+q];
                q = b+2;
                float B = w0*tmp[r0+q] + w1*tmp[r1+q] + w2*tmp[r2+q] + w3*tmp[r3+q]
                        + w4*tmp[r4+q] + w5*tmp[r5+q] + w6*tmp[r6+q] + w7*tmp[r7+q];
                int ir = int(R + 0.5);
                int ig = int(G + 0.5);
                int ib = int(B + 0.5);
                if (ir < 0) ir = 0;
                if (ig < 0) ig = 0;
                if (ib < 0) ib = 0;
                if (ir > 255) ir = 255;
                if (ig > 255) ig = 255;
                if (ib > 255) ib = 255;
                dst[ob+dx] = 0xff000000 | (ir<<16) | (ig<<8) | ib;
                b = b + 3;
            }
            if ((dy & 63) == 0)
                Poll();
        }
    }
    else
    {
        for (int dy = 0; dy < DSTH; dy++)
        {
            int o = dy*TAPS;
            int r0 = rowI[o];
            int r1 = rowI[o+1];
            int r2 = rowI[o+2];
            int r3 = rowI[o+3];
            int r4 = rowI[o+4];
            int r5 = rowI[o+5];
            int r6 = rowI[o+6];
            int r7 = rowI[o+7];
            int r8 = rowI[o+8];
            int r9 = rowI[o+9];
            int r10 = rowI[o+10];
            int r11 = rowI[o+11];
            float w0 = rowW[o];
            float w1 = rowW[o+1];
            float w2 = rowW[o+2];
            float w3 = rowW[o+3];
            float w4 = rowW[o+4];
            float w5 = rowW[o+5];
            float w6 = rowW[o+6];
            float w7 = rowW[o+7];
            float w8 = rowW[o+8];
            float w9 = rowW[o+9];
            float w10 = rowW[o+10];
            float w11 = rowW[o+11];
            int ob = dy*DSTW;
            int b = 0;
            for (int dx = 0; dx < DSTW; dx++)
            {
                float R = w0*tmp[r0+b] + w1*tmp[r1+b] + w2*tmp[r2+b] + w3*tmp[r3+b]
                        + w4*tmp[r4+b] + w5*tmp[r5+b] + w6*tmp[r6+b] + w7*tmp[r7+b]
                        + w8*tmp[r8+b] + w9*tmp[r9+b] + w10*tmp[r10+b] + w11*tmp[r11+b];
                int q = b+1;
                float G = w0*tmp[r0+q] + w1*tmp[r1+q] + w2*tmp[r2+q] + w3*tmp[r3+q]
                        + w4*tmp[r4+q] + w5*tmp[r5+q] + w6*tmp[r6+q] + w7*tmp[r7+q]
                        + w8*tmp[r8+q] + w9*tmp[r9+q] + w10*tmp[r10+q] + w11*tmp[r11+q];
                q = b+2;
                float B = w0*tmp[r0+q] + w1*tmp[r1+q] + w2*tmp[r2+q] + w3*tmp[r3+q]
                        + w4*tmp[r4+q] + w5*tmp[r5+q] + w6*tmp[r6+q] + w7*tmp[r7+q]
                        + w8*tmp[r8+q] + w9*tmp[r9+q] + w10*tmp[r10+q] + w11*tmp[r11+q];
                int ir = int(R + 0.5);
                int ig = int(G + 0.5);
                int ib = int(B + 0.5);
                if (ir < 0) ir = 0;
                if (ig < 0) ig = 0;
                if (ib < 0) ib = 0;
                if (ir > 255) ir = 255;
                if (ig > 255) ig = 255;
                if (ib > 255) ib = 255;
                dst[ob+dx] = 0xff000000 | (ir<<16) | (ig<<8) | ib;
                b = b + 3;
            }
            if ((dy & 63) == 0)
                Poll();
        }
    }
}

/* ---------- user interface ------------------------------------------- */

/* button 0 = nearest, 1 = bilinear, 2..4 = Lanczos a = 2, 4, 6 */

int BtnMode(int i)
{
    if (i == 0) return 0;
    if (i == 1) return 1;
    return 2;
}

int BtnRadius(int i)
{
    if (i == 2) return 2;
    if (i == 3) return 4;
    if (i == 4) return 6;
    return 0;
}

int BtnTaps(int i)
{
    if (i == 0) return 1;
    if (i == 1) return 2;
    return 2*BtnRadius(i);
}

void BtnLabel(char* out, int i)
{
    if (i == 0)
        snprintf(out, 16, "nearest");
    else if (i == 1)
        snprintf(out, 16, "bilinear");
    else
        snprintf(out, 16, "a = %d", BtnRadius(i));
}

/* which filter button is currently active */
int SelBtn()
{
    if (gMode == 0) return 0;
    if (gMode == 1) return 1;
    if (gRadius == 2) return 2;
    if (gRadius == 4) return 3;
    return 4;
}

int HitButton(int mx, int my)
{
    for (int i = 0; i < NBTN; i++)
    {
        int y = BTNY + i*BTNSTEP;
        if (mx >= BTNX && mx < BTNX+BTNW && my >= y && my < y+BTNH)
            return i;
    }
    return -1;
}

int HitPause(int mx, int my)
{
    if (mx >= BTNX && mx < BTNX+BTNW && my >= PBY && my < PBY+BTNH)
        return 1;
    return 0;
}

/* the shared button chrome: filled body, 1px edge, centred label */
void ButtonBox(int y, char* label, int sel, int hot, int accent)
{
    int bg = 0xff222b36;
    if (hot)
        bg = 0xff2e3a49;
    if (sel)
    {
        bg = 0xff1d5876;
        if (accent)
            bg = 0xff6b4a12;
    }
    g.fillrect(BTNX, y, BTNW, BTNH, bg);

    int edge = COL_FRAME;
    int txt = COL_DIM;
    if (sel)
    {
        edge = COL_HI;
        txt = 0xffeaf6ff;
        if (accent)
        {
            edge = COL_WARN;
            txt = 0xfffff0d0;
        }
    }
    else if (hot)
    {
        txt = COL_TEXT;
    }
    g.fillrect(BTNX, y, BTNW, 1, edge);
    g.fillrect(BTNX, y+BTNH-1, BTNW, 1, edge);
    g.fillrect(BTNX, y, 1, BTNH, edge);
    g.fillrect(BTNX+BTNW-1, y, 1, BTNH, edge);

    float sc = 1.7;
    float tx = BTNX + (BTNW - TextW(label, sc))*0.5;
    Text(label, tx, y + 21, sc, txt);
}

void DrawFilterButton(int i, int hot)
{
    int y = BTNY + i*BTNSTEP;
    char s[16];
    BtnLabel(s, i);
    ButtonBox(y, s, i == SelBtn(), hot, 0);

    snprintf(s, 16, "%d tap", BtnTaps(i));
    Text(s, BTNX + BTNW + 8, y + 20, 1.0, COL_DIM);
}

/* ---------- main ------------------------------------------------------ */

int main()
{
    pfont = (CFont*)malloc(sizeof(CFont));
    CFont& font = *pfont;
    CFontMaker fm;
    fm.Init(font, 6);
    MakeLetters(fm);

    src  = (float*)malloc(SRCW*SRCH*3*sizeof(float));
    tmp  = (float*)malloc(DSTW*SRCH*3*sizeof(float));
    dst  = (int*)  malloc(DSTW*DSTH*sizeof(int));
    spv  = (int*)  malloc(SRCW*SRCH*sizeof(int));
    colW = (float*)malloc(DSTW*TAPS*sizeof(float));
    colI = (int*)  malloc(DSTW*TAPS*sizeof(int));
    rowW = (float*)malloc(DSTH*TAPS*sizeof(float));
    rowI = (int*)  malloc(DSTH*TAPS*sizeof(int));

    InitShapes();
    SetFilter(2, 6);

    int paused = 0;
    int needFilter = 1;
    int quit = 0;
    float animT = 0;
    double filterMs = 0;
    double frameMs = 1;
    double prev = Time();

    char line[64];

    while (true)
    {
        double now = Time();
        float dt = now - prev;
        prev = now;
        if (dt > 0.5)                /* a slow kernel must not teleport things */
            dt = 0.5;
        if (!paused)
        {
            animT = animT + dt;
            needFilter = 1;
        }

        /* ---- input ----
           A frame can take a quarter of a second, which is longer than a
           short click, so clicks and keys are taken from the event queues
           rather than sampled once per frame.  Poll() runs inside the
           vertical pass as well, so input that arrives mid-resample is
           queued there and handled on the next frame instead of lost. */
        int mx; int my; int mb;
        GetMouseState(mx, my, mb);
        int hot = HitButton(mx, my);
        int hotPause = HitPause(mx, my);

        SScincEvent ev;
        while (GetScincEvent(ev))
        {
            if (ev.type == 'MLDN')
            {
                int h = HitButton(ev.x, ev.y);
                if (h >= 0 && h != SelBtn())
                {
                    SetFilter(BtnMode(h), BtnRadius(h));
                    needFilter = 1;
                }
                if (HitPause(ev.x, ev.y))
                    paused = 1 - paused;
            }
        }

        int key;
        int press;
        while (GetKeyEvent(key, press))
        {
            if (press <= 0)
                continue;
            if (key == 'n' || key == 'N') { SetFilter(0, 0); needFilter = 1; }
            if (key == 'b' || key == 'B') { SetFilter(1, 0); needFilter = 1; }
            if (key == '2') { SetFilter(2, 2); needFilter = 1; }
            if (key == '4') { SetFilter(2, 4); needFilter = 1; }
            if (key == '6') { SetFilter(2, 6); needFilter = 1; }
            if (key == ' ')
                paused = 1 - paused;
            if (key == 'q' || key == 'Q')
                quit = 1;
        }
        if (KeyPressed(1000))
            quit = 1;
        if (quit)
            break;

        /* ---- resample ---- */
        if (needFilter)
        {
            BuildSource(animT);
            PackSource();
            double t0 = Time();
            HPass();
            VPass();
            filterMs = (Time() - t0)*1000.0;
            needFilter = 0;
        }

        /* ---- draw ---- */
        g.rgba32(COL_BG);
        g.FillRT();

        g.BitBlt(spv, SRCW, SRCH, PANX, PANY);
        g.BitBlt(dst, DSTW, DSTH, BIGX, BIGY);
        Frame(PANX, PANY, SRCW, SRCH, COL_FRAME);
        Frame(BIGX, BIGY, DSTW, DSTH, COL_FRAME);

        Text("RESAMPLING FILTERS", 24, 30, 1.9, COL_TEXT);
        snprintf(line, 64, "source %d x %d", SRCW, SRCH);
        Text(line, PANX, PANY-10, 1.0, COL_DIM);
        snprintf(line, 64, "magnified %d x %d   (%dx)", DSTW, DSTH, ZOOM);
        Text(line, BIGX + DSTW - TextW(line, 1.0), BIGY-10, 1.0, COL_DIM);
        Text("filter", BTNX, BTNY-12, 1.0, COL_DIM);

        for (int i = 0; i < NBTN; i++)
            DrawFilterButton(i, hot == i);

        int ty = BTNY + NBTN*BTNSTEP + 30;
        Text("time", BTNX, ty, 1.0, COL_DIM);
        snprintf(line, 64, "%.0f ms", filterMs);
        Text(line, BTNX + 54, ty, 1.0, COL_TEXT);

        ty = ty + 20;
        Text("frame", BTNX, ty, 1.0, COL_DIM);
        snprintf(line, 64, "%.1f fps", 1000.0/frameMs);
        Text(line, BTNX + 54, ty, 1.0, COL_TEXT);

        ty = ty + 20;
        Text("output", BTNX, ty, 1.0, COL_DIM);
        snprintf(line, 64, "%d px", DSTW*DSTH);
        Text(line, BTNX + 54, ty, 1.0, COL_TEXT);

        /* the pause button is labelled with what it will do, and lights up
           amber while the animation is actually frozen */
        if (paused)
            ButtonBox(PBY, "resume", 1, hotPause, 1);
        else
            ButtonBox(PBY, "pause", 0, hotPause, 1);

        Text("N B 2 4 6  filter", BTNX, BIGY+DSTH-42, 1.0, COL_DIM);
        Text("SPACE      pause",  BTNX, BIGY+DSTH-24, 1.0, COL_DIM);
        Text("Q          quit",   BTNX, BIGY+DSTH-6,  1.0, COL_DIM);

        Present();
        frameMs = (Time() - now)*1000.0;
        if (frameMs < 1.0)
            frameMs = 1.0;
    }
    return 0;
}
