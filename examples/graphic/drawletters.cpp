#ifdef __JS__
	#define G_SCREEN_SCALE 2
#else
	//#define G_SCREEN_WIDTH 1280
	//#define G_SCREEN_HEIGHT 960
	#define G_SCREEN_SCALE 4
#endif

#include "graphics.h"

Graph g;

#include "font.h"
#include "letters.h"
#include "gtext.h"
#include "Bezier.h"


void PP(float x, float y, float dx, float dy, int col)
{
	g.clear();
	g.alpha(.3);
	g.M(x+dy*5,y-dx*5);
	g.fin();
	g.width(10,1);
	g.rgb(dx*.5+.5,0,dy*.5+.5);
	g.stroke();
	g.clear();
	g.M(x-dy*5,y+dx*5);
	g.fin();
	g.width(10,1);
	g.rgb(-dx*.5+.5,0,-dy*.5+.5);
	g.stroke();
//	PutPixel(x,y,col);
}

#include "FontGlyphConvert.h"

int main()
{
	CFont* pfont=(CFont*)malloc(sizeof(CFont));
	CFont& font=*pfont;
	CFontMaker fm;
	fm.Init(font,6);
	MakeLetters(fm);
	bool redraw=true;
	float step=2.5;
	float sx=14;
	float sy=14;
	int sym='R';
	while(true)
	{
		SScincEvent ev;
		if(!redraw)
		while(GetScincEvent(ev))
		{
			if(ev.type=='MMOV')
			{
				sx=ev._1/10.;
				sy=ev._2/10.;
			}
			if(ev.type=='KBDN')
			{
				sym=ev._0;
			}
		}
		redraw=false;

		g.rgba32(0xff406080);
		g.FillRT();
		g.clear();
		//DrawGlyph(g,font,sym,200,200,sx,sy);
		g.fin();
		g.rgba(.3,.1,1,1);
		g.width(7.,7.);
		g.stroke();

		float x=200;
		float y=200;
		ConvertGlyph(font, sym, x, y, sx, sy, step);

		Present();
		WaitForScincEvent();
	}
	return 0;
}