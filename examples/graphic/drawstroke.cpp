#include "graphics.h"

#define M_PI 3.141592654

Graph g;

#define NsP 3000

int main()
{
	float p[NsP*2];
	g.t_0(50,100);
	g.t_x(12,0);
	g.t_y(0,-20);
	g.clear();
	gtext("Powered with SCINC");
	g.t_0(100,200);
	gtext("compiler and VM");
	g.t_0(0,0);g.t_x(1,0);g.t_y(0,1);
	g.M(30,30);
	g.l(580,0);
	g.l(0,200);
	g.l(-580,0);
	g.l(0,-200);
	g.calclen();
	
	int np=0;
	for(int s=0;s<g.getnshapes();s++)
	{
		int nn=g.getshapelen(s)/1.5;
		if(nn<5)nn=5;
		if(nn>800)nn=800;
		for(int i=0;i<nn;i++)
		{
			g.getshapepos(i/float(nn-1),s,p[np*2],p[np*2+1]);
			np++;
			if(np>=NsP-1)
				break;
		}
		if(np>=NsP-1)
		{
			printf("Dot count overflows %i\n",NsP);
			break;
		}
	}
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
		//g.width(1,1);
		//g.stroke();
		//g.t_0(0,0);
		//g.t_x(1,0);
		//g.t_y(0,1);

		float t=Time()*.2;
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
					g.M(p[i*2]+sin(i*131.155)*d*d*30+d*d*40,p[i*2+1]+d*d*18+cos(i*131.731)*d*d*30);
					//g.M(p[i*2],p[i*2+1]);
					//g.l(0,0);
					g.fin();
					g.width(2+d*d*35,2-d);
					//g.width(3,1);
					g.rgb(1*v,(1-d)*v,(1-d*d)*v);
					g.alpha((1.-d)*(1.-d)*v);
					//g.alpha(1);
					//g.gray(1);
					g.stroke();
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
			g.M(px,py);
			g.l(20,10);
			g.fin();
			g.width(8,4);
			g.gray(.4);
			g.stroke();
			g.gray(1);
			g.width(6,2);
			g.stroke();
		}
		Present();
	}
	return 0;
}