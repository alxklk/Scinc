#include "graphics.h"

#define M_PI 3.141592654

Graph g;

#define NsP 3000

#include "font.h"
#include "letters.h"
#include "gtext.h"
#include "Bezier.h"

float* ppp;
int* npp;

void PP(float x, float y, float dx, float dy, int col)
{
	//printf(" add point %f %f\n", x, y);
	ppp[(*npp)*2]=x;
	ppp[(*npp)*2+1]=y;
	(*npp)++;
	//printf(" count=%i\n", (*npp));
}

#include "FontGlyphConvert.h"

void ConvertLine(CFont& font, char* s, float x, float y, float sx, float sy, float step)
{
	float xi=x;
	for(int i=0;i<1024;i++)
	{
		if(int(s[i])==0)
			break;
		ConvertGlyph(font,int(s[i]),xi,y,sx,sy,step);
		xi+=font.g[int(s[i])].w*sx;
	}
}

void gCircle(float x, float y, float r, float w, float a, int col)
{
	g.clear();
	g.M(x,y);
	g.width(a*2,w*1.5);
	g.fin();
	g.rgba32(col);
	g.stroke();	
}


int main()
{
	float p[NsP*2];
	ppp=&p[0];
	int np=0;
	npp=&np;

	CFont* pfont=(CFont*)malloc(sizeof(CFont));
	CFont& font=*pfont;
	CFontMaker fm;
	fm.Init(font,6);
	MakeLetters(fm);

	//g.t_0(50,100);
	//g.t_x(12,0);
	//g.t_y(0,-20);
	//g.clear();
	//gtext("Powered with SCINC");
	//g.t_0(100,200);
	//gtext("compiler and VM");
	//g.t_0(0,0);g.t_x(1,0);g.t_y(0,1);
	//g.M(30,30);
	//g.l(580,0);
	//g.l(0,200);
	//g.l(-580,0);
	//g.l(0,-200);
	//g.calclen();

	ConvertLine(font,"*-----*-*-*-*-*------*",60, 50,4,4,2.5);
	ConvertLine(font,"| Powered with SCINC |",60,100,4,4,2.5);
	ConvertLine(font,"*------*-*-*-*-------*",60,150,4,4,2.5);

	printf(" NP=%i\n", np);

	//for(int s=0;s<g.getnshapes();s++)
	//{
	//	int nn=g.getshapelen(s)/1.5;
	//	if(nn<5)nn=5;
	//	if(nn>800)nn=800;
	//	for(int i=0;i<nn;i++)
	//	{
	//		g.getshapepos(i/float(nn-1),s,p[np*2],p[np*2+1]);
	//		np++;
	//		if(np>=NsP-1)
	//			break;
	//	}
	//	if(np>=NsP-1)
	//	{
	//		printf("Dot count overflows %i\n",NsP);
	//		break;
	//	}
	//}
	int nl=800;

	while(true)
	{
		g.rgb(.4,.2,.25);
		g.FillRT();

		//g.clear();
		//g.t_0(50,100);
		//g.t_x(12,0);
		//g.t_y(0,-20);
		//gtext("Powered with SCINC");
		//g.rgb(.1,.1,.1);
		//g.fin();
		//g.calclen();
		//g.cut(0.5,1);
		//g.width(1,1);
		//g.stroke();
		//g.t_0(0,0);
		//g.t_x(1,0);
		//g.t_y(0,1);

		float t=Time();
		float l=(t*0.025-int(t*0.025))*(1+float(nl)/np);
		g.clear();
		int N=np*l;
		for(int i=0;i<N;i++)
		{
			if(i<np)
			{
				g.clear();
				float d=(N-i)/float(nl);
				if((d>0)&&(d<1.))
				{
					float v=(1.-(sin(i*131.71)+1)*d*.5);
					float x=p[i*2]+sin(i*131.155)*d*d*30+d*d*40;
					float y=p[i*2+1]+d*d*18+cos(i*131.731)*d*d*30;
					float a=(2+d*d*35)*.7;
					int cr=1.*v*255.;
					int cg=(1.-d)*v*255.;
					int cb=(1.-d*d)*v*255.;
					g.Circle(x,y,0,1,a,(cb<<16)|(cg<<8)|cr|((int((1.-d)*(1.-d)*v*255)<<24)));
				}
			}
		}

		if(N<np)
		{
			float px=p[N*2];
			float py=p[N*2+1];

			g.clear();
			g.M(px,py);
			g.l(10,25);
			g.fin();
			g.width(8,8);
			g.gray(.0);
			g.alpha(.2);
			g.stroke();

			g.clear();
			g.alpha(1);
			g.M(px,py);
			g.l(20,10);
			g.fin();
			g.width(8,8);
			g.gray(.4);
			g.stroke();
			g.gray(1);
			g.width(8,1);
			g.stroke();
		}
		Present();
	}
	return 0;
}