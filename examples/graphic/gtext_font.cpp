#ifdef __JS__
	#define G_SCREEN_SCALE 2
#else
	//#define G_SCREEN_WIDTH 1280
	//#define G_SCREEN_HEIGHT 960
	#define G_SCREEN_SCALE 4
#endif

#include "graphics.h"

Graph g;

#include "font.h"
#include "letters.h"
#include "gtext.h"


int main()
{
	CFont* pfont=(CFont*)malloc(sizeof(CFont));
	CFont& font=*pfont;
	CFontMaker fm;
	fm.Init(font,6);
	MakeLetters(fm);
	fm.StartGlyph(128);
	fm.Width(10);
	fm.clear();
	fm.M(4,5);
	fm.l(0,2);
	fm.stroke(.4,0xff000000);
	fm.clear();
	fm.M(4,0);
	fm.l(0,6);
	fm.c(5,4,5,-4,0,-5);
	fm.z();
	fm.fill(0xffff0000);
	fm.clear();
	fm.M(4,0);
	fm.l(0,6);
	fm.c(-5,4,-5,-4,0,-5);
	fm.z();
	fm.fill(0xffff8000);
	fm.clear();
	fm.M(4,7);
	fm.c(-1,2,-2,0,4,2);
	fm.z();
	fm.fill(0xff00c080);
	fm.clear();
	fm.M(8,9);
	fm.c(-1,-2,2,-2,-4,-2);
	fm.z();
	fm.fill(0xff80f000);
	fm.EndGlyph();

	fm.StartGlyph(129);
	fm.Width(10);
	fm.clear();
	fm.M(4,.5);
	fm.c(5,0,5,0,0,7.5);
	fm.c(-5,0,-5,0,0,-7.5);
	fm.z();
	fm.stroke(.75,0xffff8000);
	fm.fill(0xffffc000);
	fm.clear();
	fm.M(3,4.5);
	fm.c( .7,0, .7,0,0, 1.75);
	fm.c(-.7,0,-.7,0,0,-1.75);
	fm.z();
	fm.M(5,4.5);
	fm.c( .7,0, .7,0,0, 1.75);
	fm.c(-.7,0,-.7,0,0,-1.75);
	fm.z();
	fm.fill(0xff000000);
	fm.clear();
	fm.M(2,3.5);
	fm.c( 1.,-1.2,-1.,-1.2, 4,0);
	fm.c(-.2,-2.5, .2,-2.5,-4,0);
	fm.fill(0xff800000);
	fm.EndGlyph();

	fm.StartGlyph(130);
	fm.Width(8);
	fm.clear();
	fm.M(3,.5);
	fm.c( 1,2, 0,-2, 3.25, 5.);
	fm.c( 0,1.5, 1.5,2,-3.25, 1);
	fm.c( -1.5,2,0,1.5, -3.25, -1);
	fm.c( 0,-2, -1,2, 3.25, -5.);
	fm.z();
	fm.fill(0xffe80020);
	fm.EndGlyph();

	fm.StartGlyph(131);
	fm.Width(8);
	fm.clear();
	fm.M(0,4);
	fm.c(  1.5, 0.5,-0.5,-2.5, 3, 4.5);
	fm.c(  0.5,-2.5,-1.5, 0.5, 3,-4.5);
	fm.c( -1.5,-0.5, 0.5, 2.5,-3,-4.5);
	fm.c( -0.5, 2.5, 1.5,-0.5,-3, 4.5);
	fm.z();
	fm.fill(0xffe80020);
	fm.EndGlyph();

	fm.StartGlyph(132);
	fm.Width(8);
	fm.clear();
	fm.M(1,0);
	fm.c( 1.0, 0.5,-1.0, 0.5, 4.0, 0.0);
	fm.l( 0.5, 0.25);
	fm.c(-1.5, 0.5,-0.5,-0.5,-2.0, 3.5);
	fm.c( 0.5, 0.5,-1.0, 0.0, 1.5,-1.0);
	fm.c( 1.5, 0.0, 1.5, 0.0, 0.0, 3.0);
	fm.c(-1.0, 0.0, 0.5,-0.5,-1.5,-1.0);
	fm.c(-0.7, 0.7, 0.0,-1.0, 0.8, 2.5);
	fm.c( 0.0, 2.0, 0.0, 2.0,-2.6, 0.0);
	fm.c( 0.0,-1.0, 0.7, 0.7, 0.8,-2.5);
	fm.c(-0.5,-0.5, 1.0, 0.0,-1.5, 1.0);
	fm.c(-1.5, 0.0,-1.5, 0.0, 0.0,-3.0);
	fm.c( 1.0, 0.0,-0.5, 0.5, 1.5, 1.0);
	fm.c( 0.5,-0.5, 1.5, 0.5,-2.0,-3.5);
	fm.l( 0.5,-0.25);
	fm.z();
	fm.fill(0xff000000);
	fm.EndGlyph();

	fm.StartGlyph(133);
	fm.Width(8);
	fm.clear();
	fm.M(3,8.75);
	fm.c( .5,-2, 0,2, 3, -4.5);
	fm.c( 0,-1.5, 1.,-1.5,-2.5, -1);
	fm.c( -.5,-1.5, -1, .5, 2.,  -3);
	fm.l( -0.5,-0.25);
	fm.c( -1,.5, 1, .5, -4,  0);
	fm.l( -0.5,0.25);
	fm.c( 1,.5, .5, -1.5, 2.,  3);
	fm.c( -1.,-1.5,0,-1.5, -2.5, 1);
	fm.c( 0,2, -.5,-2, 3, 4.5);
	fm.z();
	fm.fill(0xff000000);
	fm.EndGlyph();

	fm.StartGlyph(134);
	fm.Width(6);
	fm.clear();
	fm.M(-0.5,-0.5);
	fm.l(0,10);
	fm.l(2,0);
	fm.c(4,0,4,0,0,-6);
	fm.l(-1,0);
	fm.l(0,-4);
	fm.l(-1,0);
	fm.z();
	fm.M(.5,8.5);
	fm.l(0,-4);
	fm.l(1.5,0);
	fm.c(2.,0,2.,0,0,4);
	fm.l(-1.5,0);
	fm.z();
	fm.fill(0xff000000);
	fm.EndGlyph();

	fm.StartGlyph(135);
	fm.Width(6);
	fm.clear();
	fm.M(-0.5,-0.5);
	fm.l(0,10);
	fm.l(2,0);
	fm.c(4,0,2.7,.5,1,-6);
	fm.c(1,0,-1.5,0,2.5,-3.5);
	fm.l(-1,-.5);
	fm.c(-1.5,0,1.7,0,-3,4);
	fm.l(-.5,0);
	fm.l(0,-4);
	fm.l(-1,0);
	fm.z();
	fm.M(.5,8.5);
	fm.l(0,-4);
	fm.l(1.5,0);
	fm.c(2.,0,2.,0,0,4);
	fm.l(-1.5,0);
	fm.z();
	fm.fill(0xff000000);
	fm.EndGlyph();


	char st[32];
	//MLC_Typer t;
	//t.Init();
	int cx=-1;
	int cy=-1;
	//SetPresentWait(1);
	cx=GetPersistentInt("cx",cx);
	cy=GetPersistentInt("cy",cy);
	int code=cx+cy*16;
	char teststr[33]={};
	int testcur=0;
	for(int i=0;i<31;i++){teststr[i]='A'+i;}

	int mx=0;
	int my=0;
	bool redraw=true;
	while(true)
	{
		SScincEvent ev;
		WaitForScincEvent();
		while(redraw||GetScincEvent(ev))
		{
			redraw=false;

			bool clicked=false;

			if(ev.type=='MLDN')
			{
				mx=ev._1;
				my=ev._2;
				printf("mx=%i, my=%i\n", mx, my);
				clicked=true;
			}


			g.t_0(0,0);g.t_x(1,0);g.t_y(0,1);
			g.alpha(1);
			g.rgb(.7,.75,.9);
			g.clear();
			g.M(0,0);g.l(640,0);g.l(0,480);g.l(-640,0);g.close();g.fin();
			g.fill1();
			g.clear();

			g.clear();
			for(int i=0;i<=16;i++)
			{
				g.M(i*20+10.5,130.5);
				g.l(0,320);
				g.M(10.5,130.5+i*20);
				g.l(320,0);
			}
			for(int i=0;i<=32;i++)
			{
				g.M(i*7+400.5-42,400.5+42);
				g.l(0,-224);
				g.M(400.5-42,400.5+42-i*7);
				g.l(224,0);
			}
			g.fin();
			g.rgb(.6,.7,.8);
			g.width(1,1);
			g.stroke();

			g.clear();
			for(int i=0;i<=32;i+=2)
			{
				g.M(i*7+400.5-42,400.5+42);
				g.l(0,-224);
				g.M(400.5-42,400.5+42-i*7);
				g.l(224,0);
			}
			g.fin();
			g.rgb(.8,.85,.9);
			g.width(1,1);
			g.stroke();
			g.clear();

			g.clear();
			g.M(400.5,400.5+42);
			g.l(0,-224);
			g.M(400.5+7*10,400.5+42);
			g.l(0,-224);
			g.M(400.5-42,400.5);
			g.l(224,0);
			g.M(400.5-42,400.5-7*18);
			g.l(224,0);
			g.M(400.5-42,400.5-7*12);
			g.l(224,0);
			g.fin();
			g.rgba(0,0,0,0.2);
			g.width(2,2);
			g.stroke();
			g.clear();
			g.alpha(1);



			snprintf(st,32,"Time %f", Time());
			DrawText(g,font,st,0,10.5,30.5,1.5,1);

			DrawText(g,font,"!\"#$%&\'()*+,-./:;<=>?@[\\]^_{|}~0123456789",0,10.5, 80.5,1,1);
			DrawText(g,font,"the quick brown fox jumps over the lazy dog" ,0,10.5,100.5,1,1);
			DrawText(g,font,"THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG" ,0,10.5,120.5,1,1);

			g.fin();
			g.rgb(.3,.1,0);
			g.width(1,1);
			g.stroke();

			if(clicked)
			{
				cx=(mx-10)/20;
				cy=(my-130)/20;
				SetPersistentInt("cx",cx);
				SetPersistentInt("cy",cy);
				code=cx+cy*16;
				teststr[testcur]=code;
				testcur=(testcur+1)%32;
			}
			if((cx>=0)&&(cx<16)&&(cy>=0)&&(cy<16))
			{
				g.clear();
				char s0[32];
				snprintf(s0,32,"%i %i code %i",cx,cy,code);
				DrawText(g,font,s0,0,400.,160.,2,2);
				g.fin();
				g.rgb(.3,.1,0);
				g.width(1.5,1.5);
				g.stroke();

				g.clear();
				g.t_t(0,0,1,0,0,1);
				//t.b0=Flt2(400,400);
				//t.x=Flt2(14,0);
				//t.y=Flt2(0,-14);
				//t.Glyph(code);
				DrawGlyph(g,font,code,400,400,14,14);
				if(!font.g[code].col)
				{
					g.fin();
					g.rgba(.3,.1,0,.5);
					g.width(7.,7.);
					g.stroke();
				}

				g.clear();
				DrawText(g,font,teststr,0,220,30,2,2);
				g.fin();
				g.rgba(0,0,0,1);
				g.width(3.5,1.);
				g.stroke();
				g.rgb(.8,.9,1.);
				g.width(1.5,1.5);
				g.stroke();
				DrawText(g,font,teststr,1,220,30,2,2);

				g.clear();
				DrawText(g,font,teststr,0,220.5,50.5,2,1);
				g.fin();
				g.rgba(.3,.1,0,1);
				g.width(1,1);
				g.stroke();
				DrawText(g,font,teststr,1,220.5,50.5,2,1);

				g.clear();
				DrawText(g,font,teststr,0,320.5,70.5,1,1);
				g.fin();
				g.rgba(.3,.1,0,1);
				g.width(1,1);
				g.stroke();
				DrawText(g,font,teststr,1,320.5,70.5,1,1);

				float testdx=0;
				float testw=0;
				for(int i=0;i<testcur;i++)
				{
					int s=teststr[i];
					testw=font.g[s].w;
					testdx+=testw;
				}
				g.clear();
				g.M(320.5+testdx,73.5);
				g.l(5,0);
				g.fin();
				g.width(2,10);
				g.rgba32(0xffff8000);
				g.stroke();

				{
					float x=400;
					float y=400;
					float sx=14;
					float sy=14;
					CGlyph& gl=font.g[code];
					float cx=x;
					float cy=y;
					int* pc=&font.cmds[0];
					float* pd=&font.data[0];
					int c=gl.c0;
					int d=gl.d0;
					int nl=0;
					int dbgP=0x00000000;
					for(int i=0;i<gl.n;i++)
					{
						int C=pc[c];
						if((C=='M')||(C=='L'))
						{
							float D0=pd[d];
							float D1=pd[d+1];
							if(C=='L')
							{
								g.clear();
								g.M(cx,cy);
								g.L(x+D0*sx,y+D1*sy);
								g.fin();
								g.rgba(0,1,0,.5);
								g.width(1.,1.);
								g.stroke();
							}
							cx=x+D0*sx;
							cy=y+D1*sy;
							if(dbgP){char s[128];snprintf(s,128,"'%c' %f %f", C,D0,D1);stext(s,20,30+nl*10,dbgP);nl++;}
							c+=1;d+=2;
						}
						else if((C=='m')||(C=='l'))
						{
							float D0=pd[d];
							float D1=pd[d+1];
							if(C=='l')
							{
								g.clear();
								g.M(.5+cx,.5+cy);
								g.l(D0*sx,D1*sy);
								g.fin();
								g.rgba(1,0,1,1);
								g.width(1.,1.);
								g.stroke();
							}
							cx+=D0*sx;
							cy+=D1*sy;
							if(dbgP){char s[128];snprintf(s,128,"'%c' %f %f", C,D0,D1);stext(s,20,30+nl*10,dbgP);nl++;}
							c+=1;d+=2;
						}
						else if(C=='c')
						{
							float D0=pd[d];
							float D1=pd[d+1];
							float D2=pd[d+2];
							float D3=pd[d+3];
							float D4=pd[d+4];
							float D5=pd[d+5];
							g.clear();
							g.M(.5+cx,.5+cy);
							g.l(D0*sx,D1*sy);
							g.fin();
							g.rgba(1,0,0,1);
							g.width(1.,1.);
							g.stroke();
							g.clear();
							g.M(.5+cx+D4*sx,.5+cy+D5*sy);
							g.L(.5+cx+D2*sx,.5+cy+D3*sy);
							g.fin();
							g.rgba(0,0,1,1);
							g.width(1.,1.);
							g.stroke();
							cx+=D4*sx;
							cy+=D5*sy;
							if(dbgP){char s[128];snprintf(s,128,"'%c' %f %f %f %f %f %f", C,D0,D1,D2,D3,D4,D5);stext(s,20,30+nl*10,dbgP);nl++;}
							c+=1;d+=6;
						}
						else if(C=='z')
						{
							if(dbgP){stext("'Z'",20,30+nl*10,dbgP);nl++;}
							c+=1;
						}
						else if(C=='0')
						{
							if(dbgP){stext("'0'",20,30+nl*10,dbgP);nl++;}
							c+=1;
						}
						else if(C=='/')
						{
							float D0=pd[d];
							float D1=pd[d+1];
							if(dbgP){char s[128];snprintf(s,128,"'%c' %f 0x%x", C,D0,int(D1));stext(s,20,30+nl*10,dbgP);nl++;}
							c+=1;d+=2;
						}
						else if(C=='f')
						{
							float D0=pd[d];
							if(dbgP){char s[128];snprintf(s,128,"'%c' 0x%x", C,int(D0));stext(s,20,30+nl*10,dbgP);nl++;}
							c+=1;d+=1;
						}
						else
						{
							printf("Unknown command %c\n",C);
							c++;
						}
					}
				}
			}

			g.alpha(1);


			for(int i=0;i<16;i++)
			{
				for(int j=0;j<16;j++)
				{
					char s[2];
					s[1]=0;
					s[0]=i+j*16;
					if((s[0]<127)&&(s[0]>32))
						stext(s,20+i*20,141+j*20,0xffffffff);
					//stext(s,11+i*20,142+j*20,0x40ffffff);
				}
			}

			g.clear();
			g.t_t(0,0,1,0,0,1);
			for(int i=0;i<16;i++)
			{
				for(int j=0;j<16;j++)
				{
					int c=i+j*16;
					if(font.g[c].col==0)
						DrawGlyph(g,font,c,10.5+i*20,150.5+j*20,1,1);
				}
			}
			g.fin();
			g.rgb(.3,.1,0);
			g.width(1,1);
			g.stroke();

			for(int i=0;i<16;i++)
			{
				for(int j=0;j<16;j++)
				{
					int c=i+j*16;
					if(font.g[c].col!=0)
						DrawGlyph(g,font,c,10.5+i*20,150.5+j*20,1,1);
				}
			}

		}
		Present();
	}
	return 0;
}