// sudoku.cpp - Sudoku with generated puzzles in five levels of difficulty.
//
// Fill the grid so that every row, every column and every 3x3 box holds
// the digits 1 to 9 once. Click a cell to select it (or move with the
// arrow keys), then type 1-9, click a digit on the panel below the board,
// or right-click the cell (or click it again) for a menu of digits right
// beside it. 0, Delete or Backspace clear a cell, U or Z undoes.
// A digit that clashes with another one in its row, column or box is red.
//
// Pencil marks: with NOTES on (or the N key) the digits mark and unmark
// small notes in the cell instead of filling it, and the popup stays open
// to mark several; Shift+digit makes a note in either mode. Placing a
// digit removes it from the notes of every cell it sees, and the notes of
// the selected digit are highlighted. CLEAR on an empty cell wipes its notes.
// AUTO NOTES (or the A key) marks in every empty cell the digits that its
// row, column and box still allow; a cell that has notes already only loses
// the ones those rules forbid, so notes worked out by hand are kept. UNDO
// takes it back in one step.
//
// HELP (bottom right) greys out and disables the digits that are already
// in the selected cell's row, column or box, so only the digits the rules
// still allow can be entered. The puzzle is the same either way.
//
// Every puzzle is new, made at the start of a game (see "generator"):
//   1 What is it, a new game?
//   2 I know how to play it
//   3 Sometimes it can be tricky
//   4 Easy to learn, hard to master
//   5 Is it even possible?
// The game in progress (notes included), HELP, NOTES and the statistics are kept in
// sudoku.sav, so closing the window and coming back continues the game.
//
// Normally the window is unscaled, one pixel per unit. Run with -x2
// (Scinc sudoku.cpp -x2) for a window twice as big, e.g. on HiDPI screens: it is
// rendered at twice the resolution, so everything, strokes included, comes
// out sharp rather than magnified. The x2 button switches between the two any time.

#include <stdio.h>

#define G_SCREEN_WIDTH 600
#define G_SCREEN_HEIGHT 774
#define G_SCREEN_SCALE 1  // unscaled; -x2 makes a window twice as big
#define G_SCREEN_MODE 1   // XShm: faster presents than mode 0 (plain XPutImage)

#include "graphics.h"
#include "../../ws.h"
#include "../../graphic/font.h"
#include "../../graphic/letters.h"
#include "../../graphic/gtext.h"

Graph g;
CFont* pfont=0;

typedef char* pchar;

int irand(int& seed)
{
	seed=(seed*1103515245+12345)&0x7ffffff;
	return seed;
}

int seed=6917832;

int Rnd(int n)
{
	return (irand(seed)>>8)%n;
}

// ---------------------------------------------------------------- tables
//
// A digit d (1..9) is bit d-1 of a candidate mask. The 27 units are the 9
// rows, 9 columns and 9 boxes; every cell belongs to three of them and
// sees 20 peers.

int popc[512];
int bitDigit[512];        // digit of a one-bit mask
int unitCell[27*9];
int cellUnit[81*3];       // row, column and box unit of a cell
int peer[81*20];
int boxOf[81];

void InitTables()
{
	for(int m=0;m<512;m++)
	{
		int n=0;
		int d=0;
		for(int b=0;b<9;b++)
			if(m&(1<<b))
			{
				n++;
				d=b+1;
			}
		popc[m]=n;
		bitDigit[m]=d;
	}
	for(int i=0;i<81;i++)
	{
		int r=i/9;
		int c=i%9;
		int b=(r/3)*3+c/3;
		boxOf[i]=b;
		cellUnit[i*3]=r;
		cellUnit[i*3+1]=9+c;
		cellUnit[i*3+2]=18+b;
	}
	for(int u=0;u<27;u++)
	{
		int k=0;
		for(int i=0;i<81;i++)
			if(cellUnit[i*3]==u||cellUnit[i*3+1]==u||cellUnit[i*3+2]==u)
			{
				unitCell[u*9+k]=i;
				k++;
			}
	}
	for(int i=0;i<81;i++)
	{
		int k=0;
		for(int j=0;j<81;j++)
		{
			if(j==i)continue;
			if(j/9==i/9||j%9==i%9||boxOf[j]==boxOf[i])
			{
				peer[i*20+k]=j;
				k++;
			}
		}
	}
}

int Sees(int a, int b)
{
	return a!=b&&(a/9==b/9||a%9==b%9||boxOf[a]==boxOf[b]);
}

// ---------------------------------------------------------------- backtracking
//
// Used to fill a random solved grid and to count the solutions of a puzzle
// (stopping at 2: all we need to know is whether it is unique).

int sg[81];
int rowM[9];
int colM[9];
int boxM[9];
int solCount=0;
int solLimit=1;
int randomFill=0;
int nodes=0;

void BtSetup(int* grid)
{
	for(int k=0;k<9;k++)
	{
		rowM[k]=0;
		colM[k]=0;
		boxM[k]=0;
	}
	for(int i=0;i<81;i++)
	{
		sg[i]=grid[i];
		if(grid[i]!=0)
		{
			int b=1<<(grid[i]-1);
			rowM[i/9]|=b;
			colM[i%9]|=b;
			boxM[boxOf[i]]|=b;
		}
	}
}

void BtRec()
{
	nodes++;
	int best=-1;
	int bestN=10;
	int bestMask=0;
	for(int i=0;i<81;i++)
	{
		if(sg[i]!=0)continue;
		int m=511&~(rowM[i/9]|colM[i%9]|boxM[boxOf[i]]);
		int n=popc[m];
		if(n<bestN)
		{
			best=i;
			bestN=n;
			bestMask=m;
			if(n<=1)break;
		}
	}
	if(best<0)
	{
		solCount++;
		return;
	}
	if(bestN==0)return;
	int r=best/9;
	int c=best%9;
	int bx=boxOf[best];
	int start=0;
	if(randomFill)start=Rnd(9);
	for(int k=0;k<9;k++)
	{
		int d=(start+k)%9;
		int b=1<<d;
		if(!(bestMask&b))continue;
		sg[best]=d+1;
		rowM[r]|=b;
		colM[c]|=b;
		boxM[bx]|=b;
		BtRec();
		if(solCount>=solLimit)return;   // keep sg[] as the found solution
		sg[best]=0;
		rowM[r]&=~b;
		colM[c]&=~b;
		boxM[bx]&=~b;
	}
}

// fill sol[] with a random solved grid
void RandomSolution(int* sol)
{
	int empty[81];
	for(int i=0;i<81;i++)empty[i]=0;
	BtSetup(empty);
	// a random first row keeps the search short and the grids varied
	for(int c=0;c<9;c++)sg[c]=c+1;
	for(int c=8;c>0;c--)
	{
		int j=Rnd(c+1);
		int t=sg[c];
		sg[c]=sg[j];
		sg[j]=t;
	}
	for(int c=0;c<9;c++)
	{
		int b=1<<(sg[c]-1);
		rowM[0]|=b;
		colM[c]|=b;
		boxM[c/3]|=b;
	}
	solCount=0;
	solLimit=1;
	randomFill=1;
	BtRec();
	randomFill=0;
	for(int i=0;i<81;i++)sol[i]=sg[i];
}

// number of solutions, 0, 1 or 2 (meaning "more than one")
int CountSolutions(int* grid)
{
	BtSetup(grid);
	solCount=0;
	solLimit=2;
	BtRec();
	return solCount;
}

// ---------------------------------------------------------------- logic solver
//
// Solves like a person does, with candidates, and reports the hardest
// technique it needed. The difficulty levels are defined by these:
//   T_BOX     box scanning: a digit that fits only one cell of its box, or
//             the last empty cell of a row, column or box
//   T_SINGLE  naked and hidden singles
//   T_LOCKED  locked candidates (pointing and claiming)
//   T_PAIR    naked and hidden pairs
//   T_TRIPLE  naked and hidden triples and quads
//   T_FISH    X-wing and swordfish
//   T_XYWING  XY-wing
// Every step is a sound deduction, so a puzzle the solver finishes has
// exactly one solution.

#define T_BOX 1
#define T_SINGLE 2
#define T_LOCKED 3
#define T_PAIR 4
#define T_TRIPLE 5
#define T_FISH 6
#define T_XYWING 7

int lv[81];       // values
int cand[81];     // candidate masks of empty cells
int nEmpty=0;

void LSetup(int* grid)
{
	nEmpty=0;
	for(int i=0;i<81;i++)
	{
		lv[i]=grid[i];
		cand[i]=0;
		if(grid[i]==0)
		{
			cand[i]=511;
			nEmpty++;
		}
	}
	for(int i=0;i<81;i++)
	{
		if(lv[i]==0)continue;
		int nb=~(1<<(lv[i]-1));
		for(int k=0;k<20;k++)cand[peer[i*20+k]]&=nb;
	}
}

void LPlace(int i, int d)
{
	lv[i]=d;
	cand[i]=0;
	nEmpty--;
	int nb=~(1<<(d-1));
	for(int k=0;k<20;k++)cand[peer[i*20+k]]&=nb;
}

int Singles()
{
	for(int i=0;i<81;i++)
		if(lv[i]==0&&popc[cand[i]]==1)
		{
			LPlace(i,bitDigit[cand[i]]);
			return 1;
		}
	for(int u=0;u<27;u++)
	{
		for(int d=0;d<9;d++)
		{
			int b=1<<d;
			int n=0;
			int at=-1;
			for(int k=0;k<9;k++)
			{
				int c=unitCell[u*9+k];
				if(lv[c]==d+1)
				{
					n=9;
					break;
				}
				if(cand[c]&b)
				{
					n++;
					at=c;
				}
			}
			if(n==1)
			{
				LPlace(at,d+1);
				return 1;
			}
		}
	}
	return 0;
}

// the easiest steps there are: the last empty cell of a unit, and a digit
// with only one place left in its box
int BoxSingles()
{
	for(int u=0;u<27;u++)
	{
		int n=0;
		int at=-1;
		for(int k=0;k<9;k++)
		{
			int c=unitCell[u*9+k];
			if(lv[c]==0)
			{
				n++;
				at=c;
			}
		}
		if(n==1&&popc[cand[at]]==1)
		{
			LPlace(at,bitDigit[cand[at]]);
			return 1;
		}
	}
	for(int u=18;u<27;u++)
	{
		for(int d=0;d<9;d++)
		{
			int b=1<<d;
			int n=0;
			int at=-1;
			for(int k=0;k<9;k++)
			{
				int c=unitCell[u*9+k];
				if(lv[c]==d+1)
				{
					n=9;
					break;
				}
				if(cand[c]&b)
				{
					n++;
					at=c;
				}
			}
			if(n==1)
			{
				LPlace(at,d+1);
				return 1;
			}
		}
	}
	return 0;
}

// remove mask m from cell c; 1 if something went
int Elim(int c, int m)
{
	if(cand[c]&m)
	{
		cand[c]&=~m;
		return 1;
	}
	return 0;
}

// a digit confined to one line inside a box leaves the rest of that line,
// and confined to one box inside a line it leaves the rest of the box
int Locked()
{
	for(int u=0;u<27;u++)
	{
		for(int d=0;d<9;d++)
		{
			int b=1<<d;
			int first=-1;
			int sameRow=1;
			int sameCol=1;
			int sameBox=1;
			int n=0;
			for(int k=0;k<9;k++)
			{
				int c=unitCell[u*9+k];
				if(!(cand[c]&b))continue;
				n++;
				if(first<0)first=c;
				else
				{
					if(c/9!=first/9)sameRow=0;
					if(c%9!=first%9)sameCol=0;
					if(boxOf[c]!=boxOf[first])sameBox=0;
				}
			}
			if(n<2)continue;
			int other=-1;
			if(u>=18)
			{
				if(sameRow)other=first/9;
				else if(sameCol)other=9+first%9;
			}
			else if(sameBox)other=18+boxOf[first];
			if(other<0)continue;
			int done=0;
			for(int k=0;k<9;k++)
			{
				int c=unitCell[other*9+k];
				if(cellUnit[c*3]==u||cellUnit[c*3+1]==u||cellUnit[c*3+2]==u)continue;
				done+=Elim(c,b);
			}
			if(done>0)return 1;
		}
	}
	return 0;
}

// naked and hidden subsets of size n in one unit
int ucell[9];
int ucnt=0;

int SubsetsIn(int u, int n)
{
	// naked: n cells whose candidates together are n digits
	ucnt=0;
	for(int k=0;k<9;k++)
	{
		int c=unitCell[u*9+k];
		if(lv[c]==0)
		{
			ucell[ucnt]=c;
			ucnt++;
		}
	}
	if(ucnt<=n)return 0;
	for(int s=1;s<(1<<ucnt);s++)
	{
		if(popc[s]!=n)continue;
		int m=0;
		for(int k=0;k<ucnt;k++)if(s&(1<<k))m|=cand[ucell[k]];
		if(popc[m]!=n)continue;
		int done=0;
		for(int k=0;k<ucnt;k++)
			if(!(s&(1<<k)))done+=Elim(ucell[k],m);
		if(done>0)return 1;
	}
	// hidden: n digits that fit only in the same n cells
	int place[9];     // cells (as bits of ucell) where each digit fits
	int freeD=0;
	for(int d=0;d<9;d++)
	{
		place[d]=0;
		for(int k=0;k<ucnt;k++)if(cand[ucell[k]]&(1<<d))place[d]|=1<<k;
		if(place[d]!=0)freeD|=1<<d;
	}
	for(int s=1;s<512;s++)
	{
		if(popc[s]!=n||(s&freeD)!=s)continue;
		int cells=0;
		for(int d=0;d<9;d++)if(s&(1<<d))cells|=place[d];
		if(popc[cells]!=n)continue;
		int done=0;
		for(int k=0;k<ucnt;k++)
			if(cells&(1<<k))done+=Elim(ucell[k],511&~s);
		if(done>0)return 1;
	}
	return 0;
}

int Subsets(int n)
{
	for(int u=0;u<27;u++)if(SubsetsIn(u,n))return 1;
	return 0;
}

// X-wing (n=2) and swordfish (n=3): a digit whose places in n rows lie in
// the same n columns leaves those columns elsewhere; same with rows and
// columns swapped
int Fish(int n)
{
	for(int d=0;d<9;d++)
	{
		int b=1<<d;
		for(int dir=0;dir<2;dir++)
		{
			int pos[9];
			int lines=0;
			for(int l=0;l<9;l++)
			{
				pos[l]=0;
				for(int k=0;k<9;k++)
				{
					int c=l*9+k;
					if(dir==1)c=k*9+l;
					if(cand[c]&b)pos[l]|=1<<k;
				}
				if(popc[pos[l]]>=2&&popc[pos[l]]<=n)lines|=1<<l;
			}
			for(int s=1;s<512;s++)
			{
				if(popc[s]!=n||(s&lines)!=s)continue;
				int m=0;
				for(int l=0;l<9;l++)if(s&(1<<l))m|=pos[l];
				if(popc[m]!=n)continue;
				int done=0;
				for(int l=0;l<9;l++)
				{
					if(s&(1<<l))continue;
					for(int k=0;k<9;k++)
					{
						if(!(m&(1<<k)))continue;
						int c=l*9+k;
						if(dir==1)c=k*9+l;
						done+=Elim(c,b);
					}
				}
				if(done>0)return 1;
			}
		}
	}
	return 0;
}

// XY-wing: a pivot {a,b} sees pincers {a,c} and {b,c}; whatever the pivot
// is, one pincer is c, so c leaves every cell that sees both pincers
int XYWing()
{
	for(int p=0;p<81;p++)
	{
		if(popc[cand[p]]!=2)continue;
		for(int i=0;i<20;i++)
		{
			int x=peer[p*20+i];
			int mx=cand[x];
			if(popc[mx]!=2||popc[mx&cand[p]]!=1)continue;
			int c=mx&~cand[p];
			for(int j=i+1;j<20;j++)
			{
				int y=peer[p*20+j];
				int my=cand[y];
				if(popc[my]!=2||!(my&c))continue;
				if((my&~c)!=(cand[p]&~mx))continue;
				int done=0;
				for(int k=0;k<20;k++)
				{
					int z=peer[x*20+k];
					if(z!=p&&z!=y&&Sees(z,y))done+=Elim(z,c);
				}
				if(done>0)return 1;
			}
		}
	}
	return 0;
}

// solve grid with techniques up to maxTech; returns the hardest technique
// used, or 0 when it gets stuck
int LogicSolve(int* grid, int maxTech)
{
	LSetup(grid);
	int hardest=T_BOX;
	while(nEmpty>0)
	{
		int t=0;
		if(BoxSingles())t=T_BOX;
		else if(maxTech>=T_SINGLE&&Singles())t=T_SINGLE;
		else if(maxTech>=T_LOCKED&&Locked())t=T_LOCKED;
		else if(maxTech>=T_PAIR&&Subsets(2))t=T_PAIR;
		else if(maxTech>=T_TRIPLE&&(Subsets(3)||Subsets(4)))t=T_TRIPLE;
		else if(maxTech>=T_FISH&&(Fish(2)||Fish(3)))t=T_FISH;
		else if(maxTech>=T_XYWING&&XYWing())t=T_XYWING;
		if(t==0)return 0;
		if(t>hardest)hardest=t;
	}
	return hardest;
}

// ---------------------------------------------------------------- generator
//
// A random solved grid loses clues in symmetric pairs (the grid looks the
// same turned half a turn) as long as the puzzle stays within its level:
//   1 "What is it, a new game?"       box scanning only, stops at 46 clues
//   2 "I know how to play it"         singles only, stops at 36 clues
//   3 "Sometimes it can be tricky"    needs locked candidates or pairs
//   4 "Easy to learn, hard to master" needs triples, fish or an XY-wing
//   5 "Is it even possible?"          unique, but none of the above crack it
// When the result is not hard enough for its level a new grid is tried.

#define NLEVELS 5

int maxTechOf[NLEVELS];
int minTechOf[NLEVELS];
int minCluesOf[NLEVELS];

int solution[81];
int puzzle[81];
int genTries=0;

int Allowed(int* grid, int level)
{
	if(level==4)return CountSolutions(grid)==1;
	return LogicSolve(grid,maxTechOf[level])!=0;
}

int GenerateOnce(int level)
{
	RandomSolution(solution);
	for(int i=0;i<81;i++)puzzle[i]=solution[i];
	int order[41];
	for(int i=0;i<41;i++)order[i]=i;
	for(int i=40;i>0;i--)
	{
		int j=Rnd(i+1);
		int t=order[i];
		order[i]=order[j];
		order[j]=t;
	}
	int clues=81;
	for(int k=0;k<41;k++)
	{
		int a=order[k];
		int b=80-a;
		int n=2;
		if(a==b)n=1;
		if(clues-n<minCluesOf[level])continue;
		puzzle[a]=0;
		puzzle[b]=0;
		if(Allowed(puzzle,level))clues-=n;
		else
		{
			puzzle[a]=solution[a];
			puzzle[b]=solution[b];
		}
	}
	if(level==4)return LogicSolve(puzzle,T_XYWING)==0;
	return LogicSolve(puzzle,maxTechOf[level])>=minTechOf[level];
}

void Generate(int level)
{
	genTries=0;
	while(true)
	{
		genTries++;
		if(GenerateOnce(level))return;
	}
}

void InitLevels()
{
	maxTechOf[0]=T_BOX;
	minTechOf[0]=T_BOX;
	minCluesOf[0]=46;
	maxTechOf[1]=T_SINGLE;
	minTechOf[1]=T_SINGLE;
	minCluesOf[1]=36;
	maxTechOf[2]=T_PAIR;
	minTechOf[2]=T_LOCKED;
	minCluesOf[2]=0;
	maxTechOf[3]=T_XYWING;
	minTechOf[3]=T_TRIPLE;
	minCluesOf[3]=0;
	maxTechOf[4]=T_XYWING;
	minTechOf[4]=T_XYWING;
	minCluesOf[4]=0;
}

// ---------------------------------------------------------------- game state

#define COL_BG      0xff1a1e28
#define COL_CELL    0xff252b39
#define COL_PEER    0xff2d3446
#define COL_SAME    0xff34496e
#define COL_SEL     0xff44639a
#define COL_THIN    0xff3a4256
#define COL_THICK   0xff8d97ad
#define COL_GIVEN   0xffeef1f7
#define COL_USER    0xff8cc0ff
#define COL_BAD     0xffff6b6b
#define COL_BTN     0xff3a4256
#define COL_ACCENT  0xff4f8a5b
#define COL_TEXT    0xffffffff
#define COL_DIM     0xff8a93a6
#define COL_OFF     0xff2a303d
#define COL_NOTE    0xff8a6d2c

#define KEY_UP 4000
#define KEY_DOWN 4001
#define KEY_LEFT 4002
#define KEY_RIGHT 4003
#define KEY_BACK 4008
#define KEY_DEL 4011
#define KEY_ENTER 4013
#define KEY_ENTER2 5013
#define KEY_ESC 1000

// rendering scale: 1 normally, 2 with -x2 (see Wordy)
float SC=1;
int scaleReq=0;    // the x2 button was pressed

char* levelName[NLEVELS]={
	"What is it, a new game?",
	"I know how to play it",
	"Sometimes it can be tricky",
	"Easy to learn, hard to master",
	"Is it even possible?"};

#define ST_NONE 0      // no game yet (the level menu is open)
#define ST_PLAY 1
#define ST_SOLVED 2

int state=ST_NONE;
int level=0;
int given[81];     // 1 for the clues
int cell[81];      // what the board shows, clues included
int sel=-1;        // selected cell
int help=1;        // grey out digits that break the rules

float playTime=0;  // seconds played before resumeT
float resumeT=0;
float winT=-10;

#define NUNDO 1024
int undoCell[NUNDO];
int undoOld[NUNDO];
int undoNotes[NUNDO];
int undoMove[NUNDO];
int nUndo=0;
int moveNo=0;

int notes[81];     // pencil marks, a bit per digit
int noteMode=0;    // digits make notes instead of filling the cell

// overlays
int menuOpen=0;    // level menu
int menuLevel=0;   // level highlighted in it
int statsOpen=0;
int cardOpen=0;    // "Solved!" card
int popupOpen=0;
int popupCell=-1;
float popX=0;
float popY=0;

int genLevel=-1;   // a puzzle of this level is wanted
int genShown=0;    // the "making a puzzle" frame has been presented

int played[NLEVELS];
int solved[NLEVELS];
int best[NLEVELS];  // best time in seconds, 0 = none yet
int newBest=0;

char msg[64];
float msgT=-10;
int flashCell=-1;  // the cell that refused a digit
float flashT=-10;
float giveT=-10;   // NEW GAME asked for a second click

void Message(char* s)
{
	int i=0;
	while(s[i]!=0&&i<63)
	{
		msg[i]=s[i];
		i++;
	}
	msg[i]=0;
	msgT=Time();
}

float Elapsed()
{
	if(state==ST_PLAY)return playTime+Time()-resumeT;
	return playTime;
}

void TimeText(float t, char* s)
{
	int n=t;
	if(n>=3600)snprintf(s,15,"%i:%02i:%02i",n/3600,n/60%60,n%60);
	else snprintf(s,15,"%i:%02i",n/60,n%60);
}

// ---------------------------------------------------------------- saving

char* SAVE_FILE="sudoku.sav";

// reads up to max numbers from a line
int LineInts(char* s, int* v, int max)
{
	int n=0;
	int i=0;
	while(s[i]!=0&&n<max)
	{
		if(s[i]>='0'&&s[i]<='9')
		{
			int x=0;
			while(s[i]>='0'&&s[i]<='9')
			{
				x=x*10+s[i]-'0';
				i++;
			}
			v[n]=x;
			n++;
		}
		else i++;
	}
	return n;
}

void GridLine(FILE* f, int* a)
{
	char s[96];
	for(int i=0;i<81;i++)
	{
		s[i]='0'+a[i];
	}
	s[81]='\n';
	s[82]=0;
	fputs(s,f);
}

void ReadGrid(char* s, int* a)
{
	for(int i=0;i<81;i++)
	{
		int v=s[i]-'0';
		if(v<0||v>9)v=0;
		a[i]=v;
	}
}

void Save()
{
	FILE* f=fopen(SAVE_FILE,"wb");
	if(f==0)return;
	char s[96];
	snprintf(s,95,"%i %i help, notes\n",help,noteMode);
	fputs(s,f);
	snprintf(s,95,"%i last level\n",menuLevel+1);
	fputs(s,f);
	for(int l=0;l<NLEVELS;l++)
	{
		snprintf(s,95,"%i %i %i level %i: played, solved, best seconds\n",played[l],solved[l],best[l],l+1);
		fputs(s,f);
	}
	if(state==ST_PLAY)
	{
		snprintf(s,95,"%i %i game: level, seconds\n",level+1,int(Elapsed()));
		fputs(s,f);
		GridLine(f,cell);
		GridLine(f,given);
		GridLine(f,solution);
		for(int i=0;i<81;i++)
		{
			snprintf(s,95,"%i ",notes[i]);
			fputs(s,f);
		}
		fputs("notes\n",f);
	}
	fclose(f);
}

void Load()
{
	help=1;
	noteMode=0;
	menuLevel=0;
	for(int l=0;l<NLEVELS;l++)
	{
		played[l]=0;
		solved[l]=0;
		best[l]=0;
	}
	FILE* f=fopen(SAVE_FILE,"rb");
	if(f==0)return;
	char s[400];
	int v[81];
	int k=0;
	int gameOk=0;
	int gl=0;
	int gt=0;
	while(true)
	{
		s[0]=0;
		fgets(s,399,f);
		if(s[0]==0)break;
		int n=LineInts(s,v,4);
		if(k==0&&n>0)help=v[0]!=0;
		if(k==0&&n>1)noteMode=v[1]!=0;
		if(k==1&&n>0&&v[0]>=1&&v[0]<=NLEVELS)menuLevel=v[0]-1;
		if(k>=2&&k<2+NLEVELS&&n>=3)
		{
			played[k-2]=v[0];
			solved[k-2]=v[1];
			best[k-2]=v[2];
		}
		if(k==2+NLEVELS&&n>=2&&v[0]>=1&&v[0]<=NLEVELS)
		{
			gameOk=1;
			gl=v[0]-1;
			gt=v[1];
		}
		if(k==3+NLEVELS)ReadGrid(s,cell);
		if(k==4+NLEVELS)ReadGrid(s,given);
		if(k==5+NLEVELS)ReadGrid(s,solution);
		if(k==6+NLEVELS&&LineInts(s,v,81)==81)
			for(int i=0;i<81;i++)notes[i]=v[i]&511;
		k++;
	}
	fclose(f);
	if(gameOk&&k>=6+NLEVELS)
	{
		level=gl;
		playTime=gt;
		resumeT=Time();
		state=ST_PLAY;
		nUndo=0;
		for(int i=0;i<81;i++)
			if(given[i])cell[i]=solution[i];
	}
}

// ---------------------------------------------------------------- rules

// is digit d already in the row, column or box of cell i? 0 no, 1 row,
// 2 column, 3 box; at tells the cell
int Clash(int i, int d, int& at)
{
	at=-1;
	if(d==0)return 0;
	for(int k=0;k<20;k++)
	{
		int j=peer[i*20+k];
		if(cell[j]==d)
		{
			at=j;
			if(j/9==i/9)return 1;
			if(j%9==i%9)return 2;
			return 3;
		}
	}
	return 0;
}

int Fits(int i, int d)
{
	int at=0;
	return Clash(i,d,at)==0;
}

int Count(int d)
{
	int n=0;
	for(int i=0;i<81;i++)if(cell[i]==d)n++;
	return n;
}

// may the player change cell i?
int Open(int i)
{
	return state==ST_PLAY&&i>=0&&given[i]==0;
}

// with HELP on, can digit d go into cell i?
int Enabled(int i, int d)
{
	if(!Open(i))return 0;
	if(d==0||!help)return 1;
	return Fits(i,d);
}

void CheckSolved()
{
	for(int i=0;i<81;i++)
	{
		if(cell[i]==0)return;
		if(!Fits(i,cell[i]))return;
	}
	playTime=Elapsed();
	state=ST_SOLVED;
	winT=Time();
	cardOpen=1;
	popupOpen=0;
	solved[level]++;
	int t=playTime;
	if(t<1)t=1;
	newBest=best[level]==0||t<best[level];
	if(newBest)best[level]=t;
	Save();
}

// every change goes on the undo stack as (cell, old digit, old notes); the
// records of one move share a move number and are undone together
void Record(int i)
{
	if(nUndo==NUNDO)
	{
		for(int k=1;k<NUNDO;k++)
		{
			undoCell[k-1]=undoCell[k];
			undoOld[k-1]=undoOld[k];
			undoNotes[k-1]=undoNotes[k];
			undoMove[k-1]=undoMove[k];
		}
		nUndo--;
	}
	undoCell[nUndo]=i;
	undoOld[nUndo]=cell[i];
	undoNotes[nUndo]=notes[i];
	undoMove[nUndo]=moveNo;
	nUndo++;
}

// a placed digit wipes the cell's notes and that digit from the notes
// of every cell it sees
void SetCell(int i, int d)
{
	moveNo++;
	Record(i);
	cell[i]=d;
	if(d!=0)
	{
		notes[i]=0;
		int b=1<<(d-1);
		for(int k=0;k<20;k++)
		{
			int j=peer[i*20+k];
			if(notes[j]&b)
			{
				Record(j);
				notes[j]&=~b;
			}
		}
	}
	Save();
	CheckSolved();
}

void SetNotes(int i, int m)
{
	moveNo++;
	Record(i);
	notes[i]=m;
	Save();
}

void Undo()
{
	if(state!=ST_PLAY||nUndo==0)return;
	int m=undoMove[nUndo-1];
	while(nUndo>0&&undoMove[nUndo-1]==m)
	{
		nUndo--;
		int i=undoCell[nUndo];
		cell[i]=undoOld[nUndo];
		notes[i]=undoNotes[nUndo];
		sel=i;     // the last one undone is the cell the move was made on
	}
	Save();
}

// with HELP on, may d go into cell i? Says why not and flashes the clash.
int Allowed(int i, int d)
{
	if(d==0||!help)return 1;
	int at=0;
	int w=Clash(i,d,at);
	if(w==0)return 1;
	char s[64];
	if(w==1)snprintf(s,63,"%i is already in this row",d);
	if(w==2)snprintf(s,63,"%i is already in this column",d);
	if(w==3)snprintf(s,63,"%i is already in this box",d);
	Message(s);
	flashCell=at;
	flashT=Time();
	return 0;
}

// put digit d (0 clears) into cell i, as the rules and HELP allow
void Enter(int i, int d)
{
	if(state!=ST_PLAY||i<0)return;
	if(given[i])
	{
		Message("That digit is a clue");
		return;
	}
	if(d==0)
	{
		// clear the digit, or the notes of an empty cell
		if(cell[i]!=0)SetCell(i,0);
		else if(notes[i]!=0)SetNotes(i,0);
		return;
	}
	if(cell[i]==d)return;
	if(!Allowed(i,d))return;
	SetCell(i,d);
}

// mark or unmark d as a pencil note of cell i
void Note(int i, int d)
{
	if(state!=ST_PLAY||i<0)return;
	if(given[i])
	{
		Message("That digit is a clue");
		return;
	}
	if(d==0)
	{
		Enter(i,0);
		return;
	}
	if(cell[i]!=0)
	{
		Message("Clear the digit to make notes");
		return;
	}
	int b=1<<(d-1);
	if(!(notes[i]&b)&&!Allowed(i,d))return;   // unmarking is always fine
	SetNotes(i,notes[i]^b);
}

// mark in every empty cell the digits its row, column and box allow;
// existing notes only lose the forbidden ones. One undoable move.
void AutoNotes()
{
	if(state!=ST_PLAY)return;
	moveNo++;
	int n=0;
	for(int i=0;i<81;i++)
	{
		if(cell[i]!=0)continue;
		int m=0;
		for(int d=1;d<=9;d++)
			if(Fits(i,d))m|=1<<(d-1);
		if(notes[i]!=0)m&=notes[i];
		if(m!=notes[i])
		{
			Record(i);
			notes[i]=m;
			n++;
		}
	}
	if(n==0)
	{
		Message("The notes are up to date");
		return;
	}
	Save();
}

// a digit from the keyboard, the panel or the popup
void Put(int i, int d)
{
	if(noteMode)Note(i,d);
	else Enter(i,d);
}

void StartGame(int l)
{
	level=l;
	menuLevel=l;
	menuOpen=0;
	statsOpen=0;
	cardOpen=0;
	popupOpen=0;
	genLevel=l;      // made in the main loop once "Making a puzzle" shows
	genShown=0;
}

void MakePuzzle()
{
	Generate(genLevel);
	genLevel=-1;
	for(int i=0;i<81;i++)
	{
		cell[i]=puzzle[i];
		given[i]=puzzle[i]!=0;
		notes[i]=0;
	}
	sel=-1;
	nUndo=0;
	state=ST_PLAY;
	playTime=0;
	resumeT=Time();
	played[level]++;
	Save();
}

// ---------------------------------------------------------------- layout

#define CELL 58
#define BOARD (CELL*9)
#define BX 39
#define BY 92

#define BTN_Y 10
#define BTN_H 28
#define BTN_W 100
#define NEWBTN_X 12
#define STATBTN_X (G_SCREEN_WIDTH-12-BTN_W)
#define X2BTN_W 40
#define X2BTN_X (STATBTN_X-8-X2BTN_W)   // x2: the window twice as big

// digit panel: 1..9 and a clear button
#define PAD_W 50
#define PAD_H 58
#define PAD_GAP 5
#define PAD_X 27.5
#define PAD_Y (BY+BOARD+16)

#define LOW_Y (PAD_Y+PAD_H+14)
#define LOW_H 32
#define UNDO_W 90
#define NOTES_W 116
#define AUTO_W 120
#define LOW_GAP ((PAD_W*10+PAD_GAP*9-UNDO_W-NOTES_W-AUTO_W-HELP_W)/3)
#define NOTES_X (PAD_X+UNDO_W+LOW_GAP)
#define AUTO_X (NOTES_X+NOTES_W+LOW_GAP)
#define MSG_Y (LOW_Y+LOW_H+20)   // messages and hints, under the buttons
#define HELP_W 130
#define HELP_X (G_SCREEN_WIDTH-PAD_X-HELP_W)

// the popup next to a cell
#define POP_B 44
#define POP_G 4
#define POP_PAD 8
#define POP_W (POP_PAD*2+POP_B*3+POP_G*2)
#define POP_H (POP_PAD*2+POP_B*3+POP_G*3+30)

// cards
#define MENU_W 480
#define MENU_ROW 58
#define MENU_H (96+NLEVELS*MENU_ROW+50)
#define MENU_X ((G_SCREEN_WIDTH-MENU_W)/2)
#define MENU_Y ((G_SCREEN_HEIGHT-MENU_H)/2)
#define CARD_BTN_W 150
#define CARD_BTN_H 34

#define STATS_W 540
#define STATS_H 380
#define STATS_X ((G_SCREEN_WIDTH-STATS_W)/2)
#define STATS_Y 170

#define WIN_W 400
#define WIN_H 250
#define WIN_X ((G_SCREEN_WIDTH-WIN_W)/2)
#define WIN_Y 230

int Over(float x, float y, float w, float h, int mx, int my)
{
	return mx>=x&&mx<x+w&&my>=y&&my<y+h;
}

int CellAt(int mx, int my)
{
	if(!Over(BX,BY,BOARD,BOARD,mx,my))return -1;
	int c=(mx-BX)/CELL;
	int r=(my-BY)/CELL;
	return r*9+c;
}

float PadX(int k)
{
	return PAD_X+k*(PAD_W+PAD_GAP);
}

// 1..9 a digit, 10 clear, -1 none
int PadAt(int mx, int my)
{
	for(int k=0;k<10;k++)
		if(Over(PadX(k),PAD_Y,PAD_W,PAD_H,mx,my))return k+1;
	return -1;
}

void OpenPopup(int i)
{
	popupOpen=1;
	popupCell=i;
	sel=i;
	float cx=BX+(i%9)*CELL;
	float cy=BY+(i/9)*CELL;
	popX=cx+CELL+6;
	if(popX+POP_W>G_SCREEN_WIDTH-6)popX=cx-6-POP_W;
	popY=cy+CELL/2-POP_H/2;
	if(popY<52)popY=52;
	if(popY+POP_H>G_SCREEN_HEIGHT-6)popY=G_SCREEN_HEIGHT-6-POP_H;
}

float PopBX(int d)
{
	return popX+POP_PAD+((d-1)%3)*(POP_B+POP_G);
}

float PopBY(int d)
{
	return popY+POP_PAD+((d-1)/3)*(POP_B+POP_G);
}

// 1..9 a digit, 10 clear, 0 inside but on no button, -1 outside
int PopupAt(int mx, int my)
{
	if(!Over(popX,popY,POP_W,POP_H,mx,my))return -1;
	for(int d=1;d<=9;d++)
		if(Over(PopBX(d),PopBY(d),POP_B,POP_B,mx,my))return d;
	if(Over(popX+POP_PAD,popY+POP_H-POP_PAD-30,POP_W-POP_PAD*2,30,mx,my))return 10;
	return 0;
}

float MenuRowY(int l)
{
	return MENU_Y+84+l*MENU_ROW;
}

float MenuBackY()
{
	return MENU_Y+MENU_H-CARD_BTN_H-16;
}

// ---------------------------------------------------------------- input

void NewGameButton()
{
	// mid-game the level menu opens on its own; BACK in it keeps the game
	menuOpen=1;
	statsOpen=0;
	cardOpen=0;
	popupOpen=0;
}

void Move(int dc, int dr)
{
	if(sel<0)
	{
		sel=40;
		return;
	}
	int r=(sel/9+dr+9)%9;
	int c=(sel%9+dc+9)%9;
	sel=r*9+c;
}

// Shift+1..9 (on a US layout) makes a note whatever the mode
int ShiftedDigit(int k)
{
	char* sh="!@#$%^&*(";
	for(int d=0;d<9;d++)
		if(k==sh[d])return d+1;
	return 0;
}

void OnKey(int k)
{
	if(genLevel>=0)return;
	if(k==KEY_ESC)
	{
		if(popupOpen)popupOpen=0;
		else if(statsOpen)statsOpen=0;
		else if(cardOpen)cardOpen=0;
		else if(menuOpen&&state!=ST_NONE)menuOpen=0;
		else sel=-1;
		return;
	}
	if(menuOpen)
	{
		if(k==KEY_UP)menuLevel=(menuLevel+NLEVELS-1)%NLEVELS;
		if(k==KEY_DOWN)menuLevel=(menuLevel+1)%NLEVELS;
		if(k>='1'&&k<'1'+NLEVELS)menuLevel=k-'1';
		if(k==KEY_ENTER||k==KEY_ENTER2)StartGame(menuLevel);
		return;
	}
	if(statsOpen)return;
	if(cardOpen)
	{
		if(k==KEY_ENTER||k==KEY_ENTER2)NewGameButton();
		return;
	}
	int target=sel;
	if(popupOpen)target=popupCell;
	int shifted=ShiftedDigit(k);
	if(k>='1'&&k<='9')
	{
		if(!noteMode)popupOpen=0;    // the popup stays up for more notes
		Put(target,k-'0');
	}
	else if(shifted>0)Note(target,shifted);
	else if(k=='0'||k==KEY_DEL||k==KEY_BACK||k==' ')
	{
		popupOpen=0;
		Enter(target,0);
	}
	else if(k=='n'||k=='N')
	{
		noteMode=1-noteMode;
		Save();
	}
	else if(k=='u'||k=='z'||k=='U'||k=='Z')Undo();
	else if(k=='a'||k=='A')
	{
		popupOpen=0;
		AutoNotes();
	}
	else if(k=='h'||k=='H')
	{
		help=1-help;
		Save();
	}
	else if(popupOpen)return;
	else if(k==KEY_UP)Move(0,-1);
	else if(k==KEY_DOWN)Move(0,1);
	else if(k==KEY_LEFT)Move(-1,0);
	else if(k==KEY_RIGHT)Move(1,0);
}

void MenuClick(int mx, int my)
{
	for(int l=0;l<NLEVELS;l++)
		if(Over(MENU_X+20,MenuRowY(l),MENU_W-40,MENU_ROW-8,mx,my))
		{
			StartGame(l);
			return;
		}
	if(state!=ST_NONE)
	{
		if(Over(MENU_X+(MENU_W-CARD_BTN_W)/2,MenuBackY(),CARD_BTN_W,CARD_BTN_H,mx,my))menuOpen=0;
		else if(!Over(MENU_X,MENU_Y,MENU_W,MENU_H,mx,my))menuOpen=0;
	}
}

float CardBtnY()
{
	return WIN_Y+WIN_H-CARD_BTN_H-20;
}

void CardClick(int mx, int my)
{
	if(Over(WIN_X+30,CardBtnY(),CARD_BTN_W,CARD_BTN_H,mx,my))cardOpen=0;
	else if(Over(WIN_X+WIN_W-30-CARD_BTN_W,CardBtnY(),CARD_BTN_W,CARD_BTN_H,mx,my))NewGameButton();
}

void StatsClick(int mx, int my)
{
	statsOpen=0;     // BACK, or anywhere else: there is nothing to press inside
}

void OnClick(int mx, int my, int right)
{
	if(Over(X2BTN_X,BTN_Y,X2BTN_W,BTN_H,mx,my))
	{
		scaleReq=1;    // done by the main loop, which owns the window
		return;
	}
	if(genLevel>=0)return;
	if(Over(NEWBTN_X,BTN_Y,BTN_W,BTN_H,mx,my))
	{
		if(menuOpen&&state!=ST_NONE)menuOpen=0;
		else NewGameButton();
		return;
	}
	if(Over(STATBTN_X,BTN_Y,BTN_W,BTN_H,mx,my))
	{
		statsOpen=1-statsOpen;
		popupOpen=0;
		return;
	}
	if(menuOpen)
	{
		MenuClick(mx,my);
		return;
	}
	if(statsOpen)
	{
		StatsClick(mx,my);
		return;
	}
	if(cardOpen)
	{
		CardClick(mx,my);
		return;
	}
	int wasOpen=popupOpen;
	if(popupOpen)
	{
		int p=PopupAt(mx,my);
		if(p==0)return;
		if(p>0)
		{
			if(p==10)p=0;
			// in notes mode the popup stays up to mark several digits
			if(!noteMode||p==0)popupOpen=0;
			if(Enabled(popupCell,p))Put(popupCell,p);
			return;
		}
		// a click outside closes the popup and goes on as a normal click
		popupOpen=0;
	}
	int i=CellAt(mx,my);
	if(i>=0)
	{
		if(Open(i)&&(right||(i==sel&&!wasOpen)))OpenPopup(i);
		else sel=i;
		return;
	}
	int d=PadAt(mx,my);
	if(d>0)
	{
		if(d==10)d=0;
		if(sel<0)Message("Select a cell first");
		else Put(sel,d);
		return;
	}
	if(Over(PAD_X,LOW_Y,UNDO_W,LOW_H,mx,my))
	{
		Undo();
		return;
	}
	if(Over(NOTES_X,LOW_Y,NOTES_W,LOW_H,mx,my))
	{
		noteMode=1-noteMode;
		Save();
		return;
	}
	if(Over(AUTO_X,LOW_Y,AUTO_W,LOW_H,mx,my))
	{
		AutoNotes();
		return;
	}
	if(Over(HELP_X,LOW_Y,HELP_W,LOW_H,mx,my))
	{
		help=1-help;
		Save();
		return;
	}
	sel=-1;
}

// ---------------------------------------------------------------- drawing

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

float TextW(char* s, float sc)
{
	return (TextLen(s)*6-2)*sc;
}

// centred on (cx,cy)
void TextC(char* s, float cx, float cy, float sc, int c, float r)
{
	Text(s,cx-TextW(s,sc)/2,cy+4.5*sc,sc,c,r);
}

void Digit(int d, float cx, float cy, float sc, int c, float r)
{
	g.clear();
	DrawGlyph(g,*pfont,'0'+d,cx-2*sc,cy+4.5*sc,sc,sc);
	g.fin();
	g.rgba32(c);
	Stroke(r,2.);
}

void Button(char* s, float x, float y, float w, float h, int c, int mx, int my)
{
	FillBox(x,y,w,h,5,c);
	if(Over(x,y,w,h,mx,my))FillBox(x,y,w,h,5,0x20ffffff);
	TextC(s,x+w/2,y+h/2,1.4,COL_TEXT,1.2);
}

// a card over a darkened screen
void Card(float x, float y, float w, float h)
{
	FillBox(0,0,G_SCREEN_WIDTH,G_SCREEN_HEIGHT,0,0x99000000);
	FillBox(x+4,y+6,w,h,12,0x60000000);
	FillBox(x,y,w,h,12,0xff2a3142);
	FillBox(x+1,y+1,w-2,h-2,11,0xff222838);
}

float Clamp01(float x)
{
	if(x<0)return 0;
	if(x>1)return 1;
	return x;
}

int Mix(int a, int b, float t)
{
	int r=0;
	for(int s=0;s<32;s+=8)
	{
		int ca=(a>>s)&255;
		int cb=(b>>s)&255;
		r|=(int(ca+(cb-ca)*t)&255)<<s;
	}
	return r;
}

void Pips(int l, float x, float cy, int on, int off)
{
	for(int k=0;k<NLEVELS;k++)
	{
		int c=off;
		if(k<=l)c=on;
		FillBox(x+k*13,cy-4.5,9,9,4.5,c);
	}
}

void DrawClock()
{
	char s[16];
	TimeText(Elapsed(),s);
	int c=COL_TEXT;
	if(state==ST_SOLVED)c=COL_USER;
	Text(s,BX+BOARD-TextW(s,1.8),70+8.1,1.8,c,1.3);
}

// The clock ticks once a second while the rest of the screen stays still:
// then only its corner is cleared and drawn again, which costs a fraction
// of a full frame. Not under an overlay, which would have to be redrawn too.
int ClockOnly()
{
	return state==ST_PLAY&&!menuOpen&&!statsOpen&&!cardOpen&&!popupOpen&&genLevel<0;
}

void RedrawClock()
{
	FillBox(BX+BOARD-130,56,132,30,0,COL_BG);
	DrawClock();
}

void DrawHeader(int mx, int my)
{
	TextC("SUDOKU",G_SCREEN_WIDTH/2,26,3.0,COL_TEXT,2.);
	FillBox(0,48,G_SCREEN_WIDTH,1,0,COL_THIN);
	Button("NEW GAME",NEWBTN_X,BTN_Y,BTN_W,BTN_H,COL_BTN,mx,my);
	Button("STATISTICS",STATBTN_X,BTN_Y,BTN_W,BTN_H,COL_BTN,mx,my);
	if(SC==2)Button("x2",X2BTN_X,BTN_Y,X2BTN_W,BTN_H,COL_ACCENT,mx,my);
	else Button("x2",X2BTN_X,BTN_Y,X2BTN_W,BTN_H,COL_BTN,mx,my);
	if(state==ST_NONE)return;
	Pips(level,BX,70,COL_USER,COL_THIN);
	Text(levelName[level],BX+72,70+6.3,1.4,COL_DIM,1.1);
	DrawClock();
}

void DrawBoard(float T)
{
	int sv=0;
	if(sel>=0)sv=cell[sel];
	for(int i=0;i<81;i++)
	{
		float x=BX+(i%9)*CELL;
		float y=BY+(i/9)*CELL;
		int c=COL_CELL;
		if(sel>=0&&state!=ST_NONE)
		{
			if(Sees(i,sel))c=COL_PEER;
			if(sv!=0&&cell[i]==sv)c=COL_SAME;
			if(i==sel)c=COL_SEL;
		}
		if(i==flashCell&&T-flashT<0.8)c=Mix(0xff8a3434,c,Clamp01((T-flashT)/0.8));
		if(state==ST_SOLVED)
		{
			// a wave of light across the board, corner to corner
			float u=(T-winT)*1.4-((i/9)+(i%9))/16.0;
			if(u>0&&u<1)c=Mix(c,0xff5f86c8,sin(u*3.14159));
		}
		FillBox(x,y,CELL,CELL,0,c);
	}
	for(int k=1;k<9;k++)
	{
		if(k%3==0)continue;
		FillBox(BX+k*CELL-0.5,BY,1,BOARD,0,COL_THIN);
		FillBox(BX,BY+k*CELL-0.5,BOARD,1,0,COL_THIN);
	}
	for(int k=0;k<=3;k++)
	{
		FillBox(BX+k*CELL*3-1.5,BY-1.5,3,BOARD+3,0,COL_THICK);
		FillBox(BX-1.5,BY+k*CELL*3-1.5,BOARD+3,3,0,COL_THICK);
	}
	if(state==ST_NONE)return;
	for(int i=0;i<81;i++)
	{
		if(cell[i]!=0||notes[i]==0)continue;
		for(int d=1;d<=9;d++)
		{
			if(!(notes[i]&(1<<(d-1))))continue;
			float nx=BX+(i%9)*CELL+CELL/6.0+((d-1)%3)*(CELL/3.0);
			float ny=BY+(i/9)*CELL+CELL/6.0+((d-1)/3)*(CELL/3.0);
			int c=0xff9aa6be;
			if(d==sv)
			{
				// notes of the selected digit stand out
				FillBox(nx-8.5,ny-8.5,17,17,4,0xff4a6aa4);
				c=COL_TEXT;
			}
			Digit(d,nx,ny,1.3,c,1.05);
		}
	}
	for(int i=0;i<81;i++)
	{
		int d=cell[i];
		if(d==0)continue;
		float cx=BX+(i%9)*CELL+CELL/2;
		float cy=BY+(i/9)*CELL+CELL/2;
		int c=COL_USER;
		if(given[i])c=COL_GIVEN;
		if(!Fits(i,d))c=COL_BAD;     // a clue too, when a wrong digit clashes with it
		Digit(d,cx,cy,3.1,c,2.1);
	}
}

void DrawPad(int mx, int my)
{
	for(int k=0;k<10;k++)
	{
		float x=PadX(k);
		int d=k+1;
		if(d==10)d=0;
		int on=state==ST_PLAY;
		if(on&&sel>=0)on=Enabled(sel,d);
		int c=COL_BTN;
		if(!on)c=COL_OFF;
		if(on&&noteMode&&d!=0&&sel>=0&&(notes[sel]&(1<<(d-1))))c=COL_NOTE;
		FillBox(x,PAD_Y,PAD_W,PAD_H,7,c);
		if(on&&Over(x,PAD_Y,PAD_W,PAD_H,mx,my))FillBox(x,PAD_Y,PAD_W,PAD_H,7,0x20ffffff);
		int tc=COL_TEXT;
		if(!on)tc=0xff4a5266;
		if(d==0)
		{
			TextC("CLEAR",x+PAD_W/2,PAD_Y+PAD_H/2,1.15,tc,1.1);
			continue;
		}
		Digit(d,x+PAD_W/2,PAD_Y+PAD_H/2-3,2.8,tc,1.9);
		// how many of this digit are still to place
		int left=9-Count(d);
		if(state!=ST_NONE&&left>0)
		{
			char s[4];
			snprintf(s,3,"%i",left);
			int lc=COL_DIM;
			if(!on)lc=0xff4a5266;
			TextC(s,x+PAD_W/2,PAD_Y+PAD_H-9,1.0,lc,1.0);
		}
	}
}

void DrawLow(float T, int mx, int my)
{
	Button("UNDO",PAD_X,LOW_Y,UNDO_W,LOW_H,COL_BTN,mx,my);
	if(noteMode)Button("NOTES: ON",NOTES_X,LOW_Y,NOTES_W,LOW_H,COL_NOTE,mx,my);
	else Button("NOTES: OFF",NOTES_X,LOW_Y,NOTES_W,LOW_H,COL_BTN,mx,my);
	Button("AUTO NOTES",AUTO_X,LOW_Y,AUTO_W,LOW_H,COL_BTN,mx,my);
	int c=COL_BTN;
	if(help)c=COL_ACCENT;
	if(help)Button("HELP: ON",HELP_X,LOW_Y,HELP_W,LOW_H,c,mx,my);
	else Button("HELP: OFF",HELP_X,LOW_Y,HELP_W,LOW_H,c,mx,my);
	float cx=G_SCREEN_WIDTH/2;
	float cy=MSG_Y;
	if(T-msgT<2.5)
	{
		int a=255*Clamp01((2.5-(T-msgT))/0.4);
		TextC(msg,cx,cy,1.1,(a<<24)|0xffd27a,1.0);
	}
	else if(state==ST_PLAY&&noteMode)TextC("digits mark notes",cx,cy,1.1,0xffb09a6a,1.0);
	else if(state==ST_PLAY)TextC("right-click: digit menu",cx,cy,1.1,0xff5d667a,1.0);
}

void DrawPopup(int mx, int my)
{
	FillBox(popX+3,popY+5,POP_W,POP_H,10,0x70000000);
	int frame=0xff4a5470;
	if(noteMode)frame=0xffc9a24a;    // notes mode: the popup marks notes
	FillBox(popX,popY,POP_W,POP_H,10,frame);
	FillBox(popX+1,popY+1,POP_W-2,POP_H-2,9,0xff2a3142);
	for(int d=1;d<=10;d++)
	{
		float x=popX+POP_PAD;
		float y=popY+POP_H-POP_PAD-30;
		float w=POP_W-POP_PAD*2;
		float h=30;
		if(d<10)
		{
			x=PopBX(d);
			y=PopBY(d);
			w=POP_B;
			h=POP_B;
		}
		int v=d;
		if(v==10)v=0;
		int on=Enabled(popupCell,v);
		int c=COL_BTN;
		if(!on)c=COL_OFF;
		if(v!=0&&cell[popupCell]==v)c=COL_SEL;
		if(v!=0&&noteMode&&(notes[popupCell]&(1<<(v-1))))c=COL_NOTE;
		FillBox(x,y,w,h,6,c);
		if(on&&Over(x,y,w,h,mx,my))FillBox(x,y,w,h,6,0x20ffffff);
		int tc=COL_TEXT;
		if(!on)tc=0xff4a5266;
		if(v==0)TextC("CLEAR",x+w/2,y+h/2,1.2,tc,1.1);
		else Digit(v,x+w/2,y+h/2,2.4,tc,1.7);
	}
}

void DrawMenu(int mx, int my)
{
	Card(MENU_X,MENU_Y,MENU_W,MENU_H);
	TextC("New game",G_SCREEN_WIDTH/2,MENU_Y+30,2.4,COL_TEXT,1.7);
	TextC("choose how hard it is",G_SCREEN_WIDTH/2,MENU_Y+60,1.2,COL_DIM,1.0);
	for(int l=0;l<NLEVELS;l++)
	{
		float x=MENU_X+20;
		float y=MenuRowY(l);
		float w=MENU_W-40;
		float h=MENU_ROW-8;
		int c=COL_BTN;
		if(l==menuLevel)c=0xff3d5584;
		FillBox(x,y,w,h,8,c);
		if(Over(x,y,w,h,mx,my))FillBox(x,y,w,h,8,0x20ffffff);
		Pips(l,x+18,y+h/2,COL_USER,0xff283044);
		Text(levelName[l],x+100,y+h/2+7.2,1.6,COL_TEXT,1.25);
	}
	if(state!=ST_NONE)
		Button("BACK",MENU_X+(MENU_W-CARD_BTN_W)/2,MenuBackY(),CARD_BTN_W,CARD_BTN_H,COL_BTN,mx,my);
	else TextC("1-5 or the arrows and Enter work too",G_SCREEN_WIDTH/2,MenuBackY()+CARD_BTN_H/2,1.1,COL_DIM,1.0);
}

void DrawStats(int mx, int my)
{
	Card(STATS_X,STATS_Y,STATS_W,STATS_H);
	TextC("Statistics",G_SCREEN_WIDTH/2,STATS_Y+34,2.4,COL_TEXT,1.7);
	float c1=STATS_X+375;
	float c2=STATS_X+440;
	float c3=STATS_X+500;
	float y=STATS_Y+80;
	TextC("PLAYED",c1,y,1.1,COL_DIM,1.0);
	TextC("SOLVED",c2,y,1.1,COL_DIM,1.0);
	TextC("BEST",c3,y,1.1,COL_DIM,1.0);
	for(int l=0;l<NLEVELS;l++)
	{
		y=STATS_Y+118+l*44;
		FillBox(STATS_X+16,y-20,STATS_W-32,40,6,0xff2a3142);
		Pips(l,STATS_X+28,y,COL_USER,0xff3a4256);
		Text(levelName[l],STATS_X+100,y+5.85,1.3,COL_TEXT,1.1);
		char s[16];
		snprintf(s,15,"%i",played[l]);
		TextC(s,c1,y,1.5,COL_TEXT,1.2);
		snprintf(s,15,"%i",solved[l]);
		TextC(s,c2,y,1.5,COL_TEXT,1.2);
		if(best[l]>0)TimeText(best[l],s);
		else snprintf(s,15,"-");
		TextC(s,c3,y,1.5,COL_TEXT,1.2);
	}
	Button("BACK",(G_SCREEN_WIDTH-CARD_BTN_W)/2,STATS_Y+STATS_H-CARD_BTN_H-14,CARD_BTN_W,CARD_BTN_H,COL_BTN,mx,my);
}

void DrawCard(float T, int mx, int my)
{
	Card(WIN_X,WIN_Y,WIN_W,WIN_H);
	TextC("Solved!",G_SCREEN_WIDTH/2,WIN_Y+42,3.4,COL_TEXT,2.2);
	Pips(level,G_SCREEN_WIDTH/2-30,WIN_Y+86,COL_USER,COL_THIN);
	TextC(levelName[level],G_SCREEN_WIDTH/2,WIN_Y+108,1.4,COL_DIM,1.1);
	char t[16];
	TimeText(playTime,t);
	char s[48];
	snprintf(s,47,"in %s",t);
	TextC(s,G_SCREEN_WIDTH/2,WIN_Y+142,2.0,COL_TEXT,1.5);
	if(newBest)TextC("best time so far!",G_SCREEN_WIDTH/2,WIN_Y+170,1.3,0xffffd27a,1.1);
	Button("LOOK",WIN_X+30,CardBtnY(),CARD_BTN_W,CARD_BTN_H,COL_BTN,mx,my);
	Button("NEW GAME",WIN_X+WIN_W-30-CARD_BTN_W,CardBtnY(),CARD_BTN_W,CARD_BTN_H,COL_ACCENT,mx,my);
}

void DrawMaking(float T)
{
	Card(150,320,300,110);
	TextC("Making a puzzle",G_SCREEN_WIDTH/2,350,1.8,COL_TEXT,1.4);
	TextC(levelName[genLevel],G_SCREEN_WIDTH/2,388,1.3,COL_DIM,1.1);
	Pips(genLevel,G_SCREEN_WIDTH/2-30,410,COL_USER,COL_THIN);
}

void Draw(int mx, int my)
{
	float T=Time();
	g.rgba32(COL_BG);
	g.FillRT();
	DrawHeader(mx,my);
	DrawBoard(T);
	DrawPad(mx,my);
	DrawLow(T,mx,my);
	if(popupOpen)DrawPopup(mx,my);
	if(cardOpen&&T-winT>1.2)DrawCard(T,mx,my);
	if(menuOpen&&genLevel<0)DrawMenu(mx,my);
	if(statsOpen)DrawStats(mx,my);
	if(genLevel>=0)DrawMaking(T);
}

// ---------------------------------------------------------------- main

int Animating(float T)
{
	if(state==ST_SOLVED&&T-winT<2.0)return 1;
	if(T-flashT<0.9)return 1;
	if(T-msgT<2.6)return 1;
	return quitReq!=0;      // ws.h draws its quit prompt in Present
}

// slow changes: the clock (see ClockOnly)
int Look(float T)
{
	return int(Elapsed());
}

// The x2 button switches between the unscaled window and one twice as big, rendered at
// twice the resolution (-x2 starts with the big one). The layout stays in
// logical units: only the window, the transform and SC change.
float winSC=1;     // the scale of the window we have now

void SetScale(float s)
{
	SC=s;
	if(winSC!=SC)
	{
		wsys.DeleteWindow(mainWin);
		mainWin=wsys.CreateWindow(G_SCREEN_WIDTH*SC,G_SCREEN_HEIGHT*SC,1,1,G_SCREEN_MODE);
		g.SetActiveRT(wsys.GetWindowRT(mainWin));
		winSC=SC;
	}
	g.t_0(0,0);
	g.t_x(SC,0);
	g.t_y(0,SC);
}

int main(int argc, pchar* argv)
{
	for(int i=1;i<argc;i++)
	{
		char* a=argv[i];
		if(a[0]=='-'&&a[1]=='x'&&a[2]=='2'&&a[3]==0)SC=2;
	}
	SetScale(SC);

	pfont=(CFont*)malloc(sizeof(CFont));
	CFontMaker fm;
	fm.Init(*pfont,6);
	MakeLetters(fm);

	InitTables();
	InitLevels();
	int timeSeed=Time()*1e9;
	seed+=timeSeed;
	irand(seed);

	for(int i=0;i<81;i++)
	{
		cell[i]=0;
		given[i]=0;
		notes[i]=0;
		solution[i]=0;
	}
	Load();
	if(state==ST_NONE)menuOpen=1;

	int prevMB=0;
	int prevMX=-1;
	int prevMY=-1;
	int prevLook=-1;
	float lastPresent=0;
	int dirty=1;
	int wasAnim=0;
	while(true)
	{
		float T=Time();
		if(genLevel>=0&&genShown)
		{
			MakePuzzle();
			dirty=1;
		}

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
		if((mb&1)&&!(prevMB&1))OnClick(mx,my,0);
		if(scaleReq)
		{
			scaleReq=0;
			SetScale(3-SC);
			dirty=1;
		}
		if((mb&4)&&!(prevMB&4))OnClick(mx,my,1);
		if(mx!=prevMX||my!=prevMY||mb!=prevMB)dirty=1;
		prevMB=mb;
		prevMX=mx;
		prevMY=my;

		int look=Look(T);
		int tick=look!=prevLook;
		prevLook=look;

		int anim=Animating(T);
		if(wasAnim&&!anim)dirty=1;
		wasAnim=anim;
		if(cardOpen&&T-winT>1.2&&T-winT<1.3)dirty=1;

		if(dirty||anim)
		{
			Draw(mx,my);
			Present();
			if(genLevel>=0)genShown=1;
			lastPresent=T;
			dirty=0;
			Wait(0.01);
		}
		else if(tick&&ClockOnly())
		{
			RedrawClock();
			Present();
			lastPresent=T;
			Wait(0.01);
		}
		else if(tick)dirty=1;     // drawn in full on the next pass
		else
		{
			// nothing changed: keep the events flowing and, every few
			// seconds, show the frame again in case the window was covered
			// (the runtime reports no expose event). Showing a still frame
			// is the main idle cost, more so in the x2 window, so not often.
			Poll();
			SScincEvent ev;
			while(GetScincEvent(ev)){}
			if(T-lastPresent>3)
			{
				wsys.Present(mainWin);
				lastPresent=T;
			}
			Wait(0.03);
		}
	}
	return 0;
}
