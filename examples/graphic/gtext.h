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

void DrawText(Graph& g, CFont& f, char* s, int col, float x, float y, float sx, float sy)
{
	int i=0;
	float cx=x;
	while(true)
	{
		int c=s[i];
		if(c<0)
		{
			//printf(" c<0\n");
			i++;
			continue;
		}
		if(c==0)
			break;
		if(
			((!col)&&(!f.g[c].col))||
			(col&&f.g[c].col)
			)
			DrawGlyph(g,f,c,cx,y,sx,sy);
		cx+=f.g[c].w*sx;
		i++;
	}
}
