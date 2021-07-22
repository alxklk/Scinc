#define G_SCREEN_WIDTH 320
#define G_SCREEN_HEIGHT 240
#define G_SCREEN_SCALE 4

#include "graphics.h"

int gseed=4734234;

int irand(int& seed)
{
	seed=(seed*1103515245+12345)&0x7ffffff;
	return seed;
}

float frand(int& seed)
{
	return irand(seed)/float(0x7ffffff);
}

float Fmod(float x,float y)
{
	return x-int(x/y)*y;
}

float Fract(float x)
{
	return x-int(x);
}

float Fabs(float x)
{
	return x<0?-x:x;
}

float S_Curve(float x)
{
	return x*x*(-2.0f*x+3.0f);
}


float Min(float x,float y){return x<y?x:y;}

float bell_curve3(float x)
{
	float x2=x*x;
	return x2*16.*(x2-2.*x+1.);
}

#define M_PI 3.141592654

int MakeShootSound(int len)
{
	int res=snd_add(len);
	float dlen=1.0/len;
	{
		float t=0.;
		float f=0.;
		for(int i=0;i<len;i++)
		{
			float v=bell_curve3(f)*.05;
			float x=t*440.*12.*M_PI;
			float l=sin(x)*v*sin(f*M_PI*14);
			if(f<0.25)l=l+sin(x*8.)*bell_curve3(f/0.25)*0.25*cos(f*M_PI*14.);
			snd_data(res,i,l,l);
			t+=1./44100.*(f*f)*2.2;
			f+=dlen;
		}
	}
	return res;
}

int MakeExplodeSound(int len, int seed)
{
	int res=snd_add(len);
	{
		float t=0.;
		float f=0.;
		float dlen=1./len;
		float fr0=frand(seed);
		float fr1=frand(seed);
		for(int i=0;i<len;i++)
		{
			float v=(1.-f)*(1-f)*bell_curve3(f);
			float l=0;
			if(!(i&16))fr0=frand(seed);
			if(!(i&256))fr1=frand(seed);
			if(f<0.1)l=l+fr0*(1.-f/0.1)*.5;
			if(f<0.5)l=l+(1.-f/0.5)*0.75*sin(f*(fr1+1)*4200);
			snd_data(res,i,l,l);
			f+=dlen;
		}
	}
	return res;
}

int MakeEngineSound(int len, int seed)
{
	int res=snd_add(len);
	{
		float t=0.;
		float fr0=frand(seed);
		float f=0.;
		float dlen=1./len;
		for(int i=0;i<len;i++)
		{
			float f=i/12000.0;
			float v=bell_curve3(f);
			float l=fr0*v*.125+frand(seed)*v*0.125;
			if(!(i&64))
				fr0=frand(seed);
			snd_data(res,i,l,l);
			f+=dlen;
		}
	}
	return res;
}


#define ROT_SPEED 8
#define MOV_SPEED 8

#define HIT_LEFT 2
#define HIT_RIGHT 4
#define HIT_SIDES (HIT_LEFT|HIT_RIGHT)
#define HIT_BOTTOM 8
#define HIT_CELL 1

#define M_PI 3.141592654

float rotCenter[14]=
{
	0.5,2.0,//I
	1.5,1.5,//J
	0.5,1.5,//L
	1.0,1.0,//O
	1.5,1.0,//S
	1.5,0.5,//T
	1.5,1.0 //Z
};

int deltasNew[7*4]=
{
	0, 0, 0, 0, //I
	0, 1, 0, 0, //J
	0, 1, 0, 0, //L
	0, 0, 0, 0, //O
	0, 1, 0, 1, //S
	0, 1, 0, 0, //T
	0, 1, 0, 1  //Z
};

int offsets[7*8]=
{
	 0,  1, -2, -1, 0, -2, 1, -1,  //I
	-1,  0, -1, -2, 1, -2, 1,  0,  //J
	 0,  0, -1, -1, 0, -2, 1, -1,  //L
	-1,  0, -1, -1, 0, -1, 0,  0,  //O
	-1,  0, -1, -2, 1, -1, 0,  0,  //S
	-1, -1,  0, -2, 1, -1, 0,  0,  //T
	-1,  0, -1, -2, 1, -1, 0,  0   //Z
};

int boxes[7*2]=
{
	1, 4,  //I
	2, 3,  //J
	2, 3,  //L
	2, 2,  //O
	3, 2,  //S
	3, 2,  //T
	3, 2   //Z
};

char* figtpl=//IJLOSTZ
	"1   "
	"1   "
	"1   "
	"1   "

	" 1  "
	" 1  "
	"11  "
	"    "

	"1   "
	"1   "
	"11  "
	"    "

	"11  "
	"11  "
	"    "
	"    "

	" 11 "
	"11  "
	"    "
	"    "

	"111 "
	" 1  "
	"    "
	"    "

	"11  "
	" 11 "
	"    "
	"    "
;

int stdcols[12]=
{
	0xffe010a0,
	0xff8010f0,
	0xff0735ff,
	0xff0090ff,// cyan
	0xff00d070,// aquamarine
	0xff60c000,// lime
	0xffe8d000,// yellow
	0xffff8000,// orange
	0xffff0020,// red
	0xffffffff,
	0xff000000, 
	0xff808080 
};

Graph g;


void Background()
{
	g.clear();
	g.rgba(.3,.2,.1,1);
	//g.FillRT();
	//return;
	float t=Time();
	g.graddef(0);
	g.gradstop(0.0,.9,.5,.0,1);
	g.gradstop( .4,.2,.1,.1,1);
	g.gradstop( .6,.3,.0,.2,1);
	g.gradstop(1.0,.4,.2,.3,1);
	g.alpha(1);
	g.gradtype(1);
	g.gradmethod(2);
	g.gradend();
	g.graduse(0);
	g.clear();
	g.M(0,0);
	g.l(640,0);
	g.l(0,480);
	g.l(-640,0);
	g.close();
	g.fin();
	g.g_0(0,0);
	g.g_y(320,0);
	g.g_x(0,-240);
	g.fill1();
	g.graduse(-1);
	int seed=23452345;
	int seed1=76853345;
	g.clear();
	g.gray(1);
	g.alpha(1);
	g.clear();
	float T=t*.5;
	//#define N 2000
	//for(int i=0;i<N;i++)
	//{
	//	float x=(cos(i*M_PI/N*2.*5+T*1.1)*.8+sin(i*M_PI/N*2.*29-T*0.12)*.2)*150+160;
	//	float y=(sin(i*M_PI/N*2.*4+T*2.93)*.8+cos(i*M_PI/N*2.*27-T*0.37)*.2)*110+120;
	//	g.L(x,y);
	//}
	//g.close();
	//g.fin();
	//g.gray(1);
	//g.alpha(.2);
	//g.fill2();

	g.clear();
	for(int i=0;i<30;i++)
	{
		float x=(cos(i*M_PI/100*2.*5+T*1.1 )*.8+sin(i*M_PI/100*2.*29-T*0.12)*.2)*150+160;
		float y=(sin(i*M_PI/100*2.*4+T*2.93)*.8+cos(i*M_PI/100*2.*27-T*0.37)*.2)*110+120;
		float w=(1.+frand(seed1))*.25;
		g.M(x,y);
	}
	g.fin();
	g.width(50,.2);
	g.stroke();
	g.width(2,1);
	g.stroke();


	for(int i=0;i<60;i++)
	{
		float x=(1.+frand(seed));
		float y=int(220*(frand(seed))+10)+.5;
		float w=(1.+frand(seed1))*.25;
		//w*=w;
		x=Fmod(x+t*w*.5+27.72,1.);
		g.M(x*380-20,y);
		g.l(30*w,w*.5);
		g.l(0,-w);
		g.close();
	}
	g.fin();
	g.fill1();
}

struct sRemLine
{
	int y;
	double time;
	float rnd[32];
	int line[32];
};

struct IPOS
{
	int x;
	int y;
	float dy;
	int c;
};

class CBrickSet
{
public:
	IPOS p[32];
	int n;
	void Add(int x, int y, float dy, int c)
	{
		p[n].x=x;
		p[n].y=y;
		p[n].dy=dy;
		p[n].c=c;
		n++;
	}
};

struct SFloodFiller{
	int W;
	int H;
	int *field;
	float* fieldD;
	int *mask;
	int fill(int x, int y, CBrickSet& bs, int c)
	{
		if(x<0)
			return 0;
		if(y<0)
			return 0;
		if(y>=H)
			return 0;
		if(x>=W)
			return 0;
		if(c!=field[W*y+x])
			return 0;
		if(mask[W*y+x])
			return 0;
		if(bs.n>31)
			return 0;
		bs.Add(x,y,fieldD[W*y+x],c);
		mask[W*y+x]=1;
		return 1+
		fill(x+1, y  , bs, c)+
		fill(x-1, y  , bs, c)+
		fill(x  , y+1, bs, c)+
		fill(x  , y-1, bs, c);
	}
};

int FindPiece(int *field, float* fieldD, int* imask, int W, int H, int x, int y, CBrickSet& bs)
{
	if(!field[W*y+x])
	{
		return 0;
	}
	SFloodFiller ff;
	ff.mask=imask;
	ff.field=field;
	ff.fieldD=fieldD;
	ff.W=W;
	ff.H=H;
	return ff.fill(x,y,bs,field[W*y+x]);
};



#define NREMLINES 6
#define W 10
#define H 20
class Game
{
public:
	float cellSize;
	int field[W*H];
	int mask[W*H];
	float fieldD[W*H];
	float x;
	float y;
	int fig4[4*4*7];
	sRemLine remLines[NREMLINES];
	int lastRemLine;
	int col;
	int nextcol;
	int piece;
	int nextPiece;
	int order;

	int sndExplode;
	int sndRocket;
	int sndShoot;

	void InitFig()
	{
		sndExplode=MakeExplodeSound(20000,gseed);
		sndShoot=MakeShootSound(12000);
		sndRocket=MakeEngineSound(12000,gseed);

		for(int i=0;i<4*4*7;i++)
		{
			if(figtpl[i]==' ')
				fig4[i]=0x00;
			else
				fig4[i]=0x01;
		}
	}
	void NewPiece()
	{
		falling=false;
		px=W/2;
		py=H;
		pa=0;
		newpx=px;
		newpy=py;
		newpa=pa;
		col=nextcol;
		nextcol=irand(gseed)%7+1;
		piece=nextPiece;
		nextPiece=irand(gseed)%7;
		order++;

	}
	void ClearField()
	{
		for(int i=0;i<W*H;i++)field[i]=0;
		for(int i=0;i<W*H;i++)fieldD[i]=0;
		for(int i=0;i<NREMLINES;i++)
			remLines[i].time=-10;
	}
	void Init()
	{
		nextPiece=irand(gseed)%7;
		nextcol=irand(gseed)%7;
		NewPiece();
		t=0;
		scores=0;
		doUpdate=true;
		cellSize=10;
		x=G_SCREEN_WIDTH/2 -W*cellSize/2+.5;
		y=G_SCREEN_HEIGHT/2-H*cellSize/2+.5;
		//field=(int*)malloc(sizeof(int)*W*H);
		//fieldD=(float*)malloc(sizeof(float)*W*H);
		ClearField();
		InitFig();
		lastRemLine=0;
	}

	void DrawChip(int j, int i, float d, int col)
	{
//		printf("%f\n",d);
		g.clear();
		g.M(x+j*cellSize+.5,y+i*cellSize+d*cellSize+.5);
		g.l(cellSize-.25,0);
		g.l(0,cellSize-.25);
		g.l(-cellSize+.25,0);
		g.close();
		g.fin();
		g.rgba32(stdcols[col]);
		g.fill1();
		//g.alpha(1);
		//g.rgba(cols[col*3]*.5,cols[col*3+1]*.5,cols[col*3+2]*.5,1);
		//g.width(1.,1.);
		//g.stroke();
	}

	void DrawAChip(float x, float y, float angle, float size, int col)
	{
		float dx=size*cos(angle);
		float dy=size*sin(angle);
		g.clear();
		g.M(x+dx*2,y-dy*2);
		g.l(dy*2,dx*2);
		g.l(-dx*2,dy*2);
		g.l(-dy*2,-dx*2);
		g.close();
		g.fin();
		g.rgba32(stdcols[col]);
		g.fill1();
	}

	void CalcPieceDelta(int piece, int a, float& dx, float& dy)
	{
		if(a==0){dx=Fract(rotCenter[piece*2  ]);dy=Fract(rotCenter[piece*2+1]);}
		if(a==1){dx=Fract(rotCenter[piece*2+1]);dy=Fract(rotCenter[piece*2  ]);}
		if(a==2){dx=Fract(rotCenter[piece*2  ]);dy=Fract(rotCenter[piece*2+1]);}
		if(a==3){dx=Fract(rotCenter[piece*2+1]);dy=Fract(rotCenter[piece*2  ]);}
	}

	void DrawPiece(float x, float y, float pa, int piece, int col)
	{
		int ia=pa;
		float da=Fract(pa);
		// char s[64];
		// snprintf(s,64,"%f",da);
		// stext(s,x+100,y,0xff00ff00);
		float dx0;
		float dx1;
		float dy0;
		float dy1;
		CalcPieceDelta(piece, ia%4   ,dx0,dy0);
		CalcPieceDelta(piece,(ia+1)%4,dx1,dy1);
		float cs=cellSize;
		//da=da*da;
		pa=ia+da;
		x+=(dx0*(1.-da)+dx1*da)*cs+.5;
		y+=(dy0*(1.-da)+dy1*da)*cs+.5;

		float angle=pa*M_PI/2.;
		float dx=cellSize*cos(angle);
		float dy=cellSize*sin(angle);

		for(int i=0;i<4;i++)
		{
			for(int j=0;j<4;j++)
			{
				float cj=j-rotCenter[piece*2  ];
				float ci=i-rotCenter[piece*2+1];
				if(fig4[i*4+j+piece*16])
				{
					DrawAChip(x+cj*dx+ci*dy,y-cj*dy+ci*dx,angle,cellSize/2,col);
				}
			}
		}
		for(int i=0;i<4;i++)
		{
			for(int j=0;j<4;j++)
			{
				float cj=j-rotCenter[piece*2  ];
				float ci=i-rotCenter[piece*2+1];
				if(fig4[i*4+j+piece*16])
				{
						if((i==0)||(!fig4[(i-1)*4+j  +piece*16]))
						{g.gray(.5+sin(angle+  M_PI/4)*.5);g.hairline(x+cj*dx+ci*dy,y-cj*dy+ci*dx,x+cj*dx+ci*dy+dx,y-cj*dy+ci*dx-dy);}
						if((j==0)||(!fig4[ i   *4+j-1+piece*16]))
						{g.gray(.5+sin(angle+3*M_PI/4)*.5);g.hairline(x+cj*dx+ci*dy,y-cj*dy+ci*dx,x+cj*dx+ci*dy+dy,y-cj*dy+ci*dx+dx);}
						if((i==3)||(!fig4[(i+1)*4+j  +piece*16]))
						{g.gray(.5+sin(angle-3*M_PI/4)*.5);g.hairline(x+cj*dx+ci*dy+dx+dy,y-cj*dy+ci*dx+dx-dy,x+cj*dx+ci*dy+dy,y-cj*dy+ci*dx+dx);}
						if((j==3)||(!fig4[ i   *4+j+1+piece*16]))
						{g.gray(.5+sin(angle-  M_PI/4)*.5);g.hairline(x+cj*dx+ci*dy+dx+dy,y-cj*dy+ci*dx+dx-dy,x+cj*dx+ci*dy+dx,y-cj*dy+ci*dx-dy);}
				}
			}
		}
	}

	bool IsLineFull(int y)
	{
		for(int j=0;j<W;j++)
			if((field[y*W+j]==0)||(fieldD[y*W+j]>0))
				return false;
		return true;
	}

	bool foolLines[H];
	int lines;
	int scores;
	int level;

	void RemoveLines()
	{
		for(int l=0;l<H;l++)
		{
			foolLines[l]=IsLineFull(l);
		}
		int i=0;
		int moreLines=0;
		while(i<H)
		{
			if(foolLines[i])
			{
				lines++;
				remLines[lastRemLine].y=i+moreLines;
				remLines[lastRemLine].time=t;
				for(int j=0;j<W;j++)
				{
					remLines[lastRemLine].line[j]=field[i*W+j]&0x0f;
					remLines[lastRemLine].rnd[j]=frand(gseed);
				}

				lastRemLine=(lastRemLine+1)%NREMLINES;
				moreLines++;
				for(int i1=i;i1<(H-1);i1++)
				{
					foolLines[i1]=foolLines[i1+1];
					for(int j=0;j<W;j++)
					{
						field[i1*W+j]=field[(i1+1)*W+j];
						if(field[i1*W+j])field[i1*W+j]+=(moreLines<<20);
						fieldD[i1*W+j]=fieldD[(i1+1)*W+j]+1.0f;
					}
				}
				for(int j=0;j<W;j++)
					field[(H-1)*W+j]=0;
				foolLines[H-1]=false;
			}
			else
				i++;
		}
		if(moreLines)snd_play(sndExplode);

		if(moreLines==1)
		{
			scores+=10;
			//sound->Play(3);
		}
		if(moreLines==2)
		{
			scores+=30;
			//sound->Play(3);
		}
		if(moreLines==3)
		{
			scores+=60;
			//sound->Play(3);
		}
		if(moreLines==4)
		{
			scores+=100;
			//sound->Play(3);
		}
		level=lines/25;
	}

	bool doUpdate;
	bool falling;

	float t;

	void UpdateField(float dt)
	{
		if(pa>4.0)
		{
			pa-=4.0;
			newpa-=4;
		}
		{
			float delta=newpx-px;
			float fd=Min(Fabs(delta),dt*MOV_SPEED);
			if(delta>0)px+=fd;
			else px-=fd;
		}
		{
			float delta=newpy-py;
			float fd=Min(Fabs(delta),dt*MOV_SPEED);
			if(delta>0)py+=fd;
			else py-=fd;
		}
		{
			float delta=newpa-pa;
			float fd=Min(Fabs(delta),dt*ROT_SPEED);
			if(delta>0)pa+=fd;
			else pa-=fd;
		}
		if(doUpdate)
		{
			if(falling)
			{
				for(int i=0;i<4;i++)
				{
					if(Down())
					{
						snd_play(sndShoot);
						break;
					}
				}
			}
			else
				if(int(t)<int(t+dt))
					Down();
			t+=dt;
			RemoveLines();

			for(int i=0;i<H;i++)
			{
				for(int j=0;j<W;j++)
				{
					fieldD[i*W+j]-=dt*6;
					if(fieldD[i*W+j]<0)
						fieldD[i*W+j]=0;
				}
			}

			{
				for(int i=0;i<W*H;i++)mask[i]=0;
				for(int mfy=1;mfy<H;mfy++)
				{
					for(int mfx=0;mfx<W;mfx++)
					{
						if(field[mfx+(mfy)*W]!=0)
						{
							CBrickSet bs;
							bs.n=0;
							FindPiece(field, fieldD, mask, W, H, mfx, mfy, bs);
							if(bs.n)
							{
								bool shouldfall=true;
								for(int i=0;i<bs.n;i++)
								{
									if(bs.p[i].y==0)
									{
										shouldfall=false;
										break;
									}
									if(!
										(
										(field[bs.p[i].x+(bs.p[i].y-1)*W]==0)||
										(field[bs.p[i].x+(bs.p[i].y-1)*W]==field[bs.p[i].x+(bs.p[i].y)*W])
										)
									)
									{
										shouldfall=false;
										break;
									}
								}
								if(shouldfall)
								{
									for(int i=0;i<bs.n;i++)
									{
										field[bs.p[i].x+(bs.p[i].y)*W]=0;
										mask[bs.p[i].x+(bs.p[i].y)*W]=0;
										fieldD[bs.p[i].x+(bs.p[i].y)*W]=0.0f;
									}
									for(int i=0;i<bs.n;i++)
									{
										field[bs.p[i].x+(bs.p[i].y-1)*W]=bs.p[i].c;
										mask[bs.p[i].x+(bs.p[i].y-1)*W]=1;
										fieldD[bs.p[i].x+(bs.p[i].y-1)*W]=bs.p[i].dy+1.0f;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	bool Down()
	{
		int res=Put(piece,newpx,newpy-1,newpa%4,1,false);
		if(res&9)
		{
			Put(piece,newpx,newpy,newpa%4,col,true);
			NewPiece();
			falling=false;
			return true;
		}
		else
		{
			py=newpy;
			newpy--;
		}
		return false;
	}


	void Key(int key)
	{
		//printf("%i\n",key);
		if(key==4001)
		{
			Down();
		}
		else if(key==4002)
		{
			int res=Put(piece,newpx-1,newpy,newpa%4,1,false);
			if(res==0)
			{
				px=newpx;
				newpx--;
			}
		}
		else if(key==4003)
		{
			int res=Put(piece,newpx+1,newpy,newpa%4,1,false);
			if(res==0)
			{
				px=newpx;
				newpx++;
			}
		}
		else if(key==4000)
		{
			int res=Put(piece,newpx,newpy,(newpa+1)%4,1,false);
			if(res==0)
			{
				pa=newpa;
				newpa++;
				snd_play(sndRocket);
			}
		}
		else if(key==4004)
		{
			py=newpy;
			newpy--;
		}
		else if(key==4005)
		{
			py=newpy;
			newpy++;
		}
		//else if(key==4004)
		//{
		//	pa=newpa;
		//	newpa++;
		//}
		//else if(key==4005)
		//{
		//	pa=newpa+4;
		//	newpa+=3;
		//}
		else if(key==4008)
		{
			ClearField();
			NewPiece();	
		}
		else if(key==5103)
		{
			piece=(piece+1)%7;
		}
		else if(key==32)
		{
			falling=true;
		}
	}

	void Mouse(int mx, int my, int mb)
	{
		// if(mb)
		// {
		// 	int i=int((my-y)/cellSize);
		// 	int j=int((mx-x)/cellSize);
		// 	if((i>=0)&&(i<H)&&(j>=0)&&(j<W))
		// 	{
		// 		field[j+(H-i-1)*W]=1;
		// 	}
		// }

		if(0)
		{
			int i=int((my-y)/cellSize);
			int j=int((mx-x)/cellSize);
			if((i>=0)&&(i<H)&&(j>=0)&&(j<W))
			{
				i=H-i-1;
				printf("mm: %i %i >",j,i);
				CBrickSet bs;
				bs.n=0;
				for(int i=0;i<W*H;i++)mask[i]=0;
				FindPiece(field,fieldD,mask,W,H,j,i,bs);
				printf("%i\n",bs.n);
			}
		}
	}

	float px;
	float py;
	float pa;
	int newpx;
	int newpy;
	int newpa;

	int Put(int piece, int newpx, int newpy, int ori, int col, bool put)
	{
		int hit=0;
		int dx=offsets[piece*8+ori*2];
		int dy=offsets[piece*8+ori*2+1];
		int boxw=boxes[piece*2  ];
		int boxh=boxes[piece*2+1];
		int orxi[4]={ 0, 1, 0,-1};
		int orxj[4]={ 1, 0,-1, 0};
		int oryi[4]={-1, 0, 1, 0};
		int oryj[4]={ 0, 1, 0,-1};
		for(int i=0;i<boxh;i++)
		{
			for(int j=0;j<boxw;j++)
			{
				if(fig4[piece*16+i*4+j])
				{
					int cx=newpx+i*orxi[ori]+j*orxj[ori]+dx;
					if(cx<0)hit|=HIT_LEFT;
					if(cx>=W)hit|=HIT_RIGHT;
					int cy=newpy+i*oryi[ori]+j*oryj[ori]+dy;
					if(cy<0)hit|=HIT_BOTTOM;
					int index=cy*W+cx;
					if(put)
					{
						if((cx>=0)&&(cx<W)&&(cy>=0)&&(cy<H))
							field[index]=col|(order<<4);
					}
					else
					{
						if((cx>=0)&&(cx<W)&&(cy>=0)&&(cy<H))
						{
							if(field[index])
								hit|=HIT_CELL;
						}
					}
				}
			}
		}
		return hit;
	}

	void RenderGrid()
	{
		g.clear();
		g.M(x,y);
		g.l(W*cellSize,0);
		g.l(0,H*cellSize);
		g.l(-W*cellSize,0);
		g.close();
		g.fin();
		g.rgb(1,1,1);
		g.alpha(.35);
		g.fill1();
		g.width(1,1);
		g.rgb(.9,.6,.5);
		g.stroke();

		g.clear();
		for(int i=1;i<H;i++)
		{
			g.M(x,y+i*cellSize);
			g.l(W*cellSize,0);
		}
		for(int i=1;i<W;i++)
		{
			g.M(x+i*cellSize,y);
			g.l(0,H*cellSize);
		}
		g.fin();
		g.width(1.,1.);
		g.rgb(.9,.6,.5);
		g.stroke();
	}

	void RenderField()
	{
		RenderGrid();

		float apx=int(px+256)-256;
		float dpx=px-apx;
		if(px<newpx)apx+=(dpx)*(dpx);
		else apx+=1-(1-dpx)*(1-dpx);

		float apy=int(py+256)-256;
		float dpy=py-apy;
		if(py<newpy)apy+=(dpy)*(dpy);
		else apy+=1-(1-dpy)*(1-dpy);

		if(1)
		{
			//newpa=int(Time())%4;
			//piece=6;
			int ori=newpa%4;


			// for(int i=0;i<W*H;i++)field[i]=0;
			// Put(ori,dx,dy,true);

			bool hit=Put(piece,newpx, newpy, ori, 1, false);
			char s[64];
			snprintf(s,64,"Score: %i", scores);
			stext(s,240,30,0xff00ffff);
		}


		for(int i=0;i<H;i++)
		{
			for(int j=0;j<W;j++)
			{
				if(field[(H-i-1)*W+j])
				{
					DrawChip(j,i, -fieldD[(H-i-1)*W+j], field[(H-i-1)*W+j]&0x0f);
				}
			}
		}
		for(int i=0;i<H;i++)
		{
			for(int j=0;j<W;j++)
			{
				int c=field[(H-i-1)*W+j];
				float d=-fieldD[(H-i-1)*W+j];
				if(c)
				{
					bool tp=true;
					bool bt=true;
					bool lt=true;
					bool rt=true;
					if((j<(W-1))&&(field[j+1+(H-i-1)*W]==c))rt=false;
					if((j>0    )&&(field[j-1+(H-i-1)*W]==c))lt=false;
					if((i<(H-1))&&(field[j+  (H-i-2)*W]==c))tp=false;
					if((i>0    )&&(field[j+  (H-i-0)*W]==c))bt=false;
					float X=x+j*cellSize-.5;
					float Y=y+i*cellSize+d*cellSize-.5;

					g.rgba32(stdcols[c&0xf]);
					if(!rt)g.hairline(X+cellSize,Y+1,X+cellSize,Y+cellSize);
					if(!tp)g.hairline(X+1,Y+cellSize,X+cellSize,Y+cellSize);

					g.rgba(1,1,1,.5);
					if(lt)g.hairline(X+1,Y+1,X+1,Y+cellSize);
					if(bt)g.hairline(X+1,Y+1,X+cellSize,Y+1);
					g.rgba(0,0,0,.5);
					if(rt)g.hairline(X+cellSize,Y,X+cellSize,Y+cellSize);
					if(tp)g.hairline(X,Y+cellSize,X+cellSize,Y+cellSize);
				}
			}
		}

		// for(int i=0;i<H;i++)
		// {
		// 	if(fieldD[(H-i-1)*W])
		// 	{
		// 		char s[64];
		// 		int ly=i;
		// 		snprintf(s,64,"%3.2f",fieldD[(H-i-1)*W]);
		// 		stext(s,240,y+ly*cellSize,0xff00ff00);
		// 	}
		// }

		for(int i=NREMLINES-1;i>=0;i--)
		{
			float dt=(t-remLines[i].time);
			if(dt<0)
				continue;
			if(dt>2)
				continue;
			int ly=H-1-remLines[i].y;
			for(int j=0;j<W;j++)
			{
				int ci=remLines[i].line[j];
				float rnd=remLines[i].rnd[j];
				float lx=j/(float)W-.5;
				DrawAChip(
					x+j*cellSize+lx*Fabs(lx)*cellSize*20*dt,
					y+ly*cellSize+dt*dt*cellSize*10*(1.5+rnd),dt*rnd,cellSize/2,ci);
			}
		}
		DrawPiece(x+(apx)*cellSize,y+H*cellSize-apy*cellSize,pa,piece,col);
		DrawPiece(50.5,70.5,0,nextPiece, nextcol);
	}
};

Game game;

int main()
{
	float tframe0;
	tframe0=Time();
	game.Init();
	while(true)
	{
		float tframe1=Time();
		float dt=tframe1-tframe0;
		tframe0=tframe1;
		Background();
		//Present();continue;
		game.UpdateField(dt);
		game.RenderField();
		char s[64];
		snprintf(s,64,"%f",Time());
		//stext(s,10,10,0xffffffff);
		if(!game.doUpdate)stext("Paused",10,20,0xffff0000);
		stext("Next",40,30,0xff00ffff);

		int mb;
		int mx;
		int my;
		GetMouseState(mx, my, mb);
		game.Mouse(mx, my, mb);

		int key;
		int press;
		if(GetKeyEvent(key,press))
		{
			if(press>0)
			{
				if(key=='1'){puts("Explode");snd_play(game.sndExplode);}
				if(key=='2'){puts("Rocket");snd_play(game.sndRocket);}
				if(key=='3'){puts("Shoot");snd_play(game.sndShoot);}
				if(key==4010)
				{
					game.doUpdate=!game.doUpdate;
				}
				else if(key==1000)
				{
					exit(0);
				}
				else
				{
					game.Key(key);
				}
				//printf("Key pressed %i %i\n", key, press);
			}
		}
		Present();
	}
	return 0;
}