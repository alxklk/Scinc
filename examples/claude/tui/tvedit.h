// A multi-window text editor on top of tv.h, with C/C++ syntax colours, undo, a
// clipboard, search and replace. Windows of type W_VIEW show one file each. A program
// forwards the hooks to DrawViewer, ViewerKey, ViewerMouse, EdCanClose, EdPaste, EdClosed
// and EdCommand, calls EdInit once and defines RunSaveAs to ask for a file name.
//
// All text lives in one pool of NUL-terminated lines, and every window owns a run of
// entries in the line tables. A line that outgrows its room moves to the end of the
// pool. The space it leaves is reclaimed when the pool fills up.
#ifndef TVEDIT_H
#define TVEDIT_H

#define W_VIEW 1

#define CM_NEW 150
#define CM_SAVE 151
#define CM_SAVEAS 152
#define CM_UNDO 153
#define CM_REDO 154
#define CM_CUT 155
#define CM_COPY 156
#define CM_PASTE 157
#define CM_SELALL 158
#define CM_FIND 159
#define CM_REPLACE 160
#define CM_FINDNEXT 161
#define CM_GOTO 162
#define CM_MATCH 163

#define VW_POOL 4000000
#define VW_MAXLINES 100000
#define ED_MAXLINE 4000
#define ED_PATH 1024
#define ED_UNDOTEXT 1000000
#define ED_UNDORECS 20000
#define ED_CLIPMAX 1000000
#define ED_TAB 4

#define AT_SELECT A_ED_SELECT

typedef char* EdChars;
typedef int* EdInts;

char* vwText;
char* vwSpare;      // takes the text while compacting, and a file while it loads
int* vwLine;        // where each line starts in vwText
int* vwLen;         // its length in bytes
int* vwCap;         // the room it has, with the NUL
int* vwState;       // the highlighter state it starts in, -1 if not known yet
int vwTextN=0;
int vwLineN=0;

int RunSaveAs(int win);

// ---------------------------------------------------------------- syntax highlighting

// The languages, the window's field f: C/C++, Python, and anything else, where
// numbers, punctuation and the common comments (#, // and /* */) are coloured.
// LANG_NONE (no colours) is only for files with lines too long to colour.
#define LANG_NONE 0
#define LANG_C 1
#define LANG_PY 2
#define LANG_OTHER 3

#define HL_TEXT A_ED_TEXT
#define HL_KEYWORD A_ED_KEYWORD
#define HL_COMMENT A_ED_COMMENT
#define HL_STRING A_ED_STRING
#define HL_NUMBER A_ED_NUMBER
#define HL_PREPROC A_ED_PREPROC
#define HL_SYMBOL A_ED_SYMBOL

// The state a line starts in, carried over from the line before it.
#define HS_NORMAL 0
#define HS_COMMENT 1  // inside /* */
#define HS_STRING 2   // a string continued with a backslash
#define HS_PREPROC 3  // a directive continued with a backslash
#define HS_TRIPLE2 4  // inside a Python """ string
#define HS_TRIPLE1 5  // inside a Python ''' string

int hlRun=0;        // the state while a file is loading
int hlAt[4100];     // the colour of each byte of the line last scanned

char* hlWords;       // the C/C++ keywords
char* hlPyWords;     // the Python ones
int hlFirst[256];    // where the words with this first letter start, or -1: C/C++ at
                     // 0..127, Python at 128..255

// Fills hlFirst[base..base+127] for a word list grouped by first letter.
void HlIndex(char* words, int base)
{
	for(int i=0;i<128;i++)
		hlFirst[base+i]=-1;
	int i=0;
	int atStart=1;
	while(words[i]!=0)
	{
		int c=words[i];
		if(atStart)
			if(hlFirst[base+c]<0)hlFirst[base+c]=i;
		atStart=(c=='|');
		i++;
	}
}

void HlInit()
{
	// grouped by first letter, which is all the lookup needs
	hlWords="asm|auto|bool|break|case|catch|char|class|const|constexpr|const_cast|continue|"
		"decltype|default|delete|do|double|dynamic_cast|else|enum|explicit|extern|false|float|"
		"for|friend|goto|if|inline|int|long|mutable|namespace|new|noexcept|nullptr|operator|"
		"private|protected|public|register|reinterpret_cast|return|short|signed|sizeof|static|"
		"static_cast|struct|switch|template|this|throw|true|try|typedef|typename|union|unsigned|"
		"using|virtual|void|volatile|wchar_t|while|";
	hlPyWords="False|None|True|and|as|assert|async|await|break|class|cls|continue|def|del|"
		"elif|else|except|finally|for|from|global|if|import|in|is|lambda|nonlocal|not|or|"
		"pass|raise|return|self|try|while|with|yield|";
	HlIndex(hlWords,0);
	HlIndex(hlPyWords,128);
}

// Is the n bytes of text at vwText[p] a keyword of lang (LANG_C or LANG_PY)?
int HlIsKeyword(int lang, int p, int n)
{
	int first=vwText[p];
	if(first>=128)return 0;
	char* words=hlWords;
	int w=hlFirst[first];
	if(lang==LANG_PY)
	{
		words=hlPyWords;
		w=hlFirst[128+first];
	}
	if(w<0)return 0;
	while(words[w]==first)
	{
		int k=0;
		while((k<n)&&(words[w+k]==vwText[p+k]))
			k++;
		if((k==n)&&(words[w+k]=='|'))
			return 1;
		while(words[w]!='|')
			w++;
		w++;
	}
	return 0;
}

int HlIdentStart(int c)
{
	if((c>='a')&&(c<='z'))return 1;
	if((c>='A')&&(c<='Z'))return 1;
	return c=='_';
}

int HlDigit(int c)
{
	return (c>='0')&&(c<='9');
}

// Colours a number starting at vwText[i] (0x1F, 1.5e-3, 1_000, suffixes) and
// returns where it ends.
int HlNumber(int start, int i)
{
	while(HlDigit(vwText[i])||HlIdentStart(vwText[i])||(vwText[i]=='.'))
	{
		int e=vwText[i];
		hlAt[i-start]=HL_NUMBER;
		i++;
		if((e=='e')||(e=='E'))
			if((vwText[i]=='-')||(vwText[i]=='+'))
			{
				hlAt[i-start]=HL_NUMBER;
				i++;
			}
	}
	return i;
}

// Colours the rest of the line from vwText[i] as a comment.
void HlToEnd(int start, int i)
{
	while(vwText[i]!=0)
	{
		hlAt[i-start]=HL_COMMENT;
		i++;
	}
}

// A byte that is not a letter, digit, space or UTF-8 is punctuation.
int HlPunct(int c)
{
	return (c>' ')&&(c<127);
}

// Colours the line starting at vwText[start] into hlAt and returns the state the
// next line starts in.
int HlLineC(int start, int state)
{
	int i=start;
	int pre=(state==HS_PREPROC);
	int blank=1;      // only spaces so far, so a # begins a directive
	int inString=(state==HS_STRING);
	int quote='"';
	int stringAt=HL_STRING;
	while(vwText[i]!=0)
	{
		int c=vwText[i];
		int n=vwText[i+1];
		if(state==HS_COMMENT)
		{
			hlAt[i-start]=HL_COMMENT;
			i++;
			if((c=='*')&&(n=='/'))
			{
				hlAt[i-start]=HL_COMMENT;
				i++;
				state=HS_NORMAL;
				if(pre)state=HS_PREPROC;
			}
		}
		else if(inString)
		{
			hlAt[i-start]=stringAt;
			i++;
			if((c==92)&&(n!=0))
			{
				hlAt[i-start]=stringAt;
				i++;
			}
			else if(c==quote)
			{
				inString=0;
				state=HS_NORMAL;
				if(pre)state=HS_PREPROC;
			}
		}
		else if((c=='/')&&(n=='/'))
		{
			while(vwText[i]!=0)
			{
				hlAt[i-start]=HL_COMMENT;
				i++;
			}
		}
		else if((c=='/')&&(n=='*'))
		{
			hlAt[i-start]=HL_COMMENT;
			hlAt[i+1-start]=HL_COMMENT;
			i=i+2;
			state=HS_COMMENT;
		}
		else if((c=='"')||(c==39))
		{
			quote=c;
			stringAt=HL_STRING;
			if(pre)stringAt=HL_PREPROC;
			inString=1;
			hlAt[i-start]=stringAt;
			i++;
		}
		else if(pre)
		{
			hlAt[i-start]=HL_PREPROC;
			i++;
		}
		else if((c=='#')&&blank)
		{
			pre=1;
			state=HS_PREPROC;
			hlAt[i-start]=HL_PREPROC;
			i++;
		}
		else if(HlDigit(c)||((c=='.')&&HlDigit(n)))i=HlNumber(start,i);
		else if(HlIdentStart(c))
		{
			int p=i;
			while(HlIdentStart(vwText[i])||HlDigit(vwText[i]))
				i++;
			int at=HL_TEXT;
			if(HlIsKeyword(LANG_C,p,i-p))at=HL_KEYWORD;
			for(int k=p;k<i;k++)
				hlAt[k-start]=at;
		}
		else
		{
			// spaces and UTF-8 bytes stay plain, the rest is punctuation
			hlAt[i-start]=HL_TEXT;
			if((c>' ')&&(c<127))hlAt[i-start]=HL_SYMBOL;
			i++;
		}
		if((c!=' ')&&(c!=9))blank=0;
	}
	if(state==HS_COMMENT)return HS_COMMENT;
	int continued=0;
	if(i>start)
		if(vwText[i-1]==92)continued=1;
	if(continued)
	{
		if(inString&&(quote=='"')&&!pre)return HS_STRING;
		if(pre)return HS_PREPROC;
	}
	return HS_NORMAL;
}

// Python: keywords, # comments, strings with their r/b/f/u prefixes, triple quoted
// strings across lines, @decorators, numbers and punctuation.
int HlLinePy(int start, int state)
{
	int i=start;
	int quote=0;        // the quote of the open string, 0 for none
	int triple=0;
	if(state==HS_TRIPLE2)
	{
		quote='"';
		triple=1;
	}
	if(state==HS_TRIPLE1)
	{
		quote=39;
		triple=1;
	}
	while(vwText[i]!=0)
	{
		int c=vwText[i];
		int n=vwText[i+1];
		if(quote!=0)
		{
			hlAt[i-start]=HL_STRING;
			i++;
			if((c==92)&&(n!=0))
			{
				hlAt[i-start]=HL_STRING;
				i++;
			}
			else if(c==quote)
			{
				if(!triple)quote=0;
				else if((vwText[i]==quote)&&(vwText[i+1]==quote))
				{
					hlAt[i-start]=HL_STRING;
					hlAt[i+1-start]=HL_STRING;
					i=i+2;
					quote=0;
				}
			}
		}
		else if(c=='#')
		{
			HlToEnd(start,i);
			while(vwText[i]!=0)i++;
		}
		else if((c=='"')||(c==39))
		{
			quote=c;
			triple=(n==c)&&(vwText[i+2]==c);
			int k=1;
			if(triple)k=3;
			for(int j=0;j<k;j++)
				hlAt[i+j-start]=HL_STRING;
			i=i+k;
		}
		else if(c=='@')
		{
			// a decorator: the @ and the dotted name after it
			hlAt[i-start]=HL_PREPROC;
			i++;
			while(HlIdentStart(vwText[i])||HlDigit(vwText[i])||(vwText[i]=='.'))
			{
				hlAt[i-start]=HL_PREPROC;
				i++;
			}
		}
		else if(HlDigit(c)||((c=='.')&&HlDigit(n)))i=HlNumber(start,i);
		else if(HlIdentStart(c))
		{
			int p=i;
			while(HlIdentStart(vwText[i])||HlDigit(vwText[i]))
				i++;
			int at=HL_TEXT;
			int q=vwText[i];
			if(((q=='"')||(q==39))&&(i-p<=2))
			{
				// a string prefix such as r, b, f, rb
				int all=1;
				for(int k=p;k<i;k++)
				{
					int l=LowerKey(vwText[k]);
					if((l!='r')&&(l!='b')&&(l!='f')&&(l!='u'))all=0;
				}
				if(all)at=HL_STRING;
			}
			if(at==HL_TEXT)
				if(HlIsKeyword(LANG_PY,p,i-p))at=HL_KEYWORD;
			for(int k=p;k<i;k++)
				hlAt[k-start]=at;
		}
		else
		{
			hlAt[i-start]=HL_TEXT;
			if(HlPunct(c))hlAt[i-start]=HL_SYMBOL;
			i++;
		}
	}
	if(triple&&(quote=='"'))return HS_TRIPLE2;
	if(triple&&(quote==39))return HS_TRIPLE1;
	return HS_NORMAL;
}

// Any other language: numbers and punctuation, and comments: // to the end of the
// line, /* */ across lines, and # to the end of the line where it starts the line
// or follows a space (so that #fff or a#b stay as they are).
int HlLineOther(int start, int state)
{
	int i=start;
	int prev=' ';
	while(vwText[i]!=0)
	{
		int c=vwText[i];
		int n=vwText[i+1];
		if(state==HS_COMMENT)
		{
			hlAt[i-start]=HL_COMMENT;
			i++;
			if((c=='*')&&(n=='/'))
			{
				hlAt[i-start]=HL_COMMENT;
				i++;
				state=HS_NORMAL;
			}
		}
		else if(((c=='/')&&(n=='/'))||((c=='#')&&((prev==' ')||(prev==9))))
		{
			HlToEnd(start,i);
			while(vwText[i]!=0)i++;
		}
		else if((c=='/')&&(n=='*'))
		{
			hlAt[i-start]=HL_COMMENT;
			hlAt[i+1-start]=HL_COMMENT;
			i=i+2;
			state=HS_COMMENT;
		}
		else if(HlDigit(c)||((c=='.')&&HlDigit(n)))i=HlNumber(start,i);
		else if(HlIdentStart(c))
		{
			// words stay plain, so that the digits in x86 or utf8 do too
			while(HlIdentStart(vwText[i])||HlDigit(vwText[i]))
			{
				hlAt[i-start]=HL_TEXT;
				i++;
			}
		}
		else
		{
			hlAt[i-start]=HL_TEXT;
			if(HlPunct(c))hlAt[i-start]=HL_SYMBOL;
			i++;
		}
		prev=vwText[i-1];
	}
	if(state==HS_COMMENT)return HS_COMMENT;
	return HS_NORMAL;
}

// Colours a line of lang into hlAt; returns the state the next line starts in.
int HlLine(int lang, int start, int state)
{
	if(lang==LANG_PY)return HlLinePy(start,state);
	if(lang==LANG_OTHER)return HlLineOther(start,state);
	return HlLineC(start,state);
}

// The extension of name, lower case, in ext (8 bytes), empty if none.
void HlExt(char* name, char* ext)
{
	int dot=-1;
	int k=0;
	while(name[k]!=0)
	{
		if(name[k]=='.')dot=k;
		k++;
	}
	int n=0;
	if(dot>=0)
	{
		k=dot+1;
		while((name[k]!=0)&&(n<7))
		{
			ext[n]=LowerKey(name[k]);
			n++;
			k++;
		}
	}
	ext[n]=0;
}

// Is ext one of the count names in the list known (separated by |)?
int HlExtIn(char* ext, char* known, int count)
{
	char item[8];
	for(int i=0;i<count;i++)
	{
		TvItemAt(known,i,&item[0],8);
		int same=1;
		int j=0;
		while((ext[j]!=0)||(item[j]!=0))
		{
			if(ext[j]!=item[j])same=0;
			if((ext[j]==0)||(item[j]==0))break;
			j++;
		}
		if(same)return 1;
	}
	return 0;
}

// The language of a file, from its name.
int HlLangOf(char* name)
{
	char ext[8];
	HlExt(name,&ext[0]);
	if(HlExtIn(&ext[0],"c|cc|cpp|cxx|h|hh|hpp|hxx|inl",9))return LANG_C;
	if(HlExtIn(&ext[0],"py|pyw|pyi",3))return LANG_PY;
	return LANG_OTHER;
}

// ---------------------------------------------------------------- the line pool

// Window fields: a top line, b left column, c first line in the tables, d line count,
// e the first wrapped row of line a shown, f the language (LANG_*) it is coloured as.

struct TEdit
{
	char path[ED_PATH];  // absolute, or empty for a new file
	int stamp;           // fs_stamp when last read or saved, -1 if not on disk
	int cx;              // the cursor, as a byte offset in line cy
	int cy;
	int want;            // the column up and down aim for, or -1
	int sel;             // set if the text between ax,ay and the cursor is selected
	int ax;
	int ay;
	int ro;              // read-only
	int crlf;            // lines end in CR LF
	int ver;             // the id of the undo record the text is at
	int savedVer;
	int baseId;          // the id for "at the start of the undo log"
	int recN;            // undo records in use, with redo ones
	int recTop;          // those not undone
	int txtN;
	int skipGroup;       // a group too large to log
	int lastKind;        // what the last edit was, for grouping typing into one undo
};

TEdit ed[TV_MAXWIN];
EdInts edRec[TV_MAXWIN];
EdChars edTxt[TV_MAXWIN];
int edHasLog[TV_MAXWIN];  // the log is allocated on a window's first edit
char edErr[300];
int edId=1;
int edGroup=1;
int edTooBig=0;
int edEndX=0;       // where the last insert ended
int edEndY=0;
char edTail[4100];
char edIndent[4100];
char* edClip;
int edClipN=0;
int edUntitled=0;
int edDrag=0;       // what a captured mouse is doing: 1 scroll thumb, 2 selecting
float edClickTime=0.;
int edClickX=-1;
int edClickY=-1;

void EdSetErr(char* s)
{
	StrCpy(&edErr[0],s,300);
}

void EdShowErr()
{
	TvMessage("Editor",&edErr[0],0);
}

// Copies every line to the spare pool, leaving out the gaps.
void VwCompact()
{
	int n=0;
	for(int i=0;i<vwLineN;i++)
	{
		int size=vwLen[i]+1;
		memmove(&vwSpare[n],&vwText[vwLine[i]],size);
		vwLine[i]=n;
		vwCap[i]=size;
		n=n+size;
	}
	char* swap=vwText;
	vwText=vwSpare;
	vwSpare=swap;
	vwTextN=n;
}

// Makes sure this many bytes can be taken from the pool. Check first, since this moves text.
int VwRoom(int bytes)
{
	if(vwTextN+bytes<=VW_POOL)return 1;
	VwCompact();
	if(vwTextN+bytes<=VW_POOL)return 1;
	EdSetErr("The text pool is full.");
	return 0;
}

// Moves line i to the end of the pool with room for need bytes. VwRoom has made sure of
// those, so only a lone line being typed in may take more, for the keys still to come.
void VwGrow(int i, int need, int spare)
{
	if(vwCap[i]>=need)return;
	int size=need;
	if(spare)
	{
		size=need+need/2+16;
		if(vwTextN+size>VW_POOL)size=need;
	}
	int at=vwTextN;
	vwTextN=vwTextN+size;
	memmove(&vwText[at],&vwText[vwLine[i]],vwLen[i]+1);
	vwLine[i]=at;
	vwCap[i]=size;
}

// Opens k empty table entries at index at. Windows further on move along.
void VwOpenLines(int at, int k)
{
	int size=(vwLineN-at)*4;
	memmove(&vwLine[at+k],&vwLine[at],size);
	memmove(&vwLen[at+k],&vwLen[at],size);
	memmove(&vwCap[at+k],&vwCap[at],size);
	memmove(&vwState[at+k],&vwState[at],size);
	vwLineN=vwLineN+k;
	for(int i=0;i<TV_MAXWIN;i++)
		if(tvWin[i].used)
			if((tvWin[i].type==W_VIEW)&&(tvWin[i].c>=at))
				tvWin[i].c=tvWin[i].c+k;
}

void VwCloseLines(int at, int k)
{
	int size=(vwLineN-at-k)*4;
	memmove(&vwLine[at],&vwLine[at+k],size);
	memmove(&vwLen[at],&vwLen[at+k],size);
	memmove(&vwCap[at],&vwCap[at+k],size);
	memmove(&vwState[at],&vwState[at+k],size);
	vwLineN=vwLineN-k;
	for(int i=0;i<TV_MAXWIN;i++)
		if(tvWin[i].used)
			if((tvWin[i].type==W_VIEW)&&(tvWin[i].c>at))
				tvWin[i].c=tvWin[i].c-k;
}

// Adds a line at the end of the newest window. Returns 0 if it does not fit.
int VwAppend(TWin& t, char* s, int n)
{
	if(vwLineN>=VW_MAXLINES)return 0;
	if(vwTextN+n+1>VW_POOL)return 0;
	int at=vwTextN;
	memmove(&vwText[at],s,n);
	vwText[at+n]=0;
	vwTextN=vwTextN+n+1;
	int i=vwLineN;
	vwLine[i]=at;
	vwLen[i]=n;
	vwCap[i]=n+1;
	vwState[i]=-1;
	vwLineN++;
	t.d++;
	return 1;
}

void VwAddLine(TWin& t, char* s)
{
	VwAppend(t,s,StrLen(s));
}

// Recomputes line states from line y on, until they agree with what follows.
void EdRehl(TWin& t, int y)
{
	if(!t.f)return;
	int state=vwState[t.c+y];
	if(state<0)state=HS_NORMAL;
	while(y<t.d)
	{
		vwState[t.c+y]=state;
		state=HlLine(t.f,vwLine[t.c+y],state);
		y++;
		if(y<t.d)
			if(vwState[t.c+y]==state)break;
	}
}

void EdRehlAll(TWin& t)
{
	int state=HS_NORMAL;
	for(int y=0;y<t.d;y++)
	{
		vwState[t.c+y]=state;
		if(t.f)state=HlLine(t.f,vwLine[t.c+y],state);
	}
}

// ---------------------------------------------------------------- edits without undo

// Inserts n bytes, which may hold line ends, at line y byte x. All or nothing.
int EdRawInsert(int win, int y, int x, char* s, int n)
{
	TWin& t=tvWin[win];
	int i=t.c+y;
	int len=vwLen[i];
	int k=0;
	int seg=0;
	int firstSeg=-1;
	int tooLong=0;
	for(int o=0;o<n;o++)
	{
		int c=s[o];
		if(c==10)
		{
			if(firstSeg<0)firstSeg=seg;
			else if(seg>ED_MAXLINE)tooLong=1;
			k++;
			seg=0;
		}
		else seg++;
	}
	if(k==0)
	{
		if(len+n>ED_MAXLINE)tooLong=1;
	}
	else
	{
		if(x+firstSeg>ED_MAXLINE)tooLong=1;
		if(seg+len-x>ED_MAXLINE)tooLong=1;
	}
	if(tooLong)
	{
		snprintf(&edErr[0],300,"A line would be longer than the limit of %i bytes.",ED_MAXLINE);
		return 0;
	}
	if(vwLineN+k>VW_MAXLINES)
	{
		EdSetErr("There are too many lines.");
		return 0;
	}
	if(!VwRoom(len+n+k+16))return 0;
	if(k==0)
	{
		VwGrow(i,len+n+1,1);
		int p=vwLine[i];
		memmove(&vwText[p+x+n],&vwText[p+x],len-x+1);
		memmove(&vwText[p+x],s,n);
		vwLen[i]=len+n;
		edEndY=y;
		edEndX=x+n;
		EdRehl(t,y);
		return 1;
	}
	int tail=len-x;
	memmove(&edTail[0],&vwText[vwLine[i]+x],tail);
	VwGrow(i,x+firstSeg+1,0);
	int p=vwLine[i];
	memmove(&vwText[p+x],s,firstSeg);
	vwText[p+x+firstSeg]=0;
	vwLen[i]=x+firstSeg;
	VwOpenLines(i+1,k);
	t.d=t.d+k;
	int o=firstSeg+1;
	for(int j=1;j<=k;j++)
	{
		int e=o;
		while(e<n)
		{
			int c=s[e];
			if(c==10)break;
			e++;
		}
		int size=e-o;
		int total=size;
		if(j==k)total=size+tail;
		int at=vwTextN;
		vwTextN=vwTextN+total+1;
		memmove(&vwText[at],&s[o],size);
		if(j==k)memmove(&vwText[at+size],&edTail[0],tail);
		vwText[at+total]=0;
		vwLine[i+j]=at;
		vwLen[i+j]=total;
		vwCap[i+j]=total+1;
		vwState[i+j]=-1;
		if(j==k)
		{
			edEndY=y+k;
			edEndX=size;
		}
		o=e+1;
	}
	EdRehl(t,y);
	return 1;
}

// Can the text between these positions go? Joining lines may be too long.
int EdCanDelete(TWin& t, int y1, int x1, int y2, int x2)
{
	if(y1==y2)return 1;
	int total=x1+vwLen[t.c+y2]-x2;
	if(total>ED_MAXLINE)
	{
		snprintf(&edErr[0],300,"A line would be longer than the limit of %i bytes.",ED_MAXLINE);
		return 0;
	}
	return VwRoom(total+16);
}

// EdCanDelete comes first.
void EdRawDelete(int win, int y1, int x1, int y2, int x2)
{
	TWin& t=tvWin[win];
	int i1=t.c+y1;
	int i2=t.c+y2;
	if(y1==y2)
	{
		int p=vwLine[i1];
		memmove(&vwText[p+x1],&vwText[p+x2],vwLen[i1]-x2+1);
		vwLen[i1]=vwLen[i1]-(x2-x1);
	}
	else
	{
		int tail=vwLen[i2]-x2;
		VwGrow(i1,x1+tail+1,0);
		memmove(&vwText[vwLine[i1]+x1],&vwText[vwLine[i2]+x2],tail+1);
		vwLen[i1]=x1+tail;
		VwCloseLines(i1+1,y2-y1);
		t.d=t.d-(y2-y1);
	}
	EdRehl(t,y1);
}

// The bytes of the text between two positions, as EdGetText writes it.
int EdTextLen(TWin& t, int y1, int x1, int y2, int x2)
{
	if(y1==y2)return x2-x1;
	int n=vwLen[t.c+y1]-x1+1;
	for(int y=y1+1;y<y2;y++)
		n=n+vwLen[t.c+y]+1;
	return n+x2;
}

void EdGetText(TWin& t, int y1, int x1, int y2, int x2, char* out)
{
	int o=0;
	for(int y=y1;y<=y2;y++)
	{
		int from=0;
		if(y==y1)from=x1;
		int to=vwLen[t.c+y];
		if(y==y2)to=x2;
		memmove(&out[o],&vwText[vwLine[t.c+y]+from],to-from);
		o=o+to-from;
		if(y<y2)
		{
			out[o]=10;
			o++;
		}
	}
}

// ---------------------------------------------------------------- undo log

#define R_KIND 0
#define R_Y 1
#define R_X 2
#define R_LEN 3
#define R_OFF 4
#define R_GROUP 5
#define R_ID 6
#define R_SIZE 7

#define U_INSERT 1
#define U_DELETE 2

// Forgets the log. The text no longer matches any id, so it counts as modified.
void EdUndoClear(TEdit& e)
{
	e.recN=0;
	e.recTop=0;
	e.txtN=0;
	edId++;
	e.baseId=edId;
	e.ver=edId;
}

// Drops older groups so that the log has room. An undo must take back a whole group,
// so a group that does not fit goes unlogged and the log is cleared.
void EdUndoTrim(int win, int needText)
{
	TEdit& e=ed[win];
	int* rec=edRec[win];
	char* txt=edTxt[win];
	int cut=e.recN/2;
	if(cut<1)cut=1;
	// && would read past the records, so the bound is checked inside
	while(cut<e.recN)
	{
		if(e.txtN-rec[cut*R_SIZE+R_OFF]+needText<=ED_UNDOTEXT)break;
		cut++;
	}
	while(cut<e.recN)
	{
		if(rec[cut*R_SIZE+R_GROUP]!=rec[(cut-1)*R_SIZE+R_GROUP])break;
		cut++;
	}
	if(cut>=e.recN)
	{
		int group=edGroup;
		int partOfGroup=0;
		if(e.recN>0)
			if(rec[(e.recN-1)*R_SIZE+R_GROUP]==group)partOfGroup=1;
		EdUndoClear(e);
		if(partOfGroup)
		{
			e.skipGroup=group;
			edTooBig=1;
		}
		return;
	}
	int delta=rec[cut*R_SIZE+R_OFF];
	e.baseId=rec[(cut-1)*R_SIZE+R_ID];
	memmove(&txt[0],&txt[delta],e.txtN-delta);
	e.txtN=e.txtN-delta;
	memmove(&rec[0],&rec[cut*R_SIZE],(e.recN-cut)*R_SIZE*4);
	e.recN=e.recN-cut;
	for(int i=0;i<e.recN;i++)
		rec[i*R_SIZE+R_OFF]=rec[i*R_SIZE+R_OFF]-delta;
	e.recTop=e.recN;
}

void EdLog(int win, int kind, int y, int x, char* s, int n)
{
	TEdit& e=ed[win];
	if(!edHasLog[win])
	{
		int* newRec=malloc(ED_UNDORECS*R_SIZE*4);
		char* newTxt=malloc(ED_UNDOTEXT);
		edRec[win]=newRec;
		edTxt[win]=newTxt;
		edHasLog[win]=1;
	}
	// anything that could be redone no longer can be
	e.recN=e.recTop;
	if(e.recN>0)
	{
		int* top=edRec[win];
		e.txtN=top[(e.recN-1)*R_SIZE+R_OFF]+top[(e.recN-1)*R_SIZE+R_LEN];
	}
	else e.txtN=0;
	if(n>ED_UNDOTEXT)
	{
		EdUndoClear(e);
		if(e.skipGroup!=edGroup)edTooBig=1;
		e.skipGroup=edGroup;
		return;
	}
	if(e.skipGroup==edGroup)
	{
		EdUndoClear(e);
		return;
	}
	while((e.recN>=ED_UNDORECS)||(e.txtN+n>ED_UNDOTEXT))
	{
		EdUndoTrim(win,n);
		if(e.skipGroup==edGroup)return;
	}
	int* rec=edRec[win];
	char* txt=edTxt[win];
	int r=e.recN*R_SIZE;
	memmove(&txt[e.txtN],s,n);
	edId++;
	rec[r+R_KIND]=kind;
	rec[r+R_Y]=y;
	rec[r+R_X]=x;
	rec[r+R_LEN]=n;
	rec[r+R_OFF]=e.txtN;
	rec[r+R_GROUP]=edGroup;
	rec[r+R_ID]=edId;
	e.txtN=e.txtN+n;
	e.recN++;
	e.recTop=e.recN;
	e.ver=edId;
}

int EdInsert(int win, int y, int x, char* s, int n)
{
	if(n==0)
	{
		edEndY=y;
		edEndX=x;
		return 1;
	}
	if(!EdRawInsert(win,y,x,s,n))return 0;
	int ey=edEndY;
	int ex=edEndX;
	EdLog(win,U_INSERT,y,x,s,n);
	edEndY=ey;
	edEndX=ex;
	return 1;
}

int EdDelete(int win, int y1, int x1, int y2, int x2)
{
	TWin& t=tvWin[win];
	if((y1==y2)&&(x1==x2))return 1;
	if(!EdCanDelete(t,y1,x1,y2,x2))return 0;
	int n=EdTextLen(t,y1,x1,y2,x2);
	// the spare pool is free between compactions, and too much for the log is not copied
	if(n<=ED_UNDOTEXT)EdGetText(t,y1,x1,y2,x2,vwSpare);
	EdLog(win,U_DELETE,y1,x1,vwSpare,n);
	EdRawDelete(win,y1,x1,y2,x2);
	return 1;
}

// Where inserting these n bytes at y,x ends.
void EdEndOf(char* s, int n, int y, int x)
{
	edEndY=y;
	edEndX=x;
	for(int o=0;o<n;o++)
	{
		int c=s[o];
		if(c==10)
		{
			edEndY++;
			edEndX=0;
		}
		else edEndX++;
	}
}

void EdSetCursor(int win, int y, int x)
{
	TEdit& e=ed[win];
	e.cy=y;
	e.cx=x;
	e.sel=0;
	e.want=-1;
}

// Takes back the newest group of edits.
int EdUndo(int win)
{
	TEdit& e=ed[win];
	if(e.recTop==0)return 0;
	int* rec=edRec[win];
	char* txt=edTxt[win];
	int group=rec[(e.recTop-1)*R_SIZE+R_GROUP];
	while(e.recTop>0)
	{
		int r=(e.recTop-1)*R_SIZE;
		if(rec[r+R_GROUP]!=group)break;
		int y=rec[r+R_Y];
		int x=rec[r+R_X];
		int n=rec[r+R_LEN];
		int off=rec[r+R_OFF];
		if(rec[r+R_KIND]==U_INSERT)
		{
			EdEndOf(&txt[off],n,y,x);
			int ey=edEndY;
			int ex=edEndX;
			if(!EdCanDelete(tvWin[win],y,x,ey,ex))return 0;
			EdRawDelete(win,y,x,ey,ex);
			EdSetCursor(win,y,x);
		}
		else
		{
			if(!EdRawInsert(win,y,x,&txt[off],n))return 0;
			EdSetCursor(win,edEndY,edEndX);
		}
		e.recTop--;
	}
	if(e.recTop>0)e.ver=rec[(e.recTop-1)*R_SIZE+R_ID];
	else e.ver=e.baseId;
	e.lastKind=0;
	return 1;
}

int EdRedo(int win)
{
	TEdit& e=ed[win];
	if(e.recTop>=e.recN)return 0;
	int* rec=edRec[win];
	char* txt=edTxt[win];
	int group=rec[e.recTop*R_SIZE+R_GROUP];
	while(e.recTop<e.recN)
	{
		int r=e.recTop*R_SIZE;
		if(rec[r+R_GROUP]!=group)break;
		int y=rec[r+R_Y];
		int x=rec[r+R_X];
		int n=rec[r+R_LEN];
		int off=rec[r+R_OFF];
		if(rec[r+R_KIND]==U_INSERT)
		{
			if(!EdRawInsert(win,y,x,&txt[off],n))return 0;
			EdSetCursor(win,edEndY,edEndX);
		}
		else
		{
			EdEndOf(&txt[off],n,y,x);
			int ey=edEndY;
			int ex=edEndX;
			if(!EdCanDelete(tvWin[win],y,x,ey,ex))return 0;
			EdRawDelete(win,y,x,ey,ex);
			EdSetCursor(win,y,x);
		}
		e.ver=rec[r+R_ID];
		e.recTop++;
	}
	e.lastKind=0;
	return 1;
}

int EdModified(int win)
{
	return ed[win].ver!=ed[win].savedVer;
}

// ---------------------------------------------------------------- cursor

// The screen column of byte x in table line i.
int EdCol(int i, int x)
{
	int p=vwLine[i];
	int end=p+x;
	int col=0;
	while(p<end)
	{
		int c=TvUtf8(vwText,p);
		if(c==9)col=(col/ED_TAB+1)*ED_TAB;
		else col++;
	}
	return col;
}

// The byte in table line i at this screen column. Inside a tab it is the tab itself.
int EdByteAt(int i, int want)
{
	int start=vwLine[i];
	int p=start;
	int col=0;
	while(vwText[p]!=0)
	{
		int q=p;
		int c=TvUtf8(vwText,q);
		int next=col+1;
		if(c==9)next=(col/ED_TAB+1)*ED_TAB;
		if(next>want)break;
		col=next;
		p=q;
	}
	return p-start;
}

// ---------------------------------------------------------------- line numbers and wrapping

int edLineNums=0;    // line numbers on the left of all editor windows
int edWrap=0;        // long lines wrap at the window edge, breaking after a space
int edRowAt[4100];   // where each row of the line last wrapped starts, as byte offsets
int edRowN=1;

// The columns that the line numbers take.
int EdGutter(TWin& t)
{
	if(!edLineNums)return 0;
	int g=2;
	int n=t.d;
	while(n>=10)
	{
		n=n/10;
		g++;
	}
	if(g<4)g=4;
	return g;
}

int EdCols(TWin& t)
{
	int c=t.w-2-EdGutter(t);
	if(c<2)c=2;
	return c;
}

// Splits table line i into rows of cols columns (one row when not wrapping), in edRowAt.
void EdWrapLine(int i, int cols)
{
	edRowN=1;
	edRowAt[0]=0;
	if(!edWrap)return;
	int start=vwLine[i];
	int p=start;
	int col=0;
	int rowCol=0;
	int brk=-1;
	int brkCol=0;
	while(vwText[p]!=0)
	{
		int q=p;
		int c=TvUtf8(vwText,q);
		int next=col+1;
		if(c==9)next=(col/ED_TAB+1)*ED_TAB;
		if((next-rowCol>cols)&&(edRowN<4096))
			if(p-start>edRowAt[edRowN-1])
			{
				int at=p-start;
				int atCol=col;
				if(brk>edRowAt[edRowN-1])
				{
					at=brk;
					atCol=brkCol;
				}
				edRowAt[edRowN]=at;
				edRowN++;
				rowCol=atCol;
				brk=-1;
			}
		if((c==' ')||(c==9))
		{
			brk=q-start;
			brkCol=next;
		}
		col=next;
		p=q;
	}
}

// The row of the last wrapped line that byte x is on.
int EdRowOf(int x)
{
	int r=0;
	while(r+1<edRowN)
	{
		if(edRowAt[r+1]>x)break;
		r++;
	}
	return r;
}

// The byte in row r of the last wrapped line, table line i, nearest to column want of the row.
int EdByteInRow(int i, int r, int want)
{
	int x=EdByteAt(i,EdCol(i,edRowAt[r])+want);
	if(r+1<edRowN)
		if(x>=edRowAt[r+1])x=TvPrevChar(&vwText[vwLine[i]],edRowAt[r+1]);
	return x;
}

// Scrolls so that the cursor is in view.
void EdReveal(int win)
{
	TWin& t=tvWin[win];
	TEdit& e=ed[win];
	int rows=t.h-2;
	int cols=EdCols(t);
	if(edWrap)
	{
		t.b=0;
		EdWrapLine(t.c+e.cy,cols);
		int cr=EdRowOf(e.cx);
		if(e.cy<t.a)
		{
			t.a=e.cy;
			t.e=cr;
			return;
		}
		if((e.cy==t.a)&&(cr<t.e))
		{
			t.e=cr;
			return;
		}
		// the screen row of the cursor
		int n=0;
		int y=t.a;
		int sub=t.e;
		while((y<e.cy)&&(n<rows))
		{
			EdWrapLine(t.c+y,cols);
			n=n+edRowN-sub;
			sub=0;
			y++;
		}
		if(y==e.cy)n=n+cr-sub;
		if(n<rows)return;
		// it goes on the last row: count rows back up from it
		int need=rows-1;
		y=e.cy;
		sub=cr;
		while(need>0)
		{
			if(sub>0)
			{
				sub--;
				need--;
			}
			else if(y>0)
			{
				y--;
				EdWrapLine(t.c+y,cols);
				sub=edRowN;
			}
			else break;
		}
		t.a=y;
		t.e=sub;
		return;
	}
	t.e=0;
	if(e.cy<t.a)t.a=e.cy;
	if(e.cy>=t.a+rows)t.a=e.cy-rows+1;
	int col=EdCol(t.c+e.cy,e.cx);
	if(col<t.b)t.b=col;
	if(col>=t.b+cols)t.b=col-cols+1;
}

// ---------------------------------------------------------------- matching brackets

int edBmY=-1;        // the bracket at the cursor, or -1
int edBmX=0;
int edBmY2=-1;       // and its partner, or -1 if it has none
int edBmX2=0;

int EdBracket(int c)
{
	if((c=='(')||(c=='[')||(c=='{'))return 1;
	if((c==')')||(c==']')||(c=='}'))return -1;
	return 0;
}

// Finds the bracket at or just before the cursor and its partner, skipping
// brackets in comments and strings when the syntax is known.
void EdFindBrackets(int win)
{
	TWin& t=tvWin[win];
	TEdit& e=ed[win];
	edBmY=-1;
	edBmY2=-1;
	int i=t.c+e.cy;
	int start=vwLine[i];
	int x=-1;
	if(e.cx<vwLen[i])
		if(EdBracket(vwText[start+e.cx]))x=e.cx;
	if(x<0)
		if(e.cx>0)
			if(EdBracket(vwText[start+e.cx-1]))x=e.cx-1;
	if(x<0)return;
	if(t.f)
	{
		HlLine(t.f,start,vwState[i]);
		if((hlAt[x]==HL_COMMENT)||(hlAt[x]==HL_STRING))return;
	}
	int c=vwText[start+x];
	int dir=EdBracket(c);
	int other=')';
	if(c=='[')other=']';
	if(c=='{')other='}';
	if(c==')')other='(';
	if(c==']')other='[';
	if(c=='}')other='{';
	edBmY=e.cy;
	edBmX=x;
	int depth=0;
	int y=e.cy;
	int p=x+dir;
	for(int n=0;n<3000;n++)
	{
		int li=t.c+y;
		int ls=vwLine[li];
		if(n>0)
		{
			if(t.f)HlLine(t.f,ls,vwState[li]);
			if(dir>0)p=0;
			else p=vwLen[li]-1;
		}
		while((p>=0)&&(p<vwLen[li]))
		{
			int b=vwText[ls+p];
			int skip=0;
			if(t.f)skip=(hlAt[p]==HL_COMMENT)||(hlAt[p]==HL_STRING);
			if(!skip)
			{
				if(b==c)depth++;
				else if(b==other)
				{
					if(depth==0)
					{
						edBmY2=y;
						edBmX2=p;
						return;
					}
					depth--;
				}
			}
			p=p+dir;
		}
		y=y+dir;
		if((y<0)||(y>=t.d))return;
	}
}

// The selection in text order, in these.
int edY1=0;
int edX1=0;
int edY2=0;
int edX2=0;

void EdSelRange(TEdit& e)
{
	int forward=(e.ay<e.cy);
	if(e.ay==e.cy)forward=(e.ax<=e.cx);
	if(forward)
	{
		edY1=e.ay;
		edX1=e.ax;
		edY2=e.cy;
		edX2=e.cx;
	}
	else
	{
		edY1=e.cy;
		edX1=e.cx;
		edY2=e.ay;
		edX2=e.ax;
	}
}

// Moves the cursor, extending the selection or dropping it.
void EdMoveTo(int win, int y, int x, int extend)
{
	TEdit& e=ed[win];
	if(extend)
	{
		if(!e.sel)
		{
			e.sel=1;
			e.ay=e.cy;
			e.ax=e.cx;
		}
	}
	else e.sel=0;
	e.cy=y;
	e.cx=x;
	if(e.sel)
		if((e.ay==y)&&(e.ax==x))e.sel=0;
	e.lastKind=0;
	EdReveal(win);
}

int EdIsWord(int c)
{
	if(c>=0x80)return 1;
	return HlIdentStart(c)||HlDigit(c);
}

void EdWordLeft(int win, int extend)
{
	TWin& t=tvWin[win];
	TEdit& e=ed[win];
	if(e.cx==0)
	{
		if(e.cy>0)EdMoveTo(win,e.cy-1,vwLen[t.c+e.cy-1],extend);
		return;
	}
	int p=vwLine[t.c+e.cy];
	int x=e.cx;
	while(x>0)
	{
		int c=vwText[p+x-1];
		if(c>' ')break;
		x--;
	}
	if(x>0)
	{
		int c=vwText[p+x-1];
		int word=EdIsWord(c);
		while(x>0)
		{
			int k=vwText[p+x-1];
			if(EdIsWord(k)!=word)break;
			if(k<=' ')break;
			x--;
		}
	}
	EdMoveTo(win,e.cy,x,extend);
}

void EdWordRight(int win, int extend)
{
	TWin& t=tvWin[win];
	TEdit& e=ed[win];
	int i=t.c+e.cy;
	int len=vwLen[i];
	if(e.cx>=len)
	{
		if(e.cy<t.d-1)EdMoveTo(win,e.cy+1,0,extend);
		return;
	}
	int p=vwLine[i];
	int x=e.cx;
	int first=vwText[p+x];
	if(first>' ')
	{
		int word=EdIsWord(first);
		while(x<len)
		{
			int k=vwText[p+x];
			if(EdIsWord(k)!=word)break;
			if(k<=' ')break;
			x++;
		}
	}
	while(x<len)
	{
		int c=vwText[p+x];
		if(c>' ')break;
		x++;
	}
	EdMoveTo(win,e.cy,x,extend);
}

// Selects the word at the cursor, for a double click.
void EdSelectWord(int win)
{
	TWin& t=tvWin[win];
	TEdit& e=ed[win];
	int i=t.c+e.cy;
	int p=vwLine[i];
	int len=vwLen[i];
	int a=e.cx;
	int b=e.cx;
	while(a>0)
	{
		int c=vwText[p+a-1];
		if(!EdIsWord(c))break;
		a--;
	}
	while(b<len)
	{
		int c=vwText[p+b];
		if(!EdIsWord(c))break;
		b++;
	}
	if(a==b)return;
	e.ay=e.cy;
	e.ax=a;
	e.cx=b;
	e.sel=1;
	e.want=-1;
	EdReveal(win);
}

// ---------------------------------------------------------------- edits with the cursor

void EdNewGroup(TEdit& e)
{
	edGroup++;
	e.lastKind=0;
}

// Runs of typing or of erasing undo as one step.
void EdKindGroup(TEdit& e, int kind)
{
	if(e.lastKind!=kind)edGroup++;
	e.lastKind=kind;
}

void EdAfterEdit(int win)
{
	if(edTooBig)
	{
		edTooBig=0;
		EdReveal(win);
		TvMessage("Editor","That change was too large to undo.",0);
	}
	EdReveal(win);
}

int EdDeleteSel(int win)
{
	TEdit& e=ed[win];
	if(!e.sel)return 1;
	EdSelRange(e);
	if(!EdDelete(win,edY1,edX1,edY2,edX2))return 0;
	EdSetCursor(win,edY1,edX1);
	return 1;
}

// Replaces the selection with this text, within the current undo group.
int EdPutText(int win, char* s, int n)
{
	TEdit& e=ed[win];
	int hadSel=e.sel;
	if(!EdDeleteSel(win))return 0;
	if(!EdInsert(win,e.cy,e.cx,s,n))
	{
		if(hadSel)EdUndo(win);
		return 0;
	}
	EdSetCursor(win,edEndY,edEndX);
	return 1;
}

void EdEraseBack(int win)
{
	TWin& t=tvWin[win];
	TEdit& e=ed[win];
	if(e.sel)
	{
		EdNewGroup(e);
		if(!EdDeleteSel(win))EdShowErr();
		return;
	}
	EdKindGroup(e,2);
	if(e.cx>0)
	{
		int i=t.c+e.cy;
		// counted from the line start, since UTF-8 may be damaged
		int p=vwLine[i];
		int q=p;
		int end=p+e.cx;
		int prev=p;
		while(q<end)
		{
			prev=q;
			TvUtf8(vwText,q);
		}
		int x=prev-p;
		EdDelete(win,e.cy,x,e.cy,e.cx);
		EdSetCursor(win,e.cy,x);
	}
	else if(e.cy>0)
	{
		int y=e.cy-1;
		int x=vwLen[t.c+y];
		if(EdDelete(win,y,x,e.cy,0))EdSetCursor(win,y,x);
		else EdShowErr();
	}
}

void EdEraseForward(int win)
{
	TWin& t=tvWin[win];
	TEdit& e=ed[win];
	if(e.sel)
	{
		EdNewGroup(e);
		if(!EdDeleteSel(win))EdShowErr();
		return;
	}
	EdKindGroup(e,3);
	int i=t.c+e.cy;
	if(e.cx<vwLen[i])
	{
		int q=vwLine[i]+e.cx;
		int from=q;
		TvUtf8(vwText,q);
		EdDelete(win,e.cy,e.cx,e.cy,e.cx+q-from);
	}
	else if(e.cy<t.d-1)
	{
		if(!EdDelete(win,e.cy,e.cx,e.cy+1,0))EdShowErr();
	}
	e.want=-1;
}

// A new line starts with the indentation of the one it came from.
void EdEnter(int win)
{
	TWin& t=tvWin[win];
	TEdit& e=ed[win];
	EdNewGroup(e);
	if(!EdDeleteSel(win))
	{
		EdShowErr();
		return;
	}
	int p=vwLine[t.c+e.cy];
	int n=0;
	edIndent[0]=10;
	while(n<e.cx)
	{
		int c=vwText[p+n];
		if((c!=' ')&&(c!=9))break;
		edIndent[n+1]=c;
		n++;
	}
	// one more level after an opening brace in C, or a colon in Python (comments
	// after it aside), with spaces if the line uses them or, in Python, always
	if((t.f==LANG_C)||(t.f==LANG_PY))
	{
		if(t.f==LANG_PY)HlLine(t.f,p,vwState[t.c+e.cy]);
		int b=e.cx-1;
		while(b>=0)
		{
			int c=vwText[p+b];
			int skip=(c==' ')||(c==9);
			if(t.f==LANG_PY)
				if(hlAt[b]==HL_COMMENT)skip=1;
			if(!skip)break;
			b--;
		}
		int opens=0;
		if(b>=0)
		{
			if((t.f==LANG_C)&&(vwText[p+b]=='{'))opens=1;
			if((t.f==LANG_PY)&&(vwText[p+b]==':')&&(hlAt[b]==HL_SYMBOL))opens=1;
		}
		if(opens)
		{
			if(((n>0)&&(edIndent[1]==' '))||((n==0)&&(t.f==LANG_PY)))
			{
				for(int k=0;k<ED_TAB;k++)
					edIndent[n+1+k]=' ';
				n=n+ED_TAB;
			}
			else
			{
				edIndent[n+1]=9;
				n++;
			}
		}
	}
	if(EdInsert(win,e.cy,e.cx,&edIndent[0],n+1))EdSetCursor(win,edEndY,edEndX);
	else EdShowErr();
	EdNewGroup(e);
}

void EdType(int win, char* s, int n, int isSpace)
{
	TEdit& e=ed[win];
	if(e.sel)EdNewGroup(e);
	else
	{
		EdKindGroup(e,1);
		// a word at a time
		if(isSpace)edGroup++;
	}
	if(!EdPutText(win,s,n))EdShowErr();
	e.lastKind=1;
}

// The lines a Tab on a selection works on, in edY1..edY2.
void EdSelLines(TEdit& e)
{
	if(e.sel)
	{
		EdSelRange(e);
		if((edX2==0)&&(edY2>edY1))edY2--;
	}
	else
	{
		edY1=e.cy;
		edY2=e.cy;
	}
}

void EdIndentLines(int win)
{
	TWin& t=tvWin[win];
	TEdit& e=ed[win];
	EdSelLines(e);
	int y1=edY1;
	int y2=edY2;
	char tab[2];
	tab[0]=9;
	tab[1]=0;
	EdNewGroup(e);
	for(int y=y1;y<=y2;y++)
	{
		if(vwLen[t.c+y]>0)
		{
			if(!EdInsert(win,y,0,&tab[0],1))
			{
				EdShowErr();
				break;
			}
			if(y==e.cy)e.cx++;
			if(y==e.ay)e.ax++;
		}
	}
	EdNewGroup(e);
}

void EdUnindentLines(int win)
{
	TWin& t=tvWin[win];
	TEdit& e=ed[win];
	EdSelLines(e);
	int y1=edY1;
	int y2=edY2;
	EdNewGroup(e);
	for(int y=y1;y<=y2;y++)
	{
		int i=t.c+y;
		int p=vwLine[i];
		int n=0;
		int first=vwText[p];
		if(first==9)n=1;
		else
			while(n<ED_TAB)
			{
				int c=vwText[p+n];
				if(c!=' ')break;
				n++;
			}
		if(n>0)
		{
			EdDelete(win,y,0,y,n);
			if(y==e.cy)
			{
				e.cx=e.cx-n;
				if(e.cx<0)e.cx=0;
			}
			if(y==e.ay)
			{
				e.ax=e.ax-n;
				if(e.ax<0)e.ax=0;
			}
		}
	}
	EdNewGroup(e);
}

void EdCopy(int win)
{
	TWin& t=tvWin[win];
	TEdit& e=ed[win];
	if(!e.sel)return;
	EdSelRange(e);
	int n=EdTextLen(t,edY1,edX1,edY2,edX2);
	if(n>ED_CLIPMAX)
	{
		TvMessage("Copy","The selection is too large for the clipboard.",0);
		return;
	}
	EdGetText(t,edY1,edX1,edY2,edX2,edClip);
	edClipN=n;
}

// ---------------------------------------------------------------- opening and saving

// Where the part of the path after the last slash starts.
int EdBase(char* path)
{
	int base=0;
	int k=0;
	while(path[k]!=0)
	{
		if(path[k]=='/')base=k+1;
		k++;
	}
	return base;
}

// As fs_abspath, and -2 if the result would be cut short.
int EdAbsPath(char* path, char* out)
{
	int r=fs_abspath(path,out,ED_PATH);
	if(r>=0)
		if(StrLen(out)>=ED_PATH-1)return -2;
	return r;
}

void EdResetWindow(int win)
{
	TEdit& e=ed[win];
	e.path[0]=0;
	e.stamp=-1;
	e.cx=0;
	e.cy=0;
	e.want=-1;
	e.sel=0;
	e.ax=0;
	e.ay=0;
	e.ro=0;
	e.crlf=0;
	e.skipGroup=-1;
	e.lastKind=0;
	EdUndoClear(e);
	e.savedVer=e.ver;
}

int NewViewer(char* title)
{
	int win=TvNew(W_VIEW,title,0,1,10,10,WF_RESIZE);
	if(win<0)return -1;
	TWin& t=tvWin[win];
	int n=tvNZ-1;
	t.x=n*2;
	t.y=1+n;
	t.w=tvW*3/4;
	t.h=tvH-4;
	if(t.w<30)t.w=30;
	t.c=vwLineN;
	t.d=0;
	EdResetWindow(win);
	edHasLog[win]=0;
	return win;
}

// Gives a closed window's lines back. Its text stays until the next compaction.
void EdClosed(int win)
{
	TWin& t=tvWin[win];
	if(t.d>0)VwCloseLines(t.c,t.d);
	t.d=0;
	if(edHasLog[win])
	{
		int* oldRec=edRec[win];
		char* oldTxt=edTxt[win];
		free(oldRec);
		free(oldTxt);
		edHasLog[win]=0;
	}
}

// A new empty window, with a file name if it is to create that file.
int EdNewFile(char* path)
{
	char title[48];
	if(path[0]==0)
	{
		edUntitled++;
		snprintf(&title[0],48,"Untitled %i",edUntitled);
	}
	else StrCpy(&title[0],&path[EdBase(path)],48);
	int win=NewViewer(&title[0]);
	if(win<0)
	{
		TvMessage("Error","Too many windows are open.",0);
		return -1;
	}
	if(!VwAppend(tvWin[win],"",0))
	{
		TvClose(win);
		TvMessage("Error","The text pool is full.",0);
		return -1;
	}
	StrCpy(&ed[win].path[0],path,ED_PATH);
	tvWin[win].f=HlLangOf(path);
	EdRehlAll(tvWin[win]);
	return win;
}

// The window showing this file, or -1.
int EdFindPath(char* path)
{
	for(int i=0;i<TV_MAXWIN;i++)
		if(tvWin[i].used)
			if(tvWin[i].type==W_VIEW)
			{
				char* other=&ed[i].path[0];
				int k=0;
				int same=(other[0]!=0);
				while(same)
				{
					int a=other[k];
					int b=path[k];
					if(a!=b)same=0;
					if((a==0)||(b==0))break;
					k++;
				}
				if(same)return i;
			}
	return -1;
}

// Reads a file into a new window, as read-only if saving it could lose anything.
int OpenFile(char* name)
{
	char full[ED_PATH];
	int where=EdAbsPath(name,&full[0]);
	if(where==-2)
	{
		TvMessage("Error","The path of this file is too long.",0);
		return -1;
	}
	if(where<0)
	{
		TvMessage("Error","There is no such directory.",0);
		return -1;
	}
	int open=EdFindPath(&full[0]);
	if(open>=0)
	{
		TvToFront(open);
		return open;
	}
	if(where==0)return EdNewFile(&full[0]);
	int size=fs_size(&full[0]);
	if(size<0)
	{
		fs_error(&edErr[0],300);
		EdShowErr();
		return -1;
	}
	if(vwTextN+size+1024>VW_POOL)VwCompact();
	int n=fs_load(&full[0],vwSpare,VW_POOL-1);
	if(n<0)
	{
		fs_error(&edErr[0],300);
		EdShowErr();
		return -1;
	}
	int win=NewViewer(&full[EdBase(&full[0])]);
	if(win<0)
	{
		TvMessage("Error","Too many windows are open.",0);
		return -1;
	}
	TWin& t=tvWin[win];
	TEdit& e=ed[win];
	StrCpy(&e.path[0],&full[0],ED_PATH);
	e.stamp=fs_stamp(&full[0]);
	int truncated=(n<size);
	int nuls=0;
	int longLines=0;
	int crlfs=0;
	int lfs=0;
	int start=0;
	int i=0;
	while(i<=n)
	{
		int c=0;
		int atEnd=(i==n);
		if(!atEnd)c=vwSpare[i];
		if(atEnd||(c==10))
		{
			int len=i-start;
			if(!atEnd)
			{
				int cr=0;
				if(len>0)
					if(vwSpare[i-1]==13)cr=1;
				if(cr)
				{
					len--;
					crlfs++;
				}
				else lfs++;
			}
			if(len>ED_MAXLINE)longLines=1;
			if(!VwAppend(t,&vwSpare[start],len))
			{
				truncated=1;
				break;
			}
			start=i+1;
		}
		else if(c==0)
		{
			nuls=1;
			vwSpare[i]='.';
		}
		i++;
	}
	if(t.d==0)
		VwAppend(t,"",0);
	e.crlf=(crlfs>lfs);
	e.ro=truncated|nuls|longLines;
	t.f=LANG_NONE;
	if(!longLines)t.f=HlLangOf(&full[0]);
	EdRehlAll(t);
	if(truncated)
		TvMessage("Read only","The file is too large, so only its beginning is shown.\nIt cannot be edited.",0);
	else if(nuls)
		TvMessage("Read only","This looks like a binary file, with NUL bytes shown as dots.\nIt cannot be edited.",0);
	else if(longLines)
	{
		snprintf(&edErr[0],300,"This file has lines longer than %i bytes.\nIt cannot be edited.",ED_MAXLINE);
		TvMessage("Read only",&edErr[0],0);
	}
	else if((crlfs>0)&&(lfs>0))
	{
		if(e.crlf)TvMessage("Line ends","This file mixes CR LF and LF line ends.\nSaving it will use CR LF for all of them.",0);
		else TvMessage("Line ends","This file mixes CR LF and LF line ends.\nSaving it will use LF for all of them.",0);
	}
	return win;
}

// Writes the window's text to path. Returns 1 on success.
int EdWriteTo(int win, char* path)
{
	TWin& t=tvWin[win];
	TEdit& e=ed[win];
	if(!fs_save_begin(path))
	{
		fs_error(&edErr[0],300);
		EdShowErr();
		return 0;
	}
	char nl[3];
	int nlN=1;
	nl[0]=10;
	if(e.crlf)
	{
		nl[0]=13;
		nl[1]=10;
		nlN=2;
	}
	nl[nlN]=0;
	int ok=1;
	for(int y=0;y<t.d;y++)
	{
		int i=t.c+y;
		if(!fs_save_write(&vwText[vwLine[i]],vwLen[i]))ok=0;
		if(ok)
			if(y<t.d-1)
				if(!fs_save_write(&nl[0],nlN))ok=0;
		if(!ok)break;
	}
	if(!fs_save_end(ok))ok=0;
	if(!ok)
	{
		fs_error(&edErr[0],300);
		EdShowErr();
		return 0;
	}
	return 1;
}

// Saves under this absolute path and makes it the window's file.
int EdSaveAs(int win, char* path)
{
	TWin& t=tvWin[win];
	TEdit& e=ed[win];
	if(!EdWriteTo(win,path))return 0;
	char copy[ED_PATH];
	StrCpy(&copy[0],path,ED_PATH);
	StrCpy(&e.path[0],&copy[0],ED_PATH);
	StrCpy(&t.title[0],&copy[EdBase(&copy[0])],48);
	e.stamp=fs_stamp(&copy[0]);
	e.savedVer=e.ver;
	int c=HlLangOf(&copy[0]);
	if(c!=t.f)
	{
		t.f=c;
		EdRehlAll(t);
	}
	return 1;
}

int EdSave(int win)
{
	TEdit& e=ed[win];
	if(e.ro)
	{
		TvMessage("Save","This window is read-only.",0);
		return 0;
	}
	if(e.path[0]==0)return RunSaveAs(win);
	if(e.stamp>=0)
	{
		int now=fs_stamp(&e.path[0]);
		if((now>=0)&&(now!=e.stamp))
			if(TvMessage("Save","The file has changed on disk since it was read.\nOverwrite those changes?",1)!=CM_YES)
				return 0;
	}
	return EdSaveAs(win,&e.path[0]);
}

// Asks about unsaved changes. Returns 0 if the window should stay.
int EdCanClose(int win)
{
	if(tvWin[win].type!=W_VIEW)return 1;
	if(!EdModified(win))return 1;
	TvToFront(win);
	char msg[100];
	snprintf(&msg[0],100,"Save the changes to %s?",&tvWin[win].title[0]);
	int r=TvChoose("Unsaved changes",&msg[0],"~S~ave|~D~iscard|~C~ancel");
	if(r==0)return EdSave(win);
	return r==1;
}

// Asks about every window with unsaved changes. Returns 0 to keep running, 1 to quit
// and 2 if nothing was unsaved, so that the caller may still want to ask.
int EdCanQuit()
{
	int asked=0;
	for(int i=0;i<TV_MAXWIN;i++)
		if(tvWin[i].used)
			if((tvWin[i].type==W_VIEW)&&EdModified(i))
			{
				asked=1;
				if(!EdCanClose(i))return 0;
			}
	if(asked)return 1;
	return 2;
}

// ---------------------------------------------------------------- search

char edFind[200];
char edRepl[200];
int edCase=0;
int edWhole=0;
int edFoundY=0;
int edFoundX=0;

int EdMatch(int p, int x, int n, int len)
{
	for(int k=0;k<n;k++)
	{
		int a=vwText[p+x+k];
		int b=edFind[k];
		if(!edCase)
		{
			a=LowerKey(a);
			b=LowerKey(b);
		}
		if(a!=b)return 0;
	}
	if(edWhole)
	{
		if(x>0)
		{
			int c=vwText[p+x-1];
			if(EdIsWord(c))return 0;
		}
		if(x+n<len)
		{
			int c=vwText[p+x+n];
			if(EdIsWord(c))return 0;
		}
	}
	return 1;
}

// The first match at or after y,x, in edFoundY,edFoundX. Returns 0 if there is none.
int EdSearch(TWin& t, int y, int x)
{
	int n=StrLen(&edFind[0]);
	if(n==0)return 0;
	while(y<t.d)
	{
		int i=t.c+y;
		int p=vwLine[i];
		int len=vwLen[i];
		while(x+n<=len)
		{
			if(EdMatch(p,x,n,len))
			{
				edFoundY=y;
				edFoundX=x;
				return 1;
			}
			x++;
		}
		y++;
		x=0;
	}
	return 0;
}

void EdSelectFound(int win)
{
	TEdit& e=ed[win];
	e.ay=edFoundY;
	e.ax=edFoundX;
	e.cy=edFoundY;
	e.cx=edFoundX+StrLen(&edFind[0]);
	e.sel=1;
	e.want=-1;
	e.lastKind=0;
	EdReveal(win);
}

void EdFindNext(int win)
{
	TWin& t=tvWin[win];
	TEdit& e=ed[win];
	int found=EdSearch(t,e.cy,e.cx);
	if(!found)found=EdSearch(t,0,0);
	if(found)EdSelectFound(win);
	else
	{
		snprintf(&edErr[0],300,"Not found: %s",&edFind[0]);
		TvMessage("Find",&edErr[0],0);
	}
}

// The selection, if on one line, is the likely thing to search for.
void EdFindFromSel(int win)
{
	TEdit& e=ed[win];
	if(!e.sel)return;
	if(e.ay!=e.cy)return;
	EdSelRange(e);
	int n=edX2-edX1;
	if(n>190)return;
	memmove(&edFind[0],&vwText[vwLine[tvWin[win].c+e.cy]+edX1],n);
	edFind[n]=0;
}

void EdReplaceAll(int win)
{
	TEdit& e=ed[win];
	TWin& t=tvWin[win];
	int n=StrLen(&edFind[0]);
	int rn=StrLen(&edRepl[0]);
	int count=0;
	int y=0;
	int x=0;
	EdNewGroup(e);
	while(EdSearch(t,y,x))
	{
		y=edFoundY;
		x=edFoundX;
		if(!EdDelete(win,y,x,y,x+n))break;
		if(!EdInsert(win,y,x,&edRepl[0],rn))
		{
			EdShowErr();
			break;
		}
		x=x+rn;
		count++;
	}
	EdNewGroup(e);
	if(count>0)EdSetCursor(win,y,x);
	EdAfterEdit(win);
	snprintf(&edErr[0],300,"%i replaced.",count);
	TvMessage("Replace",&edErr[0],0);
}

// Asks about each match from the cursor on.
void EdReplaceAsking(int win)
{
	TEdit& e=ed[win];
	TWin& t=tvWin[win];
	int rn=StrLen(&edRepl[0]);
	int count=0;
	int y=e.cy;
	int x=e.cx;
	if(e.sel)
	{
		EdSelRange(e);
		y=edY1;
		x=edX1;
	}
	EdNewGroup(e);
	int all=0;
	while(EdSearch(t,y,x))
	{
		y=edFoundY;
		x=edFoundX;
		int answer=0;
		if(!all)
		{
			EdSelectFound(win);
			int row=t.y+1+e.cy-t.a;
			int at=row+2;
			if(row>=tvH/2)at=row-8;
			if(at<1)at=1;
			answer=TvChooseAt("Replace","Replace this one?","~Y~es|~N~o|~A~ll|~C~ancel",at);
		}
		if(answer==2)
		{
			all=1;
			answer=0;
		}
		if((answer<0)||(answer==3))break;
		if(answer==1)
			x=x+StrLen(&edFind[0]);
		else
		{
			if(!EdDelete(win,y,x,y,x+StrLen(&edFind[0])))break;
			if(!EdInsert(win,y,x,&edRepl[0],rn))
			{
				EdShowErr();
				break;
			}
			x=x+rn;
			count++;
		}
		EdSetCursor(win,y,x);
	}
	EdNewGroup(e);
	EdAfterEdit(win);
	snprintf(&edErr[0],300,"%i replaced.",count);
	TvMessage("Replace",&edErr[0],0);
}

void RunFind(int win, int replacing)
{
	TEdit& e=ed[win];
	EdFindFromSel(win);
	int h=11;
	if(replacing)h=13;
	char* title="Find";
	if(replacing)title="Replace";
	int dlg=TvNewCentered(0,title,56,h,WF_DIALOG);
	if(dlg<0)return;
	TvAdd(dlg,C_LABEL,3,2,10,1,"~F~ind:",0);
	int inFind=TvAdd(dlg,C_INPUT,14,2,38,1,&edFind[0],0);
	tvCtl[inFind].val=StrLen(&edFind[0]);
	int inRepl=-1;
	int row=4;
	if(replacing)
	{
		TvAdd(dlg,C_LABEL,3,4,10,1,"~W~ith:",0);
		inRepl=TvAdd(dlg,C_INPUT,14,4,38,1,&edRepl[0],0);
		tvCtl[inRepl].val=StrLen(&edRepl[0]);
		row=6;
	}
	int boxCase=TvAdd(dlg,C_CHECK,3,row,24,1,"Match ~c~ase",0);
	int boxWhole=TvAdd(dlg,C_CHECK,3,row+1,24,1,"Whole w~o~rds",0);
	tvCtl[boxCase].val=edCase;
	tvCtl[boxWhole].val=edWhole;
	int by=row+3;
	if(replacing)
	{
		int b=TvAdd(dlg,C_BUTTON,4,by,14,2,"~R~eplace",CM_OK);
		tvCtl[b].isDefault=1;
		TvAdd(dlg,C_BUTTON,21,by,14,2,"Replace ~a~ll",CM_YES);
		TvAdd(dlg,C_BUTTON,38,by,14,2,"Cancel",CM_CANCEL);
	}
	else
	{
		int b=TvAdd(dlg,C_BUTTON,12,by,14,2,"~N~ext",CM_OK);
		tvCtl[b].isDefault=1;
		TvAdd(dlg,C_BUTTON,30,by,14,2,"Cancel",CM_CANCEL);
	}
	tvWin[dlg].focus=inFind;
	int res=TvExec(dlg);
	if(res!=CM_CANCEL)
	{
		StrCpy(&edFind[0],&tvCtl[inFind].text[0],200);
		if(replacing)StrCpy(&edRepl[0],&tvCtl[inRepl].text[0],200);
		edCase=tvCtl[boxCase].val;
		edWhole=tvCtl[boxWhole].val;
	}
	TvClose(dlg);
	if(res==CM_CANCEL)return;
	if(edFind[0]==0)return;
	if(!replacing)
	{
		// from the start of a selection, so the text just searched for is found again
		if(e.sel)
		{
			EdSelRange(e);
			EdSetCursor(win,edY1,edX1);
		}
		EdFindNext(win);
	}
	else if(res==CM_YES)EdReplaceAll(win);
	else EdReplaceAsking(win);
}

void RunGoto(int win)
{
	TWin& t=tvWin[win];
	int dlg=TvNewCentered(0,"Go to line",36,9,WF_DIALOG);
	if(dlg<0)return;
	TvAdd(dlg,C_LABEL,3,2,10,1,"~L~ine:",0);
	int in=TvAdd(dlg,C_INPUT,11,2,20,1,"",0);
	int b=TvAdd(dlg,C_BUTTON,5,5,12,2,"O~K~",CM_OK);
	tvCtl[b].isDefault=1;
	TvAdd(dlg,C_BUTTON,19,5,12,2,"Cancel",CM_CANCEL);
	tvWin[dlg].focus=in;
	int res=TvExec(dlg);
	int line=0;
	int digits=0;
	int k=0;
	while(tvCtl[in].text[k]!=0)
	{
		int c=tvCtl[in].text[k];
		if((c>='0')&&(c<='9')&&(line<100000000))
		{
			line=line*10+c-'0';
			digits=1;
		}
		k++;
	}
	TvClose(dlg);
	if((res!=CM_OK)||!digits)return;
	if(line<1)line=1;
	if(line>t.d)line=t.d;
	EdSetCursor(win,line-1,0);
	ed[win].lastKind=0;
	EdReveal(win);
}

// ---------------------------------------------------------------- the window

void EdPutCell(int x, int y, int c, int at)
{
	if(c<32)c=0x2400+c;
	else if(c==127)c=0x2421;
	else if((c>=0x80)&&(c<0xA0))c='?';
	TvPut(x,y,c,at);
}

void DrawViewer(int win)
{
	TWin& t=tvWin[win];
	TEdit& e=ed[win];
	tvWinDirty[win]=EdModified(win);   // for the window list
	int rows=t.h-2;
	int cols=EdCols(t);
	int g=EdGutter(t);
	int x0=t.x+1+g;
	int front=(TvTop()==win);
	if(e.sel)EdSelRange(e);
	edBmY=-1;
	edBmY2=-1;
	if(front)EdFindBrackets(win);
	int curRow=-1;
	int curCol=0;
	int j=0;
	int k=t.a;
	// the editor's own background, under the text and past its end
	TvFill(t.x+1,t.y+1,t.w-2,rows,' ',A_ED_TEXT);
	if(g>0)TvFill(t.x+1,t.y+1,g,rows,' ',A_ED_LINENO);
	int sub=0;
	if(edWrap)sub=t.e;
	while((j<rows)&&(k<t.d))
	{
		int i=t.c+k;
		int start=vwLine[i];
		int selA=-1;
		int selB=-1;
		if(e.sel)
			if((k>=edY1)&&(k<=edY2))
			{
				selA=0;
				selB=vwLen[i]+1;
				if(k==edY1)selA=edX1;
				if(k==edY2)selB=edX2;
			}
		if(t.f)HlLine(t.f,start,vwState[i]);
		EdWrapLine(i,cols);
		if(k==e.cy)
		{
			int cr=EdRowOf(e.cx);
			if(cr>=sub)
			{
				curRow=j+cr-sub;
				curCol=EdCol(i,e.cx)-EdCol(i,edRowAt[cr])-t.b;
			}
		}
		if(g>0)
		{
			char num[16];
			snprintf(&num[0],16,"%i",k+1);
			int nl=StrLen(&num[0]);
			while(nl<g-1)
			{
				// right-aligned
				for(int q=nl;q>=0;q--)
					num[q+1]=num[q];
				num[0]=' ';
				nl++;
			}
			num[nl]=' ';
			num[nl+1]=0;
			int at=A_ED_LINENO;
			if(k==e.cy)at=A_ED_LINECUR;
			if(sub==0)TvText(t.x+1,t.y+1+j,&num[0],at);
		}
		for(int r=sub;(r<edRowN)&&(j<rows);r++)
		{
			int p=start+edRowAt[r];
			int end=start+vwLen[i];
			if(r+1<edRowN)end=start+edRowAt[r+1];
			int col=EdCol(i,edRowAt[r]);
			int base=col+t.b;
			if(g>0)
				if(r>0)TvFill(t.x+1,t.y+1+j,g,1,' ',A_ED_LINENO);
			while(p<end)
			{
				int off=p-start;
				int at=HL_TEXT;
				if(t.f)at=hlAt[off];
				if((off>=selA)&&(off<selB))at=AT_SELECT;
				if((k==edBmY)&&(off==edBmX))
				{
					at=A_ED_FOUND;
					if(edBmY2>=0)at=A_ED_MATCH;
				}
				if((k==edBmY2)&&(off==edBmX2))at=A_ED_MATCH;
				int c=TvUtf8(vwText,p);
				if(c==9)
				{
					int next=(col/ED_TAB+1)*ED_TAB;
					if(at==AT_SELECT)
						while(col<next)
						{
							if(col>=base)TvPut(x0+col-base,t.y+1+j,' ',at);
							col++;
						}
					col=next;
				}
				else
				{
					if(col>=base)EdPutCell(x0+col-base,t.y+1+j,c,at);
					col++;
				}
				// the rest of a very long line is out of sight
				if(col-base>=cols)break;
			}
			// a selected line end shows as one cell
			if(r==edRowN-1)
				if((selB>vwLen[i])&&(col>=base))
					TvPut(x0+col-base,t.y+1+j,' ',AT_SELECT);
			j++;
		}
		sub=0;
		k++;
	}
	if(front)
	{
		if((curRow>=0)&&(curRow<rows)&&(curCol>=0)&&(curCol<cols))
		{
			int sx=x0+curCol;
			int sy=t.y+1+curRow;
			if((sx>=clipX0)&&(sx<clipX1)&&(sy>=clipY0)&&(sy<clipY1))
			{
				tvCurX=sx;
				tvCurY=sy;
			}
		}
	}
	// scroll bar and details live on the frame
	int mxTop=t.d-rows;
	if(mxTop<0)mxTop=0;
	int save=clipX1;
	int savey=clipY1;
	clipX1=tvW;
	if(clipX1>t.x+t.w)clipX1=t.x+t.w;
	clipY1=tvH-1;
	if(front)
	{
		TvVBar(t.x+t.w-1,t.y+1,rows,t.a,mxTop,A_ED_SCROLL);
		char pos[40];
		snprintf(&pos[0],40," %i:%i ",e.cy+1,EdCol(t.c+e.cy,e.cx)+1);
		int after=TvText(t.x+3,t.y+t.h-1,&pos[0],A_WIN_FRAME);
		if(e.ro)TvText(after,t.y+t.h-1,"Read only ",A_WIN);
	}
	if(EdModified(win))
		TvPut(t.x+1,t.y+t.h-1,'*',A_WIN);
	clipX1=save;
	clipY1=savey;
}

void ViewerClamp(TWin& t)
{
	int mxTop=t.d-(t.h-2);
	if(mxTop<0)mxTop=0;
	if(t.a>mxTop)t.a=mxTop;
	if(t.a<0)t.a=0;
	if(t.b<0)t.b=0;
	t.e=0;
}

// Up and down keep aiming at the column they started from.
void EdMoveLines(int win, int dy, int extend)
{
	TWin& t=tvWin[win];
	TEdit& e=ed[win];
	if(edWrap)
	{
		// by rows on the screen
		int cols=EdCols(t);
		int y=e.cy;
		EdWrapLine(t.c+y,cols);
		int r=EdRowOf(e.cx);
		if(e.want<0)e.want=EdCol(t.c+y,e.cx)-EdCol(t.c+y,edRowAt[r]);
		while(dy<0)
		{
			if(r>0)r--;
			else if(y>0)
			{
				y--;
				EdWrapLine(t.c+y,cols);
				r=edRowN-1;
			}
			dy++;
		}
		while(dy>0)
		{
			if(r<edRowN-1)r++;
			else if(y<t.d-1)
			{
				y++;
				EdWrapLine(t.c+y,cols);
				r=0;
			}
			dy--;
		}
		int want=e.want;
		EdMoveTo(win,y,EdByteInRow(t.c+y,r,want),extend);
		e.want=want;
		return;
	}
	if(e.want<0)e.want=EdCol(t.c+e.cy,e.cx);
	int y=e.cy+dy;
	if(y<0)y=0;
	if(y>=t.d)y=t.d-1;
	int want=e.want;
	EdMoveTo(win,y,EdByteAt(t.c+y,want),extend);
	e.want=want;
}

// Cursor keys. Returns 1 if it was one.
int EdNavKey(int win, int key)
{
	TWin& t=tvWin[win];
	TEdit& e=ed[win];
	int shift=evMod&MOD_SHIFT;
	int ctrl=evMod&MOD_CTRL;
	int rows=t.h-2;
	int i=t.c+e.cy;
	if(key==K_UP)
		EdMoveLines(win,-1,shift);
	else if(key==K_DOWN)
		EdMoveLines(win,1,shift);
	else if(key==K_PGUP)
	{
		t.a=t.a-rows;
		ViewerClamp(t);
		EdMoveLines(win,-rows,shift);
	}
	else if(key==K_PGDN)
	{
		t.a=t.a+rows;
		ViewerClamp(t);
		EdMoveLines(win,rows,shift);
	}
	else if(key==K_LEFT)
	{
		e.want=-1;
		if(ctrl)EdWordLeft(win,shift);
		else if(e.sel&&!shift)
		{
			EdSelRange(e);
			EdMoveTo(win,edY1,edX1,0);
		}
		else if(e.cx>0)
		{
			int p=vwLine[i];
			int q=p;
			int end=p+e.cx;
			int prev=p;
			while(q<end)
			{
				prev=q;
				TvUtf8(vwText,q);
			}
			EdMoveTo(win,e.cy,prev-p,shift);
		}
		else if(e.cy>0)
			EdMoveTo(win,e.cy-1,vwLen[i-1],shift);
	}
	else if(key==K_RIGHT)
	{
		e.want=-1;
		if(ctrl)EdWordRight(win,shift);
		else if(e.sel&&!shift)
		{
			EdSelRange(e);
			EdMoveTo(win,edY2,edX2,0);
		}
		else if(e.cx<vwLen[i])
		{
			int q=vwLine[i]+e.cx;
			int from=q;
			TvUtf8(vwText,q);
			EdMoveTo(win,e.cy,e.cx+q-from,shift);
		}
		else if(e.cy<t.d-1)
			EdMoveTo(win,e.cy+1,0,shift);
	}
	else if(key==K_HOME)
	{
		e.want=-1;
		if(ctrl)EdMoveTo(win,0,0,shift);
		else
		{
			// first to the indentation, then to the very start
			int p=vwLine[i];
			int n=0;
			while(n<vwLen[i])
			{
				int c=vwText[p+n];
				if((c!=' ')&&(c!=9))break;
				n++;
			}
			if(e.cx==n)n=0;
			EdMoveTo(win,e.cy,n,shift);
		}
	}
	else if(key==K_END)
	{
		e.want=-1;
		if(ctrl)EdMoveTo(win,t.d-1,vwLen[t.c+t.d-1],shift);
		else EdMoveTo(win,e.cy,vwLen[i],shift);
	}
	else return 0;
	return 1;
}

// A closing brace typed on a line of only indentation takes one level away first.
// Returns 1 if it did.
int EdDedent(int win)
{
	TWin& t=tvWin[win];
	TEdit& e=ed[win];
	if((t.f!=LANG_C)||e.sel||(e.cx==0))return 0;
	int p=vwLine[t.c+e.cy];
	for(int k=0;k<e.cx;k++)
	{
		int c=vwText[p+k];
		if((c!=' ')&&(c!=9))return 0;
	}
	int n=1;
	if(vwText[p+e.cx-1]==' ')
	{
		n=0;
		while((n<ED_TAB)&&(n<e.cx))
		{
			if(vwText[p+e.cx-1-n]!=' ')break;
			n++;
		}
	}
	EdNewGroup(e);
	if(!EdDelete(win,e.cy,e.cx-n,e.cy,e.cx))
	{
		EdShowErr();
		return 0;
	}
	EdSetCursor(win,e.cy,e.cx-n);
	return 1;
}

int ViewerKey(int win, int key)
{
	TEdit& e=ed[win];
	if(EdNavKey(win,key))return 1;
	if(e.ro)
		return (key==K_ENTER)||(key==K_TAB)||(key==K_BTAB)||(key==K_BACKSPACE)||(key==K_DEL)||((key>=32)&&(key<127));
	if(key==K_BACKSPACE)EdEraseBack(win);
	else if(key==K_DEL)EdEraseForward(win);
	else if(key==K_ENTER)EdEnter(win);
	else if(key==K_BTAB)EdUnindentLines(win);
	else if(key==K_TAB)
	{
		int lines=0;
		if(e.sel)lines=(e.ay!=e.cy);
		if(lines)EdIndentLines(win);
		else
		{
			char tab[2];
			tab[0]=9;
			tab[1]=0;
			EdType(win,&tab[0],1,0);
		}
	}
	else if(evUtfN>0)EdType(win,&evUtf[0],evUtfN,0);
	else if(key=='}')
	{
		EdDedent(win);
		char one[2];
		one[0]=key;
		one[1]=0;
		EdType(win,&one[0],1,0);
	}
	else if((key>=32)&&(key<127))
	{
		char one[2];
		one[0]=key;
		one[1]=0;
		EdType(win,&one[0],1,key==' ');
	}
	else return 0;
	EdAfterEdit(win);
	return 1;
}

// The text position under window-relative x,y.
void EdMouseTo(int win, int x, int y, int extend)
{
	TWin& t=tvWin[win];
	TEdit& e=ed[win];
	int col=x-1-EdGutter(t);
	if(col<0)col=0;
	e.want=-1;
	if(edWrap)
	{
		// down the rows from the top of the window
		int cols=EdCols(t);
		int ly=t.a;
		int r=t.e;
		EdWrapLine(t.c+ly,cols);
		if(r>=edRowN)r=edRowN-1;
		int left=y-1;
		while(left>0)
		{
			if(r<edRowN-1)r++;
			else if(ly<t.d-1)
			{
				ly++;
				EdWrapLine(t.c+ly,cols);
				r=0;
			}
			else break;
			left--;
		}
		if(y<1)r=0;
		EdMoveTo(win,ly,EdByteInRow(t.c+ly,r,col),extend);
		return;
	}
	int ly=t.a+y-1;
	if(ly<0)ly=0;
	if(ly>=t.d)ly=t.d-1;
	col=col+t.b;
	EdMoveTo(win,ly,EdByteAt(t.c+ly,col),extend);
}

int ViewerMouse(int win, int ev, int x, int y)
{
	TWin& t=tvWin[win];
	TEdit& e=ed[win];
	int rows=t.h-2;
	if(ev==EV_WHEEL)
	{
		if(evWheel>0)t.a=t.a-3;
		else t.a=t.a+3;
		ViewerClamp(t);
		return 1;
	}
	if(ev==EV_MDOWN)
	{
		edDrag=0;
		// the frame cell is outside the interior, so the bar is at column w-1
		if(x==t.w-1)
		{
			int mxTop=t.d-rows;
			if(mxTop<0)mxTop=0;
			int h=TvVBarHit(1,rows,t.a,mxTop,y);
			if(h==2)
			{
				edDrag=1;
				TvCaptureMouse(win);
			}
			if(h==0)t.a--;
			if(h==1)t.a=t.a-rows;
			if(h==3)t.a=t.a+rows;
			if(h==4)t.a++;
			ViewerClamp(t);
		}
		else if((x>=1)&&(x<t.w-1)&&(y>=1)&&(y<t.h-1))
		{
			float now=Time();
			int twice=(x==edClickX)&&(y==edClickY)&&(now-edClickTime<0.4);
			edClickX=x;
			edClickY=y;
			edClickTime=now;
			EdMouseTo(win,x,y,0);
			if(twice)
			{
				edClickX=-1;
				EdSelectWord(win);
			}
			else
			{
				edDrag=2;
				TvCaptureMouse(win);
			}
		}
		return 1;
	}
	if(ev==EV_MDRAG)
	{
		// only arrives after TvCaptureMouse
		if(edDrag==1)
		{
			int mxTop=t.d-rows;
			if(mxTop<0)mxTop=0;
			t.a=TvVBarPos(1,rows,mxTop,y);
		}
		else if(edDrag==2)
		{
			if(!e.sel)
			{
				e.ay=e.cy;
				e.ax=e.cx;
			}
			EdMouseTo(win,x,y,1);
		}
		return 1;
	}
	return 0;
}

int EdPaste(int win, char* s, int n)
{
	if(tvWin[win].type!=W_VIEW)return 0;
	TEdit& e=ed[win];
	if(e.ro)return 1;
	if(tvPasteLost)
	{
		TvMessage("Paste","The pasted text is too large.",0);
		return 1;
	}
	EdNewGroup(e);
	if(!EdPutText(win,s,n))EdShowErr();
	EdNewGroup(e);
	EdAfterEdit(win);
	return 1;
}

// The menu commands that act on the front window. Returns 1 if it was one.
int EdCommand(int cmd)
{
	if(cmd==CM_NEW)
	{
		EdNewFile("");
		return 1;
	}
	if((cmd<CM_SAVE)||(cmd>CM_MATCH))return 0;
	int win=TvTop();
	if(win<0)return 1;
	if(tvWin[win].type!=W_VIEW)return 1;
	TEdit& e=ed[win];
	if(cmd==CM_SAVE)EdSave(win);
	else if(cmd==CM_SAVEAS)
	{
		if(e.ro)TvMessage("Save as","This window is read-only.",0);
		else RunSaveAs(win);
	}
	else if(cmd==CM_COPY)EdCopy(win);
	else if(cmd==CM_SELALL)
	{
		TWin& t=tvWin[win];
		e.ay=0;
		e.ax=0;
		e.cy=t.d-1;
		e.cx=vwLen[t.c+t.d-1];
		e.sel=(t.d>1)||(e.cx>0);
		e.want=-1;
		e.lastKind=0;
		EdReveal(win);
	}
	else if(cmd==CM_FIND)RunFind(win,0);
	else if(cmd==CM_FINDNEXT)
	{
		if(edFind[0]==0)RunFind(win,0);
		else EdFindNext(win);
	}
	else if(cmd==CM_GOTO)RunGoto(win);
	else if(cmd==CM_MATCH)
	{
		EdFindBrackets(win);
		if(edBmY2>=0)EdMoveTo(win,edBmY2,edBmX2,0);
	}
	else if(e.ro)
		TvMessage("Editor","This window is read-only.",0);
	else if(cmd==CM_UNDO)
	{
		if(EdUndo(win))EdReveal(win);
	}
	else if(cmd==CM_REDO)
	{
		if(EdRedo(win))EdReveal(win);
	}
	else if(cmd==CM_CUT)
	{
		if(e.sel)
		{
			EdCopy(win);
			EdSelRange(e);
			int n=EdTextLen(tvWin[win],edY1,edX1,edY2,edX2);
			if(n<=ED_CLIPMAX)
			{
				EdNewGroup(e);
				if(!EdDeleteSel(win))EdShowErr();
				EdNewGroup(e);
				EdAfterEdit(win);
			}
		}
	}
	else if(cmd==CM_PASTE)
	{
		if(edClipN>0)
		{
			EdNewGroup(e);
			if(!EdPutText(win,edClip,edClipN))EdShowErr();
			EdNewGroup(e);
			EdAfterEdit(win);
		}
	}
	else if(cmd==CM_REPLACE)RunFind(win,1);
	return 1;
}

void EdInit()
{
	vwText=malloc(VW_POOL+16);
	vwSpare=malloc(VW_POOL+16);
	vwLine=malloc(VW_MAXLINES*4);
	vwLen=malloc(VW_MAXLINES*4);
	vwCap=malloc(VW_MAXLINES*4);
	vwState=malloc(VW_MAXLINES*4);
	edClip=malloc(ED_CLIPMAX+1);
	edFind[0]=0;
	edRepl[0]=0;
	for(int i=0;i<TV_MAXWIN;i++)
		edHasLog[i]=0;
	HlInit();
}

#endif // TVEDIT_H
