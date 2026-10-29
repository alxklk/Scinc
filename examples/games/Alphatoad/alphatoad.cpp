// alphatoad.cpp - feed the toads with words.
//
// Letter boxes float on a pond of M x N cells. Each box shows a letter and a
// number: how many turns are left before it turns to stone. Stones stay for
// good and can't be used any more.
//
// A turn:
//   1. two or three new boxes land on random free cells; their letters are
//      drawn by English letter frequency (E often, Z rarely), their numbers
//      are random (2..9);
//   2. you spell a word of 2+ letters from any boxes on the pond, in any
//      order: click them, or type the letters (typing picks the matching box
//      with the fewest turns left). The letters gather into the word under
//      the pond. FEED (Enter) checks it against the dictionary; a real word
//      is eaten, scores, and the panel on the right explains it. SKIP TURN
//      passes.
// Then every number goes down by one, and boxes that reach 0 turn to stone.
//
// A word scores the sum of the numbers on its boxes - fresh boxes are worth
// more, old ones are about to be lost. Up to three toads sit under the pond,
// each with an ability that multiplies the score:
//   letter toad   - the numbers on boxes of its letter count x(level+1)
//   length toad   - a word of its length scores x(level+1)
//   double toad   - a word with a double letter (bOOk) scores x(level+1)
//
// The game ends when the gold score is reached (gold prize) or when every
// cell has turned to stone (silver prize if the silver score was reached on
// the way). A silver prize is one prize, gold is two; each prize raises a
// toad one level (up to 5) or claims a new toad, on the TOADS screen. Toads,
// unspent prizes and totals are kept in alphatoad.sav; every prize won
// makes the next targets a bit higher.
//
// A few words always score x2, whatever toads sit on the pads: TOAD, FROG,
// TADPOLE, POND, BULLFROG, TOADSTOOL, NEWT, POLLIWOG, POLLYWOG, AMPHIBIAN,
// CROAK, LEAP, RIBBIT, SCINC and their plurals.
//
// Keys: letters pick boxes, Backspace drops the last one, Esc clears the
// word, Enter feeds.
//
//   Scinc alphatoad.cpp [-x2] [COLSxROWS]
//
// -x2 renders at twice the resolution for HiDPI screens; COLSxROWS sets the
// size of the pond (4..10 each way, 7x7 by default), e.g. 9x6.
//
// The dictionary is words.txt: the public domain ENABLE word list, newer
// words from the SCOWL american-english list, and RIBBIT and SCINC. The
// meanings come from dictionaryapi.dev / wiktionary.org through
// ../../claude/net/http_dl.so (build it with ../../claude/net/build_so.sh);
// without a network the game plays as usual.

#pragma STACK_SIZE 4000000

#include <stdio.h>

#define HTTP_DL "../../claude/net/http_dl"
#include "../../claude/net/http.h"

#define G_SCREEN_WIDTH 860
#define G_SCREEN_HEIGHT 800
#define G_SCREEN_MODE 1   // XShm

#include "graphics.h"
#include "../../ws.h"
#include "../../graphic/font.h"
#include "../../graphic/letters.h"
#include "../../graphic/gtext.h"

#define M_PI 3.14159265358979

#define COL_BG      0xff0e2426
#define COL_WATER   0xff143436
#define COL_LINE    0xff27494a
#define COL_TEXT    0xfff2ecd8
#define COL_DIM     0xff86a09c
#define COL_FAINT   0xff52706c
#define COL_GOOD    0xff7fb83c
#define COL_BTN     0xff2a4a4a
#define COL_GO      0xff4f8a2a
#define COL_GOLD    0xffe8b84a
#define COL_SILVER  0xffc4ccd0

#define KEY_ENTER 4013
#define KEY_ENTER2 5013
#define KEY_BACK  4008
#define KEY_ESC   1000

#define PLAY_W 530
#define PANEL_X 546
#define PANEL_Y 64
#define PANEL_W 300
#define PANEL_H 724

// pond area; the cell size is fitted to the number of cells
#define POND_Y 108
#define POND_W 490
#define POND_H 432

#define TRAY_Y 580
#define BTN2_Y 618
#define BTN2_H 32
#define TOADS_Y 708
#define TOAD_K 0.6

Graph g;
CFont* pfont=0;

typedef char* pchar;

float SC=1;   // 2 with -x2: layout stays in logical units

int seed=5150243;

int irand()
{
	seed=(seed*1103515245+12345)&0x7ffffff;
	return seed>>4;
}

// ---------------------------------------------------------------- dictionary
//
// words.txt is read whole into dictBuf; dictOff[i] is where word i starts.
// The file is sorted, so a lookup is a binary search.

char* dictBuf;
int* dictOff;
int nDict=0;

void LoadDict()
{
	dictBuf=(char*)malloc(1900000);
	dictOff=(int*)malloc(4*190000);
	FILE* f=fopen("words.txt","rb");
	if(f==0)
	{
		printf("words.txt not found - run Alphatoad from its own folder\n");
		return;
	}
	int n=fread(dictBuf,1,1899999,f);
	fclose(f);
	dictBuf[n]=10;
	int st=0;
	for(int i=0;i<n;i++)
	{
		if(dictBuf[i]==10)
		{
			if(i>st&&nDict<190000)
			{
				dictOff[nDict]=st;
				nDict++;
			}
			st=i+1;
		}
	}
}

int DictCmp(char* w, int o)
{
	for(int i=0;i<40;i++)
	{
		int a=w[i];
		int b=dictBuf[o+i];
		if(b==10||b==13)b=0;
		if(a!=b)return a-b;
		if(a==0)return 0;
	}
	return 0;
}

int InDict(char* w)
{
	int lo=0;
	int hi=nDict-1;
	while(lo<=hi)
	{
		int m=(lo+hi)/2;
		int c=DictCmp(w,dictOff[m]);
		if(c==0)return 1;
		if(c<0)hi=m-1;
		else lo=m+1;
	}
	return 0;
}

// ---------------------------------------------------------------- letters

// how often each letter occurs in English text, per 10000
int freq[26]={817,149,278,425,1270,223,202,609,697,15,77,403,241,675,751,193,10,599,633,906,276,98,236,15,197,7};

int IsVowel(int c)
{
	return c=='a'||c=='e'||c=='i'||c=='o'||c=='u';
}

int RandLetter(int vowelsOnly)
{
	int sum=0;
	for(int i=0;i<26;i++)
	{
		if(vowelsOnly&&!IsVowel('a'+i))continue;
		sum+=freq[i];
	}
	int r=irand()%sum;
	for(int i=0;i<26;i++)
	{
		if(vowelsOnly&&!IsVowel('a'+i))continue;
		r-=freq[i];
		if(r<0)return 'a'+i;
	}
	return 'e';
}

// ---------------------------------------------------------------- toads
//
// toadType/Param/Level: the toads the player owns (up to 3), kept in the
// save file. A toad's multiplier is level+1.

#define TT_LETTER 0
#define TT_LENGTH 1
#define TT_DOUBLE 2
#define MAX_LEVEL 5

int nToads=0;
int toadType[3];
int toadParam[3];
int toadLevel[3];

// new toads offered on the TOADS screen
int offerType[3];
int offerParam[3];

char* toadLetters="etaoinsrhldcumpgyfwb";   // letters a letter toad may love

int ToadMult(int i)
{
	return 1+toadLevel[i];
}

int ToadOwned(int t, int p)
{
	for(int i=0;i<nToads;i++)if(toadType[i]==t&&toadParam[i]==p)return 1;
	return 0;
}

int Offered(int t, int p, int n)
{
	for(int i=0;i<n;i++)if(offerType[i]==t&&offerParam[i]==p)return 1;
	return 0;
}

// a random ability nobody has and that isn't offered yet
void RandAbility(int n, int& t, int& p)
{
	for(int tries=0;tries<100;tries++)
	{
		int r=irand()%10;
		t=TT_LETTER;
		if(r>=5)t=TT_LENGTH;
		if(r>=8)t=TT_DOUBLE;
		p=0;
		if(t==TT_LETTER)p=toadLetters[irand()%20];
		if(t==TT_LENGTH)p=3+irand()%4;
		if(!ToadOwned(t,p)&&!Offered(t,p,n))return;
	}
}

void MakeOffers()
{
	for(int i=0;i<3;i++)
	{
		offerType[i]=-1;
		offerParam[i]=-1;
	}
	for(int i=0;i<3;i++)
	{
		int t;
		int p;
		RandAbility(i,t,p);
		offerType[i]=t;
		offerParam[i]=p;
	}
}

void AbilityText(int t, int p, char* s)
{
	if(t==TT_LETTER)snprintf(s,31,"%c boxes",p-'a'+'A');
	else if(t==TT_LENGTH)snprintf(s,31,"%i-letter words",p);
	else snprintf(s,31,"double letters");
}

// ---------------------------------------------------------------- the pond

#define MAXC 100
#define MAXW 15

#define K_EMPTY 0
#define K_BOX 1
#define K_STONE 2

int cols=7;
int rows=7;
int ncell=49;
float CS=62;     // cell size
float BX=0;
float BY=0;

int kind[MAXC];
int let[MAXC];
int tim[MAXC];
float bornT[MAXC];
float stoneT[MAXC];
float tickT=-10;

int sel[MAXW];   // the word being spelled, as cell indices
int nsel=0;
char word[MAXW+1];
int wordOk=0;
int wordSum=0;   // the numbers on its boxes, letter toads applied
int wordPts=0;
int trig[3];     // which toads the word would please
int toadWord=0;  // one of the easter egg words below

#define PH_INPUT 0
#define PH_EAT 1
#define PH_TICK 2
#define PH_SPAWN 3
#define PH_OVER 4

#define BOX_EAT 0.24
#define T_TICK 0.5
#define T_SPAWN 0.35

int phase=PH_INPUT;
float phaseT0=0;

// boxes on their way into a toad
int eatN=0;
int eatToad=0;
int eatLet[MAXW];
int eatNum[MAXW];
float eatX[MAXW];
float eatY[MAXW];
int eatTrig[3];

int score=0;
int turn=1;
int nWords=0;
int silverAt=0;
int goldAt=0;
int prize=0;            // 0 none yet, 1 silver, 2 gold
char gBest[MAXW+1];     // best word of this game
int gBestPts=0;
int newBest=0;

float popT=-10;         // "+points" over the toad
int popPts=0;

char msg[64];
float msgT0=-10;
float msgDur=0;
float shakeT0=-10;
float newGameT=-10;     // first click on NEW GAME, waiting for the second
int cardShown=0;
float cardT0=-10;

// totals, kept in alphatoad.sav
int best=0;
int games=0;
int totalWords=0;
int silvers=0;
int golds=0;
int prizes=0;           // prizes not spent yet
char longest[MAXW+1];
char bestWord[MAXW+1];
int bestWordPts=0;

int statsOpen=0;
float statsT0=-10;
int toadsOpen=0;
float toadsT0=-10;

void CopyStr(char* d, char* s, int max)
{
	int i=0;
	while(s[i]!=0&&i<max)
	{
		d[i]=s[i];
		i++;
	}
	d[i]=0;
}

int TextLen(char* s)
{
	int n=0;
	while(s[n]!=0)n++;
	return n;
}

void Upper(char* d, char* s)
{
	int i=0;
	while(s[i]!=0&&i<MAXW)
	{
		d[i]=s[i]-'a'+'A';
		i++;
	}
	d[i]=0;
}

void Message(char* s, float dur)
{
	CopyStr(msg,s,63);
	msgT0=Time();
	msgDur=dur;
}

void Layout()
{
	ncell=cols*rows;
	CS=POND_W/cols;
	if(POND_H/rows<CS)CS=POND_H/rows;
	if(CS>72)CS=72;
	BX=(PLAY_W-cols*CS)/2;
	BY=POND_Y+(POND_H-rows*CS)/2;
}

// silver is SILVER_PER_CELL points per cell of the pond, gold twice that;
// the targets grow by a quarter with every prize won before
#define SILVER_PER_CELL 5
void Targets()
{
	float f=1+0.25*(silvers+golds);
	silverAt=int(SILVER_PER_CELL*ncell*f/10+0.5)*10;
	goldAt=silverAt*2;
}

float CellX(int i)
{
	return BX+(i%cols)*CS+CS/2;
}

float CellY(int i)
{
	return BY+(i/cols)*CS+CS/2;
}

int SelIndex(int c)
{
	for(int i=0;i<nsel;i++)if(sel[i]==c)return i;
	return -1;
}

int HasDouble(char* w)
{
	for(int i=0;w[i]!=0&&w[i+1]!=0;i++)if(w[i]==w[i+1])return 1;
	return 0;
}

int StrEq(char* a, char* b)
{
	int i=0;
	while(a[i]!=0&&a[i]==b[i])i++;
	return a[i]==b[i];
}

// the toads' favourite words (and their plurals) always score x2
int IsToadWord(char* w)
{
	int n=TextLen(w);
	char b[MAXW+1];
	CopyStr(b,w,MAXW);
	if(n>4&&b[n-1]=='s')b[n-1]=0;
	return StrEq(b,"toad")||StrEq(b,"frog")||StrEq(b,"tadpole")||StrEq(b,"pond")||
		StrEq(b,"bullfrog")||StrEq(b,"toadstool")||StrEq(b,"newt")||StrEq(b,"scinc")||
		StrEq(b,"polliwog")||StrEq(b,"pollywog")||StrEq(b,"amphibian")||StrEq(b,"croak")||
		StrEq(b,"leap")||StrEq(b,"ribbit");
}

// word[], whether it is in the dictionary, and what it would score
void UpdateWord()
{
	for(int k=0;k<3;k++)trig[k]=0;
	int sum=0;
	for(int i=0;i<nsel;i++)
	{
		int c=let[sel[i]];
		word[i]=c;
		int v=tim[sel[i]];
		for(int k=0;k<nToads;k++)
		{
			if(toadType[k]==TT_LETTER&&toadParam[k]==c)
			{
				v=v*ToadMult(k);
				trig[k]=1;
			}
		}
		sum+=v;
	}
	word[nsel]=0;
	wordOk=nsel>=2&&InDict(word);
	int mult=1;
	for(int k=0;k<nToads;k++)
	{
		if(toadType[k]==TT_LENGTH&&toadParam[k]==nsel)
		{
			mult=mult*ToadMult(k);
			trig[k]=1;
		}
		if(toadType[k]==TT_DOUBLE&&HasDouble(word))
		{
			mult=mult*ToadMult(k);
			trig[k]=1;
		}
	}
	toadWord=wordOk&&IsToadWord(word);
	if(toadWord)mult=mult*2;
	wordSum=sum;
	wordPts=sum*mult;
}

void ClearWord()
{
	nsel=0;
	UpdateWord();
}

// ---------------------------------------------------------------- saving

char* SAVE_FILE="alphatoad.sav";

// the k-th number on a line of the save file
int LineInt(char* s, int k)
{
	int i=0;
	for(int n=0;n<=k;n++)
	{
		while(s[i]!=0&&(s[i]<'0'||s[i]>'9'))i++;
		int v=0;
		while(s[i]>='0'&&s[i]<='9')
		{
			v=v*10+s[i]-'0';
			i++;
		}
		if(n==k)return v;
	}
	return 0;
}

// the word after the number: "12 kangaroo"
void LineWord(char* s, char* d)
{
	int i=0;
	while(s[i]>='0'&&s[i]<='9')i++;
	while(s[i]==' ')i++;
	int n=0;
	while(s[i]>='a'&&s[i]<='z'&&n<MAXW)
	{
		d[n]=s[i];
		n++;
		i++;
	}
	d[n]=0;
}

void LoadTotals()
{
	best=0;
	games=0;
	totalWords=0;
	silvers=0;
	golds=0;
	prizes=0;
	longest[0]=0;
	bestWord[0]=0;
	bestWordPts=0;
	nToads=0;
	FILE* f=fopen(SAVE_FILE,"rb");
	if(f==0)return;
	char s[64];
	int k=0;
	while(fgets(s,64,f))
	{
		int v=LineInt(s,0);
		if(k==0)best=v;
		if(k==1)games=v;
		if(k==2)totalWords=v;
		if(k==3)silvers=v;
		if(k==4)golds=v;
		if(k==5)prizes=v;
		if(k==6)
		{
			bestWordPts=v;
			LineWord(s,bestWord);
		}
		if(k==7)LineWord(s,longest);
		if(k>=8&&k<11&&nToads<3)
		{
			toadType[nToads]=v;
			toadParam[nToads]=LineInt(s,1);
			toadLevel[nToads]=LineInt(s,2);
			if(toadLevel[nToads]<1)toadLevel[nToads]=1;
			if(toadLevel[nToads]>MAX_LEVEL)toadLevel[nToads]=MAX_LEVEL;
			if(v>=0&&v<=2)nToads++;
		}
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
	snprintf(s,63,"%i words\n",totalWords);
	fputs(s,f);
	snprintf(s,63,"%i silver prizes\n",silvers);
	fputs(s,f);
	snprintf(s,63,"%i gold prizes\n",golds);
	fputs(s,f);
	snprintf(s,63,"%i prizes to spend\n",prizes);
	fputs(s,f);
	snprintf(s,63,"%i %s\n",bestWordPts,bestWord);
	fputs(s,f);
	snprintf(s,63,"%i %s\n",TextLen(longest),longest);
	fputs(s,f);
	for(int i=0;i<nToads;i++)
	{
		snprintf(s,63,"%i %i %i toad\n",toadType[i],toadParam[i],toadLevel[i]);
		fputs(s,f);
	}
	fclose(f);
}

// ---------------------------------------------------------------- word meanings
//
// As in Wordy: every eaten word is looked up in the Free Dictionary API
// (dictionaryapi.dev) and, when that has no entry, in Wiktionary. The last
// NSLOTS words keep their meanings; the list at the bottom of the panel
// shows them and a click brings one back.

#define NSLOTS 8
#define MAX_ITEMS 8
#define ITEM_LEN 320

#define LK_NONE 0
#define LK_DICT 1
#define LK_WIKI 2
#define LK_DONE 3
#define LK_NOTFOUND 4
#define LK_OFFLINE 5
#define LK_TOADS 6

int lkState[NSLOTS];
int lkReq[NSLOTS];
int lkSource[NSLOTS];
char lkWord[NSLOTS*16];
int lkPts[NSLOTS];
int nItems[NSLOTS];
int itemKind[NSLOTS*MAX_ITEMS];
char itemText[NSLOTS*MAX_ITEMS*ITEM_LEN];
int shownSlot=-1;
int nHist=0;          // words eaten this game; the newest is in slot (nHist-1)%NSLOTS

char lkUrl[160];
char lkPath[64];
char lkBuf[4096];

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
	char w[16];
	for(int i=0;i<16;i++)w[i]=lkWord[s*16+i];
	if(source==LK_DICT)snprintf(lkUrl,159,"https://api.dictionaryapi.dev/api/v2/entries/en/%s",w);
	else snprintf(lkUrl,159,"https://en.wiktionary.org/api/rest_v1/page/definition/%s",w);
	lkReq[s]=HttpGet(lkUrl);
	lkState[s]=source;
	if(lkReq[s]==0)lkState[s]=LK_OFFLINE;
}

void Lookup(int s, char* w)
{
	if(lkReq[s]>0)HttpFree(lkReq[s]);
	lkReq[s]=0;
	int i=0;
	while(w[i]!=0&&i<15)
	{
		lkWord[s*16+i]=w[i];
		i++;
	}
	lkWord[s*16+i]=0;
	nItems[s]=0;
	// no dictionary knows the scinc; the toads do
	if(StrEq(w,"scinc")||StrEq(w,"scincs"))
	{
		snprintf(lkBuf,4095,"noun");
		AddItem(s,0);
		snprintf(lkBuf,4095,"A small lizard of the family Scincidae, a skink.");
		AddItem(s,1);
		snprintf(lkBuf,4095,"A small interpreted C++ this pond runs on.");
		AddItem(s,1);
		lkSource[s]=LK_TOADS;
		lkState[s]=LK_DONE;
		return;
	}
	StartLookup(s,LK_DICT);
}

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

int UpdateLookups()
{
	int changed=0;
	for(int s=0;s<NSLOTS;s++)
	{
		int k=lkState[s];
		if(k!=LK_DICT&&k!=LK_WIKI)continue;
		int st=HttpStatus(lkReq[s]);
		if(st==HTTP_PENDING)continue;
		changed=1;
		if(st==200)
		{
			if(k==LK_DICT)ParseDict(s,lkReq[s]);
			else ParseWiki(s,lkReq[s]);
			lkSource[s]=k;
		}
		HttpFree(lkReq[s]);
		lkReq[s]=0;
		if(nItems[s]>0)lkState[s]=LK_DONE;
		else if(k==LK_DICT)StartLookup(s,LK_WIKI);
		else if(st<0)lkState[s]=LK_OFFLINE;
		else lkState[s]=LK_NOTFOUND;
	}
	return changed;
}

void ResetLookups()
{
	for(int s=0;s<NSLOTS;s++)
	{
		if(lkReq[s]>0)HttpFree(lkReq[s]);
		lkReq[s]=0;
		lkState[s]=LK_NONE;
		nItems[s]=0;
		lkPts[s]=0;
	}
	shownSlot=-1;
	nHist=0;
}

// ---------------------------------------------------------------- a turn

// part 1 of a turn: n new boxes on random free cells
void Spawn(int n, int tmin)
{
	int boxes=0;
	int vowels=0;
	for(int i=0;i<ncell;i++)
	{
		if(kind[i]!=K_BOX)continue;
		boxes++;
		if(IsVowel(let[i]))vowels++;
	}
	float T=Time();
	for(int k=0;k<n;k++)
	{
		int nfree=0;
		for(int i=0;i<ncell;i++)if(kind[i]==K_EMPTY)nfree++;
		if(nfree==0)break;
		int pick=irand()%nfree;
		int c=0;
		for(int i=0;i<ncell;i++)
		{
			if(kind[i]!=K_EMPTY)continue;
			if(pick==0)
			{
				c=i;
				break;
			}
			pick--;
		}
		// keep roughly one vowel in three on the pond, or few words fit
		int v=0;
		if(vowels*3<boxes+1&&irand()%10<6)v=1;
		kind[c]=K_BOX;
		let[c]=RandLetter(v);
		tim[c]=tmin+irand()%(10-tmin);
		bornT[c]=T+k*0.08;
		stoneT[c]=-10;
		boxes++;
		if(IsVowel(let[c]))vowels++;
	}
}

int AllStone()
{
	for(int i=0;i<ncell;i++)if(kind[i]!=K_STONE)return 0;
	return 1;
}

void GameOver()
{
	phase=PH_OVER;
	phaseT0=Time();
	cardShown=1;
	cardT0=Time();
	games++;
	if(prize==1)
	{
		silvers++;
		prizes++;
	}
	if(prize==2)
	{
		golds++;
		prizes+=2;
	}
	newBest=score>best;
	if(newBest)best=score;
	ClearWord();
	SaveTotals();
}

// the numbers go down; boxes at 0 turn to stone
void Tick()
{
	float T=Time();
	for(int i=0;i<ncell;i++)
	{
		if(kind[i]!=K_BOX)continue;
		tim[i]--;
		if(tim[i]<=0)
		{
			tim[i]=0;
			kind[i]=K_STONE;
			stoneT[i]=T;
		}
	}
	tickT=T;
	phase=PH_TICK;
	phaseT0=T;
	UpdateWord();
}

void NextTurn()
{
	if(AllStone())
	{
		GameOver();
		return;
	}
	turn++;
	Spawn(2+irand()%2,2);
	phase=PH_SPAWN;
	phaseT0=Time();
}

void Feed()
{
	if(phase!=PH_INPUT)return;
	if(nsel<2)
	{
		Message("Two letters at least",1.4);
		shakeT0=Time();
		return;
	}
	if(!wordOk)
	{
		char s[64];
		char up[MAXW+1];
		Upper(up,word);
		snprintf(s,63,"%s is not in the dictionary",up);
		Message(s,1.8);
		shakeT0=Time();
		return;
	}
	float T=Time();
	for(int i=0;i<nsel;i++)
	{
		int c=sel[i];
		eatLet[i]=let[c];
		eatNum[i]=tim[c];
		eatX[i]=CellX(c);
		eatY[i]=CellY(c);
		kind[c]=K_EMPTY;
	}
	eatN=nsel;
	// the pleased toad with the biggest multiplier eats; the first one if none
	eatToad=0;
	int bm=0;
	for(int k=0;k<nToads;k++)
	{
		eatTrig[k]=trig[k];
		if(trig[k]&&ToadMult(k)>bm)
		{
			bm=ToadMult(k);
			eatToad=k;
		}
	}
	score+=wordPts;
	nWords++;
	totalWords++;
	popPts=wordPts;
	if(wordPts>gBestPts)
	{
		gBestPts=wordPts;
		CopyStr(gBest,word,MAXW);
	}
	if(wordPts>bestWordPts)
	{
		bestWordPts=wordPts;
		CopyStr(bestWord,word,MAXW);
	}
	if(nsel>TextLen(longest))CopyStr(longest,word,MAXW);
	SaveTotals();

	int s=nHist%NSLOTS;
	Lookup(s,word);
	lkPts[s]=wordPts;
	nHist++;
	shownSlot=s;

	ClearWord();
	phase=PH_EAT;
	phaseT0=T;
}

void SkipTurn()
{
	if(phase!=PH_INPUT)return;
	ClearWord();
	Tick();
}

void NewGame()
{
	ResetLookups();
	for(int i=0;i<MAXC;i++)
	{
		kind[i]=K_EMPTY;
		let[i]='a';
		tim[i]=0;
		bornT[i]=-10;
		stoneT[i]=-10;
	}
	score=0;
	turn=1;
	nWords=0;
	prize=0;
	gBest[0]=0;
	gBestPts=0;
	newBest=0;
	eatN=0;
	popT=-10;
	msgDur=0;
	newGameT=-10;
	cardShown=0;
	statsOpen=0;
	toadsOpen=0;
	Targets();
	ClearWord();
	// a few boxes to begin with, and the first turn's new ones
	Spawn(3,4);
	Spawn(2+irand()%2,2);
	phase=PH_SPAWN;
	phaseT0=Time();
}

// called every frame: moves the turn along when an animation is done
int StepPhase(float T)
{
	float t=T-phaseT0;
	if(phase==PH_EAT&&t>eatN*BOX_EAT+0.15)
	{
		popT=T;
		eatN=0;
		if(score>=goldAt)
		{
			prize=2;
			GameOver();
			return 1;
		}
		if(prize==0&&score>=silverAt)
		{
			prize=1;
			char s[64];
			snprintf(s,63,"Silver prize! Gold at %i",goldAt);
			Message(s,3);
		}
		Tick();
		return 1;
	}
	if(phase==PH_TICK&&t>T_TICK)
	{
		NextTurn();
		return 1;
	}
	if(phase==PH_SPAWN&&t>T_SPAWN+0.2)
	{
		phase=PH_INPUT;
		return 1;
	}
	return 0;
}

// ---------------------------------------------------------------- spelling

void AddBox(int c)
{
	if(nsel>=MAXW)return;
	sel[nsel]=c;
	nsel++;
	UpdateWord();
}

// a typed letter takes the matching box with the fewest turns left
void TypeLetter(int ch)
{
	int bestC=-1;
	for(int i=0;i<ncell;i++)
	{
		if(kind[i]!=K_BOX||let[i]!=ch||SelIndex(i)>=0)continue;
		if(bestC<0||tim[i]<tim[bestC])bestC=i;
	}
	if(bestC<0)
	{
		shakeT0=Time();
		return;
	}
	AddBox(bestC);
}

void ClickCell(int c)
{
	if(kind[c]!=K_BOX)return;
	int k=SelIndex(c);
	if(k<0)AddBox(c);
	else
	{
		nsel=k;   // drop it and everything spelled after it
		UpdateWord();
	}
}

int CellAt(int mx, int my)
{
	if(mx<BX||my<BY||mx>=BX+cols*CS||my>=BY+rows*CS)return -1;
	int cx=(mx-BX)/CS;
	int cy=(my-BY)/CS;
	if(cx<0||cy<0||cx>=cols||cy>=rows)return -1;
	return cy*cols+cx;
}

// ---------------------------------------------------------------- the TOADS screen

void ToggleToads()
{
	toadsOpen=!toadsOpen;
	toadsT0=Time();
	if(toadsOpen)MakeOffers();
}

void RaiseToad(int i)
{
	if(prizes<=0||i>=nToads||toadLevel[i]>=MAX_LEVEL)return;
	toadLevel[i]++;
	prizes--;
	SaveTotals();
	UpdateWord();
}

void ClaimToad(int j)
{
	if(prizes<=0||nToads>=3)return;
	toadType[nToads]=offerType[j];
	toadParam[nToads]=offerParam[j];
	toadLevel[nToads]=1;
	nToads++;
	prizes--;
	MakeOffers();
	SaveTotals();
	UpdateWord();
}

// ---------------------------------------------------------------- drawing helpers

void Stroke(float r, float k)
{
	g.width(r*SC,k);
	g.stroke();
}

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

void TextR(char* s, float rx, float cy, float sc, int c, float r, float k)
{
	float tw=(TextLen(s)*6-2)*sc;
	Text(s,rx-tw,cy+4.5*sc,sc,sc,c,r,k);
}

void Letter(int ch, float cx, float cy, float sx, float sy, int c, float r, float k)
{
	g.clear();
	DrawGlyph(g,*pfont,ch,cx-2*sx,cy+4.5*sy,sx,sy);
	g.fin();
	g.rgba32(c);
	Stroke(r,k);
}

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

void EllipsePath(float cx, float cy, float rx, float ry)
{
	float k=0.5523;
	g.M(cx+rx,cy);
	g.C(cx+rx,cy+ry*k,cx+rx*k,cy+ry,cx,cy+ry);
	g.C(cx-rx*k,cy+ry,cx-rx,cy+ry*k,cx-rx,cy);
	g.C(cx-rx,cy-ry*k,cx-rx*k,cy-ry,cx,cy-ry);
	g.C(cx+rx*k,cy-ry,cx+rx,cy-ry*k,cx+rx,cy);
	g.close();
}

void Ellipse(float cx, float cy, float rx, float ry, int c)
{
	g.clear();
	EllipsePath(cx,cy,rx,ry);
	g.fin();
	g.rgba32(c);
	g.fill1();
}

// colour c with its alpha scaled by a (0..1)
int Fade(int c, float a)
{
	int ca=(c>>24)&255;
	return (int(ca*a)<<24)|(c&0xffffff);
}

float Clamp01(float x)
{
	if(x<0)return 0;
	if(x>1)return 1;
	return x;
}

float Smooth(float x)
{
	x=Clamp01(x);
	return x*x*(3-2*x);
}

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

// ---------------------------------------------------------------- boxes and stones

int NumColor(int t)
{
	if(t<=1)return 0xffd8432f;
	if(t==2)return 0xffe8862a;
	if(t==3)return 0xffc9a030;
	return 0xff6b4a2a;
}

// a letter box of size s centred on cx,cy with number t; order>0 marks a
// box that is part of the word being spelled, a is its opacity
void DrawBox(float cx, float cy, float s, int ch, int t, int order, int hover, float tickPop, float a)
{
	float x=cx-s/2;
	float y=cy-s/2;
	float r=s*0.14;
	FillBox(x,y+s*0.05,s,s,r,Fade(0x60000000,a));          // shadow
	int edge=0xff8a5a2c;
	int face=0xffe9cd96;
	if(order>0)
	{
		edge=0xff4f7a1e;
		face=0xffc8e88a;
	}
	FillBox(x,y,s,s,r,Fade(edge,a));
	FillBox(x+s*0.06,y+s*0.05,s*0.88,s*0.84,r*0.8,Fade(face,a));
	if(hover&&order==0)FillBox(x+s*0.06,y+s*0.05,s*0.88,s*0.84,r*0.8,Fade(0x30ffffff,a));
	float lw=s/62*2.;
	if(lw<1.2)lw=1.2;
	Letter(ch-'a'+'A',cx-s*0.04,cy-s*0.03,s*0.052,s*0.046,Fade(0xff3c2412,a),lw,2.);
	float bx=x+s*0.80;
	float by=y+s*0.80;
	float p=1+0.35*tickPop;
	float br=s*0.15*p;
	Ellipse(bx,by,br,br,Fade(NumColor(t),a));
	Letter('0'+t,bx,by,s*0.021*p,s*0.019*p,Fade(0xfffff4e0,a),lw*0.65,2.);
	if(order>0)
	{
		char n[4];
		snprintf(n,3,"%i",order);
		Text(n,x+s*0.12,y+s*0.30,s*0.022,s*0.022,Fade(0xff2c4a10,a),lw*0.6,2.);
	}
}

// a rock with the letter it once was faintly carved in; its outline is
// fixed per cell, so every stone looks a bit different
void DrawStone(int i, float cx, float cy, float s, int ch, float a)
{
	int h=i*7919+13;
	g.clear();
	for(int k=0;k<9;k++)
	{
		h=(h*1103515245+12345)&0x7ffffff;
		float rr=s*(0.40+0.07*((h>>8)%100)/100.0);
		float an=k*2*M_PI/9+0.2;
		float px=cx+rr*cos(an);
		float py=cy+rr*sin(an)*0.92;
		if(k==0)g.M(px,py);
		else g.L(px,py);
	}
	g.close();
	g.fin();
	g.rgba32(Fade(0xff5b6365,a));
	g.fill1();
	g.width(1.5*SC,2.);
	g.rgba32(Fade(0xff3e4547,a));
	g.stroke();
	Ellipse(cx-s*0.12,cy-s*0.16,s*0.16,s*0.08,Fade(0x30ffffff,a));
	float lw=s/62*1.8;
	Letter(ch-'a'+'A',cx,cy+s*0.02,s*0.045,s*0.040,Fade(0x50202628,a),lw,2.);
}

float Shake(float T)
{
	float t=T-shakeT0;
	if(t<0.45)return 8*sin(t*45)*(1-t/0.45);
	return 0;
}

void DrawPond(float T, int mx, int my)
{
	FillBox(BX-8,BY-8,cols*CS+16,rows*CS+16,14,0xff10302f);
	int hc=-1;
	if(phase==PH_INPUT&&!statsOpen&&!toadsOpen&&!cardShown)hc=CellAt(mx,my);
	float tickPop=0;
	if(T-tickT<0.35)tickPop=sin(M_PI*(T-tickT)/0.35);
	for(int i=0;i<ncell;i++)
	{
		float x=BX+(i%cols)*CS;
		float y=BY+(i/cols)*CS;
		FillBox(x+2,y+2,CS-4,CS-4,CS*0.16,COL_WATER);
	}
	for(int i=0;i<ncell;i++)
	{
		float cx=CellX(i);
		float cy=CellY(i);
		float s=CS*0.86;
		if(kind[i]==K_STONE)
		{
			float u=Clamp01((T-stoneT[i])/0.5);
			if(u<1)
			{
				float j=2*sin(T*60)*(1-u);
				DrawBox(cx+j,cy,s,let[i],0,0,0,0,1-u);
				DrawStone(i,cx+j,cy,s,let[i],u);
			}
			else DrawStone(i,cx,cy,s,let[i],1);
		}
		if(kind[i]!=K_BOX)continue;
		float b=T-bornT[i];
		if(b<0)continue;
		float sc=1;
		float drop=0;
		if(b<T_SPAWN)
		{
			float u=b/T_SPAWN;
			sc=0.4+0.6*Smooth(u)+0.12*sin(M_PI*u);
			drop=-(1-Smooth(u))*CS*0.4;
		}
		int order=SelIndex(i)+1;
		float lift=0;
		if(order>0)lift=-3;
		if(i==hc)lift-=2.0;
		DrawBox(cx,cy+drop+lift,s*sc,let[i],tim[i],order,i==hc,tickPop,Clamp01(b/0.12));
	}
}

// ---------------------------------------------------------------- the toads

float ToadX(int i)
{
	return 95+170*i;
}

int ToadSkin(int t)
{
	if(t==TT_LENGTH)return 0xffa0803a;
	if(t==TT_DOUBLE)return 0xff3f8c74;
	return 0xff7a8c3a;
}

int ToadDark(int t)
{
	if(t==TT_LENGTH)return 0xff74592a;
	if(t==TT_DOUBLE)return 0xff2c6656;
	return 0xff55662a;
}

// where the eaten boxes are this frame; the tongue holds the one in flight
float flyX[MAXW];
float flyY[MAXW];
float flyS[MAXW];
int flyOn[MAXW];
float tongueX=0;
float tongueY=0;
int tongueOut=0;

void MouthPos(int i, float& x, float& y)
{
	x=ToadX(i);
	y=TOADS_Y-4*TOAD_K;
}

void UpdateEaten(float T)
{
	tongueOut=0;
	for(int i=0;i<MAXW;i++)flyOn[i]=0;
	if(phase!=PH_EAT)return;
	float mxp;
	float myp;
	MouthPos(eatToad,mxp,myp);
	for(int i=0;i<eatN;i++)
	{
		float t=(T-phaseT0-i*BOX_EAT)/BOX_EAT;
		if(t>=1)continue;
		float x=eatX[i];
		float y=eatY[i];
		float s=CS*0.86;
		if(t>0)
		{
			// the tongue shoots out, then pulls the box in
			if(t<0.35)
			{
				float u=t/0.35;
				tongueX=mxp+(x-mxp)*u;
				tongueY=myp+(y-myp)*u;
			}
			else
			{
				float u=Smooth((t-0.35)/0.65);
				x=x+(mxp-x)*u;
				y=y+(myp-y)*u;
				s=s*(1-0.75*u);
				tongueX=x;
				tongueY=y;
			}
			tongueOut=1;
		}
		flyX[i]=x;
		flyY[i]=y;
		flyS[i]=s;
		flyOn[i]=1;
	}
}

void DrawEaten()
{
	for(int i=0;i<eatN;i++)
	{
		if(flyOn[i])DrawBox(flyX[i],flyY[i],flyS[i],eatLet[i],eatNum[i],0,0,0,1);
	}
}

// a toad of type t at x,y scaled by k; open 0..1 opens the mouth, the eyes
// look at lx,ly; bg fills the notch of the lily pad
void DrawToadAt(float x, float y, float k, int t, float open, float lx, float ly, int bg)
{
	int skin=ToadSkin(t);
	int dark=ToadDark(t);

	// lily pad
	g.clear();
	EllipsePath(x,y+44*k,118*k,22*k);
	g.fin();
	g.rgba32(0xff2d6a33);
	g.fill1();
	g.clear();
	g.M(x+50*k,y+44*k);g.L(x+120*k,y+34*k);g.L(x+120*k,y+56*k);g.close();
	g.fin();
	g.rgba32(bg);
	g.fill1();

	// back legs, body, belly, front feet
	Ellipse(x-62*k,y+26*k,30*k,16*k,dark);
	Ellipse(x+62*k,y+26*k,30*k,16*k,dark);
	Ellipse(x,y+6*k,80*k,42*k,skin);
	Ellipse(x,y+22*k,54*k,22*k,0xffcfc48c);
	Ellipse(x-38*k,y+40*k,16*k,7*k,skin);
	Ellipse(x+38*k,y+40*k,16*k,7*k,skin);
	// warts
	Ellipse(x-52*k,y-6*k,6*k,5*k,dark);
	Ellipse(x-40*k,y+10*k,4*k,4*k,dark);
	Ellipse(x+48*k,y-2*k,5*k,5*k,dark);
	Ellipse(x+60*k,y+12*k,4*k,3*k,dark);
	Ellipse(x-64*k,y+8*k,3*k,3*k,dark);

	for(int e=0;e<2;e++)
	{
		float ex=x+(-34+68*e)*k;
		float ey=y-30*k;
		Ellipse(ex,ey,21*k,19*k,skin);
		Ellipse(ex,ey-2*k,14*k,14*k,0xffe0a83a);
		float dx=lx-ex;
		float dy=ly-ey;
		float d=sqrt(dx*dx+dy*dy);
		if(d>1)
		{
			dx=dx/d*4*k;
			dy=dy/d*4*k;
		}
		Ellipse(ex+dx,ey-2*k+dy,8*k,4*k,0xff181408);
		Ellipse(ex-4*k,ey-7*k,3*k,2*k,0xa0ffffff);
	}

	if(open>0.01)Ellipse(x,y-4*k,42*k,(4+12*open)*k,0xff3a1216);
	else
	{
		g.clear();
		g.M(x-48*k,y-10*k);
		g.C(x-20*k,y+2*k,x+20*k,y+2*k,x+48*k,y-10*k);
		g.fin();
		g.rgba32(0xff2e3a14);
		Stroke(1.8*k,2.);
	}
}

// the sign under a toad: its ability and multiplier, lit when the word
// being spelled (or eaten) pleases it
void DrawSign(float cx, float y, int t, int p, int level, int lit)
{
	char s[32];
	AbilityText(t,p,s);
	int c=COL_DIM;
	if(lit)c=COL_GOLD;
	TextC(s,cx,y,1.3,c,1.1,2.);
	snprintf(s,31,"x%i  level %i",level+1,level);
	TextC(s,cx,y+17,1.2,c,1.0,2.);
}

void DrawToads(float T, int mx, int my)
{
	float open=0;
	if(phase==PH_EAT)
	{
		float t=T-phaseT0;
		open=Clamp01(t/0.1)*Clamp01((eatN*BOX_EAT+0.1-t)/0.1);
	}
	for(int i=0;i<3;i++)
	{
		float x=ToadX(i);
		if(i>=nToads)
		{
			Ellipse(x,TOADS_Y+44*TOAD_K,118*TOAD_K,22*TOAD_K,0xff17403a);
			TextC("empty pad",x,TOADS_Y+4,1.3,COL_FAINT,1.1,2.);
			TextC("win a prize",x,TOADS_Y+60,1.2,COL_FAINT,1.0,2.);
			continue;
		}
		int lit=0;
		if(phase==PH_EAT)lit=eatTrig[i];
		else if(nsel>0)lit=trig[i];
		if(lit)Ellipse(x,TOADS_Y+4,62,44,0x18ffe080);
		float o=0;
		if(i==eatToad)o=open;
		float lx=mx;
		float ly=my;
		if(tongueOut)
		{
			lx=tongueX;
			ly=tongueY;
		}
		DrawToadAt(x,TOADS_Y,TOAD_K,toadType[i],o,lx,ly,COL_BG);
		DrawSign(x,TOADS_Y+60,toadType[i],toadParam[i],toadLevel[i],lit);
	}
	if(tongueOut)
	{
		float x;
		float y;
		MouthPos(eatToad,x,y);
		g.clear();
		g.M(x,y);
		g.L(tongueX,tongueY);
		g.fin();
		g.rgba32(0xffe0707a);
		Stroke(3,2.);
		Ellipse(tongueX,tongueY,5,4,0xffe0707a);
	}
}

void DrawPop(float T)
{
	float t=(T-popT)/1.1;
	if(t<0||t>1)return;
	float a=Clamp01((1-t)/0.3);
	char s[24];
	snprintf(s,23,"+%i",popPts);
	TextC(s,ToadX(eatToad),TOADS_Y-50-40*t,2.6,Fade(COL_GOLD,a),1.8,2.);
}

// ---------------------------------------------------------------- word tray and buttons

#define BTN_CLEAR_X 22
#define BTN_FEED_X 172
#define BTN_SKIP_X 388
#define BTN_SMALL_W 120
#define BTN_FEED_W 186

void DrawTray(float T, int mx, int my)
{
	float dx=Shake(T);
	if(nsel==0)
	{
		if(phase==PH_INPUT)TextC("Click boxes or type letters to spell a word",PLAY_W/2+dx,TRAY_Y,1.4,COL_DIM,1.2,2.);
	}
	else
	{
		float s=34;
		if(nsel*38>PLAY_W-140)s=(PLAY_W-140)/nsel-4;
		float w=nsel*(s+4)-4;
		float x0=PLAY_W/2-w/2-40+dx;
		for(int i=0;i<nsel;i++)
		{
			int c=sel[i];
			float cx=x0+i*(s+4)+s/2;
			DrawBox(cx,TRAY_Y,s,let[c],tim[c],0,0,0,1);
		}
		char p[24];
		snprintf(p,23,"%i",wordPts);
		int pc=COL_FAINT;
		if(wordOk)pc=COL_GOLD;
		float px=x0+w+14;
		Text(p,px,TRAY_Y+8,2.4,2.4,pc,1.7,2.);
		if(wordPts!=wordSum)
		{
			snprintf(p,23,"%i x%i",wordSum,wordPts/wordSum);
			Text(p,px,TRAY_Y-14,1.1,1.1,pc,1.0,2.);
		}
		if(toadWord)TextC("a toad word! x2",PLAY_W/2,TRAY_Y+26,1.3,COL_GOOD,1.1,2.);
	}
	if(phase==PH_OVER)return;
	int fc=COL_BTN;
	if(wordOk)fc=COL_GO;
	Button("CLEAR",BTN_CLEAR_X,BTN2_Y,BTN_SMALL_W,BTN2_H,COL_BTN,mx,my);
	Button("FEED",BTN_FEED_X,BTN2_Y,BTN_FEED_W,BTN2_H,fc,mx,my);
	Button("SKIP TURN",BTN_SKIP_X,BTN2_Y,BTN_SMALL_W,BTN2_H,COL_BTN,mx,my);
}

void DrawMessage(float T)
{
	if(msgDur==0)return;
	float t=T-msgT0;
	float a=1;
	if(t>msgDur)
	{
		msgDur=0;
		return;
	}
	if(t>msgDur-0.3)a=(msgDur-t)/0.3;
	float sc=1.8;
	float w=(TextLen(msg)*6-2)*sc+30;
	float h=36;
	FillBox(PLAY_W/2-w/2,TRAY_Y-h/2,w,h,6,Fade(0xfff2ecd8,a));
	TextC(msg,PLAY_W/2,TRAY_Y,sc,Fade(0xff1a1a14,a),1.5,2.);
}

// score, and a bar towards the silver and gold targets
void DrawInfo()
{
	char s[24];
	Text("SCORE",22,70,1.2,1.2,COL_DIM,1.0,2.);
	snprintf(s,23,"%i",score);
	Text(s,22,96,2.4,2.4,COL_TEXT,1.7,2.);
	snprintf(s,23,"turn %i",turn);
	Text(s,100,96,1.3,1.3,COL_DIM,1.1,2.);
	float x0=180;
	float x1=PLAY_W-22;
	float y=70;
	FillBox(x0,y,x1-x0,8,4,0xff1c3c3e);
	float f=Clamp01(score*1.0/goldAt);
	int bc=COL_GOOD;
	if(score>=silverAt)bc=COL_SILVER;
	if(f>0)FillBox(x0,y,(x1-x0)*f,8,4,bc);
	float sx=x0+(x1-x0)*silverAt/goldAt;
	FillBox(sx-1,y-3,2,14,0,COL_SILVER);
	FillBox(x1-2,y-3,2,14,0,COL_GOLD);
	snprintf(s,23,"silver %i",silverAt);
	TextC(s,sx,y+22,1.2,COL_SILVER,1.0,2.);
	snprintf(s,23,"gold %i",goldAt);
	TextR(s,x1,y+22,1.2,COL_GOLD,1.0,2.);
}

// ---------------------------------------------------------------- meaning panel

#define PANEL_TEXT 0xffdcdcd0
#define DEF_SC 1.4
#define DEF_LINE 19.0
#define LIST_Y (PANEL_Y+500)
#define LIST_ROW 25

float panelBottom=PANEL_Y+440;

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

void SlotWord(int s, char* w)
{
	int i=0;
	while(lkWord[s*16+i]!=0&&i<15)
	{
		w[i]=lkWord[s*16+i]-'a'+'A';
		i++;
	}
	w[i]=0;
}

void DrawMeaning(float T)
{
	float cx=PANEL_X+PANEL_W/2;
	if(shownSlot<0)
	{
		TextC("MEANING",cx,PANEL_Y+36,2.2,COL_DIM,1.6,2.);
		PanelLine("Every word the toads eat is",PANEL_Y+90,COL_DIM);
		PanelLine("looked up in an online",PANEL_Y+90+DEF_LINE,COL_DIM);
		PanelLine("dictionary and explained here.",PANEL_Y+90+2*DEF_LINE,COL_DIM);
		PanelLine("A word scores the numbers on",PANEL_Y+90+4*DEF_LINE,COL_DIM);
		PanelLine("its boxes. Each number is the",PANEL_Y+90+5*DEF_LINE,COL_DIM);
		PanelLine("turns left before the box",PANEL_Y+90+6*DEF_LINE,COL_DIM);
		PanelLine("turns to stone.",PANEL_Y+90+7*DEF_LINE,COL_DIM);
		PanelLine("The toads multiply the score",PANEL_Y+90+9*DEF_LINE,COL_DIM);
		PanelLine("of the words they like.",PANEL_Y+90+10*DEF_LINE,COL_DIM);
		return;
	}
	int s=shownSlot;
	char w[16];
	SlotWord(s,w);
	float tsc=3.2;
	int n=TextLen(w);
	if(n>8)tsc=3.2*8/n;
	TextC(w,cx,PANEL_Y+36,tsc,COL_TEXT,2.,2.);
	float y=PANEL_Y+66;
	FillBox(PANEL_X+16,y,PANEL_W-32,1,0,COL_LINE);
	y+=30.0;

	int k=lkState[s];
	if(k==LK_DICT||k==LK_WIKI)
	{
		char dots[8];
		int nd=int(T*3)%4;
		for(int i=0;i<nd;i++)dots[i]='.';
		dots[nd]=0;
		char t[40];
		snprintf(t,39,"Looking it up%s",dots);
		PanelLine(t,y,COL_DIM);
		return;
	}
	if(k==LK_NOTFOUND)
	{
		PanelLine("No definition found.",y,COL_DIM);
		return;
	}
	if(k==LK_OFFLINE)
	{
		PanelLine("Could not reach the dictionary.",y,COL_DIM);
		PanelLine("Is the network up?",y+DEF_LINE,COL_DIM);
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
			y=DrawWrapped(base,PANEL_X+16,y,31,COL_GOLD)+4;
			num=0;
		}
		else
		{
			num++;
			char ns[8];
			snprintf(ns,7,"%i.",num);
			Text(ns,PANEL_X+16,y,DEF_SC,DEF_SC,COL_DIM,1.2,2.);
			y=DrawWrapped(base,PANEL_X+38,y,29,PANEL_TEXT)+4;
		}
	}
	if(lkSource[s]==LK_DICT)TextC("from dictionaryapi.dev",cx,PANEL_Y+462,1.2,COL_FAINT,1.1,2.);
	else if(lkSource[s]==LK_TOADS)TextC("from the toads",cx,PANEL_Y+462,1.2,COL_FAINT,1.1,2.);
	else TextC("from wiktionary.org",cx,PANEL_Y+462,1.2,COL_FAINT,1.1,2.);
}

// slot shown in row r of the eaten list (newest first), or -1
int ListSlot(int r)
{
	if(r<0||r>=NSLOTS||r>=nHist)return -1;
	return (nHist-1-r)%NSLOTS;
}

int ListRowAt(int mx, int my)
{
	if(mx<PANEL_X+8||mx>=PANEL_X+PANEL_W-8)return -1;
	if(my<LIST_Y+10)return -1;
	int r=(my-LIST_Y-10)/LIST_ROW;
	if(ListSlot(r)<0)return -1;
	return r;
}

void DrawPanel(float T, int mx, int my)
{
	FillBox(PANEL_X,PANEL_Y,PANEL_W,PANEL_H,8,0xff122e30);
	DrawMeaning(T);
	FillBox(PANEL_X+16,LIST_Y-16,PANEL_W-32,1,0,COL_LINE);
	Text("EATEN",PANEL_X+16,LIST_Y,1.4,1.4,COL_DIM,1.2,2.);
	if(nHist==0)
	{
		Text("nothing yet",PANEL_X+16,LIST_Y+34,1.4,1.4,COL_FAINT,1.2,2.);
		return;
	}
	int hr=ListRowAt(mx,my);
	for(int r=0;r<NSLOTS;r++)
	{
		int s=ListSlot(r);
		if(s<0)break;
		float y=LIST_Y+10+r*LIST_ROW;
		if(s==shownSlot)FillBox(PANEL_X+8,y,PANEL_W-16,LIST_ROW-2,4,0xff1e4446);
		else if(r==hr)FillBox(PANEL_X+8,y,PANEL_W-16,LIST_ROW-2,4,0x14ffffff);
		char w[16];
		SlotWord(s,w);
		Text(w,PANEL_X+18,y+18,1.6,1.6,COL_TEXT,1.3,2.);
		char p[16];
		snprintf(p,15,"%i",lkPts[s]);
		TextR(p,PANEL_X+PANEL_W-18,y+12,1.5,COL_GOLD,1.2,2.);
	}
}

// ---------------------------------------------------------------- header, overlays

#define BTN_Y 10
#define BTN_H 28
#define BTN_W 100
#define NEWBTN_X 12
#define TOADBTN_X 120
#define STATBTN_X (G_SCREEN_WIDTH-12-BTN_W)

#define ST_X 230
#define ST_Y 170
#define ST_W 400
#define ST_H 380
#define ST_BTN_Y (ST_Y+ST_H-52)
#define ST_BTN_W 150
#define ST_BTN_H 34
#define ST_CLOSE 28

#define CARD_X 65
#define CARD_Y 180
#define CARD_W 400
#define CARD_H 270
#define CARD_BTN_Y (CARD_Y+CARD_H-52)
#define CARD_BTN_W 110

// the TOADS screen: owned toads on top, new ones below
#define TS_X 110
#define TS_Y 90
#define TS_W 640
#define TS_H 640
#define TS_CARD_W 180
#define TS_CARD_H 200
#define TS_ROW1 (TS_Y+110)
#define TS_ROW2 (TS_Y+380)

void DrawHeader(int mx, int my)
{
	TextC("ALPHATOAD",G_SCREEN_WIDTH/2,26,3.0,COL_TEXT,2.,2.);
	FillBox(0,48,G_SCREEN_WIDTH,1,0,COL_LINE);
	if(phase==PH_OVER)Button("NEW GAME",NEWBTN_X,BTN_Y,BTN_W,BTN_H,COL_GO,mx,my);
	else if(Time()-newGameT<3)Button("SURE?",NEWBTN_X,BTN_Y,BTN_W,BTN_H,0xffa83a32,mx,my);
	else Button("NEW GAME",NEWBTN_X,BTN_Y,BTN_W,BTN_H,COL_BTN,mx,my);
	if(prizes>0)
	{
		char s[24];
		snprintf(s,23,"TOADS (%i)",prizes);
		Button(s,TOADBTN_X,BTN_Y,BTN_W,BTN_H,0xff8a6a1e,mx,my);
	}
	else Button("TOADS",TOADBTN_X,BTN_Y,BTN_W,BTN_H,COL_BTN,mx,my);
	Button("STATISTICS",STATBTN_X,BTN_Y,BTN_W,BTN_H,COL_BTN,mx,my);
}

void NewGameButton()
{
	if(phase==PH_OVER||Time()-newGameT<3)
	{
		NewGame();
		return;
	}
	newGameT=Time();
	Message("Click again for a new game",3);
}

void DrawCross(float bx, float by, int mx, int my)
{
	if(Over(bx,by,ST_CLOSE,ST_CLOSE,mx,my))FillBox(bx,by,ST_CLOSE,ST_CLOSE,5,0x20ffffff);
	g.clear();
	g.M(bx+9,by+9);g.L(bx+ST_CLOSE-9,by+ST_CLOSE-9);
	g.M(bx+ST_CLOSE-9,by+9);g.L(bx+9,by+ST_CLOSE-9);
	g.fin();
	g.rgba32(0xffb0b0b0);
	Stroke(1.3,2.);
}

// a medal: a disc with a ribbon
void DrawMedal(float x, float y, float r, int c)
{
	g.clear();
	g.M(x-r*0.7,y-r*2.2);g.L(x-r*0.1,y-r*0.6);g.L(x+r*0.4,y-r*0.6);g.L(x-r*0.2,y-r*2.2);g.close();
	g.M(x+r*0.7,y-r*2.2);g.L(x+r*0.1,y-r*0.6);g.L(x-r*0.4,y-r*0.6);g.L(x+r*0.2,y-r*2.2);g.close();
	g.fin();
	g.rgba32(0xffb0403a);
	g.fill1();
	Ellipse(x,y,r,r,c);
	Ellipse(x,y,r*0.7,r*0.7,Fade(0x30000000,1));
	Ellipse(x-r*0.3,y-r*0.35,r*0.25,r*0.18,0x60ffffff);
}

void DrawCard(float T, int mx, int my)
{
	float a=Clamp01((T-cardT0)/0.3);
	FillBox(0,49,PLAY_W,G_SCREEN_HEIGHT-49,0,(int(a*140)<<24));
	FillBox(CARD_X,CARD_Y+(1-a)*30,CARD_W,CARD_H,10,Fade(0xff163638,a));
	if(a<1)return;
	FrameBox(CARD_X,CARD_Y,CARD_W,CARD_H,1,COL_LINE);
	float cx=CARD_X+CARD_W/2;
	char s[48];
	if(prize==2)
	{
		TextC("GOLD PRIZE!",cx,CARD_Y+34,2.6,COL_GOLD,1.9,2.);
		DrawMedal(CARD_X+44,CARD_Y+70,18,COL_GOLD);
		DrawMedal(CARD_X+CARD_W-44,CARD_Y+70,18,COL_GOLD);
	}
	else if(prize==1)
	{
		TextC("SILVER PRIZE",cx,CARD_Y+34,2.6,COL_SILVER,1.9,2.);
		DrawMedal(CARD_X+44,CARD_Y+70,18,COL_SILVER);
	}
	else TextC("THE POND TURNED TO STONE",cx,CARD_Y+34,2.1,COL_TEXT,1.7,2.);
	snprintf(s,47,"%i",score);
	TextC(s,cx,CARD_Y+86,4.4,COL_TEXT,2.4,2.);
	if(newBest)TextC("NEW BEST SCORE!",cx,CARD_Y+124,1.6,COL_GOOD,1.3,2.);
	else
	{
		snprintf(s,47,"best %i",best);
		TextC(s,cx,CARD_Y+124,1.5,COL_DIM,1.2,2.);
	}
	if(nWords==1)snprintf(s,47,"1 word in %i turns",turn);
	else snprintf(s,47,"%i words in %i turns",nWords,turn);
	TextC(s,cx,CARD_Y+150,1.5,COL_TEXT,1.2,2.);
	if(gBestPts>0)
	{
		char u[16];
		Upper(u,gBest);
		snprintf(s,47,"best word %s, %i pts",u,gBestPts);
		TextC(s,cx,CARD_Y+172,1.5,COL_TEXT,1.2,2.);
	}
	if(prize==1)TextC("one prize for your toads",cx,CARD_Y+196,1.4,COL_GOLD,1.2,2.);
	if(prize==2)TextC("two prizes for your toads",cx,CARD_Y+196,1.4,COL_GOLD,1.2,2.);
	float bx=CARD_X+20;
	float step=(CARD_W-40-CARD_BTN_W)/2;
	Button("LOOK",bx,CARD_BTN_Y,CARD_BTN_W,ST_BTN_H,COL_BTN,mx,my);
	int tc=COL_BTN;
	if(prizes>0)tc=0xff8a6a1e;
	Button("TOADS",bx+step,CARD_BTN_Y,CARD_BTN_W,ST_BTN_H,tc,mx,my);
	Button("NEW GAME",bx+2*step,CARD_BTN_Y,CARD_BTN_W,ST_BTN_H,COL_GO,mx,my);
}

void CardClick(int mx, int my)
{
	float bx=CARD_X+20;
	float step=(CARD_W-40-CARD_BTN_W)/2;
	if(Over(bx,CARD_BTN_Y,CARD_BTN_W,ST_BTN_H,mx,my))cardShown=0;
	if(Over(bx+step,CARD_BTN_Y,CARD_BTN_W,ST_BTN_H,mx,my))ToggleToads();
	if(Over(bx+2*step,CARD_BTN_Y,CARD_BTN_W,ST_BTN_H,mx,my))NewGame();
}

float StatsBackX()
{
	if(phase==PH_OVER)return ST_X+30;
	return ST_X+ST_W/2-ST_BTN_W/2;
}

float StatsNewX()
{
	return ST_X+ST_W-30-ST_BTN_W;
}

void DrawStats(float T, int mx, int my)
{
	float a=Clamp01((T-statsT0)/0.2);
	FillBox(0,49,G_SCREEN_WIDTH,G_SCREEN_HEIGHT-49,0,(int(a*150)<<24));
	FillBox(ST_X,ST_Y+(1-a)*30,ST_W,ST_H,10,Fade(0xff163638,a));
	if(a<1)return;
	FrameBox(ST_X,ST_Y,ST_W,ST_H,1,COL_LINE);
	float cx=ST_X+ST_W/2;
	TextC("STATISTICS",cx,ST_Y+30,2.2,COL_TEXT,1.6,2.);
	DrawCross(ST_X+ST_W-ST_CLOSE-8,ST_Y+8,mx,my);

	int vals[4];
	vals[0]=games;
	vals[1]=best;
	vals[2]=silvers;
	vals[3]=golds;
	for(int i=0;i<4;i++)
	{
		float x=ST_X+ST_W*(i+0.5)/4;
		char s[16];
		snprintf(s,15,"%i",vals[i]);
		int c=COL_TEXT;
		if(i==2)c=COL_SILVER;
		if(i==3)c=COL_GOLD;
		TextC(s,x,ST_Y+86,3.2,c,2.,2.);
	}
	TextC("Games",ST_X+ST_W*0.5/4,ST_Y+120,1.4,COL_DIM,1.2,2.);
	TextC("Best",ST_X+ST_W*1.5/4,ST_Y+120,1.4,COL_DIM,1.2,2.);
	TextC("Silver",ST_X+ST_W*2.5/4,ST_Y+120,1.4,COL_DIM,1.2,2.);
	TextC("Gold",ST_X+ST_W*3.5/4,ST_Y+120,1.4,COL_DIM,1.2,2.);

	char u[16];
	char s[48];
	Text("Words eaten",ST_X+30,ST_Y+176,1.5,1.5,COL_DIM,1.2,2.);
	snprintf(s,47,"%i",totalWords);
	Text(s,ST_X+160,ST_Y+176,1.7,1.7,COL_TEXT,1.3,2.);
	Text("Best word",ST_X+30,ST_Y+212,1.5,1.5,COL_DIM,1.2,2.);
	if(bestWordPts>0)
	{
		Upper(u,bestWord);
		snprintf(s,47,"%s  %i pts",u,bestWordPts);
	}
	else snprintf(s,47,"-");
	Text(s,ST_X+160,ST_Y+212,1.7,1.7,COL_TEXT,1.3,2.);
	Text("Longest",ST_X+30,ST_Y+248,1.5,1.5,COL_DIM,1.2,2.);
	if(longest[0]!=0)
	{
		Upper(u,longest);
		snprintf(s,47,"%s  %i letters",u,TextLen(longest));
	}
	else snprintf(s,47,"-");
	Text(s,ST_X+160,ST_Y+248,1.7,1.7,COL_TEXT,1.3,2.);

	Button("BACK",StatsBackX(),ST_BTN_Y,ST_BTN_W,ST_BTN_H,COL_BTN,mx,my);
	if(phase==PH_OVER)Button("NEW GAME",StatsNewX(),ST_BTN_Y,ST_BTN_W,ST_BTN_H,COL_GO,mx,my);
}

float TsCardX(int i)
{
	return TS_X+(TS_W-3*TS_CARD_W-2*20)/2+i*(TS_CARD_W+20);
}

// one card on the TOADS screen; action is the label of what a click does,
// "" when it can't be clicked
void ToadCard(float x, float y, int t, int p, int level, char* action, int mx, int my)
{
	int hov=action[0]!=0&&Over(x,y,TS_CARD_W,TS_CARD_H,mx,my);
	int bg=0xff1b4244;
	if(hov)bg=0xff304f50;
	FillBox(x,y,TS_CARD_W,TS_CARD_H,8,bg);
	DrawToadAt(x+TS_CARD_W/2,y+62,0.55,t,0,mx,my,bg);
	char s[32];
	AbilityText(t,p,s);
	TextC(s,x+TS_CARD_W/2,y+118,1.4,COL_TEXT,1.2,2.);
	if(level>0)snprintf(s,31,"level %i   x%i",level,level+1);
	else snprintf(s,31,"x2");
	TextC(s,x+TS_CARD_W/2,y+140,1.3,COL_DIM,1.1,2.);
	if(action[0]!=0)
	{
		int c=0xff8a6a1e;
		FillBox(x+30,y+TS_CARD_H-40,TS_CARD_W-60,28,5,c);
		if(hov)FillBox(x+30,y+TS_CARD_H-40,TS_CARD_W-60,28,5,0x20ffffff);
		TextC(action,x+TS_CARD_W/2,y+TS_CARD_H-26,1.4,COL_TEXT,1.2,2.);
	}
}

void DrawToadsScreen(float T, int mx, int my)
{
	float a=Clamp01((T-toadsT0)/0.2);
	FillBox(0,49,G_SCREEN_WIDTH,G_SCREEN_HEIGHT-49,0,(int(a*160)<<24));
	FillBox(TS_X,TS_Y+(1-a)*30,TS_W,TS_H,10,Fade(0xff163638,a));
	if(a<1)return;
	FrameBox(TS_X,TS_Y,TS_W,TS_H,1,COL_LINE);
	float cx=TS_X+TS_W/2;
	TextC("TOADS",cx,TS_Y+30,2.4,COL_TEXT,1.8,2.);
	DrawCross(TS_X+TS_W-ST_CLOSE-8,TS_Y+8,mx,my);
	char s[64];
	if(prizes==0)snprintf(s,63,"No prizes to spend - win silver or gold");
	else if(prizes==1)snprintf(s,63,"1 prize to spend: raise a toad or claim a new one");
	else snprintf(s,63,"%i prizes to spend: raise a toad or claim a new one",prizes);
	int pc=COL_DIM;
	if(prizes>0)pc=COL_GOLD;
	TextC(s,cx,TS_Y+62,1.5,pc,1.2,2.);

	Text("YOUR TOADS",TS_X+30,TS_ROW1-14,1.4,1.4,COL_DIM,1.2,2.);
	for(int i=0;i<3;i++)
	{
		float x=TsCardX(i);
		if(i>=nToads)
		{
			FillBox(x,TS_ROW1,TS_CARD_W,TS_CARD_H,8,0xff133234);
			TextC("empty pad",x+TS_CARD_W/2,TS_ROW1+TS_CARD_H/2,1.4,COL_FAINT,1.2,2.);
			continue;
		}
		char* act="";
		if(prizes>0&&toadLevel[i]<MAX_LEVEL)act="RAISE LEVEL";
		ToadCard(x,TS_ROW1,toadType[i],toadParam[i],toadLevel[i],act,mx,my);
		if(toadLevel[i]>=MAX_LEVEL)TextC("top level",x+TS_CARD_W/2,TS_ROW1+TS_CARD_H-26,1.3,COL_GOLD,1.1,2.);
	}

	if(nToads<3)
	{
		Text("NEW TOADS",TS_X+30,TS_ROW2-14,1.4,1.4,COL_DIM,1.2,2.);
		for(int j=0;j<3;j++)
		{
			char* act="";
			if(prizes>0)act="CLAIM";
			ToadCard(TsCardX(j),TS_ROW2,offerType[j],offerParam[j],0,act,mx,my);
		}
	}
	else TextC("All three pads are taken.",cx,TS_ROW2+60,1.5,COL_FAINT,1.2,2.);
	TextC("A toad multiplies the score of the words it likes by its level + 1.",cx,TS_Y+TS_H-24,1.3,COL_DIM,1.1,2.);
}

void ToadsClick(int mx, int my)
{
	if(!Over(TS_X,TS_Y,TS_W,TS_H,mx,my)||Over(TS_X+TS_W-ST_CLOSE-8,TS_Y+8,ST_CLOSE,ST_CLOSE,mx,my))
	{
		toadsOpen=0;
		return;
	}
	for(int i=0;i<3;i++)
	{
		if(Over(TsCardX(i),TS_ROW1,TS_CARD_W,TS_CARD_H,mx,my))RaiseToad(i);
		if(nToads<3&&Over(TsCardX(i),TS_ROW2,TS_CARD_W,TS_CARD_H,mx,my))ClaimToad(i);
	}
}

void Draw(float T, int mx, int my)
{
	g.rgba32(COL_BG);
	g.FillRT();
	DrawHeader(mx,my);
	DrawInfo();
	DrawPond(T,mx,my);
	DrawTray(T,mx,my);
	UpdateEaten(T);
	DrawToads(T,mx,my);
	DrawEaten();
	DrawPop(T);
	DrawPanel(T,mx,my);
	if(cardShown)DrawCard(T,mx,my);
	DrawMessage(T);
	if(statsOpen)DrawStats(T,mx,my);
	if(toadsOpen)DrawToadsScreen(T,mx,my);
}

// ---------------------------------------------------------------- input

void OnKey(int k)
{
	if(statsOpen||toadsOpen)
	{
		if(k==KEY_ESC)
		{
			statsOpen=0;
			toadsOpen=0;
		}
		return;
	}
	if(phase==PH_OVER)
	{
		if(k==KEY_ENTER||k==KEY_ENTER2)
		{
			if(Time()-phaseT0>0.8)NewGame();
		}
		else if(k==KEY_ESC)cardShown=0;
		return;
	}
	if(phase!=PH_INPUT)return;
	if(k>='A'&&k<='Z')k=k-'A'+'a';
	if(k>='a'&&k<='z')TypeLetter(k);
	else if(k==KEY_BACK)
	{
		if(nsel>0)
		{
			nsel--;
			UpdateWord();
		}
	}
	else if(k==KEY_ESC)ClearWord();
	else if(k==KEY_ENTER||k==KEY_ENTER2)Feed();
}

void OnClick(int mx, int my)
{
	if(toadsOpen)
	{
		ToadsClick(mx,my);
		return;
	}
	if(statsOpen)
	{
		if(!Over(ST_X,ST_Y,ST_W,ST_H,mx,my))statsOpen=0;
		else if(Over(ST_X+ST_W-ST_CLOSE-8,ST_Y+8,ST_CLOSE,ST_CLOSE,mx,my))statsOpen=0;
		else if(Over(StatsBackX(),ST_BTN_Y,ST_BTN_W,ST_BTN_H,mx,my))statsOpen=0;
		else if(phase==PH_OVER&&Over(StatsNewX(),ST_BTN_Y,ST_BTN_W,ST_BTN_H,mx,my))NewGame();
		return;
	}
	if(Over(NEWBTN_X,BTN_Y,BTN_W,BTN_H,mx,my))
	{
		NewGameButton();
		return;
	}
	if(Over(TOADBTN_X,BTN_Y,BTN_W,BTN_H,mx,my))
	{
		ToggleToads();
		return;
	}
	if(Over(STATBTN_X,BTN_Y,BTN_W,BTN_H,mx,my))
	{
		statsOpen=1;
		statsT0=Time();
		return;
	}
	int r=ListRowAt(mx,my);
	if(r>=0)
	{
		shownSlot=ListSlot(r);
		return;
	}
	if(cardShown)
	{
		CardClick(mx,my);
		return;
	}
	if(phase!=PH_INPUT)return;
	int c=CellAt(mx,my);
	if(c>=0)
	{
		ClickCell(c);
		return;
	}
	if(Over(BTN_CLEAR_X,BTN2_Y,BTN_SMALL_W,BTN2_H,mx,my))ClearWord();
	if(Over(BTN_FEED_X,BTN2_Y,BTN_FEED_W,BTN2_H,mx,my))Feed();
	if(Over(BTN_SKIP_X,BTN2_Y,BTN_SMALL_W,BTN2_H,mx,my))SkipTurn();
}

// ---------------------------------------------------------------- main

int Animating(float T)
{
	if(phase!=PH_INPUT&&phase!=PH_OVER)return 1;
	if(T-shakeT0<0.5)return 1;
	if(T-popT<1.15)return 1;
	if(T-tickT<0.4)return 1;
	if(msgDur>0)return 1;
	if(statsOpen&&T-statsT0<0.25)return 1;
	if(toadsOpen&&T-toadsT0<0.25)return 1;
	if(cardShown&&T-cardT0<0.35)return 1;
	for(int i=0;i<ncell;i++)
	{
		if(kind[i]==K_STONE&&T-stoneT[i]<0.55)return 1;
		if(kind[i]==K_BOX&&T-bornT[i]<T_SPAWN+0.05)return 1;
	}
	return quitReq!=0;
}

int Look(float T)
{
	int v=0;
	if(T-newGameT<3)v=1;
	for(int s=0;s<NSLOTS;s++)
	{
		if(lkState[s]==LK_DICT||lkState[s]==LK_WIKI)v+=2+(int(T*3)%4)*2;
	}
	return v;
}

// "9x6" -> cols 9, rows 6
void ParseSize(char* a)
{
	int i=0;
	int c=0;
	while(a[i]>='0'&&a[i]<='9')
	{
		c=c*10+a[i]-'0';
		i++;
	}
	if(i==0||a[i]!='x')return;
	i++;
	int r=0;
	int j=i;
	while(a[i]>='0'&&a[i]<='9')
	{
		r=r*10+a[i]-'0';
		i++;
	}
	if(i==j||a[i]!=0)return;
	if(c<4)c=4;
	if(c>10)c=10;
	if(r<4)r=4;
	if(r>10)r=10;
	cols=c;
	rows=r;
}

int main(int argc, pchar* argv)
{
	for(int i=1;i<argc;i++)
	{
		char* a=argv[i];
		if(a[0]=='-'&&a[1]=='x'&&a[2]=='2'&&a[3]==0)SC=2;
		else ParseSize(a);
	}
	if(SC!=1)
	{
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

	int timeSeed=Time()*1e9;
	seed+=timeSeed;
	irand();

	LoadDict();
	LoadTotals();
	if(nToads==0)
	{
		// the first toad comes for free
		int t;
		int p;
		for(int i=0;i<3;i++)
		{
			offerType[i]=-1;
			offerParam[i]=-1;
		}
		RandAbility(0,t,p);
		toadType[0]=t;
		toadParam[0]=p;
		toadLevel[0]=1;
		nToads=1;
	}
	Layout();
	for(int s=0;s<NSLOTS;s++)lkReq[s]=0;
	NewGame();

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
		if(StepPhase(T))dirty=1;
		if(UpdateLookups())dirty=1;

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
		if((mb&1)&&!(prevMB&1))OnClick(mx,my);
		if(mx!=prevMX||my!=prevMY||mb!=prevMB)dirty=1;
		prevMB=mb;
		prevMX=mx;
		prevMY=my;

		int look=Look(T);
		if(look!=prevLook)dirty=1;
		prevLook=look;

		int anim=Animating(T);
		if(wasAnim&&!anim)dirty=1;
		wasAnim=anim;

		if(dirty||anim)
		{
			Draw(T,mx,my);
			Present();
			lastPresent=T;
			dirty=0;
			Wait(0.01);
		}
		else
		{
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
