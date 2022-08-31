#include <stdio.h>

//#include "words.h"
//#include "words30.h"
#include "all5letterw.h"



#include "graphics.h"
#include "../../graphic/font.h"
#include "../../graphic/letters.h"
#include "../../graphic/gtext.h"
Graph g;

int irand(int& seed)
{
	seed=(seed*1103515245+12345)&0x7ffffff;
	return seed;
}

float frand(int& seed)
{
	return (irand(seed))/134217727.0;
}

int seed=6917832469;

void RoundRect(float x, float y, float w, float h, float r)
{
	g.M(x,y+r);
	if(r>0)g.c(0,-r,r,-r,r,-r);
	g.l(w-r*2,0);
	if(r>0)g.c(r,0,r,r,r,r);
	g.l(0,h-r*2);
	if(r>0)g.c(0,r,-r,r,-r,r);
	g.l(-w+r*2,0);
	if(r>0)g.c(-r,0,-r,-r,-r,-r);
	g.close();
}


int main()
{
	CFont font;
	CFontMaker fm;
	fm.Init(font,6);
	MakeLetters(fm);
	int nwords=0;
	for(int i=0;;i++)
	{
		if(all5letter[i]){}//printf("'%s'\n", wordlist[i]);
		else break;
		nwords++;
	}
	int n=0;
	while(1)
	{
		g.rgba32(0xff406080);
		g.FillRT();
		g.clear();
		DrawText(g,font,"Guess the word",0,160,40,3,2.5);
		g.fin();
		g.rgba32(0xff000000);
		g.width(4.5,1.);
		g.stroke();
		g.rgba32(0xffffffff);
		g.width(2.,1.5);
		g.stroke();

		char* word=all5letter[n];
		n++;
		if(n>=nwords)n=0;

		for(int i=0;i<5;i++)
		{
			g.clear();
			RoundRect(60+i*60,110,50,50,5);
			g.fin();
			g.rgba32(0xff203040);
			g.fill1();

			g.clear();
			RoundRect(60+i*60,100,50,50,5);
			g.fin();
			g.rgba32(0xffa0c0c0);
			g.fill1();

			g.clear();
			DrawGlyph(g,font,(int)word[i],80+i*60,135,3,3);
			g.fin();
			g.rgba32(0xffffffff);
			g.width(4.5,1.);
			g.stroke();
			g.rgba32(0xff000000);
			g.width(2.,1.5);
			g.stroke();
		}

		Present();
	}
	return 0;
}