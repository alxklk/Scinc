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

struct Shape
{
	StrokeML sts[100];
	int ns;
	int cellSize;
	void AddStroke(int x0, int y0, int x1, int y1)
	{
		if(ns>99)
		{
			//printf("Overflow\n");
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
		ns=0;
		for(int i=0;i<h;i++)
		{
			for(int j=0;j<w;j++)
			{
				if(
					(pm[j+i*w]!=' ')&&(
						(j<1)||((j>=1)&&(pm[(j-1)+i*w]==' '))
						)
					)
				{
					AddStroke(j,i,j,i+1);
				}
				if((j>=(w-1))&&(pm[(j)+i*w]!=' '))
				{
					AddStroke(j+1,i,j+1,i+1);
				}
				if((pm[(j)+i*w]!=' ')&&((j<(w-1))&&(pm[(j+1)+i*w]==' ')))
				{
					AddStroke(j+1,i,j+1,i+1);
				}
				if((pm[j+i*w]!=' ')&&((i<1)||(pm[j+(i-1)*w]==' ')))
				{
					AddStroke(j,i,j+1,i);
				}
				if((i>=(h-1))&&(pm[j+i*w]!=' '))
				{
					AddStroke(j,i+1,j+1,i+1);
				}
				if((pm[(j)+i*w]!=' ')&&((i<(h-1))&&(pm[j+(i+1)*w]==' ')))
				{
					AddStroke(j,i+1,j+1,i+1);
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
					sts[ns-5].x1-=1;
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

