// The parts of a tv.h program that are the same in every program: empty hooks for
// the ones it does not need, and starting and stopping the desktop.
//
// tv.h calls twelve TvApp* hooks. A program writes only those it uses and says so
// with a TVAPP_* define before including this file, for example
//
//   #define TVAPP_DRAW
//   #define TVAPP_KEY
//   #include "tv.h"
//   #include "tvapp.h"
//   void TvAppDraw(int win) { ... }
//   int TvAppKey(int win, int key) { ... }
//
// and then in main: if(!TvStart())return 1; ...menus, windows...; TvRun(-1); TvStop();
// A program in another folder defines TERM_DL first (see term.h).
// tv.h is not included from here: the program includes it itself, because one more
// level of #include makes the stroke font's headers (tv.h -> tvgfx.h -> font.h)
// nest too deep for Scinc.
#ifndef TVAPP_H
#define TVAPP_H

// Scinc's preprocessor mishandles nested conditionals, so each hook has a flat block.
#ifndef TVAPP_DRAW
void TvAppDraw(int win){}
#endif
#ifndef TVAPP_KEY
int TvAppKey(int win, int key){return 0;}
#endif
#ifndef TVAPP_MOUSE
int TvAppMouse(int win, int ev, int x, int y){return 0;}
#endif
#ifndef TVAPP_COMMAND
void TvAppCommand(int cmd){}
#endif
#ifndef TVAPP_BUTTON
void TvAppButton(int win, int ctl, int cmd){}
#endif
#ifndef TVAPP_CLOSED
void TvAppClosed(int win){}
#endif
#ifndef TVAPP_LISTITEM
void TvAppListItem(int win, int ctl, int i, char* out, int max){out[0]=0;}
#endif
#ifndef TVAPP_CANCLOSE
int TvAppCanClose(int win){return 1;}
#endif
#ifndef TVAPP_PASTE
int TvAppPaste(int win, char* s, int n){return 0;}
#endif
#ifndef TVAPP_TICK
void TvAppTick(){}
#endif
#ifndef TVAPP_STATUS
int TvAppStatus(int win)
{
	if(tvWin[win].modal)return TV_ST_DIALOG;
	return tvWin[win].type;
}
#endif
#ifndef TVAPP_PIXELS
void TvAppPixels(int id, int* buf, int fullW, int fullH, int ox, int oy, int w, int h){}
#endif

// Opens the terminal, or with TV_GFX the window, and the desktop. quitCmd is the
// command the graphic window's close button sends (0 quits at once). Returns 0,
// after saying why, when there is no interactive terminal.
int TvStart(int quitCmd)
{
#ifdef TV_GFX
	tvCloseCmd=quitCmd;
#endif
#ifndef TV_GFX
	if(!term_open())
	{
		printf("This program needs an interactive terminal.\n");
		return 0;
	}
#endif
	TvInit();
	return 1;
}

void TvStop()
{
	TvDone();
}

#endif // TVAPP_H
