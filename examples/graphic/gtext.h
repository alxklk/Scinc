void DrawGlyph(Graph& g, CFont& f, int s, float x, float y, float sx, float sy)
{
	if(s<0)
	{
		//printf("Negative s %i\n",s);
		return;
	}
	CGlyph* pgl=FindGlyph(f,s);
	if(!pgl)
		return;
	CGlyph gl=*pgl;
	int c=gl.c0;
	int d=gl.d0;
	float* fd=&f.data[0];
	for(int i=0;i<gl.n;i++)
	{
		int C=f.cmds[c+i];
		if(C=='M')
		{
			g.M(x+fd[d]*sx,y+fd[d+1]*sy);
			d+=2;
		}
		else if(C=='m')
		{
			g.m(fd[d]*sx,fd[d+1]*sy);
			d+=2;
		}
		else if(C=='L')
		{
			g.L(x+fd[d]*sx,y+fd[d+1]*sy);
			d+=2;
		}
		else if(C=='l')
		{
			g.l(fd[d]*sx,fd[d+1]*sy);
			d+=2;
		}
		else if(C=='c')
		{
			g.c(
				fd[d  ]*sx,fd[d+1]*sy,
				fd[d+2]*sx,fd[d+3]*sy,
				fd[d+4]*sx,fd[d+5]*sy);
			d+=6;
		}
		else if(C=='z')
		{
			g.close();
		}
		else if(C=='0')
		{
			g.clear();
		}
		else if(C=='/')
		{
			float w=fd[d]*sqrt(sx*sx+sy*sy);
			g.width(w,w);
			g.rgba32(int(fd[d+1]));
			g.fin();
			g.stroke();
			d+=2;
		}
		else if(C=='f')
		{
			g.rgba32(int(fd[d]));
			g.fin();
			g.fill1();
			d++;
		}
	}
}

// next character of UTF-8 text s at i (i moves past it); 0 at the end,
// -1 for a broken sequence
int Utf8Next(char* s, int& i)
{
	int c=s[i];
	if(c<0)
		c+=256;
	if(c==0)
		return 0;
	i++;
	if(c<128)
		return c;
	int n=0;
	if((c&0xE0)==0xC0)
	{
		c=c&0x1F;
		n=1;
	}
	else if((c&0xF0)==0xE0)
	{
		c=c&0x0F;
		n=2;
	}
	else if((c&0xF8)==0xF0)
	{
		c=c&0x07;
		n=3;
	}
	else
		return -1;
	for(int k=0;k<n;k++)
	{
		int b=s[i];
		if(b<0)
			b+=256;
		if((b&0xC0)!=0x80)
			return -1;
		c=(c<<6)|(b&0x3F);
		i++;
	}
	return c;
}

// number of characters (not bytes) in UTF-8 text
int Utf8Len(char* s)
{
	int i=0;
	int n=0;
	while(Utf8Next(s,i)!=0)
		n++;
	return n;
}

void DrawText(Graph& g, CFont& f, char* s, int col, float x, float y, float sx, float sy)
{
	int i=0;
	float cx=x;
	while(true)
	{
		int c=Utf8Next(s,i);
		if(c==0)
			break;
		if(c<0)
			continue;
		CGlyph* pgl=FindGlyph(f,c);
		if(!pgl)
			continue;
		CGlyph gl=*pgl;
		if(
			((!col)&&(!gl.col))||
			(col&&gl.col)
			)
			DrawGlyph(g,f,c,cx,y,sx,sy);
		cx+=gl.w*sx;
	}
}
