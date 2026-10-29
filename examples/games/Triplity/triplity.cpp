// triplity.cpp - a night-sky merging puzzle.
//
// Drop the piece shown on the right onto an empty cell of the 6x6 sky.
// Three or more equal pieces touching side by side fuse into the next one,
// on the cell where the last piece landed:
//
//   Stardust -> Spark -> Star -> Twin stars -> Constellation -> Nebula
//            -> Galaxy -> Quasar,  and three Quasars end in a Big Bang
//            that clears them away for a huge bonus.
//
// Bigger groups and chains of fusions in one move score more.
//
// Comets fly one cell after every move, straight ahead; when blocked they
// turn (right first, then left, then back). A comet with nowhere to go burns
// out and leaves Stardust behind - which may fuse in turn.
// A Black hole swallows whatever it is dropped on, comets included.
// The telescope (right column) keeps one piece aside: click it, or press
// Space, to swap. The game ends when the sky is full.
//
// Run with -x2 (Scinc triplity.cpp -x2) for HiDPI screens: the window keeps
// its size on screen but renders at twice the resolution.
//
// The sprites are SVG files in sprites/, drawn with svg.h. Best score and
// a few totals are kept in triplity.sav next to this file.

#include <stdio.h>

#define G_SCREEN_WIDTH 780
#define G_SCREEN_HEIGHT 640
#define G_SCREEN_MODE 1   // XShm: faster presents than mode 0 (plain XPutImage)

#include "graphics.h"
#include "svg.h"
#include "../../ws.h"
#include "../../graphic/font.h"
#include "../../graphic/letters.h"
#include "../../graphic/gtext.h"

#define M_PI 3.14159265358979

Graph g;
CFont* pfont=0;

typedef char* pchar;

// rendering scale: 1 normally, 2 with -x2. Layout stays in logical units:
// the path transform scales paths, and the calls it doesn't reach (stroke
// widths, fillrect, Circle, SVG) multiply by SC themselves; mouse positions
// are divided back.
float SC=1;

void Circle(float x, float y, float r, float w, float a, int c)
{
	g.Circle(x*SC,y*SC,r*SC,w*SC,a*SC,c);
}

// ---------------------------------------------------------------- layout

#define N 6
#define NC 36
#define CELL 84
#define BX 24
#define BY 72
#define RX 552
#define RW 208

#define COL_TEXT  0xffffffff
#define COL_DIM   0xff8a93c0
#define COL_PANEL 0xff141a40
#define COL_CELL  0xff1b2352
#define COL_CELL2 0xff19204c
#define COL_GOLD  0xffffc850
#define COL_BTN   0xff2c3670
#define COL_GO    0xff3f8f5a

#define KEY_ENTER 4013
#define KEY_ENTER2 5013
#define KEY_ESC   1000

// ---------------------------------------------------------------- pieces

// board values: 0 empty, 1..8 sky objects, COMET; BHOLE only ever in hand
#define TOP 8
#define COMET 20
#define BHOLE 21

int sprite[22];        // SVG id per piece value
int tierPts[9];

char* PieceName(int p)
{
	if(p==1)return "Stardust";
	if(p==2)return "Spark";
	if(p==3)return "Star";
	if(p==4)return "Twin stars";
	if(p==5)return "Constellation";
	if(p==6)return "Nebula";
	if(p==7)return "Galaxy";
	if(p==8)return "Quasar";
	if(p==COMET)return "Comet";
	if(p==BHOLE)return "Black hole";
	return "";
}

void LoadSprites()
{
	for(int i=0;i<22;i++)sprite[i]=-1;
	sprite[1]=SVGLoad("sprites/dust.svg");
	sprite[2]=SVGLoad("sprites/spark.svg");
	sprite[3]=SVGLoad("sprites/star.svg");
	sprite[4]=SVGLoad("sprites/twin.svg");
	sprite[5]=SVGLoad("sprites/constellation.svg");
	sprite[6]=SVGLoad("sprites/nebula.svg");
	sprite[7]=SVGLoad("sprites/galaxy.svg");
	sprite[8]=SVGLoad("sprites/quasar.svg");
	sprite[COMET]=SVGLoad("sprites/comet.svg");
	sprite[BHOLE]=SVGLoad("sprites/blackhole.svg");
	tierPts[0]=0;
	tierPts[1]=5;
	tierPts[2]=20;
	tierPts[3]=100;
	tierPts[4]=400;
	tierPts[5]=1500;
	tierPts[6]=5000;
	tierPts[7]=15000;
	tierPts[8]=50000;
}

// ---------------------------------------------------------------- random

int seed=1234567;

int irand()
{
	seed=(seed*1103515245+12345)&0x7fffffff;
	return (seed>>8)&0x7fffff;
}

int rnd(int n)
{
	return irand()%n;
}

// ---------------------------------------------------------------- game state

int board[NC];
int cdir[NC];          // comet heading: 0 up, 1 right, 2 down, 3 left
int fresh[NC];         // comet placed during this move - it waits a move
int cur=1;             // piece in hand
int nxt=1;             // the one after it
int hold=0;            // telescope, 0 empty
int score=0;
int moves=0;
int over=0;            // the sky is full
float overT=0;
int bestTierRun=0;     // highest tier made in this game
float newConfirmT=-10;
int statsOpen=0;
int overClosed=0;      // game-over card dismissed to look at the sky

// saved totals
int best=0;
int games=0;
int bestTier=0;
int totalFusions=0;

int dx4[4];
int dy4[4];

int Px(int i){return i%N;}
int Py(int i){return i/N;}

// neighbour of cell i in direction d, -1 off the board
int Nb(int i, int d)
{
	int x=Px(i)+dx4[d];
	int y=Py(i)+dy4[d];
	if(x<0||y<0||x>=N||y>=N)return -1;
	return y*N+x;
}

int Gen()
{
	int r=rnd(1000);
	if(r<580)return 1;
	if(r<760)return 2;
	if(r<795)return 3;
	if(r<800)return 4;
	if(r<940||moves<6)return COMET;   // no black holes in the first moves
	return BHOLE;
}

// group of equal sky objects connected to cell c; marks it in grp[], returns size
int grp[NC];
int stack[NC];

int Group(int c, int t)
{
	for(int i=0;i<NC;i++)grp[i]=0;
	int n=0;
	int sp=0;
	stack[sp]=c;
	sp++;
	grp[c]=1;
	while(sp>0)
	{
		sp--;
		int i=stack[sp];
		n++;
		for(int d=0;d<4;d++)
		{
			int j=Nb(i,d);
			if(j>=0&&grp[j]==0&&board[j]==t)
			{
				grp[j]=1;
				stack[sp]=j;
				sp++;
			}
		}
	}
	return n;
}

// group that a piece of tier t dropped on empty cell c would join
int GroupIfPlaced(int c, int t)
{
	int keep=board[c];
	board[c]=t;
	int n=Group(c,t);
	board[c]=keep;
	return n;
}

int EmptyCells()
{
	int n=0;
	for(int i=0;i<NC;i++)if(board[i]==0)n++;
	return n;
}

// ---------------------------------------------------------------- effects

#define NPOP 12
float popX[NPOP];
float popY[NPOP];
float popT[NPOP];
char popS[NPOP*24];
int popNext=0;

void Popup(int cell, char* s)
{
	int k=popNext;
	popNext=(popNext+1)%NPOP;
	popX[k]=BX+Px(cell)*CELL+CELL/2;
	popY[k]=BY+Py(cell)*CELL+CELL/2;
	popT[k]=Time();
	int i=0;
	while(s[i]!=0&&i<23)
	{
		popS[k*24+i]=s[i];
		i++;
	}
	popS[k*24+i]=0;
}

void AddScore(int cell, int pts)
{
	score+=pts;
	char s[24];
	snprintf(s,23,"+%i",pts);
	Popup(cell,s);
}

float bornT[NC];       // pop-in animation per cell

char msg[80];
float msgT0=-10;

void Message(char* s)
{
	int i=0;
	while(s[i]!=0&&i<79)
	{
		msg[i]=s[i];
		i++;
	}
	msg[i]=0;
	msgT0=Time();
}

// ---------------------------------------------------------------- saving

char* SAVE_FILE="triplity.sav";

int ParseInt(char* s)
{
	int v=0;
	int i=0;
	while(s[i]>='0'&&s[i]<='9')
	{
		v=v*10+s[i]-'0';
		i++;
	}
	return v;
}

void LoadTotals()
{
	best=0;
	games=0;
	bestTier=0;
	totalFusions=0;
	FILE* f=fopen(SAVE_FILE,"rb");
	if(f==0)return;
	char s[64];
	int k=0;
	while(fgets(s,64,f))
	{
		int v=ParseInt(s);
		if(k==0)best=v;
		if(k==1)games=v;
		if(k==2)bestTier=v;
		if(k==3)totalFusions=v;
		k++;
	}
	fclose(f);
}

void SaveTotals()
{
	FILE* f=fopen(SAVE_FILE,"wb");
	if(f==0)return;
	char s[64];
	snprintf(s,63,"%i best score\n",best);
	fputs(s,f);
	snprintf(s,63,"%i games\n",games);
	fputs(s,f);
	snprintf(s,63,"%i best piece\n",bestTier);
	fputs(s,f);
	snprintf(s,63,"%i fusions\n",totalFusions);
	fputs(s,f);
	fclose(f);
}

// ---------------------------------------------------------------- a move, step by step
//
// A move runs as a little pipeline so every step can be animated:
// fusions (one group at a time, cascading), then the comets fly, then
// trapped comets burn out into Stardust, which may fuse again.

#define PH_IDLE 0
#define PH_FUSE 1
#define PH_FLY 2
#define PH_BURN 3
#define PH_SWALLOW 4

#define T_FUSE 0.26
#define T_FLY 0.30
#define T_BURN 0.45
#define T_SWALLOW 0.45

int phase=PH_IDLE;
float phaseT0=0;

int checkQ[64];
int nCheck=0;
int cometsFlown=0;
int chain=0;

int fuseCell=-1;
int fuseMask[NC];
int fuseTier=0;
int fuseSize=0;

int flyFrom[NC];       // for a comet now at i: the cell it came from, -1 none
int burnMask[NC];
int swallowCell=-1;
int swallowWhat=0;

void PushCheck(int c)
{
	if(nCheck<64)
	{
		checkQ[nCheck]=c;
		nCheck++;
	}
}

int PopCheck()
{
	int c=checkQ[0];
	for(int i=1;i<nCheck;i++)checkQ[i-1]=checkQ[i];
	nCheck--;
	return c;
}

void StartPhase(int p)
{
	phase=p;
	phaseT0=Time();
}

int CanPlay()
{
	if(EmptyCells()>0)return 1;
	// a full sky still has a move while a black hole is at hand
	if(cur==BHOLE||hold==BHOLE)return 1;
	return 0;
}

void EndMove()
{
	phase=PH_IDLE;
	moves++;
	if(CanPlay()==0)
	{
		over=1;
		overT=Time();
		games++;
		if(score>best)best=score;
		if(bestTierRun>bestTier)bestTier=bestTierRun;
		SaveTotals();
	}
}

// Move every comet one step. Returns 1 if anything moved.
int FlyComets()
{
	int moved=0;
	int done[NC];
	for(int i=0;i<NC;i++)
	{
		done[i]=0;
		flyFrom[i]=-1;
	}
	for(int i=0;i<NC;i++)
	{
		if(board[i]!=COMET||done[i]!=0)continue;
		if(fresh[i]!=0)
		{
			fresh[i]=0;
			done[i]=1;
			continue;
		}
		int turns[4];
		turns[0]=0;
		turns[1]=1;
		turns[2]=3;
		turns[3]=2;
		for(int k=0;k<4;k++)
		{
			int d=(cdir[i]+turns[k])%4;
			int j=Nb(i,d);
			if(j>=0&&board[j]==0)
			{
				board[j]=COMET;
				cdir[j]=d;
				fresh[j]=0;
				done[j]=1;
				flyFrom[j]=i;
				board[i]=0;
				moved=1;
				break;
			}
		}
	}
	return moved;
}

// mark comets with no free neighbour; returns how many
int FindTrapped()
{
	int n=0;
	for(int i=0;i<NC;i++)
	{
		burnMask[i]=0;
		if(board[i]!=COMET)continue;
		int free=0;
		for(int d=0;d<4;d++)
		{
			int j=Nb(i,d);
			if(j>=0&&board[j]==0)free=1;
		}
		if(free==0)
		{
			burnMask[i]=1;
			n++;
		}
	}
	return n;
}

// continue the move from wherever it stands
void Advance()
{
	while(nCheck>0)
	{
		int c=PopCheck();
		int t=board[c];
		if(t<1||t>TOP)continue;
		int n=Group(c,t);
		if(n>=3)
		{
			fuseCell=c;
			fuseTier=t;
			fuseSize=n;
			for(int i=0;i<NC;i++)fuseMask[i]=grp[i];
			StartPhase(PH_FUSE);
			return;
		}
	}
	if(cometsFlown==0)
	{
		cometsFlown=1;
		int any=0;
		for(int i=0;i<NC;i++)if(board[i]==COMET)any=1;
		if(any!=0)
		{
			FlyComets();
			StartPhase(PH_FLY);
			return;
		}
	}
	EndMove();
}

void FuseDone()
{
	chain++;
	int t=fuseTier;
	for(int i=0;i<NC;i++)if(fuseMask[i]!=0)board[i]=0;
	totalFusions++;
	if(t==TOP)
	{
		// three quasars: Big Bang - they vanish, the sky gets room
		AddScore(fuseCell,tierPts[TOP]*4*(fuseSize-2)*chain);
		Message("BIG BANG!");
	}
	else
	{
		board[fuseCell]=t+1;
		bornT[fuseCell]=Time();
		AddScore(fuseCell,tierPts[t+1]*(fuseSize-2)*chain);
		if(t+1>bestTierRun)bestTierRun=t+1;
		if(chain>1)
		{
			char s[32];
			snprintf(s,31,"Chain x%i",chain);
			Message(s);
		}
		PushCheck(fuseCell);
	}
	StartPhase(PH_IDLE);
	Advance();
}

void FlyDone()
{
	for(int i=0;i<NC;i++)flyFrom[i]=-1;
	if(FindTrapped()>0)
	{
		StartPhase(PH_BURN);
		return;
	}
	StartPhase(PH_IDLE);
	Advance();
}

void BurnDone()
{
	for(int i=0;i<NC;i++)
	{
		if(burnMask[i]==0)continue;
		burnMask[i]=0;
		board[i]=1;
		bornT[i]=Time();
		AddScore(i,50);
		PushCheck(i);
	}
	StartPhase(PH_IDLE);
	Advance();
}

void SwallowDone()
{
	board[swallowCell]=0;
	if(swallowWhat==COMET)AddScore(swallowCell,100);
	swallowCell=-1;
	StartPhase(PH_IDLE);
	Advance();
}

// best heading for a new comet: the longest free run, ties at random
int CometHeading(int c)
{
	int bestD=rnd(4);
	int bestRun=-1;
	int start=rnd(4);
	for(int k=0;k<4;k++)
	{
		int d=(start+k)%4;
		int run=0;
		int j=Nb(c,d);
		while(j>=0&&board[j]==0)
		{
			run++;
			j=Nb(j,d);
		}
		if(run>bestRun)
		{
			bestRun=run;
			bestD=d;
		}
	}
	return bestD;
}

void TakeNext()
{
	cur=nxt;
	nxt=Gen();
}

// the player clicked cell c
void Play(int c)
{
	if(phase!=PH_IDLE||over!=0)return;
	nCheck=0;
	cometsFlown=0;
	chain=0;
	if(cur==BHOLE)
	{
		if(board[c]==0)
		{
			Message("A black hole needs something to swallow");
			return;
		}
		swallowCell=c;
		swallowWhat=board[c];
		TakeNext();
		StartPhase(PH_SWALLOW);
		return;
	}
	if(board[c]!=0)return;
	board[c]=cur;
	bornT[c]=Time();
	if(cur==COMET)
	{
		cdir[c]=CometHeading(c);
		fresh[c]=1;
	}
	else
	{
		AddScore(c,tierPts[cur]);
		if(cur>bestTierRun)bestTierRun=cur;
		PushCheck(c);
	}
	TakeNext();
	Advance();
}

void SwapHold()
{
	if(phase!=PH_IDLE||over!=0)return;
	if(hold==0)
	{
		hold=cur;
		TakeNext();
	}
	else
	{
		int t=hold;
		hold=cur;
		cur=t;
	}
	if(CanPlay()==0)EndMove();
}

void NewGame()
{
	for(int i=0;i<NC;i++)
	{
		board[i]=0;
		cdir[i]=0;
		fresh[i]=0;
		bornT[i]=-10;
		fuseMask[i]=0;
		burnMask[i]=0;
		flyFrom[i]=-1;
	}
	for(int k=0;k<NPOP;k++)popT[k]=-10;
	score=0;
	moves=0;
	over=0;
	hold=0;
	bestTierRun=1;
	phase=PH_IDLE;
	nCheck=0;
	swallowCell=-1;
	msgT0=-10;
	overClosed=0;
	newConfirmT=-10;
	statsOpen=0;
	// a few pieces to start with, never three of a kind together
	int placed=0;
	while(placed<9)
	{
		int c=rnd(NC);
		if(board[c]!=0)continue;
		int t=1;
		int r=rnd(10);
		if(r>=6)t=2;
		if(r>=9)t=3;
		if(GroupIfPlaced(c,t)>=3)continue;
		board[c]=t;
		placed++;
	}
	cur=Gen();
	if(cur==COMET)cur=1;
	nxt=Gen();
}

// ---------------------------------------------------------------- drawing helpers

void RoundRect(float x, float y, float w, float h, float r)
{
	g.M(x,y+r);
	if(r>0)g.c(0,-r,r,-r,r,-r);
	g.l(w-r*2,0);
	if(r>0)g.c(r,0,r,r,r,r);
	g.l(0,h-r*2);
	if(r>0)g.c(0,r,-r,r,-r,r);
	g.l(-w+r*2,0);
	if(r>0)g.c(-r,0,-r,-r,-r,-r);
	g.close();
}

void FillBox(float x, float y, float w, float h, float r, int c)
{
	g.clear();
	RoundRect(x,y,w,h,r);
	g.fin();
	g.rgba32(c);
	g.fill1();
}

// g.width(r,k): r is the half-width, r/k the soft edge (see Wordy)
void Stroke(float r, float k)
{
	g.width(r*SC,k);
	g.stroke();
}

int TextLen(char* s)
{
	int n=0;
	while(s[n]!=0)n++;
	return n;
}

void Text(char* s, float x, float y, float sc, int c, float r)
{
	g.clear();
	DrawText(g,*pfont,s,0,x,y,sc,sc);
	g.fin();
	g.rgba32(c);
	Stroke(r,2.);
}

void TextC(char* s, float cx, float cy, float sc, int c, float r)
{
	float tw=(TextLen(s)*6-2)*sc;
	Text(s,cx-tw/2,cy+4.5*sc,sc,c,r);
}

float Clamp01(float x)
{
	if(x<0)return 0;
	if(x>1)return 1;
	return x;
}

float EaseOut(float t)
{
	return 1-(1-t)*(1-t);
}

int Over(float x, float y, float w, float h, int mx, int my)
{
	return mx>=x&&mx<x+w&&my>=y&&my<y+h;
}

void Button(char* s, float x, float y, float w, float h, int c, int mx, int my)
{
	FillBox(x,y,w,h,6,c);
	if(Over(x,y,w,h,mx,my))FillBox(x,y,w,h,6,0x24ffffff);
	TextC(s,x+w/2,y+h/2,1.4,COL_TEXT,1.2);
}

// piece p centred at (cx,cy), size in pixels, rotated by angle (radians)
void DrawPiece(int p, float cx, float cy, float size, float ang)
{
	if(p<=0||sprite[p]<0)return;
	float s=size/100;
	float a=cos(ang)*s;
	float b=sin(ang)*s;
	SVGSetTransform((cx-(a*50-b*50))*SC,(cy-(b*50+a*50))*SC,a*SC,b*SC,-b*SC,a*SC);
	SVGDraw(sprite[p]);
}

float CometAngle(int d)
{
	if(d==0)return -M_PI/2;
	if(d==1)return 0;
	if(d==2)return M_PI/2;
	return M_PI;
}

// ---------------------------------------------------------------- background

#define NSTARS 90
float starX[NSTARS];
float starY[NSTARS];
float starR[NSTARS];
float starPh[NSTARS];

void MakeStars()
{
	for(int i=0;i<NSTARS;i++)
	{
		starX[i]=rnd(G_SCREEN_WIDTH);
		starY[i]=rnd(G_SCREEN_HEIGHT);
		starR[i]=0.4+rnd(100)*0.012;
		starPh[i]=rnd(628)*0.01;
	}
}

void DrawSky(float T)
{
	// vertical gradient in bands
	for(int i=0;i<16;i++)
	{
		float t=i/15.0;
		int r=int(8+t*22);
		int gg=int(10+t*8);
		int b=int(32+t*30);
		int c=0xff000000|(r<<16)|(gg<<8)|b;
		float y=i*40;
		float h=41;
		g.rgba32(c);
		g.fillrect(0,y*SC,G_SCREEN_WIDTH*SC,h*SC);
	}
	for(int i=0;i<NSTARS;i++)
	{
		float a=0.35+0.3*sin(starPh[i]);
		int ai=int(a*255);
		Circle(starX[i],starY[i],0,starR[i],1.,(ai<<24)|0xdfe6ff);
	}
}

// ---------------------------------------------------------------- board drawing

float CellX(int i){return BX+Px(i)*CELL+CELL/2;}
float CellY(int i){return BY+Py(i)*CELL+CELL/2;}

int hoverCell=-1;
float hoverT0=0;   // when the mouse entered hoverCell

// does the hovered placement fuse this cell? (for the preview wiggle)
int previewMask[NC];
int previewOn=0;

void UpdatePreview()
{
	previewOn=0;
	for(int i=0;i<NC;i++)previewMask[i]=0;
	if(hoverCell<0||phase!=PH_IDLE||over!=0)return;
	if(cur<1||cur>TOP||board[hoverCell]!=0)return;
	if(GroupIfPlaced(hoverCell,cur)>=3)
	{
		board[hoverCell]=cur;
		Group(hoverCell,cur);
		board[hoverCell]=0;
		for(int i=0;i<NC;i++)previewMask[i]=grp[i];
		previewMask[hoverCell]=0;
		previewOn=1;
	}
}

void DrawBoard(float T)
{
	FillBox(BX-10,BY-10,N*CELL+20,N*CELL+20,14,0xc0101538);
	for(int i=0;i<NC;i++)
	{
		int c=COL_CELL;
		if((Px(i)+Py(i))%2==1)c=COL_CELL2;
		float x=BX+Px(i)*CELL;
		float y=BY+Py(i)*CELL;
		FillBox(x+3,y+3,CELL-6,CELL-6,10,c);
		if(i==hoverCell&&phase==PH_IDLE&&over==0)
		{
			int hc=0x30ffffff;
			if(cur==BHOLE&&board[i]!=0)hc=0x50ff5080;
			FillBox(x+3,y+3,CELL-6,CELL-6,10,hc);
		}
	}

	float pt=(T-phaseT0);
	for(int i=0;i<NC;i++)
	{
		int p=board[i];
		if(p==0)continue;
		float cx=CellX(i);
		float cy=CellY(i);
		float size=CELL*0.92;
		float ang=0;
		// pop in
		float u=(T-bornT[i])/0.25;
		if(u>=0&&u<1)size=size*(0.6+0.4*EaseOut(u)+0.15*sin(M_PI*u));
		// fusing pieces slide into the target cell
		if(phase==PH_FUSE&&fuseMask[i]!=0&&i!=fuseCell)
		{
			float f=EaseOut(Clamp01(pt/T_FUSE));
			cx=cx+(CellX(fuseCell)-cx)*f;
			cy=cy+(CellY(fuseCell)-cy)*f;
			size=size*(1-0.4*f);
		}
		if(phase==PH_FUSE&&i==fuseCell)size=size*(1+0.12*sin(M_PI*Clamp01(pt/T_FUSE)));
		// hover preview: pieces that would fuse lean towards the cell
		if(previewOn!=0&&previewMask[i]!=0)
		{
			float w=0.11*EaseOut(Clamp01((T-hoverT0)/0.2));
			cx=cx+(CellX(hoverCell)-cx)*w;
			cy=cy+(CellY(hoverCell)-cy)*w;
		}
		if(p==COMET)
		{
			ang=CometAngle(cdir[i]);
			if(phase==PH_FLY&&flyFrom[i]>=0)
			{
				float f=EaseOut(Clamp01(pt/T_FLY));
				cx=CellX(flyFrom[i])+(cx-CellX(flyFrom[i]))*f;
				cy=CellY(flyFrom[i])+(cy-CellY(flyFrom[i]))*f;
			}
			if(phase==PH_BURN&&burnMask[i]!=0)
			{
				float f=Clamp01(pt/T_BURN);
				size=size*(1-f);
				ang=ang+f*6;
				int a=int((1-f)*200);
				Circle(cx,cy,f*CELL*0.6,4,6,(a<<24)|0xffe0a0);
			}
		}
		if(phase==PH_SWALLOW&&i==swallowCell)
		{
			float f=Clamp01(pt/T_SWALLOW);
			DrawPiece(BHOLE,cx,cy,CELL*(0.5+0.6*f),0);
			size=size*(1-f);
			ang=f*8;
		}
		DrawPiece(p,cx,cy,size,ang);
	}

	// ghost of the piece in hand
	if(hoverCell>=0&&phase==PH_IDLE&&over==0&&board[hoverCell]==0&&cur!=BHOLE)
	{
		float cx=CellX(hoverCell);
		float cy=CellY(hoverCell);
		float ang=0;
		if(cur==COMET)ang=CometAngle(CometHeading(hoverCell));
		DrawPiece(cur,cx,cy,CELL*0.8,ang);
		FillBox(cx-CELL/2+3,cy-CELL/2+3,CELL-6,CELL-6,10,0x701b2352);
	}
	if(hoverCell>=0&&phase==PH_IDLE&&over==0&&cur==BHOLE&&board[hoverCell]!=0)
	{
		DrawPiece(BHOLE,CellX(hoverCell),CellY(hoverCell),CELL*0.5,2*EaseOut(Clamp01((T-hoverT0)/0.6)));
	}

	// score popups
	for(int k=0;k<NPOP;k++)
	{
		float u=(T-popT[k])/1.1;
		if(u<0||u>=1)continue;
		int a=int((1-u*u)*255);
		char s[24];
		for(int i=0;i<24;i++)s[i]=popS[k*24+i];
		TextC(s,popX[k],popY[k]-20-u*40,1.8,(a<<24)|0xffe090,1.4);
	}
}

// ---------------------------------------------------------------- side column

#define HOLD_Y 416
#define HOLD_H 112

void Label(char* s, float y)
{
	TextC(s,RX+RW/2,y,1.3,COL_DIM,1.1);
}

void DrawSide(float T, int mx, int my)
{
	char s[40];
	FillBox(RX,BY-10,RW,96,12,COL_PANEL);
	Label("SCORE",BY+8);
	snprintf(s,39,"%i",score);
	TextC(s,RX+RW/2,BY+40,3.2,COL_TEXT,2.);
	snprintf(s,39,"best %i",best);
	TextC(s,RX+RW/2,BY+70,1.3,COL_DIM,1.1);

	// piece in hand
	FillBox(RX,BY+96,RW,176,12,COL_PANEL);
	Label("PLACE",BY+114);
	float ang=0;
	if(cur==COMET)ang=-0.4;
	DrawPiece(cur,RX+RW/2,BY+186,110,ang);
	TextC(PieceName(cur),RX+RW/2,BY+254,1.6,COL_GOLD,1.3);

	// next
	FillBox(RX,BY+282,RW,52,12,COL_PANEL);
	Text("NEXT",RX+16,BY+313,1.3,COL_DIM,1.1);
	DrawPiece(nxt,RX+84,BY+308,46,0);
	Text(PieceName(nxt),RX+112,BY+313,1.3,COL_TEXT,1.1);

	// telescope (hold)
	FillBox(RX,HOLD_Y,RW,HOLD_H,12,COL_PANEL);
	if(Over(RX,HOLD_Y,RW,HOLD_H,mx,my)&&phase==PH_IDLE&&over==0)FillBox(RX,HOLD_Y,RW,HOLD_H,12,0x20ffffff);
	Label("TELESCOPE",HOLD_Y+18);
	if(hold!=0)
	{
		DrawPiece(hold,RX+RW/2,HOLD_Y+60,58,0);
		TextC(PieceName(hold),RX+RW/2,HOLD_Y+97,1.3,COL_TEXT,1.1);
	}
	else
	{
		TextC("empty - click or",RX+RW/2,HOLD_Y+52,1.3,COL_DIM,1.1);
		TextC("Space to keep a",RX+RW/2,HOLD_Y+70,1.3,COL_DIM,1.1);
		TextC("piece for later",RX+RW/2,HOLD_Y+88,1.3,COL_DIM,1.1);
	}

	// the ladder of sky objects; the ones reached this game are lit
	float ly=HOLD_Y+HOLD_H+10;
	FillBox(RX,ly,RW,60,12,COL_PANEL);
	for(int t=1;t<=TOP;t++)
	{
		float x=RX+14+(t-1)*24.5+11;
		float y=ly+30;
		DrawPiece(t,x,y,32,0);
		if(t>bestTierRun)Circle(x,y,0,13,2.,0xc0141a40);
	}
}

// one line of advice about the piece in hand, under the board
void DrawHint()
{
	char* a="";
	char* b="";
	if(cur>=1&&cur<TOP)
	{
		char s[64];
		snprintf(s,63,"Three %s together make a %s.",PieceName(cur),PieceName(cur+1));
		TextC(s,BX+N*CELL/2,BY+N*CELL+28,1.4,COL_DIM,1.1);
		return;
	}
	if(cur==TOP)
	{
		a="Three Quasars collapse in a Big Bang:";
		b="a huge bonus and room to breathe.";
	}
	if(cur==COMET)
	{
		a="Comets fly a cell every move and turn when blocked.";
		b="Box one in and it burns out into Stardust.";
	}
	if(cur==BHOLE)
	{
		a="Drop the black hole on anything to swallow it.";
		b="Swallowing a comet is worth 100.";
	}
	TextC(a,BX+N*CELL/2,BY+N*CELL+24,1.4,COL_DIM,1.1);
	TextC(b,BX+N*CELL/2,BY+N*CELL+44,1.4,COL_DIM,1.1);
}

void DrawMessage(float T)
{
	float t=T-msgT0;
	if(t<0||t>1.6)return;
	float a=1;
	if(t>1.2)a=(1.6-t)/0.4;
	int ai=int(a*255);
	float sc=2.4;
	float w=(TextLen(msg)*6-2)*sc+40;
	float cx=BX+N*CELL/2;
	float y=BY+N*CELL/2-26-t*12;
	FillBox(cx-w/2,y,w,52,10,(int(a*220)<<24)|0x0a0e28);
	TextC(msg,cx,y+26,sc,(ai<<24)|0xffe090,1.6);
}

// ---------------------------------------------------------------- header, overlays

#define BTN_Y 12
#define BTN_H 30
#define BTN_W 120


#define OV_X 130
#define OV_Y 190
#define OV_W 300
#define OV_H 230

void DrawHeader(int mx, int my)
{
	TextC("TRIPLITY",G_SCREEN_WIDTH/2,30,3.0,COL_TEXT,1.8);
	if(over!=0)Button("NEW GAME",BX,BTN_Y,BTN_W,BTN_H,COL_GO,mx,my);
	else if(Time()-newConfirmT<3)Button("SURE?",BX,BTN_Y,BTN_W,BTN_H,0xffa83a50,mx,my);
	else Button("NEW GAME",BX,BTN_Y,BTN_W,BTN_H,COL_BTN,mx,my);
	Button("STATISTICS",RX+RW-BTN_W,BTN_Y,BTN_W,BTN_H,COL_BTN,mx,my);
}

void Dim()
{
	FillBox(0,0,G_SCREEN_WIDTH,G_SCREEN_HEIGHT,0,0xa0050816);
}

void DrawGameOver(float T, int mx, int my)
{
	float a=Clamp01((T-overT-0.5)/0.3);
	if(a<=0)return;
	Dim();
	FillBox(OV_X,OV_Y+(1-a)*20,OV_W,OV_H,14,0xff161d48);
	if(a<1)return;
	TextC("THE SKY IS FULL",OV_X+OV_W/2,OV_Y+34,2.2,COL_GOLD,1.5);
	char s[40];
	snprintf(s,39,"%i",score);
	TextC(s,OV_X+OV_W/2,OV_Y+84,3.6,COL_TEXT,2.);
	if(score>=best&&score>0)TextC("a new best score!",OV_X+OV_W/2,OV_Y+120,1.4,COL_GOLD,1.2);
	else
	{
		snprintf(s,39,"best %i",best);
		TextC(s,OV_X+OV_W/2,OV_Y+120,1.4,COL_DIM,1.2);
	}
	Button("LOOK",OV_X+24,OV_Y+OV_H-56,120,34,COL_BTN,mx,my);
	Button("NEW GAME",OV_X+OV_W-144,OV_Y+OV_H-56,120,34,COL_GO,mx,my);
}

#define ST_X 140
#define ST_Y 140
#define ST_W 500
#define ST_H 340

void DrawStats(int mx, int my)
{
	Dim();
	FillBox(ST_X,ST_Y,ST_W,ST_H,14,0xff161d48);
	TextC("STATISTICS",ST_X+ST_W/2,ST_Y+34,2.4,COL_TEXT,1.6);
	char s[40];
	int vals[3];
	vals[0]=games;
	vals[1]=best;
	vals[2]=totalFusions;
	char* labels[3];
	labels[0]="Games";
	labels[1]="Best score";
	labels[2]="Fusions";
	for(int i=0;i<3;i++)
	{
		float cx=ST_X+ST_W*(i+0.5)/3;
		snprintf(s,39,"%i",vals[i]);
		TextC(s,cx,ST_Y+92,3.0,COL_TEXT,1.8);
		TextC(labels[i],cx,ST_Y+126,1.4,COL_DIM,1.2);
	}
	TextC("FURTHEST YOU HAVE GOT",ST_X+ST_W/2,ST_Y+172,1.6,COL_TEXT,1.3);
	if(bestTier>0)
	{
		DrawPiece(bestTier,ST_X+ST_W/2-70,ST_Y+226,70,0);
		Text(PieceName(bestTier),ST_X+ST_W/2-20,ST_Y+232,1.8,COL_GOLD,1.4);
	}
	else TextC("nothing yet",ST_X+ST_W/2,ST_Y+226,1.4,COL_DIM,1.2);
	Button("BACK",ST_X+ST_W/2-60,ST_Y+ST_H-52,120,34,COL_BTN,mx,my);
}

// ---------------------------------------------------------------- input

void NewGameButton()
{
	if(over!=0||moves==0)
	{
		NewGame();
		return;
	}
	if(Time()-newConfirmT<3)
	{
		newConfirmT=-10;
		NewGame();
		return;
	}
	newConfirmT=Time();
	Message("Click again to start over");
}

int CellAt(int mx, int my)
{
	if(mx<BX||my<BY||mx>=BX+N*CELL||my>=BY+N*CELL)return -1;
	return ((my-BY)/CELL)*N+(mx-BX)/CELL;
}

void OnClick(int mx, int my)
{
	if(statsOpen!=0)
	{
		statsOpen=0;   // BACK or anywhere
		return;
	}
	if(over!=0&&overClosed==0&&Time()-overT>0.8)
	{
		if(Over(OV_X+24,OV_Y+OV_H-56,120,34,mx,my))overClosed=1;
		if(Over(OV_X+OV_W-144,OV_Y+OV_H-56,120,34,mx,my))NewGame();
		return;
	}
	if(Over(BX,BTN_Y,BTN_W,BTN_H,mx,my))
	{
		NewGameButton();
		return;
	}
	if(Over(RX+RW-BTN_W,BTN_Y,BTN_W,BTN_H,mx,my))
	{
		statsOpen=1;
		return;
	}
	if(Over(RX,HOLD_Y,RW,HOLD_H,mx,my))
	{
		SwapHold();
		return;
	}
	int c=CellAt(mx,my);
	if(c>=0)Play(c);
}

void OnKey(int k)
{
	if(k==KEY_ESC)
	{
		if(statsOpen!=0)statsOpen=0;
		else if(over!=0)overClosed=1;
		return;
	}
	if(statsOpen!=0)return;
	if(k==' ')SwapHold();
	if((k==KEY_ENTER||k==KEY_ENTER2)&&over!=0)NewGame();
}

// ---------------------------------------------------------------- main

void Draw(int mx, int my)
{
	float T=Time();
	DrawSky(T);
	DrawHeader(mx,my);
	DrawBoard(T);
	DrawSide(T,mx,my);
	if(over==0)DrawHint();
	else TextC("The sky is full. NEW GAME starts another night.",BX+N*CELL/2,BY+N*CELL+32,1.4,COL_DIM,1.1);
	DrawMessage(T);
	if(over!=0&&overClosed==0)DrawGameOver(T,mx,my);
	if(statsOpen!=0)DrawStats(mx,my);
}

// Is something moving? Then every frame is drawn; otherwise the screen is
// only redrawn on input or when Look() changes.
int Animating(float T)
{
	if(phase!=PH_IDLE)return 1;
	if(T-msgT0<1.7)return 1;
	if(over!=0&&T-overT<0.9)return 1;
	if(hoverCell>=0&&T-hoverT0<0.7)return 1;
	for(int i=0;i<NC;i++)if(T-bornT[i]<0.3)return 1;
	for(int k=0;k<NPOP;k++)if(T-popT[k]<1.15)return 1;
	return quitReq!=0;   // ws.h draws its quit prompt in Present
}

// slow changes: the NEW GAME button turning back from SURE?
int Look(float T)
{
	if(T-newConfirmT<3)return 1;
	return 0;
}

int main(int argc, pchar* argv)
{
	for(int i=1;i<argc;i++)
	{
		char* a=argv[i];
		if(a[0]=='-'&&a[1]=='x'&&a[2]=='2'&&a[3]==0)SC=2;
	}
	if(SC!=1)
	{
		// ws.h made a window magnified 2x; replace it with one of the same
		// screen size that has real pixels
		wsys.DeleteWindow(mainWin);
		mainWin=wsys.CreateWindow(G_SCREEN_WIDTH*SC,G_SCREEN_HEIGHT*SC,1,1,G_SCREEN_MODE);
		g.SetActiveRT(wsys.GetWindowRT(mainWin));
	}
	g.t_0(0,0);
	g.t_x(SC,0);
	g.t_y(0,SC);

	pfont=(CFont*)malloc(sizeof(CFont));
	CFontMaker fm;
	fm.Init(*pfont,6);
	MakeLetters(fm);

	dx4[0]=0;dy4[0]=-1;
	dx4[1]=1;dy4[1]=0;
	dx4[2]=0;dy4[2]=1;
	dx4[3]=-1;dy4[3]=0;

	seed+=int(Time()*1e6);
	LoadSprites();
	LoadTotals();
	MakeStars();
	NewGame();

	int prevMB=0;
	int prevMX=-1;
	int prevMY=-1;
	int prevLook=-1;
	int wasAnim=0;
	int dirty=1;
	float lastPresent=0;
	while(true)
	{
		float T=Time();
		float pt=T-phaseT0;
		int ph=phase;
		if(phase==PH_FUSE&&pt>T_FUSE)FuseDone();
		else if(phase==PH_FLY&&pt>T_FLY)FlyDone();
		else if(phase==PH_BURN&&pt>T_BURN)BurnDone();
		else if(phase==PH_SWALLOW&&pt>T_SWALLOW)SwallowDone();
		if(phase!=ph)dirty=1;

		int key;
		int press;
		while(GetKeyEvent(key,press))
		{
			if(press>0)OnKey(key);
			dirty=1;
		}

		int mx;
		int my;
		int mb;
		GetMouseState(mx,my,mb);
		mx=mx/SC;
		my=my/SC;
		int hc=-1;
		if(statsOpen==0&&(over==0||overClosed!=0))hc=CellAt(mx,my);
		if(hc!=hoverCell)hoverT0=T;
		hoverCell=hc;
		UpdatePreview();
		if((mb&1)&&!(prevMB&1))OnClick(mx,my);
		if(mx!=prevMX||my!=prevMY||mb!=prevMB)dirty=1;
		prevMB=mb;
		prevMX=mx;
		prevMY=my;

		int look=Look(T);
		if(look!=prevLook)dirty=1;
		prevLook=look;

		// one more frame after an animation ends, to draw its final state
		int anim=Animating(T);
		if(wasAnim&&!anim)dirty=1;
		wasAnim=anim;

		if(dirty||anim)
		{
			Draw(mx,my);
			Present();
			lastPresent=T;
			dirty=0;
			Wait(0.01);   // ~60 fps is plenty, leave the rest of the CPU alone
		}
		else
		{
			// nothing changed: keep the events flowing (Present would drain
			// the queue we don't read) and show the frame again now and
			// then in case the window was covered
			Poll();
			SScincEvent ev;
			while(GetScincEvent(ev)){}
			if(T-lastPresent>0.5)
			{
				wsys.Present(mainWin);
				lastPresent=T;
			}
			Wait(0.03);
		}
	}
	return 0;
}
