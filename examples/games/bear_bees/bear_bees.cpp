#include "graphics.h"
#include "svg.h"

Graph g;

int irand(int& seed)
{
	seed=(seed*1103515245+12345)&0x7ffffff;
	return seed;
}

float frand(int& seed)
{
	return (irand(seed))/134217727.0;
}

float X(float x, float y)
{
	return 320+(x-4)*48.*3./(2.+1.-y/8.);
}

float Y(float x, float y)
{
	return 140+y*24*3./(2.+1.-y/8.);
}

void Draw(int svg, float x, float y)
{
	x+=.5;
	y+=.75;
	float s=(X(x+.5,y)-X(x-.5,y))*.05;
	SVGSetTransform(X(x,y),Y(x,y),s,0,0,s);
	SVGDraw(svg);
}

void DrawAnimated(int svg, int anim, float x, float y, float t)
{
	x+=.5;
	y+=.75;
	float s=(X(x+.5,y)-X(x-.5,y))*.05;
	SVGSetTransform(X(x,y),Y(x,y),s,0,0,s);
	SVGDrawAnimated(svg,anim,t);
}

int main()
{
	int bg=SVGLoad("bg.svg");
	int No=SVGLoad("no.svg");
	int bear=SVGLoad("bear.svg","bear.cen");
	int bearAnim=SVGLoadAnimation(bear,"bear.ami");
	int bee=SVGLoad("honeybee.svg","honeybee.cen");
	int beeAnim=SVGLoadAnimation(bee,"honeybee.ami");
	int hive=SVGLoad("beehive.svg","beehive.cen");
	int hiveAnim=SVGLoadAnimation(hive,"beehive.ami");
	int grass=SVGLoad("grass.svg");
	int bush=SVGLoad("bush.svg");

	int field[64]={};
	int prevmb=0;
	int seed=1734934523;
	int next=irand(seed)%5+1;

	while(true)
	{
		float t=Time();
		SVGSetTransform(0,0,1,0,0,1);
		SVGDraw(bg);

		g.t_0(0,0);
		g.t_x(1,0);
		g.t_y(0,1);
		g.clear();
		for(int i=0;i<8;i++)
		for(int j=i&1;j<8;j+=2)
		{
			g.M(X(i,j),Y(i,j));
			g.l(X(i+1,j)-X(i,j),0);
			g.l(X(i+1,j+1)-X(i+1,j),Y(i+1,j+1)-Y(i+1,j));
			g.l(X(i,j+1)-X(i+1,j+1),Y(i,j+1)-Y(i+1,j+1));
			g.close();
		}
		g.fin();
		g.rgb(0,0,0);
		g.alpha(.1);
		g.fill1();
		g.clear();
		g.M(X(0,0),Y(0,0));
		g.l(X(0,8)-X(0,0),Y(0,8)-Y(0,0));
		g.l(X(8,8)-X(0,8),Y(8,8)-Y(0,8));
		g.l(X(8,0)-X(8,8),Y(8,0)-Y(8,8));
		g.close();
		g.fin();
		g.alpha(.25);
		g.width(2,1);
		g.stroke();

		int mx;
		int my;

		int mb;
		GetMouseState(mx, my, mb);
		mb=mb&1;
		bool click=false;
		if((prevmb==0)&&(mb==1))
		{
			click=true;
		}
		prevmb=mb;
		float oy=(24.*my-3360.)/(my+436.);
		float ox=-((mx-320.)*oy-24.*mx+3072.)/1152.;
		int ix=0;
		int iy=0;
		bool hover=false;
		bool canplace=false;
		if((ox>0)&&(oy>0)&&(ox<8)&&(oy<8))
		{
			ix=ox;
			iy=oy;
			hover=true;
			if(field[ix+iy*8]==0)
				canplace=true;
			g.clear();
			g.M(X(ix  ,iy),  Y(ix,  iy));
			g.l(X(ix+1,iy)  -X(ix,  iy),0);
			g.l(X(ix+1,iy+1)-X(ix+1,iy),  Y(ix+1,iy+1)-Y(ix+1,iy));
			g.l(X(ix  ,iy+1)-X(ix+1,iy+1),Y(ix  ,iy+1)-Y(ix+1,iy+1));
			g.close();
			g.fin();
			g.rgb(0,1,0);
			g.alpha(.25);
			g.fill1();
			g.rgb(0,1,1);
			g.alpha(.5);
			g.stroke();
			if(click)
			{
				if(field[ix+iy*8]==0)
				{
					field[ix+iy*8]=next;
					next=irand(seed)%5+1;
				}
			}
		}

		for(int i=0;i<8;i++)
		{
			for(int j=0;j<8;j++)
			{
				int obj=field[i+j*8];
				if(obj==1)
					DrawAnimated(hive, hiveAnim, i, j, t+j+i*.3);
				else if(obj==2)
					Draw(bush, i, j);
				else if(obj==3)
					Draw(grass, i, j);
				else if(obj==4)
					DrawAnimated(bee, beeAnim, i, j, t+j+i*.3);
				else if(obj==5)
					DrawAnimated(bear, bearAnim, i, j, t+j+i*.3);
			}
		}

		float s=(X(ox+.5,oy)-X(ox-.5,oy))*.05;
		SVGSetTransform(mx,my,s,0,0,s);
		if(hover)
		{
			if(canplace)
			{
				if(next==1)
					SVGDraw(hive);
				else if(next==2)
					SVGDraw(bush);
				else if(next==3)
					SVGDraw(grass);
				else if(next==4)
					SVGDraw(bee);
				else if(next==5)
					SVGDraw(bear);
			}
			else
			{
				SVGDraw(No);
			}
		}
		Present();
	}
	return 0;
}