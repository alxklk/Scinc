#include <stdio.h>
#include "graphics.h"      // intrinsic: the Graph drawing API (g.*)
#include "../../ws.h"      // windowing: Present(), Time(); real file at examples/ws.h

// Animated Lissajous curve.
//   x = cx + R * sin(a*t + phase)
//   y = cy + R * sin(b*t)
// The phase is driven by Time(), so the 3:2 figure slowly morphs.
//
// Note the Scinc idioms: no comma-initialised declarations (each variable on
// its own line), and the drawing loop is the usual clear -> path -> stroke.

Graph g;

#define M_PI 3.14159265358979

int main()
{
	float cx = 320;        // window is 640x480 by default -> center at (320,240)
	float cy = 240;
	float R  = 180;        // amplitude / radius
	float a  = 3;          // horizontal frequency
	float b  = 2;          // vertical frequency
	int   N  = 512;        // points along the curve

	while (true)
	{
		double T = Time();
		float phase = T;

		// dark background
		g.rgba32(0xff101828);
		g.FillRT();

		// build the curve as one polyline
		g.clear();
		g.M(cx + R * sin(phase), cy);
		for (int i = 0; i <= N; i++)
		{
			float t = i * 2.0 * M_PI / N;
			float x = cx + R * sin(a * t + phase);
			float y = cy + R * sin(b * t);
			g.L(x, y);
		}
		g.fin();

		// stroke twice: a wide translucent glow, then a thin bright core
		g.rgba32(0x50ff40b0);
		g.width(14, 14);
		g.stroke();
		g.rgba32(0xffa0ffe0);
		g.width(3, 3);
		g.stroke();

		Present();
	}
	return 0;
}
