#include "graphics.h"
#include "../../ws.h"

#pragma STACK_SIZE 512000

#define M_PI 3.141592654

int rseed;

int irand(int& seed)
{
	seed=(seed*1103515245+12345)%0x7ffffff;
	return seed;
}

float mod(float a, float b)
{
	int r=a/b;
	return a-b*r;
}


Graph g;

void Rect(float x, float y ,float w, float h)
{
	g.M(x,y);g.l(w,0);g.l(0,h);g.l(-w,0);g.close();
}

void Circle(float x, float y, float r)
{
	g.M(x-r,y);
	g.a(1,1,0,0,1,2*r,0);
	g.a(1,1,0,0,1,-2*r,0);
	g.close();
}


int mx;
int my;
int mb;
int prevmb;
int prevmx;
int prevmy;

int cursnd;

#define NOTESIZE 20
#define FW 32
#define FH 24

class Editor
{
public:
	int* field;
	int x;
	int y;
	int hoverx;
	int hovery;
	bool put;
	int& F(int x, int y)
	{
		return field[(x+FW)%FW+FW*((y+FH)%FH)];
	}
	void Init()
	{
		x=0;
		y=0;
		put=false;
		field=(int*)malloc(sizeof(int)*FW*FH);
		for(int j=0;j<FW;j++)
		{
			for(int i=0;i<FH;i++)
			{
				field[i*FW+j]=(irand(rseed)&3)==3;
			}
		}
	}
	void Edit()
	{
		hoverx=hovery=-1;
		int sx=(mx-x)/NOTESIZE;
		int sy=(my-y)/NOTESIZE;
		if((sx>=0)&&(sx<FW)&&(sy>=0)&&(sy<FH))
		{
			hoverx=sx;
			hovery=sy;
			if((((mb&1)==1)&&((prevmb&1)==0)))
			{
				put=!(field[sx+sy*FW]&1);
				field[sx+sy*FW]^=1;
			}
			else if((((mb&1)==1)&&((mx!=prevmx)||(my!=prevmy))))
			{
				if(put)
					field[sx+sy*FW]|=1;
				else
					field[sx+sy*FW]&=(~1);
			}
		}
	}

	void Update()
	{
		for(int i=0;i<FW*FH;i++)
		{
			field[i]=(field[i]<<1);
		}
		for(int j=0;j<FW;j++)
		{
			for(int i=0;i<FH;i++)
			{
				int cnt=
					(F(j-1,i-1)&2)+(F(j  ,i-1)&2)+(F(j+1,i-1)&2)+
					(F(j-1,i  )&2)               +(F(j+1,i  )&2)+
					(F(j-1,i+1)&2)+(F(j  ,i+1)&2)+(F(j+1,i+1)&2)
					;
				
				cnt/=2;
				int cell=field[j+i*FW];
				if(cnt==3)
				{
					cell|=1;
				}
				else if((cnt<2)||(cnt>3))
				{
					cell&=~1;
				}
				else
				{
					cell|=((cell&2)>>1);
				}
				field[j+i*FW]=cell;
			}
		}

	}
	void Draw()
	{
		g.clear();
		g.M(x,y);
		g.l(FW*NOTESIZE,0);
		g.l(0,FH*NOTESIZE);
		g.l(-FW*NOTESIZE,0);
		g.close();
		g.fin();
		g.rgb(1,1,1);
		g.alpha(.5);
		g.fill1();
		g.width(1,1);
		g.rgb(.5,.8,1.0);
		g.stroke();

		g.clear();
		for(int i=1;i<FH;i++)
		{
			g.M(x,y+i*NOTESIZE+.5);
			g.l(FW*NOTESIZE,0);
		}
		for(int i=1;i<FW;i++)
		{
			g.M(x+i*NOTESIZE+.5,y);
			g.l(0,FH*NOTESIZE);
		}
		g.fin();
		g.width(1.,1.);
		g.rgb(.8,.0,0.3);
		g.stroke();

		bool hasItem=false;
		g.alpha(1);
		g.clear();
		for(int j=0;j<FW;j++)
		{
			for(int i=0;i<FH;i++)
			{
				if(field[j+i*FW]&1)
				{
					Circle(j*NOTESIZE+NOTESIZE/2+x+.5,i*NOTESIZE+NOTESIZE/2+y+.5,NOTESIZE/2-1);
					hasItem=true;
				}
			}
		}
		if(hasItem)
		{
			g.fin();
			g.rgb(.4,.1,0);
			g.fill1();

			g.clear();
			for(int j=0;j<FW;j++)
			{
				for(int i=0;i<FH;i++)
				{
					if(field[j+i*FW]&1)
					{
						g.M(j*NOTESIZE+NOTESIZE/2+x-1,i*NOTESIZE+NOTESIZE/2+y-1);
					}
				}
			}
			g.fin();
			g.rgb(1.,.5,.3);
			g.width(NOTESIZE/2-1,1);
			g.stroke();

			g.clear();
			for(int j=0;j<FW;j++)
			{
				for(int i=0;i<FH;i++)
				{
					if(field[j+i*FW]&1)
					{
						g.M(j*NOTESIZE+NOTESIZE/2+x-1.5,i*NOTESIZE+NOTESIZE/2+y-1.5);
					}
				}
			}
			g.fin();
			g.rgb(1,1,1);
			g.width(1.7,.7);
			g.stroke();
		}

		if((hoverx>=0)&&(hovery>=0))
		{
			g.clear();
			Rect(hoverx*NOTESIZE+x,hovery*NOTESIZE+y,NOTESIZE,NOTESIZE);
			g.fin();
			g.width(2.,2.);
			g.rgba(0,1,1,.5);
			g.stroke();
		}

	}
};


int main()
{
	float tframe;
	tframe=Time();
	float dt;
	dt=0.;
	cursnd=0;
	prevmb=0;
	prevmx=0;
	prevmy=0;
	rseed=21397862;

	Editor ed;
	ed.Init();

	bool doUpdate=false;
	bool doUpdateOnce=false;

	while(true)
	{
		g.gray(0);
		g.clear();
		g.fill1();
		g.t_0(0,0);
		g.t_x(1,0);
		g.t_y(0,1);
		g.clear();

		prevmb=mb;
		prevmx=mx;
		prevmy=my;
		GetMouseState(mx, my, mb);
		ed.Edit();

		if(doUpdate||doUpdateOnce)
			ed.Update();
		doUpdateOnce=false;

		ed.Draw();

		int key;
		int press;
		if(GetKeyEvent(key,press))
		{
			if(press>0)
			{
				if(key==4010)
				{
					doUpdate=!doUpdate;
				}
				if(key==32)
				{
					doUpdateOnce=true;
				}
				printf("Key pressed %i %i\n", key, press);
			}
		}

		Present();
	}
	return 0;
}