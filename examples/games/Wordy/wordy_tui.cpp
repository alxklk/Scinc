// Wordy in a text window: the game of wordy_core.h on the desktop of claude/tui.
//
// Guess the hidden five-letter word in six tries. A scored letter shows as [A] in
// red when it is in the right place, (A) in brown when the word has it elsewhere,
// and dim when the word does not have it (or not that many times); the legend in
// the window says so too. The letters beside the board keep the best mark found
// for each; click one to type it. The cyan window beside it explains the guessed
// words (looked up online, see wordy.cpp) and scrolls; a click on a guessed row
// shows that word's meaning again, and F5 opens the window if it was closed.
//
//   ../../claude/tui/build_so.sh    once, builds the terminal library
//   Scinc wordy_tui.cpp             from this folder
#include <stdio.h>

#define TERM_DL "../../claude/tui/term_dl"
#define TVAPP_DRAW
#define TVAPP_KEY
#define TVAPP_MOUSE
#define TVAPP_COMMAND
#define TVAPP_BUTTON
#define TVAPP_TICK
#define TVAPP_CLOSED
// Scinc looks for a header's own #include files in the folder of this program,
// and quietly skips them when they are not there, so the desktop's headers come
// here one by one, each before the one that needs it.
#include "../../claude/tui/term.h"
#include "../../claude/tui/tv.h"
#include "../../claude/tui/tvapp.h"
#include "wordy_core.h"

#define CM_QUIT 100
#define CM_NEWGAME 101
#define CM_GIVEUP 102
#define CM_LANG 103
#define CM_STATS 104
#define CM_MEANING 105

#define W_GAME 1
#define W_MEANING 2
#define BX 3       // the board: five letters of 4 columns, a row every second line
#define BY 3
#define PX 26      // the letters, seven to a row, and the legend on the right
#define GAME_W 44
#define MEAN_W 34
#define AT_RIGHT 0x4F
#define AT_ELSE 0x6F
#define AT_NONE 0x87   // grey on dark grey, so it stands apart from letters not tried yet

int gameWin=-1;
int meanWin=-1;
int meanSlot=-2;   // the slot the meaning window shows, to notice a new one
int meanTotal=0;   // the lines of its text, as last drawn
int newBtn=-1;
int revealN=5;     // letters of the newest row shown so far; 5 when none is turning
int revealNext=0;  // term_ms() of the next one
char note[80];     // the line above the board

void Note(char* s)
{
	StrCpy(&note[0],s,80);
	tvDirty=1;
}

// a letter byte of the game as a capital for the screen
int Shown(int c)
{
	return CpToUni(UpChar(c));
}

// the letters of the language, in alphabet order
int LetterCount()
{
	if(lang==LANG_UA)return nUaAlpha;
	return 26;
}

int LetterAt(int i)
{
	if(lang==LANG_UA)return uaAlpha[i];
	return 'a'+i;
}

int MarkAt(int m)
{
	if(m==3)return AT_RIGHT;
	if(m==2)return AT_ELSE;
	if(m==1)return AT_NONE;
	return 0x70;
}

void OpenMeaning();

void NewGame()
{
	WordNewGame();
	revealN=5;
	Note("Type a five-letter word and press Enter.");
	StrCpy(&tvCtl[newBtn].text[0],"Give ~u~p",200);
}

void Lose()
{
	WordLose();
	char w[8];
	for(int i=0;i<5;i++)w[i]=UpChar(secret[i]);
	w[5]=0;
	char u8[16];
	CpToUtf8(&w[0],&u8[0],16);
	char s[80];
	snprintf(&s[0],80,"The word was %s. Enter starts a new game.",&u8[0]);
	Note(&s[0]);
	StrCpy(&tvCtl[newBtn].text[0],"~N~ew game",200);
}

// the last letter of a row has turned
void RowShown()
{
	int r=row;
	int res=WordRowDone(r);
	OpenMeaning();
	if(res==WD_WON)
	{
		char s[80];
		snprintf(&s[0],80,"Found in %i! Enter starts a new game.",r+1);
		Note(&s[0]);
		StrCpy(&tvCtl[newBtn].text[0],"~N~ew game",200);
	}
	else if(res==WD_LOST)Lose();
	else Note("");
}

void Submit()
{
	int r=WordSubmit();
	if(r==WD_SHORT)Note("Not enough letters.");
	else if(r==WD_UNKNOWN)Note("That word is not in the list.");
	else
	{
		revealN=0;
		revealNext=term_ms();
		Note("");
	}
}

void TvAppTick()
{
	if(UpdateLookups())tvDirty=1;
	if(revealN<5)
		if(term_ms()>=revealNext)
		{
			revealN++;
			revealNext=term_ms()+150;
			tvDirty=1;
			if(revealN==5)RowShown();
		}
}

void ShowStats()
{
	int pct=0;
	if(played>0)pct=wins*100/played;
	char s[400];
	int n=snprintf(&s[0],400,"Played %i   Won %i%%   Streak %i   Best %i\n",played,pct,streak,maxStreak);
	int most=1;
	for(int i=0;i<6;i++)
		if(dist[i]>most)most=dist[i];
	// the dialog centres every line, so the bars are padded to one width
	for(int i=0;i<6;i++)
	{
		char bar[24];
		int len=dist[i]*20/most;
		for(int k=0;k<20;k++)
		{
			bar[k]=' ';
			if(k<len)bar[k]='#';
		}
		bar[20]=0;
		n=n+snprintf(&s[n],400-n,"\n%i %s %3i",i+1,&bar[0],dist[i]);
	}
	TvMessage("Statistics",&s[0],0);
}

void TvAppCommand(int cmd)
{
	int busy=(revealN<5);
	if(cmd==CM_QUIT)tvRunning=0;
	else if(cmd==CM_STATS)ShowStats();
	else if(cmd==CM_MEANING)OpenMeaning();
	else if(busy)return;
	else if(cmd==CM_NEWGAME)NewGame();
	else if(cmd==CM_GIVEUP)
	{
		if(state!=0)NewGame();
		else if(TvMessage("Give up","Show the word and end this game?",1)==CM_YES)Lose();
	}
	else if(cmd==CM_LANG)
	{
		int playing=(state==0)&&((row>0)||(col>0));
		if(playing)
			if(TvMessage("Language","Switching drops this game. Switch?",1)!=CM_YES)return;
		if(!WordSetLanguage(1-lang))TvMessage("Language","words_ua.txt was not found.",0);
		StrCpy(&tvWin[gameWin].title[0],"Wordy",48);
		if(lang==LANG_UA)StrCpy(&tvWin[gameWin].title[0],"Wordy - українська",48);
		NewGame();
	}
}

void TvAppButton(int win, int ctl, int cmd)
{
	TvAppCommand(cmd);
}

int GameKey(int key)
{
	if(revealN<5)return 1;
	if(key==K_ENTER)
	{
		if(state!=0)NewGame();
		else Submit();
		return 1;
	}
	if(key==K_BACKSPACE)
	{
		WordErase();
		return 1;
	}
	int k=key;
	if(evUtfN>0)
	{
		int i=0;
		k=UniToCp(WordUtf8Next(&evUtf[0],i));
	}
	k=WordLetterOf(k);
	if(k!=0)
	{
		if(WordType(k))Note("");
		return 1;
	}
	return 0;
}

// a click on a letter types it, on a guessed row shows its meaning
// ---------------------------------------------------------------- the meaning window

int mlLine=0;      // the text is drawn line by line: the line number,
int mlTop=0;       // the first one shown, the rows there are and where they go
int mlRows=0;
int mlX=0;
int mlY=0;

// a line of n letters (Windows-1251 or ASCII) from s, indented, if it is in view
void MeanLine(char* s, int n, int indent, int at)
{
	int r=mlLine-mlTop;
	if((r>=0)&&(r<mlRows))
		for(int k=0;k<n;k++)
			TvPut(mlX+indent+k,mlY+r,CpToUni(s[k]),at);
	mlLine++;
}

// text wrapped at words into w columns
void MeanText(char* s, int indent, int w, int at)
{
	w=w-indent;
	if(w<4)w=4;
	int i=0;
	while(s[i]!=0)
	{
		int n=0;
		int cut=-1;
		while((s[i+n]!=0)&&(n<w))
		{
			if(s[i+n]==' ')cut=n;
			n++;
		}
		if(s[i+n]==0)cut=n;
		if(cut<=0)cut=n;
		MeanLine(&s[i],cut,indent,at);
		i=i+cut;
		while(s[i]==' ')i++;
	}
}

void OpenMeaning()
{
	if(meanWin>=0)return;
	TWin& g=tvWin[gameWin];
	int x=g.x+g.w;
	int w=MEAN_W;
	if(x+w>tvW)w=tvW-x;
	if(w<20)
	{
		x=tvW-MEAN_W;
		w=MEAN_W;
	}
	meanWin=TvNew(W_MEANING,"Meaning",x,g.y,w,g.h,WF_RESIZE|WF_HELP);
	meanSlot=-2;
	// the game keeps the keys
	TvToFront(gameWin);
}

void MeanScroll(int top)
{
	TWin& t=tvWin[meanWin];
	int last=meanTotal-(t.h-2);
	if(top>last)top=last;
	if(top<0)top=0;
	t.a=top;
	tvDirty=1;
}

// the meaning of the row picked, or of the hidden word after a lost game
void DrawMeaning(int win)
{
	TWin& t=tvWin[win];
	int s0=shownSlot;
	if(s0!=meanSlot)
	{
		meanSlot=s0;
		t.a=0;
		StrCpy(&t.title[0],"Meaning",48);
		if(s0>=0)
		{
			char w[8];
			for(int k=0;k<5;k++)w[k]=UpChar(lkWord[s0*6+k]);
			w[5]=0;
			CpToUtf8(&w[0],&t.title[0],48);
		}
	}
	int w=t.w-3;
	mlLine=0;
	mlTop=t.a;
	mlRows=t.h-2;
	mlX=t.x+2;
	mlY=t.y+1;
	if(s0<0)MeanText("The meanings of the words you guess show here.",0,w,0x30);
	else
	{
		int k=lkState[s0];
		if((k==LK_DICT)||(k==LK_WIKI))MeanText("Looking it up...",0,w,0x37);
		else if(k==LK_OFFLINE)MeanText("No network for the meaning.",0,w,0x37);
		else if(k==LK_NOTFOUND)MeanText("No meaning found.",0,w,0x37);
		int num=0;
		for(int n=0;n<nItems[s0];n++)
		{
			char* text=&itemText[(s0*MAX_ITEMS+n)*ITEM_LEN];
			if(itemKind[s0*MAX_ITEMS+n]==0)
			{
				if(n>0)MeanLine("",0,0,0x30);
				MeanText(text,0,w,0x3E);
				num=0;
			}
			else
			{
				num++;
				char d[4];
				snprintf(&d[0],4,"%i.",num);
				// the number goes on the first line of the definition
				int r=mlLine-mlTop;
				if((r>=0)&&(r<mlRows))TvText(mlX,mlY+r,&d[0],0x3F);
				MeanText(text,3,w,0x30);
			}
		}
	}
	meanTotal=mlLine;
	int last=meanTotal-mlRows;
	if(last<0)last=0;
	TvVBar(t.x+t.w-1,t.y+1,mlRows,t.a,last,0x31);
}

int MeaningKey(int key)
{
	TWin& t=tvWin[meanWin];
	int page=t.h-3;
	if(key==K_UP)MeanScroll(t.a-1);
	else if(key==K_DOWN)MeanScroll(t.a+1);
	else if(key==K_PGUP)MeanScroll(t.a-page);
	else if(key==K_PGDN)MeanScroll(t.a+page);
	else if(key==K_HOME)MeanScroll(0);
	else if(key==K_END)MeanScroll(meanTotal);
	else return GameKey(key);
	return 1;
}

int MeaningMouse(int ev, int x, int y)
{
	TWin& t=tvWin[meanWin];
	int rows=t.h-2;
	int last=meanTotal-rows;
	if(last<0)last=0;
	if(ev==EV_WHEEL)
	{
		MeanScroll(t.a-3*evWheel);
		return 1;
	}
	if(x!=t.w-1)return 0;
	if(ev==EV_MDOWN)
	{
		int h=TvVBarHit(1,rows,t.a,last,y);
		if(h==0)MeanScroll(t.a-1);
		if(h==1)MeanScroll(t.a-rows);
		if(h==3)MeanScroll(t.a+rows);
		if(h==4)MeanScroll(t.a+1);
		if(h==2)TvCaptureMouse(meanWin);
		return 1;
	}
	if(ev==EV_MDRAG)
	{
		MeanScroll(TvVBarPos(1,rows,last,y));
		return 1;
	}
	return 0;
}

// ---------------------------------------------------------------- the game window

// the hooks also run for dialogs, which keep their own keys and drawing
int TvAppKey(int win, int key)
{
	if(win==gameWin)return GameKey(key);
	if(win==meanWin)return MeaningKey(key);
	return 0;
}

void TvAppClosed(int win)
{
	if(win==meanWin)meanWin=-1;
}

// a click on a letter types it, on a guessed row shows its meaning
int TvAppMouse(int win, int ev, int x, int y)
{
	if(win==meanWin)return MeaningMouse(ev,x,y);
	if((ev!=EV_MDOWN)||(win!=gameWin))return 0;
	if((x>=PX)&&(y>=BY)&&(y<BY+5))
	{
		int i=(y-BY)*7+(x-PX)/2;
		if(((x-PX)%2==0)&&(x<PX+14))
			if((i<LetterCount())&&(revealN==5))WordType(LetterAt(i));
		return 1;
	}
	if((x>=BX)&&(x<BX+20)&&(y>=BY))
	{
		int r=(y-BY)/2;
		if((r<6)&&((y-BY)%2==0))
			if(lkState[r]!=LK_NONE)
			{
				shownSlot=r;
				OpenMeaning();
			}
		return 1;
	}
	return 0;
}

// a sample cell of the legend and what it means
void LegendLine(int x, int y, int open, int close, int at, char* what)
{
	TvPut(x,y,open,at);
	TvPut(x+1,y,'A',at);
	TvPut(x+2,y,close,at);
	TvText(x+4,y,what,0x70);
}

void TvAppDraw(int win)
{
	if(win==meanWin)
	{
		DrawMeaning(win);
		return;
	}
	if(win!=gameWin)return;
	TWin& t=tvWin[win];
	int x0=t.x;
	int y0=t.y;
	TvText(x0+2,y0+1,&note[0],0x74);
	// the board
	for(int r=0;r<6;r++)
		for(int i=0;i<5;i++)
		{
			int px=x0+BX+i*4;
			int py=y0+BY+r*2;
			int c=guess[r*5+i];
			int m=mark[r*5+i];
			if((r==row)&&(i>=revealN))m=0;
			int at=0x30;
			if(c!=0)at=0x1F;
			if(m>0)at=MarkAt(m);
			int open=' ';
			int close=' ';
			if(m==3)
			{
				open='[';
				close=']';
			}
			if(m==2)
			{
				open='(';
				close=')';
			}
			int ch=' ';
			if(c!=0)ch=Shown(c);
			TvPut(px,py,open,at);
			TvPut(px+1,py,ch,at);
			TvPut(px+2,py,close,at);
		}
	// arrows either side of the row whose meaning is shown
	if((shownSlot>=0)&&(shownSlot<6))
	{
		TvPut(x0+BX-2,y0+BY+shownSlot*2,G_RIGHT,0x71);
		TvPut(x0+BX+20,y0+BY+shownSlot*2,G_LEFT,0x71);
	}
	char s[48];
	snprintf(&s[0],48,"%i possible words left",remaining);
	TvText(x0+BX,y0+BY+12,&s[0],0x70);
	// the letters with the best mark found, and what the marks mean
	for(int i=0;i<LetterCount();i++)
	{
		int c=LetterAt(i);
		TvPut(x0+PX+(i%7)*2,y0+BY+i/7,Shown(c),MarkAt(keyMark[c]));
	}
	int ly=y0+BY+6;
	LegendLine(x0+PX,ly,'[',']',AT_RIGHT,"right place");
	LegendLine(x0+PX,ly+1,'(',')',AT_ELSE,"elsewhere");
	LegendLine(x0+PX,ly+2,' ',' ',AT_NONE,"not in it");
	LegendLine(x0+PX,ly+3,' ',' ',0x70,"not tried");
}

int main()
{
	if(!TvStart(CM_QUIT))return 1;
	WordInit();
	if(!WordSetLanguage(LoadLang()))WordSetLanguage(LANG_EN);

	TvMenuAdd("~G~ame");
	TvMenuItem("~N~ew game","F2",CM_NEWGAME,K_F2);
	TvMenuItem("~G~ive up","",CM_GIVEUP,0);
	TvMenuItem("~L~anguage","F3",CM_LANG,K_F3);
	TvMenuItem("~S~tatistics","F4",CM_STATS,K_F4);
	TvMenuItem("~M~eaning window","F5",CM_MEANING,K_F5);
	TvMenuLine();
	TvMenuItem("E~x~it","Alt+X",CM_QUIT,K_ALT+'x');
	TvStatusAdd("~F2~ New",CM_NEWGAME,0);
	TvStatusAdd("~F3~ Language",CM_LANG,0);
	TvStatusAdd("~F4~ Statistics",CM_STATS,0);
	TvStatusAdd("~F5~ Meaning",CM_MEANING,0);
	TvStatusAdd("~Alt+X~ Exit",CM_QUIT,17);

	// the game and the meanings side by side
	int x=(tvW-GAME_W-MEAN_W)/2;
	if(x<0)x=0;
	gameWin=TvNew(W_GAME,"Wordy",x,2,GAME_W,20,WF_DIALOG);
	if(lang==LANG_UA)StrCpy(&tvWin[gameWin].title[0],"Wordy - українська",48);
	newBtn=TvAdd(gameWin,C_BUTTON,2,17,11,2,"Give ~u~p",CM_GIVEUP);
	TvAdd(gameWin,C_BUTTON,15,17,12,2,"~L~anguage",CM_LANG);
	TvAdd(gameWin,C_BUTTON,29,17,12,2,"~S~tatistics",CM_STATS);
	OpenMeaning();
	tvTickMs=50;
	NewGame();

	TvRun(-1);
	TvStop();
	return 0;
}
