// Circuitfall: a falling-block game for tv.h. Every piece carries a piece of wire.
// Rows never clear. A wire that closes into a loop, with no loose end left, flashes
// and burns away, and the blocks above it drop down, which may close more loops.
// Every fourth burn earns a fuse, which burns all wiring next to where it lands.
//
// Two modes: Loops has pieces of one or two blocks and burns closed loops only.
// Wires has pieces of up to three blocks and also burns any connected wire of
// CF_LONG blocks or more, loose ends and all.
//
// The program includes this after tv.h, routes W_CIRCUIT windows to CfDraw, CfKey,
// CfMouse and CfClosed, calls CfTick from TvAppTick and CfOpen to start a game.
#ifndef CIRCUITFALL_H
#define CIRCUITFALL_H

#include "tv.h"

#define W_CIRCUIT 40

#define CF_W 10
#define CF_H 18
#define CF_N 180
#define CF_WINW 50
#define CF_WINH 20

// wire directions
#define CF_N_ 1
#define CF_E_ 2
#define CF_S_ 4
#define CF_W_ 8

#define CF_PLAY 0
#define CF_FLASH 1
#define CF_OVER 2

#define CF_LOOPS 0
#define CF_WIRES 1
#define CF_LONG 10

#define CF_FLASHMS 450
#define CF_SAVE "circuitfall.sav"

int cfOcc[CF_N];
int cfMask[CF_N];
int cfCol[CF_N];      // background colour 1..6, 0 for a fuse
int cfMark[CF_N];
int cfQueue[CF_N];

int cfHeavy[16];      // the wire glyphs of placed blocks
int cfLight[16];      // and of the landing shadow

// the falling piece and the next one: up to 3 cells, x,y relative to px,py
int cfPN=0;
int cfPX[3];
int cfPY[3];
int cfPM[3];
int cfPCol=1;
int cfPFuse=0;
int cfPx=0;
int cfPy=0;
int cfNN=0;
int cfNX[3];
int cfNY[3];
int cfNM[3];
int cfNCol=1;
int cfNFuse=0;

int cfWin=-1;
int cfMode=CF_LOOPS;
int cfBests[2];
int cfState=CF_PLAY;
int cfScore=0;
int cfBest=0;
int cfLoops=0;
int cfLevel=1;
int cfFuses=0;        // loops counted towards the next fuse
int cfChain=0;
int cfPaused=0;       // paused with P
int cfHeld=0;         // paused because the window is behind or a menu is open
int cfNextDrop=0;
int cfFlashEnd=0;
int cfFlashPhase=0;
int cfFlashFuse=0;    // the flash is a fuse burning, not a loop
int cfNewBest=0;
int cfRng=1;
char cfLast[24];      // the last gain, shown under the score
int cfLastEnd=0;

int CfRand(int n)
{
	cfRng=cfRng*1103515245+12345;
	return ((cfRng>>16)&0x7fff)%n;
}

int CfRot(int m)
{
	return ((m<<1)|(m>>3))&15;
}

int CfOpp(int d)
{
	if(d==CF_N_)return CF_S_;
	if(d==CF_E_)return CF_W_;
	if(d==CF_S_)return CF_N_;
	return CF_E_;
}

int CfDX(int d)
{
	if(d==CF_E_)return 1;
	if(d==CF_W_)return -1;
	return 0;
}

int CfDY(int d)
{
	if(d==CF_S_)return 1;
	if(d==CF_N_)return -1;
	return 0;
}

// the direction from cell a to the neighbouring cell b of the piece being built
int CfDirTo(int ax, int ay, int bx, int by)
{
	if(bx>ax)return CF_E_;
	if(bx<ax)return CF_W_;
	if(by>ay)return CF_S_;
	return CF_N_;
}

int CfDropMs()
{
	int ms=900-70*(cfLevel-1);
	if(ms<110)ms=110;
	return ms;
}

// ---------------------------------------------------------------- saving the best score

// the file holds the best scores of both modes: "loops wires"
void CfLoadBest()
{
	cfBests[0]=0;
	cfBests[1]=0;
	char buf[32];
	int n=fs_load(CF_SAVE,&buf[0],31);
	int k=0;
	int inNum=0;
	for(int i=0;i<n;i++)
	{
		int c=buf[i];
		if((c>='0')&&(c<='9'))
		{
			if(k<2)cfBests[k]=cfBests[k]*10+c-'0';
			inNum=1;
		}
		else if(inNum)
		{
			k++;
			inNum=0;
		}
	}
}

void CfSaveBest()
{
	char buf[32];
	snprintf(&buf[0],32,"%i %i\n",cfBests[0],cfBests[1]);
	if(fs_save_begin(CF_SAVE))
	{
		int ok=fs_save_write(&buf[0],StrLen(&buf[0]));
		fs_save_end(ok);
	}
}

// ---------------------------------------------------------------- pieces

// A new piece in the next slot: a monomino, domino, straight or bent tromino with
// one wire running through all its cells and out of both ends.
void CfMakeNext()
{
	cfNFuse=0;
	cfNCol=1+CfRand(6);
	if(cfFuses>=4)
	{
		cfFuses-=4;
		cfNFuse=1;
		cfNN=1;
		cfNX[0]=0;
		cfNY[0]=0;
		cfNM[0]=0;
		cfNCol=0;
		return;
	}
	int r=CfRand(100);
	if(cfMode==CF_LOOPS)r=r*66/100; // single blocks and dominoes only
	if(r<36)
	{
		cfNN=1;
		cfNX[0]=0;
		cfNY[0]=0;
	}
	else if(r<66)
	{
		cfNN=2;
		cfNX[0]=0;
		cfNY[0]=0;
		cfNX[1]=1;
		cfNY[1]=0;
	}
	else if(r<81)
	{
		cfNN=3;
		cfNX[0]=-1;
		cfNY[0]=0;
		cfNX[1]=0;
		cfNY[1]=0;
		cfNX[2]=1;
		cfNY[2]=0;
	}
	else
	{
		cfNN=3;
		cfNX[0]=0;
		cfNY[0]=-1;
		cfNX[1]=0;
		cfNY[1]=0;
		cfNX[2]=1;
		cfNY[2]=0;
	}
	for(int i=0;i<cfNN;i++)
		cfNM[i]=0;
	for(int i=0;i<cfNN-1;i++)
	{
		int d=CfDirTo(cfNX[i],cfNY[i],cfNX[i+1],cfNY[i+1]);
		cfNM[i]|=d;
		cfNM[i+1]|=CfOpp(d);
	}
	if(cfNN==1)
	{
		// a bend more often than a straight, since loops need four of them
		int a=1<<CfRand(4);
		int b=CfOpp(a);
		if(CfRand(100)<70)
		{
			b=CfRot(a);
			if(CfRand(2)==0)b=CfRot(CfRot(b));
		}
		cfNM[0]=a|b;
	}
	else
	{
		// each end gets one way out, through any side the piece does not cover
		for(int e=0;e<2;e++)
		{
			int k=0;
			if(e==1)k=cfNN-1;
			int d=1<<CfRand(4);
			while((cfNM[k]&d)!=0)
				d=1<<CfRand(4);
			cfNM[k]|=d;
		}
	}
	// a random turn, so the shapes come in every orientation
	int turns=CfRand(4);
	for(int t=0;t<turns;t++)
		for(int i=0;i<cfNN;i++)
		{
			int x=cfNX[i];
			cfNX[i]=-cfNY[i];
			cfNY[i]=x;
			cfNM[i]=CfRot(cfNM[i]);
		}
}

int CfFits(int px, int py)
{
	for(int i=0;i<cfPN;i++)
	{
		int x=px+cfPX[i];
		int y=py+cfPY[i];
		if((x<0)||(x>=CF_W)||(y<0)||(y>=CF_H))return 0;
		if(cfOcc[y*CF_W+x])return 0;
	}
	return 1;
}

// Takes the next piece. Returns 0 if there is no room for it, which ends the game.
int CfSpawn()
{
	cfPN=cfNN;
	cfPCol=cfNCol;
	cfPFuse=cfNFuse;
	int top=0;
	for(int i=0;i<cfPN;i++)
	{
		cfPX[i]=cfNX[i];
		cfPY[i]=cfNY[i];
		cfPM[i]=cfNM[i];
		if(cfPY[i]<top)top=cfPY[i];
	}
	cfPx=CF_W/2-1;
	cfPy=-top;
	CfMakeNext();
	cfNextDrop=term_ms()+CfDropMs();
	return CfFits(cfPx,cfPy);
}

void CfRotate()
{
	int ox[3];
	int oy[3];
	int om[3];
	for(int i=0;i<cfPN;i++)
	{
		ox[i]=cfPX[i];
		oy[i]=cfPY[i];
		om[i]=cfPM[i];
		cfPX[i]=-oy[i];
		cfPY[i]=ox[i];
		cfPM[i]=CfRot(om[i]);
	}
	// against a wall or a block, try a step aside or up
	int kx[5];
	int ky[5];
	kx[0]=0;
	ky[0]=0;
	kx[1]=-1;
	ky[1]=0;
	kx[2]=1;
	ky[2]=0;
	kx[3]=0;
	ky[3]=1;
	kx[4]=0;
	ky[4]=-1;
	for(int k=0;k<5;k++)
		if(CfFits(cfPx+kx[k],cfPy+ky[k]))
		{
			cfPx+=kx[k];
			cfPy+=ky[k];
			return;
		}
	for(int i=0;i<cfPN;i++)
	{
		cfPX[i]=ox[i];
		cfPY[i]=oy[i];
		cfPM[i]=om[i];
	}
}

int CfLandY()
{
	int y=cfPy;
	while(CfFits(cfPx,y+1))
		y++;
	return y;
}

// ---------------------------------------------------------------- circuits

// whether the wire leaving cell i in direction d meets a wire coming the other way
int CfLinked(int i, int d)
{
	if((cfMask[i]&d)==0)return 0;
	int x=i%CF_W+CfDX(d);
	int y=i/CF_W+CfDY(d);
	if((x<0)||(x>=CF_W)||(y<0)||(y>=CF_H))return 0;
	int j=y*CF_W+x;
	if(!cfOcc[j])return 0;
	if((cfMask[j]&CfOpp(d))==0)return 0;
	return 1;
}

// Collects the wiring connected to cell i into cfQueue, marking it with tag.
// Returns the cell count; cfOpenEnds and cfLinks describe it.
int cfOpenEnds=0;
int cfLinks=0;

int CfNetwork(int i, int tag)
{
	int n=1;
	cfQueue[0]=i;
	cfMark[i]=tag;
	cfOpenEnds=0;
	cfLinks=0;
	int head=0;
	while(head<n)
	{
		int c=cfQueue[head];
		head++;
		for(int b=0;b<4;b++)
		{
			int d=1<<b;
			if((cfMask[c]&d)!=0)
			{
				if(CfLinked(c,d))
				{
					cfLinks++;
					int j=(c/CF_W+CfDY(d))*CF_W+c%CF_W+CfDX(d);
					if(cfMark[j]==0)
					{
						cfMark[j]=tag;
						cfQueue[n]=j;
						n++;
					}
				}
				else cfOpenEnds++;
			}
		}
	}
	cfLinks=cfLinks/2;
	return n;
}

// kind: 0 burnt by a fuse, 1 a loop, 2 a long wire
void CfGain(int pts, int cells, int kind)
{
	int mult=cfChain;
	if(mult<1)mult=1;
	cfScore+=pts*mult;
	if(kind>0)
	{
		char what[8];
		StrCpy(&what[0],"loop",8);
		if(kind==2)StrCpy(&what[0],"wire",8);
		if(mult>1)snprintf(&cfLast[0],24,"+%i %s x%i",pts*mult,&what[0],mult);
		else snprintf(&cfLast[0],24,"+%i %s",pts,&what[0]);
	}
	else snprintf(&cfLast[0],24,"+%i burnt",pts*mult);
	cfLastEnd=term_ms()+2500;
}

// Marks every closed loop, and in Wires mode every long wire, for burning.
// Returns the number of cells marked.
int CfFindLoops()
{
	for(int i=0;i<CF_N;i++)
		cfMark[i]=0;
	int total=0;
	int found=0;
	int pts=0;
	int kind=1;
	for(int i=0;i<CF_N;i++)
	{
		if(cfOcc[i])
			if(cfMark[i]==0)
				if(cfMask[i]!=0)
				{
					int n=CfNetwork(i,2);
					int closed=0;
					if(cfOpenEnds==0)
						if(cfLinks>=n)closed=1;
					int burn=closed;
					if(cfMode==CF_WIRES)
						if(n>=CF_LONG)burn=1;
					if(burn)
					{
						// keep its cells marked for burning
						for(int k=0;k<n;k++)
							cfMark[cfQueue[k]]=1;
						total+=n;
						found++;
						if(closed)pts+=10*n*n;
						else
						{
							pts+=5*n*n;
							kind=2;
						}
						cfLoops++;
						cfFuses++;
					}
				}
	}
	for(int i=0;i<CF_N;i++)
		if(cfMark[i]!=1)cfMark[i]=0;
	if(found>0)
	{
		cfChain++;
		CfGain(pts,total,kind);
		cfLevel=1+cfLoops/5;
	}
	return total;
}

// A fuse at cell f burns itself and every network touching it.
int CfFuseBurn(int f)
{
	for(int i=0;i<CF_N;i++)
		cfMark[i]=0;
	cfMark[f]=1;
	int fx=f%CF_W;
	int fy=f/CF_W;
	for(int b=0;b<4;b++)
	{
		int d=1<<b;
		int x=fx+CfDX(d);
		int y=fy+CfDY(d);
		if((x>=0)&&(x<CF_W)&&(y>=0)&&(y<CF_H))
		{
			int j=y*CF_W+x;
			if(cfOcc[j])
				if(cfMark[j]==0)
				{
					int n=CfNetwork(j,2);
					for(int k=0;k<n;k++)
						cfMark[cfQueue[k]]=1;
				}
		}
	}
	int total=0;
	for(int i=0;i<CF_N;i++)
	{
		if(cfMark[i]!=1)cfMark[i]=0;
		else total++;
	}
	cfChain=1;
	CfGain(15*total,total,0);
	return total;
}

void CfStartFlash(int fuse)
{
	cfState=CF_FLASH;
	cfFlashFuse=fuse;
	cfFlashEnd=term_ms()+CF_FLASHMS;
	cfFlashPhase=0;
}

// Removes the burnt cells and lets everything above them fall.
void CfCollapse()
{
	for(int i=0;i<CF_N;i++)
		if(cfMark[i])
		{
			cfOcc[i]=0;
			cfMark[i]=0;
		}
	for(int x=0;x<CF_W;x++)
	{
		int to=CF_H-1;
		for(int y=CF_H-1;y>=0;y--)
		{
			int i=y*CF_W+x;
			if(cfOcc[i])
			{
				if(y!=to)
				{
					int j=to*CF_W+x;
					cfOcc[j]=1;
					cfMask[j]=cfMask[i];
					cfCol[j]=cfCol[i];
					cfOcc[i]=0;
				}
				to--;
			}
		}
	}
}

void CfGameOver()
{
	cfState=CF_OVER;
	if(cfScore>cfBest)
	{
		cfBest=cfScore;
		cfBests[cfMode]=cfBest;
		cfNewBest=1;
		CfSaveBest();
	}
}

void CfNextPiece()
{
	cfChain=0;
	cfState=CF_PLAY;
	if(!CfSpawn())CfGameOver();
}

void CfLock()
{
	int fuseCell=-1;
	for(int i=0;i<cfPN;i++)
	{
		int c=(cfPy+cfPY[i])*CF_W+cfPx+cfPX[i];
		cfOcc[c]=1;
		cfMask[c]=cfPM[i];
		cfCol[c]=cfPCol;
		if(cfPFuse)fuseCell=c;
	}
	cfPN=0;
	if(fuseCell>=0)
	{
		CfFuseBurn(fuseCell);
		CfStartFlash(1);
		return;
	}
	if(CfFindLoops()>0)CfStartFlash(0);
	else CfNextPiece();
}

void CfNewGame()
{
	for(int i=0;i<CF_N;i++)
	{
		cfOcc[i]=0;
		cfMask[i]=0;
		cfCol[i]=0;
		cfMark[i]=0;
	}
	cfScore=0;
	cfLoops=0;
	cfLevel=1;
	cfFuses=0;
	cfChain=0;
	cfPaused=0;
	cfNewBest=0;
	cfLast[0]=0;
	cfState=CF_PLAY;
	cfBest=cfBests[cfMode];
	if(cfWin>=0)
	{
		if(cfMode==CF_LOOPS)StrCpy(&tvWin[cfWin].title[0],"Circuitfall: Loops",48);
		else StrCpy(&tvWin[cfWin].title[0],"Circuitfall: Wires",48);
	}
	CfMakeNext();
	CfNextPiece();
	tvDirty=1;
}

// ---------------------------------------------------------------- the window

void CfInitGlyphs()
{
	cfHeavy[0]=G_BOX;
	cfHeavy[1]=0x2579;
	cfHeavy[2]=0x257A;
	cfHeavy[3]=0x2517;
	cfHeavy[4]=0x257B;
	cfHeavy[5]=0x2503;
	cfHeavy[6]=0x250F;
	cfHeavy[7]=0x2523;
	cfHeavy[8]=0x2578;
	cfHeavy[9]=0x251B;
	cfHeavy[10]=0x2501;
	cfHeavy[11]=0x253B;
	cfHeavy[12]=0x2513;
	cfHeavy[13]=0x252B;
	cfHeavy[14]=0x2533;
	cfHeavy[15]=0x254B;
	cfLight[0]=G_DOT;
	cfLight[1]=0x2575;
	cfLight[2]=0x2576;
	cfLight[3]=0x2514;
	cfLight[4]=0x2577;
	cfLight[5]=0x2502;
	cfLight[6]=0x250C;
	cfLight[7]=0x251C;
	cfLight[8]=0x2574;
	cfLight[9]=0x2518;
	cfLight[10]=0x2500;
	cfLight[11]=0x2534;
	cfLight[12]=0x2510;
	cfLight[13]=0x2524;
	cfLight[14]=0x252C;
	cfLight[15]=0x253C;
}

// Opens the game in a mode, CF_LOOPS or CF_WIRES. If it is open in the other
// mode, a new game starts in this one.
void CfOpen(int mode)
{
	if(cfWin>=0)
	{
		TvToFront(cfWin);
		if(mode!=cfMode)
		{
			cfMode=mode;
			CfNewGame();
		}
		return;
	}
	cfMode=mode;
	cfRng=term_ms()+term_time(2)*7919+term_time(1)*104729;
	CfInitGlyphs();
	CfLoadBest();
	cfWin=TvNewCentered(W_CIRCUIT,"Circuitfall",CF_WINW,CF_WINH,WF_DIALOG);
	if(cfWin<0)return;
	CfNewGame();
	tvTickMs=30;
	tvNextTick=term_ms()+30;
}

void CfClosed(int win)
{
	cfWin=-1;
	tvTickMs=0;
}

// one block, three columns wide so that horizontal wires join up
void CfCell(int sx, int sy, int mask, int at, int ghost)
{
	int h=0x2501;
	int g=cfHeavy[mask];
	if(ghost)
	{
		h=0x2500;
		g=cfLight[mask];
	}
	int l=' ';
	int r=' ';
	if(mask&CF_W_)l=h;
	if(mask&CF_E_)r=h;
	TvPut(sx,sy,l,at);
	TvPut(sx+1,sy,g,at);
	TvPut(sx+2,sy,r,at);
}

void CfDrawPiece(int sx, int sy, int n, int* xs, int* ys, int* ms, int col, int fuse)
{
	for(int i=0;i<n;i++)
	{
		int x=sx+xs[i]*3;
		int y=sy+ys[i];
		if(fuse)
		{
			TvPut(x,y,' ',0x0C);
			TvPut(x+1,y,'*',0x0C);
			TvPut(x+2,y,' ',0x0C);
		}
		else CfCell(x,y,ms[i],(col<<4)|0x0F,0);
	}
}

void CfDraw(int win)
{
	TWin& t=tvWin[win];
	int x0=t.x+1;
	int y0=t.y+1;
	TvFill(x0,y0,CF_W*3,CF_H,' ',0x00);
	for(int y=0;y<CF_H;y++)
		TvPut(x0+CF_W*3,y0+y,G_VERT,0x78);
	for(int i=0;i<CF_N;i++)
	{
		int sx=x0+(i%CF_W)*3;
		int sy=y0+i/CF_W;
		if(!cfOcc[i])TvPut(sx+1,sy,G_DOT,0x08);
		else if((cfState==CF_FLASH)&&(cfMark[i]!=0))
		{
			int at=0xEC;
			if(cfFlashPhase%2==1)at=0x4E;
			if(cfMask[i]==0)TvFill(sx,sy,3,1,'*',at);
			else CfCell(sx,sy,cfMask[i],at,0);
		}
		else if(cfMask[i]==0)
		{
			TvPut(sx,sy,' ',0x0C);
			TvPut(sx+1,sy,'*',0x0C);
			TvPut(sx+2,sy,' ',0x0C);
		}
		else CfCell(sx,sy,cfMask[i],(cfCol[i]<<4)|0x0F,0);
	}
	if((cfState==CF_PLAY)&&(cfPN>0))
	{
		int ly=CfLandY();
		if(ly!=cfPy)
			for(int i=0;i<cfPN;i++)
				CfCell(x0+(cfPx+cfPX[i])*3,y0+ly+cfPY[i],cfPM[i],0x08,1);
		CfDrawPiece(x0+cfPx*3,y0+cfPy,cfPN,&cfPX[0],&cfPY[0],&cfPM[0],cfPCol,cfPFuse);
	}

	// the side panel
	int px=x0+CF_W*3+2;
	char s[32];
	TvText(px,y0,"SCORE",0x70);
	snprintf(&s[0],32,"%i",cfScore);
	TvText(px,y0+1,&s[0],0x71);
	if(cfLast[0]!=0)
		if(term_ms()<cfLastEnd)TvText(px,y0+2,&cfLast[0],0x74);
	TvText(px,y0+3,"BEST",0x70);
	snprintf(&s[0],32,"%i",cfBest);
	TvText(px,y0+4,&s[0],0x71);
	snprintf(&s[0],32,"Level %i",cfLevel);
	TvText(px,y0+6,&s[0],0x70);
	if(cfMode==CF_LOOPS)snprintf(&s[0],32,"Loops %i",cfLoops);
	else snprintf(&s[0],32,"Burnt %i",cfLoops);
	TvText(px,y0+7,&s[0],0x70);
	TvText(px,y0+9,"NEXT",0x70);
	TvFill(px,y0+10,13,3,' ',0x00);
	int top=0;
	for(int i=0;i<cfNN;i++)
		if(cfNY[i]<top)top=cfNY[i];
	int minx=0;
	for(int i=0;i<cfNN;i++)
		if(cfNX[i]<minx)minx=cfNX[i];
	CfDrawPiece(px+2-minx*3,y0+10-top,cfNN,&cfNX[0],&cfNY[0],&cfNM[0],cfNCol,cfNFuse);
	if(cfNFuse)TvText(px+6,y0+11,"fuse!",0x0C);
	TvText(px,y0+14,"←→ move  ↑ turn",0x78);
	TvText(px,y0+15,"↓ step  Spc drop",0x78);
	if(cfMode==CF_LOOPS)TvText(px,y0+13,"Goal: any loop",0x70);
	else TvText(px,y0+13,"Goal: 10+ wire",0x70);
	TvText(px,y0+16,"P pause  N new",0x78);
	TvText(px,y0+17,"M other mode",0x78);

	// messages over the board
	int mx=x0+5;
	int my=y0+7;
	if(cfState==CF_OVER)
	{
		TvFill(mx-1,my-1,22,5,' ',0x4F);
		TvText(mx+5,my,"GAME OVER",0x4E);
		if(cfNewBest)TvText(mx+4,my+1,"a new best!",0x4F);
		TvText(mx+1,my+2,"N or Enter: again",0x4F);
	}
	else if(cfPaused||cfHeld)
	{
		TvFill(mx-1,my-1,22,4,' ',0x3F);
		TvText(mx+6,my,"PAUSED",0x3E);
		if(cfPaused)TvText(mx+5,my+1,"P to play on",0x3F);
		else if(menuOpen<0)TvText(mx+3,my+1,"click to play on",0x3F);
	}
}

int CfKey(int win, int key)
{
	if((key=='n')||(key=='N'))
	{
		CfNewGame();
		return 1;
	}
	if((key=='m')||(key=='M'))
	{
		cfMode=1-cfMode;
		CfNewGame();
		return 1;
	}
	if(cfState==CF_OVER)
	{
		if(key==K_ENTER)
		{
			CfNewGame();
			return 1;
		}
		return 0;
	}
	if((key=='p')||(key=='P'))
	{
		cfPaused=!cfPaused;
		cfNextDrop=term_ms()+CfDropMs();
		return 1;
	}
	if(cfPaused)return 0;
	if((cfState!=CF_PLAY)||(cfPN==0))
	{
		if((key==K_LEFT)||(key==K_RIGHT)||(key==K_UP)||(key==K_DOWN)||(key==' '))return 1;
		return 0;
	}
	if(key==K_LEFT)
	{
		if(CfFits(cfPx-1,cfPy))cfPx--;
		return 1;
	}
	if(key==K_RIGHT)
	{
		if(CfFits(cfPx+1,cfPy))cfPx++;
		return 1;
	}
	if((key==K_UP)||(key=='x')||(key=='X'))
	{
		CfRotate();
		return 1;
	}
	if((key=='z')||(key=='Z'))
	{
		CfRotate();
		CfRotate();
		CfRotate();
		return 1;
	}
	if(key==K_DOWN)
	{
		if(CfFits(cfPx,cfPy+1))
		{
			cfPy++;
			cfScore++;
			cfNextDrop=term_ms()+CfDropMs();
		}
		else CfLock();
		return 1;
	}
	if(key==' ')
	{
		int ly=CfLandY();
		cfScore+=2*(ly-cfPy);
		cfPy=ly;
		CfLock();
		return 1;
	}
	return 0;
}

int CfMouse(int win, int ev, int x, int y)
{
	if(ev==EV_MDOWN)
		if(cfState==CF_OVER)
			CfNewGame();
	return 1;
}

void CfTick()
{
	if(cfWin<0)return;
	int now=term_ms();
	int held=(TvTop()!=cfWin)||(menuOpen>=0);
	if(held!=cfHeld)
	{
		cfHeld=held;
		cfNextDrop=now+CfDropMs();
		tvDirty=1;
	}
	if(cfLast[0]!=0)
		if(now>=cfLastEnd)
		{
			cfLast[0]=0;
			tvDirty=1;
		}
	if(cfHeld||cfPaused)return;
	if(cfState==CF_FLASH)
	{
		int phase=(CF_FLASHMS-(cfFlashEnd-now))/75;
		if(phase!=cfFlashPhase)
		{
			cfFlashPhase=phase;
			tvDirty=1;
		}
		if(now>=cfFlashEnd)
		{
			CfCollapse();
			tvDirty=1;
			// falling blocks may close more loops, which score a higher chain
			if(CfFindLoops()>0)CfStartFlash(0);
			else CfNextPiece();
		}
		return;
	}
	if(cfState!=CF_PLAY)return;
	if(now>=cfNextDrop)
	{
		if(CfFits(cfPx,cfPy+1))
		{
			cfPy++;
			cfNextDrop=now+CfDropMs();
		}
		else CfLock();
		tvDirty=1;
	}
}

#endif // CIRCUITFALL_H
