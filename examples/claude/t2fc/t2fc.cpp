// t2fc: a two-panel file manager on the Turbo Vision style desktop of ../tui.
// Each panel window holds two file panels in the manner of Midnight Commander (Ctrl+N
// opens another). F3 views a file (text in a read-only editor window, binary in the
// hex viewer, pictures and, through ffmpeg or GStreamer, videos in a picture window), F4 edits it in the tui editor, F5-F8 copy, move, make directories and
// delete, with progress, and F2 or Enter on an executable runs a program: on the
// terminal itself, or in a new terminal window when t2fc draws in a graphic window.
// The editor, hex viewer, theme editor and dialogs all come from ../tui.
//
//   (cd ../tui && ./build_so.sh)       once, builds the terminal and file library
//   ./build_so.sh                       once, builds the picture decoder img_dl.so
//   Scinc t2fc.cpp [left [right]]       from this folder, in a terminal
//
// t2fc_gfx.cpp, t2fc_raster.cpp and t2fc_small.cpp draw the same desktop in a graphic
// window (stroke font, 9x16 and 6x12 bitmap fonts); t2fc_gl.cpp and t2fc_gl_small.cpp
// draw it with OpenGL (build ../opengl first).
#include <stdio.h>
#define TERM_DL "../tui/term_dl"
#define TVAPPS_TOOLS                       // only the hex viewer and the theme editor
#ifndef TE_FILE                           // the native build (t2fc-native) keeps them elsewhere
#define TE_FILE "../tui/tvthemes.cfg"      // the themes are shared with tvdemo
#define TE_OLDFILE "../tui/tvtheme.cfg"
#endif
#define TVAPP_DRAW
#define TVAPP_KEY
#define TVAPP_MOUSE
#define TVAPP_COMMAND
#define TVAPP_BUTTON
#define TVAPP_CLOSED
#define TVAPP_LISTITEM
#define TVAPP_CANCLOSE
#define TVAPP_PASTE
#define TVAPP_TICK
#define TVAPP_PIXELS
#define TVAPP_STATUS
#define TV_TERM_HINT "Scinc t2fc.cpp"
#include "../tui/tv.h"
#include "../tui/tvapp.h"
#include "../tui/tvedit.h"
#include "../tui/tvapps.h"

#pragma STACK_SIZE 655360

typedef char* pchar;

#define W_FC 60
#define W_JOB 61
#define W_ASK 62
#define W_PREFS 63
#define W_ARCWAIT 66

#define CM_QUIT 500
#define CM_HELP 501
#define CM_ABOUT 502
#define CM_PREFS 503
#define CM_THEMEED 504
#define CM_THEMEPICK 505
#define CM_DESKPICK 509
#define CM_PIX 510       // + 1 to 4: the pixel size of a graphic window
#define CM_LANCZOS 515   // pictures shown smaller through a Lanczos filter, on or off
#define CM_LINENUMS 506
#define CM_WRAP 507
#define CM_LYNX 508

// the panels' commands, FC_CMD0 to FC_CMD1
#define FC_CMD0 600
#define CM_VIEW 600
#define CM_EDIT 601
#define CM_NEWFILE 602
#define CM_FCOPY 603
#define CM_MOVE 604
#define CM_RENAME 605
#define CM_MKDIR 606
#define CM_DELETE 607
#define CM_HEXVIEW 608
#define CM_SELGROUP 609
#define CM_UNSELGROUP 610
#define CM_INVERT 611
#define CM_SWAP 612
#define CM_SAMEDIR 613
#define CM_REREAD 614
#define CM_HIDDEN 615
#define CM_COMPARE 616
#define CM_HOME 617
#define CM_GODIR 618
#define CM_NEWPANELS 619
#define CM_PANEL 620    // + panel*10 + one of the PN_ actions below
#define CM_RUN 640
#define CM_OPENSYS 641
#define CM_OTHERPANEL 642
#define CM_QUICKOTHER 643
#define CM_PARENT 644
#define CM_MARK 645
#define CM_COPYAS 646  // Shift+F5: copy in the same directory under a name typed
#define CM_COPYPATH 647 // Ctrl+C, Ctrl+Insert: the path, or the marked names, to the clipboard
#define CM_MARKEXT 648  // Shift+keypad +: mark the files with the extension of the one under the cursor
#define CM_UNMARKEXT 649 // Shift+keypad -: unmark them
#define FC_CMD1 659

#define PN_FULL 0
#define PN_BRIEF 1
#define PN_NAME 2
#define PN_EXT 3
#define PN_SIZE 4
#define PN_TIME 5
#define PN_REVERSE 6
#define PN_QUICK 7

#define FC_PATH 1024
#ifndef CFG_FILE
#define CFG_FILE "t2fc.cfg"
#define KEYS_FILE "t2fc.keys"
#define HIST_FILE "t2fc.hist"
#endif
// input histories (tv.h TvInputHistory); the editor has 14..16
#define H_RUN 1
#define H_TARGET 2     // copy and move targets
#define H_NAME 3       // new directories and files, Save as
#define H_PATTERN 4    // select and unselect groups
#define H_DIR 5        // go to directory
#define H_TERM 6       // the terminal program in Preferences

// the panel colours, VGA attributes as Midnight Commander draws them; the themes
// recolour these through their 16-colour palette
#define FC_BG 0x17
#define FC_FRAME 0x17
#define FC_HEAD 0x1E
#define FC_FILE 0x17
#define FC_DIR 0x1F
#define FC_EXEC 0x1A
#define FC_LINK 0x1B
#define FC_ARCH 0x1D
#define FC_MEDIA 0x13
#define FC_JUNK 0x18
#define FC_MARK 0x1E
#define FC_CUR 0x30
#define FC_CURMARK 0x3E
#define FC_PATHON 0x30

// ---------------------------------------------------------------- panel state

// There can be several panel windows. Panel window c has the panels 2c (left) and
// 2c+1 (right); the arrays below are indexed by panel. The functions work on the
// current panel window, which FcSelect picks: fcWin, fcBase (its left panel) and
// fcActive (its active panel, a panel number) describe it.
#define FC_MAXC 6
#define FC_MAXP 12

int fcWinOf[FC_MAXC];       // the window of each panel window, -1 if not open
int fcAct[FC_MAXC];         // its active panel
int fcReload[FC_MAXC];      // files may have changed: read them again on the next draw
int fcC=0;
int fcWin=-1;
int fcBase=0;
int fcActive=0;
int fcHidden=0;
int fcLynx=0;              // Left and Right leave and enter directories, as in lynx
int fcQSCase=0;            // the panels' quick search tells upper from lower case
int fcQS=-1;          // the panel searching, or -1
char fcQSText[200];
int fcQSN=0;
int fcQSecs=5;             // quick view counts what a directory holds for this
                           // many seconds at most, or not at all if 0
// Captured output: up to four runs whose output files are still to be opened.
#define FC_CAPS 4
int fcCapUsed[FC_CAPS];
char fcCapOut[4096];        // FC_CAPS paths each: the temporary stdout file or ""
char fcCapErr[4096];        // the stderr file or ""
char fcCapMark[4096];       // made when the program has ended
char fcCapDir[4096];        // the panel's directory, where Save as suggests to keep it
char fcSaveName[16384];     // TV_MAXWIN paths: Save as suggests this for a window with no file

char fcDir[12288];          // FC_MAXP paths of FC_PATH
int fcCount[FC_MAXP];
int fcCur[FC_MAXP];
int fcTop[FC_MAXP];
int fcSort[FC_MAXP];        // term_dl sort modes: 0 name, 1 ext, 2 size, 3 time, +4 reversed
int fcBrief[FC_MAXP];
int fcStamp[FC_MAXP];
int fcLastClick=0;          // for double clicks
int fcLastP=-1;
int fcLastI=-1;
int fcView[TV_MAXWIN];      // an editor window opened by F3, to close with F3 or Esc
int fcLocked[TV_MAXWIN];    // the editor made it read-only for its own reasons
int prefTheme=0;
char fcTermCmd[200];        // how to open a terminal window, as "xterm -e"; empty finds one
int fcAskAtStart=0;         // FcAsk puts the cursor at the start of the text
int fcAskHist=0;            // and gives its input this history (H_*), once
int popPanel=-1;
int popEdit=-1;

void ShowToggles();

void FcInitState()
{
	for(int c=0;c<FC_MAXC;c++)
	{
		fcWinOf[c]=-1;
		fcAct[c]=c*2;
		fcReload[c]=0;
	}
	for(int p=0;p<FC_MAXP;p++)
	{
		fcDir[p*FC_PATH]=0;
		fcCount[p]=0;
		fcCur[p]=0;
		fcTop[p]=0;
		fcSort[p]=0;
		fcBrief[p]=0;
		fcStamp[p]=-1;
	}
	fcTermCmd[0]=0;
	for(int i=0;i<TV_MAXWIN;i++)
	{
		fcView[i]=0;
		fcLocked[i]=0;
		fcSaveName[i*FC_PATH]=0;
	}
	for(int c=0;c<FC_CAPS;c++)
		fcCapUsed[c]=0;
}

// Makes panel window c the current one.
void FcSelect(int c)
{
	fcC=c;
	fcWin=fcWinOf[c];
	fcBase=c*2;
	fcActive=fcAct[c];
}

void FcSetActive(int p)
{
	fcActive=p;
	fcAct[p/2]=p;
}

// Files changed: every panel window reads its directories again when drawn.
void FcReloadAll()
{
	for(int c=0;c<FC_MAXC;c++)
		fcReload[c]=1;
}

// The panel window in front of the others, or -1.
int FcTopC()
{
	for(int i=tvNZ-1;i>=0;i--)
		if(tvWin[tvZ[i]].type==W_FC)return tvWin[tvZ[i]].a;
	return -1;
}

// ---------------------------------------------------------------- archives

// arc_dl.so (./build_so.sh) shows archives as read-only folders: a path running
// through an archive file, as "/home/me/a.zip/docs", names something in it. Its
// listing calls mirror term_dl's, so the panel calls below go to one or the other:
// LsName and the rest to the listing of the panel FcUse picked.
#define ARC_DL "arc_dl"
[[scinc::dynlink(ARC_DL,"arc_where_wrapper")]]int arc_where(char* path, char* file, int fmax, char* inner, int imax);
[[scinc::dynlink(ARC_DL,"arc_is_wrapper")]]int arc_is(char* path);
[[scinc::dynlink(ARC_DL,"arc_isdir_wrapper")]]int arc_isdir(char* path);
[[scinc::dynlink(ARC_DL,"arc_size_wrapper")]]int arc_size(char* path);
[[scinc::dynlink(ARC_DL,"arc_load_wrapper")]]int arc_load(char* path, char* buf, int max);
[[scinc::dynlink(ARC_DL,"arc_stamp_wrapper")]]int arc_stamp(char* path);
[[scinc::dynlink(ARC_DL,"arc_error_wrapper")]]void arc_error(char* out, int max);
[[scinc::dynlink(ARC_DL,"arc_temp_wrapper")]]int arc_temp(char* path, char* out, int max);
[[scinc::dynlink(ARC_DL,"arc_tempdir_wrapper")]]void arc_tempdir(char* out, int max);
[[scinc::dynlink(ARC_DL,"arc_tmpname_wrapper")]]int arc_tmpname(char* name, char* out, int max);
[[scinc::dynlink(ARC_DL,"arc_tmpdrop_wrapper")]]void arc_tmpdrop(char* path);
[[scinc::dynlink(ARC_DL,"arc_slot_wrapper")]]int arc_slot(int k);
[[scinc::dynlink(ARC_DL,"arc_sort_wrapper")]]void arc_sort(int mode);
[[scinc::dynlink(ARC_DL,"arc_list_wrapper")]]int arc_list(char* path, char* resolved, int max, int showHidden);
[[scinc::dynlink(ARC_DL,"arc_name_wrapper")]]int arc_name(int i, char* out, int max);
[[scinc::dynlink(ARC_DL,"arc_index_wrapper")]]int arc_index(char* name);
[[scinc::dynlink(ARC_DL,"arc_find_wrapper")]]int arc_find(int letter, int start);
[[scinc::dynlink(ARC_DL,"arc_field_wrapper")]]void arc_field(int i, int k, char* out, int max);
[[scinc::dynlink(ARC_DL,"arc_mark_wrapper")]]int arc_mark(int i, int v);
[[scinc::dynlink(ARC_DL,"arc_marked_wrapper")]]int arc_marked(int i);
[[scinc::dynlink(ARC_DL,"arc_mark_match_wrapper")]]int arc_mark_match(int v, char* pattern, int dirsToo);
[[scinc::dynlink(ARC_DL,"arc_mark_info_wrapper")]]int arc_mark_info(char* out, int max);
[[scinc::dynlink(ARC_DL,"arc_prepare_wrapper")]]int arc_prepare(char* path);
[[scinc::dynlink(ARC_DL,"arc_progress_wrapper")]]int arc_progress(char* out, int max);
[[scinc::dynlink(ARC_DL,"arc_cancel_wrapper")]]void arc_cancel(char* path);
[[scinc::dynlink(ARC_DL,"arc_tree_wrapper")]]int arc_tree(int id, char* path);
[[scinc::dynlink(ARC_DL,"arc_row_wrapper")]]int arc_row(int id, int i, int k, char* out, int max);
[[scinc::dynlink(ARC_DL,"arc_summary_wrapper")]]void arc_summary(int id, char* out, int max);
[[scinc::dynlink(ARC_DL,"arc_job_clear_wrapper")]]void arc_job_clear();
[[scinc::dynlink(ARC_DL,"arc_job_add_wrapper")]]void arc_job_add(char* path);
[[scinc::dynlink(ARC_DL,"arc_job_start_wrapper")]]int arc_job_start(char* dst);
[[scinc::dynlink(ARC_DL,"arc_job_step_wrapper")]]int arc_job_step(int ms);
[[scinc::dynlink(ARC_DL,"arc_job_answer_wrapper")]]void arc_job_answer(int k);
[[scinc::dynlink(ARC_DL,"arc_job_info_wrapper")]]int arc_job_info(int k);
[[scinc::dynlink(ARC_DL,"arc_job_name_wrapper")]]void arc_job_name(int k, char* out, int max);

int fcInArc[FC_MAXP];       // the panel shows a directory in an archive
int fcArcNow=0;             // the listing FcUse picked is an archive's
int fcJobArc=0;             // the job is a copy out of an archive
char fcArcFile[FC_PATH];    // arc_where's answers
char fcArcInner[FC_PATH];

// Every listing call works on the slot of a panel, panel+1 (0 is the file dialog's).
void FcUse(int p)
{
	fs_slot(p+1);
	arc_slot(p+1);
	fcArcNow=fcInArc[p];
}

int LsName(int i, char* out, int max)
{
	if(fcArcNow)return arc_name(i,out,max);
	return fs_name(i,out,max);
}

void LsField(int i, int k, char* out, int max)
{
	if(fcArcNow)arc_field(i,k,out,max);
	else fs_field(i,k,out,max);
}

int LsMark(int i, int v)
{
	if(fcArcNow)return arc_mark(i,v);
	return fs_mark(i,v);
}

int LsMarked(int i)
{
	if(fcArcNow)return arc_marked(i);
	return fs_marked(i);
}

int LsMarkMatch(int v, char* pattern, int dirsToo)
{
	if(fcArcNow)return arc_mark_match(v,pattern,dirsToo);
	return fs_mark_match(v,pattern,dirsToo);
}

int LsMarkInfo(char* out, int max)
{
	if(fcArcNow)return arc_mark_info(out,max);
	return fs_mark_info(out,max);
}

int LsIndex(char* name)
{
	if(fcArcNow)return arc_index(name);
	return fs_index(name);
}

int LsFind(int letter, int start)
{
	if(fcArcNow)return arc_find(letter,start);
	return fs_find(letter,start);
}

// 0 an ordinary path, 1 an archive file's top, 2 something in an archive.
int FcArcWhere(char* path)
{
	return arc_where(path,&fcArcFile[0],FC_PATH,&fcArcInner[0],FC_PATH);
}

// A directory, or one in an archive; an archive file itself is a file here.
int FcIsDir(char* path)
{
	if(FcArcWhere(path)==2)return arc_isdir(path);
	return fs_isdir(path);
}

int FcLoadFile(char* path, char* buf, int max)
{
	if(FcArcWhere(path)==2)return arc_load(path,buf,max);
	return fs_load(path,buf,max);
}

int FcStampOf(char* path)
{
	if(FcArcWhere(path)!=0)return arc_stamp(path);
	return fs_stamp(path);
}

void FcError(char* out, int max)
{
	if(fcJobArc)arc_error(out,max);
	else fs_error(out,max);
}

// The free space of the disk under a directory, or under the archive it is in.
int FcDisk(char* path, char* out, int max)
{
	if(FcArcWhere(path)==0)return fs_disk(path,out,max);
	int base=0;
	int k=0;
	while(fcArcFile[k]!=0)
	{
		if(fcArcFile[k]=='/')base=k;
		k++;
	}
	fcArcFile[base]=0;
	if(base==0)StrCpy(&fcArcFile[0],"/",FC_PATH);
	return fs_disk(&fcArcFile[0],out,max);
}

// A copy out of an archive runs in arc_dl, the other jobs in term_dl.
void JobClear()
{
	fs_job_clear();
	arc_job_clear();
}

void JobAdd(char* path)
{
	if(fcJobArc)arc_job_add(path);
	else fs_job_add(path);
}

int JobStep(int ms)
{
	if(fcJobArc)return arc_job_step(ms);
	return fs_job_step(ms);
}

void JobAnswer(int k)
{
	if(fcJobArc)arc_job_answer(k);
	else fs_job_answer(k);
}

int JobInfo(int k)
{
	if(fcJobArc)return arc_job_info(k);
	return fs_job_info(k);
}

void JobName(int k, char* out, int max)
{
	if(fcJobArc)arc_job_name(k,out,max);
	else fs_job_name(k,out,max);
}

// Says so and returns 1 if the active panel is in an archive, which is read-only.
int FcReadOnly(char* title)
{
	if(!fcInArc[fcActive])return 0;
	TvMessage(title,"This is inside an archive, which t2fc opens read-only.\nF5 copies files out of it.",0);
	return 1;
}

char* FcDirOf(int p)
{
	return &fcDir[p*FC_PATH];
}

void FcJoin(char* out, char* dir, char* name)
{
	if((dir[0]=='/')&&(dir[1]==0))snprintf(out,FC_PATH,"/%s",name);
	else snprintf(out,FC_PATH,"%s/%s",dir,name);
}

int FcIsUp(char* s)
{
	if(s[0]!='.')return 0;
	if(s[1]!='.')return 0;
	if(s[2]!=0)return 0;
	return 1;
}

// The entry's name, and 1 if it is a directory.
int FcName(int p, int i, char* out)
{
	FcUse(p);
	return LsName(i,out,256);
}

void FcPathOf(int p, int i, char* out)
{
	char name[256];
	FcName(p,i,&name[0]);
	FcJoin(out,FcDirOf(p),&name[0]);
}

// Rows of entries a panel shows, from the window's size.
int FcRows(int p)
{
	int r=tvWin[fcWinOf[p/2]].h-6;
	if(r<1)r=1;
	return r;
}

int FcVisible(int p)
{
	int r=FcRows(p);
	if(fcBrief[p])return r*2;
	return r;
}

void FcClamp(int p)
{
	int n=fcCount[p];
	if(fcCur[p]>=n)fcCur[p]=n-1;
	if(fcCur[p]<0)fcCur[p]=0;
	int vis=FcVisible(p);
	if(fcCur[p]<fcTop[p])fcTop[p]=fcCur[p];
	if(fcCur[p]>=fcTop[p]+vis)fcTop[p]=fcCur[p]-vis+1;
	if(fcTop[p]>n-vis)fcTop[p]=n-vis;
	if(fcTop[p]<0)fcTop[p]=0;
}

// Lists a directory in panel p with selectName under the cursor if it is there.
// Returns 0, keeping the panel as it was, if the directory cannot be read.
int FcArcReady(char* path);
int fcArcCancelled=0;      // the last FcArcReady was cancelled: say nothing more
int fcArcWin=-1;           // the dialog waiting for an archive's index, or -1
char fcArcWaitPath[FC_PATH];
int fcArcBar=-1;
int fcQArcWait=0;          // quick view waits for an archive's index
char fcQArcWaitPath[FC_PATH];


int FcLoad(int p, char* path, char* selectName)
{
	char resolved[FC_PATH];
	if(!FcArcReady(path))return 0;
	FcUse(p);
	fs_sort(fcSort[p]);
	arc_sort(fcSort[p]);
	// arc_list takes a path in an archive; for any other it gives the path with . and
	// .. taken out, which may have left an archive
	int n=arc_list(path,&resolved[0],FC_PATH,fcHidden);
	if(n==-1)return 0;
	int inArc=1;
	if(n==-2)
	{
		char plain[FC_PATH];
		StrCpy(&plain[0],&resolved[0],FC_PATH);
		n=fs_list(&plain[0],&resolved[0],FC_PATH,fcHidden);
		if(n<0)return 0;
		inArc=0;
	}
	fcInArc[p]=inArc;
	fcArcNow=inArc;
	int same=1;
	int k=0;
	while(same)
	{
		if(resolved[k]!=fcDir[p*FC_PATH+k])same=0;
		if(resolved[k]==0)break;
		k++;
	}
	StrCpy(FcDirOf(p),&resolved[0],FC_PATH);
	fcCount[p]=n;
	fcStamp[p]=FcStampOf(&resolved[0]);
	int at=LsIndex(selectName);
	if(at>=0)fcCur[p]=at;
	else if(!same)
	{
		fcCur[p]=0;
		fcTop[p]=0;
	}
	FcClamp(p);
	return 1;
}

// Lists the panel's directory again, keeping the cursor on the same name, or goes
// up until a directory can be read if this one is gone.
void FcQForget(int p);

void FcReload(int p)
{
	FcQForget(p);
	char keep[256];
	FcName(p,fcCur[p],&keep[0]);
	char path[FC_PATH];
	StrCpy(&path[0],FcDirOf(p),FC_PATH);
	int tries=0;
	while(!FcLoad(p,&path[0],&keep[0]))
	{
		int base=0;
		int k=0;
		while(path[k]!=0)
		{
			if(path[k]=='/')base=k;
			k++;
		}
		if(base==0)
		{
			StrCpy(&path[0],"/",FC_PATH);
			tries++;
			if(tries>1)return;
		}
		else path[base]=0;
	}
}

void FcReloadBoth()
{
	FcReload(fcBase);
	FcReload(fcBase+1);
}

// Lists the parent directory with the one we left under the cursor.
void FcGoUp(int p)
{
	char* dir=FcDirOf(p);
	if((dir[0]=='/')&&(dir[1]==0))return;
	int base=0;
	int k=0;
	while(dir[k]!=0)
	{
		if(dir[k]=='/')base=k+1;
		k++;
	}
	char left[256];
	StrCpy(&left[0],&dir[base],256);
	char path[FC_PATH];
	FcJoin(&path[0],dir,"..");
	if(!FcLoad(p,&path[0],&left[0]))
		TvMessage("Error","Cannot read the parent directory.",0);
}

void FcChangeDir(int p, char* path)
{
	fcArcCancelled=0;
	if(!FcLoad(p,path,""))
		if(!fcArcCancelled)TvMessage("Error","Cannot read this directory.",0);
}

// ---------------------------------------------------------------- drawing the panels

int FcEndsWith(char* s, char* ext)
{
	int n=StrLen(s);
	int m=StrLen(ext);
	if(n<m)return 0;
	for(int i=0;i<m;i++)
		if(LowerKey(s[n-m+i])!=ext[i])return 0;
	return 1;
}

int FcIsArchive(char* s)
{
	if(FcEndsWith(s,".zip"))return 1;
	if(FcEndsWith(s,".tar"))return 1;
	if(FcEndsWith(s,".gz"))return 1;
	if(FcEndsWith(s,".tgz"))return 1;
	if(FcEndsWith(s,".bz2"))return 1;
	if(FcEndsWith(s,".xz"))return 1;
	if(FcEndsWith(s,".zst"))return 1;
	if(FcEndsWith(s,".7z"))return 1;
	if(FcEndsWith(s,".rar"))return 1;
	if(FcEndsWith(s,".deb"))return 1;
	if(FcEndsWith(s,".rpm"))return 1;
	return 0;
}

int FcIsMedia(char* s)
{
	if(FcEndsWith(s,".png"))return 1;
	if(FcEndsWith(s,".jpg"))return 1;
	if(FcEndsWith(s,".jpeg"))return 1;
	if(FcEndsWith(s,".gif"))return 1;
	if(FcEndsWith(s,".bmp"))return 1;
	if(FcEndsWith(s,".svg"))return 1;
	if(FcEndsWith(s,".svgz"))return 1;
	if(FcEndsWith(s,".mp3"))return 1;
	if(FcEndsWith(s,".wav"))return 1;
	if(FcEndsWith(s,".ogg"))return 1;
	if(FcEndsWith(s,".flac"))return 1;
	if(FcEndsWith(s,".mp4"))return 1;
	if(FcEndsWith(s,".mkv"))return 1;
	if(FcEndsWith(s,".avi"))return 1;
	if(FcEndsWith(s,".webm"))return 1;
	return 0;
}

int FcIsJunk(char* s)
{
	if(FcEndsWith(s,"~"))return 1;
	if(FcEndsWith(s,".bak"))return 1;
	if(FcEndsWith(s,".tmp"))return 1;
	if(FcEndsWith(s,".o"))return 1;
	if(FcEndsWith(s,".swp"))return 1;
	return 0;
}

// The colour of an entry: by type, then by name, as the panels of MC colour them.
int FcAttrOf(int isDir, int mark, char* name)
{
	if(mark=='@')return FC_LINK;
	if(isDir)return FC_DIR;
	if(mark=='*')return FC_EXEC;
	if(mark!=' ')return FC_LINK;
	if(FcIsArchive(name))return FC_ARCH;
	if(FcIsMedia(name))return FC_MEDIA;
	if(FcIsJunk(name))return FC_JUNK;
	return FC_FILE;
}

// Writes s into w columns at x, ending in } if it had to be cut, as MC does.
void FcCut(int x, int y, char* s, int at, int w)
{
	if(w<=0)return;
	if(TvCols(s)<=w)
	{
		TvText(x,y,s,at);
		return;
	}
	TvTextN(x,y,s,at,w-1);
	TvPut(x+w-1,y,'}',at);
}

// Right-aligns s in w columns.
void FcRight(int x, int y, char* s, int at, int w)
{
	int n=TvCols(s);
	if(n>w)n=w;
	TvTextN(x+w-n,y,s,at,n);
}

// ---------------------------------------------------------------- pictures

// img_dl.so (build_so.sh here) decodes pictures on threads of its own. In a terminal
// it draws them straight into the screen arrays as half-block cells, two pixels a
// cell; in a graphic window it gives the backend real pixels (TvPixels). The view
// works in those pixels either way: TvCellPxW by TvCellPxH of them a cell. A
// picture lives in a slot: a picture window uses its window's number, the quick
// view of panel p uses 16+p.
#define IMG_DL "img_dl"
int StrCmp(char* a, char* b);
[[scinc::dynlink(IMG_DL,"img_is_wrapper")]]int img_is(char* path);
[[scinc::dynlink(IMG_DL,"img_sniff_wrapper")]]int img_sniff(char* head, int n, char* name);
[[scinc::dynlink(IMG_DL,"img_open_wrapper")]]int img_open(int s, char* path);
[[scinc::dynlink(IMG_DL,"img_free_wrapper")]]void img_free(int s);
[[scinc::dynlink(IMG_DL,"img_state_wrapper")]]int img_state(int s);
[[scinc::dynlink(IMG_DL,"img_error_wrapper")]]void img_error(int s, char* buf, int max);
[[scinc::dynlink(IMG_DL,"img_format_wrapper")]]void img_format(int s, char* buf, int max);
[[scinc::dynlink(IMG_DL,"img_info_wrapper")]]int img_info(int s, int k);
[[scinc::dynlink(IMG_DL,"img_delay_wrapper")]]int img_delay(int s, int f);
[[scinc::dynlink(IMG_DL,"img_rotate_wrapper")]]void img_rotate(int s);
[[scinc::dynlink(IMG_DL,"img_pixels_wrapper")]]int img_pixels(int s, int f, float cx, float cy, float scale, int fullW, int fullH, int ox, int oy, int w, int h, int bg, int* out);
[[scinc::dynlink(IMG_DL,"img_video_opts_wrapper")]]void img_video_opts(int s, int maxW, int maxH, int fps, int loop);
[[scinc::dynlink(IMG_DL,"img_video_ctl_wrapper")]]void img_video_ctl(int s, int op, int arg);
[[scinc::dynlink(IMG_DL,"img_filter_wrapper")]]void img_filter(int mode);
[[scinc::dynlink(IMG_DL,"img_draw_wrapper")]]int img_draw(int s, int f, float cx, float cy, float scale, float aspect, int x0, int y0, int cw, int ch, int* cells, int* attrs, int stride, int clx0, int cly0, int clx1, int cly1, int uphalf, int bg);

#define PV_SLOTS 32
#define PV_QUICK 16          // the quick view slots start here

int pvUsed[PV_SLOTS];
int pvSeen[PV_SLOTS];        // the state of the slot when it was last drawn
float pvCX[PV_SLOTS];        // the picture point in the middle of the view
float pvCY[PV_SLOTS];
int pvLanczos=0;             // pictures shown smaller go through a Lanczos filter
float pvScale[PV_SLOTS];     // picture pixels per screen pixel; 0 fits the view
int pvFrame[PV_SLOTS];       // the animation frame shown
int pvNext[PV_SLOTS];        // and when the next one is due
char pvPath[32768];          // PV_SLOTS paths of FC_PATH
float pvAspect=1.;           // a view pixel's width over its height
int pvGen[PV_SLOTS];         // counts the pictures opened and turned, for TvPixels
int pvVid[PV_SLOTS];         // a video's frame count when it was last drawn

void PvInit()
{
	for(int s=0;s<PV_SLOTS;s++)
	{
		pvUsed[s]=0;
		pvSeen[s]=0;
		pvScale[s]=0.;
		pvCX[s]=0.;
		pvCY[s]=0.;
		pvFrame[s]=0;
		pvNext[s]=0;
		pvGen[s]=0;
		pvVid[s]=0;
		pvPath[s*FC_PATH]=0;
	}
}

char* PvPathOf(int s)
{
	return &pvPath[s*FC_PATH];
}

void FcTickRate();

// 1 if the file is a picture img_dl decodes, in an archive or not.
int FcIsPicture(char* path)
{
	if(FcArcWhere(path)!=2)return img_is(path);
	char head[16];
	int n=arc_load(path,&head[0],16);
	if(n<=0)return 0;
	return img_sniff(&head[0],n,path);
}

// A file img_dl and the editor can open: the path itself, or for a file in an archive
// a temporary copy (arc_dl removes them at the end). Returns 0 if it cannot.
int FcLocal(char* path, char* out)
{
	StrCpy(out,path,FC_PATH);
	if(FcArcWhere(path)!=2)return 1;
	return arc_temp(path,out,FC_PATH);
}

// 1 for a temporary copy from an archive, which is not to be edited.
int FcIsArcCopy(char* path)
{
	char dir[FC_PATH];
	arc_tempdir(&dir[0],FC_PATH);
	if(dir[0]==0)return 0;
	int k=0;
	while(dir[k]!=0)
	{
		if(path[k]!=dir[k])return 0;
		k++;
	}
	return path[k]=='/';
}

void PvOpen(int s, char* path)
{
	StrCpy(PvPathOf(s),path,FC_PATH);
	char local[FC_PATH];
	FcLocal(path,&local[0]);
	// videos: smaller and slower in quick view, which loops them; real pixels in a
	// graphic window
	if(s>=PV_QUICK)img_video_opts(s,320,240,8,1);
	else if(TvHasPixels())img_video_opts(s,960,540,20,0);
	else img_video_opts(s,480,360,12,0);
	img_open(s,&local[0]);
	pvUsed[s]=1;
	pvSeen[s]=-2;
	pvScale[s]=0.;
	pvFrame[s]=0;
	pvNext[s]=0;
	pvGen[s]++;
	tvDirty=1;
	FcTickRate();
}

void PvFree(int s)
{
	if(!pvUsed[s])return;
	img_free(s);
	pvUsed[s]=0;
	pvPath[s*FC_PATH]=0;
	FcTickRate();
}

// Some slot is decoding or shows an animation: the tick must come often.
int PvBusy()
{
	for(int s=0;s<PV_SLOTS;s++)
		if(pvUsed[s])
		{
			int st=img_state(s);
			if(st==1)return 1;
			if(st==2)
			{
				if(img_info(s,2)>1)return 1;
				if(img_info(s,4)&&!img_info(s,7))return 1;
			}
		}
	return 0;
}

// Screen pixels per pixel of the picture's own size at scale 1: a video comes
// decoded smaller than it is, and its zoom counts against its own size.
float PvOwn(int s)
{
	if(img_info(s,4)&&(img_info(s,9)>0))return 1.*img_info(s,0)/img_info(s,9);
	return 1.;
}

// The scale that shows the whole picture in pw*ph screen pixels.
float PvFit(int s, int pw, int ph)
{
	float sx=1.*img_info(s,0)/(pw*pvAspect);
	float sy=1.*img_info(s,1)/ph;
	if(sy>sx)return sy;
	return sx;
}

// Keeps a zoomed picture covering the view, or centred where it is smaller.
void PvClamp(int s, int pw, int ph)
{
	if(pvScale[s]==0.)return;
	float w=1.*img_info(s,0);
	float h=1.*img_info(s,1);
	float hw=pw*pvScale[s]*pvAspect/2.;
	float hh=ph*pvScale[s]/2.;
	if(w<=2.*hw)pvCX[s]=w/2.;
	else if(pvCX[s]<hw)pvCX[s]=hw;
	else if(pvCX[s]>w-hw)pvCX[s]=w-hw;
	if(h<=2.*hh)pvCY[s]=h/2.;
	else if(pvCY[s]<hh)pvCY[s]=hh;
	else if(pvCY[s]>h-hh)pvCY[s]=h-hh;
}

// Sets the scale keeping the picture point at screen pixel px,py where it is.
void PvSetScale(int s, float scale, int px, int py, int pw, int ph)
{
	if(img_state(s)!=2)return;
	float fit=PvFit(s,pw,ph);
	float cur=pvScale[s];
	if(cur==0.)
	{
		cur=fit;
		pvCX[s]=img_info(s,0)/2.;
		pvCY[s]=img_info(s,1)/2.;
	}
	float wx=pvCX[s]+(px-pw/2.)*cur*pvAspect;
	float wy=pvCY[s]+(py-ph/2.)*cur;
	if(scale<PvOwn(s)/8.)scale=PvOwn(s)/8.;
	if(scale>=fit)
	{
		pvScale[s]=0.;
		tvDirty=1;
		return;
	}
	pvScale[s]=scale;
	pvCX[s]=wx-(px-pw/2.)*scale*pvAspect;
	pvCY[s]=wy-(py-ph/2.)*scale;
	PvClamp(s,pw,ph);
	tvDirty=1;
}

// The zoom levels, as screen pixels per pixel of the picture: 8 4 3 2 1, then
// 1/2 1/3 1/4 and on by halves. The next one above m (dir 1) or below it (-1),
// or 0 when there is none.
float PvLevel(float m, int dir)
{
	float up[5];
	up[0]=8.;
	up[1]=4.;
	up[2]=3.;
	up[3]=2.;
	up[4]=1.;
	float best=0.;
	for(int k=0;k<40;k++)
	{
		float l=0.;
		if(k<5)l=up[k];
		else if(k==5)l=1./2.;
		else if(k==6)l=1./3.;
		else l=1./(1<<(k-5));        // 1/4 1/8 1/16 ...
		if(l<1./4096.)break;
		if(dir>0)
		{
			if(l>m*1.001)best=l;     // the list goes down: the last above m
		}
		else if(l<m*0.999)return l;
	}
	return best;
}

// One zoom level in (dir 1) or out (-1), keeping screen pixel px,py where it is;
// past the whole picture going out it fits the view again.
void PvZoom(int s, int dir, int px, int py, int pw, int ph)
{
	float cur=pvScale[s];
	if(cur==0.)cur=PvFit(s,pw,ph);
	float own=PvOwn(s);
	float l=PvLevel(own/cur,dir);
	if(l==0.)
	{
		if(dir>0)return;           // 8x already
		pvScale[s]=0.;
		tvDirty=1;
		return;
	}
	PvSetScale(s,own/l,px,py,pw,ph);
}

// Moves a zoomed picture by dx,dy screen pixels.
void PvPan(int s, int dx, int dy, int pw, int ph)
{
	if(pvScale[s]==0.)return;
	pvCX[s]=pvCX[s]+dx*pvScale[s]*pvAspect;
	pvCY[s]=pvCY[s]+dy*pvScale[s];
	PvClamp(s,pw,ph);
	tvDirty=1;
}

// The scale and middle point slot s shows at in a view of pw*ph pixels.
void PvView(int s, int pw, int ph, float& scale, float& cx, float& cy)
{
	scale=pvScale[s];
	cx=pvCX[s];
	cy=pvCY[s];
	if(scale==0.)
	{
		scale=PvFit(s,pw,ph);
		cx=img_info(s,0)/2.;
		cy=img_info(s,1)/2.;
	}
}

// m:ss, or h:mm:ss.
void PvClock(int ms, char* out, int max)
{
	int t=ms/1000;
	if(t>=3600)snprintf(out,max,"%i:%02i:%02i",t/3600,(t/60)%60,t%60);
	else snprintf(out,max,"%i:%02i",t/60,t%60);
}

// "PNG 1920x1080", with the frame of an animation, or the time of a video.
void PvInfo(int s, char* out, int max)
{
	char fmt[16];
	img_format(s,&fmt[0],16);
	if(img_info(s,4))
	{
		char at[16];
		char len[16];
		PvClock(img_info(s,5),&at[0],16);
		len[0]=0;
		if(img_info(s,6)>0)
		{
			len[0]='/';
			PvClock(img_info(s,6),&len[1],15);
		}
		char* st="";
		if(img_info(s,11))st=", end";
		else if(img_info(s,7))st=", paused";
		int w=img_info(s,9);
		int h=img_info(s,10);
		if(w==0)
		{
			// not known yet (GStreamer looks it up beside the playing)
			w=img_info(s,0);
			h=img_info(s,1);
		}
		snprintf(out,max,"%s %ix%i, %s%s%s",&fmt[0],w,h,&at[0],&len[0],st);
		return;
	}
	int n=img_info(s,2);
	if(n>1)snprintf(out,max,"%s %ix%i, frame %i/%i",&fmt[0],img_info(s,0),img_info(s,1),pvFrame[s]+1,n);
	else snprintf(out,max,"%s %ix%i",&fmt[0],img_info(s,0),img_info(s,1));
}

// Draws slot s into w*h cells at x,y: the picture, or why there is none yet.
void PvDraw(int s, int x, int y, int w, int h)
{
	int st=img_state(s);
	pvSeen[s]=st;
	TvFill(x,y,w,h,' ',0x07);
	if((w<1)||(h<1))return;
	if(st==1)
	{
		char* msg="Loading...";
		TvText(x+(w-StrLen(msg))/2,y+h/2,msg,0x07);
		return;
	}
	if(st==-1)
	{
		char err[200];
		img_error(s,&err[0],200);
		FcCut(x+1,y+h/2,&err[0],0x0C,w-2);
		return;
	}
	if(st!=2)return;
	if(TvHasPixels())
	{
		// what the picture looks like now, so that the backend asks for it only
		// when that changes
		int key=pvGen[s]*7919+pvFrame[s]*104729+img_info(s,8)*15485863+(int)(pvCX[s]*8.)*31+(int)(pvCY[s]*8.)*1009+(int)(pvScale[s]*1000000.)+pvLanczos*7;
		TvPixels(s,x,y,w,h,key);
		return;
	}
	float scale=0.;
	float cx=0.;
	float cy=0.;
	PvView(s,w,h*2,scale,cx,cy);
	img_draw(s,pvFrame[s],cx,cy,scale,pvAspect,x,y,w,h,&tvCh[0],&tvAt[0],TV_MAXW,clipX0,clipY0,clipX1,clipY1,G_UPHALF,0);
}

// The pixels of slot id for the graphic backend: its view is fullW*fullH pixels.
void TvAppPixels(int id, int* buf, int fullW, int fullH, int ox, int oy, int w, int h)
{
	float scale=0.;
	float cx=0.;
	float cy=0.;
	PvView(id,fullW,fullH,scale,cx,cy);
	if(!img_pixels(id,pvFrame[id],cx,cy,scale,fullW,fullH,ox,oy,w,h,0,buf))
		for(int i=0;i<w*h;i++)
			buf[i]=0xFF000000;
}

// Notices finished decoding and moves animations on.
void PvTick()
{
	int now=term_ms();
	for(int s=0;s<PV_SLOTS;s++)
		if(pvUsed[s])
		{
			int st=img_state(s);
			if(st!=pvSeen[s])tvDirty=1;
			if(st==2)
			{
				int n=img_info(s,2);
				if(img_info(s,4))
				{
					// a video: a new frame, or a new time
					int f=img_info(s,8)*2+img_info(s,7);
					if(f!=pvVid[s])tvDirty=1;
					pvVid[s]=f;
				}
				else if((n>1)&&(now>=pvNext[s]))
				{
					if(pvNext[s]!=0)pvFrame[s]=(pvFrame[s]+1)%n;
					pvNext[s]=now+img_delay(s,pvFrame[s]);
					tvDirty=1;
				}
			}
		}
	FcTickRate();
}

// ---------------------------------------------------------------- quick view

// A panel in quick view shows the file under the other panel's cursor: a picture,
// the start of a text, a hex dump, or what a directory is.
int fcQuick[FC_MAXP];
char fcQPath[12288];        // FC_MAXP paths: what each quick view shows
int fcQKind[FC_MAXP];       // 1 picture, 2 text, 3 binary, 4 directory, 5 unreadable
char fcQText[16384];        // the start of the last text or binary file shown
int fcQTextN=0;
char fcQTextPath[FC_PATH];
char fcQArcPath[FC_PATH];   // the archive whose tree quick view shows
int fcQArcN=0;
int fcQScanSeen[FC_MAXP];   // the scan state at the last draw, to draw again when it changes

// Reads the start of a file into fcQText, unless it is there already.
void FcQLoad(char* path)
{
	if(StrCmp(&fcQTextPath[0],path)==0)return;
	fcQTextN=FcLoadFile(path,&fcQText[0],16384);
	StrCpy(&fcQTextPath[0],path,FC_PATH);
}

void FcQText(int x, int y, int w, int h)
{
	char line[600];
	int i=0;
	for(int r=0;r<h;r++)
	{
		if(i>=fcQTextN)return;
		int n=0;
		while((i<fcQTextN)&&(fcQText[i]!=10)&&(n<590))
		{
			int c=fcQText[i];
			if(c==9)
			{
				line[n]=' ';
				n++;
				while((n%4)!=0)
				{
					line[n]=' ';
					n++;
				}
			}
			else
			{
				if((c>=0)&&(c<32))c='.';
				line[n]=c;
				n++;
			}
			i++;
		}
		// a long line is cut, and the rest of it skipped
		while((i<fcQTextN)&&(fcQText[i]!=10))i++;
		i++;
		line[n]=0;
		FcCut(x,y+r,&line[0],FC_FILE,w);
	}
}

void FcQHex(int x, int y, int w, int h)
{
	int per=16;
	if(w<76)per=8;
	if(w<42)per=4;
	char line[120];
	char part[8];
	for(int r=0;r<h;r++)
	{
		int o=r*per;
		if(o>=fcQTextN)return;
		snprintf(&line[0],120,"%06x ",o);
		int n=StrLen(&line[0]);
		for(int k=0;k<per;k++)
		{
			if(o+k<fcQTextN)snprintf(&part[0],8,"%02x ",fcQText[o+k]&255);
			else StrCpy(&part[0],"   ",8);
			StrCpy(&line[n],&part[0],4);
			n=n+3;
		}
		for(int k=0;k<per;k++)
			if(o+k<fcQTextN)
			{
				int c=fcQText[o+k]&255;
				if((c<32)||(c>126))c='.';
				line[n]=c;
				n++;
			}
		line[n]=0;
		FcCut(x,y+r,&line[0],FC_FILE,w);
	}
}

// What the quick view of panel p should show now, worked out again only when the
// other panel's cursor moves to another file.
void FcQUpdate(int p, char* path, int isDir)
{
	char* was=&fcQPath[p*FC_PATH];
	if(StrCmp(was,path)==0)return;
	StrCpy(was,path,FC_PATH);
	int kind=4;
	if(!isDir)
	{
		if(FcArcWhere(path)==1)kind=6;
		else if(FcIsPicture(path))kind=1;
		else
		{
			fcQTextPath[0]=0;
			FcQLoad(path);
			kind=2;
			if(fcQTextN<0)kind=5;
			for(int i=0;i<fcQTextN;i++)
				if(fcQText[i]==0)kind=3;
		}
	}
	fcQKind[p]=kind;
	if(kind==1)PvOpen(PV_QUICK+p,path);
	else PvFree(PV_QUICK+p);
	// a directory is counted on a thread of term_dl's, as long as that takes; not
	// the parent (which holds this one and more), nor one inside an archive
	int scan=0;
	if((kind==4)&&(fcQSecs>0)&&!fcInArc[p^1])
	{
		int n=StrLen(path);
		scan=1;
		if((n>=3)&&(path[n-1]=='.')&&(path[n-2]=='.')&&(path[n-3]=='/'))scan=0;
	}
	if(scan)dir_scan(p,path,fcQSecs*1000);
	else dir_scan_stop(p);
	fcQScanSeen[p]=-1;
	FcTickRate();
}

// Sizes as people read them: 512 bytes, 12.3 MiB.
void FcQSize(int kib, char* bytes, char* out, int max)
{
	float v=kib;
	if(kib<=1)snprintf(out,max,"%s bytes",bytes);
	else if(kib<1024)
	{
		// exact: below a MiB the bytes fit in an int
		int b=0;
		for(int i=0;bytes[i]!=0;i++)
			b=b*10+bytes[i]-'0';
		float k=b;
		snprintf(out,max,"%.1f KiB (%s bytes)",k/1024.,bytes);
	}
	else if(kib<1048576)snprintf(out,max,"%.1f MiB (%s bytes)",v/1024.,bytes);
	else if(kib<1073741824)snprintf(out,max,"%.1f GiB (%s bytes)",v/1048576.,bytes);
	else snprintf(out,max,"%.1f TiB (%s bytes)",v/1073741824.,bytes);
}

// "1 file", "3 files"
void FcQCount(char* out, int max, int n, char* one, char* many)
{
	if(n==1)snprintf(out,max,"1 %s",one);
	else snprintf(out,max,"%d %s",n,many);
}

// "3 files, 1 folder", with ", 2 other" when there are other entries.
void FcQCounts(char* out, int max, int files, int dirs, int other)
{
	char f[40];
	char d[40];
	FcQCount(&f[0],40,files,"file","files");
	FcQCount(&d[0],40,dirs,"folder","folders");
	if(other>0)snprintf(out,max,"%s, %s, %d other",&f[0],&d[0],other);
	else snprintf(out,max,"%s, %s",&f[0],&d[0]);
}

// What the scan of the directory in quick view p found: the entries directly in it,
// and all that is under it, with the bytes of its files.
void FcQDir(int p, int x, int y, int w, int h, char* info)
{
	int st=dir_scan_info(p,0);
	fcQScanSeen[p]=st;
	if(st==0)return;
	char line[300];
	if(st==4)
	{
		FcCut(x,y,"The folder cannot be read.",FC_FILE,w);
		return;
	}
	int r=0;
	if(dir_scan_info(p,10))
	{
		char c[120];
		FcQCounts(&c[0],120,dir_scan_info(p,7),dir_scan_info(p,8),dir_scan_info(p,9));
		snprintf(&line[0],300,"Holds     %s",&c[0]);
		FcCut(x,y+r,&line[0],FC_FILE,w);
		r++;
	}
	if(r<h)
	{
		char* lead="In all    ";
		if(st==3)lead="At least  ";
		char c[120];
		FcQCounts(&c[0],120,dir_scan_info(p,1),dir_scan_info(p,2),dir_scan_info(p,3));
		snprintf(&line[0],300,"%s%s",lead,&c[0]);
		FcCut(x,y+r,&line[0],FC_FILE,w);
		r++;
	}
	char bytes[40];
	char size[100];
	dir_scan_bytes(p,&bytes[0],40);
	FcQSize(dir_scan_info(p,4),&bytes[0],&size[0],100);
	if(r<h)
	{
		snprintf(&line[0],300,"Size      %s",&size[0]);
		FcCut(x,y+r,&line[0],FC_FILE,w);
		r++;
	}
	r++;
	int denied=dir_scan_info(p,6);
	if(st==1)StrCpy(&line[0],"Counting...",300);
	else if(st==3)snprintf(&line[0],300,"Stopped after %d s: there is more",fcQSecs);
	else line[0]=0;
	if(r<h)
	{
		FcCut(x,y+r,&line[0],FC_FILE,w);
		r++;
	}
	if((denied>0)&&(r<h))
	{
		snprintf(&line[0],300,"%d folders could not be read",denied);
		FcCut(x,y+r,&line[0],FC_FILE,w);
	}
	// in short, for the mini status line
	char kb[40];
	StrCpy(&kb[0],&size[0],40);
	int cut=0;
	while((kb[cut]!=0)&&(kb[cut]!='('))cut++;
	if((kb[cut]=='(')&&(cut>0))kb[cut-1]=0;
	char nf[40];
	FcQCount(&nf[0],40,dir_scan_info(p,1),"file","files");
	snprintf(info,120,"%s, %s",&nf[0],&kb[0]);
	if(st==3)snprintf(info,120,"at least %s, %s",&nf[0],&kb[0]);
	if(st==1)snprintf(info,120,"counting...");
}

// A scan running or just finished: quick view draws again.
int FcQScanBusy()
{
	int busy=0;
	for(int p=0;p<FC_MAXP;p++)
		if(fcQuick[p]&&(fcQKind[p]==4))
		{
			int st=dir_scan_info(p,0);
			if(st==1)
			{
				busy=1;
				tvDirty=1;
			}
			if(st!=fcQScanSeen[p])tvDirty=1;
		}
	return busy;
}

void FcQArc(char* path, int x, int y, int w, int h, char* info);

void FcDrawQuick(int p, int x, int y, int w, int h)
{
	int q=p^1;
	TvFill(x,y,w,h,' ',FC_BG);
	char* title=" Quick view ";
	TvText(x+(w-StrLen(title))/2,y,title,FC_HEAD);
	for(int k=0;k<w;k++)
		TvPut(x+k,y+h-2,G_HORZ,FC_FRAME);
	if(fcCount[q]==0)return;
	char name[256];
	char path[FC_PATH];
	int isDir=FcName(q,fcCur[q],&name[0]);
	FcJoin(&path[0],FcDirOf(q),&name[0]);
	FcQUpdate(p,&path[0],isDir);
	int kind=fcQKind[p];
	int cy=y+1;
	int chh=h-3;
	char info[120];
	info[0]=0;
	if(fcQArcWait&&((kind!=6)||(StrCmp(&path[0],&fcQArcWaitPath[0])!=0)))
	{
		// the cursor left the archive quick view waited for: no more reading it,
		// unless a panel waits for it too
		if((fcArcWin<0)||(StrCmp(&fcQArcWaitPath[0],&fcArcWaitPath[0])!=0))arc_cancel(&fcQArcWaitPath[0]);
		fcQArcWait=0;
		FcTickRate();
	}
	if(kind==1)
	{
		int s=PV_QUICK+p;
		PvDraw(s,x,cy,w,chh);
		if(img_state(s)==2)PvInfo(s,&info[0],120);
	}
	else if(kind==2)
	{
		FcQLoad(&path[0]);
		FcQText(x,cy,w,chh);
		StrCpy(&info[0],"text",120);
	}
	else if(kind==3)
	{
		FcQLoad(&path[0]);
		FcQHex(x,cy,w,chh);
		StrCpy(&info[0],"binary",120);
	}
	else if(kind==6)FcQArc(&path[0],x,cy,w,chh,&info[0]);
	else if(kind==5)
	{
		char err[300];
		if(fcInArc[q])arc_error(&err[0],300);
		else fs_error(&err[0],300);
		FcCut(x+1,cy+1,&err[0],FC_FILE,w-2);
	}
	else
	{
		char* what="Directory";
		if(FcIsUp(&name[0]))what="The parent directory";
		TvText(x+1,cy+1,what,FC_DIR);
		FcCut(x+1,cy+2,&name[0],FC_DIR,w-2);
		if(chh>4)FcQDir(p,x+1,cy+4,w-2,chh-4,&info[0]);
	}
	// the name and what it is, on the mini status line
	char foot[400];
	if(info[0]!=0)snprintf(&foot[0],400,"%s  %s",&name[0],&info[0]);
	else StrCpy(&foot[0],&name[0],400);
	FcCut(x,y+h-1,&foot[0],FC_FILE,w);
}

void FcQInit()
{
	fcQArcPath[0]=0;
	for(int p=0;p<FC_MAXP;p++)
	{
		fcQuick[p]=0;
		fcQPath[p*FC_PATH]=0;
		fcQKind[p]=0;
		fcQScanSeen[p]=0;
	}
	fcQTextPath[0]=0;
	fcQTextN=0;
}

// Panel p was read again: its files may have changed under the other's quick view.
void FcQForget(int p)
{
	fcQPath[(p^1)*FC_PATH]=0;
	fcQTextPath[0]=0;
}

// Turns the quick view of panel p on or off. The other panel takes the keys.
void FcSetQuick(int p, int on)
{
	if(on)
	{
		if(fcQuick[p^1])FcSetQuick(p^1,0);
		fcQuick[p]=1;
		fcQPath[p*FC_PATH]=0;
		if(fcActive==p)FcSetActive(p^1);
	}
	else
	{
		fcQuick[p]=0;
		fcQPath[p*FC_PATH]=0;
		PvFree(PV_QUICK+p);
		dir_scan_stop(p);
	}
	tvDirty=1;
}

// One entry in a cell w wide. The cursor bar spans the cell.
void FcCell(int p, int i, int x, int y, int w, int sizeX, int timeX, int cursor)
{
	char name[256];
	char mark[4];
	char text[260];
	int isDir=FcName(p,i,&name[0]);
	LsField(i,3,&mark[0],4);
	int m=mark[0];
	int marked=LsMarked(i);
	int at=FcAttrOf(isDir,m,&name[0]);
	if(marked)at=FC_MARK;
	if(cursor)
	{
		at=FC_CUR;
		if(marked)at=FC_CURMARK;
	}
	if(FcIsUp(&name[0]))m='/';
	snprintf(&text[0],260,"%c%s",m,&name[0]);
	int nameW=w;
	if(sizeX>0)nameW=sizeX-x;
	if(cursor)TvFill(x,y,w,1,' ',at);
	FcCut(x,y,&text[0],at,nameW);
	int sepAt=FC_FRAME;
	if(cursor)sepAt=at;
	if(sizeX>0)
	{
		char size[16];
		LsField(i,0,&size[0],16);
		TvPut(sizeX,y,G_VERT,sepAt);
		FcRight(sizeX+1,y,&size[0],at,7);
	}
	if(timeX>0)
	{
		char when[24];
		LsField(i,1,&when[0],24);
		TvPut(timeX,y,G_VERT,sepAt);
		TvTextN(timeX+1,y,&when[0],at,12);
	}
}

// The header of a column, with an arrow if the panel is sorted by it.
void FcHead(int x, int y, int w, char* title, int sorted, int reversed)
{
	char s[40];
	if(sorted)
	{
		if(reversed)snprintf(&s[0],40,"%s%s",title,"\xe2\x96\xb2");
		else snprintf(&s[0],40,"%s%s",title,"\xe2\x96\xbc");
	}
	else StrCpy(&s[0],title,40);
	int n=TvCols(&s[0]);
	if(n>w)n=w;
	TvTextN(x+(w-n)/2,y,&s[0],FC_HEAD,n);
}

// The columns of a full listing: where the size and time separators go, or 0.
void FcColumns(int p, int x, int w, int& sizeX, int& timeX)
{
	sizeX=0;
	timeX=0;
	if(fcBrief[p])return;
	if(w>=36)
	{
		timeX=x+w-13;
		sizeX=timeX-8;
	}
	else if(w>=20)sizeX=x+w-8;
}

void FcDrawPanel(int p, int x, int y, int w, int h)
{
	if(fcQuick[p])
	{
		FcDrawQuick(p,x,y,w,h);
		return;
	}
	int active=(p==fcActive);
	char* dir=FcDirOf(p);
	// the path, its end if it is long
	int at=FC_FRAME;
	if(active)at=FC_PATHON;
	TvFill(x,y,w,1,' ',at);
	int room=w-2;
	int cols=TvCols(dir);
	int i=0;
	int px=x+1;
	if(cols>room)
	{
		for(int k=0;k<cols-room+1;k++)
			TvUtf8(dir,i);
		TvPut(px,y,G_LEFT,at);
		px++;
	}
	TvText(px,y,&dir[i],at);
	// the column headers
	int sizeX=0;
	int timeX=0;
	FcColumns(p,x,w,sizeX,timeX);
	int key=fcSort[p]&3;
	int rev=fcSort[p]&4;
	int byName=0;
	if(key<2)byName=1;
	int rows=h-4;
	if(fcBrief[p])
	{
		int cw=(w-1)/2;
		char* t="Name";
		if(key==1)t="Name.ext";
		FcHead(x,y+1,cw,t,byName,rev);
		FcHead(x+cw+1,y+1,w-cw-1,t,byName,rev);
		for(int r=0;r<rows+1;r++)
			TvPut(x+cw,y+1+r,G_VERT,FC_FRAME);
		TvPut(x+cw,y+h-2,0x2534,FC_FRAME);
	}
	else
	{
		int nameW=w;
		if(sizeX>0)nameW=sizeX-x;
		char* t="Name";
		if(key==1)t="Name.ext";
		FcHead(x,y+1,nameW,t,byName,rev);
		if(sizeX>0)
		{
			TvPut(sizeX,y+1,G_VERT,FC_FRAME);
			FcHead(sizeX+1,y+1,7,"Size",key==2,rev);
		}
		if(timeX>0)
		{
			TvPut(timeX,y+1,G_VERT,FC_FRAME);
			FcHead(timeX+1,y+1,12,"Modified",key==3,rev);
		}
		for(int r=0;r<rows;r++)
		{
			if(sizeX>0)TvPut(sizeX,y+2+r,G_VERT,FC_FRAME);
			if(timeX>0)TvPut(timeX,y+2+r,G_VERT,FC_FRAME);
		}
	}
	// the entries
	FcUse(p);
	for(int r=0;r<rows;r++)
	{
		if(fcBrief[p])
		{
			int cw=(w-1)/2;
			for(int c=0;c<2;c++)
			{
				int k=fcTop[p]+c*rows+r;
				int cx=x;
				int ww=cw;
				if(c==1)
				{
					cx=x+cw+1;
					ww=w-cw-1;
				}
				if(k<fcCount[p])FcCell(p,k,cx,y+2+r,ww,0,0,active&&(k==fcCur[p]));
			}
		}
		else
		{
			int k=fcTop[p]+r;
			if(k<fcCount[p])FcCell(p,k,x,y+2+r,w,sizeX,timeX,active&&(k==fcCur[p]));
		}
	}
	// the line under them, with the free space on the disk
	int ly=y+h-2;
	TvFill(x,ly,w,1,G_HORZ,FC_FRAME);
	if(sizeX>0)TvPut(sizeX,ly,0x2534,FC_FRAME);
	if(timeX>0)TvPut(timeX,ly,0x2534,FC_FRAME);
	if(fcBrief[p])TvPut(x+(w-1)/2,ly,0x2534,FC_FRAME);
	char disk[64];
	char shown[72];
	if(FcDisk(dir,&disk[0],64))
	{
		snprintf(&shown[0],72," %s ",&disk[0]);
		int n=StrLen(&shown[0]);
		if(n<w-2)TvText(x+(w-n)/2,ly,&shown[0],FC_FRAME);
	}
	// the cursor entry, or what is marked, or the quick search
	int iy=y+h-1;
	char info[80];
	FcUse(p);
	int marked=LsMarkInfo(&info[0],80);
	if(fcQS==p)
	{
		TvText(x+1,iy,"Search:",FC_FILE);
		int bw=w-10;
		if(bw>32)bw=32;
		if(bw>2)
		{
			TvFill(x+9,iy,bw,1,' ',A_INPUT);
			int cols=TvCols(&fcQSText[0]);
			int from=0;
			while(cols>bw-1)
			{
				TvUtf8(&fcQSText[0],from);
				cols=TvCols(&fcQSText[from]);
			}
			TvTextN(x+9,iy,&fcQSText[from],A_INPUT,bw);
			if(TvTop()==fcWin)
			{
				tvCurX=x+9+cols;
				tvCurY=iy;
			}
		}
	}
	else if(marked>0)
	{
		int n=TvCols(&info[0]);
		if(n>w)n=w;
		TvTextN(x+(w-n)/2,iy,&info[0],FC_MARK,n);
	}
	else if(fcCount[p]>0)
	{
		char name[256];
		char perm[16];
		char size[16];
		char right[40];
		int k=fcCur[p];
		FcName(p,k,&name[0]);
		LsField(k,2,&perm[0],16);
		LsField(k,0,&size[0],16);
		snprintf(&right[0],40,"%s %s",&size[0],&perm[0]);
		int rn=StrLen(&right[0]);
		if(rn>w-4)rn=0;
		FcCut(x,iy,&name[0],FC_FILE,w-rn-1);
		if(rn>0)TvText(x+w-rn,iy,&right[0],FC_FILE);
	}
}

void FcPanelBox(int p, int& x, int& y, int& w, int& h)
{
	TWin& t=tvWin[fcWin];
	int side=p&1;
	int iw=t.w-2;
	int lw=(iw-1)/2;
	x=t.x+1;
	y=t.y+1;
	w=lw;
	h=t.h-2;
	if(side==1)
	{
		x=t.x+2+lw;
		w=iw-lw-1;
	}
}

void FcDraw(int win)
{
	if(fcReload[fcC])
	{
		fcReload[fcC]=0;
		FcReloadBoth();
	}
	TWin& t=tvWin[win];
	TvFill(t.x+1,t.y+1,t.w-2,t.h-2,' ',FC_BG);
	if((t.w<20)||(t.h<8))return;
	FcClamp(fcBase);
	FcClamp(fcBase+1);
	int x=0;
	int y=0;
	int w=0;
	int h=0;
	for(int p=fcBase;p<fcBase+2;p++)
	{
		FcPanelBox(p,x,y,w,h);
		FcDrawPanel(p,x,y,w,h);
	}
	FcPanelBox(fcBase+1,x,y,w,h);
	ShowToggles();     // keys and clicks change the modes the menus show
	for(int r=0;r<h;r++)
		TvPut(x-1,y+r,G_VERT,FC_FRAME);
	TvPut(x-1,y+h-2,0x253C,FC_FRAME);
}

// ---------------------------------------------------------------- dialogs

char fcPrompt[300];
int fcAskIn=-1;

// Asks for a line of text, starting from init. Returns 1 if it was given.
int FcAsk(char* title, char* prompt, char* init, char* out)
{
	int w=64;
	if(w>tvW-2)w=tvW-2;
	int win=TvNewCentered(W_ASK,title,w,9,WF_DIALOG);
	if(win<0)return 0;
	StrCpy(&fcPrompt[0],prompt,300);
	fcAskIn=TvAdd(win,C_INPUT,3,3,w-6,1,"",0);
	StrCpy(&tvCtl[fcAskIn].text[0],init,200);
	tvCtl[fcAskIn].val=StrLen(&tvCtl[fcAskIn].text[0]);
	if(fcAskAtStart)tvCtl[fcAskIn].val=0;
	fcAskAtStart=0;
	tvCtl[fcAskIn].top=0;
	TvInputHistory(fcAskIn,fcAskHist);
	fcAskHist=0;
	int bx=(w-26)/2;
	int b=TvAdd(win,C_BUTTON,bx,5,12,2,"O~K~",CM_OK);
	tvCtl[b].isDefault=1;
	TvAdd(win,C_BUTTON,bx+14,5,12,2,"Cancel",CM_CANCEL);
	tvWin[win].focus=fcAskIn;
	int res=TvExec(win);
	StrCpy(out,&tvCtl[fcAskIn].text[0],FC_PATH);
	TvClose(win);
	if(res!=CM_OK)return 0;
	if(out[0]==0)return 0;
	return 1;
}

void DrawAsk(int win)
{
	TWin& t=tvWin[win];
	TvTextN(t.x+3,t.y+2,&fcPrompt[0],A_DLG,t.w-6);
}

// A typed path: ~ means home, and a relative one is in the active panel's directory.
void FcResolve(char* typed, char* out)
{
	if((typed[0]=='~')&&((typed[1]=='/')||(typed[1]==0)))
	{
		char home[FC_PATH];
		fs_home(&home[0],FC_PATH);
		if(typed[1]==0)StrCpy(out,&home[0],FC_PATH);
		else FcJoin(out,&home[0],&typed[2]);
	}
	else if(typed[0]=='/')StrCpy(out,typed,FC_PATH);
	else FcJoin(out,FcDirOf(fcActive),typed);
}

// ---------------------------------------------------------------- copying, moving, deleting

int fcJobWin=-1;
int fcJobOp=0;
int fcJobAsking=0;
int fcJobTotal=-1;
int fcJobFile=-1;
int fcJobDone=0;           // 1 finished, 2 aborted

void FcTickRate()
{
	int ms=1000;      // watch the directories for changes
	if(PvBusy())ms=40;  // pictures being decoded, or animations
	if(FcQScanBusy())ms=100;   // a directory being counted
	if(fcJobWin>=0)ms=15;
	if(fcArcWin>=0)ms=100;      // an archive being read
	if(fcQArcWait)if(ms>200)ms=200;
	if(ms!=tvTickMs)
	{
		tvTickMs=ms;
		tvNextTick=term_ms()+ms;
	}
}

void DrawJob(int win)
{
	TWin& t=tvWin[win];
	char name[FC_PATH];
	char line[80];
	char* verb="Copying";
	if(fcJobOp==1)verb="Moving";
	if(fcJobOp==2)verb="Deleting";
	TvText(t.x+3,t.y+2,verb,A_DLG);
	JobName(0,&name[0],FC_PATH);
	int w=t.w-6;
	// the end of a long path is the part worth showing
	int cols=TvCols(&name[0]);
	int i=0;
	if(cols>w)
		for(int k=0;k<cols-w;k++)
			TvUtf8(&name[0],i);
	TvTextN(t.x+3,t.y+3,&name[i],A_DLG,w);
	if(fcJobOp!=2)
	{
		TvText(t.x+3,t.y+4,"to",A_DLG);
		JobName(1,&name[0],FC_PATH);
		cols=TvCols(&name[0]);
		i=0;
		if(cols>w)
			for(int k=0;k<cols-w;k++)
				TvUtf8(&name[0],i);
		TvTextN(t.x+3,t.y+5,&name[i],A_DLG,w);
	}
	snprintf(&line[0],80,"%i of %i",JobInfo(1),JobInfo(2));
	TvText(t.x+3,t.y+8,&line[0],A_DLG);
	tvCtl[fcJobFile].val=JobInfo(3);
	tvCtl[fcJobTotal].val=JobInfo(0);
}

// Asks what to do about a failed action or an existing target, for fs_job_answer.
void FcJobQuestion(int r)
{
	char msg[600];
	char name[FC_PATH];
	char err[300];
	int k=-1;
	if(r==-2)
	{
		JobName(1,&name[0],FC_PATH);
		snprintf(&msg[0],600,"\"%s\"\nalready exists. Overwrite it?",&name[EdBase(&name[0])]);
		k=TvChoose("Target exists",&msg[0],"~Y~es|~N~o|~A~ll|No~n~e|A~b~ort");
		if(k<0)k=4;
	}
	else
	{
		FcError(&err[0],300);
		JobName(0,&name[0],FC_PATH);
		snprintf(&msg[0],600,"%s\n\"%s\"",&err[0],&name[EdBase(&name[0])]);
		int c=TvChoose("Error",&msg[0],"~S~kip|Skip ~a~ll|~R~etry|A~b~ort");
		k=4;
		if(c==0)k=1;
		if(c==1)k=2;
		if(c==2)k=0;
	}
	JobAnswer(k);
	if(k==4)fcJobDone=2;
}

void FcJobTick()
{
	if(fcJobWin<0)return;
	if(fcJobAsking)return;     // a question's dialog runs the loop meanwhile
	if(fcJobDone)return;
	int r=JobStep(40);
	tvDirty=1;
	if(r<0)
	{
		fcJobAsking=1;
		FcJobQuestion(r);
		fcJobAsking=0;
	}
	else if(r==0)fcJobDone=1;
	if(fcJobDone)tvWin[fcJobWin].result=CM_OK;
}

// Reading a big archive's list of files can take long (a compressed tar is read
// through to the end): arc_dl reads it on a thread of its own, and meanwhile a dialog
// shows how far it got, with Cancel. Returns 1 when the index is there (or the path
// is not in an archive, or it cannot be read: the caller then hears why), 0 when
// cancelled.
int FcArcReady(char* path)
{
	if(fcArcWin>=0)return 1;       // already waiting: whatever asks now reads at once
	if(FcArcWhere(path)==0)return 1;
	if(arc_prepare(path)!=0)return 1;
	// most archives are read in a moment: no dialog for those
	int until=term_ms()+300;
	while(term_ms()<until)
	{
		Wait(0.02);
		if(arc_prepare(path)!=0)return 1;
	}
	int w=60;
	if(w>tvW-2)w=tvW-2;
	int win=TvNewCentered(W_ARCWAIT,"Archive",w,10,WF_DIALOG);
	if(win<0)return 1;
	StrCpy(&fcArcWaitPath[0],path,FC_PATH);
	fcArcBar=TvAdd(win,C_PROGRESS,3,5,w-6,1,"",0);
	tvCtl[fcArcBar].count=1000;
	TvAdd(win,C_BUTTON,(w-12)/2,7,12,2,"Cancel",CM_CANCEL);
	fcArcWin=win;
	FcTickRate();
	int res=TvExec(win);
	TvClose(win);
	fcArcWin=-1;
	FcTickRate();
	if(res==CM_OK)return 1;
	arc_cancel(path);
	fcArcCancelled=1;
	return 0;
}

void DrawArcWait(int win)
{
	TWin& t=tvWin[win];
	char file[FC_PATH];
	char inner[FC_PATH];
	char line[120];
	arc_where(&fcArcWaitPath[0],&file[0],FC_PATH,&inner[0],FC_PATH);
	TvText(t.x+3,t.y+2,"Reading the list of files of",A_DLG);
	int w=t.w-6;
	int cols=TvCols(&file[0]);
	int i=0;
	if(cols>w)
		for(int k=0;k<cols-w;k++)
			TvUtf8(&file[0],i);
	TvTextN(t.x+3,t.y+3,&file[i],A_DLG,w);
	int k=arc_progress(&line[0],120);
	TvText(t.x+3,t.y+4,&line[0],A_DLG);
	tvCtl[fcArcBar].used=k>=0;
	if(k>=0)tvCtl[fcArcBar].val=k;
}

// The tick while the dialog waits: done when the index is there, or failed.
void FcArcWaitTick()
{
	if(arc_prepare(&fcArcWaitPath[0])!=0)tvWin[fcArcWin].result=CM_OK;
	tvDirty=1;
}

// Runs the job fs_job_start planned, in a dialog with progress bars and Cancel.
// Returns 1 if it ran to the end.
int FcRunJob(int op, char* title)
{
	int w=60;
	if(w>tvW-2)w=tvW-2;
	int win=TvNewCentered(W_JOB,title,w,14,WF_DIALOG);
	if(win<0)return 0;
	fcJobOp=op;
	fcJobDone=0;
	fcJobAsking=0;
	fcJobFile=TvAdd(win,C_PROGRESS,3,6,w-6,1,"",0);
	tvCtl[fcJobFile].count=1000;
	fcJobTotal=TvAdd(win,C_PROGRESS,3,7,w-6,1,"",0);
	tvCtl[fcJobTotal].count=1000;
	if(op==2)
	{
		// one bar is enough when nothing is copied
		tvCtl[fcJobFile].used=0;
		tvCtl[fcJobTotal].y=6;
	}
	TvAdd(win,C_BUTTON,(w-12)/2,10,12,2,"Cancel",CM_CANCEL);
	fcJobWin=win;
	FcTickRate();
	int res=TvExec(win);
	if(res!=CM_OK)
	{
		JobAnswer(4);
		fcJobDone=2;
	}
	fcJobWin=-1;
	FcTickRate();
	TvClose(win);
	FcReloadAll();
	return fcJobDone==1;
}

// The marked entries of the active panel, or the one under the cursor. Adds them to
// a new job and describes them in what. Returns how many there are.
int FcSources(char* what)
{
	int p=fcActive;
	FcUse(p);
	JobClear();
	fcJobArc=fcInArc[p];
	int n=0;
	char path[FC_PATH];
	char name[256];
	for(int i=0;i<fcCount[p];i++)
		if(LsMarked(i))
		{
			FcPathOf(p,i,&path[0]);
			FcUse(p);
			JobAdd(&path[0]);
			n++;
		}
	if(n>0)
	{
		snprintf(what,300,"%i entries",n);
		if(n==1)StrCpy(what,"1 entry",300);
		return n;
	}
	if(fcCount[p]==0)return 0;
	FcName(p,fcCur[p],&name[0]);
	if(FcIsUp(&name[0]))return 0;
	FcPathOf(p,fcCur[p],&path[0]);
	JobAdd(&path[0]);
	snprintf(what,300,"\"%s\"",&name[0]);
	return 1;
}

// Ctrl+C in a panel: the full path of the entry under the cursor (of the archive,
// for one inside an archive; of the directory itself on ..), or the marked names,
// one a line, go to the clipboard, the editor's too.
char fcClipText[262144];

void FcCopyPath()
{
	int p=fcActive;
	FcUse(p);
	int n=0;
	char name[256];
	for(int i=0;i<fcCount[p];i++)
		if(LsMarked(i))
		{
			FcName(p,i,&name[0]);
			int len=StrLen(&name[0]);
			if(n+len+1>=262144)break;
			if(n>0)
			{
				fcClipText[n]=10;
				n++;
			}
			StrCpy(&fcClipText[n],&name[0],len+1);
			n=n+len;
		}
	if(n==0)
	{
		if(fcCount[p]==0)return;
		char path[FC_PATH];
		FcName(p,fcCur[p],&name[0]);
		if(FcIsUp(&name[0]))StrCpy(&path[0],FcDirOf(p),FC_PATH);
		else FcPathOf(p,fcCur[p],&path[0]);
		if(FcArcWhere(&path[0])==2)StrCpy(&path[0],&fcArcFile[0],FC_PATH);
		StrCpy(&fcClipText[0],&path[0],FC_PATH);
		n=StrLen(&fcClipText[0]);
	}
	fcClipText[n]=0;
	clip_set(&fcClipText[0],n);
	if(n<=ED_CLIPMAX)
	{
		for(int i=0;i<n;i++)
			edClip[i]=fcClipText[i];
		edClipN=n;
		edClipBlock=0;
	}
}

void FcUnmark(int p)
{
	FcUse(p);
	LsMarkMatch(0,"",1);
}

// F5 and F6: copy or move to the other panel's directory, or to a typed name.
// inPlace (Shift+F5, Shift+F6) offers the entry's own name, to copy it under another
// name or rename it in the same directory.
void FcCopyMove(int op, int inPlace)
{
	if(op==1)
		if(FcReadOnly("Rename or move"))return;
	char what[300];
	int n=FcSources(&what[0]);
	if(n==0)return;
	char prompt[340];
	char target[FC_PATH];
	char typed[FC_PATH];
	char* verb="Copy";
	if(op==1)verb="Move";
	snprintf(&prompt[0],340,"%s %s to:",verb,&what[0]);
	StrCpy(&target[0],FcDirOf(fcActive^1),FC_PATH);
	if(inPlace)
	{
		if(n>1)
		{
			if(op==1)TvMessage("Rename","Rename works on one entry; unmark the others first.",0);
			else TvMessage("Copy","Copying under a new name works on one entry; unmark the\nothers first, or use F5 to copy them all to a directory.",0);
			return;
		}
		FcName(fcActive,fcCur[fcActive],&target[0]);
		if(op==1)StrCpy(&prompt[0],"Rename to:",340);
		else snprintf(&prompt[0],340,"Copy %s to:",&what[0]);
	}
	char* title="Copy";
	if(op==1)title="Rename or move";
	fcAskHist=H_TARGET;
	if(!FcAsk(title,&prompt[0],&target[0],&typed[0]))return;
	FcResolve(&typed[0],&target[0]);
	if(inPlace)
	{
		// the same name in the same place is the entry itself
		char self[FC_PATH];
		FcPathOf(fcActive,fcCur[fcActive],&self[0]);
		if(StrCmp(&self[0],&target[0])==0)return;
	}
	if(FcArcWhere(&target[0])!=0)
	{
		TvMessage(title,"Archives are read-only: nothing can be copied into one.",0);
		return;
	}
	// FcSources cleared the job and added the sources; the dialog did not touch them
	int started=0;
	if(fcJobArc)started=arc_job_start(&target[0]);
	else started=fs_job_start(op,&target[0]);
	if(!started)
	{
		char err[300];
		FcError(&err[0],300);
		TvMessage(title,&err[0],0);
		return;
	}
	int ok=FcRunJob(op,title);
	if(ok)FcUnmark(fcActive);
	FcReloadBoth();
	// a rename keeps the cursor on the entry under its new name
	if(ok&&inPlace)
		FcLoad(fcActive,FcDirOf(fcActive),&target[EdBase(&target[0])]);
}

void FcDelete()
{
	if(FcReadOnly("Delete"))return;
	char what[300];
	int n=FcSources(&what[0]);
	if(n==0)return;
	char msg[400];
	char info[80];
	snprintf(&msg[0],400,"Delete %s?",&what[0]);
	FcUse(fcActive);
	if(LsMarkInfo(&info[0],80)==0)
	{
		char name[256];
		if(FcName(fcActive,fcCur[fcActive],&name[0]))
			snprintf(&msg[0],400,"Delete the directory \"%s\"\nand everything in it?",&name[0]);
	}
	if(TvMessage("Delete",&msg[0],1)!=CM_YES)return;
	fs_job_start(2,"");
	int ok=FcRunJob(2,"Delete");
	if(ok)FcUnmark(fcActive);
	FcReloadBoth();
}

void FcMkdir()
{
	char typed[FC_PATH];
	char path[FC_PATH];
	if(FcReadOnly("Make directory"))return;
	fcAskHist=H_NAME;
	if(!FcAsk("Make directory","Name of the new directory:","",&typed[0]))return;
	FcResolve(&typed[0],&path[0]);
	if(!fs_mkdir(&path[0]))
	{
		char err[300];
		fs_error(&err[0],300);
		TvMessage("Make directory",&err[0],0);
		return;
	}
	FcReloadBoth();
	FcLoad(fcActive,FcDirOf(fcActive),&path[EdBase(&path[0])]);
}

// ---------------------------------------------------------------- viewing and editing

void FcSetTitle(int win, char* prefix, char* path)
{
	snprintf(&tvWin[win].title[0],48,"%s%s",prefix,&path[EdBase(path)]);
}

// Keeps an editor window's title "View: name" or "Edit: name", whatever the editor
// set it to on opening or Save as; the help (a view with no file) keeps its own.
void FcEditorTitle(int win)
{
	TWin& t=tvWin[win];
	char want[48];
	if(ed[win].path[0]!=0)
	{
		if(fcView[win])FcSetTitle(win,"View: ",&ed[win].path[0]);
		else FcSetTitle(win,"Edit: ",&ed[win].path[0]);
		return;
	}
	if(fcView[win])return;
	if((t.title[0]=='E')&&(t.title[4]==':'))return;
	snprintf(&want[0],48,"Edit: %s",&t.title[0]);
	StrCpy(&t.title[0],&want[0],48);
}

// ---------------------------------------------------------------- picture windows

#define W_PIC 64
#define ST_VIEWER 900  // the status line and keyboard context of view windows
// picture window commands, CM_PIC0 to CM_PIC1
#define CM_PIC0 660
#define CM_PICIN 660
#define CM_PICOUT 661
#define CM_PICFIT 662
#define CM_PIC1X 663
#define CM_PIC2X 664
#define CM_PICNEXT 665
#define CM_PICPREV 666
#define CM_PICTURN 667
#define CM_PICPLAY 668        // videos: pause or play
#define CM_PICFWD 669         // 10 s on
#define CM_PIC1 669
#define CM_PICBACK 673        // 10 s back
#define CM_VIEWEDIT 670

int pvPanel[TV_MAXWIN];      // the panel a picture window came from
int pvDragX=0;
int pvDragY=0;
int pvClickMs=0;
int pvClickWin=-1;

// The picture window showing this file, or -1.
int PicFind(char* path)
{
	for(int w=0;w<TV_MAXWIN;w++)
		if(tvWin[w].used&&(tvWin[w].type==W_PIC))
			if(StrCmp(PvPathOf(w),path)==0)return w;
	return -1;
}

// Opens a picture window for a file of panel p.
int PicOpen(char* path, int p)
{
	int win=PicFind(path);
	if(win>=0)
	{
		TvToFront(win);
		return win;
	}
	int w=tvW-4;
	int h=tvH-3;
	if(w<20)w=tvW;
	win=TvNew(W_PIC,"Picture",(tvW-w)/2,1,w,h,WF_RESIZE);
	if(win<0)
	{
		TvMessage("View","Too many windows are open.",0);
		return -1;
	}
	pvPanel[win]=p;
	PvOpen(win,path);
	FcSetTitle(win,"Picture: ",path);
	return win;
}

void PicDraw(int win)
{
	TWin& t=tvWin[win];
	PvDraw(win,t.x+1,t.y+1,t.w-2,t.h-2);
	if(img_state(win)!=2)return;
	char info[120];
	char s[160];
	PvInfo(win,&info[0],120);
	float scale=pvScale[win];
	if(scale==0.)scale=PvFit(win,(t.w-2)*TvCellPxW(),(t.h-2)*TvCellPxH());
	float px=100./scale;
	// a video comes scaled down: the size against its own
	if(img_info(win,4)&&(img_info(win,9)>0))px=px*img_info(win,0)/img_info(win,9);
	int pct=(int)(px+0.5);
	if(pvScale[win]==0.)snprintf(&s[0],160," %s  %i%% fit ",&info[0],pct);
	else snprintf(&s[0],160," %s  %i%% ",&info[0],pct);
	TvClip(t.x,t.y,t.x+t.w-2,t.y+t.h);
	TvText(t.x+2,t.y+t.h-1,&s[0],A_WIN_FRAME);
}

// Space and Backspace: the next or previous picture of the panel, whose cursor
// follows.
void PicStep(int win, int dir)
{
	int p=pvPanel[win];
	int c=p/2;
	if(fcWinOf[c]<0)return;
	int keep=fcC;
	FcSelect(c);
	char name[256];
	char path[FC_PATH];
	int k=fcCur[p];
	int found=-1;
	while(found<0)
	{
		k=k+dir;
		if((k<0)||(k>=fcCount[p]))break;
		if(!FcName(p,k,&name[0]))
		{
			FcJoin(&path[0],FcDirOf(p),&name[0]);
			if(FcIsPicture(&path[0]))found=k;
		}
	}
	if(found>=0)
	{
		fcCur[p]=found;
		FcClamp(p);
		PvOpen(win,&path[0]);
		FcSetTitle(win,"Picture: ",&path[0]);
	}
	FcSelect(keep);
}

int PicKey(int win, int key)
{
	TWin& t=tvWin[win];
	int pw=(t.w-2)*TvCellPxW();
	int ph=(t.h-2)*TvCellPxH();
	int step=pw/8;
	if(step<2)step=2;
	// the other keys are commands of the keyboard map (PicCommand)
	if(img_info(win,4)&&(pvScale[win]==0.))
	{
		// a video that fits the window: the arrows go back and on in it
		if(key==K_LEFT)img_video_ctl(win,3,-5000);
		else if(key==K_RIGHT)img_video_ctl(win,3,5000);
		else if(key==K_DOWN)img_video_ctl(win,3,-60000);
		else if(key==K_UP)img_video_ctl(win,3,60000);
		else return 0;
		tvDirty=1;
		return 1;
	}
	if(key==K_LEFT)PvPan(win,-step,0,pw,ph);
	else if(key==K_RIGHT)PvPan(win,step,0,pw,ph);
	else if(key==K_UP)PvPan(win,0,-step,pw,ph);
	else if(key==K_DOWN)PvPan(win,0,step,pw,ph);
	else return 0;
	return 1;
}

void PicCommand(int win, int cmd)
{
	TWin& t=tvWin[win];
	int pw=(t.w-2)*TvCellPxW();
	int ph=(t.h-2)*TvCellPxH();
	if(cmd==CM_PICIN)PvZoom(win,1,pw/2,ph/2,pw,ph);
	else if(cmd==CM_PICOUT)PvZoom(win,-1,pw/2,ph/2,pw,ph);
	else if(cmd==CM_PICFIT)
	{
		pvScale[win]=0.;
		tvDirty=1;
	}
	else if(cmd==CM_PIC1X)PvSetScale(win,PvOwn(win),pw/2,ph/2,pw,ph);
	else if(cmd==CM_PIC2X)PvSetScale(win,PvOwn(win)/2.,pw/2,ph/2,pw,ph);
	else if(cmd==CM_PICPLAY)img_video_ctl(win,1,0);
	else if(cmd==CM_PICFWD)img_video_ctl(win,3,10000);
	else if(cmd==CM_PICBACK)img_video_ctl(win,3,-10000);
	else if((cmd==CM_PICTURN)&&!img_info(win,4))
	{
		img_rotate(win);
		pvGen[win]++;
		pvScale[win]=0.;
		tvDirty=1;
	}
	else if(cmd==CM_PICNEXT)PicStep(win,1);
	else if(cmd==CM_PICPREV)PicStep(win,-1);
}

int PicMouse(int win, int ev, int x, int y)
{
	TWin& t=tvWin[win];
	int cpw=TvCellPxW();
	int cph=TvCellPxH();
	int pw=(t.w-2)*cpw;
	int ph=(t.h-2)*cph;
	int px=(x-1)*cpw+cpw/2;
	int py=(y-1)*cph+cph/2;
	if(ev==EV_WHEEL)
	{
		if(evWheel>0)PvZoom(win,1,px,py,pw,ph);
		else PvZoom(win,-1,px,py,pw,ph);
		return 1;
	}
	if(ev==EV_MDOWN)
	{
		if((px<0)||(px>=pw)||(py<0)||(py>=ph))return 0;
		// a double click shows the pixels one to one there, or the whole picture again
		int now=term_ms();
		if((pvClickWin==win)&&(now-pvClickMs<400))
		{
			pvClickWin=-1;
			if(pvScale[win]==0.)PvSetScale(win,PvOwn(win),px,py,pw,ph);
			else
			{
				pvScale[win]=0.;
				tvDirty=1;
			}
			return 1;
		}
		pvClickWin=win;
		pvClickMs=now;
		pvDragX=x;
		pvDragY=y;
		TvCaptureMouse(win);
		return 1;
	}
	if(ev==EV_MDRAG)
	{
		PvPan(win,(pvDragX-x)*cpw,(pvDragY-y)*cph,pw,ph);
		pvDragX=x;
		pvDragY=y;
		return 1;
	}
	return 0;
}

// F3: directories are entered, binary files go to the hex viewer, and text opens in a
// read-only editor window.
int ArcOpen(char* path);

void FcViewPath(char* path)
{
	if(FcIsDir(path))return;
	// an archive opens in an archive window, its tree
	if(FcArcWhere(path)==1)
	{
		ArcOpen(path);
		return;
	}
	char head[4096];
	int n=FcLoadFile(path,&head[0],4096);
	if(n<0)
	{
		char err[300];
		if(FcArcWhere(path)==2)arc_error(&err[0],300);
		else fs_error(&err[0],300);
		TvMessage("View",&err[0],0);
		return;
	}
	if(FcIsPicture(path))
	{
		PicOpen(path,fcActive);
		return;
	}
	// a file in an archive is viewed from a temporary copy
	char local[FC_PATH];
	if(!FcLocal(path,&local[0]))
	{
		char err[300];
		arc_error(&err[0],300);
		TvMessage("View",&err[0],0);
		return;
	}
	if(arc_is(&local[0]))
	{
		ArcOpen(&local[0]);
		return;
	}
	path=&local[0];
	int binary=0;
	for(int i=0;i<n;i++)
		if(head[i]==0)binary=1;
	if(binary)
	{
		HxOpen(path);
		return;
	}
	char full[ED_PATH];
	int was=-1;
	if(EdAbsPath(path,&full[0])==1)was=EdFindPath(&full[0]);
	int win=OpenFile(path);
	if(win<0)return;
	if(win==was)return;     // already open, perhaps for editing
	fcLocked[win]=ed[win].ro;
	ed[win].ro=1;
	fcView[win]=1;
	FcSetTitle(win,"View: ",path);
}

// F4, or F4 in a view window: an editor window for the file.
void FcEditPath(char* path)
{
	if(FcIsDir(path))return;
	if((FcArcWhere(path)==2)||FcIsArcCopy(path))
	{
		TvMessage("Edit","This file is in an archive, which t2fc opens read-only.",0);
		return;
	}
	char full[ED_PATH];
	int was=-1;
	if(EdAbsPath(path,&full[0])==1)was=EdFindPath(&full[0]);
	int win=OpenFile(path);
	if(win<0)return;
	if(win!=was)
	{
		fcView[win]=0;
		fcLocked[win]=ed[win].ro;
		return;
	}
	if(fcView[win])
	{
		fcView[win]=0;
		ed[win].ro=fcLocked[win];
		FcSetTitle(win,"",&ed[win].path[0]);
	}
}

void FcView()
{
	int p=fcActive;
	if(fcCount[p]==0)return;
	char path[FC_PATH];
	FcPathOf(p,fcCur[p],&path[0]);
	FcViewPath(&path[0]);
}

void FcEdit()
{
	int p=fcActive;
	if(fcCount[p]==0)return;
	char path[FC_PATH];
	FcPathOf(p,fcCur[p],&path[0]);
	if(FcIsDir(&path[0]))
	{
		TvMessage("Edit","That is a directory.",0);
		return;
	}
	FcEditPath(&path[0]);
}

void FcNewFile()
{
	char typed[FC_PATH];
	char path[FC_PATH];
	if(FcReadOnly("New file"))return;
	fcAskHist=H_NAME;
	if(!FcAsk("New file","Name of the file to edit:","",&typed[0]))return;
	FcResolve(&typed[0],&path[0]);
	FcEditPath(&path[0]);
}

void FcHex()
{
	int p=fcActive;
	if(fcCount[p]==0)return;
	char path[FC_PATH];
	FcPathOf(p,fcCur[p],&path[0]);
	if(FcIsDir(&path[0]))return;
	char local[FC_PATH];
	if(FcLocal(&path[0],&local[0]))HxOpen(&local[0]);
}

// ---------------------------------------------------------------- archive windows

// F3 on an archive: its whole tree in a window, with sizes and times. Enter views a
// file or shows a directory in the panel, F5 copies the entry out to the panel's
// directory. arc_dl keeps the trees, six for windows (0-5) and one for quick view (7).
#define W_ARC 65
#define CM_ARCOPEN 671
#define CM_ARCCOPY 672
#define ARC_WINS 6

int arTree[TV_MAXWIN];
int arCur[TV_MAXWIN];
int arTop[TV_MAXWIN];
int arN[TV_MAXWIN];
char arPath[16384];          // TV_MAXWIN paths of FC_PATH
int arClickWin=-1;
int arClickMs=0;
int arClickRow=-1;

int ArcFind(char* path)
{
	for(int i=0;i<TV_MAXWIN;i++)
		if(tvWin[i].used&&(tvWin[i].type==W_ARC))
			if(StrCmp(&arPath[i*FC_PATH],path)==0)return i;
	return -1;
}

int ArcOpen(char* path)
{
	int win=ArcFind(path);
	if(win>=0)
	{
		TvToFront(win);
		return win;
	}
	int id=-1;
	for(int k=ARC_WINS-1;k>=0;k--)
	{
		int used=0;
		for(int i=0;i<TV_MAXWIN;i++)
			if(tvWin[i].used&&(tvWin[i].type==W_ARC)&&(arTree[i]==k))used=1;
		if(!used)id=k;
	}
	if(id<0)
	{
		TvMessage("Archive","Six archive windows are open already.",0);
		return -1;
	}
	fcArcCancelled=0;
	if(!FcArcReady(path))return -1;
	int n=arc_tree(id,path);
	if(n<0)
	{
		char err[300];
		arc_error(&err[0],300);
		TvMessage("Archive",&err[0],0);
		return -1;
	}
	int w=tvW-8;
	if(w>100)w=100;
	if(w<30)w=tvW;
	int h=tvH-4;
	win=TvNew(W_ARC,"Archive",(tvW-w)/2,2,w,h,WF_RESIZE);
	if(win<0)
	{
		TvMessage("Archive","Too many windows are open.",0);
		return -1;
	}
	arTree[win]=id;
	arN[win]=n;
	arCur[win]=0;
	arTop[win]=0;
	StrCpy(&arPath[win*FC_PATH],path,FC_PATH);
	FcSetTitle(win,"Archive: ",path);
	return win;
}

int ArcRows(int win)
{
	return tvWin[win].h-4;
}

void ArcClamp(int win)
{
	int n=arN[win];
	int rows=ArcRows(win);
	if(arCur[win]>=n)arCur[win]=n-1;
	if(arCur[win]<0)arCur[win]=0;
	if(arCur[win]<arTop[win])arTop[win]=arCur[win];
	if(arCur[win]>=arTop[win]+rows)arTop[win]=arCur[win]-rows+1;
	if(arTop[win]>n-rows)arTop[win]=n-rows;
	if(arTop[win]<0)arTop[win]=0;
}

// Rows of tree id from top, in a box: the tree and names, sizes and times.
void ArcRowsDraw(int id, int top, int cur, int n, int x, int y, int w, int h, int withTime)
{
	char name[600];
	char size[32];
	char when[32];
	int tw=0;
	if(withTime)tw=13;
	int nw=w-9-tw;
	for(int r=0;r<h;r++)
	{
		int i=top+r;
		if(i>=n)return;
		int isDir=arc_row(id,i,0,&name[0],600);
		if(isDir<0)return;
		arc_row(id,i,1,&size[0],32);
		int at=FC_FILE;
		if(isDir)at=FC_DIR;
		else if(FcIsArchive(&name[0]))at=FC_ARCH;
		if(i==cur)
		{
			at=FC_CUR;
			TvFill(x,y+r,w,1,' ',at);
		}
		FcCut(x+1,y+r,&name[0],at,nw);
		TvText(x+1+nw+7-StrLen(&size[0]),y+r,&size[0],at);
		if(withTime)
		{
			arc_row(id,i,2,&when[0],32);
			TvText(x+w-13,y+r,&when[0],at);
		}
	}
}

void ArcDraw(int win)
{
	TWin& t=tvWin[win];
	ArcClamp(win);
	TvFill(t.x+1,t.y+1,t.w-2,t.h-2,' ',FC_BG);
	char sum[200];
	arc_summary(arTree[win],&sum[0],200);
	FcCut(t.x+2,t.y+1,&sum[0],FC_HEAD,t.w-4);
	for(int k=1;k<t.w-1;k++)
		TvPut(t.x+k,t.y+2,G_HORZ,FC_FRAME);
	ArcRowsDraw(arTree[win],arTop[win],arCur[win],arN[win],t.x+1,t.y+3,t.w-2,ArcRows(win),t.w>50);
}

// The path of the row under the cursor, and 1 if it is a directory, or -1 if none.
int ArcCurPath(int win, char* out)
{
	out[0]=0;
	if(arN[win]==0)return -1;
	return arc_row(arTree[win],arCur[win],3,out,FC_PATH);
}

void ArcCommand(int win, int cmd)
{
	char path[FC_PATH];
	int isDir=ArcCurPath(win,&path[0]);
	if(isDir<0)return;
	int c=FcTopC();
	if(c<0)c=fcC;
	if(fcWinOf[c]<0)
	{
		TvMessage("Archive","There is no panel window open.",0);
		return;
	}
	FcSelect(c);
	if(cmd==CM_ARCOPEN)
	{
		if(isDir)
		{
			// the active panel goes to the directory in the archive
			FcChangeDir(fcActive,&path[0]);
			TvToFront(fcWin);
		}
		else FcViewPath(&path[0]);
		return;
	}
	if(cmd==CM_ARCCOPY)
	{
		int p=fcActive;
		if(fcInArc[p])
		{
			TvMessage("Copy","The active panel is inside an archive, which is read-only.",0);
			return;
		}
		char prompt[340];
		char typed[FC_PATH];
		char target[FC_PATH];
		snprintf(&prompt[0],340,"Copy \"%s\" to:",&path[EdBase(&path[0])]);
		fcAskHist=H_TARGET;
		if(!FcAsk("Copy",&prompt[0],FcDirOf(p),&typed[0]))return;
		FcResolve(&typed[0],&target[0]);
		JobClear();
		fcJobArc=1;
		arc_job_add(&path[0]);
		if(!arc_job_start(&target[0]))
		{
			char err[300];
			arc_error(&err[0],300);
			TvMessage("Copy",&err[0],0);
			return;
		}
		FcRunJob(0,"Copy");
		FcReloadBoth();
	}
}

int ArcKey(int win, int key)
{
	int rows=ArcRows(win);
	if(key==K_UP)arCur[win]--;
	else if(key==K_DOWN)arCur[win]++;
	else if(key==K_PGUP)arCur[win]=arCur[win]-rows+1;
	else if(key==K_PGDN)arCur[win]=arCur[win]+rows-1;
	else if(key==K_HOME)arCur[win]=0;
	else if(key==K_END)arCur[win]=arN[win]-1;
	else return 0;
	ArcClamp(win);
	return 1;
}

int ArcMouse(int win, int ev, int x, int y)
{
	if(ev==EV_WHEEL)
	{
		arTop[win]=arTop[win]-evWheel*3;
		int n=arN[win];
		int rows=ArcRows(win);
		if(arTop[win]>n-rows)arTop[win]=n-rows;
		if(arTop[win]<0)arTop[win]=0;
		if(arCur[win]<arTop[win])arCur[win]=arTop[win];
		if(arCur[win]>=arTop[win]+rows)arCur[win]=arTop[win]+rows-1;
		return 1;
	}
	if(ev!=EV_MDOWN)return 0;
	int r=y-3;
	if((r<0)||(r>=ArcRows(win)))return 0;
	int i=arTop[win]+r;
	if(i>=arN[win])return 1;
	arCur[win]=i;
	int now=term_ms();
	if((arClickWin==win)&&(arClickRow==i)&&(now-arClickMs<400))
	{
		arClickWin=-1;
		ArcCommand(win,CM_ARCOPEN);
		return 1;
	}
	arClickWin=win;
	arClickRow=i;
	arClickMs=now;
	return 1;
}

// The quick view of an archive: its tree, from tree 7.

void FcQArc(char* path, int x, int y, int w, int h, char* info)
{
	if(StrCmp(&fcQArcPath[0],path)!=0)
	{
		// never waits: a big archive is read in the background meanwhile, and the
		// tick draws again until it is there
		int r=arc_prepare(path);
		if(r==0)
		{
			if(!fcQArcWait)
			{
				fcQArcWait=1;
				StrCpy(&fcQArcWaitPath[0],path,FC_PATH);
				FcTickRate();
			}
			char line[120];
			int k=arc_progress(&line[0],120);
			TvText(x+1,y+1,"Reading the archive...",FC_FILE);
			FcCut(x+1,y+2,&line[0],FC_FILE,w-2);
			if(k>=0)
			{
				snprintf(&line[0],120,"%i%%",k/10);
				TvText(x+1,y+3,&line[0],FC_FILE);
			}
			return;
		}
		if(fcQArcWait)
		{
			fcQArcWait=0;
			FcTickRate();
		}
		fcQArcN=arc_tree(7,path);
		StrCpy(&fcQArcPath[0],path,FC_PATH);
	}
	if(fcQArcN<0)
	{
		char err[300];
		arc_error(&err[0],300);
		FcCut(x+1,y+1,&err[0],FC_FILE,w-2);
		return;
	}
	ArcRowsDraw(7,0,-1,fcQArcN,x,y,w,h,0);
	arc_summary(7,info,120);
}

// The editor's Save as: a name typed in a dialog, in the commander's directory if it
// is not absolute.
int RunSaveAs(int ew)
{
	char typed[FC_PATH];
	char path[FC_PATH];
	char full[FC_PATH];
	char* old=&ed[ew].path[0];
	char init[FC_PATH];
	if(old[0]!=0)StrCpy(&init[0],old,FC_PATH);
	else if(fcSaveName[ew*FC_PATH]!=0)StrCpy(&init[0],&fcSaveName[ew*FC_PATH],FC_PATH);
	else FcJoin(&init[0],FcDirOf(fcActive),"untitled.txt");
	fcAskHist=H_NAME;
	if(!FcAsk("Save as","Save the file as:",&init[0],&typed[0]))return 0;
	FcResolve(&typed[0],&path[0]);
	int where=EdAbsPath(&path[0],&full[0]);
	if(where==-2)
	{
		TvMessage("Save as","That path is too long.",0);
		return 0;
	}
	if(where<0)
	{
		TvMessage("Save as","There is no such directory.",0);
		return 0;
	}
	if(FcIsDir(&full[0]))
	{
		TvMessage("Save as","That is a directory.",0);
		return 0;
	}
	int other=EdFindPath(&full[0]);
	if((other>=0)&&(other!=ew))
	{
		TvMessage("Save as","That file is open in another window.",0);
		return 0;
	}
	if((where==1)&&(other!=ew))
		if(TvMessage("Save as","This file already exists.\nReplace it?",1)!=CM_YES)return 0;
	int saved=EdSaveAs(ew,&full[0]);
	if(saved)FcReloadAll();
	return saved;
}

// ---------------------------------------------------------------- running programs

// In a terminal the command gets the terminal itself: the desktop steps aside, the
// command runs with the screen and keyboard as in a shell, and a key brings the
// desktop back.
#ifndef TV_GFX
void FcRunIn(char* cmd, char* dir)
{
	TvSuspend();
	run_here(cmd,dir);
	TvResume();
	FcReloadAll();
}
#endif

// A graphic window has no terminal of its own, so the command runs in a new terminal
// window, which stays open until Enter is pressed so that its output can be read.
#ifdef TV_GFX
void FcRunIn(char* cmd, char* dir)
{
	if(!run_has_display())
	{
		TvMessage("Run","There is no display to open a terminal window on.",0);
		return;
	}
	char term[200];
	if(fcTermCmd[0]!=0)StrCpy(&term[0],&fcTermCmd[0],200);
	else if(!run_terminal(&term[0],200))
	{
		TvMessage("Run","No terminal program was found.\nName one in Options > Preferences.",0);
		return;
	}
	char qdir[2100];
	char script[6000];
	char quoted[8000];
	char full[8300];
	run_quote(dir,&qdir[0],2100);
	snprintf(&script[0],6000,"cd %s || exit; (%s\n); s=$?; echo; echo \"[exit code $s] Press Enter to close this window\"; read x",&qdir[0],cmd);
	run_quote(&script[0],&quoted[0],8000);
	snprintf(&full[0],8300,"%s sh -c %s",&term[0],&quoted[0]);
	if(!run_detached(&full[0],dir))
	{
		char err[300];
		fs_error(&err[0],300);
		TvMessage("Run",&err[0],0);
	}
	FcReloadAll();
}
#endif

// The entry under the cursor is an executable file (not a link to one).
int FcIsExec(int p)
{
	if(fcCount[p]==0)return 0;
	char name[256];
	char mark[4];
	if(FcName(p,fcCur[p],&name[0]))return 0;
	LsField(fcCur[p],3,&mark[0],4);
	if(mark[0]=='*')return 1;
	return 0;
}

// Enter on an executable file runs it in its directory.
void FcRunCursor()
{
	int p=fcActive;
	char name[256];
	char rel[262];
	char cmd[600];
	FcName(p,fcCur[p],&name[0]);
	snprintf(&rel[0],262,"./%s",&name[0]);
	run_quote(&rel[0],&cmd[0],600);
	FcRunIn(&cmd[0],FcDirOf(p));
}

int FcCapSlot()
{
	for(int c=0;c<FC_CAPS;c++)
		if(!fcCapUsed[c])return c;
	return -1;
}

// Opens a temporary output file as a new editor window called name, which is not
// on disk: closing asks to save it, and Save as suggests name in dir.
void FcOpenCapture(char* temp, char* name, char* dir)
{
	if(temp[0]==0)return;
	int win=OpenFile(temp);
	arc_tmpdrop(temp);
	if(win<0)return;
	fcView[win]=0;
	fcLocked[win]=ed[win].ro;
	ed[win].path[0]=0;
	ed[win].stamp=-1;
	ed[win].savedVer=-1;
	snprintf(&tvWin[win].title[0],48,"Edit: %s",name);
	FcJoin(&fcSaveName[win*FC_PATH],dir,name);
}

// Opens the output of the runs that have ended.
void FcCapCheck()
{
	for(int c=0;c<FC_CAPS;c++)
		if(fcCapUsed[c])
		{
			char* mark=&fcCapMark[c*FC_PATH];
			if(fs_size(mark)<0)continue;
			fcCapUsed[c]=0;
			arc_tmpdrop(mark);
			char* tOut=&fcCapOut[c*FC_PATH];
			char* tErr=&fcCapErr[c*FC_PATH];
			char* dir=&fcCapDir[c*FC_PATH];
			// stderr first, so that stdout ends up in front
			if(tErr[0]!=0)FcOpenCapture(tErr,&tErr[EdBase(tErr)],dir);
			if(tOut[0]!=0)FcOpenCapture(tOut,&tOut[EdBase(tOut)],dir);
			tvDirty=1;
		}
}

// The run dialog: FcAsk with two check boxes for capturing the output.
int fcRunOut=0;
int fcRunErr=0;

int FcRunAsk(char* prompt, char* init, char* out)
{
	int w=64;
	if(w>tvW-2)w=tvW-2;
	int win=TvNewCentered(W_ASK,"Run",w,11,WF_DIALOG);
	if(win<0)return 0;
	StrCpy(&fcPrompt[0],prompt,300);
	fcAskIn=TvAdd(win,C_INPUT,3,3,w-6,1,"",0);
	StrCpy(&tvCtl[fcAskIn].text[0],init,200);
	tvCtl[fcAskIn].val=StrLen(&tvCtl[fcAskIn].text[0]);
	if(fcAskAtStart)tvCtl[fcAskIn].val=0;
	fcAskAtStart=0;
	tvCtl[fcAskIn].top=0;
	TvInputHistory(fcAskIn,H_RUN);
	int co=TvAdd(win,C_CHECK,3,5,w-6,1,"Capture program std~o~ut to a new file",0);
	int ce=TvAdd(win,C_CHECK,3,6,w-6,1,"Capture program std~e~rr to a new file",0);
	tvCtl[co].val=0;
	tvCtl[ce].val=0;
	int bx=(w-26)/2;
	int b=TvAdd(win,C_BUTTON,bx,8,12,2,"O~K~",CM_OK);
	tvCtl[b].isDefault=1;
	TvAdd(win,C_BUTTON,bx+14,8,12,2,"Cancel",CM_CANCEL);
	tvWin[win].focus=fcAskIn;
	int res=TvExec(win);
	StrCpy(out,&tvCtl[fcAskIn].text[0],FC_PATH);
	fcRunOut=tvCtl[co].val;
	fcRunErr=tvCtl[ce].val;
	TvClose(win);
	if(res!=CM_OK)return 0;
	if(out[0]==0)return 0;
	return 1;
}

// Names for captured output: the program's name (the command's first word, without
// its directory) with .stdout and .stderr.
void FcCaptureNames(char* cmd, char* outName, char* errName)
{
	char prog[64];
	int i=0;
	while((cmd[i]==' ')||(cmd[i]==9))i++;
	int start=i;
	while((cmd[i]!=0)&&(cmd[i]!=' ')&&(cmd[i]!=9))
	{
		if(cmd[i]=='/')start=i+1;
		i++;
	}
	int o=0;
	for(int k=start;(k<i)&&(o<40);k++)
	{
		int c=cmd[k];
		int ok=0;
		if((c>='a')&&(c<='z'))ok=1;
		if((c>='A')&&(c<='Z'))ok=1;
		if((c>='0')&&(c<='9'))ok=1;
		if((c=='_')||(c=='-')||(c=='+'))ok=1;
		if((c=='.')&&(o>0))ok=1;
		if(ok)
		{
			prog[o]=c;
			o++;
		}
	}
	prog[o]=0;
	if(o==0)StrCpy(&prog[0],"run",64);
	snprintf(outName,100,"%s.stdout",&prog[0]);
	snprintf(errName,100,"%s.stderr",&prog[0]);
}

// F2 or Alt+Enter: a command typed in a dialog, run in the active panel's directory.
// It starts with the entry under the cursor, quoted for the shell, and the cursor
// before it, ready for the program's name; an executable is offered as ./name.
void FcRunDialog()
{
	int p=fcActive;
	if(fcInArc[p])
	{
		TvMessage("Run","Nothing runs inside an archive; F5 copies files out of it.",0);
		return;
	}
	char init[FC_PATH];
	char typed[FC_PATH];
	char prompt[300];
	init[0]=0;
	if(fcCount[p]>0)
	{
		char name[256];
		char arg[600];
		FcName(p,fcCur[p],&name[0]);
		if(!FcIsUp(&name[0]))
		{
			if(FcIsExec(p))
			{
				char rel[262];
				snprintf(&rel[0],262,"./%s",&name[0]);
				run_quote(&rel[0],&arg[0],600);
				snprintf(&init[0],FC_PATH,"%s ",&arg[0]);
			}
			else
			{
				run_quote(&name[0],&arg[0],600);
				snprintf(&init[0],FC_PATH," %s",&arg[0]);
				fcAskAtStart=1;
			}
		}
	}
	snprintf(&prompt[0],300,"Run in %s:",FcDirOf(p));
	if(!FcRunAsk(&prompt[0],&init[0],&typed[0]))return;
	if(!fcRunOut&&!fcRunErr)
	{
		FcRunIn(&typed[0],FcDirOf(p));
		return;
	}
	// the output goes to temporary files, which open as new, unsaved editor windows
	// when the program has ended: at once in a terminal, or when the marker file
	// appears for one run in a terminal window of its own
	int c=FcCapSlot();
	if(c<0)
	{
		TvMessage("Run","Too many programs are still running with their output captured.",0);
		return;
	}
	char outName[100];
	char errName[100];
	FcCaptureNames(&typed[0],&outName[0],&errName[0]);
	char* tOut=&fcCapOut[c*FC_PATH];
	char* tErr=&fcCapErr[c*FC_PATH];
	char* tMark=&fcCapMark[c*FC_PATH];
	tOut[0]=0;
	tErr[0]=0;
	int ok=arc_tmpname("done",tMark,FC_PATH);
	if(ok&&fcRunOut)ok=arc_tmpname(&outName[0],tOut,FC_PATH);
	if(ok&&fcRunErr)ok=arc_tmpname(&errName[0],tErr,FC_PATH);
	if(!ok)
	{
		char err[300];
		arc_error(&err[0],300);
		TvMessage("Run",&err[0],0);
		return;
	}
	StrCpy(&fcCapDir[c*FC_PATH],FcDirOf(p),FC_PATH);
	fcCapUsed[c]=1;
	char qo[2100];
	char qe[2100];
	char qm[2100];
	char redir[4300];
	char full[7400];
	run_quote(tOut,&qo[0],2100);
	run_quote(tErr,&qe[0],2100);
	run_quote(tMark,&qm[0],2100);
	if(fcRunOut&&fcRunErr)snprintf(&redir[0],4300,"exec >%s 2>%s",&qo[0],&qe[0]);
	else if(fcRunOut)snprintf(&redir[0],4300,"exec >%s",&qo[0]);
	else snprintf(&redir[0],4300,"exec 2>%s",&qe[0]);
	snprintf(&full[0],7400,"set -C; %s; %s\ns=$?; : >%s; exit $s",&redir[0],&typed[0],&qm[0]);
	FcRunIn(&full[0],FcDirOf(p));
	FcCapCheck();
}

// Opens the entry with the desktop's application for it (xdg-open), without waiting.
void FcOpenSystem()
{
	int p=fcActive;
	if(fcCount[p]==0)return;
	if(!run_has_display())
	{
		TvMessage("Open","There is no graphic desktop to open it on.",0);
		return;
	}
	char path[FC_PATH];
	char quoted[2100];
	char cmd[2200];
	char where[FC_PATH];
	FcPathOf(p,fcCur[p],&where[0]);
	// a file in an archive goes to the desktop as a temporary copy
	if(!FcLocal(&where[0],&path[0]))
	{
		char err[300];
		arc_error(&err[0],300);
		TvMessage("Open",&err[0],0);
		return;
	}
	run_quote(&path[0],&quoted[0],2100);
	snprintf(&cmd[0],2200,"xdg-open %s",&quoted[0]);
	char* dir=FcDirOf(p);
	if(fcInArc[p])dir="/";
	if(!run_detached(&cmd[0],dir))
	{
		char err[300];
		fs_error(&err[0],300);
		TvMessage("Open",&err[0],0);
	}
}

// ---------------------------------------------------------------- the panel's keys and mouse

void FcEnter()
{
	int p=fcActive;
	if(fcCount[p]==0)return;
	char name[256];
	int isDir=FcName(p,fcCur[p],&name[0]);
	if(!isDir)
	{
		char file[FC_PATH];
		FcPathOf(p,fcCur[p],&file[0]);
		// an archive is entered as a directory; nothing in one runs
		if(FcArcWhere(&file[0])==1)FcChangeDir(p,&file[0]);
		else if(FcIsExec(p)&&!fcInArc[p])FcRunCursor();
		else FcView();
		return;
	}
	if(FcIsUp(&name[0]))
	{
		FcGoUp(p);
		return;
	}
	char path[FC_PATH];
	FcJoin(&path[0],FcDirOf(p),&name[0]);
	FcChangeDir(p,&path[0]);
}

void FcSetSort(int p, int key)
{
	if((fcSort[p]&3)==key)fcSort[p]=fcSort[p]^4;
	else fcSort[p]=key;
	FcReload(p);
}

void FcMarkGroup(int v)
{
	char typed[FC_PATH];
	char* title="Select group";
	if(v==0)title="Unselect group";
	fcAskHist=H_PATTERN;
	if(!FcAsk(title,"Files whose names match (* and ? are wild):","*",&typed[0]))return;
	FcUse(fcActive);
	LsMarkMatch(v,&typed[0],v==0);
}

int FcQSMatch(char* name, char* pre, int n);

// Where a name's extension starts: after its last dot, or at its end if it has none.
int FcExtAt(char* name)
{
	int at=-1;
	int i=0;
	while(name[i]!=0)
	{
		if(name[i]=='.')at=i+1;
		i++;
	}
	if(at<0)return i;
	return at;
}

// Marks (v 1) or unmarks (v 0) the files whose extension is that of the file under
// the cursor; upper and lower case are told apart as for the quick search.
void FcMarkExt(int v)
{
	int p=fcActive;
	if(fcCount[p]==0)return;
	char name[256];
	if(FcName(p,fcCur[p],&name[0]))return;   // a directory has no extension here
	char ext[256];
	int e=FcExtAt(&name[0]);
	int noExt=1;
	if(e>0)
		if(name[e-1]=='.')noExt=0;
	StrCpy(&ext[0],&name[e],256);
	int n=StrLen(&ext[0]);
	for(int i=0;i<fcCount[p];i++)
	{
		if(FcName(p,i,&name[0]))continue;
		int a=FcExtAt(&name[0]);
		int dot=0;
		if(a>0)dot=(name[a-1]=='.');
		int same=0;
		if(noExt)same=!dot;
		else if(dot&&(StrLen(&name[a])==n))same=FcQSMatch(&name[a],&ext[0],n);
		if(same)
		{
			FcUse(p);
			LsMark(i,v);
		}
	}
	tvDirty=1;
}

void FcSwap()
{
	char a[FC_PATH];
	char b[FC_PATH];
	int l=fcBase;
	int r=fcBase+1;
	StrCpy(&a[0],FcDirOf(l),FC_PATH);
	StrCpy(&b[0],FcDirOf(r),FC_PATH);
	int s=fcSort[l];
	fcSort[l]=fcSort[r];
	fcSort[r]=s;
	int br=fcBrief[l];
	fcBrief[l]=fcBrief[r];
	fcBrief[r]=br;
	FcLoad(l,&b[0],"");
	FcLoad(r,&a[0],"");
}

int StrCmp(char* a, char* b)
{
	int i=0;
	while(1)
	{
		int x=a[i];
		int y=b[i];
		if(x!=y)return x-y;
		if(x==0)return 0;
		i++;
	}
	return 0;
}

// Marks, in both panels, the files missing from the other one or different there in
// size or time.
void FcCompare()
{
	char name[256];
	char s1[24];
	char s2[24];
	char t1[24];
	char t2[24];
	int differ=0;
	for(int p=fcBase;p<fcBase+2;p++)
	{
		int q=p^1;
		FcUnmark(p);
		for(int i=0;i<fcCount[p];i++)
		{
			FcUse(p);
			int isDir=LsName(i,&name[0],256);
			if(!isDir)
			{
				LsField(i,0,&s1[0],24);
				LsField(i,1,&t1[0],24);
				FcUse(q);
				int k=LsIndex(&name[0]);
				int mark=1;
				if(k>=0)
				{
					LsField(k,0,&s2[0],24);
					LsField(k,1,&t2[0],24);
					mark=0;
					if(StrCmp(&s1[0],&s2[0])!=0)mark=1;
					if(StrCmp(&t1[0],&t2[0])!=0)mark=1;
				}
				if(mark)
				{
					FcUse(p);
					LsMark(i,1);
					differ++;
				}
			}
		}
	}
	if(differ==0)TvMessage("Compare","The files of both directories are the same.",0);
}

// Right with lynx-like motion: into the directory or archive under the cursor.
void FcLynxIn(int p)
{
	if(fcCount[p]==0)return;
	char name[256];
	char path[FC_PATH];
	int isDir=FcName(p,fcCur[p],&name[0]);
	if(FcIsUp(&name[0]))return;
	FcPathOf(p,fcCur[p],&path[0]);
	if(isDir||(FcArcWhere(&path[0])==1))FcChangeDir(p,&path[0]);
}

// Quick search: letters typed in a panel go to a small box on its bottom line, and
// the cursor goes to the next name that starts with them. Esc, a cursor key, a click
// or any other command ends it, with the cursor where the search left it.

int FcQSMatch(char* name, char* pre, int n)
{
	for(int k=0;k<n;k++)
	{
		int a=name[k];
		int b=pre[k];
		if(a==0)return 0;
		if(!fcQSCase)
		{
			if((a>='A')&&(a<='Z'))a=a+32;
			if((b>='A')&&(b<='Z'))b=b+32;
		}
		if(a!=b)return 0;
	}
	return 1;
}

// The first entry from the cursor on (round to the start) that begins with the n
// bytes of pre, or -1.
int FcQSFind(int p, char* pre, int n)
{
	int c=fcCount[p];
	if(c==0)return -1;
	char name[256];
	for(int s=0;s<c;s++)
	{
		int k=(fcCur[p]+s)%c;
		FcName(p,k,&name[0]);
		if(FcIsUp(&name[0]))continue;
		if(FcQSMatch(&name[0],pre,n))return k;
	}
	return -1;
}

void FcQSEnd()
{
	fcQS=-1;
	fcQSN=0;
	fcQSText[0]=0;
	tvKeyGrab=-1;
	tvDirty=1;
}

// Adds n typed bytes to the search, if some name still starts with it.
void FcQSType(int p, char* s, int n)
{
	if(fcQS!=p)
	{
		fcQS=p;
		fcQSN=0;
		fcQSText[0]=0;
	}
	tvKeyGrab=fcWin;
	if(fcQSN+n>=190)return;
	char want[200];
	for(int k=0;k<fcQSN;k++)
		want[k]=fcQSText[k];
	for(int k=0;k<n;k++)
		want[fcQSN+k]=s[k];
	int k=FcQSFind(p,&want[0],fcQSN+n);
	if(k<0)return;
	for(int i=0;i<n;i++)
		fcQSText[fcQSN+i]=s[i];
	fcQSN=fcQSN+n;
	fcQSText[fcQSN]=0;
	fcCur[p]=k;
	FcClamp(p);
	tvDirty=1;
}

// Keys while searching. Returns 1 if the search used the key; any other key ends it.
int FcQSKey(int p, int key)
{
	if(evUtfN>0)
	{
		FcQSType(p,&evUtf[0],evUtfN);
		return 1;
	}
	if((key>=' ')&&(key<127))
	{
		char one[2];
		one[0]=key;
		one[1]=0;
		FcQSType(p,&one[0],1);
		return 1;
	}
	if(key==K_BACKSPACE)
	{
		// one character less, and the first name that starts with the rest
		if(fcQSN>0)
		{
			fcQSN--;
			while((fcQSN>0)&&((fcQSText[fcQSN]&192)==128))fcQSN--;
			fcQSText[fcQSN]=0;
			if(fcQSN>0)
			{
				int keep=fcCur[p];
				fcCur[p]=0;
				int k=FcQSFind(p,&fcQSText[0],fcQSN);
				if(k>=0)fcCur[p]=k;
				else fcCur[p]=keep;
				FcClamp(p);
			}
		}
		tvDirty=1;
		return 1;
	}
	FcQSEnd();
	if(key==K_ESC)return 1;
	return 0;
}

int FcKey(int win, int key)
{
	int p=fcActive;
	if(fcQS>=0)
	{
		if(fcQS!=p)FcQSEnd();
		else if(FcQSKey(p,key))return 1;
	}
	int vis=FcVisible(p);
	int rows=FcRows(p);
	// the other keys are commands of the keyboard map (FcKeys)
	if(fcLynx)
	{
		if(key==K_LEFT)
		{
			FcGoUp(p);
			return 1;
		}
		if(key==K_RIGHT)
		{
			FcLynxIn(p);
			return 1;
		}
	}
	if(key==K_UP)fcCur[p]--;
	else if(key==K_DOWN)fcCur[p]++;
	else if(key==K_PGUP)fcCur[p]=fcCur[p]-vis+1;
	else if(key==K_PGDN)fcCur[p]=fcCur[p]+vis-1;
	else if(key==K_HOME)fcCur[p]=0;
	else if(key==K_END)fcCur[p]=fcCount[p]-1;
	else if((key==K_LEFT)&&fcBrief[p])fcCur[p]=fcCur[p]-rows;
	else if((key==K_RIGHT)&&fcBrief[p])fcCur[p]=fcCur[p]+rows;
	else if(key==K_ENTER)FcEnter();
	else if(((key>' ')&&(key<127))||(evUtfN>0))
	{
		// letters start a quick search
		FcUse(p);
		if(fcCount[p]>0)FcQSKey(p,key);
		return 1;
	}
	else return 0;
	FcClamp(p);
	return 1;
}

// The entry at a point inside the window, in panel p, or -1. part is 1 on the header.
int FcHit(int x, int y, int& p, int& part)
{
	TWin& t=tvWin[fcWin];
	int ax=t.x+x;
	int ay=t.y+y;
	int px=0;
	int py=0;
	int pw=0;
	int ph=0;
	part=0;
	p=-1;
	for(int k=fcBase;k<fcBase+2;k++)
	{
		FcPanelBox(k,px,py,pw,ph);
		if((ax>=px)&&(ax<px+pw))p=k;
	}
	if(p<0)return -1;
	FcPanelBox(p,px,py,pw,ph);
	int row=ay-py-2;
	if(ay==py+1)part=1;
	int rows=ph-4;
	if((row<0)||(row>=rows))return -1;
	int k=fcTop[p]+row;
	if(fcBrief[p])
		if(ax>px+(pw-1)/2)k=k+rows;
	if(k>=fcCount[p])return -1;
	return k;
}

int FcMouse(int win, int ev, int x, int y)
{
	if((fcQS>=0)&&(ev!=EV_MUP)&&(ev!=EV_MDRAG))FcQSEnd();
	if((ev==EV_RDOWN)&&(x<0))
	{
		TvOpenPopup(popPanel,-1,-1);
		return 1;
	}
	int p=0;
	int part=0;
	int k=FcHit(x,y,p,part);
	if(p<0)return 1;
	if(fcQuick[p])return 1;       // the quick view is not a list to click in
	if(ev==EV_WHEEL)
	{
		fcCur[p]=fcCur[p]-evWheel*3;
		FcClamp(p);
		return 1;
	}
	if(ev==EV_RDOWN)
	{
		FcSetActive(p);
		if(k>=0)
		{
			FcUse(p);
			LsMark(k,-1);
			fcCur[p]=k;
		}
		return 1;
	}
	if(ev!=EV_MDOWN)return 1;
	FcSetActive(p);
	if(part==1)
	{
		// a click on a column header sorts by it, again reverses
		int px=0;
		int py=0;
		int pw=0;
		int ph=0;
		FcPanelBox(p,px,py,pw,ph);
		int sizeX=0;
		int timeX=0;
		FcColumns(p,px,pw,sizeX,timeX);
		int ax=tvWin[fcWin].x+x;
		int key=0;
		if((sizeX>0)&&(ax>sizeX))key=2;
		if((timeX>0)&&(ax>timeX))key=3;
		FcSetSort(p,key);
		return 1;
	}
	if(k<0)return 1;
	int now=term_ms();
	int twice=0;
	if((fcLastP==p)&&(fcLastI==k))
		if(now-fcLastClick<400)twice=1;
	fcLastClick=now;
	fcLastP=p;
	fcLastI=k;
	fcCur[p]=k;
	FcClamp(p);
	if(twice)
	{
		fcLastI=-1;
		FcEnter();
	}
	return 1;
}

// ---------------------------------------------------------------- settings

// t2fc.cfg: a line of numbers (theme, hidden, the panels' sort and brief modes, the
// active panel, line numbers, wrap) and then the two directories.
int prefScale=0;     // the pixel scale of a graphic window, 0 the program's own

void PrefsLoad(char* left, char* right)
{
	char buf[2200];
	int n=fs_load(CFG_FILE,&buf[0],2199);
	if(n<=0)return;
	buf[n]=0;
	int v[16];
	int k=0;
	int i=0;
	while((k<16)&&(buf[i]!=0)&&(buf[i]!=10))
	{
		if((buf[i]>='0')&&(buf[i]<='9'))
		{
			int x=0;
			while((buf[i]>='0')&&(buf[i]<='9'))
			{
				x=x*10+buf[i]-'0';
				i++;
			}
			v[k]=x;
			k++;
		}
		else i++;
	}
	if(k<9)return;
	prefTheme=v[0];
	fcHidden=(v[1]!=0);
	fcSort[0]=v[2]&7;
	fcSort[1]=v[3]&7;
	fcBrief[0]=(v[4]!=0);
	fcBrief[1]=(v[5]!=0);
	FcSetActive(v[6]!=0);
	edLineNums=(v[7]!=0);
	edWrap=(v[8]!=0);
	fcLynx=0;
	if(k>=10)fcLynx=(v[9]!=0);
	// the 11th number was once only on or off; the 12th is the seconds
	fcQSecs=5;
	if((k>=11)&&(v[10]==0))fcQSecs=0;
	if(k>=12)fcQSecs=v[11];
	fcQSCase=0;
	if(k>=13)fcQSCase=(v[12]!=0);
	teDeskPick=0;
	if((k>=14)&&(v[13]<TE_DESKN))teDeskPick=v[13];
	prefScale=0;
	if(k>=15)prefScale=v[14];
	pvLanczos=0;
	if(k>=16)pvLanczos=(v[15]!=0);
	img_filter(pvLanczos);
	if(fcQSecs>600)fcQSecs=600;
	// the directories, one a line, then the terminal command
	for(int d=0;d<3;d++)
	{
		while((buf[i]!=0)&&(buf[i]!=10))i++;
		if(buf[i]==0)return;
		i++;
		char* out=left;
		int room=FC_PATH-1;
		if(d==1)out=right;
		if(d==2)
		{
			out=&fcTermCmd[0];
			room=199;
		}
		int o=0;
		while((buf[i]!=0)&&(buf[i]!=10)&&(o<room))
		{
			out[o]=buf[i];
			o++;
			i++;
		}
		out[o]=0;
	}
}

void PrefsSave()
{
	char buf[2200];
	int scale=prefScale;
#ifdef TV_GFX
	scale=GfxScale();
#endif
	snprintf(&buf[0],2200,"%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i\n%s\n%s\n%s\n",prefTheme,fcHidden,fcSort[fcBase],fcSort[fcBase+1],
		fcBrief[fcBase],fcBrief[fcBase+1],fcActive&1,edLineNums,edWrap,fcLynx,fcQSecs>0,fcQSecs,fcQSCase,teDeskPick,scale,pvLanczos,FcDirOf(fcBase),FcDirOf(fcBase+1),&fcTermCmd[0]);
	if(fs_save_begin(CFG_FILE))
	{
		int ok=fs_save_write(&buf[0],StrLen(&buf[0]));
		fs_save_end(ok);
	}
}

int itemLineNums=-1;     // menu items whose hints show on or off
int itemWrap=-1;
int itemLynx=-1;
int itemHidden=-1;
int itemLanczos=-1;
int itemPix=-1;          // the first of the 1x..4x items of Options > Pixel size
int itemPanel[2];        // each panel's menu: Full and Brief, then the sort keys

void SetHint(int item, int on)
{
	if(item<0)return;
	if(on)StrCpy(&tvItem[item].hint[0],"on",12);
	else StrCpy(&tvItem[item].hint[0],"off",12);
}

// Puts a dot by the listing mode and the sort key of each panel's menu.
void ShowToggles()
{
	SetHint(itemLineNums,edLineNums);
	SetHint(itemWrap,edWrap);
	SetHint(itemLynx,fcLynx);
	SetHint(itemHidden,fcHidden);
	SetHint(itemLanczos,pvLanczos);
#ifdef TV_GFX
	if(itemPix>=0)
		for(int k=0;k<4;k++)
		{
			tvItem[itemPix+k].hint[0]=0;
			if(k+1==GfxScale())StrCpy(&tvItem[itemPix+k].hint[0],"\xe2\x80\xa2",12);
		}
#endif
	// the menus speak of the panel window in front
	int base=fcBase;
	int tc=FcTopC();
	if(tc>=0)base=tc*2;
	for(int side=0;side<2;side++)
	{
		int it=itemPanel[side];
		int p=base+side;
		for(int k=0;k<7;k++)
			tvItem[it+k].hint[0]=0;
		if(fcQuick[p])StrCpy(&tvItem[it+2].hint[0],"\xe2\x80\xa2",12);
		else if(fcBrief[p])StrCpy(&tvItem[it+1].hint[0],"\xe2\x80\xa2",12);
		else StrCpy(&tvItem[it].hint[0],"\xe2\x80\xa2",12);
		StrCpy(&tvItem[it+3+(fcSort[p]&3)].hint[0],"\xe2\x80\xa2",12);
		if(fcSort[p]&4)StrCpy(&tvItem[it+7].hint[0],"on",12);
		else StrCpy(&tvItem[it+7].hint[0],"off",12);
	}
}

int prefsThemeCtl=-1;     // the theme combo box of the open Preferences

void RunPrefs()
{
	int win=TvNewCentered(W_PREFS,"Preferences",50,20,WF_DIALOG);
	if(win<0)return;
	TvAdd(win,C_LABEL,3,2,10,1,"~T~heme",0);
	char names[200];
	TeThemeNames(&names[0],200);
	int th=TvAdd(win,C_COMBO,14,2,30,1,&names[0],CM_THEMEPICK);
	tvCtl[th].val=prefTheme;
	prefsThemeCtl=th;
	TvAdd(win,C_LABEL,3,3,10,1,"~D~esktop",0);
	int dk=TvAdd(win,C_COMBO,14,3,30,1,teDeskNames,CM_DESKPICK);
	tvCtl[dk].val=teDeskPick;
	int oldDesk=teDeskPick;
	int hid=TvAdd(win,C_CHECK,2,5,30,1,"Show ~h~idden files",0);
	tvCtl[hid].val=fcHidden;
	int ln=TvAdd(win,C_CHECK,2,6,30,1,"Editor ~l~ine numbers",0);
	tvCtl[ln].val=edLineNums;
	int wr=TvAdd(win,C_CHECK,2,7,30,1,"Editor ~w~ord wrap",0);
	tvCtl[wr].val=edWrap;
	int ly=TvAdd(win,C_CHECK,2,8,46,1,"L~y~nx-like motion: Left goes up, Right in",0);
	tvCtl[ly].val=fcLynx;
	int qc=TvAdd(win,C_CHECK,2,9,46,1,"Case-~s~ensitive file search",0);
	tvCtl[qc].val=fcQSCase;
	TvAdd(win,C_LABEL,3,10,38,1,"~Q~uick view folder count, s (0 off)",0);
	int qd=TvAdd(win,C_INPUT,41,10,6,1,"",0);
	snprintf(&tvCtl[qd].text[0],16,"%d",fcQSecs);
	tvCtl[qd].val=StrLen(&tvCtl[qd].text[0]);
	TvAdd(win,C_LABEL,3,12,40,1,"Te~r~minal for F2 in a graphic window",0);
	int term=TvAdd(win,C_INPUT,3,13,44,1,"",0);
	TvInputHistory(term,H_TERM);
	StrCpy(&tvCtl[term].text[0],&fcTermCmd[0],200);
	tvCtl[term].val=StrLen(&fcTermCmd[0]);
	int ok=TvAdd(win,C_BUTTON,11,16,12,2,"O~K~",CM_OK);
	tvCtl[ok].isDefault=1;
	TvAdd(win,C_BUTTON,26,16,12,2,"Cancel",CM_CANCEL);
	tvWin[win].focus=th;
	int res=TvExec(win);
	if(res==CM_OK)
	{
		prefTheme=tvCtl[th].val;
		teDeskPick=tvCtl[dk].val;
		int h=tvCtl[hid].val;
		edLineNums=tvCtl[ln].val;
		edWrap=tvCtl[wr].val;
		fcLynx=tvCtl[ly].val;
		fcQSCase=tvCtl[qc].val;
		// the seconds: digits only, up to ten minutes
		int secs=0;
		int bad=0;
		char* qs=&tvCtl[qd].text[0];
		for(int k=0;qs[k]!=0;k++)
		{
			if((qs[k]>='0')&&(qs[k]<='9'))
			{
				if(secs<1000)secs=secs*10+qs[k]-'0';
			}
			else if(qs[k]!=' ')bad=1;
		}
		if(secs>600)secs=600;
		if(bad)secs=fcQSecs;
		if(secs!=fcQSecs)
		{
			fcQSecs=secs;
			// quick views look again
			for(int q=0;q<FC_MAXP;q++)
				fcQPath[q*FC_PATH]=0;
		}
		StrCpy(&fcTermCmd[0],&tvCtl[term].text[0],200);
		if(h!=fcHidden)
		{
			fcHidden=h;
			FcReloadBoth();
		}
		PrefsSave();
	}
	else teDeskPick=oldDesk;
	prefsThemeCtl=-1;
	TvClose(win);
	// the combo boxes showed each theme and pattern as it was picked
	TeUseTheme(prefTheme);
	ShowToggles();
}

void DrawPrefs(int win)
{
	TWin& t=tvWin[win];
	TvText(t.x+3,t.y+14,"empty: the first one found, as \"xterm -e\"",A_DLG);
}

// ---------------------------------------------------------------- help

void OpenHelp()
{
	int win=NewViewer("Help");
	if(win<0)return;
	TWin& t=tvWin[win];
	ed[win].ro=1;
	fcView[win]=1;
	t.w=66;
	if(t.w>tvW)t.w=tvW;
	t.h=tvH-4;
	t.x=(tvW-t.w)/2;
	t.y=2;
	char* text=
		"t2fc, a two-panel file manager written in Scinc\n"
		"\n"
		"Panels\n"
		"  Tab                  the other panel\n"
		"  Enter, double click  enter a directory, view a file,\n"
		"                       run an executable file\n"
		"  Backspace            the parent directory\n"
		"  Insert, right click  mark an entry; + - * mark by a pattern,\n"
		"                       unmark by one, invert the marks\n"
		"  Shift+keypad + -,    mark, unmark the files with the\n"
		"  Alt++ Alt+-          extension of the one under the cursor\n"
		"  typing               quick search: the cursor goes to the\n"
		"                       next name starting with what is typed\n"
		"                       (Backspace one back); Esc, a cursor\n"
		"                       key or a click ends it\n"
		"  click a column title sort by it; again reverses\n"
		"  Ctrl+R               read both directories again\n"
		"  Ctrl+U               swap the panels\n"
		"  Alt+I                the other panel shows this directory\n"
		"  Alt+.                show or hide hidden files\n"
		"  Left, Right          with Options > Lynx-like motion: the\n"
		"                       parent, into a directory or archive\n"
		"  Ctrl+N               another panel window (up to six)\n"
		"  Ctrl+Q, Alt+Q        quick view in the other panel: the\n"
		"                       picture, text or hex dump of the file\n"
		"                       under the cursor (Left, Right menus);\n"
		"                       for a folder what it holds, counted\n"
		"                       for up to 5 s (Options > Preferences)\n"
		"\n"
		"Files (the marked ones, or the one under the cursor)\n"
		"  F3                   view: pictures, text read-only, binary\n"
		"                       in hex\n"
		"  F4, Shift+F4         edit, edit a new file\n"
		"  F5                   copy to the other panel\n"
		"  Ctrl+C, Ctrl+Insert  the full path to the clipboard (the\n"
		"                       archive's, inside one), or the names\n"
		"                       of the marked entries, one a line\n"
		"  Shift+F5             copy in the same directory, under a\n"
		"                       new name\n"
		"  F6, Shift+F6         move to the other panel, rename\n"
		"  F7                   make a directory\n"
		"  F8, Delete           delete\n"
		"  Copies and moves ask before they replace anything, and\n"
		"  can be cancelled. A target name may start with ~ for home.\n"
		"\n"
		"Archives (zip, tar.gz and other tars, 7z, rar, iso, deb...)\n"
		"  Enter                go in, as into a directory; .. leaves\n"
		"  F3                   the archive window: the whole tree with\n"
		"                       sizes and times; Enter views a file or\n"
		"                       shows a directory in the panel, F5\n"
		"                       copies it out\n"
		"  In an archive, view, quick view, hex view and F5 copy work;\n"
		"  nothing can be edited, moved, deleted or run there.\n"
		"  A big archive is read in the background: a dialog shows\n"
		"  how far, and Esc or Cancel stops it; quick view shows the\n"
		"  same without waiting.\n"
		"\n"
		"Running programs\n"
		"  F2, Alt+Enter        run a command in this directory; it\n"
		"                       starts with the name under the cursor\n"
		"  In a terminal the command gets the whole terminal, and a\n"
		"  key brings t2fc back afterwards. In a graphic window it runs\n"
		"  in a new terminal window, which stays open until Enter;\n"
		"  Options > Preferences can name the terminal program.\n"
		"  Command > Open with the desktop: the desktop's own program\n"
		"  for the file (xdg-open)\n"
		"\n"
		"Picture windows\n"
		"  + -, mouse wheel     zoom in, out\n"
		"  0 or f, 1, 2         fit the window, pixels 1:1, 2:1\n"
		"  arrows, drag         move a zoomed picture\n"
		"  double click         1:1 there, or fit again\n"
		"  Options > Lanczos scaling down: smoother pictures and\n"
		"  videos where they are shown smaller (slower)\n"
		"  Space, Backspace     the next, previous picture of the panel\n"
		"  r                    turn a quarter clockwise\n"
		"  F3, Esc              close\n"
		"  PNG, JPEG, GIF (animated), WebP (animated), BMP, TGA, PSD,\n"
		"  HDR, PNM, and SVG through librsvg when installed (small\n"
		"  drawings at 1024 pixels); photos are turned upright by\n"
		"  their EXIF data.\n"
		"  Videos (MP4, MKV, WebM, AVI, MOV, ...) play through ffmpeg,\n"
		"  or else GStreamer, when installed, without sound:\n"
		"  p, Enter             pause, play (at the end: again)\n"
		"  Left, Right          5 s back, on (when the video fits)\n"
		"  Down, Up             a minute back, on\n"
		"  , .                  10 s back, on\n"
		"  Quick view plays them smaller, again and again.\n"
		"\n"
		"View and edit windows\n"
		"  F3, Esc              close a view window\n"
		"  F4                   make a view window editable\n"
		"  F2, Ctrl+S           save (F2 runs a command in the panels)\n"
		"  Ctrl+Z, Ctrl+Y       undo, redo\n"
		"  Ctrl+X, Ctrl+C, Ctrl+V   cut, copy, paste\n"
		"  Shift+arrows, Home, End, PgUp, PgDn   select;\n"
		"                       with Ctrl, by words\n"
		"  Alt+Shift+arrows ...  select a block of columns; it is\n"
		"                       cut, copied, deleted and pasted as one\n"
		"  F3, Shift+F3         start selecting (a block): the cursor\n"
		"                       keys select without Shift until F3\n"
		"                       again, for terminals that keep\n"
		"                       Shift+Up and the like for themselves\n"
		"  Text selected within a line is lit where else it occurs.\n"
		"  Ctrl+F, Ctrl+L       find, find next; Ctrl+R replace\n"
		"  Ctrl+G               go to a line\n"
		"  Ctrl+W, Esc Esc      close\n"
		"\n"
		"These are the keys at first: Options > Keyboard changes\n"
		"them, and t2fc.keys keeps the changes.\n"
		"\n"
		"Desktop\n"
		"  F9, Alt+letter       the menus\n"
		"  Alt+1..9             a window by number\n"
		"  1..9 a b c on the menu bar  click one to go to that window;\n"
		"                       up to 12 windows, besides dialogs\n"
		"  Alt+0                the list of windows\n"
		"  Alt+N                the Windows menu: pick one to go to it\n"
		"  double click a title zoom the window, or put it back\n"
		"  Shift+F10            the context menu\n"
		"  Ctrl+= Ctrl+- Ctrl+0 in a graphic window: bigger, smaller\n"
		"                       pixels (1x to 4x), and back; also\n"
		"                       Options > Pixel size\n"
		"  Up, Down in a dialog's input   earlier entries;\n"
		"  Ctrl+Down, Alt+Down, F4 or its \xe2\x96\xbc   the list\n"
		"  F10, Alt+X           quit\n";
	char line[80];
	int o=0;
	int i=0;
	while(text[i]!=0)
	{
		if(text[i]==10)
		{
			line[o]=0;
			VwAddLine(t,&line[0]);
			o=0;
		}
		else if(o<78)
		{
			line[o]=text[i];
			o++;
		}
		i++;
	}
}

// ---------------------------------------------------------------- application hooks

void TvAppDraw(int win)
{
	int type=tvWin[win].type;
	if(type==W_FC)
	{
		// drawing may come in the middle of another panel window's command
		int keep=fcC;
		FcSelect(tvWin[win].a);
		FcDraw(win);
		FcSelect(keep);
	}
	else if(type==W_VIEW)
	{
		FcEditorTitle(win);
		DrawViewer(win);
	}
	else if(type==W_HEX)HxDraw(win);
	else if(type==W_PIC)PicDraw(win);
	else if(type==W_ARC)ArcDraw(win);
	else if(type==W_THEME)TeDraw(win);
	else if(type==W_JOB)DrawJob(win);
	else if(type==W_ARCWAIT)DrawArcWait(win);
	else if(type==W_ASK)DrawAsk(win);
	else if(type==W_PREFS)DrawPrefs(win);
}

int fcEscWin=-1;    // the edit window Esc was last pressed in, and when
int fcEscMs=0;

int TvAppKey(int win, int key)
{
	TWin& t=tvWin[win];
	int type=t.type;
	if(type==W_FC)
	{
		FcSelect(t.a);
		return FcKey(win,key);
	}
	// view windows close with Esc, and F3 as in MC (a command of the keyboard map)
	if((type==W_HEX)||(type==W_PIC)||(type==W_ARC)||((type==W_VIEW)&&fcView[win]))
	{
		if(key==K_ESC)
		{
			TvClose(win);
			return 1;
		}
	}
	// an edit window closes with Esc pressed twice, asking about unsaved changes
	if(type==W_VIEW)
	{
		if((key==K_ESC)&&!ed[win].mark)
		{
			int now=term_ms();
			if((fcEscWin==win)&&(now-fcEscMs<600))
			{
				fcEscWin=-1;
				if(TvAppCanClose(win))TvClose(win);
				return 1;
			}
			fcEscWin=win;
			fcEscMs=now;
			return 1;
		}
		fcEscWin=-1;
		return ViewerKey(win,key);
	}
	if(type==W_PIC)return PicKey(win,key);
	if(type==W_ARC)return ArcKey(win,key);
	if(type==W_HEX)return HxKey(win,key);
	if(type==W_THEME)return TeKey(win,key);
	return 0;
}

// The status line context of a window: view windows have their own keys.
int TvAppStatus(int win)
{
	TWin& t=tvWin[win];
	if(t.modal)return TV_ST_DIALOG;
	if((t.type==W_VIEW)&&fcView[win])return ST_VIEWER;
	return t.type;
}

int TvAppMouse(int win, int ev, int x, int y)
{
	TWin& t=tvWin[win];
	int type=t.type;
	if(type==W_FC)
	{
		FcSelect(t.a);
		return FcMouse(win,ev,x,y);
	}
	if(type==W_THEME)return TeMouse(win,ev,x,y);
	if(ev==EV_RDOWN)
	{
		if(t.flags&WF_DIALOG)return 0;
		if(type!=W_VIEW)return 0;
		if(x<0)TvOpenPopup(popEdit,-1,-1);
		else TvOpenPopup(popEdit,t.x+x,t.y+y);
		return 1;
	}
	if(type==W_VIEW)return ViewerMouse(win,ev,x,y);
	if(type==W_PIC)return PicMouse(win,ev,x,y);
	if(type==W_ARC)return ArcMouse(win,ev,x,y);
	if(type==W_HEX)return HxMouse(win,ev,x,y);
	return 0;
}

void TvAppButton(int win, int ctl, int cmd)
{
	if(cmd==CM_THEMEPICK)
	{
		TeUseTheme(tvCtl[ctl].val);
		return;
	}
	if(cmd==CM_DESKPICK)
	{
		// shown at once, over whichever theme the dialog's combo box has
		teDeskPick=tvCtl[ctl].val;
		if(prefsThemeCtl>=0)TeUseTheme(tvCtl[prefsThemeCtl].val);
		return;
	}
	if((cmd>=CM_TEDEPTH)&&(cmd<=CM_TETHEME))
		TeButton(win,ctl,cmd);
}

void TvAppClosed(int win)
{
	if(tvWin[win].type==W_VIEW)
	{
		EdClosed(win);
		fcView[win]=0;
		fcSaveName[win*FC_PATH]=0;
		FcReloadAll();
	}
	if(win==hxWin)
	{
		free(hxBuf);
		hxWin=-1;
	}
	if(tvWin[win].type==W_PIC)PvFree(win);
	if(tvWin[win].type==W_FC)
	{
		int c=tvWin[win].a;
		fcWinOf[c]=-1;
		if(fcC==c)fcWin=-1;
	}
}

void TvAppTick()
{
	if(fcArcWin>=0)
	{
		// nothing else while an archive is read for a panel: the panels are not
		// to be reloaded in the middle of loading one
		FcArcWaitTick();
		PvTick();
		return;
	}
	if(fcQArcWait)tvDirty=1;    // quick view shows how far the reading got
	PvTick();
	FcCapCheck();
	if(fcJobWin>=0)
	{
		FcJobTick();
		return;
	}
	// another program, or the shell, changed a directory a panel shows
	int keep=fcC;
	for(int c=0;c<FC_MAXC;c++)
		if(fcWinOf[c]>=0)
		{
			FcSelect(c);
			for(int p=fcBase;p<fcBase+2;p++)
				if(FcStampOf(FcDirOf(p))!=fcStamp[p])
				{
					FcReload(p);
					tvDirty=1;
				}
		}
	FcSelect(keep);
}

void TvAppListItem(int win, int ctl, int i, char* out, int max)
{
	if(tvWin[win].type==W_THEME)
	{
		TeListItem(ctl,i,out,max);
		return;
	}
	out[0]=0;
}

void FcQuit()
{
	// the settings keep the directories of the panel window in front
	if(FcTopC()>=0)FcSelect(FcTopC());
	int r=EdCanQuit();
	if(r==1)tvRunning=0;
	else if(r==2)
		if(TvMessage("Quit","Leave t2fc?",1)==CM_YES)
			tvRunning=0;
	if(!tvRunning)
	{
		PrefsSave();
		TvHistSave(HIST_FILE);
	}
}

int TvAppCanClose(int win)
{
	// closing the last panel window is quitting
	if(tvWin[win].type==W_FC)
	{
		int open=0;
		for(int c=0;c<FC_MAXC;c++)
			if(fcWinOf[c]>=0)open++;
		if(open>1)return 1;
		FcQuit();
		return 0;
	}
	return EdCanClose(win);
}

int TvAppPaste(int win, char* s, int n)
{
	return EdPaste(win,s,n);
}

// Opens a panel window on these directories, with the modes of panel window from
// (or those already set, when from is -1). Returns its number, or -1.
int FcOpen(char* left, char* right, int from)
{
	int c=-1;
	int open=0;
	for(int k=FC_MAXC-1;k>=0;k--)
	{
		if(fcWinOf[k]<0)c=k;
		else open++;
	}
	if(c<0)
	{
		TvMessage("New panels","There are six panel windows open already.",0);
		return -1;
	}
	int w=tvW-8;
	int h=tvH-6;
	int x=1+open*2;
	int y=2+open;
	if(x+w>tvW)x=tvW-w;
	if(y+h>tvH-1)y=tvH-1-h;
	int win=TvNew(W_FC,"Panels",x,y,w,h,WF_RESIZE);
	if(win<0)
	{
		TvMessage("New panels","Too many windows are open.",0);
		return -1;
	}
	tvWin[win].a=c;
	fcWinOf[c]=win;
	if(from>=0)
	{
		for(int side=0;side<2;side++)
		{
			fcSort[c*2+side]=fcSort[from*2+side];
			fcBrief[c*2+side]=fcBrief[from*2+side];
		}
		fcAct[c]=c*2+(fcAct[from]&1);
	}
	FcSelect(c);
	for(int p=fcBase;p<fcBase+2;p++)
	{
		fcDir[p*FC_PATH]=0;
		fcCur[p]=0;
		fcTop[p]=0;
		char* dir=left;
		if(p==fcBase+1)dir=right;
		if(!FcLoad(p,dir,""))
			if(!FcLoad(p,".",""))FcLoad(p,"/","");
		// a listing slot used before may still hold marks
		FcUse(p);
		LsMarkMatch(0,"",1);
	}
	return c;
}

// Window > New panels: another panel window, on the directories of the front one.
void FcNewWindow()
{
	int from=FcTopC();
	char left[FC_PATH];
	char right[FC_PATH];
	StrCpy(&left[0],".",FC_PATH);
	fs_home(&right[0],FC_PATH);
	if(from>=0)
	{
		StrCpy(&left[0],FcDirOf(from*2),FC_PATH);
		StrCpy(&right[0],FcDirOf(from*2+1),FC_PATH);
	}
	FcOpen(&left[0],&right[0],from);
}

// The panel commands act on the panels even when another window is in front.
int FcPanelCommand(int cmd)
{
	if((cmd<FC_CMD0)||(cmd>FC_CMD1))return 0;
	if(cmd==CM_NEWPANELS)
	{
		FcNewWindow();
		return 1;
	}
	int c=FcTopC();
	if(c<0)return 1;
	FcSelect(c);
	TvToFront(fcWin);
	int p=fcActive;
	if(cmd==CM_VIEW)FcView();
	else if(cmd==CM_EDIT)FcEdit();
	else if(cmd==CM_NEWFILE)FcNewFile();
	else if(cmd==CM_FCOPY)FcCopyMove(0,0);
	else if(cmd==CM_COPYAS)FcCopyMove(0,1);
	else if(cmd==CM_COPYPATH)FcCopyPath();
	else if(cmd==CM_MOVE)FcCopyMove(1,0);
	else if(cmd==CM_RENAME)FcCopyMove(1,1);
	else if(cmd==CM_MKDIR)FcMkdir();
	else if(cmd==CM_DELETE)FcDelete();
	else if(cmd==CM_HEXVIEW)FcHex();
	else if(cmd==CM_SELGROUP)FcMarkGroup(1);
	else if(cmd==CM_MARKEXT)FcMarkExt(1);
	else if(cmd==CM_UNMARKEXT)FcMarkExt(0);
	else if(cmd==CM_UNSELGROUP)FcMarkGroup(0);
	else if(cmd==CM_INVERT)
	{
		FcUse(p);
		LsMarkMatch(2,"",0);
	}
	else if(cmd==CM_SWAP)FcSwap();
	else if(cmd==CM_SAMEDIR)FcLoad(p^1,FcDirOf(p),"");
	else if(cmd==CM_REREAD)FcReloadBoth();
	else if(cmd==CM_HIDDEN)
	{
		fcHidden=!fcHidden;
		FcReloadBoth();
	}
	else if(cmd==CM_COMPARE)FcCompare();
	else if(cmd==CM_RUN)FcRunDialog();
	else if(cmd==CM_OPENSYS)FcOpenSystem();
	else if(cmd==CM_OTHERPANEL)
	{
		if(!fcQuick[p^1])FcSetActive(p^1);
	}
	else if(cmd==CM_QUICKOTHER)FcSetQuick(p^1,!fcQuick[p^1]);
	else if(cmd==CM_PARENT)FcGoUp(p);
	else if(cmd==CM_MARK)
	{
		FcUse(p);
		LsMark(fcCur[p],-1);
		fcCur[p]++;
	}
	else if(cmd==CM_HOME)
	{
		char home[FC_PATH];
		fs_home(&home[0],FC_PATH);
		FcChangeDir(p,&home[0]);
	}
	else if(cmd==CM_GODIR)
	{
		char typed[FC_PATH];
		char path[FC_PATH];
		fcAskHist=H_DIR;
		if(FcAsk("Go to directory","Directory:",FcDirOf(p),&typed[0]))
		{
			FcResolve(&typed[0],&path[0]);
			FcChangeDir(p,&path[0]);
		}
	}
	else
	{
		int q=fcBase+(cmd-CM_PANEL)/10;
		int a=(cmd-CM_PANEL)%10;
		if(a==PN_QUICK)
		{
			FcSetQuick(q,!fcQuick[q]);
			ShowToggles();
			return 1;
		}
		if(fcQuick[q]&&((a==PN_FULL)||(a==PN_BRIEF)))FcSetQuick(q,0);
		if(a==PN_FULL)fcBrief[q]=0;
		else if(a==PN_BRIEF)fcBrief[q]=1;
		else if(a==PN_REVERSE)fcSort[q]=fcSort[q]^4;
		else fcSort[q]=(fcSort[q]&4)|(a-PN_NAME);
		FcReload(q);
	}
	FcClamp(fcBase);
	FcClamp(fcBase+1);
	ShowToggles();
	return 1;
}

void TvAppCommand(int cmd)
{
	// a command ends a quick search
	if(fcQS>=0)FcQSEnd();
	int top=TvTop();
	if(((cmd>=CM_PIC0)&&(cmd<=CM_PIC1))||(cmd==CM_PICBACK))
	{
		if(top>=0)
			if(tvWin[top].type==W_PIC)PicCommand(top,cmd);
		return;
	}
	if((cmd==CM_ARCOPEN)||(cmd==CM_ARCCOPY))
	{
		if(top>=0)
			if(tvWin[top].type==W_ARC)ArcCommand(top,cmd);
		return;
	}
	if(cmd==CM_VIEWEDIT)
	{
		// F4 makes a view window editable
		if(top>=0)
			if((tvWin[top].type==W_VIEW)&&fcView[top]&&(ed[top].path[0]!=0))
				FcEditPath(&ed[top].path[0]);
		return;
	}
	if((top>=0)&&(top==hxWin)&&(cmd==CM_GOTO))
	{
		HxGoto(top);
		return;
	}
	if(FcPanelCommand(cmd))return;
	// the commands below that use the panels' directories take the front window's
	if(FcTopC()>=0)FcSelect(FcTopC());
	if(EdCommand(cmd))
	{
		if((cmd==CM_SAVE)||(cmd==CM_SAVEAS))FcReloadAll();
		return;
	}
	if(cmd==CM_QUIT)FcQuit();
	else if(cmd==CM_HELP)OpenHelp();
	else if(cmd==CM_PREFS)RunPrefs();
	else if(cmd==CM_THEMEED)
	{
		int r=TeRun(prefTheme);
		if(r>=0)
		{
			prefTheme=r;
			PrefsSave();
		}
	}
	else if(cmd==CM_LINENUMS)
	{
		edLineNums=!edLineNums;
		ShowToggles();
		PrefsSave();
	}
	else if((cmd>CM_PIX)&&(cmd<=CM_PIX+4))
	{
#ifdef TV_GFX
		GfxSetScale(cmd-CM_PIX);
		ShowToggles();
		PrefsSave();
#endif
	}
	else if(cmd==CM_LANCZOS)
	{
		pvLanczos=!pvLanczos;
		img_filter(pvLanczos);
		ShowToggles();
		PrefsSave();
		tvDirty=1;
	}
	else if(cmd==CM_LYNX)
	{
		fcLynx=!fcLynx;
		ShowToggles();
		PrefsSave();
	}
	else if(cmd==CM_WRAP)
	{
		edWrap=!edWrap;
		ShowToggles();
		PrefsSave();
	}
	else if(cmd==CM_ABOUT)
		TvMessage("About","t2fc, a two-panel file manager\nWritten in Scinc on the tui desktop",0);
}

// ---------------------------------------------------------------- main

// The keyboard map: every command a key can give, by window, with its default keys.
// Options > Keyboard changes them, and t2fc.keys keeps the changes.
void FcKeys()
{
	TvKeyContext(TV_ST_ALL,TV_ST_ALL,"Everywhere");
	TvKeyContext(W_FC,TV_ST_ALL,"Panels");
	TvKeyContext(W_VIEW,TV_ST_ALL,"Edit and view windows");
	TvKeyContext(ST_VIEWER,W_VIEW,"View windows");
	TvKeyContext(W_PIC,TV_ST_ALL,"Picture windows");
	TvKeyContext(W_HEX,TV_ST_ALL,"Hex view");
	TvKeyContext(W_ARC,TV_ST_ALL,"Archive windows");

	TvKeyCmd(TV_ST_ALL,CM_QUIT,"Quit",K_F10,K_ALT+'x',0);
	TvKeyCmd(TV_ST_ALL,CM_HELP,"Help",K_F1,0,0);
	TvKeyCmd(TV_ST_ALL,CM_NEWPANELS,"New panels",14,0,0);
	TvKeyCmd(TV_ST_ALL,CM_CLOSE,"Close the window",23,0,0);
	TvKeyCmd(TV_ST_ALL,CM_WINLIST,"Window list",K_ALT+'0',0,0);
	TvKeyCmd(TV_ST_ALL,CM_NEXT,"Next window",0,0,0);
	TvKeyCmd(TV_ST_ALL,CM_PREV,"Previous window",0,0,0);
	TvKeyCmd(TV_ST_ALL,CM_ZOOM,"Zoom the window",0,0,0);
	TvKeyCmd(TV_ST_ALL,CM_PREFS,"Preferences",0,0,0);
	TvKeyCmd(TV_ST_ALL,CM_THEMEED,"Theme editor",0,0,0);
	TvKeyCmd(TV_ST_ALL,CM_KEYS,"Keyboard",0,0,0);
	TvKeyCmd(TV_ST_ALL,CM_LINENUMS,"Line numbers",0,0,0);
	TvKeyCmd(TV_ST_ALL,CM_WRAP,"Word wrap",0,0,0);
	TvKeyCmd(TV_ST_ALL,CM_LYNX,"Lynx-like motion",0,0,0);
	TvKeyCmd(TV_ST_ALL,CM_LANCZOS,"Lanczos scaling down",0,0,0);
#ifdef TV_GFX
	TvKeyCmd(TV_ST_ALL,CM_PIX+1,"Pixel size 1x",0,0,0);
	TvKeyCmd(TV_ST_ALL,CM_PIX+2,"Pixel size 2x",0,0,0);
	TvKeyCmd(TV_ST_ALL,CM_PIX+3,"Pixel size 3x",0,0,0);
	TvKeyCmd(TV_ST_ALL,CM_PIX+4,"Pixel size 4x",0,0,0);
#endif

	TvKeyCmd(W_FC,CM_OTHERPANEL,"Other panel",K_TAB,0,0);
	TvKeyCmd(W_FC,CM_RUN,"Run",K_F2,K_ALT+K_ENTER,0);
	TvKeyCmd(W_FC,CM_VIEW,"View",K_F3,0,0);
	TvKeyCmd(W_FC,CM_EDIT,"Edit",K_F4,0,0);
	TvKeyCmd(W_FC,CM_NEWFILE,"New file",K_F4+TV_KSHIFT,0,0);
	TvKeyCmd(W_FC,CM_FCOPY,"Copy",K_F5,0,0);
	TvKeyCmd(W_FC,CM_COPYAS,"Copy in the same directory",K_F5+TV_KSHIFT,0,0);
	TvKeyCmd(W_FC,CM_COPYPATH,"Copy the path or the marked names",3,K_INS+TV_KCTRL,0);
	TvKeyCmd(W_FC,CM_MOVE,"Move",K_F6,0,0);
	TvKeyCmd(W_FC,CM_RENAME,"Rename",K_F6+TV_KSHIFT,0,0);
	TvKeyCmd(W_FC,CM_MKDIR,"Make directory",K_F7,0,0);
	TvKeyCmd(W_FC,CM_DELETE,"Delete",K_F8,K_DEL,0);
	TvKeyCmd(W_FC,CM_HEXVIEW,"Hex view",0,0,0);
	TvKeyCmd(W_FC,CM_MARK,"Mark",K_INS,0,0);
	TvKeyCmd(W_FC,CM_SELGROUP,"Select group",'+',0,0);
	TvKeyCmd(W_FC,CM_MARKEXT,"Mark the files with this extension",K_KPADD+TV_KSHIFT,K_ALT+'+',0);
	TvKeyCmd(W_FC,CM_UNMARKEXT,"Unmark the files with this extension",K_KPSUB+TV_KSHIFT,K_ALT+'-',0);
	TvKeyCmd(W_FC,CM_UNSELGROUP,"Unselect group",'-',0,0);
	TvKeyCmd(W_FC,CM_INVERT,"Invert selection",'*',0,0);
	TvKeyCmd(W_FC,CM_PARENT,"Parent directory",K_BACKSPACE,0,0);
	TvKeyCmd(W_FC,CM_SWAP,"Swap panels",21,0,0);
	TvKeyCmd(W_FC,CM_SAMEDIR,"Other panel here",K_ALT+'i',0,0);
	TvKeyCmd(W_FC,CM_QUICKOTHER,"Quick view",17,K_ALT+'q',0);
	TvKeyCmd(W_FC,CM_HIDDEN,"Hidden files",K_ALT+'.',0,0);
	TvKeyCmd(W_FC,CM_REREAD,"Reread",18,0,0);
	TvKeyCmd(W_FC,CM_COMPARE,"Compare directories",0,0,0);
	TvKeyCmd(W_FC,CM_HOME,"Home directory",0,0,0);
	TvKeyCmd(W_FC,CM_GODIR,"Go to directory",0,0,0);
	TvKeyCmd(W_FC,CM_OPENSYS,"Open with the desktop",0,0,0);

	TvKeyCmd(W_VIEW,CM_SAVE,"Save",K_F2,19,0);
	TvKeyCmd(W_VIEW,CM_SAVEAS,"Save as",0,0,0);
	TvKeyCmd(W_VIEW,CM_UNDO,"Undo",26,0,0);
	TvKeyCmd(W_VIEW,CM_REDO,"Redo",25,0,0);
	TvKeyCmd(W_VIEW,CM_CUT,"Cut",24,0,0);
	TvKeyCmd(W_VIEW,CM_COPY,"Copy",3,0,0);
	TvKeyCmd(W_VIEW,CM_PASTE,"Paste",22,0,0);
	TvKeyCmd(W_VIEW,CM_SELALL,"Select all",1,0,0);
	TvKeyCmd(W_VIEW,CM_MARKSEL,"Start or stop selecting",K_F3,0,0);
	TvKeyCmd(W_VIEW,CM_MARKBLK,"Start or stop selecting a block",K_F3+TV_KSHIFT,0,0);
	TvKeyCmd(W_VIEW,CM_FIND,"Find",6,0,0);
	TvKeyCmd(W_VIEW,CM_FINDNEXT,"Find next",12,0,0);
	TvKeyCmd(W_VIEW,CM_REPLACE,"Replace",18,0,0);
	TvKeyCmd(W_VIEW,CM_GOTO,"Go to line",7,0,0);
	TvKeyCmd(W_VIEW,CM_MATCH,"Matching bracket",2,0,0);
	TvKeyCmd(ST_VIEWER,CM_CLOSE,"Close",K_F3,0,0);
	TvKeyCmd(ST_VIEWER,CM_VIEWEDIT,"Edit",K_F4,0,0);

	TvKeyCmd(W_PIC,CM_PICIN,"Zoom in",'+','=',0);
	TvKeyCmd(W_PIC,CM_PICOUT,"Zoom out",'-',0,0);
	TvKeyCmd(W_PIC,CM_PICFIT,"Fit the window",'0','*','f');
	TvKeyCmd(W_PIC,CM_PIC1X,"Pixels 1:1",'1',0,0);
	TvKeyCmd(W_PIC,CM_PIC2X,"Pixels 2:1",'2',0,0);
	TvKeyCmd(W_PIC,CM_PICNEXT,"Next picture",' ',K_PGDN,0);
	TvKeyCmd(W_PIC,CM_PICPREV,"Previous picture",K_BACKSPACE,K_PGUP,0);
	TvKeyCmd(W_PIC,CM_PICTURN,"Turn clockwise",'r',0,0);
	TvKeyCmd(W_PIC,CM_PICPLAY,"Video: pause, play",'p',K_ENTER,0);
	TvKeyCmd(W_PIC,CM_PICFWD,"Video: 10 s on",'.','>',0);
	TvKeyCmd(W_PIC,CM_PICBACK,"Video: 10 s back",',','<',0);
	TvKeyCmd(W_PIC,CM_CLOSE,"Close",K_F3,0,0);

	TvKeyCmd(W_HEX,CM_GOTO,"Go to offset",'g',7,0);
	TvKeyCmd(W_HEX,CM_CLOSE,"Close",K_F3,0,0);

	TvKeyCmd(W_ARC,CM_ARCOPEN,"View, or show in the panel",K_ENTER,0,0);
	TvKeyCmd(W_ARC,CM_ARCCOPY,"Copy out",K_F5,0,0);
	TvKeyCmd(W_ARC,CM_CLOSE,"Close",K_F3,0,0);
}

// A panel's menu: listing mode, sort keys, reverse. Returns its first item.
int PanelMenu(char* title, int p)
{
	TvMenuAdd(title);
	int first=tvNItem;
	int base=CM_PANEL+p*10;
	TvMenuItem("~F~ull listing","",base+PN_FULL,0);
	TvMenuItem("~B~rief listing","",base+PN_BRIEF,0);
	TvMenuItem("~Q~uick view","",base+PN_QUICK,0);
	TvMenuItem("Sort by ~n~ame","",base+PN_NAME,0);
	TvMenuItem("Sort by ~e~xtension","",base+PN_EXT,0);
	TvMenuItem("Sort by ~s~ize","",base+PN_SIZE,0);
	TvMenuItem("Sort by ~t~ime","",base+PN_TIME,0);
	TvMenuItem("~R~everse order","",base+PN_REVERSE,0);
	return first;
}

int main(int argc, pchar* argv)
{
	tvMenuKey=K_F9;   // as in Midnight Commander: F9 the menus, F10 quits
	if(!TvStart(CM_QUIT))return 1;
	EdInit();
	FcInitState();
	PvInit();
	FcQInit();
	char left[FC_PATH];
	char right[FC_PATH];
	StrCpy(&left[0],".",FC_PATH);
	fs_home(&right[0],FC_PATH);
	PrefsLoad(&left[0],&right[0]);
#ifdef TV_GFX
	if(prefScale>0)GfxSetScale(prefScale);
#endif
	if(argc>1)StrCpy(&left[0],argv[1],FC_PATH);
	if(argc>2)StrCpy(&right[0],argv[2],FC_PATH);
	TeLoadThemes();
	if(prefTheme>=teThN)prefTheme=0;
	TeUseTheme(prefTheme);

	FcKeys();
	TvKeysFile(KEYS_FILE);
	TvHistLoad(HIST_FILE);
	itemPanel[0]=PanelMenu("~L~eft",0);
	TvMenuAdd("~F~ile");
	TvMenuItem("~V~iew","",CM_VIEW,0);
	TvMenuItem("~E~dit","",CM_EDIT,0);
	TvMenuItem("~N~ew file...","",CM_NEWFILE,0);
	TvMenuItem("~C~opy...","",CM_FCOPY,0);
	TvMenuItem("Copy ~a~s...","",CM_COPYAS,0);
	TvMenuItem("Copy ~p~ath","",CM_COPYPATH,0);
	TvMenuItem("Rename or ~m~ove...","",CM_MOVE,0);
	TvMenuItem("Rena~m~e...","",CM_RENAME,0);
	TvMenuItem("Make ~d~irectory...","",CM_MKDIR,0);
	TvMenuItem("De~l~ete...","",CM_DELETE,0);
	TvMenuItem("~H~ex view","",CM_HEXVIEW,0);
	TvMenuLine();
	TvMenuItem("~S~elect group...","",CM_SELGROUP,0);
	TvMenuItem("Select by e~x~tension","",CM_MARKEXT,0);
	TvMenuItem("Unselect by ex~t~ension","",CM_UNMARKEXT,0);
	TvMenuItem("~U~nselect group...","",CM_UNSELGROUP,0);
	TvMenuItem("~I~nvert selection","",CM_INVERT,0);
	TvMenuLine();
	TvMenuItem("~S~ave","",CM_SAVE,0);
	TvMenuItem("Save ~a~s...","",CM_SAVEAS,0);
	TvMenuLine();
	TvMenuItem("E~x~it","",CM_QUIT,0);
	TvMenuAdd("~C~ommand");
	TvMenuItem("~R~un...","",CM_RUN,0);
	TvMenuItem("Open ~w~ith the desktop","",CM_OPENSYS,0);
	TvMenuLine();
	TvMenuItem("~S~wap panels","",CM_SWAP,0);
	TvMenuItem("~O~ther panel here","",CM_SAMEDIR,0);
	TvMenuItem("~C~ompare directories","",CM_COMPARE,0);
	TvMenuItem("R~e~read","",CM_REREAD,0);
	TvMenuLine();
	TvMenuItem("~H~ome directory","",CM_HOME,0);
	TvMenuItem("~G~o to directory...","",CM_GODIR,0);
	TvMenuAdd("~E~dit");
	TvMenuItem("~U~ndo","",CM_UNDO,0);
	TvMenuItem("~R~edo","",CM_REDO,0);
	TvMenuLine();
	TvMenuItem("Cu~t~","",CM_CUT,0);
	TvMenuItem("~C~opy","",CM_COPY,0);
	TvMenuItem("~P~aste","",CM_PASTE,0);
	TvMenuItem("Select ~a~ll","",CM_SELALL,0);
	TvMenuItem("Start/stop selectin~g~","",CM_MARKSEL,0);
	TvMenuItem("Select a ~b~lock","",CM_MARKBLK,0);
	TvMenuLine();
	TvMenuItem("~F~ind...","",CM_FIND,0);
	TvMenuItem("Find ~n~ext","",CM_FINDNEXT,0);
	TvMenuItem("R~e~place...","",CM_REPLACE,0);
	TvMenuItem("~G~o to line...","",CM_GOTO,0);
	TvMenuItem("~M~atching bracket","",CM_MATCH,0);
	TvMenuAdd("~O~ptions");
	TvMenuItem("~P~references...","",CM_PREFS,0);
	TvMenuItem("~T~heme editor...","",CM_THEMEED,0);
	TvMenuItem("~K~eyboard...","",CM_KEYS,0);
	TvMenuLine();
	TvMenuItem("~H~idden files","Alt+.",CM_HIDDEN,0);
	itemHidden=tvNItem-1;
	TvMenuItem("~L~ine numbers","off",CM_LINENUMS,0);
	itemLineNums=tvNItem-1;
	TvMenuItem("~W~ord wrap","off",CM_WRAP,0);
	itemWrap=tvNItem-1;
	TvMenuItem("L~y~nx-like motion","off",CM_LYNX,0);
	itemLynx=tvNItem-1;
	TvMenuItem("Lan~c~zos scaling down","off",CM_LANCZOS,0);
	itemLanczos=tvNItem-1;
#ifdef TV_GFX
	int itemPixSub=TvMenuSub("Pi~x~el size");
#endif
	itemPanel[1]=PanelMenu("~R~ight",1);
	TvMenuAdd("~W~indow");
	TvMenuItem("Ne~w~ panels","",CM_NEWPANELS,0);
	TvMenuLine();
	TvMenuItem("~Z~oom","",CM_ZOOM,0);
	TvMenuItem("~N~ext","",CM_NEXT,0);
	TvMenuItem("~P~revious","",CM_PREV,0);
	TvMenuItem("~C~lose","",CM_CLOSE,0);
	TvMenuLine();
	TvMenuItem("C~a~scade","",CM_CASCADE,0);
	TvMenuItem("~T~ile","",CM_TILE,0);
	TvMenuItem("~L~ist...","",CM_WINLIST,0);
	TvWindowsMenu("Wi~n~dows");
	TvMenuAdd("~H~elp");
	TvMenuItem("~K~eys and mouse","",CM_HELP,0);
	TvMenuItem("~A~bout...","",CM_ABOUT,0);

#ifdef TV_GFX
	TvSubmenuAdd(itemPixSub);
	TvMenuItem("~1~x","",CM_PIX+1,0);
	itemPix=tvNItem-1;
	TvMenuItem("~2~x","",CM_PIX+2,0);
	TvMenuItem("~3~x","",CM_PIX+3,0);
	TvMenuItem("~4~x","",CM_PIX+4,0);
#endif
	popPanel=TvPopupAdd();
	TvMenuItem("R~u~n...","",CM_RUN,0);
	TvMenuItem("~O~pen with the desktop","",CM_OPENSYS,0);
	TvMenuItem("~V~iew","",CM_VIEW,0);
	TvMenuItem("~E~dit","",CM_EDIT,0);
	TvMenuItem("~C~opy...","",CM_FCOPY,0);
	TvMenuItem("Copy ~a~s...","",CM_COPYAS,0);
	TvMenuItem("~M~ove...","",CM_MOVE,0);
	TvMenuItem("~R~ename...","",CM_RENAME,0);
	TvMenuItem("~D~elete...","",CM_DELETE,0);
	TvMenuLine();
	TvMenuItem("~H~ex view","",CM_HEXVIEW,0);
	TvMenuItem("Make director~y~...","",CM_MKDIR,0);
	popEdit=TvPopupAdd();
	TvMenuItem("~U~ndo","",CM_UNDO,0);
	TvMenuItem("~R~edo","",CM_REDO,0);
	TvMenuLine();
	TvMenuItem("Cu~t~","",CM_CUT,0);
	TvMenuItem("~C~opy","",CM_COPY,0);
	TvMenuItem("~P~aste","",CM_PASTE,0);
	TvMenuItem("Select ~a~ll","",CM_SELALL,0);
	TvMenuItem("Start/stop selectin~g~","",CM_MARKSEL,0);
	TvMenuItem("Select a ~b~lock","",CM_MARKBLK,0);
	TvMenuLine();
	TvMenuItem("~F~ind...","",CM_FIND,0);
	TvMenuItem("~G~o to line...","",CM_GOTO,0);
	TvMenuLine();
	TvMenuItem("C~l~ose","",CM_CLOSE,0);

	// the status line shows the main keys of the front window (TvAppStatus). The F
	// keys of the panels are read by the panel window, so that view and edit windows
	// keep their own.
	TvStatusFor(W_FC);
	TvStatusCmd("Run",CM_RUN);
	TvStatusCmd("View",CM_VIEW);
	TvStatusCmd("Edit",CM_EDIT);
	TvStatusCmd("Copy",CM_FCOPY);
	TvStatusCmd("Move",CM_MOVE);
	TvStatusCmd("Mkdir",CM_MKDIR);
	TvStatusCmd("Delete",CM_DELETE);
	TvStatusFor(ST_VIEWER);
	TvStatusCmd("Close",CM_CLOSE);
	TvStatusCmd("Edit",CM_VIEWEDIT);
	TvStatusCmd("Find",CM_FIND);
	TvStatusCmd("Next",CM_FINDNEXT);
	TvStatusCmd("Line",CM_GOTO);
	TvStatusFor(W_VIEW);
	TvStatusCmd("Save",CM_SAVE);
	TvStatusCmd("Undo",CM_UNDO);
	TvStatusCmd("Find",CM_FIND);
	TvStatusCmd("Replace",CM_REPLACE);
	TvStatusCmd("Line",CM_GOTO);
	TvStatusFor(W_PIC);
	TvStatusCmd("In",CM_PICIN);
	TvStatusCmd("Out",CM_PICOUT);
	TvStatusCmd("Fit",CM_PICFIT);
	TvStatusCmd("1:1",CM_PIC1X);
	TvStatusCmd("Next",CM_PICNEXT);
	TvStatusCmd("Prev",CM_PICPREV);
	TvStatusCmd("Turn",CM_PICTURN);
	TvStatusCmd("Play",CM_PICPLAY);
	TvStatusCmd("Close",CM_CLOSE);
	TvStatusFor(W_HEX);
	TvStatusCmd("Go to",CM_GOTO);
	TvStatusAdd("~PgUp~ ~PgDn~ Page",0,0);
	TvStatusCmd("Close",CM_CLOSE);
	TvStatusFor(W_ARC);
	TvStatusCmd("View",CM_ARCOPEN);
	TvStatusCmd("Copy out",CM_ARCCOPY);
	TvStatusCmd("Close",CM_CLOSE);
	TvStatusFor(TV_ST_DESK);
	TvStatusCmd("Panels",CM_NEWPANELS);
	TvStatusFor(TV_ST_DIALOG);
	TvStatusAdd("~Tab~ Next field",0,K_TAB);
	TvStatusAdd("~Enter~ OK",0,K_ENTER);
	TvStatusAdd("~Esc~ Cancel",0,K_ESC);
	TvStatusFor(TV_ST_ALL);
	TvStatusAdd("~F9~ Menu",CM_MENU,0);
	TvStatusCmd("Quit",CM_QUIT);

	int first=FcOpen(&left[0],&right[0],-1);
	if(first>=0)TvZoom(fcWinOf[first]);
	ShowToggles();
	FcTickRate();

	TvRun(-1);
	TvStop();
	return 0;
}
