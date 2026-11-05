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

#define NGLYF 128
#define NUGLYF 320
//#define NCMDS (NGLYF*16) 
//#define NDATA (NGLYF*26*4)
#define NCMDS 8192
#define NDATA 32768

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
	UniChar u[NUGLYF];  // sorted by code
	int nu;             // number of used entries in u
};

CGlyph* FindGlyph(CFont& f, int g)
{
	if(g<NGLYF)
		return &f.g[g];
	int lo=0;
	int hi=f.nu-1;
	while(lo<=hi)
	{
		int m=(lo+hi)/2;
		int c=f.u[m].code;
		if(c==g)
			return &f.u[m].g;
		if(c<g)
			lo=m+1;
		else
			hi=m-1;
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
			f.u[i].code=0;
			f.u[i].g.n=0;
			f.u[i].g.w=w;
			f.u[i].g.col=0;
		}
		f.nu=0;
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
	// the entry for a Unicode glyph, kept in code order; an existing code is redefined
	int NewUGlyph(int uglyph)
	{
		int nu=font->nu;
		int found=nu;
		for(int i=0;i<nu;i++)
		{
			if(font->u[i].code>=uglyph)
			{
				found=i;
				break;
			}
		}
		int isnew=1;
		if(found<nu)
		{
			if(font->u[found].code==uglyph)
				isnew=0;
		}
		if(isnew)
		{
			if(nu>=NUGLYF)
			{
				printf(" Font Unicode table overflow, >= %i\n", NUGLYF);
				return -1;
			}
			for(int i=nu;i>found;i--)
			{
				font->u[i].code=font->u[i-1].code;
				font->u[i].g.col=font->u[i-1].g.col;
				font->u[i].g.w=font->u[i-1].g.w;
				font->u[i].g.n=font->u[i-1].g.n;
				font->u[i].g.c0=font->u[i-1].g.c0;
				font->u[i].g.d0=font->u[i-1].g.d0;
			}
			font->nu=nu+1;
			font->u[found].g.w=font->g[' '].w;
		}
		font->u[found].code=uglyph;
		font->u[found].g.c0=curCmd;
		font->u[found].g.d0=curData;
		font->u[found].g.n=0;
		font->u[found].g.col=0;
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
	// a Unicode glyph that looks exactly like another one (Cyrillic A, O...)
	// shares its drawing commands
	void Alias(int uglyph, int glyph)
	{
		CGlyph* p=FindGlyph(*font,glyph);
		if(!p)
			return;
		CGlyph src=*p;
		int i=NewUGlyph(uglyph);
		if(i<0)
			return;
		font->u[i].g.col=src.col;
		font->u[i].g.w=src.w;
		font->u[i].g.n=src.n;
		font->u[i].g.c0=src.c0;
		font->u[i].g.d0=src.d0;
	}
	void Width(float w)
	{
		if(curGlyph==-1)
		{
			puts("Glyph not started for Width");
			return;
		}
		if(curGlyph<NGLYF)
			font->g[curGlyph].w=w;
		else
			font->u[curUGlyphIdx].g.w=w;
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
		if(curGlyph<NGLYF)
			font->g[curGlyph].n=curCmd-font->g[curGlyph].c0;
		else
			font->u[curUGlyphIdx].g.n=curCmd-font->u[curUGlyphIdx].g.c0;
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
	void close()
	{
		Cmd('z');
	}
	void clear()
	{
		if(curGlyph<NGLYF)
			font->g[curGlyph].col=1;
		else
			font->u[curUGlyphIdx].g.col=1;
		Cmd('0');
	}
	void stroke(float w, int c)
	{
		if(curGlyph<NGLYF)
			font->g[curGlyph].col=1;
		else
			font->u[curUGlyphIdx].g.col=1;
		Cmd('/');
		Data(w);
		Data(c);
	}
	// fills the path with the colour the program set before drawing the glyph,
	// for solid symbols that take the colour of the text
	void ink()
	{
		if(curGlyph<NGLYF)
			font->g[curGlyph].col=1;
		else
			font->u[curUGlyphIdx].g.col=1;
		Cmd('F');
	}
	void fill(int c)
	{
		if(curGlyph<NGLYF)
			font->g[curGlyph].col=1;
		else
			font->u[curUGlyphIdx].g.col=1;
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