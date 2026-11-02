// The terminal services Scinc has no intrinsics for: raw mode, reading a key with
// a timeout, the terminal size and the local time. All drawing is plain ANSI
// output from the Scinc side.
#include <ctime>
#include <csignal>
#include <cstdlib>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "../../../include/ScincVM.h"
#include "../../../include/ScincBinder.h"

static termios savedMode;
static int rawActive=0;

// Must stay safe to call from a signal handler, so that a crash inside the
// interpreter does not leave the user's shell in raw mode on the alternate screen.
static void restore()
{
	if(!rawActive)
		return;
	rawActive=0;
	static const char leave[]="\x1b[?1006l\x1b[?1002l\x1b[?1000l\x1b[?7h\x1b[?25h\x1b[0m\x1b[?1049l";
	ssize_t ignored=write(1,leave,sizeof(leave)-1);
	(void)ignored;
	tcsetattr(0,TCSAFLUSH,&savedMode);
}

static void onSignal(int sig)
{
	restore();
	signal(sig,SIG_DFL);
	raise(sig);
}

static int term_open()
{
	if(!isatty(0)||!isatty(1))
		return 0;
	if(tcgetattr(0,&savedMode))
		return 0;
	termios raw=savedMode;
	cfmakeraw(&raw);
	if(tcsetattr(0,TCSAFLUSH,&raw))
		return 0;
	rawActive=1;
	atexit(restore);
	signal(SIGSEGV,onSignal);
	signal(SIGABRT,onSignal);
	signal(SIGBUS,onSignal);
	signal(SIGFPE,onSignal);
	signal(SIGTERM,onSignal);
	signal(SIGHUP,onSignal);
	return 1;
}

static int term_close()
{
	restore();
	return 0;
}

// Next input byte, or -1 if none arrives within timeoutMs.
static int term_read(int timeoutMs)
{
	static unsigned char buf[256];
	static int have=0;
	static int pos=0;
	if(pos>=have)
	{
		pollfd p;
		p.fd=0;
		p.events=POLLIN;
		p.revents=0;
		if(poll(&p,1,timeoutMs)<=0)
			return -1;
		have=read(0,buf,sizeof(buf));
		pos=0;
		if(have<=0)
		{
			// The terminal is gone. Without this the program would spin on
			// a hung-up descriptor forever, since poll keeps returning at once.
			have=0;
			restore();
			_exit(1);
		}
	}
	return buf[pos++];
}

static int term_cols()
{
	winsize ws;
	if(ioctl(1,TIOCGWINSZ,&ws)||!ws.ws_col)
		return 80;
	return ws.ws_col;
}

static int term_rows()
{
	winsize ws;
	if(ioctl(1,TIOCGWINSZ,&ws)||!ws.ws_row)
		return 25;
	return ws.ws_row;
}

// what: 0 hour, 1 minute, 2 second, 3 year, 4 month 1-12, 5 day of month
static int term_time(int what)
{
	time_t now=time(0);
	tm* t=localtime(&now);
	if(what==0)return t->tm_hour;
	if(what==1)return t->tm_min;
	if(what==2)return t->tm_sec;
	if(what==3)return t->tm_year+1900;
	if(what==4)return t->tm_mon+1;
	return t->tm_mday;
}

extern "C" int term_open_wrapper(ScincVM* s, void*){ScincBoundCall(s,term_open);return 0;}
extern "C" int term_close_wrapper(ScincVM* s, void*){ScincBoundCall(s,term_close);return 0;}
extern "C" int term_read_wrapper(ScincVM* s, void*){ScincBoundCall(s,term_read);return 0;}
extern "C" int term_cols_wrapper(ScincVM* s, void*){ScincBoundCall(s,term_cols);return 0;}
extern "C" int term_rows_wrapper(ScincVM* s, void*){ScincBoundCall(s,term_rows);return 0;}
extern "C" int term_time_wrapper(ScincVM* s, void*){ScincBoundCall(s,term_time);return 0;}
