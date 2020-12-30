#include <stdio.h>

#define G_SCREEN_WIDTH 600
#define G_SCREEN_HEIGHT 800
#define G_SCREEN_SCALE 1
#define G_SCREEN_MODE 1

#include "graphics.h"

Graph g;

void RoundRect(float x, float y, float w, float h, float r)
{
	g.M(x,y+r);
	g.a(r,r,0,0,1,r,-r);
	g.l(w-r*2,0);
	g.a(r,r,0,0,1,r,r);
	g.l(0,h-r*2);
	g.a(r,r,0,0,1,-r,r);
	g.l(-w+r*2,0);
	g.a(r,r,0,0,1,-r,-r);
	g.close();
}

void FullScreenRect()
{
	g.M(0,0);
	g.l(4000,0);
	g.l(0,4000);
	g.l(-4000,0);
	g.close();
}

int field[5*5];
float sizes[5*5];
float deltax[5*5];
float deltay[5*5];
int hoverX=-1;
int hoverY=-1;
int font=-1;

float cellSize=100;

void DrawField()
{
	g.t_0(0,0);
	g.t_x(1,0);
	g.t_y(0,1);
	double T=Time();
	g.clear();
	FullScreenRect();
	g.fin();
	g.rgb(.9,.95,.97);
	g.alpha(1.);
	g.fill1();
	
	g.clear();
	RoundRect(45.5,245.5,512.,512.,5);
	g.fin();
	g.rgb(.5,.6,.7);
	g.fill1();

	g.clear();
	for(int i=0;i<5;i++)
	{
		for(int j=0;j<5;j++)
		{
			RoundRect(55.5+j*100,255.5+i*100,92.,92.,5);
		}
	}
	g.fin();
	g.rgb(.7,.74,.78);
	g.fill1();
	
	if((hoverX>=0)&&(hoverX<5)&&(hoverY>=0)&&(hoverY<5))
	{
		g.clear();
		RoundRect(55.5+hoverX*cellSize,255.5+hoverY*cellSize,92.,92.,5);
		g.fin();
		g.rgb(1,1,0);
		g.width(2,2);
		g.stroke();
	}

	g.VSetFont(font);

	for(int i=0;i<5;i++)
	{
		for(int j=0;j<5;j++)
		{
			int n=1<<field[j+i*5];
			if(n>1)
			{
				g.clear();
				g.t_0(0,0);
				g.t_x(1,0);
				g.t_y(0,1);
				float size=sizes[j+i*5];
				float dx=deltax[j+i*5]*100.;
				float dy=deltay[j+i*5]*100.;
				RoundRect(55.5+j*100+dx+50*(1-size),255.5+i*100+dy+50*(1-size),92.*size,92.*size,5);
				g.fin();
				if     (n==   1)g.rgb(.95,.95,.95);
				else if(n==   2)g.rgb(.9,.97,1.);
				else if(n==   4)g.rgb(.8,.95,1.);
				else if(n==   8)g.rgb(.6,.8,1.);
				else if(n==  16)g.rgb(.4,.6,1.);
				else if(n==  32)g.rgb(.2,.4,1.);
				else if(n==  64)g.rgb(.1,.3,1.);
				else if(n== 128)g.rgb(.5,.9,1.);
				else if(n== 256)g.rgb(.4,.9,1.);
				else if(n== 512)g.rgb(.1,.8,1.);
				else if(n==1024)g.rgb(.0,.95,1.);
				else if(n==2048)g.rgb(.0,1.,1.);
				else g.rgb(1.,1.,1.);
				g.fill1();
				if(n==2048)
				{
					g.rgb(1.,1.,1.);
					g.width(10,1);
					g.stroke();
				}
				else if(n==1024)
				{
					g.rgb(.5,1.,1.);
					g.width(5,1);
					g.stroke();
				}
				g.clear();
			}
		}
	}

	for(int i=0;i<5;i++)
	{
		for(int j=0;j<5;j++)
		{
			int n=1<<field[j+i*5];
			if(n>1)
			{
				float scx=1.;
				float scy=1.;
				float dx=0.;
				float dy=0.;
				if(n>1000)    {scx=.4 ;dx=-15;scy=.5;dy=-10.;}
				else if(n>100){scx=.45;dx= -8;scy=.6;dy= -8.;}
				else if(n>10) {scx=.6 ;dx= -5;scy=.8;dy= -5.;}
				g.fill1();
				g.clear();
				float size=sizes[j+i*5];
				scx*=size;
				scy*=size;
				dx+=deltax[j+i*5]*100.;
				dy+=deltay[j+i*5]*100.;
				g.t_0(83+j*100+dx+25*(1-size),325+i*100+dy-25*(1-size));
				g.t_x(.07*scx,0);
				g.t_y(0,-.07*scy);
				char s[16];
				snprintf(s,15,"%i",n);
				g.VText(s);
				g.rgb(.15,.15,.15);
				g.fin();
				g.fill1();
				g.clear();
			}
		}
	}

	// for(int i=0;i<5;i++)
	// {
	// 	for(int j=0;j<5;j++)
	// 	{
	// 		int dx=deltax[j+i*5]*100.;
	// 		int dy=deltay[j+i*5]*100.;
	// 		char s[16];
	// 		snprintf(s,15,"%i,%i",dx,dy);
	// 		stext(s,83+j*100,325+i*100,0xff00ff00);
	// 	}
	// }


	g.clear();
	g.t_0(50,90);
	g.t_x(.075,0);
	g.t_y(0,-.075);
	g.VText("2048");
	g.t_0(50,125);
	g.t_x(.029,0);
	g.t_y(0,-.028);
	g.VText("Join the tiles!");
	g.fin();
	g.rgb(.2,.1,.1);
	g.fill1();

}

int Left()
{
	int result=0;
	for(int i=0;i<5;i++)
	{
		int prevn=-1;
		int previ=-1;
		int dest=0;
		for(int j=0;j<5;j++)
		{
			int cur=field[j+i*5];
			if(cur)
			{
				field[j+i*5]=0;
				if(cur==prevn)
				{
					field[previ+i*5]++;
					result++;
					deltax[previ+i*5]=j-previ;
					previ=-1;
					prevn=-1;
				}
				else
				{
					if(dest!=j)result++;
					field[dest+i*5]=cur;
					deltax[dest+i*5]=j-dest;
					previ=dest;
					prevn=cur;
					dest++;
				}
			}
		}
	}
	return result;
}

int Right()
{
	int result=0;
	for(int i=0;i<5;i++)
	{
		int prevn=-1;
		int previ=-1;
		int dest=4;
		for(int j=4;j>=0;j--)
		{
			int cur=field[j+i*5];
			if(cur)
			{
				field[j+i*5]=0;
				if(cur==prevn)
				{
					field[previ+i*5]++;
					result++;
					deltax[previ+i*5]=j-previ;
					previ=-1;
					prevn=-1;
				}
				else
				{
					if(dest!=j)result++;
					field[dest+i*5]=cur;
					deltax[dest+i*5]=j-dest;
					previ=dest;
					prevn=cur;
					dest--;
				}
			}
		}
	}
	return result;
}

int Up()
{
	int result=0;
	for(int j=0;j<5;j++)
	{
		int prevn=-1;
		int previ=-1;
		int dest=0;
		for(int i=0;i<5;i++)
		{
			int cur=field[j+i*5];
			if(cur)
			{
				field[j+i*5]=0;
				if(cur==prevn)
				{
					field[j+previ*5]++;
					result++;
					deltay[j+previ*5]=i-previ;
					previ=-1;
					prevn=-1;
				}
				else
				{
					if(dest!=i)result++;
					field[j+dest*5]=cur;
					deltay[j+dest*5]=i-dest;
					previ=dest;
					prevn=cur;
					dest++;
				}
			}
		}
	}
	return result;
}

int Down()
{
	int result=0;
	for(int j=0;j<5;j++)
	{
		int prevn=-1;
		int previ=-1;
		int dest=4;
		for(int i=4;i>=0;i--)
		{
			int cur=field[j+i*5];
			if(cur)
			{
				field[j+i*5]=0;
				if(cur==prevn)
				{
					field[j+previ*5]++;
					result++;
					deltay[j+previ*5]=i-previ;
					previ=-1;
					prevn=-1;
				}
				else
				{
					if(dest!=i)result++;
					field[j+dest*5]=cur;
					deltay[j+dest*5]=i-dest;
					previ=dest;
					prevn=cur;
					dest--;
				}
			}
		}
	}
	return result;
}

int irand(int& seed)
{
	seed=(seed*1103515245+12345)&0x7ffffff;
	return seed;
}

int seed=2342345;

int AddRand()
{
	int result=0;
	int count=(irand(seed)/13)%2+1;
	for(int i=0;i<count;i++)
	{
		int dests[25];
		int nDest=0;
		for(int i=0;i<25;i++)
		{
			if(field[i]==0)
			{
				dests[nDest]=i;
				nDest++;
			}		
		}

		if(nDest>0)
		{
			int rd=dests[(irand(seed)/17)%nDest];
			field[rd]=(irand(seed)/13)%2+1;
			result++;
			float t0=Time();
			while(true)
			{
				float t1=Time();
				float dt=t1-t0;
				if(CountKeyEvents()>1)
				{
					sizes[rd]=1;
					break;
				}
				if(dt>.15)
				{
					sizes[rd]=1;
					break;
				}
				sizes[rd]=dt/.15;
				DrawField();
				Present();
			}
		}	
	}
	return result;
}

float Fabs(float x){return x<0?-x:x;}
float Sign(float x){return x<0?-1:1;}
float Fmin(float x, float y){return x<y?x:y;}

void Toss()
{
	float t0=Time();
	while(true)
	{
		if((Time()-t0)>0.5)break;
		bool moved=false;
		for(int i=0;i<5*5;i++)
		{
			{
				float fd=Fabs(deltax[i]);
				if(fd>=0.5)
				{
					fd=0.5;
					if(deltax[i]>0)deltax[i]-=fd;
					else deltax[i]+=fd;
					moved=true;
				}
				else
				{
					deltax[i]=0;
				}
			}
			{
				float fd=Fabs(deltay[i]);
				if(fd>=0.5)
				{
					fd=0.5;
					if(deltay[i]>0)deltay[i]-=fd;
					else deltay[i]+=fd;
					moved=true;
				}
				else
				{
					deltay[i]=0;
				}
			}
		}
		DrawField();
		Present();
		if(!moved)break;
		if(CountKeyEvents()>1)
		{
			break;
		}
	}
	for(int i=0;i<5*5;i++)
	{
		deltax[i]=0;
		deltay[i]=0;
	}
}


int Key(int key)
{
	int result=0;
	//printf("%i\n",key);
	if(key==4001)
	{
		//printf("Down\n");
		result=Down();
		Toss();
	}
	else if(key==4002)
	{
		//printf("Left\n");
		result=Left();
		Toss();
	}
	else if(key==4003)
	{
		//printf("Right\n");
		result=Right();
		Toss();
	}
	else if(key==4000)
	{
		//printf("Up\n");
		result=Up();
		Toss();
	}
	else if(key==4008)
	{
		//printf("Clear\n");
		for(int i=0;i<25;i++)field[i]=0;
	}
	//printf("Result: %i\n", result);
	if(result)
		AddRand();
	return result;
}

int main()
{
	font=g.LoadTTFont("Rubik-Regular.ttf");

	int timeSeed=Time()*1e9;
	printf("%i\n", timeSeed);
	seed+=timeSeed;

	for(int i=0;i<25;i++)
	{
		field[i]=0;
		sizes[i]=1;
		deltax[i]=0;
		deltay[i]=0;
	}

	int prevMB=0;
	AddRand();
	while(true)
	{
		int mb;
		int mx;
		int my;
		GetMouseState(mx,my,mb);
		hoverX=(mx-55.5)/100;
		hoverY=(my-255.5)/100;
		if((hoverX>=0)&&(hoverX<5)&&(hoverY>=0)&&(hoverY<5))
		{
			if(((prevMB&1)==0)&&((mb&1)==1))
			{
				field[hoverX+hoverY*5]++;
			}
			if(((prevMB&2)==0)&&((mb&2)==1))
			{
				field[hoverX+hoverY*5]--;
				if(field[hoverX+hoverY*5]<0)field[hoverX+hoverY*5]=0;
			}
		}
		prevMB=mb;

		int key;
		int press;
		while(GetKeyEvent(key,press))
		{
			if(press>0)
			{
				Key(key);
			}
		}
		DrawField();
		Present();
	}
	return 0;
}