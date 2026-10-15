// wordy.cpp - a Wordle-style word game.
//
// Guess the hidden five-letter word in six tries. After each guess the
// tiles flip and show how close it was:
//   green  - right letter in the right place
//   yellow - the letter is in the word, but somewhere else
//   grey   - the letter is not in the word (or not that many times)
// The on-screen keyboard keeps the best colour found for every letter.
//
// Guesses must be real words from all5letterw.h (12972 words); the secret
// word is picked from the more common ones in answers.h.
//
// Type letters (or click the on-screen keys), Backspace deletes, Enter
// submits. When a game is over, Enter (or a click) starts a new word.

#include <stdio.h>

#define G_SCREEN_WIDTH 500
#define G_SCREEN_HEIGHT 720

#include "all5letterw.h"
#include "answers.h"

#include "graphics.h"
#include "../../ws.h"
#include "../../graphic/font.h"
#include "../../graphic/letters.h"
#include "../../graphic/gtext.h"

#define M_PI 3.14159265358979

#define COL_BG      0xff121213
#define COL_EMPTY   0xff3a3a3c
#define COL_TYPED   0xff6a6a6c
#define COL_ABSENT  0xff3a3a3c
#define COL_PRESENT 0xffb59f3b
#define COL_CORRECT 0xff538d4e
#define COL_KEY     0xff818384
#define COL_TEXT    0xffffffff

#define KEY_ENTER 4013
#define KEY_ENTER2 5013
#define KEY_BACK  4008

// board geometry
#define TILE 62
#define GAP 6
#define BOARD_X 83
#define BOARD_Y 86

Graph g;
CFont* pfont=0;

int irand(int& seed)
{
	seed=(seed*1103515245+12345)&0x7ffffff;
	return seed;
}

int seed=6917832;

// ---------------------------------------------------------------- game state

int secret[5];
int guess[6*5];    // letters 'a'..'z', 0 = empty
int mark[6*5];     // 0 not scored, 1 absent, 2 present, 3 correct
int keyMark[26];   // best mark seen for each letter
int row=0;         // row being typed
int col=0;         // next free tile in that row
int state=0;       // 0 playing, 1 won, 2 lost
float endT=0;      // when the game was decided

int revealRow=-1;  // row whose tiles are flipping
float revealT0=0;
float shakeT0=-10;
float bounceT0=-10;
float popT[6*5];

char msg[64];
float msgT0=-10;
float msgDur=0;    // <0 keeps the message up until the next game

// statistics for this session
int played=0;
int wins=0;
int streak=0;
int maxStreak=0;
int dist[6];
int lastWinRow=-1;

#define FLIP_STEP 0.3
#define FLIP_TIME 0.5

void Message(char* s, float dur)
{
	int i=0;
	while(s[i]!=0&&i<63)
	{
		msg[i]=s[i];
		i++;
	}
	msg[i]=0;
	msgT0=Time();
	msgDur=dur;
}

void NewGame()
{
	int n=(irand(seed)>>4)%N_ANSWERS;
	char* w=answers[n];
	for(int i=0;i<5;i++)secret[i]=w[i];
	for(int i=0;i<30;i++)
	{
		guess[i]=0;
		mark[i]=0;
		popT[i]=-10;
	}
	for(int i=0;i<26;i++)keyMark[i]=0;
	row=0;
	col=0;
	state=0;
	revealRow=-1;
	msgDur=0;
	lastWinRow=-1;
}

// binary search in the sorted list of allowed guesses
int InWordList(int r)
{
	int lo=0;
	int hi=N_ALL5LETTER-1;
	while(lo<=hi)
	{
		int mid=(lo+hi)/2;
		char* w=all5letter[mid];
		int d=0;
		for(int i=0;i<5;i++)
		{
			d=guess[r*5+i]-w[i];
			if(d!=0)break;
		}
		if(d==0)return 1;
		if(d<0)hi=mid-1;
		else lo=mid+1;
	}
	return 0;
}

// Wordle scoring: greens first, then yellows only while unmatched copies of
// the letter remain in the secret word.
void Score(int r)
{
	int left[26];
	for(int i=0;i<26;i++)left[i]=0;
	for(int i=0;i<5;i++)
	{
		if(guess[r*5+i]==secret[i])mark[r*5+i]=3;
		else
		{
			mark[r*5+i]=1;
			left[secret[i]-'a']++;
		}
	}
	for(int i=0;i<5;i++)
	{
		if(mark[r*5+i]==3)continue;
		int c=guess[r*5+i]-'a';
		if(left[c]>0)
		{
			mark[r*5+i]=2;
			left[c]--;
		}
	}
}

// called when the flip animation of a row has finished
void RowRevealed()
{
	int r=revealRow;
	revealRow=-1;
	int greens=0;
	for(int i=0;i<5;i++)
	{
		int c=guess[r*5+i]-'a';
		if(mark[r*5+i]>keyMark[c])keyMark[c]=mark[r*5+i];
		if(mark[r*5+i]==3)greens++;
	}
	if(greens==5)
	{
		state=1;
		endT=Time();
		bounceT0=endT;
		played++;
		wins++;
		streak++;
		if(streak>maxStreak)maxStreak=streak;
		dist[r]++;
		lastWinRow=r;
		if(r==0)Message("Genius",2.5);
		else if(r==1)Message("Magnificent",2.5);
		else if(r==2)Message("Impressive",2.5);
		else if(r==3)Message("Splendid",2.5);
		else if(r==4)Message("Great",2.5);
		else Message("Phew",2.5);
		return;
	}
	row++;
	col=0;
	if(row>=6)
	{
		state=2;
		endT=Time();
		played++;
		streak=0;
		char s[8];
		for(int i=0;i<5;i++)s[i]=secret[i]-'a'+'A';
		s[5]=0;
		Message(s,-1);
	}
}

void OnKey(int k)
{
	if(state!=0)
	{
		if(Time()-endT>0.8&&(k==KEY_ENTER||k==KEY_ENTER2||k==' '))NewGame();
		return;
	}
	if(revealRow>=0)return;
	if(k>='A'&&k<='Z')k=k-'A'+'a';
	if(k>='a'&&k<='z')
	{
		if(col<5)
		{
			guess[row*5+col]=k;
			popT[row*5+col]=Time();
			col++;
		}
	}
	else if(k==KEY_BACK)
	{
		if(col>0)
		{
			col--;
			guess[row*5+col]=0;
		}
	}
	else if(k==KEY_ENTER||k==KEY_ENTER2)
	{
		if(col<5)
		{
			Message("Not enough letters",1.2);
			shakeT0=Time();
		}
		else if(!InWordList(row))
		{
			Message("Not in word list",1.2);
			shakeT0=Time();
		}
		else
		{
			Score(row);
			revealRow=row;
			revealT0=Time();
		}
	}
}

// ---------------------------------------------------------------- on-screen keyboard

#define NKEYS 28
float keyX[NKEYS];
float keyY[NKEYS];
float keyW[NKEYS];
int keyCode[NKEYS];
#define KEY_H 56

void LayoutRow(char* s, int n, int& k, float y, int wide)
{
	float w=43;
	float gap=6;
	float total=n*w+(n-1)*gap;
	if(wide)total+=2*(66+gap);
	float x=(G_SCREEN_WIDTH-total)/2;
	if(wide)
	{
		keyX[k]=x;keyY[k]=y;keyW[k]=66;keyCode[k]=KEY_ENTER;k++;
		x+=66+gap;
	}
	for(int i=0;i<n;i++)
	{
		keyX[k]=x;keyY[k]=y;keyW[k]=w;keyCode[k]=s[i];k++;
		x+=w+gap;
	}
	if(wide)
	{
		keyX[k]=x;keyY[k]=y;keyW[k]=66;keyCode[k]=KEY_BACK;k++;
	}
}

void LayoutKeys()
{
	int k=0;
	LayoutRow("qwertyuiop",10,k,518,0);
	LayoutRow("asdfghjkl",9,k,518+KEY_H+8,0);
	LayoutRow("zxcvbnm",7,k,518+2*(KEY_H+8),1);
}

int KeyAt(int mx, int my)
{
	for(int i=0;i<NKEYS;i++)
	{
		if(mx>=keyX[i]&&mx<keyX[i]+keyW[i]&&my>=keyY[i]&&my<keyY[i]+KEY_H)return keyCode[i];
	}
	return -1;
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

void FrameBox(float x, float y, float w, float h, float t, int c)
{
	g.clear();
	g.M(x,y);g.l(w,0);g.l(0,h);g.l(-w,0);g.close();
	g.M(x+t,y+t);g.l(0,h-2*t);g.l(w-2*t,0);g.l(0,-h+2*t);g.close();
	g.fin();
	g.rgba32(c);
	g.fill1();
}

int TextLen(char* s)
{
	int n=0;
	while(s[n]!=0)n++;
	return n;
}

// stroke-font text; the glyphs are 4 units wide and 9 tall over a 6 unit advance
void Text(char* s, float x, float y, float sx, float sy, int c, float w)
{
	g.clear();
	DrawText(g,*pfont,s,0,x,y,sx,sy);
	g.fin();
	g.rgba32(c);
	g.width(w,1.);
	g.stroke();
}

void TextC(char* s, float cx, float cy, float sc, int c, float w)
{
	float tw=(TextLen(s)*6-2)*sc;
	Text(s,cx-tw/2,cy+4.5*sc,sc,sc,c,w);
}

void Letter(int ch, float cx, float cy, float sx, float sy, int c, float w)
{
	g.clear();
	DrawGlyph(g,*pfont,ch,cx-2*sx,cy+4.5*sy,sx,sy);
	g.fin();
	g.rgba32(c);
	g.width(w,1.);
	g.stroke();
}

int MarkColor(int m)
{
	if(m==3)return COL_CORRECT;
	if(m==2)return COL_PRESENT;
	if(m==1)return COL_ABSENT;
	return COL_KEY;
}

float Clamp01(float x)
{
	if(x<0)return 0;
	if(x>1)return 1;
	return x;
}

void DrawBoard(float T)
{
	for(int r=0;r<6;r++)
	{
		float dx=0;
		if(r==row&&state==0)
		{
			float t=T-shakeT0;
			if(t<0.45)dx=9*sin(t*45)*(1-t/0.45);
		}
		for(int c=0;c<5;c++)
		{
			int i=r*5+c;
			float x=BOARD_X+c*(TILE+GAP)+dx;
			float y=BOARD_Y+r*(TILE+GAP);
			float cx=x+TILE/2;
			float cy=y+TILE/2;
			float sy=1;          // vertical squash of the flip
			float sc=1;          // pop when typed
			int shown=mark[i];   // mark visible so far
			if(r==revealRow)
			{
				float t=(T-revealT0-c*FLIP_STEP)/FLIP_TIME;
				if(t<0.5)shown=0;
				if(t>0&&t<1)
				{
					sy=cos(M_PI*t);
					if(sy<0)sy=-sy;
				}
			}
			if(r==lastWinRow)
			{
				float u=(T-bounceT0-FLIP_STEP*5-c*0.1)/0.45;
				if(u>0&&u<1)cy-=22*sin(M_PI*u);
			}
			{
				float u=(T-popT[i])/0.12;
				if(u>0&&u<1)sc=1+0.1*sin(M_PI*u);
			}
			float w=TILE*sc;
			float h=TILE*sc*sy;
			if(shown!=0)
			{
				FillBox(cx-w/2,cy-h/2,w,h,0,MarkColor(shown));
			}
			else
			{
				int bc=COL_EMPTY;
				if(guess[i]!=0)bc=COL_TYPED;
				FrameBox(cx-w/2,cy-h/2,w,h,2,bc);
			}
			if(guess[i]!=0)
			{
				Letter(guess[i]-'a'+'A',cx,cy,3.6*sc,3.4*sc*sy,COL_TEXT,4.);
			}
		}
	}
}

void DrawBackArrow(float cx, float cy, int c)
{
	g.clear();
	g.M(cx-13,cy);
	g.L(cx-6,cy-8);
	g.L(cx+12,cy-8);
	g.L(cx+12,cy+8);
	g.L(cx-6,cy+8);
	g.close();
	g.M(cx-2,cy-4);g.L(cx+6,cy+4);
	g.M(cx+6,cy-4);g.L(cx-2,cy+4);
	g.fin();
	g.rgba32(c);
	g.width(2.,1.);
	g.stroke();
}

void DrawKeyboard(int mx, int my, int mb)
{
	for(int i=0;i<NKEYS;i++)
	{
		int k=keyCode[i];
		int c=COL_KEY;
		if(k>='a'&&k<='z')c=MarkColor(keyMark[k-'a']);
		float y=keyY[i];
		int hover=(mx>=keyX[i]&&mx<keyX[i]+keyW[i]&&my>=y&&my<y+KEY_H);
		if(hover&&(mb&1))y+=2.0;
		FillBox(keyX[i],y,keyW[i],KEY_H,5,c);
		if(hover)
		{
			FillBox(keyX[i],y,keyW[i],KEY_H,5,0x18ffffff);
		}
		float cx=keyX[i]+keyW[i]/2;
		float cy=y+KEY_H/2;
		if(k==KEY_ENTER)TextC("ENTER",cx,cy,1.5,COL_TEXT,2.);
		else if(k==KEY_BACK)DrawBackArrow(cx,cy,COL_TEXT);
		else Letter(k-'a'+'A',cx,cy,2.2,2.0,COL_TEXT,2.8);
	}
}

void DrawMessage(float T)
{
	if(msgDur==0)return;
	float t=T-msgT0;
	float a=1;
	if(msgDur>0)
	{
		if(t>msgDur)
		{
			msgDur=0;
			return;
		}
		if(t>msgDur-0.3)a=(msgDur-t)/0.3;
	}
	float sc=2.2;
	float w=(TextLen(msg)*6-2)*sc+36;
	float h=44;
	int ai=int(a*255);
	FillBox(G_SCREEN_WIDTH/2-w/2,BOARD_Y-36,w,h,6,(ai<<24)|0xf0f0f0);
	TextC(msg,G_SCREEN_WIDTH/2,BOARD_Y-36+h/2,sc,(ai<<24)|0x121213,3.);
}

void DrawStats(float T)
{
	float t=T-endT-FLIP_STEP*5-1.6;
	if(state==2)t=T-endT-1.0;
	if(t<0)return;
	float a=Clamp01(t/0.3);
	int ai=int(a*255);
	float x=40;
	float y=150;
	float w=G_SCREEN_WIDTH-80;
	float h=360;
	FillBox(x,y+(1-a)*30,w,h,10,(ai<<24)|0x1a1a1b);
	if(a<1)return;
	FrameBox(x,y,w,h,1,0xff3a3a3c);
	TextC("STATISTICS",G_SCREEN_WIDTH/2,y+30,2.2,COL_TEXT,3.);

	int vals[4];
	vals[0]=played;
	vals[1]=0;
	if(played>0)vals[1]=(wins*100+played/2)/played;
	vals[2]=streak;
	vals[3]=maxStreak;
	char* labels[4];
	labels[0]="Played";
	labels[1]="Win %";
	labels[2]="Streak";
	labels[3]="Max";
	for(int i=0;i<4;i++)
	{
		float cx=x+w*(i+0.5)/4;
		char s[16];
		snprintf(s,15,"%i",vals[i]);
		TextC(s,cx,y+78,3.4,COL_TEXT,4.);
		TextC(labels[i],cx,y+112,1.4,0xffb0b0b0,1.8);
	}

	TextC("GUESS DISTRIBUTION",G_SCREEN_WIDTH/2,y+150,1.8,COL_TEXT,2.4);
	int mx=1;
	for(int i=0;i<6;i++)if(dist[i]>mx)mx=dist[i];
	for(int i=0;i<6;i++)
	{
		float by=y+172+i*24;
		char s[16];
		snprintf(s,15,"%i",i+1);
		TextC(s,x+30,by+10,1.6,COL_TEXT,2.2);
		float bw=24+(w-90-24)*dist[i]/mx;
		int bc=COL_ABSENT;
		if(i==lastWinRow)bc=COL_CORRECT;
		FillBox(x+46,by,bw,20,2,bc);
		snprintf(s,15,"%i",dist[i]);
		float tw=(TextLen(s)*6-2)*1.5;
		TextC(s,x+46+bw-tw/2-7,by+10,1.5,COL_TEXT,2.);
	}
	float blink=0.6+0.4*sin(T*4);
	int bi=int(blink*255);
	TextC("ENTER or click - new word",G_SCREEN_WIDTH/2,y+h-28,1.7,(bi<<24)|0xffffff,2.2);
}

void Draw(int mx, int my, int mb)
{
	float T=Time();
	g.rgba32(COL_BG);
	g.FillRT();

	TextC("WORDY",G_SCREEN_WIDTH/2,26,3.0,COL_TEXT,4.);
	FillBox(0,48,G_SCREEN_WIDTH,1,0,0xff3a3a3c);

	DrawBoard(T);
	DrawKeyboard(mx,my,mb);
	DrawMessage(T);
	if(state!=0)DrawStats(T);
}

// ---------------------------------------------------------------- main

int main()
{
	pfont=(CFont*)malloc(sizeof(CFont));
	CFontMaker fm;
	fm.Init(*pfont,6);
	MakeLetters(fm);

	for(int i=0;i<6;i++)dist[i]=0;
	LayoutKeys();

	int timeSeed=Time()*1e9;
	seed+=timeSeed;
	irand(seed);
	NewGame();

	int prevMB=0;
	while(true)
	{
		float T=Time();
		if(revealRow>=0&&T-revealT0>FLIP_STEP*4+FLIP_TIME)RowRevealed();

		int key;
		int press;
		while(GetKeyEvent(key,press))
		{
			if(press>0)OnKey(key);
		}

		int mx;
		int my;
		int mb;
		GetMouseState(mx,my,mb);
		if((mb&1)&&!(prevMB&1))
		{
			if(state!=0)OnKey(KEY_ENTER);
			else
			{
				int k=KeyAt(mx,my);
				if(k>=0)OnKey(k);
			}
		}
		prevMB=mb;

		Draw(mx,my,mb);
		Present();
	}
	return 0;
}
