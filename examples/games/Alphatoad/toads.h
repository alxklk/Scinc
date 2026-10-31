// toads.h - the toads of Alphatoad, classified by ability.
//
// One ToadDef per toad, filled by InitToadDefs(). A toad's number (toadId,
// kept in the save files) is its index in toadDef[], so never reorder or
// delete lines: append new toads at the end, and retire one by setting its
// tier to 0.
//
// A toad's ability is code: non-capturing lambdas in its hooks, so each
// toad, however unusual, carries its own rule. All hooks look at a
// ToadCtx, which the game fills (the word, its boxes, the pond, the turn,
// the other toads); ToadScore() shows how the hooks combine into a score.
// A hook left 0 means the toad doesn't act at that moment.
//
//   OnWord   int(d,c)        multiplier for the whole word (1 = not pleased)
//   OnBox    int(d,c,i)      multiplier for the number on box i of the word
//   OnPlus   int(d,c)        flat points added to the word's score
//   OnTurn   int(d,c)        after each turn: TE_* things for the game to do
//   OnSpawn  int(d,c)        a box is about to land: its letter, '?', or 0
//   OnLevel  int(d,c,pad)    level bonus for the toad on another pad
// and `effect` names game-side effects no hook can do (TE_LOWER_BAR ...).
//
// Entries with TS_HAVE are what alphatoad.cpp plays today (toads 0..24);
// TS_IDEA entries are designs, not in the game yet (their hooks work on a
// ToadCtx, but the game doesn't call them). Edit freely.
//
// Scinc notes, found by probes:
//  - The table is filled by calls, not a braced {..} initialiser: a string
//    that appears twice in global initialisers silently points at the
//    wrong text; literals passed to functions are fine.
//  - A hook is set by passing the lambda to a setter (OnWord(...)), as in
//    ui/menu: assigning it through a returned reference, Def(..).word=..,
//    silently leaves the pointer 0. Hooks can be tested for truth
//    (if(d.word)) but not compared or negated (==, !=, ! don't compile).
//  - Letters are lowercase ASCII or Windows-1251 bytes, like the pond's.
//    When alphatoad.cpp includes this file, its own toadLetters /
//    uaToadLetter must go: the same literal twice would be mislinked.
//
// Open questions:
//  - For pond, turn and meta toads, should the level make them act more
//    often (smaller K) instead of raising a multiplier? Now: param is K.
//  - Can a player hold two toads of one class (two letter toads at once)?
//  - Rare letters follow each alphabet: English has 6, Ukrainian 13, so
//    rare toads 6..12 are Ukrainian only (langs = LG_UA).
//  - Should the timed mode toads (TF_TIMED) appear only in the timed mode?

// ---------------------------------------------------------------- classes

#define TC_LETTER  1   // a letter in the word
#define TC_SHAPE   2   // the word as a whole
#define TC_BOXES   3   // the numbers and places of the boxes used
#define TC_BONUS   4   // flat points instead of a multiplier
#define TC_POND    5   // keeps boxes from turning to stone
#define TC_MAKER   6   // what lands on the pond
#define TC_TURN    7   // more moves per turn or per game
#define TC_STREAK  8   // across turns: streaks and combos
#define TC_FRIEND  9   // works through the other toads
#define TC_META    10  // prizes and the run, not a single word
#define TC_TIMED   11  // the timed mode (if it comes)

// ---------------------------------------------------------------- languages and flags

#define TL_EN   0      // ToadCtx.lang
#define TL_UA   1
#define LG_EN   1      // ToadDef.langs: where the toad exists
#define LG_UA   2
#define LG_BOTH 3

#define TS_HAVE  1     // in the game now
#define TS_IDEA  2     // a design, not in the game yet
#define TF_TIMED 4     // only makes sense in the timed mode

// what % in desc stands for: ToadDef.arg, the list param picks from
#define TA_NONE    0
#define TA_LETTER  1   // tlLetters
#define TA_LENGTH  2   // EN 3+param, UA 4+param
#define TA_START   3   // tlStarts
#define TA_ENDING  4   // tlEndings
#define TA_RARE    5   // tlRare
#define TA_PAIR    6   // tlPairs

// things for the game to do: OnTurn returns a mix of the first ones,
// ToadDef.effect holds one of the game-side ones
#define TE_NO_AGING    1      // the numbers don't go down this turn
#define TE_CALM        2      // only 2 boxes land next turn
#define TE_REVIVE      4      // a stone turns back into a fresh box
#define TE_EAT_STONE   8      // one stone goes away
#define TE_REFRESH     16     // the word's boxes are replaced on the same cells
#define TE_SWAP        32     // the player may click a box to change its letter
#define TE_SECOND_WORD 64     // one more word this turn
#define TE_FREEZE      128    // the player may click a box that won't age
#define TE_LOWER_BAR   1024   // effect: silver and gold param% lower
#define TE_LUCKY       2048   // effect: param% chance of one more prize
#define TE_BIG_POND    4096   // effect: the pond has one more row
#define TE_UNDO        8192   // effect: one undo of a whole turn per game
#define TE_EXTRA_TIME  16384  // effect: +param seconds per word
#define TE_PAUSE       32768  // effect: the clock stops param seconds once a game

// ---------------------------------------------------------------- what the hooks see

#define TOAD_W 16      // longest word
#define TOAD_C 100     // biggest pond, 10x10

struct ToadCtx
{
	int lang;                // TL_EN, TL_UA
	int level;               // of the toad being asked (set by ToadScore)
	int self;                // its pad, 0..2
	// the word
	int len;
	int letter[TOAD_W];      // lowercase ASCII / Windows-1251
	int num[TOAD_W];         // the numbers on its boxes
	int cell[TOAD_W];        // where they are: row*cols+col
	// the pond: 0 empty, -1 stone, else the letter
	int cols;
	int rows;
	int pond[TOAD_C];
	// across turns
	int turn;                // 1, 2, ...
	int skipped;             // this turn was a SKIP (OnTurn)
	int streak;              // words in a row without a skip, this one not counted
	int prevLen;             // the previous word, 0 if none
	int prevLetter[TOAD_W];
	int rand100;             // a fresh random 0..99 for every hook call that wants one
	float timeLeft;          // timed mode
	// the toads on the pads
	int nToads;
	int toadNo[3];           // index into toadDef[]
	int toadLevel[3];
	int fired[3];            // pleased by this word (filled by ToadScore)
};

struct ToadDef;
typedef int(*TToadHook)(ToadDef& d, ToadCtx& c);
typedef int(*TToadBoxHook)(ToadDef& d, ToadCtx& c, int i);

struct ToadDef
{
	int cls;       // TC_*
	int param;     // the k-th letter / length / ending, K turns, points, ...
	int tier;      // 1..10 (comes from prizes of this tier or higher); 0 = retired
	int langs;     // LG_*
	int flags;     // TS_* | TF_*
	int effect;    // a game-side TE_* effect, 0 if none
	int arg;       // TA_*: what param picks, for % in desc
	char* name;
	char* desc;    // for the TOADS screen; % stands for ToadArgText()
	TToadHook word;
	TToadBoxHook box;
	TToadHook plus;
	TToadHook turn;
	TToadHook spawn;
	TToadBoxHook levelUp;
};

#define TOAD_DEFS_MAX 160
ToadDef toadDef[TOAD_DEFS_MAX];
int nToadDefs=0;

// ---------------------------------------------------------------- language data
// Space separated lists, Windows-1251 for Ukrainian; params pick the k-th item.

char* tlLettersEn="e t a o i n s r h l d c u m p g y f w b";
// о а н е и т в р і с м д у к л п б з я г
char* tlLettersUa="\xee \xe0 \xed \xe5 \xe8 \xf2 \xe2 \xf0 \xb3 \xf1 \xec \xe4 \xf3 \xea \xeb \xef \xe1 \xe7 \xff \xe3";
char* tlRareEn="v k j x q z";
// ж ш щ ц ф х ї є ґ ч ю й ь
char* tlRareUa="\xe6 \xf8 \xf9 \xf6 \xf4 \xf5 \xbf \xba \xb4 \xf7 \xfe \xe9 \xfc";
char* tlVowelsEn="a e i o u";
// а е є и і ї о у ю я
char* tlVowelsUa="\xe0 \xe5 \xba \xe8 \xb3 \xbf \xee \xf3 \xfe \xff";
char* tlPairsEn="th qu st ch sh";
// ст ни ов ра ко
char* tlPairsUa="\xf1\xf2 \xed\xe8 \xee\xe2 \xf0\xe0 \xea\xee";
char* tlStartsEn="s c p t a";
// п в з н с
char* tlStartsUa="\xef \xe2 \xe7 \xed \xf1";
char* tlEndingsEn="s ed ing er ly";
// ти ий ння ся ою
char* tlEndingsUa="\xf2\xe8 \xe8\xe9 \xed\xed\xff \xf1\xff \xee\xfe";
// theme toads 0..2: animals, food, colours (word lists still to make)

// the k-th item of a list: its codes into out[], returns its length (0: none)
int ToadListItem(char* list, int k, int* out)
{
	int i=0;
	while(k>0&&list[i]!=0)
	{
		if(list[i]==' ')k--;
		i++;
	}
	int n=0;
	while(list[i]!=0&&list[i]!=' '&&n<TOAD_W)
	{
		int b=list[i]&255;
		out[n]=b;
		n++;
		i++;
	}
	return n;
}

char* ToadList(char* en, char* ua, int lang)
{
	if(lang==TL_UA)return ua;
	return en;
}

// the first code of the k-th item (for one-letter lists)
int ToadListLetter(char* en, char* ua, int lang, int k)
{
	int a[TOAD_W];
	int n=ToadListItem(ToadList(en,ua,lang),k,a);
	if(n==0)return -1;
	return a[0];
}

int ToadInList(char* en, char* ua, int lang, int c)
{
	char* l=ToadList(en,ua,lang);
	for(int i=0;l[i]!=0;i++)
		if(l[i]!=' '&&(l[i]&255)==c)return 1;
	return 0;
}

int ToadIsVowel(ToadCtx& c, int ch)
{
	return ToadInList(tlVowelsEn,tlVowelsUa,c.lang,ch);
}

// does the word hold the codes a[0..n-1] at position p?
int ToadHasAt(ToadCtx& c, int* a, int n, int p)
{
	if(p<0||p+n>c.len)return 0;
	for(int i=0;i<n;i++)if(c.letter[p+i]!=a[i])return 0;
	return 1;
}

// the toad's level plus one: the usual multiplier
int ToadX(ToadCtx& c)
{
	return c.level+1;
}

// lengths start one higher in Ukrainian: its words are longer
int ToadLenBase(ToadCtx& c, int en)
{
	if(c.lang==TL_UA)return en+1;
	return en;
}

int ToadAdjacent(ToadCtx& c, int a, int b)
{
	int dr=a/c.cols-b/c.cols;
	int dc=a%c.cols-b%c.cols;
	if(dr<0)dr=-dr;
	if(dc<0)dc=-dc;
	return dr<=1&&dc<=1&&(dr+dc)>0;
}

// ---------------------------------------------------------------- the table

void Def(int cls, int param, int tier, int langs, int flags, char* name, char* desc)
{
	if(nToadDefs>=TOAD_DEFS_MAX)return;
	ToadDef& d=toadDef[nToadDefs];
	d.cls=cls;
	d.param=param;
	d.tier=tier;
	d.langs=langs;
	d.flags=flags;
	d.effect=0;
	d.arg=TA_NONE;
	d.name=name;
	d.desc=desc;
	d.word=0;
	d.box=0;
	d.plus=0;
	d.turn=0;
	d.spawn=0;
	d.levelUp=0;
	nToadDefs++;
}

// hook setters: they act on the toad defined last
void OnWord(TToadHook h)     { toadDef[nToadDefs-1].word=h; }
void OnBox(TToadBoxHook h)   { toadDef[nToadDefs-1].box=h; }
void OnPlus(TToadHook h)     { toadDef[nToadDefs-1].plus=h; }
void OnTurn(TToadHook h)     { toadDef[nToadDefs-1].turn=h; }
void OnSpawn(TToadHook h)    { toadDef[nToadDefs-1].spawn=h; }
void OnLevel(TToadBoxHook h) { toadDef[nToadDefs-1].levelUp=h; }
void Effect(int e)           { toadDef[nToadDefs-1].effect=e; }
void Arg(int a)              { toadDef[nToadDefs-1].arg=a; }

void InitToadDefs()
{
	nToadDefs=0;

	// ---- in the game (TS_HAVE)

	// 0..19 letter toads: EN E T A O I N S R H L D C U M P G Y F W B,
	// UA О А Н Е И Т В Р І С М Д У К Л П Б З Я Г
	for(int k=0;k<20;k++)
	{
		int tier=1;
		if(k>=10)tier=2;
		Def(TC_LETTER,k,tier,LG_BOTH,TS_HAVE,"Letter toad","boxes of % count x(level+1)");
		Arg(TA_LETTER);
		OnBox([](ToadDef& d, ToadCtx& c, int i)->int
		{
			if(c.letter[i]==ToadListLetter(tlLettersEn,tlLettersUa,c.lang,d.param))return ToadX(c);
			return 1;
		});
	}
	// 20..23 length toads: EN 3..6 letters, UA 4..7
	for(int k=0;k<4;k++)
	{
		int tier=1;
		if(k>=2)tier=2;
		Def(TC_SHAPE,k,tier,LG_BOTH,TS_HAVE,"Length toad","a word of % letters scores x(level+1)");
		Arg(TA_LENGTH);
		OnWord([](ToadDef& d, ToadCtx& c)->int
		{
			if(c.len==ToadLenBase(c,3)+d.param)return ToadX(c);
			return 1;
		});
	}
	// 24
	Def(TC_SHAPE,0,2,LG_BOTH,TS_HAVE,"Double toad","a double letter (bOOk) scores x(level+1)");
	OnWord([](ToadDef& d, ToadCtx& c)->int
	{
		for(int i=0;i+1<c.len;i++)if(c.letter[i]==c.letter[i+1])return ToadX(c);
		return 1;
	});

	// ---- ideas from here on (TS_IDEA)

	// flat bonuses: easy to balance, good for the first tiers
	Def(TC_BONUS,5,1,LG_BOTH,TS_IDEA,"Pocket toad","+5 x level points for any word");
	OnPlus([](ToadDef& d, ToadCtx& c)->int{ return d.param*c.level; });
	Def(TC_BONUS,1,2,LG_BOTH,TS_IDEA,"Counting toad","+1 x level points per box used");
	OnPlus([](ToadDef& d, ToadCtx& c)->int{ return d.param*c.level*c.len; });
	Def(TC_BONUS,1,3,LG_BOTH,TS_IDEA,"Stone toad","+1 x level points per stone on the pond");
	OnPlus([](ToadDef& d, ToadCtx& c)->int
	{
		int n=0;
		for(int i=0;i<c.cols*c.rows;i++)if(c.pond[i]==-1)n++;
		return d.param*c.level*n;
	});

	// tier 3: letters and shapes
	Def(TC_LETTER,0,3,LG_BOTH,TS_IDEA,"Vowel toad","vowel boxes count x(level+1)");
	OnBox([](ToadDef& d, ToadCtx& c, int i)->int
	{
		if(ToadIsVowel(c,c.letter[i]))return ToadX(c);
		return 1;
	});
	Def(TC_LETTER,0,3,LG_BOTH,TS_IDEA,"Consonant toad","consonant boxes count x(level+1)");
	OnBox([](ToadDef& d, ToadCtx& c, int i)->int
	{
		if(!ToadIsVowel(c,c.letter[i]))return ToadX(c);
		return 1;
	});
	for(int k=0;k<5;k++)
	{
		Def(TC_SHAPE,k,3,LG_BOTH,TS_IDEA,"First letter toad","a word starting with % scores x(level+1)");
		Arg(TA_START);
		OnWord([](ToadDef& d, ToadCtx& c)->int
		{
			if(c.len>0&&c.letter[0]==ToadListLetter(tlStartsEn,tlStartsUa,c.lang,d.param))return ToadX(c);
			return 1;
		});
	}
	for(int k=0;k<5;k++)
	{
		Def(TC_SHAPE,k,3,LG_BOTH,TS_IDEA,"Ending toad","a word ending in -% scores x(level+1)");
		Arg(TA_ENDING);
		OnWord([](ToadDef& d, ToadCtx& c)->int
		{
			int a[TOAD_W];
			int n=ToadListItem(ToadList(tlEndingsEn,tlEndingsUa,c.lang),d.param,a);
			if(n>0&&c.len>n&&ToadHasAt(c,a,n,c.len-n))return ToadX(c);
			return 1;
		});
	}
	Def(TC_BOXES,0,3,LG_BOTH,TS_IDEA,"Last chance toad","boxes with 1 turn left count x(level+1)");
	OnBox([](ToadDef& d, ToadCtx& c, int i)->int
	{
		if(c.num[i]==1)return ToadX(c);
		return 1;
	});

	// tier 4: rare letters (6..12 are Ukrainian only) and harder shapes
	for(int k=0;k<13;k++)
	{
		int langs=LG_BOTH;
		if(k>=6)langs=LG_UA;
		Def(TC_LETTER,k,4,langs,TS_IDEA,"Rare letter toad","a word with % scores x(level+2): it lands rarely");
		Arg(TA_RARE);
		OnWord([](ToadDef& d, ToadCtx& c)->int
		{
			int r=ToadListLetter(tlRareEn,tlRareUa,c.lang,d.param);
			for(int i=0;i<c.len;i++)if(c.letter[i]==r)return c.level+2;
			return 1;
		});
	}
	for(int k=0;k<2;k++)
	{
		int tier=4;
		if(k==1)tier=5;
		Def(TC_SHAPE,k,tier,LG_BOTH,TS_IDEA,"Long word toad","a word of % letters or more (EN 7/8, UA 8/9)");
		OnWord([](ToadDef& d, ToadCtx& c)->int
		{
			if(c.len>=ToadLenBase(c,7)+d.param)return ToadX(c);
			return 1;
		});
	}
	Def(TC_SHAPE,0,4,LG_BOTH,TS_IDEA,"Unique toad","no letter repeats in the word");
	OnWord([](ToadDef& d, ToadCtx& c)->int
	{
		for(int i=0;i<c.len;i++)
			for(int j=i+1;j<c.len;j++)
				if(c.letter[i]==c.letter[j])return 1;
		return ToadX(c);
	});
	Def(TC_BOXES,7,4,LG_BOTH,TS_IDEA,"Fresh toad","boxes numbered 7 or more count x(level+1)");
	OnBox([](ToadDef& d, ToadCtx& c, int i)->int
	{
		if(c.num[i]>=d.param)return ToadX(c);
		return 1;
	});
	Def(TC_SHAPE,0,2,LG_BOTH,TS_IDEA,"Tiny toad","a 2-letter word scores x2(level+1)");
	OnWord([](ToadDef& d, ToadCtx& c)->int
	{
		if(c.len==2)return 2*ToadX(c);
		return 1;
	});
	Def(TC_SHAPE,0,4,LG_BOTH,TS_IDEA,"Singing toad","more vowels than consonants");
	OnWord([](ToadDef& d, ToadCtx& c)->int
	{
		int v=0;
		for(int i=0;i<c.len;i++)if(ToadIsVowel(c,c.letter[i]))v++;
		if(2*v>c.len)return ToadX(c);
		return 1;
	});
	Def(TC_SHAPE,0,4,LG_BOTH,TS_IDEA,"Grumbling toad","three consonants in a row");
	OnWord([](ToadDef& d, ToadCtx& c)->int
	{
		int run=0;
		for(int i=0;i<c.len;i++)
		{
			if(ToadIsVowel(c,c.letter[i]))run=0;
			else run++;
			if(run>=3)return ToadX(c);
		}
		return 1;
	});

	// tier 5: pond keepers, letter makers, letter pairs
	Def(TC_POND,5,5,LG_BOTH,TS_IDEA,"Slow toad","every 5 turns the numbers don't go down");
	OnTurn([](ToadDef& d, ToadCtx& c)->int
	{
		if(c.turn%d.param==0)return TE_NO_AGING;
		return 0;
	});
	Def(TC_POND,4,5,LG_BOTH,TS_IDEA,"Frost toad","every 4 turns a clicked box doesn't age");
	OnTurn([](ToadDef& d, ToadCtx& c)->int
	{
		if(c.turn%d.param==0)return TE_FREEZE;
		return 0;
	});
	Def(TC_POND,4,5,LG_BOTH,TS_IDEA,"Calm toad","every 4 turns only 2 boxes land");
	OnTurn([](ToadDef& d, ToadCtx& c)->int
	{
		if(c.turn%d.param==0)return TE_CALM;
		return 0;
	});
	for(int k=0;k<2;k++)
	{
		Def(TC_MAKER,k,5,LG_BOTH,TS_IDEA,"Favourite letter toad","% lands more often (10% x level of the boxes)");
		Arg(TA_LETTER);
		OnSpawn([](ToadDef& d, ToadCtx& c)->int
		{
			if(c.rand100<10*c.level)return ToadListLetter(tlLettersEn,tlLettersUa,c.lang,d.param);
			return 0;
		});
	}
	Def(TC_MAKER,0,5,LG_BOTH,TS_IDEA,"Rain toad","with no vowel on the pond the next box is a vowel");
	OnSpawn([](ToadDef& d, ToadCtx& c)->int
	{
		for(int i=0;i<c.cols*c.rows;i++)if(c.pond[i]>0&&ToadIsVowel(c,c.pond[i]))return 0;
		int n=5;
		if(c.lang==TL_UA)n=10;
		return ToadListLetter(tlVowelsEn,tlVowelsUa,c.lang,c.rand100%n);
	});
	Def(TC_BOXES,0,5,LG_BOTH,TS_IDEA,"Even toad","boxes with even numbers count x(level+1)");
	OnBox([](ToadDef& d, ToadCtx& c, int i)->int
	{
		if(c.num[i]%2==0)return ToadX(c);
		return 1;
	});
	Def(TC_BOXES,0,5,LG_BOTH,TS_IDEA,"Odd toad","boxes with odd numbers count x(level+1)");
	OnBox([](ToadDef& d, ToadCtx& c, int i)->int
	{
		if(c.num[i]%2==1)return ToadX(c);
		return 1;
	});
	for(int k=0;k<5;k++)
	{
		Def(TC_LETTER,k,5,LG_BOTH,TS_IDEA,"Pair toad","a word with % scores x(level+1)");
		Arg(TA_PAIR);
		OnWord([](ToadDef& d, ToadCtx& c)->int
		{
			int a[TOAD_W];
			int n=ToadListItem(ToadList(tlPairsEn,tlPairsUa,c.lang),d.param,a);
			for(int p=0;p+n<=c.len;p++)if(ToadHasAt(c,a,n,p))return ToadX(c);
			return 1;
		});
	}

	// tier 6: places on the pond
	Def(TC_BOXES,0,6,LG_BOTH,TS_IDEA,"Straight toad","all boxes in one row or one column");
	OnWord([](ToadDef& d, ToadCtx& c)->int
	{
		int sameRow=1;
		int sameCol=1;
		for(int i=1;i<c.len;i++)
		{
			if(c.cell[i]/c.cols!=c.cell[0]/c.cols)sameRow=0;
			if(c.cell[i]%c.cols!=c.cell[0]%c.cols)sameCol=0;
		}
		if(c.len>=3&&(sameRow||sameCol))return ToadX(c);
		return 1;
	});
	Def(TC_BOXES,0,6,LG_BOTH,TS_IDEA,"Snake toad","each box touches the next one");
	OnWord([](ToadDef& d, ToadCtx& c)->int
	{
		for(int i=0;i+1<c.len;i++)if(!ToadAdjacent(c,c.cell[i],c.cell[i+1]))return 1;
		return ToadX(c);
	});
	Def(TC_BOXES,0,6,LG_BOTH,TS_IDEA,"Corner toad","boxes in the corners count x2(level+1)");
	OnBox([](ToadDef& d, ToadCtx& c, int i)->int
	{
		int r=c.cell[i]/c.cols;
		int q=c.cell[i]%c.cols;
		if((r==0||r==c.rows-1)&&(q==0||q==c.cols-1))return 2*ToadX(c);
		return 1;
	});
	Def(TC_BOXES,0,6,LG_BOTH,TS_IDEA,"Shore toad","boxes at the edge of the pond count x(level+1)");
	OnBox([](ToadDef& d, ToadCtx& c, int i)->int
	{
		int r=c.cell[i]/c.cols;
		int q=c.cell[i]%c.cols;
		if(r==0||r==c.rows-1||q==0||q==c.cols-1)return ToadX(c);
		return 1;
	});
	Def(TC_BOXES,0,6,LG_BOTH,TS_IDEA,"Moss toad","boxes next to a stone count x(level+1)");
	OnBox([](ToadDef& d, ToadCtx& c, int i)->int
	{
		for(int j=0;j<c.cols*c.rows;j++)
			if(c.pond[j]==-1&&ToadAdjacent(c,j,c.cell[i]))return ToadX(c);
		return 1;
	});
	Def(TC_MAKER,0,6,LG_BOTH,TS_IDEA,"Wildcard toad","sometimes (4% x level) a ? box lands: any letter");
	OnSpawn([](ToadDef& d, ToadCtx& c)->int
	{
		if(c.rand100<4*c.level)return '?';
		return 0;
	});

	// tier 7: across turns
	Def(TC_STREAK,0,7,LG_BOTH,TS_IDEA,"Streak toad","+level points per word in a row; a skip resets it");
	OnPlus([](ToadDef& d, ToadCtx& c)->int{ return c.streak*c.level; });
	Def(TC_STREAK,0,7,LG_BOTH,TS_IDEA,"Chain toad","starts with the last letter of the previous word");
	OnWord([](ToadDef& d, ToadCtx& c)->int
	{
		if(c.prevLen>0&&c.len>0&&c.letter[0]==c.prevLetter[c.prevLen-1])return ToadX(c);
		return 1;
	});
	Def(TC_STREAK,2,7,LG_BOTH,TS_IDEA,"Rhyme toad","ends like the previous word (2 letters)");
	OnWord([](ToadDef& d, ToadCtx& c)->int
	{
		int n=d.param;
		if(c.prevLen<n||c.len<n)return 1;
		for(int i=1;i<=n;i++)if(c.letter[c.len-i]!=c.prevLetter[c.prevLen-i])return 1;
		return ToadX(c);
	});
	Def(TC_STREAK,0,7,LG_BOTH,TS_IDEA,"Growing toad","longer than the previous word");
	OnWord([](ToadDef& d, ToadCtx& c)->int
	{
		if(c.prevLen>0&&c.len>c.prevLen)return ToadX(c);
		return 1;
	});

	// tier 8: changing the pond and the turn
	Def(TC_POND,6,8,LG_BOTH,TS_IDEA,"Revive toad","once in 6 turns a stone turns back into a box");
	OnTurn([](ToadDef& d, ToadCtx& c)->int
	{
		if(c.turn%d.param==0)return TE_REVIVE;
		return 0;
	});
	Def(TC_POND,6,8,LG_BOTH,TS_IDEA,"Clean-up toad","a word of 6+ letters eats a stone");
	OnTurn([](ToadDef& d, ToadCtx& c)->int
	{
		if(!c.skipped&&c.len>=d.param)return TE_EAT_STONE;
		return 0;
	});
	Def(TC_POND,0,8,LG_BOTH,TS_IDEA,"Refresh toad","the boxes used are replaced on the same cells");
	OnTurn([](ToadDef& d, ToadCtx& c)->int
	{
		if(!c.skipped)return TE_REFRESH;
		return 0;
	});
	Def(TC_MAKER,5,8,LG_BOTH,TS_IDEA,"Swap toad","once in 5 turns a click changes a box's letter");
	OnTurn([](ToadDef& d, ToadCtx& c)->int
	{
		if(c.turn%d.param==0)return TE_SWAP;
		return 0;
	});
	Def(TC_TURN,4,8,LG_BOTH,TS_IDEA,"Nap toad","once in 4 turns SKIP doesn't age the boxes");
	OnTurn([](ToadDef& d, ToadCtx& c)->int
	{
		if(c.skipped&&c.turn%d.param==0)return TE_NO_AGING;
		return 0;
	});
	Def(TC_TURN,5,8,LG_BOTH,TS_IDEA,"Hungry toad","once in 5 turns a second word in the same turn");
	OnTurn([](ToadDef& d, ToadCtx& c)->int
	{
		if(!c.skipped&&c.turn%d.param==0)return TE_SECOND_WORD;
		return 0;
	});
	Def(TC_TURN,0,8,LG_BOTH,TS_IDEA,"Undo toad","take back the last turn, once a game");
	Effect(TE_UNDO);

	// tier 9: friends of other toads
	Def(TC_FRIEND,0,9,LG_BOTH,TS_IDEA,"Chorus toad","x(level+1) when another toad is pleased too");
	OnWord([](ToadDef& d, ToadCtx& c)->int
	{
		for(int k=0;k<c.nToads;k++)if(k!=c.self&&c.fired[k])return ToadX(c);
		return 1;
	});
	Def(TC_FRIEND,0,9,LG_BOTH,TS_IDEA,"Mimic toad","copies the toad on its left, one level lower");
	OnWord([](ToadDef& d, ToadCtx& c)->int
	{
		if(c.self==0||c.level<2)return 1;
		ToadDef& o=toadDef[c.toadNo[c.self-1]];
		if(o.cls==TC_FRIEND)return 1;    // not another mimic
		int r=1;
		c.level--;
		if(o.word)r=o.word(o,c);
		c.level++;
		return r;
	});
	OnBox([](ToadDef& d, ToadCtx& c, int i)->int
	{
		if(c.self==0||c.level<2)return 1;
		ToadDef& o=toadDef[c.toadNo[c.self-1]];
		if(o.cls==TC_FRIEND)return 1;
		int r=1;
		c.level--;
		if(o.box)r=o.box(o,c,i);
		c.level++;
		return r;
	});
	Def(TC_FRIEND,0,9,LG_BOTH,TS_IDEA,"Teacher toad","the other toads count one level higher");
	OnLevel([](ToadDef& d, ToadCtx& c, int pad)->int
	{
		if(pad!=c.self)return 1;
		return 0;
	});

	// tier 10: the run, and the hardest words
	Def(TC_META,10,10,LG_BOTH,TS_IDEA,"Easy toad","silver and gold targets 10% lower");
	Effect(TE_LOWER_BAR);
	Def(TC_META,20,10,LG_BOTH,TS_IDEA,"Lucky toad","20% chance of an extra prize after a game");
	Effect(TE_LUCKY);
	Def(TC_META,0,10,LG_BOTH,TS_IDEA,"Big pond toad","the next game's pond has one more row");
	Effect(TE_BIG_POND);
	Def(TC_SHAPE,0,10,LG_BOTH,TS_IDEA,"Mirror toad","a word that reads the same both ways");
	OnWord([](ToadDef& d, ToadCtx& c)->int
	{
		if(c.len<3)return 1;
		for(int i=0;i<c.len/2;i++)if(c.letter[i]!=c.letter[c.len-1-i])return 1;
		return 2*ToadX(c);
	});
	// themes need word lists first; no hook yet
	for(int k=0;k<3;k++)
		Def(TC_STREAK,k,10,LG_BOTH,TS_IDEA,"Theme toad","a word about % (animals, food, colours) scores more");

	// timed mode
	Def(TC_TIMED,3,7,LG_BOTH,TS_IDEA|TF_TIMED,"Clock toad","+3 seconds for each word");
	Effect(TE_EXTRA_TIME);
	Def(TC_TIMED,10,8,LG_BOTH,TS_IDEA|TF_TIMED,"Pause toad","the clock stops for 10 seconds, once a game");
	Effect(TE_PAUSE);
	Def(TC_TIMED,5,9,LG_BOTH,TS_IDEA|TF_TIMED,"Last second toad","a word in the last 5 seconds scores x(level+1)");
	OnWord([](ToadDef& d, ToadCtx& c)->int
	{
		if(c.timeLeft<=d.param)return ToadX(c);
		return 1;
	});
}

// ---------------------------------------------------------------- scoring
// How the hooks combine, the way alphatoad.cpp scores today: the numbers on
// the boxes, each times its box multipliers, summed; times every word
// multiplier; plus the flat points. Fills c.fired[] for the pads.

int ToadLevelOf(ToadCtx& c, int pad)
{
	int l=c.toadLevel[pad];
	for(int k=0;k<c.nToads;k++)
	{
		ToadDef& t=toadDef[c.toadNo[k]];
		if(t.levelUp)
		{
			c.self=k;
			c.level=c.toadLevel[k];
			l+=t.levelUp(t,c,pad);
		}
	}
	return l;
}

int ToadScore(ToadCtx& c)
{
	int lv[3];
	for(int k=0;k<c.nToads;k++)
	{
		lv[k]=ToadLevelOf(c,k);
		c.fired[k]=0;
	}
	int sum=0;
	for(int i=0;i<c.len;i++)
	{
		int v=c.num[i];
		for(int k=0;k<c.nToads;k++)
		{
			ToadDef& t=toadDef[c.toadNo[k]];
			if(t.box)
			{
				c.self=k;
				c.level=lv[k];
				int m=t.box(t,c,i);
				if(m>1)c.fired[k]=1;
				v=v*m;
			}
		}
		sum+=v;
	}
	// friends last, so they see who else was pleased
	int mult=1;
	for(int pass=0;pass<2;pass++)
		for(int k=0;k<c.nToads;k++)
		{
			ToadDef& t=toadDef[c.toadNo[k]];
			if((t.cls==TC_FRIEND)!=(pass==1))continue;
			if(t.word)
			{
				c.self=k;
				c.level=lv[k];
				int m=t.word(t,c);
				if(m>1)c.fired[k]=1;
				mult=mult*m;
			}
		}
	int plus=0;
	for(int k=0;k<c.nToads;k++)
	{
		ToadDef& t=toadDef[c.toadNo[k]];
		if(t.plus)
		{
			c.self=k;
			c.level=lv[k];
			int p=t.plus(t,c);
			if(p>0)c.fired[k]=1;
			plus+=p;
		}
	}
	return sum*mult+plus;
}

// the text for % in a toad's desc: the letter, length or ending, in the
// game's encoding (ASCII / Windows-1251); "" if the toad has none
void ToadArgText(ToadDef& d, int lang, char* out)
{
	out[0]=0;
	if(d.arg==TA_LENGTH)
	{
		int len=3+d.param;
		if(lang==TL_UA)len++;
		out[0]='0'+len;
		out[1]=0;
		return;
	}
	char* l="";
	if(d.arg==TA_LETTER)l=ToadList(tlLettersEn,tlLettersUa,lang);
	if(d.arg==TA_START)l=ToadList(tlStartsEn,tlStartsUa,lang);
	if(d.arg==TA_ENDING)l=ToadList(tlEndingsEn,tlEndingsUa,lang);
	if(d.arg==TA_RARE)l=ToadList(tlRareEn,tlRareUa,lang);
	if(d.arg==TA_PAIR)l=ToadList(tlPairsEn,tlPairsUa,lang);
	if(l[0]==0)return;
	int a[TOAD_W];
	int n=ToadListItem(l,d.param,a);
	for(int i=0;i<n;i++)out[i]=a[i];
	out[n]=0;
}
