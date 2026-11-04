
#define G_SCREEN_WIDTH 1000
#define G_SCREEN_HEIGHT 300
#define G_SCREEN_SCALE 2
#define G_SCREEN_MODE 1

#include "graphics.h"
#include "../ws.h"
#include "font.h"
#include "letters.h"
#include "gtext.h"

Graph g;


int main()
{
	double T;
	char s[100];
	CFont* pfont=(CFont*)malloc(sizeof(CFont));
	CFont& font=*pfont;
	CFontMaker fm;
	fm.Init(font,6);
	MakeLetters(fm);

	while(true)
	{
		g.rgba32(0xff203040);
		g.FillRT();
		T=Time();
		snprintf(s, sizeof(s), "%3.3f", T);
		//stext(s, 10, 10, 0xffffffff);
		g.rgba32(0xffffffff);
		g.width(7,7);
		g.clear();
		DrawText(g,font,s,0,50,240,20,20);

		g.fin();
		g.stroke();
		Wait(1./60.-(Time()-T));
		Present();
	}
	return 0;
}