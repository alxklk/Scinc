// Ukrainian Cyrillic, by Unicode code point (DrawText reads UTF-8). Letters
// that look exactly like Latin ones share their glyphs.
void MakeUkrainian(CFontMaker& fm)
{
	fm.Alias(0x410,'A');
	fm.Alias(0x412,'B');
	fm.Alias(0x415,'E');
	fm.Alias(0x406,'I');
	fm.Alias(0x41A,'K');
	fm.Alias(0x41C,'M');
	fm.Alias(0x41D,'H');
	fm.Alias(0x41E,'O');
	fm.Alias(0x420,'P');
	fm.Alias(0x421,'C');
	fm.Alias(0x422,'T');
	fm.Alias(0x425,'X');
	fm.Alias(0x430,'a');
	fm.Alias(0x435,'e');
	fm.Alias(0x456,'i');
	fm.Alias(0x43E,'o');
	fm.Alias(0x440,'p');
	fm.Alias(0x441,'c');
	fm.Alias(0x443,'y');
	fm.Alias(0x445,'x');
	fm.Alias(0x2BC,'\'');   // modifier letter apostrophe
	fm.Alias(0x2019,'\'');  // right single quotation mark

	// ---- capitals
	{
		fm.StartGlyph(0x411);   // Б
		fm.M(4,9);
		fm.l(-4,0);
		fm.l(0,-9);
		fm.l(2,0);
		fm.c(2.75,0,2.75,0,0,5);
		fm.l(-2,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x413);   // Г
		fm.M(0,0);
		fm.l(0,9);
		fm.l(4,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x490);   // Ґ
		fm.M(0,0);
		fm.l(0,9);
		fm.l(4,0);
		fm.l(0,1.5);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x414);   // Д
		fm.M(0,-1.5);
		fm.l(0,1.5);
		fm.l(4,0);
		fm.l(0,-1.5);
		fm.M(.5,0);
		fm.l(1,9);
		fm.l(2,0);
		fm.l(0,-9);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x404);   // Є
		fm.M(4,7);
		fm.c(-.5,   3,   0,   5,-4,-2.5);
		fm.c(  0,-5.5, -.5,-3.5, 4,-2.5);
		fm.M(0,4.5);
		fm.l(2.75,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x416);   // Ж
		fm.M(2,0);
		fm.l(0,9);
		fm.M(0,9);
		fm.l(2,-4.5);
		fm.l(2,4.5);
		fm.M(0,0);
		fm.l(2,4.5);
		fm.l(2,-4.5);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x417);   // З
		fm.M(.25,7);
		fm.c(0,2.6,0,2.6,3.5,0);
		fm.c(0,-1.5,1,0,-2,-2);
		fm.c(2,0,0,1,2.25,-2.5);
		fm.c(0,-3.,.5,-3,-4,-.5);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x418);   // И
		fm.M(0,9);
		fm.l(0,-9);
		fm.l(4,9);
		fm.l(0,-9);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x419);   // Й
		fm.M(0,9);
		fm.l(0,-9);
		fm.l(4,9);
		fm.l(0,-9);
		fm.M(1,11);
		fm.c(0,-1.2,0,-1.2,2,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x407);   // Ї
		fm.M(1,0);
		fm.l(2,0);
		fm.M(1,9);
		fm.l(2,0);
		fm.M(2,0);
		fm.l(0,9);
		fm.M(1,10.5);
		fm.l(0,.5);
		fm.M(.75,10.75);
		fm.l(.5,0);
		fm.M(3,10.5);
		fm.l(0,.5);
		fm.M(2.75,10.75);
		fm.l(.5,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x41B);   // Л
		fm.M(0,0);
		fm.l(1.25,9);
		fm.l(2.75,0);
		fm.l(0,-9);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x41F);   // П
		fm.M(0,0);
		fm.l(0,9);
		fm.l(4,0);
		fm.l(0,-9);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x423);   // У
		fm.M(0,9);
		fm.L(2,3);
		fm.M(4,9);
		fm.L(1,0);
		fm.l(-1,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x424);   // Ф
		fm.M(2,0);
		fm.l(0,9);
		fm.M(2,7);
		fm.c(-2.75,0,-2.75,0,0,-5);
		fm.c( 2.75,0, 2.75,0,0, 5);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x426);   // Ц
		fm.M(0,9);
		fm.L(0,0);
		fm.L(4,0);
		fm.l(0,-1.5);
		fm.M(3.5,0);
		fm.L(3.5,9);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x427);   // Ч
		fm.M(0,9);
		fm.l(0,-3.5);
		fm.c(0,-2,-1.5,0,4,-1.5);
		fm.M(4,9);
		fm.l(0,-9);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x428);   // Ш
		fm.M(0,9);
		fm.l(0,-9);
		fm.l(4,0);
		fm.l(0,9);
		fm.M(2,0);
		fm.l(0,9);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x429);   // Щ
		fm.M(0,9);
		fm.L(0,0);
		fm.L(4,0);
		fm.l(0,-1.5);
		fm.M(1.75,0);
		fm.L(1.75,9);
		fm.M(3.5,0);
		fm.L(3.5,9);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x42C);   // Ь
		fm.M(0,9);
		fm.l(0,-9);
		fm.l(2,0);
		fm.c(2.75,0,2.75,0,0,5);
		fm.l(-2,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x42E);   // Ю
		fm.M(0,0);
		fm.l(0,9);
		fm.M(0,4.5);
		fm.l(1,0);
		fm.M(2.5,9);
		fm.c(-2,0,-2,0,0,-9);
		fm.c( 2,0, 2,0,0, 9);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x42F);   // Я
		fm.M(4,4);
		fm.l(-2,0);
		fm.M(4,0);
		fm.l(0,9);
		fm.l(-2,0);
		fm.c(-2.75,0,-2.75,0,0,-5);
		fm.M(2.5,4);
		fm.c(-1.5,-.5,1.5,1,-2.75,-4);
		fm.EndGlyph();
	}

	// ---- small letters
	{
		fm.StartGlyph(0x431);   // б
		fm.M(0,3);
		fm.c(0, 4,0, 4, 4,0);
		fm.c(0,-4,0,-4,-4,0);
		fm.c(0,5,-1.5,0,4,6);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x432);   // в
		fm.M(0,3.5);
		fm.l(2,0);
		fm.M(2,0);
		fm.l(-2,0);
		fm.l(0,6);
		fm.l(1.75,0);
		fm.c(1.75,0,1.75,0,0,-2.5);
		fm.m(.25,0);
		fm.c(2.25,0,2.25,0,0,-3.5);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x433);   // г
		fm.M(0,0);
		fm.l(0,6);
		fm.l(3.5,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x491);   // ґ
		fm.M(0,0);
		fm.l(0,6);
		fm.l(3.5,0);
		fm.l(0,1.5);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x434);   // д
		fm.M(0,-1.5);
		fm.l(0,1.5);
		fm.l(4,0);
		fm.l(0,-1.5);
		fm.M(.5,0);
		fm.l(1,6);
		fm.l(2,0);
		fm.l(0,-6);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x454);   // є
		fm.M(4,5);
		fm.c(-.5,1.5,0,3.5,-4,-2);
		fm.c(0,-3.5,-.5,-2,4,-2);
		fm.M(0,3);
		fm.l(3,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x436);   // ж
		fm.M(2,0);
		fm.l(0,6);
		fm.M(0,6);
		fm.l(2,-3);
		fm.l(2,3);
		fm.M(0,0);
		fm.l(2,3);
		fm.l(2,-3);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x437);   // з
		fm.M(.25,4.75);
		fm.c(0,1.75,0,1.75,3.5,0);
		fm.c(0,-1,.75,0,-1.75,-1.5);
		fm.c(1.5,0,0,.75,2,-1.75);
		fm.c(0,-2,.5,-2,-4,-.25);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x438);   // и
		fm.M(0,6);
		fm.l(0,-6);
		fm.l(4,6);
		fm.l(0,-6);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x439);   // й
		fm.M(0,6);
		fm.l(0,-6);
		fm.l(4,6);
		fm.l(0,-6);
		fm.M(1,8.5);
		fm.c(0,-1.2,0,-1.2,2,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x457);   // ї
		fm.M(2,0);
		fm.l(0,6);
		fm.M(1,0);
		fm.l(2,0);
		fm.M(1,6);
		fm.l(1,0);
		fm.M(1,7.75);
		fm.l(0,.5);
		fm.M(.75,8);
		fm.l(.5,0);
		fm.M(3,7.75);
		fm.l(0,.5);
		fm.M(2.75,8);
		fm.l(.5,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x43A);   // к
		fm.M(0,0);
		fm.l(0,6);
		fm.M(0,2.5);
		fm.L(3.5,6);
		fm.M(1.25,3.75);
		fm.L(4,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x43B);   // л
		fm.M(0,0);
		fm.l(1.25,6);
		fm.l(2.75,0);
		fm.l(0,-6);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x43C);   // м
		fm.M(0,0);
		fm.l(0,6);
		fm.l(2,-4);
		fm.l(2,4);
		fm.l(0,-6);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x43D);   // н
		fm.M(0,0);
		fm.l(0,6);
		fm.M(4,0);
		fm.l(0,6);
		fm.M(0,3);
		fm.l(4,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x43F);   // п
		fm.M(0,0);
		fm.l(0,6);
		fm.l(4,0);
		fm.l(0,-6);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x442);   // т
		fm.M(2,0);
		fm.l(0,6);
		fm.M(0,6);
		fm.l(4,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x444);   // ф
		fm.M(2,-3);
		fm.L(2,9);
		fm.M(2,6);
		fm.c(-2.75,0,-2.75,0,0,-6);
		fm.c( 2.75,0, 2.75,0,0, 6);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x446);   // ц
		fm.M(0,6);
		fm.L(0,0);
		fm.L(4,0);
		fm.l(0,-1.5);
		fm.M(3.5,0);
		fm.L(3.5,6);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x447);   // ч
		fm.M(0,6);
		fm.l(0,-2);
		fm.c(0,-1.5,-1.5,0,4,-1);
		fm.M(4,6);
		fm.L(4,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x448);   // ш
		fm.M(0,6);
		fm.L(0,0);
		fm.L(4,0);
		fm.L(4,6);
		fm.M(2,0);
		fm.L(2,6);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x449);   // щ
		fm.M(0,6);
		fm.L(0,0);
		fm.L(4,0);
		fm.l(0,-1.5);
		fm.M(1.75,0);
		fm.L(1.75,6);
		fm.M(3.5,0);
		fm.L(3.5,6);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x44C);   // ь
		fm.M(0,6);
		fm.L(0,0);
		fm.l(2,0);
		fm.c(2.5,0,2.5,0,0,3.5);
		fm.l(-2,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x44E);   // ю
		fm.M(0,0);
		fm.L(0,6);
		fm.M(0,3);
		fm.l(1,0);
		fm.M(2.5,6);
		fm.c(-2,0,-2,0,0,-6);
		fm.c( 2,0, 2,0,0, 6);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x44F);   // я
		fm.M(4,0);
		fm.L(4,6);
		fm.l(-2,0);
		fm.c(-2.5,0,-2.5,0,0,-3.5);
		fm.l(2,0);
		fm.M(2.5,2.5);
		fm.L(0,0);
		fm.EndGlyph();
	}
}

// ---------------------------------------------------------------- symbols
// Line drawing, blocks and other symbols of text-mode screens, by Unicode code
// point. Line drawing and blocks fill the whole character cell, x from -1 to 5
// (the advance is 6) and y from -4 to 12, so they join up when text lines are
// 16 units apart. The middle of the cell is (2,4), the height of '-' and '+'.
// Solid symbols (blocks, heavy lines, ■ ● ▲ ♥ ...) are colour glyphs: DrawText
// draws them when its col argument is 1, filled with the colour set last.

float symX;
float symY;

// absolute moveto, lineto and cubic, so shapes can be written in cell coordinates
void SymM(CFontMaker& fm, float x, float y)
{
	fm.M(x,y);
	symX=x;
	symY=y;
}

void SymL(CFontMaker& fm, float x, float y)
{
	fm.L(x,y);
	symX=x;
	symY=y;
}

void SymC(CFontMaker& fm, float x1, float y1, float x2, float y2, float x3, float y3)
{
	fm.c(x1-symX,y1-symY,x2-x3,y2-y3,x3-symX,y3-symY);
	symX=x3;
	symY=y3;
}

void SymLine(CFontMaker& fm, float x0, float y0, float x1, float y1)
{
	SymM(fm,x0,y0);
	SymL(fm,x1,y1);
}

// a rectangle, counterclockwise; hole=1 goes the other way, to cut it out of
// a filled shape
void SymRect(CFontMaker& fm, float x0, float y0, float x1, float y1, int hole)
{
	SymM(fm,x0,y0);
	if(hole)
	{
		SymL(fm,x0,y1);
		SymL(fm,x1,y1);
		SymL(fm,x1,y0);
	}
	else
	{
		SymL(fm,x1,y0);
		SymL(fm,x1,y1);
		SymL(fm,x0,y1);
	}
	fm.z();
}

// an ellipse, counterclockwise, or clockwise when hole=1
void SymOval(CFontMaker& fm, float cx, float cy, float rx, float ry, int hole)
{
	float kx=rx*0.5523;
	float ky=ry*0.5523;
	SymM(fm,cx+rx,cy);
	if(hole)
	{
		SymC(fm,cx+rx,cy-ky,cx+kx,cy-ry,cx,cy-ry);
		SymC(fm,cx-kx,cy-ry,cx-rx,cy-ky,cx-rx,cy);
		SymC(fm,cx-rx,cy+ky,cx-kx,cy+ry,cx,cy+ry);
		SymC(fm,cx+kx,cy+ry,cx+rx,cy+ky,cx+rx,cy);
	}
	else
	{
		SymC(fm,cx+rx,cy+ky,cx+kx,cy+ry,cx,cy+ry);
		SymC(fm,cx-kx,cy+ry,cx-rx,cy+ky,cx-rx,cy);
		SymC(fm,cx-rx,cy-ky,cx-kx,cy-ry,cx,cy-ry);
		SymC(fm,cx+kx,cy-ry,cx+rx,cy-ky,cx+rx,cy);
	}
	fm.z();
}

void SymCircle(CFontMaker& fm, float cx, float cy, float r, int hole)
{
	SymOval(fm,cx,cy,r,r,hole);
}

void SymTri(CFontMaker& fm, float x0, float y0, float x1, float y1, float x2, float y2)
{
	SymM(fm,x0,y0);
	SymL(fm,x1,y1);
	SymL(fm,x2,y2);
	fm.z();
}

// A box drawing character from its arms: 0 none, 1 light, 2 heavy, 3 double.
// Light and double lines are strokes; heavy ones are solid bars.
void SymBox(CFontMaker& fm, int code, int n, int e, int s, int w)
{
	float L=-1;
	float R=5;
	float B=-4;
	float T=12;
	float cx=2;
	float cy=4;
	float d=1.25;  // double lines run this far either side of the middle
	fm.StartGlyph(code);
	if((n==2)|(e==2)|(s==2)|(w==2))
	{
		float t=1.1;
		fm.clear();
		SymRect(fm,cx-t,cy-t,cx+t,cy+t,0);
		if(n)SymRect(fm,cx-t,cy,cx+t,T,0);
		if(s)SymRect(fm,cx-t,B,cx+t,cy,0);
		if(e)SymRect(fm,cx,cy-t,R,cy+t,0);
		if(w)SymRect(fm,L,cy-t,cx,cy+t,0);
		fm.ink();
		fm.EndGlyph();
		return;
	}
	int vD=(n==3)|(s==3);
	int hD=(e==3)|(w==3);
	if(vD&hD)
	{
		// the outline of the double channels, one quadrant at a time
		if(n&w){SymM(fm,cx-d,T);SymL(fm,cx-d,cy+d);SymL(fm,L,cy+d);}
		else if(n){SymM(fm,cx-d,T);SymL(fm,cx-d,cy);}
		else if(w){SymM(fm,L,cy+d);SymL(fm,cx,cy+d);}
		else{SymM(fm,cx-d,cy);SymL(fm,cx-d,cy+d);SymL(fm,cx,cy+d);}
		if(n&e){SymM(fm,cx+d,T);SymL(fm,cx+d,cy+d);SymL(fm,R,cy+d);}
		else if(n){SymM(fm,cx+d,T);SymL(fm,cx+d,cy);}
		else if(e){SymM(fm,R,cy+d);SymL(fm,cx,cy+d);}
		else{SymM(fm,cx+d,cy);SymL(fm,cx+d,cy+d);SymL(fm,cx,cy+d);}
		if(s&w){SymM(fm,cx-d,B);SymL(fm,cx-d,cy-d);SymL(fm,L,cy-d);}
		else if(s){SymM(fm,cx-d,B);SymL(fm,cx-d,cy);}
		else if(w){SymM(fm,L,cy-d);SymL(fm,cx,cy-d);}
		else{SymM(fm,cx-d,cy);SymL(fm,cx-d,cy-d);SymL(fm,cx,cy-d);}
		if(s&e){SymM(fm,cx+d,B);SymL(fm,cx+d,cy-d);SymL(fm,R,cy-d);}
		else if(s){SymM(fm,cx+d,B);SymL(fm,cx+d,cy);}
		else if(e){SymM(fm,R,cy-d);SymL(fm,cx,cy-d);}
		else{SymM(fm,cx+d,cy);SymL(fm,cx+d,cy-d);SymL(fm,cx,cy-d);}
	}
	else if(hD)
	{
		// double across, light up and down: a light arm stops at the near
		// line when the double one goes straight through, else at the far one
		int straight=(e>0)&(w>0);
		float x0=cx;
		float x1=cx;
		if(w)x0=L;
		if(e)x1=R;
		SymLine(fm,x0,cy+d,x1,cy+d);
		SymLine(fm,x0,cy-d,x1,cy-d);
		if(straight)
		{
			if(n)SymLine(fm,cx,T,cx,cy+d);
			if(s)SymLine(fm,cx,B,cx,cy-d);
		}
		else
		{
			if(n)SymLine(fm,cx,T,cx,cy-d);
			if(s)SymLine(fm,cx,B,cx,cy+d);
		}
	}
	else if(vD)
	{
		int straight=(n>0)&(s>0);
		float y0=cy;
		float y1=cy;
		if(s)y0=B;
		if(n)y1=T;
		SymLine(fm,cx-d,y0,cx-d,y1);
		SymLine(fm,cx+d,y0,cx+d,y1);
		if(straight)
		{
			if(w)SymLine(fm,L,cy,cx-d,cy);
			if(e)SymLine(fm,R,cy,cx+d,cy);
		}
		else
		{
			if(w)SymLine(fm,L,cy,cx+d,cy);
			if(e)SymLine(fm,R,cy,cx-d,cy);
		}
	}
	else
	{
		if(n)SymLine(fm,cx,T,cx,cy);
		if(s)SymLine(fm,cx,B,cx,cy);
		if(w)SymLine(fm,L,cy,cx,cy);
		if(e)SymLine(fm,R,cy,cx,cy);
	}
	fm.EndGlyph();
}

// k dashes across (vert=0) or down (vert=1) the cell
void SymDash(CFontMaker& fm, int code, int k, int vert)
{
	fm.StartGlyph(code);
	for(int i=0;i<k;i++)
	{
		if(vert)
		{
			float p=16.0/k;
			SymLine(fm,2,-4+p*i+p*0.2,2,-4+p*i+p*0.8);
		}
		else
		{
			float p=6.0/k;
			SymLine(fm,-1+p*i+p*0.2,4,-1+p*i+p*0.8,4);
		}
	}
	fm.EndGlyph();
}

// a rounded corner from the middle of side a to the middle of side b
// (0 up, 1 right, 2 down, 3 left), through an arc of radius 3
void SymArc(CFontMaker& fm, int code, int vy, int hx)
{
	// vy: +1 the vertical arm goes up, -1 down; hx: +1 the arm goes right, -1 left
	float k=3*0.4477;
	fm.StartGlyph(code);
	if(vy>0)
		SymM(fm,2,12);
	else
		SymM(fm,2,-4);
	SymL(fm,2,4+3*vy);
	SymC(fm,2,4+k*vy,2+k*hx,4,2+3*hx,4);
	fm.EndGlyph();
}

// a solid block covering fractions of the cell
void SymBlock(CFontMaker& fm, int code, float fx0, float fy0, float fx1, float fy1)
{
	fm.StartGlyph(code);
	fm.clear();
	SymRect(fm,-1+6*fx0,-4+16*fy0,-1+6*fx1,-4+16*fy1,0);
	fm.ink();
	fm.EndGlyph();
}

// quadrants: 1 upper left, 2 upper right, 4 lower left, 8 lower right
void SymQuad(CFontMaker& fm, int code, int q)
{
	fm.StartGlyph(code);
	fm.clear();
	if(q&1)SymRect(fm,-1,4,2,12,0);
	if(q&2)SymRect(fm,2,4,5,12,0);
	if(q&4)SymRect(fm,-1,-4,2,4,0);
	if(q&8)SymRect(fm,2,-4,5,4,0);
	fm.ink();
	fm.EndGlyph();
}

// light, medium and dark shade (level 1, 2, 3) as a pattern of 1x1 squares
void SymShade(CFontMaker& fm, int code, int level)
{
	fm.StartGlyph(code);
	fm.clear();
	if(level==3)
		SymRect(fm,-1,-4,5,12,0);
	for(int row=0;row<16;row++)
	{
		for(int col=0;col<6;col++)
		{
			int on=0;
			if(level==2)
				on=((row+col)&1)==0;
			else
				on=((row&1)==0)&(((col+row/2)&1)==0);
			if(on)
				SymRect(fm,-1+col,-4+row,-1+col+1,-4+row+1,level==3);
		}
	}
	fm.ink();
	fm.EndGlyph();
}

// an arrow along a line, with the head at x1,y1
void SymArrowHead(CFontMaker& fm, float x, float y, float dx, float dy)
{
	SymM(fm,x-dx*1.8-dy*1.6,y-dy*1.8+dx*1.6);
	SymL(fm,x,y);
	SymL(fm,x-dx*1.8+dy*1.6,y-dy*1.8-dx*1.6);
}

// a copy of glyph ch scaled by k and moved by dx,dy, for superscripts
void SymCopy(CFontMaker& fm, int ch, float dx, float dy, float k)
{
	CFont& F=*fm.font;
	CGlyph* p=FindGlyph(F,ch);
	if(!p)
		return;
	CGlyph gl=*p;
	int d=gl.d0;
	for(int i=0;i<gl.n;i++)
	{
		int C=F.cmds[gl.c0+i];
		if((C=='M')|(C=='L'))
		{
			fm.Cmd(C);
			fm.Data(dx+F.data[d]*k);
			fm.Data(-dy+F.data[d+1]*k);
			d+=2;
		}
		else if((C=='m')|(C=='l'))
		{
			fm.Cmd(C);
			fm.Data(F.data[d]*k);
			fm.Data(F.data[d+1]*k);
			d+=2;
		}
		else if(C=='c')
		{
			fm.Cmd(C);
			for(int j=0;j<6;j++)
				fm.Data(F.data[d+j]*k);
			d+=6;
		}
		else if(C=='/')
			d+=2;
		else if(C=='f')
			d++;
	}
}

void SymDot(CFontMaker& fm, float x, float y)
{
	// the dot of '.' and '!'
	SymM(fm,x,y);
	fm.c( .5,0, .5,0,0, .75);
	fm.c(-.5,0,-.5,0,0,-.75);
}

void MakeSymbols(CFontMaker& fm)
{
	// ---- box drawing: light, heavy, double, and single with double
	SymBox(fm,0x2500,0,1,0,1);  // ─
	SymBox(fm,0x2501,0,2,0,2);  // ━
	SymBox(fm,0x2502,1,0,1,0);  // │
	SymBox(fm,0x2503,2,0,2,0);  // ┃
	SymBox(fm,0x250C,0,1,1,0);  // ┌
	SymBox(fm,0x250F,0,2,2,0);  // ┏
	SymBox(fm,0x2510,0,0,1,1);  // ┐
	SymBox(fm,0x2513,0,0,2,2);  // ┓
	SymBox(fm,0x2514,1,1,0,0);  // └
	SymBox(fm,0x2517,2,2,0,0);  // ┗
	SymBox(fm,0x2518,1,0,0,1);  // ┘
	SymBox(fm,0x251B,2,0,0,2);  // ┛
	SymBox(fm,0x251C,1,1,1,0);  // ├
	SymBox(fm,0x2523,2,2,2,0);  // ┣
	SymBox(fm,0x2524,1,0,1,1);  // ┤
	SymBox(fm,0x252B,2,0,2,2);  // ┫
	SymBox(fm,0x252C,0,1,1,1);  // ┬
	SymBox(fm,0x2533,0,2,2,2);  // ┳
	SymBox(fm,0x2534,1,1,0,1);  // ┴
	SymBox(fm,0x253B,2,2,0,2);  // ┻
	SymBox(fm,0x253C,1,1,1,1);  // ┼
	SymBox(fm,0x254B,2,2,2,2);  // ╋
	SymBox(fm,0x2550,0,3,0,3);  // ═
	SymBox(fm,0x2551,3,0,3,0);  // ║
	SymBox(fm,0x2552,0,3,1,0);  // ╒
	SymBox(fm,0x2553,0,1,3,0);  // ╓
	SymBox(fm,0x2554,0,3,3,0);  // ╔
	SymBox(fm,0x2555,0,0,1,3);  // ╕
	SymBox(fm,0x2556,0,0,3,1);  // ╖
	SymBox(fm,0x2557,0,0,3,3);  // ╗
	SymBox(fm,0x2558,1,3,0,0);  // ╘
	SymBox(fm,0x2559,3,1,0,0);  // ╙
	SymBox(fm,0x255A,3,3,0,0);  // ╚
	SymBox(fm,0x255B,1,0,0,3);  // ╛
	SymBox(fm,0x255C,3,0,0,1);  // ╜
	SymBox(fm,0x255D,3,0,0,3);  // ╝
	SymBox(fm,0x255E,1,3,1,0);  // ╞
	SymBox(fm,0x255F,3,1,3,0);  // ╟
	SymBox(fm,0x2560,3,3,3,0);  // ╠
	SymBox(fm,0x2561,1,0,1,3);  // ╡
	SymBox(fm,0x2562,3,0,3,1);  // ╢
	SymBox(fm,0x2563,3,0,3,3);  // ╣
	SymBox(fm,0x2564,0,3,1,3);  // ╤
	SymBox(fm,0x2565,0,1,3,1);  // ╥
	SymBox(fm,0x2566,0,3,3,3);  // ╦
	SymBox(fm,0x2567,1,3,0,3);  // ╧
	SymBox(fm,0x2568,3,1,0,1);  // ╨
	SymBox(fm,0x2569,3,3,0,3);  // ╩
	SymBox(fm,0x256A,1,3,1,3);  // ╪
	SymBox(fm,0x256B,3,1,3,1);  // ╫
	SymBox(fm,0x256C,3,3,3,3);  // ╬
	SymBox(fm,0x2574,0,0,0,1);  // ╴
	SymBox(fm,0x2575,1,0,0,0);  // ╵
	SymBox(fm,0x2576,0,1,0,0);  // ╶
	SymBox(fm,0x2577,0,0,1,0);  // ╷
	SymBox(fm,0x2578,0,0,0,2);  // ╸
	SymBox(fm,0x2579,2,0,0,0);  // ╹
	SymBox(fm,0x257A,0,2,0,0);  // ╺
	SymBox(fm,0x257B,0,0,2,0);  // ╻
	SymDash(fm,0x2504,3,0);     // ┄
	SymDash(fm,0x2506,3,1);     // ┆
	SymDash(fm,0x2508,4,0);     // ┈
	SymDash(fm,0x250A,4,1);     // ┊
	SymDash(fm,0x254C,2,0);     // ╌
	SymDash(fm,0x254E,2,1);     // ╎
	SymArc(fm,0x256D,-1, 1);    // ╭
	SymArc(fm,0x256E,-1,-1);    // ╮
	SymArc(fm,0x256F, 1,-1);    // ╯
	SymArc(fm,0x2570, 1, 1);    // ╰
	{
		fm.StartGlyph(0x2571);  // ╱
		SymLine(fm,-1,-4,5,12);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x2572);  // ╲
		SymLine(fm,-1,12,5,-4);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x2573);  // ╳
		SymLine(fm,-1,-4,5,12);
		SymLine(fm,-1,12,5,-4);
		fm.EndGlyph();
	}

	// ---- blocks and shades
	SymBlock(fm,0x2580,0,0.5,1,1);      // ▀
	SymBlock(fm,0x2581,0,0,1,0.125);    // ▁
	SymBlock(fm,0x2582,0,0,1,0.25);     // ▂
	SymBlock(fm,0x2583,0,0,1,0.375);    // ▃
	SymBlock(fm,0x2584,0,0,1,0.5);      // ▄
	SymBlock(fm,0x2585,0,0,1,0.625);    // ▅
	SymBlock(fm,0x2586,0,0,1,0.75);     // ▆
	SymBlock(fm,0x2587,0,0,1,0.875);    // ▇
	SymBlock(fm,0x2588,0,0,1,1);        // █
	SymBlock(fm,0x2589,0,0,0.875,1);    // ▉
	SymBlock(fm,0x258A,0,0,0.75,1);     // ▊
	SymBlock(fm,0x258B,0,0,0.625,1);    // ▋
	SymBlock(fm,0x258C,0,0,0.5,1);      // ▌
	SymBlock(fm,0x258D,0,0,0.375,1);    // ▍
	SymBlock(fm,0x258E,0,0,0.25,1);     // ▎
	SymBlock(fm,0x258F,0,0,0.125,1);    // ▏
	SymBlock(fm,0x2590,0.5,0,1,1);      // ▐
	SymShade(fm,0x2591,1);              // ░
	SymShade(fm,0x2592,2);              // ▒
	SymShade(fm,0x2593,3);              // ▓
	SymBlock(fm,0x2594,0,0.875,1,1);    // ▔
	SymBlock(fm,0x2595,0.875,0,1,1);    // ▕
	SymQuad(fm,0x2596,4);               // ▖
	SymQuad(fm,0x2597,8);               // ▗
	SymQuad(fm,0x2598,1);               // ▘
	SymQuad(fm,0x2599,1|4|8);           // ▙
	SymQuad(fm,0x259A,1|8);             // ▚
	SymQuad(fm,0x259B,1|2|4);           // ▛
	SymQuad(fm,0x259C,1|2|8);           // ▜
	SymQuad(fm,0x259D,2);               // ▝
	SymQuad(fm,0x259E,2|4);             // ▞
	SymQuad(fm,0x259F,2|4|8);           // ▟

	// ---- geometric shapes
	{
		fm.StartGlyph(0x25A0);  // ■
		fm.clear();
		SymRect(fm,0,2,4,6,0);
		fm.ink();
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x25A1);  // □
		SymRect(fm,0,2,4,6,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x25AA);  // ▪
		fm.clear();
		SymRect(fm,.75,2.75,3.25,5.25,0);
		fm.ink();
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x25AB);  // ▫
		SymRect(fm,.75,2.75,3.25,5.25,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x25AC);  // ▬
		fm.clear();
		SymRect(fm,-.5,3,4.5,5,0);
		fm.ink();
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x25B2);  // ▲
		fm.clear();
		SymTri(fm,0,1,4,1,2,7);
		fm.ink();
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x25B3);  // △
		SymTri(fm,0,1,4,1,2,7);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x25B4);  // ▴
		fm.clear();
		SymTri(fm,.5,2,3.5,2,2,5.5);
		fm.ink();
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x25BA);  // ►
		fm.clear();
		SymTri(fm,0,1,4.5,4,0,7);
		fm.ink();
		fm.EndGlyph();
	}
	fm.Alias(0x25B6,0x25BA);    // ▶
	{
		fm.StartGlyph(0x25B8);  // ▸
		fm.clear();
		SymTri(fm,.5,2.25,3.75,4,.5,5.75);
		fm.ink();
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x25BC);  // ▼
		fm.clear();
		SymTri(fm,0,7,2,1,4,7);
		fm.ink();
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x25BD);  // ▽
		SymTri(fm,0,7,2,1,4,7);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x25BE);  // ▾
		fm.clear();
		SymTri(fm,.5,6,2,2.5,3.5,6);
		fm.ink();
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x25C4);  // ◄
		fm.clear();
		SymTri(fm,4,7,-.5,4,4,1);
		fm.ink();
		fm.EndGlyph();
	}
	fm.Alias(0x25C0,0x25C4);    // ◀
	{
		fm.StartGlyph(0x25C2);  // ◂
		fm.clear();
		SymTri(fm,3.5,5.75,.25,4,3.5,2.25);
		fm.ink();
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x25C6);  // ◆
		fm.clear();
		SymM(fm,2,.5);
		SymL(fm,4.25,4);
		SymL(fm,2,7.5);
		SymL(fm,-.25,4);
		fm.z();
		fm.ink();
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x25C7);  // ◇
		SymM(fm,2,.5);
		SymL(fm,4.25,4);
		SymL(fm,2,7.5);
		SymL(fm,-.25,4);
		fm.z();
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x25CB);  // ○
		SymCircle(fm,2,4,2.5,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x25CF);  // ●
		fm.clear();
		SymCircle(fm,2,4,2.5,0);
		fm.ink();
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x25D8);  // ◘
		fm.clear();
		SymRect(fm,-1,-4,5,12,0);
		SymCircle(fm,2,4,1.5,1);
		fm.ink();
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x25D9);  // ◙
		fm.clear();
		SymRect(fm,-1,-4,5,12,0);
		SymCircle(fm,2,4,2.6,1);
		SymCircle(fm,2,4,1.6,0);
		fm.ink();
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x25E6);  // ◦
		SymCircle(fm,2,4,1.1,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x2022);  // •
		fm.clear();
		SymCircle(fm,2,4,1.5,0);
		fm.ink();
		fm.EndGlyph();
	}

	// ---- arrows
	{
		fm.StartGlyph(0x2190);  // ←
		SymLine(fm,-.5,4,4.5,4);
		SymArrowHead(fm,-.5,4,-1,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x2191);  // ↑
		SymLine(fm,2,0,2,9);
		SymArrowHead(fm,2,9,0,1);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x2192);  // →
		SymLine(fm,-.5,4,4.5,4);
		SymArrowHead(fm,4.5,4,1,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x2193);  // ↓
		SymLine(fm,2,0,2,9);
		SymArrowHead(fm,2,0,0,-1);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x2194);  // ↔
		SymLine(fm,-.5,4,4.5,4);
		SymArrowHead(fm,-.5,4,-1,0);
		SymArrowHead(fm,4.5,4,1,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x2195);  // ↕
		SymLine(fm,2,-.5,2,9);
		SymArrowHead(fm,2,9,0,1);
		SymArrowHead(fm,2,-.5,0,-1);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x21A8);  // ↨
		SymLine(fm,2,.5,2,9);
		SymArrowHead(fm,2,9,0,1);
		SymArrowHead(fm,2,.5,0,-1);
		SymLine(fm,.25,-1,3.75,-1);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x21B5);  // ↵
		SymM(fm,4,8);
		SymL(fm,4,3);
		SymL(fm,0,3);
		SymArrowHead(fm,0,3,-1,0);
		fm.EndGlyph();
	}

	// ---- cards, notes, faces
	{
		fm.StartGlyph(0x2665);  // ♥
		fm.clear();
		SymM(fm,2,.5);
		SymC(fm,1.2,1.6,0,3.4,0,5);
		SymC(fm,0,6.2,.45,7,1,7);
		SymC(fm,1.6,7,2,6.4,2,5.8);
		SymC(fm,2,6.4,2.4,7,3,7);
		SymC(fm,3.55,7,4,6.2,4,5);
		SymC(fm,4,3.4,2.8,1.6,2,.5);
		fm.z();
		fm.ink();
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x2666);  // ♦
		fm.clear();
		SymM(fm,2,.5);
		SymL(fm,4,4);
		SymL(fm,2,7.5);
		SymL(fm,0,4);
		fm.z();
		fm.ink();
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x2663);  // ♣
		fm.clear();
		SymCircle(fm,2,5.8,1.25,0);
		SymCircle(fm,.85,3.6,1.25,0);
		SymCircle(fm,3.15,3.6,1.25,0);
		SymRect(fm,1.3,3.2,2.7,5.2,0);
		SymTri(fm,1.1,0,2.9,0,2,3.5);
		fm.ink();
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x2660);  // ♠
		fm.clear();
		SymM(fm,2,7.5);
		SymC(fm,2.8,6.4,4,5,4,3.4);
		SymC(fm,4,2.4,3.4,1.7,2.8,1.7);
		SymC(fm,2.4,1.7,2.15,1.9,2,2.2);
		SymC(fm,1.85,1.9,1.6,1.7,1.2,1.7);
		SymC(fm,.6,1.7,0,2.4,0,3.4);
		SymC(fm,0,5,1.2,6.4,2,7.5);
		fm.z();
		SymTri(fm,1.1,0,2,3,2.9,0);
		fm.ink();
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x266A);  // ♪
		fm.clear();
		SymOval(fm,1.3,1.2,1.3,.95,0);
		SymRect(fm,2.1,1.2,2.6,8,0);
		SymM(fm,2.6,6.2);
		SymL(fm,4,4.8);
		SymL(fm,4,5.9);
		SymL(fm,2.6,8);
		fm.z();
		fm.ink();
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x266B);  // ♫
		fm.clear();
		SymOval(fm,.8,1,1,.75,0);
		SymOval(fm,3.2,2,1,.75,0);
		SymRect(fm,1.35,1,1.8,6.8,0);
		SymRect(fm,3.75,2,4.2,7.8,0);
		SymM(fm,1.35,6.2);
		SymL(fm,4.2,7.2);
		SymL(fm,4.2,7.8);
		SymL(fm,1.35,6.8);
		fm.z();
		fm.ink();
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x263A);  // ☺
		SymCircle(fm,2,4,2.7,0);
		SymLine(fm,1.1,4.8,1.1,5.4);
		SymLine(fm,2.9,4.8,2.9,5.4);
		SymM(fm,.8,3);
		SymC(fm,1.4,2,2.6,2,3.2,3);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x263B);  // ☻
		fm.clear();
		SymCircle(fm,2,4,2.7,0);
		SymOval(fm,1.1,5,.45,.6,1);
		SymOval(fm,2.9,5,.45,.6,1);
		SymM(fm,.7,3.1);
		SymC(fm,1.4,2.4,2.6,2.4,3.3,3.1);
		SymC(fm,2.7,1.7,1.3,1.7,.7,3.1);
		fm.z();
		fm.ink();
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x263C);  // ☼
		SymCircle(fm,2,4,1.4,0);
		for(int i=0;i<8;i++)
		{
			float a=i*0.785398;
			float c=cos(a);
			float s=sin(a);
			SymLine(fm,2+c*2.1,4+s*2.1,2+c*2.9,4+s*2.9);
		}
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x2640);  // ♀
		SymCircle(fm,2,5.2,2,0);
		SymLine(fm,2,3.2,2,-1);
		SymLine(fm,.8,.8,3.2,.8);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x2642);  // ♂
		SymCircle(fm,1.6,2,1.8,0);
		SymLine(fm,2.87,3.27,4.6,5);
		SymM(fm,3,5);
		SymL(fm,4.6,5);
		SymL(fm,4.6,3.4);
		fm.EndGlyph();
	}

	// ---- signs
	{
		fm.StartGlyph(0x203C);  // ‼
		SymDot(fm,1,0);
		SymLine(fm,1,3,1,9);
		SymDot(fm,3,0);
		SymLine(fm,3,3,3,9);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0xB6);    // ¶
		SymLine(fm,2.5,9,2.5,0);
		SymLine(fm,4,9,4,0);
		SymM(fm,4,9);
		SymL(fm,1.8,9);
		SymC(fm,-.4,9,-.4,5,1.8,5);
		SymL(fm,2.5,5);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0xA7);    // §
		SymM(fm,3.5,8.2);
		SymC(fm,3,9.3,.5,9.2,.5,7.8);
		SymC(fm,.5,6.2,3.6,6,3.6,4.2);
		SymC(fm,3.6,3.3,2.8,2.9,2.2,2.9);
		SymM(fm,.5,.8);
		SymC(fm,1,-.3,3.5,-.2,3.5,1.2);
		SymC(fm,3.5,2.8,.4,3,.4,4.8);
		SymC(fm,.4,5.7,1.2,6.1,1.8,6.1);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x221F);  // ∟
		SymM(fm,.5,6);
		SymL(fm,.5,1);
		SymL(fm,4,1);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x2302);  // ⌂
		SymM(fm,0,0);
		SymL(fm,0,4);
		SymL(fm,2,7);
		SymL(fm,4,4);
		SymL(fm,4,0);
		fm.z();
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x2261);  // ≡
		SymLine(fm,0,1,4,1);
		SymLine(fm,0,4,4,4);
		SymLine(fm,0,7,4,7);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0xB1);    // ±
		SymLine(fm,0,5,4,5);
		SymLine(fm,2,7,2,3);
		SymLine(fm,0,1,4,1);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0xB0);    // °
		SymCircle(fm,2,8,1.1,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0xB7);    // ·
		SymDot(fm,2,3.6);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0xD7);    // ×
		SymLine(fm,.5,2,3.5,6);
		SymLine(fm,.5,6,3.5,2);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0xF7);    // ÷
		SymLine(fm,0,4,4,4);
		SymDot(fm,2,6);
		SymDot(fm,2,1.25);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x221A);  // √
		SymM(fm,0,4);
		SymL(fm,1,4);
		SymL(fm,2,0);
		SymL(fm,4,9);
		SymL(fm,5,9);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x221E);  // ∞
		SymM(fm,2,4);
		SymC(fm,2.6,5.3,4.2,5.3,4.2,4);
		SymC(fm,4.2,2.7,2.6,2.7,2,4);
		SymC(fm,1.4,5.3,-.2,5.3,-.2,4);
		SymC(fm,-.2,2.7,1.4,2.7,2,4);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x2248);  // ≈
		SymM(fm,0,5);
		fm.c(1.5,2,-1.5,-2,4,0);
		SymM(fm,0,2.5);
		fm.c(1.5,2,-1.5,-2,4,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x2260);  // ≠
		SymLine(fm,.5,3,4,3);
		SymLine(fm,.5,5,4,5);
		SymLine(fm,1.25,1,3.25,7);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x2264);  // ≤
		SymM(fm,3.5,7);
		SymL(fm,0,5.25);
		SymL(fm,3.5,3.5);
		SymLine(fm,0,1.75,3.5,1.75);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x2265);  // ≥
		SymM(fm,.5,7);
		SymL(fm,4,5.25);
		SymL(fm,.5,3.5);
		SymLine(fm,.5,1.75,4,1.75);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0xAC);    // ¬
		SymM(fm,0,5);
		SymL(fm,4,5);
		SymL(fm,4,3);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x2310);  // ⌐
		SymM(fm,0,3);
		SymL(fm,0,5);
		SymL(fm,4,5);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0xAB);    // «
		SymM(fm,2,6);
		SymL(fm,0,4);
		SymL(fm,2,2);
		SymM(fm,4,6);
		SymL(fm,2,4);
		SymL(fm,4,2);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0xBB);    // »
		SymM(fm,0,6);
		SymL(fm,2,4);
		SymL(fm,0,2);
		SymM(fm,2,6);
		SymL(fm,4,4);
		SymL(fm,2,2);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x2026);  // …
		SymDot(fm,0,0);
		SymDot(fm,2,0);
		SymDot(fm,4,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x2013);  // –
		SymLine(fm,0,4,4,4);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x2014);  // —
		SymLine(fm,-1,4,5,4);
		fm.EndGlyph();
	}
	fm.Alias(0xA6,'|');         // ¦
	fm.Alias(0x2219,0xB7);      // ∙
	{
		fm.StartGlyph(0xB2);    // ²
		SymCopy(fm,'2',1,5,0.5);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x207F);  // ⁿ
		SymCopy(fm,'n',1,5,0.5);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x2229);  // ∩
		SymM(fm,0,0);
		SymL(fm,0,4);
		SymC(fm,0,7.5,4,7.5,4,4);
		SymL(fm,4,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x2320);  // ⌠ top of a tall integral
		SymM(fm,2,-4);
		SymL(fm,2,8.5);
		SymC(fm,2,10.5,3,11.2,4.2,10.5);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x2321);  // ⌡ bottom of a tall integral
		SymM(fm,2,12);
		SymL(fm,2,-.5);
		SymC(fm,2,-2.5,1,-3.2,-.2,-2.5);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x2713);  // ✓
		SymM(fm,0,4);
		SymL(fm,1.5,1);
		SymL(fm,4.5,7.5);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x2717);  // ✗
		SymLine(fm,.5,6.5,3.5,1);
		SymLine(fm,3.5,7,.5,1.5);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x2610);  // ☐
		SymRect(fm,-.25,.75,4.25,7.25,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x2611);  // ☑
		SymRect(fm,-.25,.75,4.25,7.25,0);
		SymM(fm,.75,4.25);
		SymL(fm,1.75,2.25);
		SymL(fm,3.25,5.75);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(0x2612);  // ☒
		SymRect(fm,-.25,.75,4.25,7.25,0);
		SymLine(fm,.75,1.75,3.25,6.25);
		SymLine(fm,.75,6.25,3.25,1.75);
		fm.EndGlyph();
	}
}

void MakeLetters(CFontMaker& fm)
{
	{
		fm.StartGlyph(' ');
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('!');
		fm.M(2,0);
		fm.c( .5,0, .5,0,0,1);
		fm.c(-.5,0,-.5,0,0,-1);
		fm.M(2,3);
		fm.l(0,6);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('?');
		fm.M(2,0);
		fm.c( .5,0, .5,0,0,1);
		fm.c(-.5,0,-.5,0,0,-1);
		fm.M(2,3);
		fm.c(0,2,0,-2,2,4);
		fm.M(0,7);
		fm.c(0,3,0,3,4,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('"');
		fm.M(1,9);
		fm.l(0,-1);
		fm.l(-1,-2);
		fm.M(3,9);
		fm.l(0,-1);
		fm.l(-1,-2);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('#');
		fm.M(1.25,2);
		fm.l(3.5,0);
		fm.M(1.25,5);
		fm.l(3.5,0);
		fm.M(2,0);
		fm.l(0,7);
		fm.M(4,0);
		fm.l(0,7);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('$');
		fm.M(2,-1);
		fm.l(0,3);
		fm.M(2,9);
		fm.l(0,-3);
		fm.M(0,6);
		fm.c(0,2.5,-.5,2,4,0.5);
		fm.M(0,6);
		fm.c(0,-2,0,2,4,-4);
		fm.M(0,1.5);
		fm.c(.5,-2,0,-2.5,4,.5);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('%');
		fm.M(3,0);
		fm.c( 1.5,0, 1.5,0,0, 2);
		fm.c(-1.5,0,-1.5,0,0,-2);
		fm.M(1,5);
		fm.c( 1.5,0, 1.5,0,0, 2);
		fm.c(-1.5,0,-1.5,0,0,-2);
		fm.M(0,0);
		fm.l(4,7);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('&');
		fm.M(5,0);
		fm.c(-3.5,3,-3.75,1,-2,9);
		fm.c( 1,-.5,2.,1,-1,-3);
		fm.c(-2,-1,0,1.,-2,-4);
		fm.c(1,-4,1,-3,4.,2);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('\'');
		fm.M(3,9);
		fm.l(0,-1);
		fm.l(-1,-2);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('(');
		fm.M(3,10);
		fm.c(-2.75,-3,-2.75,3,0,-11);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(')');
		fm.M(1,10);
		fm.c(2.75,-3,2.75,3,0,-11);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('*');
		fm.M(0,4);
		fm.l(4,0);
		fm.M(1,6);
		fm.l(2,-4);
		fm.M(3,6);
		fm.l(-2,-4);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('+');
		fm.M(0,4);
		fm.l(4,0);
		fm.M(2,6);
		fm.l(0,-4);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(',');
		fm.M(3,1);
		fm.l(0,-1);
		fm.l(-1,-2);
		//fm.C(2,.5,1.5,.5,1.5,-.5,2,-.5);
		//fm.C(2,.5,2.5,.5,2.5,-.5,2,-.5);
		//fm.C(2.5,0,2.5,-1.,2.5,-2.5,1.,-3.);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('-');
		fm.M(1,4);
		fm.l(3,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('.');
		fm.M(2,0);
		fm.c( .5,0, .5,0,0, .75);
		fm.c(-.5,0,-.5,0,0,-.75);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('/');
		fm.M(0,0);
		fm.l(4,9);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('0');
		fm.M(2,0);
		fm.c(-2.75,0,-2.75,0,0, 9);
		fm.c( 2.75,0, 2.75,0,0,-9);
		fm.M(1.25,2);
		fm.l(1.5,5);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('1');
		fm.M(2,0);
		fm.l(0,9);
		fm.M(0,0);
		fm.l(4,0);
		fm.M(0,7);
		fm.l(2,2);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('2');
		fm.M(0,7);
		fm.c(0,2.6,0,2.6,4,0);
		fm.c(0,-3.,0,4,-4,-7);
		fm.l(4,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('3');
		fm.M(.25,7);
		fm.c(0,2.6,0,2.6,3.5,0);
		fm.c(0,-1.5,1,0,-2,-2);
		fm.c(2,0,0,1,2.5,-2.5);
		fm.c(0,-3.,.5,-3,-4.25,-.5);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('4');
		fm.M(4,0);
		fm.l(0,9);
		fm.l(-4,-6);
		fm.l(5,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('5');
		fm.M(4,9);
		fm.l(-4,0);
		fm.l(0,-4.5);
		fm.c(1.,3,0,3,4,-1.5);
		fm.c(0,-3,1,-2.5,-4,-1.5);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('6');
		fm.M(0,2.5);
		fm.c(0,3.5,0,3.5,4,0);
		fm.c(0,-3.5,0,-3.5,-4,0);
		fm.c(-.5,7.5,-1,1,3.5,6);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('7');
		fm.M(0,9);
		fm.l(4,0);
		fm.c(-2,-4,0,2,-2,-9);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('8');
		fm.M(2,9);
		fm.c(-2.25,0,-2.25, 0,0,-4);
		fm.c(-2.75,0,-2.75, 0,0,-5);
		fm.c( 2.75,0, 2.75, 0,0, 5);
		fm.c( 2.25,0, 2.25, 0,0, 4);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('9');
		fm.M(4,6.5);
		fm.c(0,-3.5, 0,-3.5,-4,0);
		fm.c(0, 3.5, 0, 3.5, 4,0);
		fm.c(0,-7.5, 1, -.5,-3.5,-6);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(':');
		fm.M(2,.75);
		fm.l(0,.5);
		fm.M(1.75,1);
		fm.l(.5,0);
		fm.M(2,4.75);
		fm.l(0,.5);
		fm.M(1.75,5);
		fm.l(.5,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(';');
		fm.M(2,1);
		fm.l(0,-1);
		fm.l(-1,-2);
		fm.M(2,4.75);
		fm.l(0,.5);
		fm.M(1.75,5);
		fm.l(.5,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('<');
		fm.M(3.5,2);
		fm.l(-3.5,2);
		fm.l(3.5,2);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('=');
		fm.M(.5,3);
		fm.l(3.5,0);
		fm.M(.5,5);
		fm.l(3.5,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('>');
		fm.M(.5,2);
		fm.l(3.5,2);
		fm.l(-3.5,2);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('@');
		fm.M(.5,4);
		fm.c(2.,.5,0,1,1.5,-3);
		fm.c(0,1.5,0,1.5,-2,0);
		fm.c(0,-1.5,0,-1.5,2,0);
		fm.c(0,-1.5,0,-1.5,2,0);
		fm.c(0,1.5,3,0,-2.5,5.25);
		fm.c(-.5,0,.5,.25,-1.5,-.25);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('A');
		fm.M(0,0);
		fm.l(2.,9);
		fm.l(2.,-9);
		fm.M(1,3);
		fm.l(2.,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('B');
		fm.M(0,5);
		fm.l(2,0);
		fm.M(2,0);
		fm.l(-2,0);
		fm.l(0,9);
		fm.l(1.75,0);
		fm.c(2.75,0,2.75,0,0,-4);
		fm.m(.25,0);
		fm.c(3,0,3,0,0,-5);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('C');
		fm.M(4,7);
		fm.c(-.5,   3,   0,   5,-4,-2.5);
		fm.c(  0,-5.5, -.5,-3.5, 4,-2.5);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('D');
		fm.M(1.5,0);
		fm.l(-1.5,0);
		fm.l(0,9);
		fm.l(1.5,0);
		fm.c(3.5,0,3.5,0,0,-9);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('E');
		fm.M(4,0);
		fm.l(-4,0);
		fm.l(0,9);
		fm.l(4,0);
		fm.M(0,5);
		fm.l(3,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('F');
		fm.M(0,0);
		fm.l(0,9);
		fm.l(4,0);
		fm.M(0,5);
		fm.l(3,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('G');
		fm.M(4,7);
		fm.c(-.5,   3,   0,   5,-4,-2.5);
		fm.c(  0,-5.5, -.5,-3.5, 4,-2.5);
		fm.M(4,0);
		fm.l(0,4);
		fm.l(-2,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('H');
		fm.M(0,0);
		fm.l(0,9);
		fm.M(4,0);
		fm.l(0,9);
		fm.M(0,5);
		fm.l(4,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('I');
		fm.M(1,0);
		fm.l(2,0);
		fm.M(1,9);
		fm.l(2,0);
		fm.M(2,0);
		fm.l(0,9);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('J');
		fm.M(1,9);
		fm.l(3,0);
		fm.M(4,9);
		fm.l(0,-7);
		fm.c(0,-2.75,0,-2.75,-4,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('K');
		fm.M(0,0);
		fm.l(0,9);
		fm.M(0,4);
		fm.l(3.5,5);
		fm.M(1,5.5);
		fm.l(3,-5.5);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('L');
		fm.M(0,9);
		fm.l(0,-9);
		fm.l(4,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('M');
		fm.M(0,0);
		fm.l(0,9);
		fm.l(2,-6);
		fm.l(2,6);
		fm.l(0,-9);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('N');
		fm.M(0,0);
		fm.l(0,9);
		fm.l(4,-9);
		fm.l(0,9);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('O');
		fm.M(2,9);
		fm.c(-2.75,0,-2.75,0,0,-9);
		fm.c( 2.75,0, 2.75,0,0, 9);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('P');
		fm.M(0,4);
		fm.l(2,0);
		fm.M(0,0);
		fm.l(0,9);
		fm.l(2,0);
		fm.c(2.75,0,2.75,0,0,-5);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('Q');
		fm.M(2,9);
		fm.c(-2.75,0,-2.75,0,0,-9);
		fm.c( 2.75,0, 2.75,0,0, 9);
		fm.M(2,2);
		fm.c(0,-1,-1,0,2,-3);
		
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('R');
		fm.M(0,4);
		fm.l(2,0);
		fm.M(0,0);
		fm.l(0,9);
		fm.l(2,0);
		fm.c(2.75,0,2.75,0,0,-5);
		fm.M(1.5,4);
		fm.c(1.5,-.5,-1.5,1,2.75,-4);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('S');
		fm.M(3.5,7.5);
		fm.c(-0,2.,0,2,-3.5,0);
		fm.c(0,-2.5,0,3,4,-5.5);
		fm.c(-.5,-3.5,0,-1.5,-4,-.5);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('T');
		fm.M(2,0);
		fm.l(0,9);
		fm.M(0,9);
		fm.l(4,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('U');
		fm.M(0,9);
		fm.l(0,-7);
		fm.c(0,-2.75,0,-2.75,4,0);
		fm.l(0,7);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('V');
		fm.M(0,9);
		fm.l(2,-9);
		fm.l(2,9);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('W');
		fm.M(0,9);
		fm.l(1,-9);
		fm.l(1,7);
		fm.l(1,-7);
		fm.l(1,9);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('X');
		fm.M(0,9);
		fm.l(4,-9);
		fm.M(0,0);
		fm.l(4,9);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('Y');
		fm.M(0,9);
		fm.l(2,-5);
		fm.l(2,5);
		fm.M(2,4);
		fm.l(0,-4);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('Z');
		fm.M(0,9);
		fm.l(4,0);
		fm.l(-4,-9);
		fm.l(4,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('[');
		fm.M(4,-1);
		fm.l(-2,0);
		fm.l(0,11);
		fm.l(2,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('\\');
		fm.M(4,0);
		fm.l(-4,9);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('^');
		fm.M(0,7);
		fm.l(2,2);
		fm.l(2,-2);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('_');
		fm.M(0,0);
		fm.l(4,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph(']');
		fm.M(0,-1);
		fm.l(2,0);
		fm.l(0,11);
		fm.l(-2,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('`');
		fm.M(2,9);
		fm.l(0,-1);
		fm.l(1,-1);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('a');
		fm.M(0,5.5);
		fm.c(1,1,0,2.5,4,-1.5);
		fm.l(0,-2);
		fm.c(0,-2.5,0,-3,-4,0);
		fm.c(0,3,0,2.5,4,0);
		fm.c(0,-1,-.5,-0,1,-2);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('b');
		fm.M(0,0);
		fm.l(0,9);
		fm.M(0,3);
		fm.c(0,4,0,4,4,0);
		fm.c(0,-4,0,-4,-4,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('c');
		fm.M(4,5);
		fm.c(-.5,1.5,0,3.5,-4,-2);
		fm.c(0,-3.5,-.5,-2,4,-2);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('d');
		fm.M(0,3);
		fm.c(0,4,0,4,4,0);
		fm.c(0,-4,0,-4,-4,0);
		fm.M(4,0);
		fm.l(0,9);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('e');
		fm.M(0,3);
		fm.l(4,0);
		fm.c(0.5,4.,0,4.5,-4,0);
		fm.c(0,-4,-1.,-1.5,4,-2);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('f');
		fm.M(1,6);
		fm.l(3,0);
		fm.M(2,0);
		fm.l(0,8);
		fm.c(0,1.5,-.5,1.5,3.,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('g');
		fm.M(0,4);
		fm.c(0, 2.7, 0, 2.7, 4,0);
		fm.c(0,-2.7, 0,-2.7,-4,0);
		fm.M(0,-1);
		fm.c(0, 1.5,0, 1.5, 4,0);
		fm.c(0,-1.5,0,-1.5,-4,0);
		fm.M(1,2.25);
		fm.c(-1.5,0,-1.5,0,0,-2.25);
		fm.M(3.75,5.25);
		fm.c(.25,.5,-.5,.25,1.25,.5);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('h');
		fm.M(0,0);
		fm.l(0,9);
		fm.M(0,4);
		fm.c(1,2.5,0,3,4,0);
		fm.l(0,-4);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('i');
		fm.M(2,0);
		fm.l(0,6);
		fm.M(1,0);
		fm.l(2,0);
		fm.M(1,6);
		fm.l(1,0);
		fm.M(2,7.75);
		fm.l(0,.5);
		fm.M(1.75,8);
		fm.l(.5,0);
		//fm.C(2,7.9,1.85,7.9,1.85,8.2,2,8.2);
		//fm.C(2,7.9,2.15,7.9,2.15,8.2,2,8.2);
		//fm.M(1.8,7.9);
		//fm.l( .2, .2);
		//fm.l( .2,-.2);
		//fm.l(-.2,-.2);
		//fm.l(-.2, .2);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('j');
		fm.M(2,6);
		fm.l(2,0);
		fm.l(0,-6);
		fm.M(4,0);
		fm.c(0,-2.75,0,-2.75,-4,0);
		fm.M(4,7.75);
		fm.l(0,.5);
		fm.M(3.75,8);
		fm.l(.5,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('l');
		fm.M(2,0);
		fm.l(0,9);
		fm.M(0,0);
		fm.l(4,0);
		fm.M(0,9);
		fm.l(2,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('k');
		fm.M(0,0);
		fm.l(0,9);
		fm.M(0,2);
		fm.L(3.5,6);
		fm.M(1.5,3);
		fm.L(4,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('m');
		fm.M(0,0);
		fm.l(0,6);
		fm.M(0,4.5);
		fm.c(0,1.35,0,1.75,2,.5);
		fm.l(0,-5);
		fm.M(2,4.5);
		fm.c(0,1.35,0,1.75,2,.5);
		fm.l(0,-5);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('n');
		fm.M(0,0);
		fm.l(0,6);
		fm.M(0,4);
		fm.c(1,2.5,0,3,4,0);
		fm.l(0,-4);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('o');
		fm.M(0,3);
		fm.c(0, 4,0, 4, 4,0);
		fm.c(0,-4,0,-4,-4,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('p');
		fm.M(0,6);
		fm.l(0,-9);
		fm.M(0,3);
		fm.c(0, 4,0, 4, 4,0);
		fm.c(0,-4,0,-4,-4,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('q');
		fm.M(4,6);
		fm.l(0,-9);
		fm.M(0,3);
		fm.c(0, 4,0, 4, 4,0);
		fm.c(0,-4,0,-4,-4,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('r');
		fm.M(0,0);
		fm.l(0,6);
		fm.M(0,3);
		fm.c(.5,3,-1,2,4,2);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('s');
		fm.M(4,4.75);
		fm.c(-.5,2.,0,2,-4,-.25);
		fm.c(0,-1.75,0,2,4,-3);
		fm.c(0,-2,0,-2,-4,-.25);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('t');
		fm.M(0,6);
		fm.l(3,0);
		fm.M(1,9);
		fm.l(0,-8);
		fm.c(0,-1.5,-.5,-1.5,2.5,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('u');
		fm.M(0,6);
		fm.l(0,-4);
		fm.c(0,-3,0,-3,4,0);
		fm.l(0,4);
		fm.M(4,2);
		fm.l(0,-2);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('v');
		fm.M(0,6);
		fm.l(2,-6);
		fm.l(2,6);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('w');
		fm.M(0,6);
		fm.l(.5,-6);
		fm.l(1.5,5);
		fm.l(1.5,-5);
		fm.l(.5,6);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('x');
		fm.M(0,6);
		fm.l(4,-6);
		fm.M(0,0);
		fm.l(4,6);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('y');
		fm.M(0,6);
		fm.l(2,-6);
		fm.M(4,6);
		fm.l(-3,-9);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('z');
		fm.M(0,6);
		fm.l(4,0);
		fm.l(-4,-6);
		fm.l(4,0);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('{');
		fm.M(4,-1);
		fm.c(-4,0, 4,0,-4,5);
		fm.c( 4,0,-4,0, 4,5);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('|');
		fm.M(2,0);
		fm.l(0,4);
		fm.M(2,5);
		fm.l(0,4);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('}');
		fm.M(0,-1);
		fm.c( 4,0,-4,0, 4,5);
		fm.c(-4,0, 4,0,-4,5);
		fm.EndGlyph();
	}
	{
		fm.StartGlyph('~');
		fm.M(0,7.5);
		fm.c(1.5,3,-1.5,-3,4,0);
		fm.EndGlyph();
	}
	MakeUkrainian(fm);
	MakeSymbols(fm);
}

