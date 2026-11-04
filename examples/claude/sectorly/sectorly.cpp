#include <stdio.h>

// Sectorly - a tiny vector editor for "lines and quarter sectors" drawings.
//
// The canvas is a 64x64 lattice of grid points. A picture is an ordered list
// of primitives; each one is a directed stroke A->B between two grid points,
// and its filled body always lies to the RIGHT of the stroke direction:
//
//   Line   (A->B horizontal or vertical, length L): the body is the L x L
//          square on the right-hand side of the line.
//   Sector (A->B diagonal, |dx| == |dy| = R): the stroke is a quarter arc of
//          radius R whose centre C is on the right of the chord, so the arc
//          turns clockwise around C. With D the opposite corner of the R x R
//          bounding square, the body is one of
//            inside  - pie C,A,arc,B
//            outside - corner A,arc,B,D (square minus the pie)
//            lens    - between the arc and its mirror across the chord AB
//
// Fill and outline colours come from a 16-entry palette; entry 0 is
// transparent. The outline strokes only the primitive's own edge. The
// palette is editable and is saved with the picture.
//
// Mouse (canvas)
//   Draw tool    drag from point to point: near horizontal/vertical -> line,
//                near diagonal -> sector (body on the right of the drag)
//   Select tool  click picks the topmost primitive (Shift toggles), drag on
//                a selected one moves the selection (Ctrl+drag moves a copy),
//                drag on empty space selects by rectangle
//   Edit tool    drag an end handle to move that end of a primitive (the other
//                end stays; the primitive re-snaps to a line or a sector);
//                handles of selected primitives win where ends coincide
//   Right click  picks fill, outline and sector mode from a primitive
//   Wheel        zoom around the cursor; middle drag pans
// Mouse (panel)  left click on a swatch = fill colour, right click = outline;
//                both also recolour the selection; double click opens an
//                RGB/HSL dialog to edit that palette colour (Enter/Esc): a
//                radio button picks one channel and the big square spans the
//                other two of its family
// Keys
//   D / S / E    draw / select / edit tool 1 2 3   inside / outside / lens
//   arrows       move selection            R / Shift+R   rotate 90 cw / ccw
//   PgUp / PgDn  selection one step up / down in draw order (a scattered
//                selection is first gathered into one solid block)
//   Del          delete selection          Ctrl+A  select all, Esc = none
//   Ctrl+C / V   copy / paste at mouse     Ctrl+D  duplicate
//   Ctrl+Z / Y   undo / redo               Tab     direction arrows
//   G            grid dots on/off          Ctrl+S / O   export / load SVG
//   Home         fit the grid in the view
//
// The window is resizable (min 560x610 logical); the panel stays at the right
// edge and a fitted view refits. The x2 button next to the title makes the
// window twice as big, rendered at twice the resolution (sharp, for HiDPI
// screens), and back.
//
// Run: DISPLAY=:0 Scinc sectorly.cpp [file.svg]
// The SVG keeps the internal model in a comment, so Load restores it exactly.

typedef char* pchar;

#define NO_GREEDY_EVENT
#define G_SCREEN_WIDTH  1000   // initial logical size
#define G_SCREEN_HEIGHT 700
#define G_SCREEN_SCALE  1      // unscaled; the x2 button doubles the window
#define G_SCREEN_MODE   2

#include "graphics.h"
#include "../../ws.h"
#include "../../graphic/font.h"
#include "../../graphic/letters.h"
#include "../../graphic/gtext.h"

Graph g;
CFont* pfont = 0;

// Rendering scale: 1, or 2 with the x2 button. All layout is in logical
// units; paths go through the graph transform (see SetScale), rects, circles
// and stroke widths through these helpers, and mouse events are divided by SC.
float SC = 1;
int scaleReq = 0;      // the x2 button was pressed

void FillRect(float x, float y, float w, float h, int col)
{
	g.fillrect(x * SC, y * SC, w * SC, h * SC, col);
}

void Ring(float x, float y, float r, float w, float a, int col)
{
	g.Circle(x * SC, y * SC, r * SC, w * SC, a * SC, col);
}

void Width(float r, float k)
{
	g.width(r * SC, k);
}

#define M_PI 3.14159265358979

#define GRID   64      // grid points per side
#define PANELW 210     // width of the tool panel at the right edge
#define MINW   560     // smallest logical window size
#define MINH   610

// Current logical window size and the derived layout; the window is
// resizable, see ApplyResize.
int scrW = G_SCREEN_WIDTH;
int scrH = G_SCREEN_HEIGHT;
int panelX = G_SCREEN_WIDTH - PANELW;   // left edge of the tool panel
int viewFitted = 1;                      // the view shows the whole grid
#define VIEWMARGIN 14  // free border around the grid when fitted
#define ARCSEG 24      // polyline segments per quarter arc in panel icons

// View transform: screen = view origin + grid * view scale (pixels per step).
float viewScale = 10;
float viewOX = 14;
float viewOY = 14;

void FitView()
{
	viewFitted = 1;
	float sx = (panelX - 2 * VIEWMARGIN) / (GRID - 1.0);
	float sy = (scrH - 2 * VIEWMARGIN) / (GRID - 1.0);
	viewScale = sx < sy ? sx : sy;
	viewOX = (panelX - viewScale * (GRID - 1)) / 2;
	viewOY = (scrH - viewScale * (GRID - 1)) / 2;
}

// Zooms by factor k keeping the grid point under screen (px,py) in place.
void ZoomAt(float px, float py, float k)
{
	float ns = viewScale * k;
	if (ns < 4)
		ns = 4;
	if (ns > 120)
		ns = 120;
	k = ns / viewScale;
	viewFitted = 0;
	viewOX = px - (px - viewOX) * k;
	viewOY = py - (py - viewOY) * k;
	viewScale = ns;
}

#define PRIM_LINE   0
#define PRIM_SECTOR 1

#define FILL_INSIDE  0
#define FILL_OUTSIDE 1
#define FILL_LENS    2

#define TOOL_DRAW   0
#define TOOL_SELECT 1
#define TOOL_EDIT   2

#define MAXP    1024   // primitives in a picture
#define PSTRIDE 8      // ints per primitive in undo / clipboard storage
#define UNDOLEV 64

/* ---------- model ----------------------------------------------------- */

struct Prim
{
	int type;
	int ax;
	int ay;
	int bx;
	int by;
	int mode;
	int fill;
	int line;
	int sel;
};

Prim prims[MAXP];
int nPrim = 0;

int palette[16];

void InitPalette()
{
	palette[0]  = 0x00000000;   // transparent
	palette[1]  = 0xff000000;   // black
	palette[2]  = 0xffffffff;   // white
	palette[3]  = 0xff808080;   // grey
	palette[4]  = 0xffc0c0c0;   // light grey
	palette[5]  = 0xff404040;   // dark grey
	palette[6]  = 0xffe03030;   // red
	palette[7]  = 0xffff9020;   // orange
	palette[8]  = 0xfff0e040;   // yellow
	palette[9]  = 0xff40c040;   // green
	palette[10] = 0xff206020;   // dark green
	palette[11] = 0xff40d0e0;   // cyan
	palette[12] = 0xff3060e0;   // blue
	palette[13] = 0xff202880;   // navy
	palette[14] = 0xffa040c0;   // purple
	palette[15] = 0xff905020;   // brown
}

int IAbs(int v)
{
	if (v < 0)
		return -v;
	return v;
}

int IRound(float f)
{
	if (f >= 0)
		return int(f + 0.5);
	return -int(-f + 0.5);
}

int IMin(int a, int b) { return a < b ? a : b; }
int IMax(int a, int b) { return a > b ? a : b; }

// Centre C and opposite corner D of a sector's bounding square. Half of the
// chord's right-hand normal leads from the chord midpoint to C; for a
// diagonal chord all of these land exactly on grid points.
void SectorCD(Prim& p, int& cx, int& cy, int& dx, int& dy)
{
	int sx = p.ax + p.bx;
	int sy = p.ay + p.by;
	int hx = -(p.by - p.ay);
	int hy = p.bx - p.ax;
	cx = (sx + hx) / 2;
	cy = (sy + hy) / 2;
	dx = (sx - hx) / 2;
	dy = (sy - hy) / 2;
}

// Bounding box of the primitive's body (its four corner grid points).
void PrimBox(Prim& p, int& x0, int& y0, int& x1, int& y1)
{
	int cx;
	int cy;
	int dx;
	int dy;
	if (p.type == PRIM_LINE)
	{
		// right-hand normal on screen (y grows down): (dx,dy) -> (-dy,dx)
		cx = p.bx - (p.by - p.ay);
		cy = p.by + (p.bx - p.ax);
		dx = p.ax - (p.by - p.ay);
		dy = p.ay + (p.bx - p.ax);
	}
	else
		SectorCD(p, cx, cy, dx, dy);
	x0 = IMin(IMin(p.ax, p.bx), IMin(cx, dx));
	y0 = IMin(IMin(p.ay, p.by), IMin(cy, dy));
	x1 = IMax(IMax(p.ax, p.bx), IMax(cx, dx));
	y1 = IMax(IMax(p.ay, p.by), IMax(cy, dy));
}

int PrimInGrid(Prim& p)
{
	int x0;
	int y0;
	int x1;
	int y1;
	PrimBox(p, x0, y0, x1, y1);
	return x0 >= 0 && y0 >= 0 && x1 < GRID && y1 < GRID;
}

void SetPrim(Prim& p, int type, int ax, int ay, int bx, int by, int mode, int fill, int line)
{
	p.type = type;
	p.ax = ax;
	p.ay = ay;
	p.bx = bx;
	p.by = by;
	p.mode = mode;
	p.fill = fill;
	p.line = line;
	p.sel = 0;
}

int AddPrim(int type, int ax, int ay, int bx, int by, int mode, int fill, int line)
{
	if (nPrim >= MAXP)
		return -1;
	SetPrim(prims[nPrim], type, ax, ay, bx, by, mode, fill, line);
	nPrim++;
	return nPrim - 1;
}

int CountSelected()
{
	int n = 0;
	for (int i = 0; i < nPrim; i++)
		if (prims[i].sel)
			n++;
	return n;
}

void SelectAll(int s)
{
	for (int i = 0; i < nPrim; i++)
		prims[i].sel = s;
}

/* ---------- undo history ---------------------------------------------- */
// A ring of whole-picture snapshots; hCur is the snapshot that matches the
// current picture.

int* hBuf = 0;
int hCount[UNDOLEV];
int hPal[UNDOLEV * 16];   // palette of each snapshot
int hStart = 0;
int hLen = 0;
int hCur = -1;

// Scinc has no pointer arithmetic, so storage is addressed as base + offset.
void PackPrim(int* d, int o, Prim& p)
{
	d[o] = p.type;
	d[o + 1] = p.ax;
	d[o + 2] = p.ay;
	d[o + 3] = p.bx;
	d[o + 4] = p.by;
	d[o + 5] = p.mode;
	d[o + 6] = p.fill;
	d[o + 7] = p.line;
}

void UnpackPrim(int* d, int o, Prim& p)
{
	SetPrim(p, d[o], d[o + 1], d[o + 2], d[o + 3], d[o + 4], d[o + 5], d[o + 6], d[o + 7]);
}

void Snapshot()
{
	if (hCur < hLen - 1)
		hLen = hCur + 1;              // drop the redo tail
	if (hLen == UNDOLEV)
	{
		hStart = (hStart + 1) % UNDOLEV;
		hLen--;
	}
	int slot = (hStart + hLen) % UNDOLEV;
	int base = slot * MAXP * PSTRIDE;
	for (int i = 0; i < nPrim; i++)
		PackPrim(hBuf, base + i * PSTRIDE, prims[i]);
	hCount[slot] = nPrim;
	for (int k = 0; k < 16; k++)
		hPal[slot * 16 + k] = palette[k];
	hLen++;
	hCur = hLen - 1;
}

void Restore(int idx)
{
	int slot = (hStart + idx) % UNDOLEV;
	int base = slot * MAXP * PSTRIDE;
	nPrim = hCount[slot];
	for (int i = 0; i < nPrim; i++)
		UnpackPrim(hBuf, base + i * PSTRIDE, prims[i]);
	for (int k = 0; k < 16; k++)
		palette[k] = hPal[slot * 16 + k];
	hCur = idx;
}

void Undo()
{
	if (hCur > 0)
		Restore(hCur - 1);
}

void Redo()
{
	if (hCur < hLen - 1)
		Restore(hCur + 1);
}

/* ---------- status line ------------------------------------------------ */

char msg[160];

void SetMsg(char* s)
{
	int i = 0;
	while (s[i] != 0 && i < 159)
	{
		msg[i] = s[i];
		i++;
	}
	msg[i] = 0;
}

/* ---------- editing operations ----------------------------------------- */

void DeleteSelected()
{
	int n = 0;
	for (int i = 0; i < nPrim; i++)
	{
		if (prims[i].sel == 0)
		{
			if (n != i)
				prims[n] = prims[i];
			n++;
		}
	}
	if (n != nPrim)
	{
		nPrim = n;
		Snapshot();
	}
}

// Moves the selection by (dx,dy) grid steps if everything stays on the grid.
int TryMoveSelected(int dx, int dy)
{
	for (int i = 0; i < nPrim; i++)
	{
		if (prims[i].sel)
		{
			Prim t = prims[i];
			t.ax = t.ax + dx;
			t.ay = t.ay + dy;
			t.bx = t.bx + dx;
			t.by = t.by + dy;
			if (PrimInGrid(t) == 0)
				return 0;
		}
	}
	for (int i = 0; i < nPrim; i++)
	{
		if (prims[i].sel)
		{
			prims[i].ax = prims[i].ax + dx;
			prims[i].ay = prims[i].ay + dy;
			prims[i].bx = prims[i].bx + dx;
			prims[i].by = prims[i].by + dy;
		}
	}
	return 1;
}

int SelectionBox(int& x0, int& y0, int& x1, int& y1)
{
	int any = 0;
	for (int i = 0; i < nPrim; i++)
	{
		if (prims[i].sel)
		{
			int a0;
			int b0;
			int a1;
			int b1;
			PrimBox(prims[i], a0, b0, a1, b1);
			if (any == 0)
			{
				x0 = a0;
				y0 = b0;
				x1 = a1;
				y1 = b1;
			}
			x0 = IMin(x0, a0);
			y0 = IMin(y0, b0);
			x1 = IMax(x1, a1);
			y1 = IMax(y1, b1);
			any = 1;
		}
	}
	return any;
}

// Rotates the selection by 90 degrees around the centre of its bounding box.
// Rotation keeps orientation, so every body stays on the right of its stroke.
// Doubled centre coordinates must have an even sum for the rotated points to
// land on grid points, so an odd sum nudges the centre by half a step.
void RotateSelected(int cw)
{
	int x0;
	int y0;
	int x1;
	int y1;
	if (SelectionBox(x0, y0, x1, y1) == 0)
		return;
	int c2x = x0 + x1;
	int c2y = y0 + y1;
	if ((c2x + c2y) % 2 != 0)
		c2x++;
	int s = (c2x + c2y) / 2;    // x' = s - y,  y' = x - d   (clockwise)
	int d = (c2x - c2y) / 2;    // x' = y + d,  y' = s - x   (counter-clockwise)
	for (int pass = 0; pass < 2; pass++)
	{
		for (int i = 0; i < nPrim; i++)
		{
			if (prims[i].sel)
			{
				Prim t = prims[i];
				if (cw)
				{
					t.ax = s - prims[i].ay;
					t.ay = prims[i].ax - d;
					t.bx = s - prims[i].by;
					t.by = prims[i].bx - d;
				}
				else
				{
					t.ax = prims[i].ay + d;
					t.ay = s - prims[i].ax;
					t.bx = prims[i].by + d;
					t.by = s - prims[i].bx;
				}
				if (pass == 0)
				{
					if (PrimInGrid(t) == 0)
					{
						SetMsg("Rotation would leave the grid");
						return;
					}
				}
				else
					prims[i] = t;
			}
		}
	}
	Snapshot();
}

// Appends copies of the selected primitives, deselects the originals and
// selects the copies.
int DuplicateSelected()
{
	int n0 = nPrim;
	for (int i = 0; i < n0; i++)
	{
		if (prims[i].sel)
		{
			if (nPrim >= MAXP)
				break;
			prims[nPrim] = prims[i];
			prims[i].sel = 0;
			nPrim++;
		}
	}
	return nPrim - n0;
}

Prim orderTmp[MAXP];

// Moves the selection one step up (later, drawn on top) or down in the draw
// order. A scattered selection is first gathered into one solid block at
// its topmost (up) or bottommost (down) member, without passing anything
// else; only an already solid block swaps places with its neighbour.
void ReorderSelected(int up)
{
	int lo = -1;
	int hi = -1;
	int cnt = 0;
	for (int i = 0; i < nPrim; i++)
	{
		if (prims[i].sel)
		{
			if (lo < 0)
				lo = i;
			hi = i;
			cnt++;
		}
	}
	if (cnt == 0)
		return;
	if (hi - lo + 1 == cnt)
	{
		// solid: rotate the neighbour to the other side of the block
		if (up && hi < nPrim - 1)
		{
			Prim t = prims[hi + 1];
			for (int i = hi + 1; i > lo; i--)
				prims[i] = prims[i - 1];
			prims[lo] = t;
		}
		else if (up == 0 && lo > 0)
		{
			Prim t = prims[lo - 1];
			for (int i = lo - 1; i < hi; i++)
				prims[i] = prims[i + 1];
			prims[hi] = t;
		}
		else
			return;
	}
	else
	{
		// gather at the anchor (a selected item): the unselected ones before
		// it, then the whole selection, then the unselected ones after it
		int anchor = up ? hi : lo;
		int n = 0;
		for (int i = 0; i < anchor; i++)
			if (prims[i].sel == 0)
			{
				orderTmp[n] = prims[i];
				n++;
			}
		for (int i = 0; i < nPrim; i++)
			if (prims[i].sel)
			{
				orderTmp[n] = prims[i];
				n++;
			}
		for (int i = anchor + 1; i < nPrim; i++)
			if (prims[i].sel == 0)
			{
				orderTmp[n] = prims[i];
				n++;
			}
		for (int i = 0; i < nPrim; i++)
			prims[i] = orderTmp[i];
	}
	Snapshot();
}

int clip[MAXP * PSTRIDE];
int nClip = 0;
int clipX0 = 0;
int clipY0 = 0;

void CopySelected()
{
	int x0;
	int y0;
	int x1;
	int y1;
	if (SelectionBox(x0, y0, x1, y1) == 0)
		return;
	nClip = 0;
	for (int i = 0; i < nPrim; i++)
	{
		if (prims[i].sel)
		{
			PackPrim(clip, nClip * PSTRIDE, prims[i]);
			nClip++;
		}
	}
	clipX0 = x0;
	clipY0 = y0;
	SetMsg("Copied");
}

// Pastes the clipboard with its bounding box's top-left at grid point (gx,gy),
// pushed back inside the grid if needed. The pasted set becomes the selection.
void Paste(int gx, int gy)
{
	if (nClip == 0 || nPrim + nClip > MAXP)
		return;
	SelectAll(0);
	int n0 = nPrim;
	for (int i = 0; i < nClip; i++)
	{
		UnpackPrim(clip, i * PSTRIDE, prims[nPrim]);
		prims[nPrim].sel = 1;
		nPrim++;
	}
	int x0;
	int y0;
	int x1;
	int y1;
	SelectionBox(x0, y0, x1, y1);
	int dx = gx - x0;
	int dy = gy - y0;
	if (x1 + dx >= GRID)
		dx = GRID - 1 - x1;
	if (y1 + dy >= GRID)
		dy = GRID - 1 - y1;
	if (x0 + dx < 0)
		dx = -x0;
	if (y0 + dy < 0)
		dy = -y0;
	if (TryMoveSelected(dx, dy) == 0)
	{
		nPrim = n0;           // larger than the grid: give up
		return;
	}
	Snapshot();
}

void ApplyFill(int col)
{
	int n = 0;
	for (int i = 0; i < nPrim; i++)
		if (prims[i].sel && prims[i].fill != col)
		{
			prims[i].fill = col;
			n++;
		}
	if (n)
		Snapshot();
}

void ApplyLine(int col)
{
	int n = 0;
	for (int i = 0; i < nPrim; i++)
		if (prims[i].sel && prims[i].line != col)
		{
			prims[i].line = col;
			n++;
		}
	if (n)
		Snapshot();
}

void ApplyMode(int mode)
{
	int n = 0;
	for (int i = 0; i < nPrim; i++)
		if (prims[i].sel && prims[i].type == PRIM_SECTOR)
		{
			prims[i].mode = mode;
			n++;
		}
	if (n)
		Snapshot();
}

/* ---------- hit testing ------------------------------------------------ */

// Pick tolerance in grid units: about 5 pixels at the current zoom.
float HitTol()
{
	float t = 5 / viewScale;
	if (t > 0.45)
		t = 0.45;
	return t;
}

#define HITTOL HitTol()

float Dist(float x0, float y0, float x1, float y1)
{
	return sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));
}

int HitPrim(Prim& p, float fx, float fy)
{
	int x0;
	int y0;
	int x1;
	int y1;
	PrimBox(p, x0, y0, x1, y1);
	if (fx < x0 - HITTOL || fy < y0 - HITTOL || fx > x1 + HITTOL || fy > y1 + HITTOL)
		return 0;
	int inBox = fx >= x0 && fy >= y0 && fx <= x1 && fy <= y1;
	if (p.type == PRIM_LINE)
	{
		if (p.fill != 0 && inBox)
			return 1;
		// distance to the axis-aligned segment
		float cx = fx;
		float cy = fy;
		if (p.ax == p.bx)
		{
			cx = p.ax;
			if (cy < IMin(p.ay, p.by)) cy = IMin(p.ay, p.by);
			if (cy > IMax(p.ay, p.by)) cy = IMax(p.ay, p.by);
		}
		else
		{
			cy = p.ay;
			if (cx < IMin(p.ax, p.bx)) cx = IMin(p.ax, p.bx);
			if (cx > IMax(p.ax, p.bx)) cx = IMax(p.ax, p.bx);
		}
		return Dist(fx, fy, cx, cy) < HITTOL;
	}
	int cx;
	int cy;
	int dx;
	int dy;
	SectorCD(p, cx, cy, dx, dy);
	float r = IAbs(p.bx - p.ax);
	float dc = Dist(fx, fy, cx, cy);
	if (inBox && dc - r < HITTOL && r - dc < HITTOL)
		return 1;            // on the arc
	if (p.fill == 0 || inBox == 0)
		return 0;
	if (p.mode == FILL_INSIDE)
		return dc <= r;
	if (p.mode == FILL_OUTSIDE)
		return dc >= r;
	return dc <= r && Dist(fx, fy, dx, dy) <= r;
}

int HitTop(float fx, float fy)
{
	for (int i = nPrim - 1; i >= 0; i--)
		if (HitPrim(prims[i], fx, fy))
			return i;
	return -1;
}

/* ---------- drawing ---------------------------------------------------- */

float SX(float gx) { return viewOX + gx * viewScale; }
float SY(float gy) { return viewOY + gy * viewScale; }

// Appends the quarter arc around centre (cx,cy) from P to Q to the current
// path. P-C and Q-C are perpendicular and equally long, so
// C + (P-C)cos t + (Q-C)sin t sweeps exactly that arc for t in [0, pi/2].
// The segment count follows the on-screen radius so arcs stay smooth zoomed.
void ArcPath(float cx, float cy, float px, float py, float qx, float qy, int moveFirst)
{
	float rpx = (IAbs(int(px - cx)) + IAbs(int(py - cy))) * viewScale;
	int seg = int(rpx / 4);
	if (seg < 8)
		seg = 8;
	if (seg > 96)
		seg = 96;
	for (int i = 0; i <= seg; i++)
	{
		float t = (M_PI / 2) * i / seg;
		float c = cos(t);
		float s = sin(t);
		float x = cx + (px - cx) * c + (qx - cx) * s;
		float y = cy + (py - cy) * c + (qy - cy) * s;
		if (i == 0 && moveFirst)
			g.M(SX(x), SY(y));
		else
			g.L(SX(x), SY(y));
	}
}

// Builds the closed outline of the primitive's body as the current path.
void BodyPath(Prim& p)
{
	g.clear();
	if (p.type == PRIM_LINE)
	{
		int nx = -(p.by - p.ay);
		int ny = p.bx - p.ax;
		g.M(SX(p.ax), SY(p.ay));
		g.L(SX(p.bx), SY(p.by));
		g.L(SX(p.bx + nx), SY(p.by + ny));
		g.L(SX(p.ax + nx), SY(p.ay + ny));
	}
	else
	{
		int cx;
		int cy;
		int dx;
		int dy;
		SectorCD(p, cx, cy, dx, dy);
		if (p.mode == FILL_INSIDE)
		{
			g.M(SX(cx), SY(cy));
			ArcPath(cx, cy, p.ax, p.ay, p.bx, p.by, 0);
		}
		else if (p.mode == FILL_OUTSIDE)
		{
			ArcPath(cx, cy, p.ax, p.ay, p.bx, p.by, 1);
			g.L(SX(dx), SY(dy));
		}
		else
		{
			ArcPath(cx, cy, p.ax, p.ay, p.bx, p.by, 1);
			ArcPath(dx, dy, p.bx, p.by, p.ax, p.ay, 0);
		}
	}
	g.close();
	g.fin();
}

// Builds the primitive's own stroke (the line or the arc) as the current path.
void StrokePath(Prim& p)
{
	g.clear();
	if (p.type == PRIM_LINE)
	{
		g.M(SX(p.ax), SY(p.ay));
		g.L(SX(p.bx), SY(p.by));
	}
	else
	{
		int cx;
		int cy;
		int dx;
		int dy;
		SectorCD(p, cx, cy, dx, dy);
		ArcPath(cx, cy, p.ax, p.ay, p.bx, p.by, 1);
	}
	g.fin();
}

void DrawPrim(Prim& p, int fillCol, int lineCol)
{
	if (fillCol != 0)
	{
		BodyPath(p);
		g.rgba32(fillCol);
		g.fill1();
	}
	if (lineCol != 0)
	{
		// the outline is part of the picture, so it scales with the zoom
		float w = viewScale / 8;
		if (w < 0.8)
			w = 0.8;
		StrokePath(p);
		g.rgba32(lineCol);
		Width(w, w);
		g.stroke();
	}
}

// Arrowhead at B pointing along the stroke direction.
void DrawArrow(Prim& p, int col)
{
	float ux = p.bx - p.ax;
	float uy = p.by - p.ay;
	if (p.type == PRIM_SECTOR)
	{
		// arc tangent at B is perpendicular to B-C, clockwise around C
		int cx;
		int cy;
		int dx;
		int dy;
		SectorCD(p, cx, cy, dx, dy);
		ux = -(p.by - cy);
		uy = p.bx - cx;
	}
	float l = sqrt(ux * ux + uy * uy);
	ux = ux / l;
	uy = uy / l;
	float tx = SX(p.bx);
	float ty = SY(p.by);
	g.clear();
	g.M(tx, ty);
	g.L(tx - ux * 9 - uy * 4, ty - uy * 9 + ux * 4);
	g.L(tx - ux * 9 + uy * 4, ty - uy * 9 - ux * 4);
	g.close();
	g.fin();
	g.rgba32(col);
	g.fill1();
}

void Text(char* s, float x, float y, float sc, int col)
{
	g.clear();
	DrawText(g, *pfont, s, 0, x, y, sc, sc);
	g.fin();
	g.rgba32(col);
	Width(sc * 0.6 + 0.5, sc * 0.6 + 0.5);
	g.stroke();
}

int TextW(char* s, float sc)
{
	int n = 0;
	while (s[n] != 0)
		n++;
	return int(n * 6 * sc);
}

void Frame(float x, float y, float w, float h, int col)
{
	FillRect(x, y, w, 1, col);
	FillRect(x, y + h - 1, w, 1, col);
	FillRect(x, y, 1, h, col);
	FillRect(x + w - 1, y, 1, h, col);
}

// Perceived brightness of a packed colour, 0..1.
float Lum(int col)
{
	return (0.299 * ((col >> 16) & 255) + 0.587 * ((col >> 8) & 255) + 0.114 * (col & 255)) / 255;
}

// Palette swatch; the transparent entry shows as a checkerboard.
void Swatch(float x, float y, float w, float h, int col)
{
	if (col == 0)
	{
		FillRect(x, y, w, h, 0xffffffff);
		float q = w / 4;
		for (int j = 0; j < 4; j++)
			for (int i = 0; i < 4; i++)
				if ((i + j) % 2 != 0)
					FillRect(x + i * q, y + j * h / 4, q, h / 4, 0xffc8c8c8);
	}
	else
		FillRect(x, y, w, h, palette[col]);
}

/* ---------- editor state ----------------------------------------------- */

int tool = TOOL_DRAW;
int curMode = FILL_INSIDE;
int curFill = 2;
int curLine = 1;
int showArrows = 0;
int showGrid = 1;

int mouseX = 0;
int mouseY = 0;

#define DRAG_NONE   0
#define DRAG_CREATE 1
#define DRAG_MOVE   2
#define DRAG_RECT   3
#define DRAG_END    4     // edit tool: moving one end of a primitive
#define DRAG_PAN    5     // middle button: panning the view

int drag = DRAG_NONE;
int dragAX = 0;          // grid point where the drag started
int dragAY = 0;
float dragFX = 0;        // exact (unsnapped) grid position of the press
float dragFY = 0;
int moveDX = 0;          // offset already applied during a move drag
int moveDY = 0;
int moveCopied = 0;
int panX = 0;            // last mouse position while panning
int panY = 0;

Prim cand;               // primitive being drawn
int candOk = 0;

char fileName[256];

float GX(float sx) { return (sx - viewOX) / viewScale; }
float GY(float sy) { return (sy - viewOY) / viewScale; }

// Snaps a stroke from grid point (fx,fy) towards the exact grid position
// (mgx,mgy): within 22.5 degrees of an axis it is a line, otherwise a
// diagonal (sector). Writes the far end and returns the primitive type, or
// -1 when the stroke is too short.
int SnapStroke(int fx, int fy, float mgx, float mgy, int& ex, int& ey)
{
	float vx = mgx - fx;
	float vy = mgy - fy;
	float adx = vx < 0 ? -vx : vx;
	float ady = vy < 0 ? -vy : vy;
	float big = adx > ady ? adx : ady;
	float small = adx > ady ? ady : adx;
	if (big < 0.5)
		return -1;
	int sx = vx < 0 ? -1 : 1;
	int sy = vy < 0 ? -1 : 1;
	if (small < 0.4142 * big)
	{
		int len = IRound(big);
		if (len == 0)
			return -1;
		ex = fx;
		ey = fy;
		if (adx > ady)
			ex = fx + sx * len;
		else
			ey = fy + sy * len;
		return PRIM_LINE;
	}
	int r = IRound((adx + ady) / 2);
	if (r == 0)
		return -1;
	ex = fx + sx * r;
	ey = fy + sy * r;
	return PRIM_SECTOR;
}

// Turns the drag from grid point A to the mouse into the candidate primitive.
int MakeCandidate(float mgx, float mgy)
{
	int ex;
	int ey;
	int type = SnapStroke(dragAX, dragAY, mgx, mgy, ex, ey);
	if (type < 0)
		return 0;
	SetPrim(cand, type, dragAX, dragAY, ex, ey, curMode, curFill, curLine);
	return 1;
}

/* ---------- edit tool: dragging primitive ends ------------------------- */

#define HANDLEPX 7       // pick radius of an end handle, in pixels

int editIdx = -1;        // primitive whose end is being dragged
int editEnd = 0;         // 0 = A, 1 = B
Prim editOrig;           // that primitive before the drag

// Finds the end handle under screen point (x,y). Selected primitives win
// over the others where ends coincide, then the topmost one; returns the
// primitive index (or -1) and the end in 'end'.
int HitHandle(float x, float y, int& end)
{
	for (int pass = 0; pass < 2; pass++)
	{
		for (int i = nPrim - 1; i >= 0; i--)
		{
			Prim& p = prims[i];
			if ((pass == 0) != (p.sel != 0))
				continue;
			float da = Dist(x, y, SX(p.ax), SY(p.ay));
			float db = Dist(x, y, SX(p.bx), SY(p.by));
			if (da <= HANDLEPX || db <= HANDLEPX)
			{
				end = db < da ? 1 : 0;
				return i;
			}
		}
	}
	return -1;
}

// Re-snaps the dragged primitive: the other end stays fixed, the dragged one
// follows the mouse, and the stroke keeps its direction. A placement whose
// body would leave the grid is skipped (the last valid one stays).
void EditDragTo(float mgx, float mgy)
{
	int fx = editEnd == 0 ? editOrig.bx : editOrig.ax;
	int fy = editEnd == 0 ? editOrig.by : editOrig.ay;
	int ex;
	int ey;
	int type = SnapStroke(fx, fy, mgx, mgy, ex, ey);
	if (type < 0)
		return;
	Prim t = editOrig;
	t.type = type;
	if (editEnd == 0)
	{
		t.ax = ex;
		t.ay = ey;
	}
	else
	{
		t.bx = ex;
		t.by = ey;
	}
	if (PrimInGrid(t))
		prims[editIdx] = t;
}

/* ---------- SVG export / load ------------------------------------------ */

#define SVGU 10        // SVG units per grid step

char* hexDigits = "0123456789abcdef";

void HexColor(char* s, int col)
{
	s[0] = '#';
	for (int i = 0; i < 6; i++)
		s[1 + i] = hexDigits[(col >> (20 - 4 * i)) & 15];
	s[7] = 0;
}

void SvgPathD(FILE* f, Prim& p, int body)
{
	char s[256];
	int r = IAbs(p.bx - p.ax) * SVGU;
	if (p.type == PRIM_LINE)
	{
		int nx = -(p.by - p.ay);
		int ny = p.bx - p.ax;
		snprintf(s, 256, "M %d %d L %d %d", p.ax * SVGU, p.ay * SVGU, p.bx * SVGU, p.by * SVGU);
		fputs(s, f);
		if (body)
		{
			snprintf(s, 256, " L %d %d L %d %d Z", (p.bx + nx) * SVGU, (p.by + ny) * SVGU,
				(p.ax + nx) * SVGU, (p.ay + ny) * SVGU);
			fputs(s, f);
		}
		return;
	}
	int cx;
	int cy;
	int dx;
	int dy;
	SectorCD(p, cx, cy, dx, dy);
	// arc A->B turns clockwise on screen, which is sweep-flag 1 in SVG
	if (body && p.mode == FILL_INSIDE)
	{
		snprintf(s, 256, "M %d %d L %d %d ", cx * SVGU, cy * SVGU, p.ax * SVGU, p.ay * SVGU);
		fputs(s, f);
	}
	else
	{
		snprintf(s, 256, "M %d %d ", p.ax * SVGU, p.ay * SVGU);
		fputs(s, f);
	}
	snprintf(s, 256, "A %d %d 0 0 1 %d %d", r, r, p.bx * SVGU, p.by * SVGU);
	fputs(s, f);
	if (body == 0)
		return;
	if (p.mode == FILL_OUTSIDE)
	{
		snprintf(s, 256, " L %d %d", dx * SVGU, dy * SVGU);
		fputs(s, f);
	}
	else if (p.mode == FILL_LENS)
	{
		// the mirrored arc around D, from B back to A, is clockwise as well
		snprintf(s, 256, " A %d %d 0 0 1 %d %d", r, r, p.ax * SVGU, p.ay * SVGU);
		fputs(s, f);
	}
	fputs(" Z", f);
}

int ExportSVG(char* name)
{
	FILE* f = fopen(name, "wb");
	if (f == 0)
		return 0;
	char s[256];
	char hx[8];
	int size = (GRID - 1) * SVGU;
	fputs("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n", f);
	snprintf(s, 256, "<svg viewBox=\"0 0 %d %d\" width=\"%d\" height=\"%d\"", size, size, size, size);
	fputs(s, f);
	fputs(" xmlns=\"http://www.w3.org/2000/svg\">\n", f);

	// the internal representation, read back by Load
	fputs("<!--\nSECTORLY 1\n", f);
	snprintf(s, 256, "grid %d\n", GRID);
	fputs(s, f);
	fputs("palette none", f);
	for (int i = 1; i < 16; i++)
	{
		HexColor(hx, palette[i]);
		fputs(" ", f);
		fputs(hx, f);
	}
	fputs("\n# L ax ay bx by fill outline\n# S ax ay bx by mode(0 inside,1 outside,2 lens) fill outline\n", f);
	for (int i = 0; i < nPrim; i++)
	{
		Prim& p = prims[i];
		if (p.type == PRIM_LINE)
			snprintf(s, 256, "L %d %d %d %d %d %d\n", p.ax, p.ay, p.bx, p.by, p.fill, p.line);
		else
			snprintf(s, 256, "S %d %d %d %d %d %d %d\n", p.ax, p.ay, p.bx, p.by, p.mode, p.fill, p.line);
		fputs(s, f);
	}
	fputs("end\n-->\n", f);

	snprintf(s, 256, "<rect id=\"background\" x=\"0\" y=\"0\" width=\"%d\" height=\"%d\" fill=\"#9a9a9a\"/>\n", size, size);
	fputs(s, f);
	for (int i = 0; i < nPrim; i++)
	{
		Prim& p = prims[i];
		if (p.fill != 0)
		{
			HexColor(hx, palette[p.fill]);
			fputs("<path d=\"", f);
			SvgPathD(f, p, 1);
			snprintf(s, 256, "\" fill=\"%s\"/>\n", hx);
			fputs(s, f);
		}
		if (p.line != 0)
		{
			HexColor(hx, palette[p.line]);
			fputs("<path d=\"", f);
			SvgPathD(f, p, 0);
			snprintf(s, 256, "\" fill=\"none\" stroke=\"%s\" stroke-width=\"1.25\" stroke-linecap=\"round\"/>\n", hx);
			fputs(s, f);
		}
	}
	fputs("</svg>\n", f);
	fclose(f);
	return 1;
}

// Reads up to maxn non-negative integers from s starting at index 1
// (index 0 is the record letter).
int ParseInts(char* s, int* v, int maxn)
{
	int n = 0;
	int i = 1;
	while (s[i] != 0 && n < maxn)
	{
		if (s[i] >= '0' && s[i] <= '9')
		{
			int x = 0;
			while (s[i] >= '0' && s[i] <= '9')
			{
				x = x * 10 + (s[i] - '0');
				i++;
			}
			v[n] = x;
			n++;
		}
		else
			i++;
	}
	return n;
}

int StartsWith(char* s, char* pre)
{
	int i = 0;
	while (pre[i] != 0)
	{
		if (s[i] != pre[i])
			return 0;
		i++;
	}
	return 1;
}

int HexDigit(int c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

// Reads "palette none #rrggbb ..." into pal[1..15]; entry 0 stays
// transparent. Returns the number of colours read.
int ParsePalette(char* s, int* pal)
{
	int k = 1;
	int i = 0;
	while (s[i] != 0 && k < 16)
	{
		if (s[i] == '#')
		{
			int v = 0;
			int ok = 1;
			for (int d = 1; d <= 6; d++)
			{
				int h = HexDigit(s[i + d]);
				if (h < 0)
					ok = 0;
				v = v * 16 + h;
			}
			if (ok == 0)
				break;
			pal[k] = 0xff000000 | v;
			k++;
			i = i + 7;
		}
		else
			i++;
	}
	return k - 1;
}

int LoadSVG(char* name)
{
	FILE* f = fopen(name, "rb");
	if (f == 0)
		return 0;
	char s[512];
	int v[8];
	int found = 0;
	int n = 0;
	int pal[16];             // the file's palette; defaults if it has none
	InitPalette();
	for (int k = 0; k < 16; k++)
		pal[k] = palette[k];
	while (fgets(s, 512, f))
	{
		if (found == 0)
		{
			if (StartsWith(s, "SECTORLY"))
				found = 1;
			continue;
		}
		if (StartsWith(s, "end") || StartsWith(s, "-->"))
			break;
		if (StartsWith(s, "palette"))
			ParsePalette(s, pal);
		if (s[0] == 'L' && ParseInts(s, v, 6) == 6 && n < MAXP)
		{
			SetPrim(prims[n], PRIM_LINE, v[0], v[1], v[2], v[3], 0, v[4] & 15, v[5] & 15);
			n++;
		}
		if (s[0] == 'S' && ParseInts(s, v, 7) == 7 && n < MAXP)
		{
			SetPrim(prims[n], PRIM_SECTOR, v[0], v[1], v[2], v[3], v[4] % 3, v[5] & 15, v[6] & 15);
			n++;
		}
	}
	fclose(f);
	if (found == 0)
		return 0;
	nPrim = n;
	for (int k = 0; k < 16; k++)
		palette[k] = pal[k];
	return 1;
}

/* ---------- starter picture -------------------------------------------- */

void DemoScene()
{
	nPrim = 0;
	int W = 2;   // white fill
	int K = 1;   // black outline

	// Row 1: lines in the four directions E, S, W, N (square on the right),
	// and a line with transparent fill.
	AddPrim(PRIM_LINE, 2, 2, 8, 2, 0, W, K);
	AddPrim(PRIM_LINE, 18, 2, 18, 8, 0, W, K);
	AddPrim(PRIM_LINE, 34, 8, 28, 8, 0, W, K);
	AddPrim(PRIM_LINE, 38, 8, 38, 2, 0, W, K);
	AddPrim(PRIM_LINE, 48, 5, 56, 5, 0, 0, K);

	// Rows 2-4: the four sector orientations (chord SE, SW, NW, NE) with
	// inside, outside and lens bodies; and an arc with transparent fill.
	for (int m = 0; m < 3; m++)
	{
		int y = 14 + m * 10;
		AddPrim(PRIM_SECTOR, 2, y, 8, y + 6, m, W, K);
		AddPrim(PRIM_SECTOR, 24, y, 18, y + 6, m, W, K);
		AddPrim(PRIM_SECTOR, 34, y + 6, 28, y, m, W, K);
		AddPrim(PRIM_SECTOR, 38, y + 6, 44, y, m, W, K);
	}
	AddPrim(PRIM_SECTOR, 50, 16, 56, 22, FILL_INSIDE, 0, K);

	// Row 5: a disc from four inside sectors and a flower from four lenses.
	AddPrim(PRIM_SECTOR, 8, 46, 14, 52, FILL_INSIDE, W, K);
	AddPrim(PRIM_SECTOR, 14, 52, 8, 58, FILL_INSIDE, W, K);
	AddPrim(PRIM_SECTOR, 8, 58, 2, 52, FILL_INSIDE, W, K);
	AddPrim(PRIM_SECTOR, 2, 52, 8, 46, FILL_INSIDE, W, K);

	AddPrim(PRIM_SECTOR, 26, 52, 32, 46, FILL_LENS, W, K);
	AddPrim(PRIM_SECTOR, 26, 52, 32, 58, FILL_LENS, W, K);
	AddPrim(PRIM_SECTOR, 26, 52, 20, 58, FILL_LENS, W, K);
	AddPrim(PRIM_SECTOR, 26, 52, 20, 46, FILL_LENS, W, K);
}

/* ---------- tool panel (immediate mode: one function draws and hit-tests) */

#define UI_DRAW 0
#define UI_HIT  1

int uiPass = UI_DRAW;
int uiX = 0;
int uiY = 0;
int uiHit = -1;

#define BTN_TOOL   300   // + tool
#define BTN_MODE   200   // + sector mode
#define BTN_SWATCH 100   // + palette index
#define BTN_ROTL   400
#define BTN_ROTR   401
#define BTN_DUP    402
#define BTN_DEL    403
#define BTN_UNDO   404
#define BTN_REDO   405
#define BTN_ALL    406
#define BTN_NONE   407
#define BTN_EXPORT 408
#define BTN_LOAD   409
#define BTN_NEW    410
#define BTN_DEMO   411
#define BTN_UP     412
#define BTN_DOWN   413
#define BTN_SCALE  414

int InRect(int x, int y, float rx, float ry, float rw, float rh)
{
	return x >= rx && y >= ry && x < rx + rw && y < ry + rh;
}

int Button(int id, float x, float y, float w, float h, char* label, int active)
{
	if (uiPass == UI_HIT)
	{
		if (InRect(uiX, uiY, x, y, w, h))
			uiHit = id;
		return 0;
	}
	int hover = InRect(mouseX, mouseY, x, y, w, h);
	int bg = active ? 0xff3a6ea5 : (hover ? 0xff5a5a5a : 0xff464646);
	FillRect(x, y, w, h, bg);
	Frame(x, y, w, h, 0xff222222);
	if (label[0] != 0)
	{
		// shrink the label if it would not fit with a small margin
		float sc = 1.6;
		float fit = (w - 10) / TextW(label, 1);
		if (fit < sc)
			sc = fit;
		Text(label, x + (w - TextW(label, sc)) / 2, y + (h + 7 * sc) / 2, sc, 0xffeeeeee);
	}
	return 0;
}

// Miniature of a sector body for the mode buttons.
void ModeIcon(float x, float y, int mode)
{
	if (uiPass == UI_HIT)
		return;
	Prim p;
	// a 3x3-cell sector drawn in panel coordinates via a temporary origin
	SetPrim(p, PRIM_SECTOR, 0, 0, 3, 3, mode, 2, 1);
	int cx;
	int cy;
	int dx;
	int dy;
	SectorCD(p, cx, cy, dx, dy);
	float s = 9;
	g.clear();
	if (mode == FILL_INSIDE)
	{
		g.M(x + cx * s, y + cy * s);
		for (int i = 0; i <= ARCSEG; i++)
		{
			float t = (M_PI / 2) * i / ARCSEG;
			g.L(x + (cx + (p.ax - cx) * cos(t) + (p.bx - cx) * sin(t)) * s,
				y + (cy + (p.ay - cy) * cos(t) + (p.by - cy) * sin(t)) * s);
		}
	}
	else
	{
		for (int i = 0; i <= ARCSEG; i++)
		{
			float t = (M_PI / 2) * i / ARCSEG;
			float px = x + (cx + (p.ax - cx) * cos(t) + (p.bx - cx) * sin(t)) * s;
			float py = y + (cy + (p.ay - cy) * cos(t) + (p.by - cy) * sin(t)) * s;
			if (i == 0)
				g.M(px, py);
			else
				g.L(px, py);
		}
		if (mode == FILL_OUTSIDE)
			g.L(x + dx * s, y + dy * s);
		else
			for (int i = 0; i <= ARCSEG; i++)
			{
				float t = (M_PI / 2) * i / ARCSEG;
				g.L(x + (dx + (p.bx - dx) * cos(t) + (p.ax - dx) * sin(t)) * s,
					y + (dy + (p.by - dy) * cos(t) + (p.ay - dy) * sin(t)) * s);
			}
	}
	g.close();
	g.fin();
	g.rgba32(0xffffffff);
	g.fill1();
	g.clear();
	for (int i = 0; i <= ARCSEG; i++)
	{
		float t = (M_PI / 2) * i / ARCSEG;
		float px = x + (cx + (p.ax - cx) * cos(t) + (p.bx - cx) * sin(t)) * s;
		float py = y + (cy + (p.ay - cy) * cos(t) + (p.by - cy) * sin(t)) * s;
		if (i == 0)
			g.M(px, py);
		else
			g.L(px, py);
	}
	g.fin();
	g.rgba32(0xff000000);
	Width(1.2, 1.2);
	g.stroke();
}

void Label(char* s, float x, float y)
{
	if (uiPass == UI_DRAW)
		Text(s, x, y, 1.3, 0xffb8b8b8);
}

void Panel()
{
	float x = panelX + 12;
	float y = 14;
	if (uiPass == UI_DRAW)
	{
		FillRect(panelX, 0, scrW - panelX, scrH, 0xff2e2e2e);
		Text("SECTORLY", x, y + 20, 3, 0xfff0f0f0);
	}
	Button(BTN_SCALE, x + 152, y, 34, 24, "x2", SC == 2);
	y = y + 30;

	Button(BTN_TOOL + TOOL_DRAW, x, y, 60, 24, "Draw", tool == TOOL_DRAW);
	Button(BTN_TOOL + TOOL_SELECT, x + 63, y, 60, 24, "Select", tool == TOOL_SELECT);
	Button(BTN_TOOL + TOOL_EDIT, x + 126, y, 60, 24, "Edit", tool == TOOL_EDIT);
	y = y + 40;

	Label("Sector fill", x, y);
	y = y + 6;
	for (int m = 0; m < 3; m++)
	{
		Button(BTN_MODE + m, x + m * 64, y, 58, 40, "", curMode == m);
		ModeIcon(x + m * 64 + 15, y + 6, m);
	}
	y = y + 56;

	Label("Palette  L:fill  R:line", x, y);
	y = y + 6;
	for (int k = 0; k < 16; k++)
	{
		float sx = x + (k % 4) * 48;
		float sy = y + (k / 4) * 40;
		if (uiPass == UI_HIT)
		{
			if (InRect(uiX, uiY, sx, sy, 42, 36))
				uiHit = BTN_SWATCH + k;
		}
		else
		{
			int ink = 0xff000000;
			if (Lum(palette[k]) < 0.5 && k != 0)
				ink = 0xffffffff;
			Swatch(sx, sy, 42, 36, k);
			Frame(sx, sy, 42, 36, 0xff1a1a1a);
			if (k == curFill)
				Text("F", sx + 4, sy + 13, 1.5, ink);
			if (k == curLine)
				Text("O", sx + 28, sy + 32, 1.5, ink);
		}
	}
	y = y + 4 * 40 + 4;

	if (uiPass == UI_DRAW)
	{
		// current style preview: fill swatch with the outline colour on top
		Text("Current", x, y + 16, 1.3, 0xffb8b8b8);
		Swatch(x + 70, y, 116, 24, curFill);
		if (curLine != 0)
			FillRect(x + 70, y, 116, 3, palette[curLine]);
		Frame(x + 70, y, 116, 24, 0xff1a1a1a);
	}
	y = y + 34;

	Button(BTN_ROTL, x, y, 90, 24, "Rot ccw", 0);
	Button(BTN_ROTR, x + 96, y, 90, 24, "Rot cw", 0);
	y = y + 28;
	Button(BTN_UP, x, y, 90, 24, "Order up", 0);
	Button(BTN_DOWN, x + 96, y, 90, 24, "Order dn", 0);
	y = y + 28;
	Button(BTN_DUP, x, y, 90, 24, "Duplicate", 0);
	Button(BTN_DEL, x + 96, y, 90, 24, "Delete", 0);
	y = y + 28;
	Button(BTN_UNDO, x, y, 90, 24, "Undo", 0);
	Button(BTN_REDO, x + 96, y, 90, 24, "Redo", 0);
	y = y + 28;
	Button(BTN_ALL, x, y, 90, 24, "Sel all", 0);
	Button(BTN_NONE, x + 96, y, 90, 24, "Sel none", 0);
	y = y + 28;
	Button(BTN_EXPORT, x, y, 90, 24, "Export", 0);
	Button(BTN_LOAD, x + 96, y, 90, 24, "Load", 0);
	y = y + 28;
	Button(BTN_NEW, x, y, 90, 24, "New", 0);
	Button(BTN_DEMO, x + 96, y, 90, 24, "Demo", 0);
	y = y + 40;

	if (uiPass == UI_DRAW)
	{
		// key help only when the window is tall enough to keep it clear of
		// the status lines
		if (y + 84 < scrH - 40)
		{
			Label("D/S/E tool  1 2 3 mode", x, y);
			Label("Arrows move  R/Sh+R rot", x, y + 14);
			Label("PgUp/PgDn draw order", x, y + 28);
			Label("Ctrl+drag copy  Del", x, y + 42);
			Label("^C ^V ^D ^Z ^Y ^A Esc", x, y + 56);
			Label("^S export  ^O load", x, y + 70);
			Label("Wheel zoom  Mid pan  Home", x, y + 84);
		}

		char s[64];
		snprintf(s, 64, "%d prims, %d selected", nPrim, CountSelected());
		Text(s, x, scrH - 24, 1.3, 0xffd0d0d0);
		Text(msg, x, scrH - 8, 1.3, 0xfff0d070);
	}
}

/* ---------- colour dialog (double click on a swatch) ------------------- */
// Six linked sliders, R G B and H S L, all kept as 0..1 floats. Editing one
// family recomputes the other; the palette entry updates live so the picture
// previews the change, and Cancel puts the old colour back.
// A radio button picks one channel; the big square then spans the other two
// channels of its family (B selected -> R x G, S selected -> H x L, ...), and
// dragging in it sets both at once.

int dlgOpen = 0;
int dlgIdx = 0;          // palette entry being edited
int dlgOld = 0;          // its colour before the dialog
int dlgDrag = -1;        // slider being dragged (0..5), DRAG_AREA, or -1
int dlgSel = 3;          // channel chosen by the radio buttons (H)
float dlgV[6];           // r g b h s l

#define DLG_AREA 10

int dlgX = 115;          // dialog position, centred over the canvas
int dlgY = 140;
#define DLGX dlgX
#define DLGY dlgY
#define DLGW 560
#define DLGH 380
#define AREAX (DLGX + 16)    // the 2D square: AREAN x AREAN cells of AREAC px
#define AREAY (DLGY + 104)
#define AREAN 40
#define AREAC 5
#define AREAS 200
#define RADX (DLGX + 242)    // radio button column
#define SLX  (DLGX + 290)    // slider track: 50 steps of 4 px
#define SLW  200
#define SLY0 (DLGY + 108)
#define SLDY 30
#define BTN_OK     500
#define BTN_CANCEL 501

float Clamp01(float v)
{
	if (v < 0)
		return 0;
	if (v > 1)
		return 1;
	return v;
}

float HueToRgb(float p, float q, float t)
{
	if (t < 0)
		t = t + 1;
	if (t > 1)
		t = t - 1;
	if (t < 1 / 6.0)
		return p + (q - p) * 6 * t;
	if (t < 0.5)
		return q;
	if (t < 2 / 3.0)
		return p + (q - p) * (2 / 3.0 - t) * 6;
	return p;
}

void HslToRgb(float h, float s, float l, float& r, float& g, float& b)
{
	if (s <= 0)
	{
		r = l;
		g = l;
		b = l;
		return;
	}
	float q = l + s - l * s;
	if (l < 0.5)
		q = l * (1 + s);
	float p = 2 * l - q;
	r = HueToRgb(p, q, h + 1 / 3.0);
	g = HueToRgb(p, q, h);
	b = HueToRgb(p, q, h - 1 / 3.0);
}

// RGB -> HSL; a grey keeps the previous hue, a black or white the saturation
void RgbToHsl(float r, float g, float b, float& h, float& s, float& l)
{
	float mx = r;
	if (g > mx) mx = g;
	if (b > mx) mx = b;
	float mn = r;
	if (g < mn) mn = g;
	if (b < mn) mn = b;
	l = (mx + mn) / 2;
	float d = mx - mn;
	if (d < 0.000001)
		return;
	if (l > 0.5)
		s = d / (2 - mx - mn);
	else
		s = d / (mx + mn);
	if (mx == r)
	{
		h = (g - b) / d;
		if (g < b)
			h = h + 6;
	}
	else if (mx == g)
		h = (b - r) / d + 2;
	else
		h = (r - g) / d + 4;
	h = h / 6;
}

int PackRgb(float r, float g, float b)
{
	int R = int(Clamp01(r) * 255 + 0.5);
	int G = int(Clamp01(g) * 255 + 0.5);
	int B = int(Clamp01(b) * 255 + 0.5);
	return 0xff000000 | (R << 16) | (G << 8) | B;
}

void OpenColorDialog(int idx)
{
	dlgOpen = 1;
	dlgIdx = idx;
	dlgOld = palette[idx];
	dlgDrag = -1;
	dlgV[0] = ((dlgOld >> 16) & 255) / 255.0;
	dlgV[1] = ((dlgOld >> 8) & 255) / 255.0;
	dlgV[2] = (dlgOld & 255) / 255.0;
	dlgV[3] = 0;
	dlgV[4] = 0;
	RgbToHsl(dlgV[0], dlgV[1], dlgV[2], dlgV[3], dlgV[4], dlgV[5]);
}

// Recomputes the other family after channels of family 'hsl' changed.
void DlgSync(int hsl)
{
	if (hsl == 0)
		RgbToHsl(dlgV[0], dlgV[1], dlgV[2], dlgV[3], dlgV[4], dlgV[5]);
	else
		HslToRgb(dlgV[3], dlgV[4], dlgV[5], dlgV[0], dlgV[1], dlgV[2]);
	palette[dlgIdx] = PackRgb(dlgV[0], dlgV[1], dlgV[2]);
}

void SetSlider(int k, float v)
{
	dlgV[k] = Clamp01(v);
	DlgSync(k >= 3);
}

// The square's horizontal and vertical channels for radio selection 'sel':
// the other two channels of the same family, in order.
void AreaAxes(int sel, int& ax, int& ay)
{
	int base = 0;
	if (sel >= 3)
		base = 3;
	int k = sel - base;
	ax = base + 1;
	if (k != 0)
		ax = base;
	ay = base + 2;
	if (k == 2)
		ay = base + 1;
}

// Sets the square's two channels from a screen point inside (or near) it;
// up is the larger value.
void SetArea(int x, int y)
{
	int ax = 0;
	int ay = 0;
	AreaAxes(dlgSel, ax, ay);
	dlgV[ax] = Clamp01((x - AREAX) / (AREAS * 1.0));
	dlgV[ay] = Clamp01(1 - (y - AREAY) / (AREAS * 1.0));
	DlgSync(dlgSel >= 3);
}

void CloseColorDialog(int ok)
{
	if (ok)
	{
		if (palette[dlgIdx] != dlgOld)
			Snapshot();
	}
	else
		palette[dlgIdx] = dlgOld;
	dlgOpen = 0;
	dlgDrag = -1;
}

// The current colour with channel k1 set to t1 and k2 to t2 (k2 < 0: only
// k1); both channels from the same family.
int ColorWith(int k1, float t1, int k2, float t2)
{
	float v[6];
	for (int i = 0; i < 6; i++)
		v[i] = dlgV[i];
	v[k1] = t1;
	if (k2 >= 0)
		v[k2] = t2;
	if (k1 >= 3)
		HslToRgb(v[3], v[4], v[5], v[0], v[1], v[2]);
	return PackRgb(v[0], v[1], v[2]);
}

float SliderY(int k)
{
	float y = SLY0 + k * SLDY;
	if (k >= 3)
		y = y + 8;
	return y;
}

void ColorDialog()
{
	if (uiPass == UI_DRAW)
	{
		FillRect(0, 0, scrW, scrH, 0x60000000);
		FillRect(DLGX, DLGY, DLGW, DLGH, 0xff383838);
		Frame(DLGX, DLGY, DLGW, DLGH, 0xff101010);
		char s[64];
		snprintf(s, 64, "Colour %d", dlgIdx);
		Text(s, DLGX + 16, DLGY + 28, 2, 0xfff0f0f0);

		// old | new preview and the hex value
		FillRect(DLGX + 16, DLGY + 44, 70, 40, dlgOld);
		FillRect(DLGX + 86, DLGY + 44, 70, 40, palette[dlgIdx]);
		Frame(DLGX + 16, DLGY + 44, 140, 40, 0xff101010);
		Text("old", DLGX + 20, DLGY + 96, 1.2, 0xffb8b8b8);
		Text("new", DLGX + 90, DLGY + 96, 1.2, 0xffb8b8b8);
		char hx[8];
		HexColor(hx, palette[dlgIdx]);
		Text(hx, DLGX + 180, DLGY + 72, 2, 0xfff0f0f0);

		// the 2D square over the two channels the radio selection leaves free
		int ax = 0;
		int ay = 0;
		AreaAxes(dlgSel, ax, ay);
		for (int j = 0; j < AREAN; j++)
		{
			float ty = 1 - (j + 0.5) / AREAN;
			for (int i = 0; i < AREAN; i++)
			{
				int c = ColorWith(ax, (i + 0.5) / AREAN, ay, ty);
				FillRect(AREAX + i * AREAC, AREAY + j * AREAC, AREAC, AREAC, c);
			}
		}
		Frame(AREAX - 1, AREAY - 1, AREAS + 2, AREAS + 2, 0xff101010);
		float mx = AREAX + dlgV[ax] * AREAS;
		float my = AREAY + (1 - dlgV[ay]) * AREAS;
		Ring(mx, my, 6, 1, 1, 0xff000000);
		Ring(mx, my, 4, 0.7, 1, 0xffffffff);
		char* names = "RGBHSL";
		char axl[4];
		axl[0] = names[ax];
		axl[1] = 0;
		Text(axl, AREAX + AREAS - 8, AREAY + AREAS + 14, 1.3, 0xffb8b8b8);
		axl[0] = names[ay];
		Text(axl, AREAX + AREAS + 5, AREAY + 9, 1.3, 0xffb8b8b8);

		for (int k = 0; k < 6; k++)
		{
			float y = SliderY(k);
			// radio button and channel letter
			Ring(RADX + 6, y + 8, 5, 0.6, 1, 0xffd0d0d0);
			if (k == dlgSel)
				Ring(RADX + 6, y + 8, 0, 2.5, 1, 0xfff0d070);
			char lab[2];
			lab[0] = names[k];
			lab[1] = 0;
			int lc = 0xffe0e0e0;
			if (k == dlgSel)
				lc = 0xfff0d070;
			Text(lab, RADX + 20, y + 13, 1.6, lc);
			for (int i = 0; i < 50; i++)
			{
				// integer coordinates: a constant double passed to fillrect
				// silently draws nothing
				int tc = ColorWith(k, (i + 0.5) / 50, -1, 0);
				FillRect(SLX + i * 4, y, 5, 16, tc);
			}
			Frame(SLX, y, SLW, 16, 0xff101010);
			float kx = SLX + dlgV[k] * SLW;
			FillRect(kx - 3, y - 4, 7, 24, 0xff101010);
			FillRect(kx - 2, y - 3, 5, 22, 0xfff8f8f8);
			int val = int(dlgV[k] * 255 + 0.5);
			if (k == 3)
				val = int(dlgV[k] * 360 + 0.5);
			if (k > 3)
				val = int(dlgV[k] * 100 + 0.5);
			snprintf(s, 64, "%d", val);
			Text(s, SLX + SLW + 12, y + 13, 1.5, 0xffe0e0e0);
		}
	}
	Button(BTN_OK, DLGX + DLGW - 200, DLGY + DLGH - 40, 88, 26, "OK", 0);
	Button(BTN_CANCEL, DLGX + DLGW - 104, DLGY + DLGH - 40, 88, 26, "Cancel", 0);
}

int SliderAt(int x, int y)
{
	for (int k = 0; k < 6; k++)
		if (InRect(x, y, SLX - 6, SliderY(k) - 6, SLW + 12, 28))
			return k;
	return -1;
}

int RadioAt(int x, int y)
{
	for (int k = 0; k < 6; k++)
		if (InRect(x, y, RADX - 4, SliderY(k) - 4, 44, 24))
			return k;
	return -1;
}

void DlgDown(int x, int y)
{
	if (InRect(x, y, AREAX, AREAY, AREAS, AREAS))
	{
		dlgDrag = DLG_AREA;
		SetArea(x, y);
		return;
	}
	int r = RadioAt(x, y);
	if (r >= 0)
	{
		dlgSel = r;
		return;
	}
	int k = SliderAt(x, y);
	if (k >= 0)
	{
		dlgDrag = k;
		SetSlider(k, (x - SLX) / (SLW * 1.0));
		return;
	}
	uiPass = UI_HIT;
	uiX = x;
	uiY = y;
	uiHit = -1;
	ColorDialog();
	uiPass = UI_DRAW;
	if (uiHit == BTN_OK)
		CloseColorDialog(1);
	else if (uiHit == BTN_CANCEL)
		CloseColorDialog(0);
}

void DlgMove(int x, int y)
{
	if (dlgDrag == DLG_AREA)
		SetArea(x, y);
	else if (dlgDrag >= 0)
		SetSlider(dlgDrag, (x - SLX) / (SLW * 1.0));
}

void DlgKey(int k)
{
	if (k == KEYCODE_ENTER)
		CloseColorDialog(1);
	else if (k == KEYCODE_ESC)
		CloseColorDialog(0);
}

/* ---------- scene ------------------------------------------------------ */

void DrawCanvas()
{
	// darker surround so the grid's extent shows when zoomed or panned
	g.rgba32(0xff7c7c7c);
	g.FillRT();
	FillRect(SX(0), SY(0), SX(GRID - 1) - SX(0), SY(GRID - 1) - SY(0), 0xff9a9a9a);
	if (showGrid)
	{
		// only the dots inside the visible part of the canvas
		int x0 = IMax(0, int(GX(0)));
		int y0 = IMax(0, int(GY(0)));
		int x1 = IMin(GRID - 1, int(GX(panelX)) + 1);
		int y1 = IMin(GRID - 1, int(GY(scrH)) + 1);
		for (int y = y0; y <= y1; y++)
			for (int x = x0; x <= x1; x++)
				FillRect(SX(x) - 1, SY(y) - 1, 2, 2, 0xff6c6c6c);
	}

	for (int i = 0; i < nPrim; i++)
	{
		Prim& p = prims[i];
		DrawPrim(p, palette[p.fill], palette[p.line]);
	}

	// selection: body outline in blue plus direction arrows
	for (int i = 0; i < nPrim; i++)
	{
		Prim& p = prims[i];
		if (p.sel)
		{
			BodyPath(p);
			g.rgba32(0xff1080ff);
			Width(1.2, 1.2);
			g.stroke();
			StrokePath(p);
			g.rgba32(0xff1080ff);
			Width(2.2, 2.2);
			g.stroke();
			DrawArrow(p, 0xff1080ff);
		}
		else if (showArrows)
			DrawArrow(p, 0xffe02020);
	}

	// hover hint in the select tool
	if (tool == TOOL_SELECT && drag == DRAG_NONE && mouseX < panelX)
	{
		int h = HitTop(GX(mouseX), GY(mouseY));
		if (h >= 0)
		{
			BodyPath(prims[h]);
			g.rgba32(0x80ffffff);
			Width(1.5, 1.5);
			g.stroke();
		}
	}

	// edit tool: a handle on every end; selected ones blue, hovered orange
	if (tool == TOOL_EDIT)
	{
		int hEnd = 0;
		int h = editIdx;
		hEnd = editEnd;
		if (drag != DRAG_END)
		{
			h = -1;
			if (mouseX < panelX)
				h = HitHandle(mouseX, mouseY, hEnd);
		}
		for (int i = 0; i < nPrim; i++)
		{
			Prim& p = prims[i];
			int col = p.sel ? 0xff1080ff : 0xffffffff;
			FillRect(SX(p.ax) - 3, SY(p.ay) - 3, 6, 6, 0xff202020);
			FillRect(SX(p.ax) - 2, SY(p.ay) - 2, 4, 4, col);
			FillRect(SX(p.bx) - 3, SY(p.by) - 3, 6, 6, 0xff202020);
			FillRect(SX(p.bx) - 2, SY(p.by) - 2, 4, 4, col);
		}
		if (h >= 0)
		{
			Prim& p = prims[h];
			float hx = SX(p.ax);
			float hy = SY(p.ay);
			if (hEnd)
			{
				hx = SX(p.bx);
				hy = SY(p.by);
			}
			FillRect(hx - 5, hy - 5, 10, 10, 0xff202020);
			FillRect(hx - 4, hy - 4, 8, 8, 0xffff9020);
			DrawArrow(p, 0xffff9020);
		}
	}

	if (drag == DRAG_CREATE && candOk)
	{
		int ok = PrimInGrid(cand);
		DrawPrim(cand, ok ? (palette[cand.fill] & 0xffffff) | 0xb0000000 : 0x80ff4040,
			ok ? palette[cand.line] : 0xffff2020);
		DrawArrow(cand, 0xffe02020);
	}

	if (drag == DRAG_RECT)
	{
		float x0 = SX(dragFX);
		float y0 = SY(dragFY);
		float x1 = mouseX;
		float y1 = mouseY;
		FillRect(x0 < x1 ? x0 : x1, y0 < y1 ? y0 : y1,
			x1 > x0 ? x1 - x0 : x0 - x1, y1 > y0 ? y1 - y0 : y0 - y1, 0x301080ff);
		g.clear();
		g.M(x0, y0);
		g.L(x1, y0);
		g.L(x1, y1);
		g.L(x0, y1);
		g.close();
		g.fin();
		g.rgba32(0xff1080ff);
		Width(1, 1);
		g.stroke();
	}

	// cursor: the snapped grid point
	if (mouseX < panelX)
	{
		int gx = IRound(GX(mouseX));
		int gy = IRound(GY(mouseY));
		if (gx >= 0 && gy >= 0 && gx < GRID && gy < GRID)
			Ring(SX(gx), SY(gy), 4, 0.6, 1, tool == TOOL_DRAW ? 0xffe02020 : (tool == TOOL_EDIT ? 0xffff9020 : 0xff1080ff));
	}
}

void DrawAll()
{
	DrawCanvas();
	uiPass = UI_DRAW;
	Panel();
	if (dlgOpen)
		ColorDialog();
}

/* ---------- input ------------------------------------------------------ */

// Modifier state is tracked from the event queue (KBDN/KBUP of the modifier
// keys) rather than polled with KeyPressed, so it is in sync with the event
// being handled even when a key is tapped faster than a frame.
int shiftState = 0;
int ctrlState = 0;

int ShiftDown() { return shiftState; }
int CtrlDown()  { return ctrlState; }

void TrackModifier(int k, int down)
{
	if (k == KEYCODE_LSHIFT || k == KEYCODE_RSHIFT)
		shiftState = down;
	if (k == KEYCODE_LCTRL || k == KEYCODE_RCTRL)
		ctrlState = down;
}

// Status-line message "<what> <file name without directories>".
void MsgWithFile(char* what)
{
	int start = 0;
	for (int i = 0; fileName[i] != 0; i++)
		if (fileName[i] == '/')
			start = i + 1;
	char base[128];
	int n = 0;
	while (fileName[start + n] != 0 && n < 127)
	{
		base[n] = fileName[start + n];
		n++;
	}
	base[n] = 0;
	char s[160];
	snprintf(s, 160, "%s %s", what, base);
	SetMsg(s);
}

void DoExport()
{
	if (ExportSVG(fileName))
	{
		MsgWithFile("Exported");
	}
	else
		SetMsg("Export failed");
}

void DoLoad()
{
	if (LoadSVG(fileName))
	{
		Snapshot();
		MsgWithFile("Loaded");
	}
	else
		SetMsg("Load failed (no Sectorly data)");
}

int lastSwatch = -1;     // for double-click detection on the palette
double lastSwatchT = 0;

void PanelClick(int x, int y, int button)
{
	uiPass = UI_HIT;
	uiX = x;
	uiY = y;
	uiHit = -1;
	Panel();
	uiPass = UI_DRAW;
	int id = uiHit;
	if (id < 0)
		return;
	if (id >= BTN_SWATCH && id < BTN_SWATCH + 16)
	{
		if (button == 1)
		{
			curFill = id - BTN_SWATCH;
			ApplyFill(curFill);
			double now = Time();
			if (curFill == lastSwatch && now - lastSwatchT < 0.4 && curFill != 0)
			{
				OpenColorDialog(curFill);
				lastSwatch = -1;
			}
			else
			{
				lastSwatch = curFill;
				lastSwatchT = now;
			}
		}
		else
		{
			curLine = id - BTN_SWATCH;
			ApplyLine(curLine);
		}
	}
	else if (id >= BTN_MODE && id < BTN_MODE + 3)
	{
		curMode = id - BTN_MODE;
		ApplyMode(curMode);
	}
	else if (id >= BTN_TOOL && id < BTN_TOOL + 3)
		tool = id - BTN_TOOL;
	else if (id == BTN_ROTL)
		RotateSelected(0);
	else if (id == BTN_ROTR)
		RotateSelected(1);
	else if (id == BTN_DUP)
	{
		if (DuplicateSelected())
		{
			if (TryMoveSelected(1, 1) == 0)
				TryMoveSelected(-1, -1);
			Snapshot();
		}
	}
	else if (id == BTN_UP)
		ReorderSelected(1);
	else if (id == BTN_DOWN)
		ReorderSelected(0);
	else if (id == BTN_DEL)
		DeleteSelected();
	else if (id == BTN_UNDO)
		Undo();
	else if (id == BTN_REDO)
		Redo();
	else if (id == BTN_ALL)
		SelectAll(1);
	else if (id == BTN_NONE)
		SelectAll(0);
	else if (id == BTN_EXPORT)
		DoExport();
	else if (id == BTN_LOAD)
		DoLoad();
	else if (id == BTN_NEW)
	{
		nPrim = 0;
		Snapshot();
	}
	else if (id == BTN_SCALE)
		scaleReq = 1;      // the main loop recreates the window
	else if (id == BTN_DEMO)
	{
		DemoScene();
		Snapshot();
	}
}

void OnDown(int x, int y, int button)
{
	if (x >= panelX)
	{
		PanelClick(x, y, button);
		return;
	}
	float fx = GX(x);
	float fy = GY(y);
	if (button == 3)
	{
		drag = DRAG_PAN;
		panX = x;
		panY = y;
		return;
	}
	if (button == 2)
	{
		// eyedropper
		int h = HitTop(fx, fy);
		if (h >= 0)
		{
			curFill = prims[h].fill;
			curLine = prims[h].line;
			if (prims[h].type == PRIM_SECTOR)
				curMode = prims[h].mode;
			SetMsg("Picked style");
		}
		return;
	}
	dragFX = fx;
	dragFY = fy;
	dragAX = IRound(fx);
	dragAY = IRound(fy);
	if (tool == TOOL_DRAW)
	{
		if (dragAX < 0 || dragAY < 0 || dragAX >= GRID || dragAY >= GRID)
			return;
		drag = DRAG_CREATE;
		candOk = 0;
		return;
	}
	if (tool == TOOL_EDIT)
	{
		int end = 0;
		int e = HitHandle(x, y, end);
		if (e >= 0)
		{
			editIdx = e;
			editEnd = end;
			editOrig = prims[e];
			drag = DRAG_END;
			return;
		}
		// away from the handles the edit tool selects like the select tool
	}
	int h = HitTop(fx, fy);
	if (h < 0)
	{
		drag = DRAG_RECT;
		return;
	}
	if (ShiftDown())
	{
		prims[h].sel = !prims[h].sel;
		return;
	}
	if (prims[h].sel == 0)
	{
		SelectAll(0);
		prims[h].sel = 1;
	}
	drag = DRAG_MOVE;
	moveDX = 0;
	moveDY = 0;
	moveCopied = 0;
	if (CtrlDown())
	{
		DuplicateSelected();
		moveCopied = 1;
	}
}

void OnMove(int x, int y)
{
	mouseX = x;
	mouseY = y;
	if (drag == DRAG_CREATE)
		candOk = MakeCandidate(GX(x), GY(y));
	else if (drag == DRAG_END)
		EditDragTo(GX(x), GY(y));
	else if (drag == DRAG_PAN)
	{
		viewFitted = 0;
		viewOX = viewOX + (x - panX);
		viewOY = viewOY + (y - panY);
		panX = x;
		panY = y;
	}
	else if (drag == DRAG_MOVE)
	{
		int dx = IRound(GX(x) - dragFX);
		int dy = IRound(GY(y) - dragFY);
		if (dx != moveDX || dy != moveDY)
		{
			if (TryMoveSelected(dx - moveDX, dy - moveDY))
			{
				moveDX = dx;
				moveDY = dy;
			}
		}
	}
}

void OnUp(int x, int y)
{
	if (drag == DRAG_CREATE)
	{
		candOk = MakeCandidate(GX(x), GY(y));
		if (candOk)
		{
			if (PrimInGrid(cand))
			{
				if (AddPrim(cand.type, cand.ax, cand.ay, cand.bx, cand.by, cand.mode, cand.fill, cand.line) >= 0)
					Snapshot();
			}
			else
				SetMsg("Body would leave the grid");
		}
	}
	else if (drag == DRAG_MOVE)
	{
		if (moveDX != 0 || moveDY != 0 || moveCopied)
			Snapshot();
	}
	else if (drag == DRAG_END)
	{
		Prim& p = prims[editIdx];
		if (p.type != editOrig.type || p.ax != editOrig.ax || p.ay != editOrig.ay ||
			p.bx != editOrig.bx || p.by != editOrig.by)
			Snapshot();
		editIdx = -1;
	}
	else if (drag == DRAG_RECT)
	{
		float fx = GX(x);
		float fy = GY(y);
		float x0 = dragFX < fx ? dragFX : fx;
		float x1 = dragFX < fx ? fx : dragFX;
		float y0 = dragFY < fy ? dragFY : fy;
		float y1 = dragFY < fy ? fy : dragFY;
		if (ShiftDown() == 0)
			SelectAll(0);
		if (x1 - x0 > 0.2 || y1 - y0 > 0.2)
		{
			for (int i = 0; i < nPrim; i++)
			{
				int a0;
				int b0;
				int a1;
				int b1;
				PrimBox(prims[i], a0, b0, a1, b1);
				if (a0 >= x0 && b0 >= y0 && a1 <= x1 && b1 <= y1)
					prims[i].sel = 1;
			}
		}
	}
	drag = DRAG_NONE;
	candOk = 0;
}

void OnKey(int k)
{
	int ctrl = CtrlDown();
	int shift = ShiftDown();
	if (k >= 'A' && k <= 'Z')
		k = k - 'A' + 'a';
	if (ctrl)
	{
		if (k == 'a')
			SelectAll(1);
		else if (k == 'c')
			CopySelected();
		else if (k == 'v')
		{
			int gx = IRound(GX(mouseX));
			int gy = IRound(GY(mouseY));
			Paste(gx < 0 ? 0 : gx, gy < 0 ? 0 : gy);
		}
		else if (k == 'd')
		{
			if (DuplicateSelected())
			{
				if (TryMoveSelected(1, 1) == 0)
					TryMoveSelected(-1, -1);
				Snapshot();
			}
		}
		else if (k == 'z')
			Undo();
		else if (k == 'y')
			Redo();
		else if (k == 's')
			DoExport();
		else if (k == 'o')
			DoLoad();
		return;
	}
	int step = shift ? 4 : 1;
	if (k == KEYCODE_LEFT || k == KEYCODE_RIGHT || k == KEYCODE_UP || k == KEYCODE_DOWN)
	{
		int dx = 0;
		int dy = 0;
		if (k == KEYCODE_LEFT) dx = -step;
		if (k == KEYCODE_RIGHT) dx = step;
		if (k == KEYCODE_UP) dy = -step;
		if (k == KEYCODE_DOWN) dy = step;
		if (CountSelected() > 0 && TryMoveSelected(dx, dy))
			Snapshot();
	}
	else if (k == KEYCODE_DEL || k == KEYCODE_BACKSPACE)
		DeleteSelected();
	else if (k == KEYCODE_ESC)
	{
		if (drag != DRAG_NONE)
		{
			if (drag == DRAG_MOVE && hCur >= 0)
				Restore(hCur);
			if (drag == DRAG_END)
			{
				prims[editIdx] = editOrig;
				editIdx = -1;
			}
			drag = DRAG_NONE;
		}
		else
			SelectAll(0);
	}
	else if (k == KEYCODE_TAB)
		showArrows = !showArrows;
	else if (k == 'd')
		tool = TOOL_DRAW;
	else if (k == 's')
		tool = TOOL_SELECT;
	else if (k == 'r')
		RotateSelected(shift ? 0 : 1);
	else if (k == 'e')
		tool = TOOL_EDIT;
	else if (k == KEYCODE_PGUP)
		ReorderSelected(1);
	else if (k == KEYCODE_PGDN)
		ReorderSelected(0);
	else if (k == KEYCODE_HOME)
		FitView();
	else if (k == 'g')
		showGrid = !showGrid;
	else if (k >= '1' && k <= '3')
	{
		curMode = k - '1';
		ApplyMode(curMode);
	}
}

/* ---------- window resizing ------------------------------------------- */
// Scinc cannot resize a window's render target, but a window smaller than its
// render target simply shows the top-left part 1:1. So a size change reported
// by the window manager ('WMOV', new physical size in z,h) only re-lays out
// while the window fits the render target; the window is recreated (bigger)
// only when it grows past it. The window manager can then only shrink the
// new window, which needs no recreation, so this cannot loop.
//
// Positions: under a reparenting window manager GetWindowPos returns the
// window's offset inside its frame, 'WMOV' events report either that same
// offset or the real position, and SetWindowPos(x,y) ends up reported as
// (x,y) + offset. So the last real reported position minus the offset puts a
// recreated window back in place.

int rtW = G_SCREEN_WIDTH;        // logical size of the window's render target
int rtH = G_SCREEN_HEIGHT;
int winX = -1;                   // last real position reported by 'WMOV'
int winY = -1;
int resizePending = 0;
int resizeW = 0;
int resizeH = 0;
double resizeT = 0;

void Layout()
{
	panelX = scrW - PANELW;
	dlgX = (panelX - DLGW) / 2;
	if (dlgX < 4)
		dlgX = 4;
	dlgY = (scrH - DLGH) / 2;
	if (dlgY < 4)
		dlgY = 4;
	if (viewFitted)
		FitView();
}

void OnWindowGeometry(int x, int y, int pw, int ph)
{
	int offX = 0;
	int offY = 0;
	wsys.GetWindowPos(mainWin, &offX, &offY);
	if (x != offX || y != offY)
	{
		winX = x;
		winY = y;
	}
	int lw = pw / SC;
	int lh = ph / SC;
	if (lw > rtW || lh > rtH)
	{
		// past the render target: recreate once the size settles
		resizePending = 1;
		resizeW = lw;
		resizeH = lh;
		resizeT = Time();
		return;
	}
	resizePending = 0;
	// smaller than the minimum: keep the minimum layout, the window crops it
	if (lw < MINW)
		lw = MINW;
	if (lh < MINH)
		lh = MINH;
	if (lw > rtW)
		lw = rtW;
	if (lh > rtH)
		lh = rtH;
	if (lw != scrW || lh != scrH)
	{
		scrW = lw;
		scrH = lh;
		Layout();
	}
}

void ApplyResize()
{
	resizePending = 0;
	int lw = resizeW;
	int lh = resizeH;
	if (lw < rtW)
		lw = rtW;
	if (lh < rtH)
		lh = rtH;
	int offX = 0;
	int offY = 0;
	wsys.GetWindowPos(mainWin, &offX, &offY);
	wsys.DeleteWindow(mainWin);
	mainWin = wsys.CreateWindow(lw * SC, lh * SC, 1, 1, G_SCREEN_MODE);
	if (winX >= 0)
		wsys.SetWindowPos(mainWin, winX - offX, winY - offY);
	g.SetActiveRT(wsys.GetWindowRT(mainWin));
	rtW = lw;
	rtH = lh;
	scrW = resizeW;
	scrH = resizeH;
	Layout();
	// a drag or held modifier does not survive the new window
	drag = DRAG_NONE;
	shiftState = 0;
	ctrlState = 0;
}

// x2 button: a window of the same logical size at scale s, in the same place
void SetScale(float s)
{
	if (s != SC)
	{
		SC = s;
		int offX = 0;
		int offY = 0;
		wsys.GetWindowPos(mainWin, &offX, &offY);
		wsys.DeleteWindow(mainWin);
		mainWin = wsys.CreateWindow(scrW * SC, scrH * SC, 1, 1, G_SCREEN_MODE);
		if (winX >= 0)
			wsys.SetWindowPos(mainWin, winX - offX, winY - offY);
		g.SetActiveRT(wsys.GetWindowRT(mainWin));
		rtW = scrW;
		rtH = scrH;
		resizePending = 0;
		Layout();
		drag = DRAG_NONE;
		shiftState = 0;
		ctrlState = 0;
	}
	g.t_0(0, 0);
	g.t_x(SC, 0);
	g.t_y(0, SC);
}

/* ---------- main ------------------------------------------------------- */

int main(int argc, pchar* argv)
{
	// replaces ws.h's callback: the same two-step close, but close events of a
	// window deleted by ApplyResize must not count towards quitting
	SetEventCallback([](SScincEvent e)->int
	{
		if (e.type == 'MLDN')
			quitReq = 0;
		else if (e.type == 'WDEL' || e.type == 'WDST')
		{
			if (e.w != mainWin)
				return 0;
			if (quitReq)
				exit(0);
			quitReq++;
		}
		return 0;
	});

	pfont = (CFont*)malloc(sizeof(CFont));
	CFont& font = *pfont;
	CFontMaker fm;
	fm.Init(font, 6);
	MakeLetters(fm);

	InitPalette();
	FitView();
	hBuf = (int*)malloc(UNDOLEV * MAXP * PSTRIDE * sizeof(int));
	msg[0] = 0;

	char* defName = "sectorly.svg";
	char* src = defName;
	if (argc > 1)
		src = argv[1];
	int i = 0;
	while (src[i] != 0 && i < 255)
	{
		fileName[i] = src[i];
		i++;
	}
	fileName[i] = 0;

	DemoScene();
	if (argc > 1 && StartsWith(argv[1], "-selftest"))
	{
		// non-interactive check: edit, export, reload, render
		Snapshot();
		DrawCanvas();
		uiPass = UI_DRAW;
		Panel();
		g.WriteImage("selftest_0.png", "png", "");
		int before = nPrim;
		prims[0].sel = 1;          // east line
		prims[5].sel = 1;          // first sector
		RotateSelected(1);
		DuplicateSelected();
		TryMoveSelected(0, 20);
		Snapshot();
		ApplyFill(6);
		ApplyLine(12);
		CopySelected();
		Paste(54, 30);
		printf("prims %d -> %d, selected %d\n", before, nPrim, CountSelected());
		DrawAll();
		g.WriteImage("selftest_1.png", "png", "");
		ExportSVG("selftest.svg");
		int n = nPrim;
		Undo();
		printf("after undo %d\n", nPrim);
		Redo();
		nPrim = 0;
		printf("reload %d, prims %d (expected %d)\n", LoadSVG("selftest.svg"), nPrim, n);
		DrawAll();
		g.WriteImage("selftest_2.png", "png", "");

		// draw order: tag the first five by their ax, select a sparse set
		DemoScene();
		SelectAll(0);
		prims[0].sel = 1;
		prims[3].sel = 1;
		ReorderSelected(1);        // gather at 3: expect 18 34 2 38 48
		printf("order up   %d %d %d %d %d\n", prims[0].ax, prims[1].ax, prims[2].ax, prims[3].ax, prims[4].ax);
		ReorderSelected(1);        // solid block swaps with next: 18 34 48 2 38
		printf("order up   %d %d %d %d %d\n", prims[0].ax, prims[1].ax, prims[2].ax, prims[3].ax, prims[4].ax);
		ReorderSelected(0);
		ReorderSelected(0);
		ReorderSelected(0);        // back down to the bottom: 2 38 18 34 48
		printf("order down %d %d %d %d %d\n", prims[0].ax, prims[1].ax, prims[2].ax, prims[3].ax, prims[4].ax);

		// edit: drag B of line 2,2->8,2 to near (8,8): becomes a sector
		DemoScene();
		editIdx = 0;
		editEnd = 1;
		editOrig = prims[0];
		EditDragTo(7.8, 8.3);
		printf("edit B     type %d  %d %d -> %d %d\n", prims[0].type, prims[0].ax, prims[0].ay, prims[0].bx, prims[0].by);
		editEnd = 0;               // drag A of the original line to (2,6)
		EditDragTo(2.2, 5.9);
		printf("edit A     type %d  %d %d -> %d %d\n", prims[0].type, prims[0].ax, prims[0].ay, prims[0].bx, prims[0].by);
		ZoomAt(100, 100, 2.5);
		tool = TOOL_EDIT;
		DrawAll();
		g.WriteImage("selftest_3.png", "png", "");

		// colour dialog: red (6) -> hue 120 = green, then undo / cancel / reload
		FitView();
		tool = TOOL_DRAW;
		DemoScene();
		for (int i = 0; i < 6; i++)
			prims[i].fill = 6;
		Snapshot();
		OpenColorDialog(6);
		printf("dialog hsl  %d %d %d\n", int(dlgV[3] * 360 + 0.5), int(dlgV[4] * 100 + 0.5), int(dlgV[5] * 100 + 0.5));
		SetSlider(3, 120 / 360.0);
		printf("hue 120  -> %08x\n", palette[6]);
		SetSlider(2, 1.0);
		printf("blue 255 -> %08x  (h %d)\n", palette[6], int(dlgV[3] * 360 + 0.5));
		dlgSel = 2;                // B selected: square is R x G
		SetArea(AREAX + AREAS / 4, AREAY + AREAS / 4);
		printf("area R.25 G.75 -> %08x\n", palette[6]);
		DrawAll();
		g.WriteImage("selftest_4.png", "png", "");
		dlgSel = 4;                // S selected: square is H x L
		DrawAll();
		g.WriteImage("selftest_5.png", "png", "");
		CloseColorDialog(1);
		int edited = palette[6];
		printf("edited   -> %08x\n", edited);
		Undo();
		printf("undo     -> %08x   redo -> ", palette[6]);
		Redo();
		printf("%08x\n", palette[6]);
		OpenColorDialog(6);
		SetSlider(0, 0);
		CloseColorDialog(0);
		printf("cancel   -> %08x\n", palette[6]);
		ExportSVG("selftest.svg");
		InitPalette();
		LoadSVG("selftest.svg");
		printf("reload   -> %08x (expected %08x)\n", palette[6], edited);
		return 0;
	}
	if (argc > 1)
	{
		if (LoadSVG(fileName))
			SetMsg("Loaded");
		else
			SetMsg("New file");
	}
	else
		SetMsg("Drag to draw, S to select");
	Snapshot();

	while (true)
	{
		SScincEvent ev;
		while (GetScincEvent(ev))
		{
			if ((ev.type >> 24) == 'M')   // mouse events: window pixels -> logical
			{
				ev.x = ev.x / SC;
				ev.y = ev.y / SC;
			}
			if (ev.type == 'WMOV')
			{
				if (ev.w == mainWin)
					OnWindowGeometry(ev.x, ev.y, ev.z, ev.h);
				continue;
			}
			if (dlgOpen)
			{
				mouseX = ev.x;
				mouseY = ev.y;
				if (ev.type == 'MLDN')
					DlgDown(ev.x, ev.y);
				else if (ev.type == 'MMOV')
					DlgMove(ev.x, ev.y);
				else if (ev.type == 'MLUP')
					dlgDrag = -1;
				else if (ev.type == 'KBDN')
				{
					TrackModifier(ev.x, 1);
					DlgKey(ev.x);
				}
				else if (ev.type == 'KBUP')
					TrackModifier(ev.x, 0);
				continue;
			}
			if (ev.type == 'MMOV')
				OnMove(ev.x, ev.y);
			else if (ev.type == 'MLDN')
			{
				mouseX = ev.x;
				mouseY = ev.y;
				OnDown(ev.x, ev.y, 1);
			}
			else if (ev.type == 'MRDN')
				OnDown(ev.x, ev.y, 2);
			else if (ev.type == 'MMDN')
				OnDown(ev.x, ev.y, 3);
			else if (ev.type == 'MLUP' || ev.type == 'MMUP')
				OnUp(ev.x, ev.y);
			else if ((ev.type == 'MWUP' || ev.type == 'MWDN') && ev.x < panelX)
				ZoomAt(ev.x, ev.y, ev.type == 'MWUP' ? 1.25 : 0.8);
			else if (ev.type == 'KBDN')
			{
				TrackModifier(ev.x, 1);
				OnKey(ev.x);
			}
			else if (ev.type == 'KBUP')
				TrackModifier(ev.x, 0);
		}
		if (resizePending && Time() - resizeT > 0.3)
			ApplyResize();
		if (scaleReq)
		{
			scaleReq = 0;
			SetScale(3 - SC);
		}
		DrawAll();
		Present();
		Wait(0.01);
	}
	return 0;
}
