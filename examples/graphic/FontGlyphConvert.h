#include "Bezier.h"
#include "font.h"

void ConvertGlyph(CFont& font, int sym, float x, float y, float sx, float sy, float step)
{
	float xi;
	float yi;

	int c=font.g[sym].c0;
	int d=font.g[sym].d0;
	float* fd=&font.data[0];

	for(int i=0;i<font.g[sym].n;i++)
	{
		int C=font.cmds[c+i];
		if(C=='M')
		{
			xi=x+fd[d]*sx;
			yi=y+fd[d+1]*sy;
			PP(xi,yi,0,0,0xffffffff);
			d+=2;
		}
		else if(C=='m')
		{
			xi=xi+fd[d]*sx;
			yi=yi+fd[d+1]*sy;
			PP(xi,yi,0,0,0xffffffff);
			d+=2;
		}
		else if(C=='L')
		{
			float xi1=x+fd[d]*sx;
			float yi1=y+fd[d+1]*sy;
			float dx=xi1-xi;
			float dy=yi1-yi;
			float l=sqrt(dx*dx+dy*dy);
			int count=l/step;
			if(count>150)count=150;
			for(int j=0;j<=count;j++)
			{
				PP(xi+dx/count*j,yi+dy/count*j,dx/l,dy/l,0xffffffff);
			}
			xi=xi1;
			yi=yi1;
			d+=2;
		}
		else if(C=='l')
		{
			float xi1=xi+fd[d]*sx;
			float yi1=yi+fd[d+1]*sy;
			float dx=xi1-xi;
			float dy=yi1-yi;
			float l=sqrt(dx*dx+dy*dy);
			int count=l/step;
			if(count>150)count=150;
			for(int j=0;j<=count;j++)
			{
				PP(xi+dx/count*j,yi+dy/count*j,dx/l,dy/l,0xffffffff);
			}
			xi=xi1;
			yi=yi1;
			d+=2;
		}
		else if(C=='c')
		{
			float xi1=xi+fd[d  ]*sx;
			float yi1=yi+fd[d+1]*sy;
			float xi2=xi+fd[d+2]*sx;
			float yi2=yi+fd[d+3]*sy;
			float xi3=xi+fd[d+4]*sx;
			float yi3=yi+fd[d+5]*sy;
			BezCalc3 bcx;bcx.Init(xi,xi1,xi2,xi3);
			BezCalc3 bcy;bcy.Init(yi,yi1,yi2,yi3);
			
			for(float t=0;t<=1.;)
			{
				float dx=bcx.Diff(t);
				float dy=bcy.Diff(t);
				float l=sqrt(dx*dx+dy*dy);
				if(l<0.01)l=0.01;
				PP(bcx.Calc(t),bcy.Calc(t),dx/l, dy/l,0xffffffff);
				t+=step/l;
			}
			xi=xi3;
			yi=yi3;
			d+=6;
		}
		else if(C=='z')
		{
			//g.close();
		}
	}
}
