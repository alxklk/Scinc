#include <stdio.h>
#include <math.h>

// ---------------------------------------------------------------------------
// Realtime software renderer:  z-buffer + stencil-shadow-volumes.
//
// Scene:  a triangulated torus (160 flat-shaded, differently coloured
//         triangles) spinning above a large -- but finite -- ground plane,
//         lit by a single point light.  Everything is rasterised by hand into
//         in-memory buffers and blitted to the screen with BitBlt:
//
//   * colour buffer  col[]  -- the visible image
//   * depth  buffer  zb[]   -- stored as 1/z (larger == nearer), classic z-buffer
//   * ambient buffer amb[]  -- the ambient-only colour of the nearest surface,
//                              used to darken pixels that end up in shadow
//   * stencil buffer st[]   -- shadow-volume counter (software stencil)
//
// Shadows use the textbook stencil-shadow-volume (z-pass) method:
//   1. render the lit scene into col/zb/amb;
//   2. find the torus silhouette as seen from the light (edges shared by one
//      light-facing and one light-back-facing triangle);
//   3. extrude each silhouette edge away from the light into a trapezoid and
//      rasterise those (invisible) trapezoids into the stencil buffer -- a
//      camera-facing trapezoid increments the count, a back-facing one
//      decrements it (winding rule), and only where it is in front of the
//      stored depth (z-pass);
//   4. any pixel whose stencil count is non-zero (odd, for a closed volume) is
//      inside the shadow volume, so we swap in its ambient-only colour.
//
// This is the z-pass variant, so (as with all z-pass shadow volumes) if the
// camera itself is inside a shadow volume the count is off by a constant and
// shadows invert -- fine for looking at the scene from the outside.
//
// Controls (printed at startup):
//   W/A/S/D  move,  R/F  up/down,  drag left mouse (or I/J/K/L) to look,
//   P pause spin,  close window then Q to quit.
//
// Scinc notes: no comma-initialised declarations; floor()/fabs() absent so we
// roll our own; structs are passed/returned by value; arrays need literal/macro
// sizes; big buffers come from malloc and are indexed flat as buf[y*W+x].
// ---------------------------------------------------------------------------

#define G_SCREEN_WIDTH  320
#define G_SCREEN_HEIGHT 200
#define G_SCREEN_SCALE  3        // window is 320*3 x 200*3 on screen
#define G_SCREEN_MODE   1        // raster mode: we own the pixels, BitBlt them

#include "graphics.h"
#include "../../ws.h"

#define M_PI 3.141592653589793

Graph g;

#define W 320
#define H 200

// ---- torus tessellation (major ring x minor ring); keep these in sync ----
#define TMAJ    10               // segments around the big ring
#define TMIN    8                // segments around the tube
#define TOR_NV  80               // = TMAJ*TMIN  vertices
#define TOR_NT  160              // = TMAJ*TMIN*2 triangles
#define TOR_NE  480              // >= unique edges (TOR_NT*3/2 = 240); over-alloc

#define NEARZ   0.15             // view-space near plane
#define EXT     32.0             // how far silhouette edges extrude for shadows
#define SHADOW_SIGN 1.0          // flip to -1 if shadows invert (winding sign)

// -------------------------- small vector maths -----------------------------
struct Vec { float x; float y; float z; };

Vec  V(float x, float y, float z){ Vec r; r.x=x; r.y=y; r.z=z; return r; }
Vec  vadd(Vec a, Vec b){ return V(a.x+b.x, a.y+b.y, a.z+b.z); }
Vec  vsub(Vec a, Vec b){ return V(a.x-b.x, a.y-b.y, a.z-b.z); }
Vec  vmul(Vec a, float s){ return V(a.x*s, a.y*s, a.z*s); }
float vdot(Vec a, Vec b){ return a.x*b.x + a.y*b.y + a.z*b.z; }
Vec  vcross(Vec a, Vec b){ return V(a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x); }
Vec  vnorm(Vec a){ float l=sqrt(vdot(a,a)); if(l<1e-9) l=1e-9; return vmul(a, 1.0/l); }

float f2min(float a, float b){ if(a<b) return a; return b; }
float f2max(float a, float b){ if(a>b) return a; return b; }
float f3min(float a, float b, float c){ return f2min(a, f2min(b,c)); }
float f3max(float a, float b, float c){ return f2max(a, f2max(b,c)); }
float myabs(float x){ if(x<0.0) return -x; return x; }

// ------------------------------- buffers -----------------------------------
int*   col;      // W*H  visible colour (0xAARRGGBB)
int*   amb;      // W*H  ambient-only colour of nearest surface (for shadowing)
float* zb;       // W*H  depth as 1/z, 0 == background (no geometry)
int*   st;       // W*H  stencil shadow counter

// ------------------------- torus geometry ----------------------------------
float mvx[TOR_NV]; float mvy[TOR_NV]; float mvz[TOR_NV];   // model space
float wx[TOR_NV];  float wy[TOR_NV];  float wz[TOR_NV];    // world space (per frame)

int ti0[TOR_NT]; int ti1[TOR_NT]; int ti2[TOR_NT];         // triangle vertex indices
int tcol[TOR_NT];                                          // triangle base colour
int facing[TOR_NT];                                        // 1 if lit-facing (per frame)

int e_a[TOR_NE]; int e_b[TOR_NE];                          // undirected edge (a<b)
int e_t0[TOR_NE]; int e_t1[TOR_NE];                        // its two adjacent triangles
int nEdge;

// ------------------------------ ground -------------------------------------
Vec g0; Vec g1; Vec g2; Vec g3;                            // 4 corners, y=0
int groundBase = 0xff41506a;                              // slate blue-grey

// ------------------------------ camera / light -----------------------------
Vec eye;
float yaw;    float pitch;
Vec cfwd; Vec cright; Vec cup;      // camera basis (rebuilt each frame)
Vec L;                              // point light position
float FOCX; float FOCY;            // projection focal terms

// ---------------------------------------------------------------------------
// hue in [0,1) -> a bright 0xAARRGGBB colour (simple 6-segment HSV, v=1)
// ---------------------------------------------------------------------------
int hue2rgb(float h)
{
    h = h - (int)h; if(h<0.0) h += 1.0;
    float x = h*6.0;
    int seg = (int)x;
    float f = x - seg;
    float lo = 0.20;
    float up = 0.20 + 0.80*f;
    float dn = 0.20 + 0.80*(1.0-f);
    float r = lo; float gg = lo; float b = lo;
    if(seg==0){ r=1.0; gg=up;  b=lo;  }
    else if(seg==1){ r=dn;  gg=1.0; b=lo;  }
    else if(seg==2){ r=lo;  gg=1.0; b=up;  }
    else if(seg==3){ r=lo;  gg=dn;  b=1.0; }
    else if(seg==4){ r=up;  gg=lo;  b=1.0; }
    else           { r=1.0; gg=lo;  b=dn;  }
    int R = r*255; int G = gg*255; int B = b*255;
    return 0xff000000 | (R<<16) | (G<<8) | B;
}

// flat shade of a base colour: ambient + Lambert*(1-ambient); ambientOnly skips
// the diffuse term (used for the darkened, in-shadow copy of every pixel).
int shade(int base, float ndl, int ambientOnly)
{
    float A = 0.28;
    float d = ndl; if(d<0.0) d=0.0;
    float f;
    if(ambientOnly) f = A;
    else            f = A + (1.0-A)*d;
    int r = ((base>>16)&255)*f;
    int gg= ((base>>8)&255)*f;
    int b = (base&255)*f;
    return 0xff000000 | (r<<16) | (gg<<8) | b;
}

// ---------------------------------------------------------------------------
// Build the torus mesh in model space (axis = +Y), its triangles (wound so
// that cross(v1-v0, v2-v0) points OUTWARD), per-triangle colours, and the
// undirected edge->triangle adjacency used for silhouette detection.
// ---------------------------------------------------------------------------
void addEdge(int a, int b, int tri)
{
    int mn = a; int mx = b;
    if(mn>mx){ mn=b; mx=a; }
    for(int i=0;i<nEdge;i++)
    {
        if(e_a[i]==mn && e_b[i]==mx){ e_t1[i]=tri; return; }
    }
    e_a[nEdge]=mn; e_b[nEdge]=mx; e_t0[nEdge]=tri; e_t1[nEdge]=-1; nEdge++;
}

void genTorus()
{
    float Rmaj = 3.2;
    float Rmin = 1.15;
    for(int i=0;i<TMAJ;i++)
    {
        float u = i*2.0*M_PI/TMAJ;
        for(int j=0;j<TMIN;j++)
        {
            float v = j*2.0*M_PI/TMIN;
            int k = i*TMIN + j;
            float ct = Rmaj + Rmin*cos(v);
            mvx[k] = ct*cos(u);
            mvy[k] = Rmin*sin(v);
            mvz[k] = ct*sin(u);
        }
    }

    nEdge = 0;
    int nt = 0;
    for(int i=0;i<TMAJ;i++)
    {
        int i1 = (i+1)%TMAJ;
        for(int j=0;j<TMIN;j++)
        {
            int j1 = (j+1)%TMIN;
            int a = i*TMIN  + j;      // (i ,j )
            int b = i1*TMIN + j;      // (i+1,j )
            int c = i1*TMIN + j1;     // (i+1,j+1)
            int d = i*TMIN  + j1;     // (i ,j+1)

            // both triangles of a quad share one colour
            int quadCol = hue2rgb((float)i/TMAJ);

            // outward-wound triangles (a,c,b) and (a,d,c)
            ti0[nt]=a; ti1[nt]=c; ti2[nt]=b;
            tcol[nt]=quadCol;
            addEdge(a,c,nt); addEdge(c,b,nt); addEdge(b,a,nt);
            nt++;

            ti0[nt]=a; ti1[nt]=d; ti2[nt]=c;
            tcol[nt]=quadCol;
            addEdge(a,d,nt); addEdge(d,c,nt); addEdge(c,a,nt);
            nt++;
        }
    }
}

void genGround()
{
    float s = 28.0;
    g0 = V(-s, 0.0, -s);
    g1 = V( s, 0.0, -s);
    g2 = V( s, 0.0,  s);
    g3 = V(-s, 0.0,  s);
}

// ---------------------------------------------------------------------------
// Rasterise one screen-space triangle.
//   mode 0: shade into col/zb/amb (c1 = lit colour, c2 = ambient colour)
//   mode 1: accumulate into the stencil buffer (z-pass increment/decrement)
// Vertices carry inverse depth iz (=1/z_view), linear in screen space, so a
// plain barycentric interpolation gives a perspective-correct depth.
// ---------------------------------------------------------------------------
void rasterSub(float x0,float y0,float iz0,
               float x1,float y1,float iz1,
               float x2,float y2,float iz2,
               int mode, int c1, int c2)
{
    float area = (x1-x0)*(y2-y0) - (x2-x0)*(y1-y0);
    if(area>-1e-6 && area<1e-6) return;                 // degenerate
    float invA = 1.0/area;

    int stInc = 0;
    if(mode==1)
    {
        if(area*SHADOW_SIGN > 0.0) stInc = 1;           // camera-facing trapezoid
        else                       stInc = -1;          // back-facing trapezoid
    }

    int minx = (int)f3min(x0,x1,x2);      if(minx<0) minx=0;
    int maxx = (int)f3max(x0,x1,x2)+1;    if(maxx>W) maxx=W;
    int miny = (int)f3min(y0,y1,y2);      if(miny<0) miny=0;
    int maxy = (int)f3max(y0,y1,y2)+1;    if(maxy>H) maxy=H;
    int pos = (area>0.0);

    // Barycentric edge functions are affine in the pixel centre, so step them
    // (and the interpolated inverse-depth) additively instead of recomputing.
    float dw0x = -(y2-y1); float dw0y = (x2-x1);
    float dw1x = -(y0-y2); float dw1y = (x0-x2);
    float dw2x = -(y1-y0); float dw2y = (x1-x0);
    float px0 = minx + 0.5; float py0 = miny + 0.5;
    float w0r = (x2-x1)*(py0-y1) - (y2-y1)*(px0-x1);
    float w1r = (x0-x2)*(py0-y2) - (y0-y2)*(px0-x2);
    float w2r = (x1-x0)*(py0-y0) - (y1-y0)*(px0-x0);
    float dizx = (dw0x*iz0 + dw1x*iz1 + dw2x*iz2)*invA;
    float dizy = (dw0y*iz0 + dw1y*iz1 + dw2y*iz2)*invA;
    float izr  = (w0r*iz0 + w1r*iz1 + w2r*iz2)*invA;

    for(int y=miny;y<maxy;y++)
    {
        float w0 = w0r; float w1 = w1r; float w2 = w2r; float iz = izr;
        int row = y*W;
        for(int x=minx;x<maxx;x++)
        {
            int inside;
            if(pos) inside = (w0>=0.0)&&(w1>=0.0)&&(w2>=0.0);
            else    inside = (w0<=0.0)&&(w1<=0.0)&&(w2<=0.0);
            if(inside)
            {
                int idx = row + x;
                if(mode==0)
                {
                    if(iz > zb[idx]){ zb[idx]=iz; col[idx]=c1; amb[idx]=c2; }
                }
                else
                {
                    if(iz > zb[idx]) st[idx] += stInc;    // z-pass
                }
            }
            w0 += dw0x; w1 += dw1x; w2 += dw2x; iz += dizx;
        }
        w0r += dw0y; w1r += dw1y; w2r += dw2y; izr += dizy;
    }
}

// Transform a world triangle to view space, clip it against the near plane,
// project the surviving polygon and fan-rasterise it.
void drawTri(Vec A, Vec B, Vec C, int mode, int c1, int c2)
{
    // world -> view (right-handed camera basis; vz>0 is in front)
    Vec ra = vsub(A,eye); Vec rb = vsub(B,eye); Vec rc = vsub(C,eye);
    float ix[3]; float iy[3]; float iz[3];
    ix[0]=vdot(ra,cright); iy[0]=vdot(ra,cup); iz[0]=vdot(ra,cfwd);
    ix[1]=vdot(rb,cright); iy[1]=vdot(rb,cup); iz[1]=vdot(rb,cfwd);
    ix[2]=vdot(rc,cright); iy[2]=vdot(rc,cup); iz[2]=vdot(rc,cfwd);

    // Sutherland-Hodgman clip of the triangle against  vz >= NEARZ
    float ox[5]; float oy[5]; float oz[5]; int m = 0;
    for(int i=0;i<3;i++)
    {
        int jn = (i+1)%3;
        int ini = (iz[i] >= NEARZ);
        int inj = (iz[jn] >= NEARZ);
        if(ini){ ox[m]=ix[i]; oy[m]=iy[i]; oz[m]=iz[i]; m++; }
        if(ini != inj)
        {
            float t = (NEARZ - iz[i]) / (iz[jn] - iz[i]);
            ox[m]=ix[i]+(ix[jn]-ix[i])*t;
            oy[m]=iy[i]+(iy[jn]-iy[i])*t;
            oz[m]=iz[i]+(iz[jn]-iz[i])*t;
            m++;
        }
    }
    if(m<3) return;

    // project surviving vertices
    float sx[5]; float sy[5]; float sz[5];
    for(int i=0;i<m;i++)
    {
        float inv = 1.0/oz[i];
        float ndcx = ox[i]*inv*FOCX;
        float ndcy = oy[i]*inv*FOCY;
        sx[i] = (ndcx*0.5 + 0.5)*W;
        sy[i] = (0.5 - ndcy*0.5)*H;
        sz[i] = inv;
    }

    for(int i=1;i<m-1;i++)
        rasterSub(sx[0],sy[0],sz[0], sx[i],sy[i],sz[i], sx[i+1],sy[i+1],sz[i+1], mode, c1, c2);
}

// Given a torus triangle and two of its vertices, return the directed edge
// (out0 -> out1) as it appears in that triangle's winding order.
void dirEdge(int tri, int va, int vb, int& out0, int& out1)
{
    int a = ti0[tri]; int b = ti1[tri]; int c = ti2[tri];
    if((a==va&&b==vb) || (b==va&&c==vb) || (c==va&&a==vb)){ out0=va; out1=vb; }
    else { out0=vb; out1=va; }
}

int main()
{
    col = (int*)  malloc(W*H*sizeof(int));
    amb = (int*)  malloc(W*H*sizeof(int));
    zb  = (float*)malloc(W*H*sizeof(float));
    st  = (int*)  malloc(W*H*sizeof(int));

    genTorus();
    genGround();

    L = V(7.0, 16.0, -5.0);
    eye = V(0.0, 8.5, -19.0);
    yaw = 0.0;
    pitch = -0.28;

    float aspect = (float)W/(float)H;
    float tanHalf = tan(0.55*M_PI*0.5*0.62);   // ~ 60 deg vertical fov
    FOCY = 1.0/tanHalf;
    FOCX = FOCY/aspect;

    Vec torusC = V(0.0, 4.6, 0.0);
    float tiltX = 0.55;                          // fixed lean so we see the hole

    printf("zbuffer_shadows -- software z-buffer + stencil shadow volumes\n");
    printf("  move : W/A/S/D    up/down : R/F\n");
    printf("  look : drag left mouse, or I/J/K/L\n");
    printf("  P pause spin.  Close window then Q to quit.\n");
    fflush(stdout);

    int pmx = -1; int pmy = -1;
    float prevT = Time();
    float spin = 0.0;
    int paused = 0;
    int pPrev = 0;
    float fpsT = prevT; int fpsN = 0;

    while(true)
    {
        float T = Time();
        float dt = T - prevT; prevT = T;
        if(dt>0.1) dt = 0.1;
        if(!paused) spin += dt*0.6;

        // ---------------- input ----------------
        int mx; int my; int mb;
        GetMouseState(mx, my, mb);
        if((mb&1) && pmx>=0)
        {
            yaw   += (mx-pmx)*0.006;
            pitch -= (my-pmy)*0.006;
        }
        pmx = mx; pmy = my;

        if(KeyPressed('j')) yaw   -= 1.4*dt;
        if(KeyPressed('l')) yaw   += 1.4*dt;
        if(KeyPressed('i')) pitch += 1.2*dt;
        if(KeyPressed('k')) pitch -= 1.2*dt;
        if(pitch >  1.5) pitch =  1.5;
        if(pitch < -1.5) pitch = -1.5;

        // camera basis
        cfwd = V(sin(yaw)*cos(pitch), sin(pitch), cos(yaw)*cos(pitch));
        cright = vnorm(vcross(V(0.0,1.0,0.0), cfwd));
        cup = vcross(cfwd, cright);

        float sp = 16.0*dt;
        Vec fwdH = vnorm(V(sin(yaw), 0.0, cos(yaw)));
        Vec rgtH = V(cos(yaw), 0.0, -sin(yaw));
        if(KeyPressed('w')) eye = vadd(eye, vmul(fwdH, sp));
        if(KeyPressed('s')) eye = vsub(eye, vmul(fwdH, sp));
        if(KeyPressed('d')) eye = vadd(eye, vmul(rgtH, sp));
        if(KeyPressed('a')) eye = vsub(eye, vmul(rgtH, sp));
        if(KeyPressed('r')) eye.y += sp;
        if(KeyPressed('f')) eye.y -= sp;
        if(eye.y < 0.6) eye.y = 0.6;                 // stay above the ground

        int pNow = KeyPressed('p');
        if(pNow && !pPrev) paused = !paused;
        pPrev = pNow;

        // ---------------- clear buffers ----------------
        int skyTop = 0xff20334d;
        for(int y=0;y<H;y++)
        {
            int row = y*W;
            for(int x=0;x<W;x++)
            {
                col[row+x] = skyTop;
                zb[row+x] = 0.0;
                st[row+x] = 0;
            }
            if((y&15)==0) Poll();
        }

        // ---------------- spin the torus into world space ----------------
        float cx = cos(tiltX); float sxr = sin(tiltX);
        float cy = cos(spin);  float syr = sin(spin);
        for(int i=0;i<TOR_NV;i++)
        {
            float px = mvx[i]; float py = mvy[i]; float pz = mvz[i];
            // tilt about X
            float y1 = py*cx - pz*sxr;
            float z1 = py*sxr + pz*cx;
            float x1 = px;
            // spin about Y
            wx[i] = (x1*cy + z1*syr) + torusC.x;
            wy[i] = y1 + torusC.y;
            wz[i] = (-x1*syr + z1*cy) + torusC.z;
        }

        // ---------------- geometry pass: ground ----------------
        Vec gN = V(0.0,1.0,0.0);
        Vec gcen = V(0.0,0.0,0.0);
        float gndl = vdot(gN, vnorm(vsub(L,gcen)));
        int gLit = shade(groundBase, gndl, 0);
        int gAmb = shade(groundBase, gndl, 1);
        drawTri(g0,g1,g2, 0, gLit, gAmb);
        drawTri(g0,g2,g3, 0, gLit, gAmb);

        // ---------------- geometry pass: torus ----------------
        for(int t=0;t<TOR_NT;t++)
        {
            Vec A = V(wx[ti0[t]], wy[ti0[t]], wz[ti0[t]]);
            Vec B = V(wx[ti1[t]], wy[ti1[t]], wz[ti1[t]]);
            Vec C = V(wx[ti2[t]], wy[ti2[t]], wz[ti2[t]]);
            Vec N = vnorm(vcross(vsub(B,A), vsub(C,A)));
            Vec cen = vmul(vadd(vadd(A,B),C), 1.0/3.0);
            Vec Lv = vnorm(vsub(L, cen));
            float ndl = vdot(N, Lv);
            facing[t] = (ndl > 0.0);
            int lit = shade(tcol[t], ndl, 0);
            int amc = shade(tcol[t], ndl, 1);
            drawTri(A,B,C, 0, lit, amc);
        }

        // ---------------- shadow pass: stencil volumes ----------------
        for(int e=0;e<nEdge;e++)
        {
            int t0 = e_t0[e]; int t1 = e_t1[e];
            if(t1<0) continue;
            if(facing[t0] == facing[t1]) continue;       // not a silhouette edge

            int front = t0; if(!facing[t0]) front = t1;   // lit-facing side
            int p; int q;
            dirEdge(front, e_a[e], e_b[e], p, q);

            Vec Aw = V(wx[p], wy[p], wz[p]);
            Vec Bw = V(wx[q], wy[q], wz[q]);
            Vec Ae = vadd(Aw, vmul(vnorm(vsub(Aw,L)), EXT));
            Vec Be = vadd(Bw, vmul(vnorm(vsub(Bw,L)), EXT));

            // extruded trapezoid, consistent winding: Aw->Bw->Be->Ae
            drawTri(Aw, Bw, Be, 1, 0, 0);
            drawTri(Aw, Be, Ae, 1, 0, 0);
        }

        // ---------------- resolve: darken shadowed geometry ----------------
        int n = W*H;
        for(int i=0;i<n;i++)
        {
            if(st[i]!=0 && zb[i]>0.0) col[i] = amb[i];
        }

        g.BitBlt(col, W, H, 0, 0);
        Present();

        fpsN++;
        if(T - fpsT > 2.0)
        {
            printf("%.1f fps   eye %.1f %.1f %.1f  edges=%i\n",
                   fpsN/(T-fpsT), eye.x, eye.y, eye.z, nEdge);
            fflush(stdout);
            fpsT = T; fpsN = 0;
        }
    }
    return 0;
}
