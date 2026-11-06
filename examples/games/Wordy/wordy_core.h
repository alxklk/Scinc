// The game behind wordy.cpp and wordy_tui.cpp, without any drawing: the word lists
// of both languages and their letter encoding, the game state and its scoring,
// the statistics, the language setting and the online word meanings.
//
// A front end calls WordInit once, WordSetLanguage(LoadLang()), then for each game
// WordNewGame; it adds letters with WordType and WordErase, sends a row with
// WordSubmit and, when it has shown the result, WordRowDone, which says whether the
// game was won or lost; for a loss it calls WordLose (also to give up). Polling
// UpdateLookups now and then fills in the meanings of the guessed words.
//
// Inside the game a letter is one byte: 'a'..'z', or Windows-1251 for Ukrainian.
#ifndef WORDY_CORE_H
#define WORDY_CORE_H

#ifndef HTTP_DL
#define HTTP_DL "../../claude/net/http_dl"
#endif
#include "../../claude/net/http.h"

#include "all5letterw.h"
#include "answers.h"

#define WD_GOON 0    // WordRowDone: the next row is open
#define WD_WON 1
#define WD_LOST 2

// the next character of UTF-8 text s at i (i moves past it); 0 at the end, -1
// for a broken sequence
int WordUtf8Next(char* s, int& i)
{
	int c=s[i]&255;
	if(c==0)return 0;
	i++;
	if(c<128)return c;
	int n=0;
	if((c&0xE0)==0xC0)
	{
		c=c&0x1F;
		n=1;
	}
	else if((c&0xF0)==0xE0)
	{
		c=c&0x0F;
		n=2;
	}
	else if((c&0xF8)==0xF0)
	{
		c=c&0x07;
		n=3;
	}
	else return -1;
	for(int k=0;k<n;k++)
	{
		int b=s[i]&255;
		if((b&0xC0)!=0x80)return -1;
		c=(c<<6)|(b&0x3F);
		i++;
	}
	return c;
}

int irand(int& seed)
{
	seed=(seed*1103515245+12345)&0x7ffffff;
	return seed;
}

int seed=6917832;

// ---------------------------------------------------------------- languages
//
// English or Ukrainian, each with its own word lists and statistics. Inside
// the game a letter is one byte: 'a'..'z', or Windows-1251 for Ukrainian
// (the encoding of words_ua.txt), so words stay plain byte strings; they turn
// into Unicode only on screen and for the meaning lookup. (The same scheme
// as Alphatoad.)

#define LANG_EN 0
#define LANG_UA 1
int lang=LANG_EN;
char* CFG_FILE="wordy.cfg";

int CpToUni(int b)
{
	b=b&255;
	if(b<128)return b;
	if(b>=0xC0)return 0x410+b-0xC0;
	if(b==0xB3)return 0x456;
	if(b==0xB2)return 0x406;
	if(b==0xBF)return 0x457;
	if(b==0xAF)return 0x407;
	if(b==0xBA)return 0x454;
	if(b==0xAA)return 0x404;
	if(b==0xB4)return 0x491;
	if(b==0xA5)return 0x490;
	return '?';
}

int UniToCp(int u)
{
	if(u<128)return u;
	if(u>=0x410&&u<=0x44F)return u-0x410+0xC0;
	if(u==0x456)return 0xB3;
	if(u==0x406)return 0xB2;
	if(u==0x457)return 0xBF;
	if(u==0x407)return 0xAF;
	if(u==0x454)return 0xBA;
	if(u==0x404)return 0xAA;
	if(u==0x491)return 0xB4;
	if(u==0x490)return 0xA5;
	return '?';
}

// capital of a letter (a byte, Latin or Windows-1251)
int UpChar(int c)
{
	c=c&255;
	if(c>='a'&&c<='z')return c-'a'+'A';
	if(c>=0xE0)return c-0x20;
	if(c==0xB3)return 0xB2;
	if(c==0xBF)return 0xAF;
	if(c==0xBA)return 0xAA;
	if(c==0xB4)return 0xA5;
	return c;
}

// Windows-1251 text to UTF-8, for DrawText and URLs
void CpToUtf8(char* s, char* d, int max)
{
	int n=0;
	int i=0;
	while(s[i]!=0&&n<max-3)
	{
		int u=CpToUni(s[i]);
		if(u<128)
		{
			d[n]=u;
			n++;
		}
		else
		{
			d[n]=0xC0|(u>>6);
			d[n+1]=0x80|(u&63);
			n+=2;
		}
		i++;
	}
	d[n]=0;
}

// stand-ins for the accented Latin letters U+00C0..U+017F (zh in "zhaba")
char* latinBase="AAAAAAACEEEEIIIIDNOOOOOxOUUUUYTsaaaaaaaceeeeiiiidnooooo/ouuuuytyAaAaAaCcCcCcCcDdDdEeEeEeEeEeGgGgGgGgHhHhIiIiIiIiIiIiJjKkkLlLlLlLlLlNnNnNnnNnOoOoOoOoRrRrRrSsSsSsSsTtTtTtUuUuUuUuUuUuWwYyYZzZzZzs";

// UTF-8 from the web to Windows-1251 in place: Ukrainian letters stay,
// accented Latin letters, typographic quotes and dashes become ASCII,
// anything else '?'
void Utf8ToCp(char* s)
{
	int i=0;
	int o=0;
	while(true)
	{
		int c=WordUtf8Next(s,i);
		if(c==0)break;
		if(c==0x301)continue;   // stress mark
		int b='?';
		if(c>0&&c<128)b=c;
		else if(c==0xA0||c==0x2002||c==0x2003||c==0x2009)b=' ';
		else if(c==0x2018||c==0x2019||c==0x201B||c==0x2032||c==0xB4||c==0x2BC)b='\'';
		else if(c==0x201C||c==0x201D||c==0x201E||c==0xAB||c==0xBB)b='"';
		else if((c>=0x2010&&c<=0x2015)||c==0x2212)b='-';
		else if(c>=0xC0&&c<0x180)b=latinBase[c-0xC0];
		else if(c>=0x400)b=UniToCp(c);
		s[o]=b;
		o++;
	}
	s[o]=0;
}

// UTF-8 text to Windows-1251 bytes as ints, for the keyboard rows below
int Utf8ToCodes(char* s, int* out)
{
	int i=0;
	int n=0;
	while(true)
	{
		int c=WordUtf8Next(s,i);
		if(c==0)break;
		out[n]=UniToCp(c);
		n++;
	}
	return n;
}

// Latin keys where a Ukrainian keyboard has its letters, for typing
// Ukrainian with a Latin system layout
char* uaKeysLatin="qwertyuiop[]asdfghjkl;'zxcvbnm,.`\\";
int uaKeysCode[40];
int uaAlpha[40];      // the 33 Ukrainian letters
int nUaAlpha=0;

void InitUaKeys()
{
	Utf8ToCodes("йцукенгшщзхїфівапролджєячсмитьбюґґ",uaKeysCode);
	nUaAlpha=Utf8ToCodes("абвгґдеєжзиіїйклмнопрстуфхцчшщьюя",uaAlpha);
}

// the Ukrainian letter on Latin key k, or 0
int UaKey(int k)
{
	for(int i=0;uaKeysLatin[i]!=0;i++)
		if(uaKeysLatin[i]==k)return uaKeysCode[i];
	return 0;
}

int IsLetter(int k)
{
	if(lang==LANG_EN)return k>='a'&&k<='z';
	for(int i=0;i<nUaAlpha;i++)if(uaAlpha[i]==k)return 1;
	return 0;
}

// The Ukrainian word lists, read from their files the first time they are
// needed: 5 bytes a word, no separators. words_ua.txt is sorted by bytes.
#define UA_WORDS 21000
#define UA_ANSWERS 1300
char* uaWords=0;
char* uaAns=0;
int nUaWords=0;
int nUaAns=0;

int ReadWords(char* file, char* dst, int max)
{
	FILE* f=fopen(file,"rb");
	if(f==0)
	{
		printf("%s not found - run Wordy from its own folder\n",file);
		return 0;
	}
	char line[64];
	int n=0;
	while(n<max)
	{
		line[0]=0;
		fgets(line,63,f);
		if(line[0]==0)break;
		int len=0;
		while(line[len]!=0&&line[len]!=10&&line[len]!=13)len++;
		if(len!=5)continue;
		for(int i=0;i<5;i++)dst[n*5+i]=line[i];
		n++;
	}
	fclose(f);
	return n;
}

void LoadUkrainian()
{
	if(uaWords!=0)return;
	uaWords=(char*)malloc(UA_WORDS*5);
	uaAns=(char*)malloc(UA_ANSWERS*5);
	nUaWords=ReadWords("words_ua.txt",uaWords,UA_WORDS);
	nUaAns=ReadWords("answers_ua.txt",uaAns,UA_ANSWERS);
}

int NumAllowed()
{
	if(lang==LANG_UA)return nUaWords;
	return N_ALL5LETTER;
}

int NumAnswers()
{
	if(lang==LANG_UA)return nUaAns;
	return N_ANSWERS;
}

// letter i of allowed word k, and of answer k
int AllowedLetter(int k, int i)
{
	int c=0;
	if(lang==LANG_UA)c=uaWords[k*5+i];
	else c=all5letter[k][i];
	return c&255;
}

int AnswerLetter(int k, int i)
{
	int c=0;
	if(lang==LANG_UA)c=uaAns[k*5+i];
	else c=all5letter[answers[k]][i];
	return c&255;
}

// ---------------------------------------------------------------- game state

int secret[5];
int guess[6*5];    // letters 'a'..'z', 0 = empty
int mark[6*5];     // 0 not scored, 1 absent, 2 present, 3 correct
int keyMark[256];  // best mark seen for each letter (by its byte)
int row=0;         // row being typed
int col=0;         // next free tile in that row
int state=0;       // 0 playing, 1 won, 2 lost
// statistics for this session
int played=0;
int wins=0;
int streak=0;
int maxStreak=0;
int dist[6];
int lastWinRow=-1;

// the statistics of both languages; the variables above hold the current one
int lsPlayed[2];
int lsWins[2];
int lsStreak[2];
int lsMax[2];
int lsDist[12];

void StoreStats(int l)
{
	lsPlayed[l]=played;
	lsWins[l]=wins;
	lsStreak[l]=streak;
	lsMax[l]=maxStreak;
	for(int i=0;i<6;i++)lsDist[l*6+i]=dist[i];
}

void FetchStats(int l)
{
	played=lsPlayed[l];
	wins=lsWins[l];
	streak=lsStreak[l];
	maxStreak=lsMax[l];
	for(int i=0;i<6;i++)dist[i]=lsDist[l*6+i];
	lastWinRow=-1;
}

int remaining=0;   // answers still consistent with every revealed row


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

char lkUrl[400];
char wikiKey[4];   // the language's section in Wiktionary's answer: en, uk
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
	char u8[16];
	char enc[128];
	CpToUtf8(w,u8,16);
	UrlEncode(u8,enc,128);
	if(source==LK_DICT)snprintf(lkUrl,399,"https://api.dictionaryapi.dev/api/v2/entries/en/%s",enc);
	else snprintf(lkUrl,399,"https://en.wiktionary.org/api/rest_v1/page/definition/%s",enc);
	lkReq[s]=HttpGet(lkUrl);
	lkState[s]=source;
	if(lkReq[s]==0)lkState[s]=LK_OFFLINE;   // http_dl found no libcurl
}

// look up the five letters in word[] (letter bytes as ints) into slot s;
// Ukrainian words go straight to Wiktionary
void Lookup(int s, int* word, int off)
{
	if(lkReq[s]>0)HttpFree(lkReq[s]);
	for(int i=0;i<5;i++)lkWord[s*6+i]=word[off+i];
	lkWord[s*6+5]=0;
	nItems[s]=0;
	if(lang==LANG_UA)StartLookup(s,LK_WIKI);
	else StartLookup(s,LK_DICT);
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
	snprintf(lkPath,63,"%s.%i.definitions.%i.definition",wikiKey,m,d);
	HttpJson(id,lkPath,lkBuf,4096);
	HtmlToText(lkBuf);
	if(lang==LANG_UA)Utf8ToCp(lkBuf);
	else Utf8ToAscii(lkBuf);
}

void ParseWiki(int s, int id)
{
	int nm=HttpJsonCount(id,wikiKey);
	for(int m=0;m<nm&&m<3;m++)
	{
		snprintf(lkPath,63,"%s.%i.definitions",wikiKey,m);
		int nd=HttpJsonCount(id,lkPath);
		int shown=0;
		for(int d=0;d<nd&&shown<2;d++)
		{
			WikiDef(id,m,d);
			if(lkBuf[0]==0)continue;
			if(shown==0)
			{
				snprintf(lkPath,63,"%s.%i.partOfSpeech",wikiKey,m);
				HttpJson(id,lkPath,lkBuf,4096);
				AddItem(s,0);
				WikiDef(id,m,d);
			}
			AddItem(s,1);
			shown++;
		}
	}
}

// returns 1 when some lookup finished, so the screen needs a redraw
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
		else if(k==LK_DICT)StartLookup(s,LK_WIKI);   // after a network error too: one more try
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
	}
	shownSlot=-1;
}

// binary search in the sorted list of allowed guesses
int InWordList(int r)
{
	int lo=0;
	int hi=NumAllowed()-1;
	while(lo<=hi)
	{
		int mid=(lo+hi)/2;
		int d=0;
		for(int i=0;i<5;i++)
		{
			d=guess[r*5+i]-AllowedLetter(mid,i);
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
	int left[256];
	for(int i=0;i<256;i++)left[i]=0;
	for(int i=0;i<5;i++)
	{
		if(guess[r*5+i]==secret[i])mark[r*5+i]=3;
		else
		{
			mark[r*5+i]=1;
			left[secret[i]]++;
		}
	}
	for(int i=0;i<5;i++)
	{
		if(mark[r*5+i]==3)continue;
		int c=guess[r*5+i];
		if(left[c]>0)
		{
			mark[r*5+i]=2;
			left[c]--;
		}
	}
}

// 1 if secret word w would have given row r exactly the marks it got
int Consistent(int r, int* w)
{
	int left[256];
	for(int i=0;i<256;i++)left[i]=0;
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
			left[w[i]]++;
		}
	}
	for(int i=0;i<5;i++)
	{
		int m=mark[r*5+i];
		if(m==3)continue;
		int c=guess[r*5+i];
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
	int w[5];
	int na=NumAnswers();
	for(int k=0;k<na;k++)
	{
		for(int i=0;i<5;i++)w[i]=AnswerLetter(k,i);
		int ok=1;
		for(int r=0;r<=last&&ok;r++)ok=Consistent(r,w);
		if(ok)n++;
	}
	remaining=n;
}


// ---------------------------------------------------------------- playing

// a new hidden word and an empty board
void WordNewGame()
{
	ResetLookups();
	int n=(irand(seed)>>4)%NumAnswers();
	for(int i=0;i<5;i++)secret[i]=AnswerLetter(n,i);
	for(int i=0;i<30;i++)
	{
		guess[i]=0;
		mark[i]=0;
	}
	for(int i=0;i<256;i++)keyMark[i]=0;
	row=0;
	col=0;
	state=0;
	lastWinRow=-1;
	remaining=NumAnswers();
}

// The letter a key gives: lower case, and in Ukrainian the letter on a Latin key
// where a Ukrainian keyboard has it. Returns 0 if it is no letter of the language.
int WordLetterOf(int k)
{
	if((k>='A')&&(k<='Z'))k=k-'A'+'a';
	if((lang==LANG_UA)&&(k>32)&&(k<128))
	{
		int u=UaKey(k);
		if(u!=0)k=u;
	}
	if(IsLetter(k))return k;
	return 0;
}

// Adds letter k to the row. Returns 1 if it fit.
int WordType(int k)
{
	if(state!=0)return 0;
	if(col>=5)return 0;
	guess[row*5+col]=k;
	col++;
	return 1;
}

void WordErase()
{
	if(state!=0)return;
	if(col>0)
	{
		col--;
		guess[row*5+col]=0;
	}
}

#define WD_SHORT 1     // WordSubmit: fewer than 5 letters
#define WD_UNKNOWN 2   // not in the word list

// Scores the typed row and looks its meaning up. Returns 0, WD_SHORT or WD_UNKNOWN.
int WordSubmit()
{
	if(col<5)return WD_SHORT;
	if(!InWordList(row))return WD_UNKNOWN;
	Score(row);
	Lookup(row,guess,row*5);
	return 0;
}

// The game is lost, out of rows or given up: the hidden word's meaning is looked up.
void WordLose()
{
	state=2;
	played++;
	streak=0;
	Lookup(6,secret,0);
	shownSlot=6;
}

// After row r has been scored and shown: the letters' best marks, the answers that
// are left, the statistics. Returns WD_WON, WD_LOST (call WordLose) or WD_GOON.
int WordRowDone(int r)
{
	shownSlot=r;
	int greens=0;
	for(int i=0;i<5;i++)
	{
		int c=guess[r*5+i];
		if(mark[r*5+i]>keyMark[c])keyMark[c]=mark[r*5+i];
		if(mark[r*5+i]==3)greens++;
	}
	CountRemaining(r);
	if(greens==5)
	{
		state=1;
		played++;
		wins++;
		streak++;
		if(streak>maxStreak)maxStreak=streak;
		dist[r]++;
		lastWinRow=r;
		return WD_WON;
	}
	row++;
	col=0;
	if(row>=6)return WD_LOST;
	return WD_GOON;
}

// ---------------------------------------------------------------- languages and settings

int LoadLang()
{
	FILE* f=fopen(CFG_FILE,"rb");
	if(f==0)return LANG_EN;
	char s[16];
	int l=LANG_EN;
	s[0]=0;
	fgets(s,16,f);
	if(s[0]=='u'&&s[1]=='a')l=LANG_UA;
	fclose(f);
	return l;
}

void SaveLang()
{
	FILE* f=fopen(CFG_FILE,"wb");
	if(f==0)return;
	if(lang==LANG_UA)fputs("ua language\n",f);
	else fputs("en language\n",f);
	fclose(f);
}


// Word lists, statistics and dictionary of language l, saved as the setting.
// Returns 0 when the Ukrainian lists are missing and English stays.
int WordSetLanguage(int l)
{
	int ok=1;
	if(l==LANG_UA)
	{
		LoadUkrainian();
		if((nUaAns==0)||(nUaWords==0))
		{
			ok=0;
			l=LANG_EN;
		}
	}
	StoreStats(lang);
	lang=l;
	FetchStats(lang);
	wikiKey[0]='e';
	wikiKey[1]='n';
	wikiKey[2]=0;
	if(lang==LANG_UA)
	{
		wikiKey[0]='u';
		wikiKey[1]='k';
	}
	SaveLang();
	return ok;
}

// Once at the start: empty statistics, the Ukrainian keys, a seed from the clock.
void WordInit()
{
	for(int i=0;i<6;i++)dist[i]=0;
	for(int l=0;l<2;l++)
	{
		lsPlayed[l]=0;
		lsWins[l]=0;
		lsStreak[l]=0;
		lsMax[l]=0;
	}
	for(int i=0;i<12;i++)lsDist[i]=0;
	InitUaKeys();
	int timeSeed=Time()*1e9;
	seed+=timeSeed;
	irand(seed);
}

#endif // WORDY_CORE_H
