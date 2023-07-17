#ifndef FONT_H
#define FONT_H

class CGlyph
{
public:
	int col;
	int w;
	int n;
	int c0;
	int d0;
};

#define NGLYF 128 // 
#define NUGLYF 128 // 
#define NCMDS (NGLYF*16) 
#define NDATA (NGLYF*26*4)

struct UniChar
{
	int code;
	CGlyph g;
};
class CFont
{
public:
	int cmds[NCMDS];
	float data[NDATA];
	CGlyph g[NGLYF];
	UniChar u[NUGLYF];
};

CGlyph* FindGlyph(CFont& f, int g)
{
	if(g<NGLYF)
		return &f.g[g];
	for(int i=0;i<NUGLYF;i++)
	{
		if(f.u[i].code==g)
		{
			return &f.u[i].g;
		}
	}
	return 0;
}


class CFontMaker
{
public:
	void Init(CFont& f, int w)
	{
		font=&f;
		for(int i=0;i<NGLYF;i++)
		{
			f.g[i].n=0;
			f.g[i].w=w;
			f.g[i].col=0;
		}
		for(int i=0;i<NUGLYF;i++)
		{
			f.u[i].code=-1;
			f.u[i].g.n=0;
			f.u[i].g.w=w;
			f.u[i].g.col=0;
		}
		curCmd=0;
		curData=0;
		curGlyph=-1;
	}
	void Cmd(int c)
	{
		if(curCmd>=NCMDS)
		{
			printf(" Font commands overflow, >= %i", NCMDS);
			return;
		}
		font->cmds[curCmd]=c;
		curCmd++;
	}
	void Data(float d)
	{
		if(curData>=NDATA)
		{
			printf(" Font data overflow, >= %i", NDATA);
			return;
		}
		font->data[curData]=d;
		curData++;
	}
	int NewUGlyph(int uglyph)
	{
		int found=-1;
		for(int i=0;i<NUGLYF;i++)
		{
			if(font->u[i].code==-1)
			{
				found=i;
				font->u[found].code=uglyph;
				font->u[found].g.c0=curCmd;
				font->u[found].g.d0=curData;
				font->u[found].g.n=0;
				break;
			}
		}
		return found;
	}
	void StartGlyph(int glyph)
	{
		if(curGlyph!=-1)
		{
			puts("Glyph not closed!");
			return;
		}
		curGlyph=glyph;
		if(glyph<NGLYF)
		{
			font->g[glyph].c0=curCmd;
			font->g[glyph].d0=curData;
			font->g[glyph].n=0;
		}
		else
		{
			curUGlyphIdx=NewUGlyph(glyph);
		}
	}
	void Width(float w)
	{
		if(curGlyph==-1)
		{
			puts("Glyph not started for Width");
			return;
		}
		font->g[curGlyph].w=w;
	}
	void EndGlyph()
	{
		if(curGlyph==-1)
		{
			puts("Glyph double closed!");
			return;
		}
		//printf(" Defined glyph '%c' with %i cmds and %i data (%i/%i %i/%i)\n", curGlyph, curCmd-font->g[curGlyph].c0,
		//curData-font->g[curGlyph].d0, curCmd, NCMDS, curData, NDATA);
		font->g[curGlyph].n=curCmd-font->g[curGlyph].c0;
		curGlyph=-1;
	}
	void M(float x, float y)
	{
		Cmd('M');
		Data(x);
		Data(-y);
	}
	void m(float x, float y)
	{
		Cmd('m');
		Data(x);
		Data(-y);
	}
	void L(float x, float y)
	{
		Cmd('L');
		Data(x);
		Data(-y);
	}
	void l(float x, float y)
	{
		Cmd('l');
		Data(x);
		Data(-y);
	}
	void c(float x1, float y1,float x2, float y2,float x3, float y3)
	{
		Cmd('c');
		Data(x1);
		Data(-y1);
		Data(x3+x2);
		Data(-y3-y2);
		Data(x3);
		Data(-y3);
	}
	void z()
	{
		Cmd('z');
	}
	void clear()
	{
		font->g[curGlyph].col=1;
		Cmd('0');
	}
	void stroke(float w, int c)
	{
		font->g[curGlyph].col=1;
		Cmd('/');
		Data(w);
		Data(c);
	}
	void fill(int c)
	{
		font->g[curGlyph].col=1;
		Cmd('f');
		Data(c);
	}
	int curCmd;
	int curData;
	int curGlyph;
	int curUGlyphIdx;
	CFont* font;
};

#endif