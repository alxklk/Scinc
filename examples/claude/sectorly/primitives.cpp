#include <stdio.h>

// Sectorly, step 1: a static gallery of every primitive kind.
//
// The editor works on a 64x64 lattice of grid points. Every primitive is a
// directed stroke A->B between two grid points, and its filled body always
// lies to the RIGHT of the stroke direction:
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
//                      (the same arc taken around D)
//
// Colours come from a 16-entry palette; index 0 is transparent. Outline
// colour strokes only the primitive's own edge (the line or the arc).
//
// Run: DISPLAY=:0 Scinc primitives.cpp   (writes primitives.png on start)

#define G_SCREEN_WIDTH  800
#define G_SCREEN_HEIGHT 800
#define G_SCREEN_SCALE  1
#define G_SCREEN_MODE   2

#include "graphics.h"
#include "../../ws.h"

Graph g;

#define M_PI 3.14159265358979

#define GRID   64      // grid points per side
#define CELL   12      // pixels between grid points
#define ORGX   22      // screen position of grid point (0,0)
#define ORGY   22
#define ARCSEG 24      // polyline segments per quarter arc

#define PRIM_LINE   0
#define PRIM_SECTOR 1

#define FILL_INSIDE  0
#define FILL_OUTSIDE 1
#define FILL_LENS    2

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

float SX(float gx) { return ORGX + gx * CELL; }
float SY(float gy) { return ORGY + gy * CELL; }

// Appends the quarter arc around centre (cx,cy) from P to Q to the current
// path. P-C and Q-C are perpendicular and equally long, so
// C + (P-C)cos t + (Q-C)sin t sweeps exactly that arc for t in [0, pi/2].
void ArcPath(float cx, float cy, float px, float py, float qx, float qy, int moveFirst)
{
	for (int i = 0; i <= ARCSEG; i++)
	{
		float t = (M_PI / 2) * i / ARCSEG;
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

void FillPath(int col)
{
	if (col == 0)
		return;
	g.rgba32(palette[col]);
	g.fill1();
}

void StrokePath(int col)
{
	if (col == 0)
		return;
	g.rgba32(palette[col]);
	g.width(1.5, 1.5);
	g.stroke();
}

void DrawLine(int ax, int ay, int bx, int by, int fill, int outline)
{
	int len = bx - ax + by - ay;
	if (len < 0)
		len = -len;
	// right-hand normal on screen (y grows downwards): (dx,dy) -> (-dy,dx)
	int nx = -(by - ay);
	int ny = bx - ax;

	g.clear();
	g.M(SX(ax), SY(ay));
	g.L(SX(bx), SY(by));
	g.L(SX(bx + nx), SY(by + ny));
	g.L(SX(ax + nx), SY(ay + ny));
	g.close();
	g.fin();
	FillPath(fill);

	g.clear();
	g.M(SX(ax), SY(ay));
	g.L(SX(bx), SY(by));
	g.fin();
	StrokePath(outline);
}

void DrawSector(int ax, int ay, int bx, int by, int mode, int fill, int outline)
{
	// half of the right-hand normal of the chord points from its midpoint
	// to the centre C; the opposite corner D is the same step the other way
	float mx = (ax + bx) * 0.5;
	float my = (ay + by) * 0.5;
	float hx = -(by - ay) * 0.5;
	float hy = (bx - ax) * 0.5;
	float cx = mx + hx;
	float cy = my + hy;
	float dx = mx - hx;
	float dy = my - hy;

	g.clear();
	if (mode == FILL_INSIDE)
	{
		g.M(SX(cx), SY(cy));
		ArcPath(cx, cy, ax, ay, bx, by, 0);
	}
	else if (mode == FILL_OUTSIDE)
	{
		ArcPath(cx, cy, ax, ay, bx, by, 1);
		g.L(SX(dx), SY(dy));
	}
	else
	{
		ArcPath(cx, cy, ax, ay, bx, by, 1);
		ArcPath(dx, dy, bx, by, ax, ay, 0);
	}
	g.close();
	g.fin();
	FillPath(fill);

	g.clear();
	ArcPath(cx, cy, ax, ay, bx, by, 1);
	g.fin();
	StrokePath(outline);
}

// Small red arrowhead at B pointing along the stroke direction, so the
// "body on the right" convention can be checked visually.
void DrawArrow(float ax, float ay, float bx, float by, int isArc)
{
	float ux = bx - ax;
	float uy = by - ay;
	if (isArc)
	{
		// arc tangent at B is perpendicular to B-C, clockwise around C
		float cx = (ax + bx) * 0.5 - (by - ay) * 0.5;
		float cy = (ay + by) * 0.5 + (bx - ax) * 0.5;
		ux = -(by - cy);
		uy = bx - cx;
	}
	float l = sqrt(ux * ux + uy * uy);
	ux = ux / l;
	uy = uy / l;
	float tx = SX(bx);
	float ty = SY(by);
	g.clear();
	g.M(tx, ty);
	g.L(tx - ux * 9 - uy * 4, ty - uy * 9 + ux * 4);
	g.L(tx - ux * 9 + uy * 4, ty - uy * 9 - ux * 4);
	g.close();
	g.fin();
	g.rgba32(0xffe02020);
	g.fill1();
}

void DrawGrid()
{
	for (int y = 0; y < GRID; y++)
		for (int x = 0; x < GRID; x++)
			g.fillrect(SX(x) - 1, SY(y) - 1, 2, 2, 0xff6c6c6c);
}

void Line(int ax, int ay, int bx, int by, int fill, int outline)
{
	DrawLine(ax, ay, bx, by, fill, outline);
	DrawArrow(ax, ay, bx, by, 0);
}

void Sector(int ax, int ay, int bx, int by, int mode, int fill, int outline)
{
	DrawSector(ax, ay, bx, by, mode, fill, outline);
	DrawArrow(ax, ay, bx, by, 1);
}

void DrawScene()
{
	g.rgba32(0xff9a9a9a);
	g.FillRT();
	DrawGrid();

	int W = 2;   // white fill
	int K = 1;   // black outline

	// Row 1: lines in the four directions E, S, W, N (square on the right).
	Line(2, 2, 8, 2, W, K);
	Line(18, 2, 18, 8, W, K);
	Line(34, 8, 28, 8, W, K);
	Line(38, 8, 38, 2, W, K);
	// a line with transparent fill = just the outline
	Line(48, 5, 56, 5, 0, K);

	// Rows 2-4: the four sector orientations (chord SE, SW, NW, NE) with
	// inside, outside and lens bodies.
	for (int m = 0; m < 3; m++)
	{
		int y = 14 + m * 10;
		Sector(2, y, 8, y + 6, m, W, K);
		Sector(24, y, 18, y + 6, m, W, K);
		Sector(34, y + 6, 28, y, m, W, K);
		Sector(38, y + 6, 44, y, m, W, K);
	}
	// sector with transparent fill = just the arc
	Sector(50, 16, 56, 22, FILL_INSIDE, 0, K);

	// Row 5: compositions - a disc from four inside sectors, a four-petal
	// flower from lenses, and a rounded tile from outside corners + lines.
	Sector(8, 46, 14, 52, FILL_INSIDE, W, K);
	Sector(14, 52, 8, 58, FILL_INSIDE, W, K);
	Sector(8, 58, 2, 52, FILL_INSIDE, W, K);
	Sector(2, 52, 8, 46, FILL_INSIDE, W, K);

	Sector(26, 52, 32, 46, FILL_LENS, W, K);
	Sector(26, 52, 32, 58, FILL_LENS, W, K);
	Sector(26, 52, 20, 58, FILL_LENS, W, K);
	Sector(26, 52, 20, 46, FILL_LENS, W, K);

	Sector(40, 46, 44, 50, FILL_OUTSIDE, 5, 0);
	Line(44, 50, 44, 54, 6, K);
	Line(48, 46, 52, 46, 8, K);
	Sector(52, 54, 48, 58, FILL_LENS, 12, 11);
	Line(40, 58, 40, 54, 9, 0);
}

int main()
{
	InitPalette();
	DrawScene();
	g.WriteImage("primitives.png", "png", "");
	while (true)
	{
		DrawScene();
		Present();
		Wait(0.05);
	}
	return 0;
}
