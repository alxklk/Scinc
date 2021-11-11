struct CGlyphSpan
{
public:
	float p[9];
	void Init(){p[0]=0;}
	void M(float x, float y){p[0]=1;p[1]=x;p[2]=y;}
	void m(float x, float y){p[0]=2;p[1]=x;p[2]=y;}
	void L(float x, float y){p[0]=3;p[1]=x;p[2]=y;}
	void l(float x, float y){p[0]=4;p[1]=x;p[2]=y;}
	void C(float x0, float y0,float x1, float y1,float x2, float y2,float x3, float y3)
	{p[0]=5;p[1]=x0;p[2]=y0;p[3]=x1;p[4]=y1;p[5]=x2;p[6]=y2;p[7]=x3;p[8]=y3;}
};

#define NSPANS 16

class CGlyph
{
public:
	int n;
	CGlyphSpan s[NSPANS];
	void Init(){n=0;for(int i=0;i<NSPANS;i++)s[i].Init();}
	void M(float x, float y){if(n<(NSPANS-1)){s[n].M(x,y);n++;}}
	void m(float x, float y){if(n<(NSPANS-1)){s[n].m(x,y);n++;}}
	void L(float x, float y){if(n<(NSPANS-1)){s[n].L(x,y);n++;}}
	void l(float x, float y){if(n<(NSPANS-1)){s[n].l(x,y);n++;}}
	void C(float x0,float y0, float x1,float y1, float x2,float y2, float x3,float y3)
	{if(n<(NSPANS-1)){s[n].C(x0,y0,x1,y1,x2,y2,x3,y3);n++;}}
};

class CFont
{
public:
	CGlyph g[256];
	void Init(){for(int i=0;i<256;i++)g[i].Init();}
};