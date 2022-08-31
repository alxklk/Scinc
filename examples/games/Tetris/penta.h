struct MLItem
{
	int cmd;
	float x;
	float y;
};

struct StrokeML
{
	int x0;
	int y0;
	int x1;
	int y1;
	int Dir()
	{
		int dx=x1-x0;
		int dy=y1-y0;
		if(dx==0)
		{
			if(dy<0)
				return 0;
			else if(dy>0)
				return 1;
		}
		else if(dy==0)
		{
			if(dx<0)
				return 2;
			else if(dx>0)
				return 3;
		}
		return -1;
	}
};

struct SFillPos
{
	int x;
	int y;
};

class CFillSet
{
public:
	SFillPos p[128*64];
	int n;
	void Add(int x, int y)
	{
		p[n].x=x;
		p[n].y=y;
		n++;
	}
};

struct SCharFiller{
	int W;
	int H;
	char *buf;
	int *mask;
	int fill(int x, int y, CFillSet& bs)
	{
		//printf(" Fill %i %i\n",x,y);
		if(x<0)
			return 0;
		if(y<0)
			return 0;
		if(y>=H)
			return 0;
		if(x>=W)
			return 0;
		if(buf[W*y+x]==' ')
			return 0;
		if(mask[W*y+x])
		{
			//printf("Filled\n");
			return 0;
		}
		if(bs.n>128*64)
			return 0;
		bs.Add(x,y);
		mask[W*y+x]=1;
		return 1+
		fill(x+1, y  , bs)+
		fill(x-1, y  , bs)+
		fill(x  , y+1, bs)+
		fill(x  , y-1, bs);
	}
};

struct Shape
{
	StrokeML sts[1000];
	int ns;
	int cellSize;
	void AddStroke(int x0, int y0, int x1, int y1)
	{
		if(ns>999)
		{
			printf("Stroke Overflow\n");
			return;
		}
		sts[ns].x0=x0*cellSize;
		sts[ns].y0=y0*cellSize;
		sts[ns].x1=x1*cellSize;
		sts[ns].y1=y1*cellSize;
		ns++;
	}
	void Make(char* pm, int w, int h)
	{
		SCharFiller cf;
		CFillSet fs;
		int mask[32*32];
		cf.buf=&(pm[0]);
		cf.mask=&(mask[0]);
		cf.W=w;
		cf.H=h;
		//memset((void*)&(mask[0]),0,w*h);
		for(int i=0;i<w*h;i++)mask[i]=0;
		ns=0;
		for(int y=0;y<h;y++)
		{
			for(int x=0;x<w;x++)
			{
				//int i=y;int j=x;
				fs.n=0;
				int count=cf.fill(x,y,fs);
				//if(count)printf("Count=%i n=%i\n",count, fs.n);
				for(int c=0;c<fs.n;c++)
				{
					int i=fs.p[c].y;int j=fs.p[c].x;
					//printf("%i %i\n",j,i);
					//if(pm[j+i*w]!=' ')
					{
						if((j==0)||((j>0)&&(pm[(j-1)+i*w]==' ')))
						{
							AddStroke(j,i,j,i+1);
						}
						if((i==0)||((i>0)&&(pm[j+(i-1)*w]==' ')))
						{
							AddStroke(j,i,j+1,i);
						}
						if((i==h-1)||((i<h-1)&&(pm[j+(i+1)*w]==' ')))
						{
							AddStroke(j,i+1,j+1,i+1);
						}
						if((j==w-1)||((j<w-1)&&(pm[(j+1)+i*w]==' ')))
						{
							AddStroke(j+1,i,j+1,i+1);
						}
					}
				}
			}
		}

		for(int i=0;i<ns-1;i++)
		{
			int x1=sts[i].x1;
			int y1=sts[i].y1;
			if((sts[i+1].x0==x1)&&(sts[i+1].y0==y1))
			{
				continue;
			}
			if((sts[i+1].x1==x1)&&(sts[i+1].y1==y1))
			{
				float tmp;
				tmp=sts[i+1].x1;
				sts[i+1].x1=sts[i+1].x0;
				sts[i+1].x0=tmp;
				tmp=sts[i+1].y1;
				sts[i+1].y1=sts[i+1].y0;
				sts[i+1].y0=tmp;
				continue;
			}
			for(int j=i+2;j<ns;j++)
			{
				if((sts[j].x0==x1)&&(sts[j].y0==y1))
				{
					StrokeML tmp=sts[i+1];
					sts[i+1]=sts[j];
					sts[j]=tmp;
					break;
				}
				if((sts[j].x1==x1)&&(sts[j].y1==y1))
				{
					{
						float tmp;
						tmp=sts[j].x1;
						sts[j].x1=sts[j].x0;
						sts[j].x0=tmp;
						tmp=sts[j].y1;
						sts[j].y1=sts[j].y0;
						sts[j].y0=tmp;
					}
					StrokeML tmp=sts[i+1];
					sts[i+1]=sts[j];
					sts[j]=tmp;
					break;
				}
			}
		}
	}

	void Shrink()
	{
		for(int i=0;i<ns;i++)
		{
			if(sts[i].Dir()==0)
			{
				if(i>0)
				{
					sts[i-1].x1-=1;
				}
				else
				{
					sts[ns-1].x1-=1;
				}
				sts[i].x0-=1;
				sts[i].x1-=1;
				if(i<ns-1)
				{
					sts[i+1].x0-=1;
				}
				else
				{
					sts[0].x0-=1;
				}
			}

			if(sts[i].Dir()==3)
			{
				if(i>0)
				{
					sts[i-1].y1-=1;
				}
				else
				{
					sts[ns-1].y1-=1;
				}
				sts[i].y0-=1;
				sts[i].y1-=1;
				if(i<ns-1)
				{
					sts[i+1].y0-=1;
				}
				else
				{
					sts[0].y0-=1;
				}
			}

		}
	}

	void Optimize()
	{
		int i=0;

		i=0;
		while(true)
		{
			if(i>=(ns-1))
				break;
			if(
				(sts[i].x1==sts[i+1].x0)
				&&(sts[i].y1==sts[i+1].y0)
				&&(sts[i].Dir()==sts[i+1].Dir())
				)
			{
				sts[i].x1=sts[i+1].x1;
				sts[i].y1=sts[i+1].y1;
				for(int j=i+1;j<ns;j++)
				{
					sts[j]=sts[j+1];
				}
				ns--;
			}
			else
			{
				i++;
			}
		}


		i=0;
		while(true)
		{
			if(i>=(ns-1))
				break;
			for(int j=i+1;j<ns;j++)
			{
				if(
					(sts[i].x0==sts[j].x1)
					&&(sts[i].y0==sts[j].y1)
					&&(sts[i].Dir()==sts[j].Dir())
					)
				{
					sts[i].x0=sts[j].x0;
					sts[i].y0=sts[j].y0;
					for(int k=j;k<ns-1;k++)
					{
						sts[k]=sts[k+1];
					}
					ns--;
				}
			}
			i++;
		}
	}

	int MakeML(MLItem* buf, int maxN)
	{
		int n=0;
		for(int i=0;i<ns;i++)
		{
			if((i==0)||((sts[i].x0!=sts[i-1].x1)||(sts[i].y0!=sts[i-1].y1)))
			{
				if(n<maxN-1)
				{
					buf[n].cmd='M';buf[n].x=sts[i].x0;buf[n].y=sts[i].y0;
					n++;
				}
				if(n<maxN-1)
				{
					buf[n].cmd='L';buf[n].x=sts[i].x1;buf[n].y=sts[i].y1;
					n++;
				}
			}
			else
			{
				if(n<maxN-1)
				{
					buf[n].cmd='L';buf[n].x=sts[i].x1;buf[n].y=sts[i].y1;
					n++;
				}
			}
		}
		return n;
	}
};

void ShrinkR(StrokeML* strokes, int N, int r)
{
	for(int i=0;i<N;i++)
	{
		
		if(strokes[i].Dir()==0)
		{
			if(i>0)
			{
				strokes[i-1].x1-=r;
			}
			else
			{
				strokes[N-1].x1-=r;
			}
			strokes[i].x0-=r;
			strokes[i].x1-=r;
			if(i<N-1)
			{
				strokes[i+1].x0-=r;
			}
			else
			{
				strokes[0].x0-=r;
			}
		}

		if(strokes[i].Dir()==1)
		{
			if(i>0)
			{
				strokes[i-1].x1+=r;
			}
			else
			{
				strokes[N-1].x1+=r;
			}
			strokes[i].x0+=r;
			strokes[i].x1+=r;
			if(i<N-1)
			{
				strokes[i+1].x0+=r;
			}
			else
			{
				strokes[0].x0+=r;
			}
		}

		
		if(strokes[i].Dir()==2)
		{
			if(i>0)
			{
				strokes[i-1].y1+=r;
			}
			else
			{
				strokes[N-1].y1+=r;
			}
			strokes[i].y0+=r;
			strokes[i].y1+=r;
			if(i<N-1)
			{
				strokes[i+1].y0+=r;
			}
			else
			{
				strokes[0].y0+=r;
			}
		}
		

		if(strokes[i].Dir()==3)
		{
			if(i>0)
			{
				strokes[i-1].y1-=r;
			}
			else
			{
				strokes[N-1].y1-=r;
			}
			strokes[i].y0-=r;
			strokes[i].y1-=r;
			if(i<N-1)
			{
				strokes[i+1].y0-=r;
			}
			else
			{
				strokes[0].y0-=r;
			}
		}
	}
}

int ShiftedContour(char* pm, int w, int h, MLItem* mls, int cellsize, int r)
{
	Shape sh;
	sh.cellSize=1;
	sh.Make(pm, w, h);
	sh.Optimize();
	
	int sstart=0;
	for(int i=1;i<sh.ns;i++)
	{
		if((i==sh.ns-1)||((sh.sts[i].x0!=sh.sts[i-1].x1)||(sh.sts[i].y0!=sh.sts[i-1].y1)))
		{
			int n=i-sstart;
			if(i==sh.ns-1)
				n++;
			StrokeML* ps=(StrokeML*)&(sh.sts[sstart]);
			bool reverse=false;
			int x=ps[0].x0;
			int y=ps[0].y0;
			int p=' ';
			if((x>=0)&&(y>=0)&&(x<w)&&(y<h))p=pm[x+y*w];

			if(ps[0].Dir()==1)
			{
				if(p==' ')reverse=true;
			}
			if(ps[0].Dir()==3)
			{
				if(p!=' ')
					reverse=true;
			}
			if(reverse)
			{
				for(int k=0;k<n;k++)
				{
					int tmp;
					tmp=ps[k].x0;ps[k].x0=ps[k].x1;ps[k].x1=tmp;
					tmp=ps[k].y0;ps[k].y0=ps[k].y1;ps[k].y1=tmp;
				}
				for(int k=0;k<n/2;k++)
				{
					StrokeML tmp;
					tmp=ps[k];ps[k]=ps[n-k-1];ps[n-k-1]=tmp;
				}
			}
			for(int k=0;k<n;k++)
			{
				ps[k].x0*=cellsize;
				ps[k].y0*=cellsize;
				ps[k].x1*=cellsize;
				ps[k].y1*=cellsize;
			}
			ShrinkR(ps, n, r);
			sstart=i;
		}
	}
	int nml=sh.MakeML(mls,1024);
	return nml;
}