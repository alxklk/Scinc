// wordy.cpp - a Wordle-style word game.
//
// Guess the hidden five-letter word in six tries. After each guess the
// tiles flip and show how close it was:
//   green  - right letter in the right place
//   yellow - the letter is in the word, but somewhere else
//   grey   - the letter is not in the word (or not that many times)
// The on-screen keyboard keeps the best colour found for every letter.
// Below the board a counter shows how many answers still fit every clue.
// The panel on the right explains each guessed word, looked up online (see
// "word meanings" below); click a guessed row to see its meaning again.
//
// Guesses must be real words from all5letterw.h (12972 words); the secret
// word is picked from the 2315 original Wordle answers in answers.h.
//
// Type letters (or click the on-screen keys), Backspace deletes, Enter
// submits. The header holds two buttons: GIVE UP (click twice) reveals the
// word, and once a game is over it turns into NEW GAME (Enter does the same);
// STATISTICS opens the statistics, Esc or BACK returns to the board. After a
// game every guessed row can still be clicked to read its meaning.
//
// The meanings need ../../claude/net/http_dl.so (build it with
// ../../claude/net/build_so.sh); without a network the panel says so and
// the game plays as usual.

#include <stdio.h>

#define HTTP_DL "../../claude/net/http_dl"
#include "../../claude/net/http.h"

#define G_SCREEN_WIDTH 820
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
#define KEY_ESC   1000

// play area on the left, meaning panel on the right
#define PLAY_W 500
#define PANEL_X 504
#define PANEL_Y 64
#define PANEL_W 300
#define PANEL_H 640

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

int remaining=0;   // answers still consistent with every revealed row

int statsOpen=0;   // statistics window shown over the board
float statsT0=-10;
float giveUpT=-10; // first click on GIVE UP, waiting for the second

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

// ---------------------------------------------------------------- word meanings
//
// Every submitted guess is looked up while its tiles flip: first in the Free
// Dictionary API (dictionaryapi.dev), then - when it has no entry, which is
// common for rare words - in Wiktionary's REST API, whose definitions come
// as HTML. Requests run in the background (http.h), UpdateLookups() polls
// them once a frame. Slots 0..5 belong to the board rows, slot 6 to the
// secret word, looked up when a game is lost.

#define NSLOTS 7
#define MAX_ITEMS 8
#define ITEM_LEN 320

#define LK_NONE 0
#define LK_DICT 1       // waiting for dictionaryapi.dev
#define LK_WIKI 2       // waiting for wiktionary.org
#define LK_DONE 3
#define LK_NOTFOUND 4
#define LK_OFFLINE 5

int lkState[NSLOTS];
int lkReq[NSLOTS];
int lkSource[NSLOTS];               // LK_DICT or LK_WIKI, where the text came from
char lkWord[NSLOTS*6];
int nItems[NSLOTS];
int itemKind[NSLOTS*MAX_ITEMS];     // 0 part of speech, 1 definition
char itemText[NSLOTS*MAX_ITEMS*ITEM_LEN];
int shownSlot=-1;                   // slot shown in the panel, -1 none

char lkUrl[160];
char lkPath[64];
char lkBuf[4096];

// append lkBuf to slot s, cut at a word with "..." when it is too long
void AddItem(int s, int kind)
{
	if(nItems[s]>=MAX_ITEMS)return;
	int base=(s*MAX_ITEMS+nItems[s])*ITEM_LEN;
	int n=0;
	while(lkBuf[n]!=0&&n<ITEM_LEN-1)n++;
	if(lkBuf[n]!=0)
	{
		n=ITEM_LEN-4;
		while(n>0&&lkBuf[n]!=' ')n--;
		lkBuf[n]='.';
		lkBuf[n+1]='.';
		lkBuf[n+2]='.';
		n+=3;
	}
	for(int i=0;i<n;i++)
	{
		int ch=lkBuf[i];
		if(kind==0&&ch>='A'&&ch<='Z')ch=ch-'A'+'a';
		itemText[base+i]=ch;
	}
	itemText[base+n]=0;
	itemKind[s*MAX_ITEMS+nItems[s]]=kind;
	nItems[s]++;
}

void StartLookup(int s, int source)
{
	char w[8];
	for(int i=0;i<6;i++)w[i]=lkWord[s*6+i];
	if(source==LK_DICT)snprintf(lkUrl,159,"https://api.dictionaryapi.dev/api/v2/entries/en/%s",w);
	else snprintf(lkUrl,159,"https://en.wiktionary.org/api/rest_v1/page/definition/%s",w);
	lkReq[s]=HttpGet(lkUrl);
	lkState[s]=source;
	if(lkReq[s]==0)lkState[s]=LK_OFFLINE;   // http_dl found no libcurl
}

// look up the five letters in word[] (ints 'a'..'z') into slot s
void Lookup(int s, int* word, int off)
{
	if(lkReq[s]>0)HttpFree(lkReq[s]);
	for(int i=0;i<5;i++)lkWord[s*6+i]=word[off+i];
	lkWord[s*6+5]=0;
	nItems[s]=0;
	StartLookup(s,LK_DICT);
}

// dictionaryapi.dev: [{"meanings":[{"partOfSpeech":..,"definitions":[{"definition":..}]}]}]
void ParseDict(int s, int id)
{
	int nm=HttpJsonCount(id,"0.meanings");
	for(int m=0;m<nm&&m<3;m++)
	{
		snprintf(lkPath,63,"0.meanings.%i.partOfSpeech",m);
		HttpJson(id,lkPath,lkBuf,4096);
		AddItem(s,0);
		snprintf(lkPath,63,"0.meanings.%i.definitions",m);
		int nd=HttpJsonCount(id,lkPath);
		for(int d=0;d<nd&&d<2;d++)
		{
			snprintf(lkPath,63,"0.meanings.%i.definitions.%i.definition",m,d);
			HttpJson(id,lkPath,lkBuf,4096);
			Utf8ToAscii(lkBuf);
			AddItem(s,1);
		}
	}
}

// wiktionary: {"en":[{"partOfSpeech":..,"definitions":[{"definition":"<html>"}]}]}
// some definitions are empty once the markup is gone; a part of speech is
// listed only when it has a real definition
void WikiDef(int id, int m, int d)
{
	snprintf(lkPath,63,"en.%i.definitions.%i.definition",m,d);
	HttpJson(id,lkPath,lkBuf,4096);
	HtmlToText(lkBuf);
	Utf8ToAscii(lkBuf);
}

void ParseWiki(int s, int id)
{
	int nm=HttpJsonCount(id,"en");
	for(int m=0;m<nm&&m<3;m++)
	{
		snprintf(lkPath,63,"en.%i.definitions",m);
		int nd=HttpJsonCount(id,lkPath);
		int shown=0;
		for(int d=0;d<nd&&shown<2;d++)
		{
			WikiDef(id,m,d);
			if(lkBuf[0]==0)continue;
			if(shown==0)
			{
				snprintf(lkPath,63,"en.%i.partOfSpeech",m);
				HttpJson(id,lkPath,lkBuf,4096);
				AddItem(s,0);
				WikiDef(id,m,d);
			}
			AddItem(s,1);
			shown++;
		}
	}
}

void UpdateLookups()
{
	for(int s=0;s<NSLOTS;s++)
	{
		int k=lkState[s];
		if(k!=LK_DICT&&k!=LK_WIKI)continue;
		int st=HttpStatus(lkReq[s]);
		if(st==HTTP_PENDING)continue;
		if(st==200)
		{
			if(k==LK_DICT)ParseDict(s,lkReq[s]);
			else ParseWiki(s,lkReq[s]);
			lkSource[s]=k;
		}
		HttpFree(lkReq[s]);
		lkReq[s]=0;
		if(nItems[s]>0)lkState[s]=LK_DONE;
		else if(k==LK_DICT)StartLookup(s,LK_WIKI);   // after a network error too: one more try
		else if(st<0)lkState[s]=LK_OFFLINE;
		else lkState[s]=LK_NOTFOUND;
	}
}

void ResetLookups()
{
	for(int s=0;s<NSLOTS;s++)
	{
		if(lkReq[s]>0)HttpFree(lkReq[s]);
		lkReq[s]=0;
		lkState[s]=LK_NONE;
		nItems[s]=0;
	}
	shownSlot=-1;
}

void NewGame()
{
	ResetLookups();
	int n=(irand(seed)>>4)%N_ANSWERS;
	char* w=all5letter[answers[n]];
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
	remaining=N_ANSWERS;
	statsOpen=0;
	giveUpT=-10;
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

// 1 if secret word w would have given row r exactly the marks it got
int Consistent(int r, char* w)
{
	int left[26];
	for(int i=0;i<26;i++)left[i]=0;
	for(int i=0;i<5;i++)
	{
		int g=guess[r*5+i];
		int m=mark[r*5+i];
		if(g==w[i])
		{
			if(m!=3)return 0;
		}
		else
		{
			if(m==3)return 0;
			left[w[i]-'a']++;
		}
	}
	for(int i=0;i<5;i++)
	{
		int m=mark[r*5+i];
		if(m==3)continue;
		int c=guess[r*5+i]-'a';
		int p=1;
		if(left[c]>0)
		{
			p=2;
			left[c]--;
		}
		if(p!=m)return 0;
	}
	return 1;
}

// count the answers that agree with all rows revealed so far (0..last)
void CountRemaining(int last)
{
	int n=0;
	for(int k=0;k<N_ANSWERS;k++)
	{
		char* w=all5letter[answers[k]];
		int ok=1;
		for(int r=0;r<=last&&ok;r++)ok=Consistent(r,w);
		if(ok)n++;
	}
	remaining=n;
}

// out of guesses, or gave up: show the word and look up its meaning
void Lose()
{
	state=2;
	endT=Time();
	played++;
	streak=0;
	char s[8];
	for(int i=0;i<5;i++)s[i]=secret[i]-'a'+'A';
	s[5]=0;
	Message(s,-1);
	Lookup(6,secret,0);
	shownSlot=6;
}

// called when the flip animation of a row has finished
void RowRevealed()
{
	int r=revealRow;
	revealRow=-1;
	shownSlot=r;
	int greens=0;
	for(int i=0;i<5;i++)
	{
		int c=guess[r*5+i]-'a';
		if(mark[r*5+i]>keyMark[c])keyMark[c]=mark[r*5+i];
		if(mark[r*5+i]==3)greens++;
	}
	CountRemaining(r);
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
	if(row>=6)Lose();
}

// header button: GIVE UP needs a second click within 3 s, NEW GAME does not
void NewGameButton()
{
	if(state!=0)
	{
		NewGame();
		return;
	}
	if(revealRow>=0)return;
	if(Time()-giveUpT<3)
	{
		giveUpT=-10;
		for(int i=0;i<5;i++)guess[row*5+i]=0;   // drop a half-typed guess
		col=0;
		Lose();
		return;
	}
	giveUpT=Time();
	Message("Click again to give up",3);
}

void OnKey(int k)
{
	if(statsOpen)
	{
		if(k==KEY_ESC)statsOpen=0;
		return;
	}
	if(state!=0)
	{
		if(Time()-endT>0.8&&(k==KEY_ENTER||k==KEY_ENTER2))NewGame();
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
			Lookup(row,guess,row*5);
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
	float x=(PLAY_W-total)/2;
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

// g.width(r,k) gives a line whose opacity is k*(1-d/r) at distance d from
// the centre (clamped to 0..1): r is the half-width, r/k the soft edge.
// k<=1 is all edge and no core and looks blurry; k=2 keeps a solid core with
// a half-unit soft edge. r grows with the text size: small text clogs up
// above r=1.5, the big tile letters want about 2.
void Stroke(float r, float k)
{
	g.width(r,k);
	g.stroke();
}

int TextLen(char* s)
{
	int n=0;
	while(s[n]!=0)n++;
	return n;
}

// stroke-font text; the glyphs are 4 units wide and 9 tall over a 6 unit advance
void Text(char* s, float x, float y, float sx, float sy, int c, float r, float k)
{
	g.clear();
	DrawText(g,*pfont,s,0,x,y,sx,sy);
	g.fin();
	g.rgba32(c);
	Stroke(r,k);
}

void TextC(char* s, float cx, float cy, float sc, int c, float r, float k)
{
	float tw=(TextLen(s)*6-2)*sc;
	Text(s,cx-tw/2,cy+4.5*sc,sc,sc,c,r,k);
}

void Letter(int ch, float cx, float cy, float sx, float sy, int c, float r, float k)
{
	g.clear();
	DrawGlyph(g,*pfont,ch,cx-2*sx,cy+4.5*sy,sx,sy);
	g.fin();
	g.rgba32(c);
	Stroke(r,k);
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
				Letter(guess[i]-'a'+'A',cx,cy,3.6*sc,3.4*sc*sy,COL_TEXT,2.,2.);
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
	Stroke(1.5,2.);
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
		if(k==KEY_ENTER)TextC("ENTER",cx,cy,1.5,COL_TEXT,1.2,2.);
		else if(k==KEY_BACK)DrawBackArrow(cx,cy,COL_TEXT);
		else Letter(k-'a'+'A',cx,cy,2.2,2.0,COL_TEXT,1.6,2.);
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
	FillBox(PLAY_W/2-w/2,BOARD_Y-36,w,h,6,(ai<<24)|0xf0f0f0);
	TextC(msg,PLAY_W/2,BOARD_Y-36+h/2,sc,(ai<<24)|0x121213,1.6,2.);
}

void DrawRemaining()
{
	if(state!=0)
	{
		TextC("Click a row to see its meaning",PLAY_W/2,503,1.5,0xffb0b0b0,1.2,2.);
		return;
	}
	char s[32];
	if(remaining==1)snprintf(s,31,"1 possible word left");
	else snprintf(s,31,"%i possible words left",remaining);
	TextC(s,PLAY_W/2,503,1.5,0xffb0b0b0,1.2,2.);
}

// ---------------------------------------------------------------- meaning panel

#define PANEL_TEXT 0xffd8d8d8
#define PANEL_DIM  0xff818384
#define DEF_SC 1.4
#define DEF_LINE 19.0

float panelBottom=PANEL_Y+PANEL_H-34;   // text below this is not drawn

// itemText from base, word-wrapped to maxc characters a line; returns the y
// of the next line
float DrawWrapped(int base, float x, float y, int maxc, int c)
{
	char ln[64];
	int i=base;
	while(itemText[i]!=0)
	{
		if(y>panelBottom)return y;
		int n=0;
		int brk=-1;
		while(n<maxc&&itemText[i+n]!=0)
		{
			if(itemText[i+n]==' ')brk=n;
			n++;
		}
		// break at the last space, unless the line ends right here anyway;
		// a word longer than a whole line is simply cut
		if(itemText[i+n]!=0&&itemText[i+n]!=' '&&brk>0)n=brk;
		for(int k=0;k<n;k++)ln[k]=itemText[i+k];
		ln[n]=0;
		Text(ln,x,y,DEF_SC,DEF_SC,c,1.2,2.);
		y+=DEF_LINE;
		i+=n;
		while(itemText[i]==' ')i++;
	}
	return y;
}

void PanelLine(char* s, float y, int c)
{
	Text(s,PANEL_X+16,y,DEF_SC,DEF_SC,c,1.2,2.);
}

void DrawPanel(float T)
{
	FillBox(PANEL_X,PANEL_Y,PANEL_W,PANEL_H,8,0xff1b1b1d);
	float cx=PANEL_X+PANEL_W/2;
	if(shownSlot<0)
	{
		TextC("MEANING",cx,PANEL_Y+36,2.2,PANEL_DIM,1.6,2.);
		PanelLine("Every word you guess is looked",PANEL_Y+90,PANEL_DIM);
		PanelLine("up in an online dictionary and",PANEL_Y+90+DEF_LINE,PANEL_DIM);
		PanelLine("explained here.",PANEL_Y+90+2*DEF_LINE,PANEL_DIM);
		PanelLine("Click a guessed row to see its",PANEL_Y+90+4*DEF_LINE,PANEL_DIM);
		PanelLine("meaning again.",PANEL_Y+90+5*DEF_LINE,PANEL_DIM);
		return;
	}
	int s=shownSlot;
	char w[8];
	for(int i=0;i<5;i++)w[i]=lkWord[s*6+i]-'a'+'A';
	w[5]=0;
	float y=PANEL_Y+20;
	if(s==6)
	{
		TextC("the hidden word was",cx,y,1.3,PANEL_DIM,1.2,2.);
		y+=30.0;
	}
	else y+=16.0;
	TextC(w,cx,y,3.4,COL_TEXT,2.,2.);
	y+=30.0;
	FillBox(PANEL_X+16,y,PANEL_W-32,1,0,0xff3a3a3c);
	y+=30.0;

	int k=lkState[s];
	if(k==LK_DICT||k==LK_WIKI)
	{
		char dots[8];
		int n=int(T*3)%4;
		for(int i=0;i<n;i++)dots[i]='.';
		dots[n]=0;
		char t[40];
		snprintf(t,39,"Looking it up%s",dots);
		PanelLine(t,y,PANEL_DIM);
		return;
	}
	if(k==LK_NOTFOUND)
	{
		PanelLine("No definition found.",y,PANEL_DIM);
		return;
	}
	if(k==LK_OFFLINE)
	{
		PanelLine("Could not reach the dictionary.",y,PANEL_DIM);
		PanelLine("Is the network up?",y+DEF_LINE,PANEL_DIM);
		return;
	}
	if(k!=LK_DONE)return;

	int num=0;
	for(int i=0;i<nItems[s];i++)
	{
		if(y>panelBottom)break;
		int base=(s*MAX_ITEMS+i)*ITEM_LEN;
		if(itemKind[s*MAX_ITEMS+i]==0)
		{
			if(i>0)y+=8.0;
			y=DrawWrapped(base,PANEL_X+16,y,31,COL_PRESENT)+4;
			num=0;
		}
		else
		{
			num++;
			char ns[8];
			snprintf(ns,7,"%i.",num);
			Text(ns,PANEL_X+16,y,DEF_SC,DEF_SC,PANEL_DIM,1.2,2.);
			y=DrawWrapped(base,PANEL_X+38,y,29,PANEL_TEXT)+4;
		}
	}
	if(lkSource[s]==LK_DICT)TextC("from dictionaryapi.dev",cx,PANEL_Y+PANEL_H-16,1.2,0xff5a5a5c,1.1,2.);
	else TextC("from wiktionary.org",cx,PANEL_Y+PANEL_H-16,1.2,0xff5a5a5c,1.1,2.);
}

// small pointer from the row whose meaning is shown towards the panel
void DrawRowMarker()
{
	if(shownSlot<0||shownSlot>5)return;
	float x=BOARD_X+5*(TILE+GAP)+4;
	float y=BOARD_Y+shownSlot*(TILE+GAP)+TILE/2;
	g.clear();
	g.M(x,y-8);
	g.L(x+10,y);
	g.L(x,y+8);
	g.close();
	g.fin();
	g.rgba32(PANEL_DIM);
	g.fill1();
}

// the guessed (already revealed) row under the mouse, or -1
int RowAt(int mx, int my)
{
	if(mx<BOARD_X||mx>=BOARD_X+5*(TILE+GAP))return -1;
	if(my<BOARD_Y)return -1;
	int r=(my-BOARD_Y)/(TILE+GAP);
	if(r>5||mark[r*5]==0||r==revealRow)return -1;
	return r;
}

// ---------------------------------------------------------------- buttons and statistics

#define BTN_Y 10
#define BTN_H 28
#define BTN_W 100
#define NEWBTN_X 12
#define STATBTN_X (PLAY_W-12-BTN_W)

// statistics window and its buttons
#define ST_X 40
#define ST_Y 150
#define ST_W (PLAY_W-80)
#define ST_H 380
#define ST_BTN_Y (ST_Y+ST_H-52)
#define ST_BTN_W 160
#define ST_BTN_H 34
#define ST_CLOSE 28

int Over(float x, float y, float w, float h, int mx, int my)
{
	return mx>=x&&mx<x+w&&my>=y&&my<y+h;
}

void Button(char* s, float x, float y, float w, float h, int c, int mx, int my)
{
	FillBox(x,y,w,h,5,c);
	if(Over(x,y,w,h,mx,my))FillBox(x,y,w,h,5,0x20ffffff);
	TextC(s,x+w/2,y+h/2,1.4,COL_TEXT,1.2,2.);
}

void DrawHeaderButtons(int mx, int my)
{
	if(state!=0)Button("NEW GAME",NEWBTN_X,BTN_Y,BTN_W,BTN_H,COL_CORRECT,mx,my);
	else if(Time()-giveUpT<3)Button("SURE?",NEWBTN_X,BTN_Y,BTN_W,BTN_H,0xffa83a32,mx,my);
	else Button("GIVE UP",NEWBTN_X,BTN_Y,BTN_W,BTN_H,COL_EMPTY,mx,my);
	Button("STATISTICS",STATBTN_X,BTN_Y,BTN_W,BTN_H,COL_EMPTY,mx,my);
}

// BACK sits in the middle while a game is on, beside NEW GAME after it
float StatsBackX()
{
	if(state!=0)return ST_X+30;
	return PLAY_W/2-ST_BTN_W/2;
}

float StatsNewX()
{
	return ST_X+ST_W-30-ST_BTN_W;
}

void OpenStats()
{
	statsOpen=1;
	statsT0=Time();
}

void StatsClick(int mx, int my)
{
	if(!Over(ST_X,ST_Y,ST_W,ST_H,mx,my))statsOpen=0;
	else if(Over(ST_X+ST_W-ST_CLOSE-8,ST_Y+8,ST_CLOSE,ST_CLOSE,mx,my))statsOpen=0;
	else if(Over(StatsBackX(),ST_BTN_Y,ST_BTN_W,ST_BTN_H,mx,my))statsOpen=0;
	else if(state!=0&&Over(StatsNewX(),ST_BTN_Y,ST_BTN_W,ST_BTN_H,mx,my))NewGame();
}

void DrawStats(float T, int mx, int my)
{
	float a=Clamp01((T-statsT0)/0.2);
	int ai=int(a*255);
	float x=ST_X;
	float y=ST_Y;
	float w=ST_W;
	float h=ST_H;
	FillBox(0,49,PLAY_W,G_SCREEN_HEIGHT-49,0,(int(a*150)<<24)|0x000000);
	FillBox(x,y+(1-a)*30,w,h,10,(ai<<24)|0x1a1a1b);
	if(a<1)return;
	FrameBox(x,y,w,h,1,0xff3a3a3c);
	TextC("STATISTICS",PLAY_W/2,y+30,2.2,COL_TEXT,1.6,2.);

	// close cross
	float bx=x+w-ST_CLOSE-8;
	float by0=y+8;
	if(Over(bx,by0,ST_CLOSE,ST_CLOSE,mx,my))FillBox(bx,by0,ST_CLOSE,ST_CLOSE,5,0x20ffffff);
	g.clear();
	g.M(bx+9,by0+9);g.L(bx+ST_CLOSE-9,by0+ST_CLOSE-9);
	g.M(bx+ST_CLOSE-9,by0+9);g.L(bx+9,by0+ST_CLOSE-9);
	g.fin();
	g.rgba32(0xffb0b0b0);
	Stroke(1.3,2.);

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
		TextC(s,cx,y+78,3.4,COL_TEXT,2.,2.);
		TextC(labels[i],cx,y+112,1.4,0xffb0b0b0,1.2,2.);
	}

	TextC("GUESS DISTRIBUTION",PLAY_W/2,y+150,1.8,COL_TEXT,1.4,2.);
	int mx1=1;
	for(int i=0;i<6;i++)if(dist[i]>mx1)mx1=dist[i];
	for(int i=0;i<6;i++)
	{
		float by=y+172+i*24;
		char s[16];
		snprintf(s,15,"%i",i+1);
		TextC(s,x+30,by+10,1.6,COL_TEXT,1.2,2.);
		float bw=24+(w-90-24)*dist[i]/mx1;
		int bc=COL_ABSENT;
		if(i==lastWinRow)bc=COL_CORRECT;
		FillBox(x+46,by,bw,20,2,bc);
		snprintf(s,15,"%i",dist[i]);
		float tw=(TextLen(s)*6-2)*1.5;
		TextC(s,x+46+bw-tw/2-7,by+10,1.5,COL_TEXT,1.2,2.);
	}

	Button("BACK",StatsBackX(),ST_BTN_Y,ST_BTN_W,ST_BTN_H,COL_EMPTY,mx,my);
	if(state!=0)Button("NEW GAME",StatsNewX(),ST_BTN_Y,ST_BTN_W,ST_BTN_H,COL_CORRECT,mx,my);
}

// on a loss the banner with the hidden word is shown at the top of the board
int OverAnswer(int mx, int my)
{
	return state==2&&Over(PLAY_W/2-80,BOARD_Y-36,160,44,mx,my);
}

// a click on the board area, the header or the keyboard
void OnClick(int mx, int my)
{
	if(statsOpen)
	{
		StatsClick(mx,my);
		return;
	}
	if(Over(NEWBTN_X,BTN_Y,BTN_W,BTN_H,mx,my))
	{
		NewGameButton();
		return;
	}
	if(Over(STATBTN_X,BTN_Y,BTN_W,BTN_H,mx,my))
	{
		OpenStats();
		return;
	}
	if(OverAnswer(mx,my))
	{
		shownSlot=6;
		return;
	}
	int r=RowAt(mx,my);
	if(r>=0)
	{
		shownSlot=r;
		return;
	}
	int k=KeyAt(mx,my);
	if(k>=0)OnKey(k);
}

void Draw(int mx, int my, int mb)
{
	float T=Time();
	g.rgba32(COL_BG);
	g.FillRT();

	TextC("WORDY",PLAY_W/2,26,3.0,COL_TEXT,2.,2.);
	FillBox(0,48,G_SCREEN_WIDTH,1,0,0xff3a3a3c);
	DrawHeaderButtons(mx,my);

	DrawBoard(T);
	DrawRowMarker();
	DrawRemaining();
	DrawKeyboard(mx,my,mb);
	DrawPanel(T);
	DrawMessage(T);
	if(statsOpen)DrawStats(T,mx,my);
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
		UpdateLookups();

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
		if((mb&1)&&!(prevMB&1))OnClick(mx,my);
		prevMB=mb;

		Draw(mx,my,mb);
		Present();
	}
	return 0;
}
