// Adopted example from Rosetta Code
// Redirect program output to file with pnm extension to see the picture of Dragon curve.
// Scinc dragon.cpp > dragon.pnm

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define W 768
#define H 512

int x;
int y;
int dx;
int dy;
int scale;
int clen;

class rgb
{
public:
	float r;
	float g;
	float b;
};

// Scinc uses stack for global vars
// It is possible to increase stack size with pragma directive:
// #pragma STACK_SIZE 10000000 // enough for 768*512*sizeof(rgb)
// and make buf array instead of pointer: rgb buf[W*H];
// Note: pragma does not calculate expressions, so,
// STACK_SIZE must be followed by int
// But it is much better to store large variables in dynamic memory,
// using malloc and free

rgb* buf;

void sc_up()
{
	int tmp=dx-dy;
	dy=dx+dy;
	dx=tmp;
	scale*=2;
	x*=2;
	y*=2;
}

float Fabs(float x)
{
	return x<0?-x:x;
}

float Fmod(float x, float y)
{
	return x-y*int(x/y);
}

void h_rgb(int x, int y)
{
	if(y>=H)
		return;
	if(x>=W)
		return;
	rgb&p=buf[y*W+x];

	float h=6.0*clen/scale;
	float VAL=1-(cos(3.141592653579*64*clen/scale)-1)/4;
	float c=VAL;
	float X=c*(1-Fabs(Fmod(h, 2)-1));

	int ih=h;
	if(ih==0)
	{
		p.r+=c;
		p.g+=X;
		return;
	}
	else if(ih==1)
	{
		p.r+=X;
		p.g+=c;
		return;
	}
	else if(ih==2)
	{
		p.g+=c;
		p.b+=X;
		return;
	}
	else if(ih==3)
	{
		p.g+=X;
		p.b+=c;
		return;
	}
	else if(ih==4)
	{
		p.r+=X;
		p.b+=c;
		return;
	}
	else
	{
		p.r+=c;
		p.b+=X;
	}
}

void iter_string(char*str, int d)
{
	int tmp;
#define LEFT  tmp = -dy; dy = dx; dx = tmp
#define RIGHT tmp = dy; dy = -dx; dx = tmp
	int i=0;
	while(str[i]!=0)
	{
		char c=str[i++];
		if(c=='X')
		{
			if(d)
				iter_string("X+YF+", d-1);
			continue;
		}
		else if(c=='Y')
		{
			if(d)
				iter_string("-FX-Y", d-1);
			continue;
		}
		else if(c=='+')
		{
			RIGHT;
			continue;
		}
		else if(c=='-')
		{
			LEFT;
			continue;
		}
		else if(c=='F')
		{
			clen++;
			h_rgb(x/scale, y/scale);
			x+=dx;
			y+=dy;
			continue;
		}
	}
}

float Max(float x, float y)
{
	return x>y?x:y;
}

void dragon(int leng, int depth)
{
	int d=leng/3+1;
	for(int i=0;i<W*H;i++)
	{
		buf[i].r=0;
		buf[i].g=0;
		buf[i].b=0;
	}

	x=y=d;
	dx=leng;
	dy=0;
	scale=1;
	clen=0;
	for(int i=0;i<depth;i++)
		sc_up();
	iter_string("FX", depth);

	float maxv=0;
	for(int i=0;i<W*H;i++)
	{
		maxv=Max(buf[i].r, maxv);
		maxv=Max(buf[i].g, maxv);
		maxv=Max(buf[i].b, maxv);
	}

	printf("P3\n%ld %ld\n255\n", W, H);
	for(int i=0;i<H;i++)
	{
		for(int j=0;j<W;j++)
		{
			int r=buf[j+i*W].r/maxv*255;
			int g=buf[j+i*W].g/maxv*255;
			int b=buf[j+i*W].b/maxv*255;
			printf("%i %i %i ", r, g, b);
		}
		printf("\n");
	}
}

typedef rgb* prgb;

int main()
{
	buf=prgb(malloc(sizeof(rgb)*W*H));
	dragon(512, 9*2);
	free(buf);
	return 0;
}