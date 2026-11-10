// The services Scinc has no intrinsics for: raw mode, reading a key with a timeout,
// the terminal size, the local time, listing directories, saving files safely, and
// copying, moving and deleting files a step at a time, and running programs. All
// drawing is plain ANSI output from the Scinc side.
#include <algorithm>
#include <chrono>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <csignal>
#include <cstdlib>
#include <string>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <dirent.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <poll.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/statvfs.h>
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
	static const char leave[]="\x1b>\x1b[?2004l\x1b[?1006l\x1b[?1002l\x1b[?1000l\x1b[?7h\x1b[?25h\x1b[0m\x1b[?1049l";
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

static unsigned char readBuf[256];
static int readHave=0;
static int readPos=0;

// Next input byte, or -1 if none arrives within timeoutMs.
static int term_read(int timeoutMs)
{
	if(readPos>=readHave)
	{
		pollfd p;
		p.fd=0;
		p.events=POLLIN;
		p.revents=0;
		if(poll(&p,1,timeoutMs)<=0)
			return -1;
		readHave=read(0,readBuf,sizeof(readBuf));
		readPos=0;
		if(readHave<=0)
		{
			// The terminal is gone. Without this the program would spin on
			// a hung-up descriptor forever, since poll keeps returning at once.
			readHave=0;
			restore();
			_exit(1);
		}
	}
	return readBuf[readPos++];
}

// Is a byte waiting, so that the screen need not be redrawn just yet?
static int term_pending()
{
	if(readPos<readHave)
		return 1;
	pollfd p;
	p.fd=0;
	p.events=POLLIN;
	p.revents=0;
	return poll(&p,1,0)>0;
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

// Milliseconds on a monotonic clock, for game timing.
static int term_ms()
{
	using namespace std::chrono;
	static steady_clock::time_point start=steady_clock::now();
	return (int)duration_cast<milliseconds>(steady_clock::now()-start).count();
}

// Asks the terminal itself whether it shows 24-bit colours. COLORTERM says so on
// the machine the terminal runs on, but ssh passes only TERM, LANG and LC_* by
// default, so over ssh the answer has to come through the terminal: the replies
// travel back like keys. Three questions, then DA1, which every terminal answers,
// so that the wait ends as soon as that answer is in:
//   DECRQSS of an SGR with a 24-bit colour: xterm, kitty, foot, WezTerm, mintty...
//     echo the colour back when they keep it
//   XTVERSION: the terminal's name, as VTE(7600) for xfce4-terminal, GNOME
//     Terminal and other VTE ones, which never answer the first question
//   DA2: older VTE (and kitty) give a version of 4000 or more
// Anything else read meanwhile (keys typed early) is kept for term_read.
// Inside tmux the terminal questions reach tmux, not the terminal around it, and
// tmux passes 24-bit colours on only when it knows that terminal has them: its
// client's features then include RGB (set -as terminal-features ',xterm*:RGB').
// tmux is asked about the client this program's pane is shown on.
static int tmuxTrueColor()
{
	FILE* f=popen("tmux display-message -p '#{client_termfeatures}' 2>/dev/null","r");
	if(!f)return 0;
	char buf[512];
	std::string out;
	while(fgets(buf,sizeof buf,f))out+=buf;
	pclose(f);
	size_t i=0;
	while(i<out.size())
	{
		size_t e=out.find_first_of(",\n",i);
		if(e==std::string::npos)e=out.size();
		if(out.compare(i,e-i,"RGB")==0)return 1;
		i=e+1;
	}
	return 0;
}

static int probeTrueColor()
{
	const char* term=getenv("TERM");
	if(term&&(!strcmp(term,"linux")||!strcmp(term,"dumb")))return 0;
	termios saved;
	bool own=!rawActive;
	if(own)
	{
		if(tcgetattr(0,&saved))return 0;
		termios raw=saved;
		cfmakeraw(&raw);
		if(tcsetattr(0,TCSANOW,&raw))return 0;
	}
	const char* ask="\x1b[0m\x1b[48;2;1;2;3m\x1bP$qm\x1b\\\x1b[0m\x1b[>0q\x1b[>c\x1b[c";
	if(write(1,ask,strlen(ask))<0){}
	std::string in;
	int start=0;
	{
		using namespace std::chrono;
		auto t0=steady_clock::now();
		while(true)
		{
			int left=1500-(int)duration_cast<milliseconds>(steady_clock::now()-t0).count();
			if(left<=0)break;
			pollfd pf;
			pf.fd=0;
			pf.events=POLLIN;
			pf.revents=0;
			if(poll(&pf,1,left)<=0)break;
			char buf[256];
			int n=read(0,buf,sizeof buf);
			if(n<=0)break;
			in.append(buf,n);
			// done once the DA1 answer, ESC [ ? ... c, is in
			size_t d=in.find("\x1b[?",start);
			if((d!=std::string::npos)&&(in.find('c',d)!=std::string::npos))break;
		}
	}
	if(own)tcsetattr(0,TCSANOW,&saved);
	int yes=0;
	std::string rest;
	size_t i=0;
	while(i<in.size())
	{
		if(in[i]!=0x1b)
		{
			rest+=in[i];
			i++;
			continue;
		}
		// a DCS reply runs to ST (ESC \), a CSI reply to its final letter
		if((i+1<in.size())&&(in[i+1]=='P'))
		{
			size_t e=in.find("\x1b\\",i+2);
			if(e==std::string::npos)e=in.size();
			std::string body=in.substr(i+2,e-i-2);
			if((body.compare(0,3,"1$r")==0)&&((body.find("48:2")!=std::string::npos)||(body.find("48;2")!=std::string::npos)))yes=1;
			if(body.compare(0,2,">|")==0)
			{
				const char* names[]={"VTE","kitty","WezTerm","foot","iTerm2","XTerm","Konsole","contour","ghostty","mintty","alacritty","Alacritty"};
				for(const char* nm : names)
					if(body.find(nm)!=std::string::npos)yes=1;
			}
			i=e+2;
			continue;
		}
		if((i+2<in.size())&&(in[i+1]=='[')&&((in[i+2]=='>')||(in[i+2]=='?')))
		{
			size_t e=i+3;
			while((e<in.size())&&((in[e]==';')||((in[e]>='0')&&(in[e]<='9'))))e++;
			if((e<in.size())&&(in[e]=='c'))
			{
				if(in[i+2]=='>')
				{
					int a=0;
					int b=0;
					sscanf(in.c_str()+i+3,"%d;%d",&a,&b);
					if(((a==1)||(a==61)||(a==65))&&(b>=4000))yes=1;
				}
				i=e+1;
				continue;
			}
		}
		rest+=in[i];
		i++;
	}
	// what was typed meanwhile goes back in front of the input
	if(!rest.empty())
	{
		std::string keep=rest+std::string((char*)readBuf+readPos,readHave-readPos);
		if(keep.size()>sizeof(readBuf))keep.resize(sizeof(readBuf));
		memcpy(readBuf,keep.data(),keep.size());
		readPos=0;
		readHave=(int)keep.size();
	}
	return yes;
}

// 1 if the terminal shows 24-bit colours: TV_COLORS=24 or 256 decides, else
// tmux's answer inside tmux, else COLORTERM, else the terminal is asked
// (probeTrueColor), once.
static int term_truecolor()
{
	static int known=-1;
	if(known>=0)return known;
	const char* force=getenv("TV_COLORS");
	if(force&&!strcmp(force,"24"))return known=1;
	if(force&&!strcmp(force,"256"))return known=0;
	// in tmux, COLORTERM may be left over from the terminal tmux was started in
	if(getenv("TMUX"))return known=tmuxTrueColor();
	const char* c=getenv("COLORTERM");
	if(c&&((strcasecmp(c,"truecolor")==0)||(strcasecmp(c,"24bit")==0)))return known=1;
	if(!isatty(0)||!isatty(1))return known=0;
	known=probeTrueColor();
	return known;
}

// ---------------------------------------------------------------- directory listing

struct DirEntry
{
	std::string name;
	int isDir;
	std::string info;
	long long size;
	time_t mtime;
	mode_t mode;      // lstat's mode: a symbolic link keeps S_IFLNK
	int marked;
};

// Sixteen independent listings; fs_slot picks the one the other calls use, so that a
// program can keep several file panels (t2fc) and still use the file dialog in slot 0.
#define FS_SLOTS 16
static std::vector<DirEntry> listings[FS_SLOTS];
static std::string listedDir[FS_SLOTS];
static int sortModes[FS_SLOTS]={0};
static int slot=0;
#define listing listings[slot]

static void copyOut(char* out, int max, const std::string& s)
{
	snprintf(out,max,"%s",s.c_str());
}

static const char* extOf(const std::string& name)
{
	size_t dot=name.rfind('.');
	if(dot==std::string::npos||dot==0)
		return "";
	return name.c_str()+dot+1;
}

// Sort modes: 0 name, 1 extension, 2 size (largest first), 3 time (newest first),
// plus 4 for the reverse order. ".." stays on top and directories come first.
static int sortNow=0;

static bool entryLess(const DirEntry& a, const DirEntry& b)
{
	if(a.name=="..")return b.name!="..";
	if(b.name=="..")return false;
	if(a.isDir!=b.isDir)return a.isDir>b.isDir;
	int key=sortNow&3;
	int c=0;
	if(key==1)
		c=strcasecmp(extOf(a.name),extOf(b.name));
	else if(key==2&&a.size!=b.size)
		c=a.size>b.size?-1:1;
	else if(key==3&&a.mtime!=b.mtime)
		c=a.mtime>b.mtime?-1:1;
	if(!c)
		c=strcasecmp(a.name.c_str(),b.name.c_str());
	if(!c)
		c=a.name<b.name?-1:(a.name==b.name?0:1);
	if(sortNow&4)
		c=-c;
	return c<0;
}

static void sortListing()
{
	sortNow=sortModes[slot];
	std::stable_sort(listing.begin(),listing.end(),entryLess);
}

static std::string describe(const struct stat& st)
{
	char when[32];
	char line[80];
	time_t t=st.st_mtime;
	strftime(when,sizeof(when),"%Y-%m-%d %H:%M",localtime(&t));
	if(S_ISDIR(st.st_mode))
		snprintf(line,sizeof(line),"<DIR>  %s",when);
	else
		snprintf(line,sizeof(line),"%lld bytes  %s",(long long)st.st_size,when);
	return line;
}

// Reads a directory, directories first. Writes its absolute path to resolved and
// returns the entry count, or -1 with the previous listing kept. Listing the same
// directory again keeps the marks of the names still there.
static int fs_list(const char* path, char* resolved, int max, int showHidden)
{
	char full[PATH_MAX];
	if(!realpath(path,full))
		return -1;
	DIR* d=opendir(full);
	if(!d)
		return -1;
	std::vector<std::string> keep;
	if(listedDir[slot]==full)
		for(const DirEntry& e:listing)
			if(e.marked)
				keep.push_back(e.name);
	std::sort(keep.begin(),keep.end());
	std::vector<DirEntry> entries;
	while(dirent* e=readdir(d))
	{
		std::string name=e->d_name;
		if(name==".")
			continue;
		if(name==".."&&!strcmp(full,"/"))
			continue;
		if(name[0]=='.'&&name!=".."&&!showHidden)
			continue;
		DirEntry entry;
		entry.name=name;
		entry.isDir=0;
		entry.size=0;
		entry.mtime=0;
		entry.mode=0;
		entry.marked=std::binary_search(keep.begin(),keep.end(),name)?1:0;
		std::string target=std::string(full)+"/"+name;
		struct stat st;
		struct stat lst;
		if(!lstat(target.c_str(),&lst))
		{
			entry.mode=lst.st_mode;
			entry.size=lst.st_size;
			entry.mtime=lst.st_mtime;
		}
		if(!stat(target.c_str(),&st))
		{
			entry.isDir=S_ISDIR(st.st_mode)?1:0;
			entry.info=describe(st);
			entry.size=st.st_size;
		}
		else
			entry.info="unreadable";
		entries.push_back(entry);
	}
	closedir(d);
	listing.swap(entries);
	listedDir[slot]=full;
	sortListing();
	copyOut(resolved,max,full);
	return (int)listing.size();
}

// Picks the listing (0-15) the other fs_ calls use. Returns the one before.
static int fs_slot(int k)
{
	int old=slot;
	if(k>=0&&k<FS_SLOTS)
		slot=k;
	return old;
}

// Sorts the current listing in a new order, and later listings in that slot too.
static void fs_sort(int mode)
{
	sortModes[slot]=mode&7;
	sortListing();
}

static int validEntry(int i)
{
	return i>=0&&i<(int)listing.size();
}

// One column of an entry: 0 its size (7 columns at most), 1 its time as ls shows it,
// 2 its permissions, 3 its type mark: / directory, @ link, * executable, | pipe,
// = socket, # device, or a space.
static void fs_field(int i, int k, char* out, int max)
{
	out[0]=0;
	if(!validEntry(i))
		return;
	const DirEntry& e=listing[i];
	char buf[64];
	buf[0]=0;
	if(k==0)
	{
		long long v=e.size;
		if(e.name=="..")
			snprintf(buf,sizeof(buf),"UP--DIR");
		else if(e.isDir)
			snprintf(buf,sizeof(buf),"<DIR>");
		else if(v<10000000)
			snprintf(buf,sizeof(buf),"%lld",v);
		else if(v<10000000LL*1024)
			snprintf(buf,sizeof(buf),"%lldK",(v+1023)/1024);
		else if(v<10000000LL*1024*1024)
			snprintf(buf,sizeof(buf),"%lldM",(v+1048575)/1048576);
		else
			snprintf(buf,sizeof(buf),"%lldG",v/1073741824LL);
	}
	else if(k==1)
	{
		time_t now=time(0);
		const char* fmt="%b %e %H:%M";
		if(e.mtime<now-183*86400||e.mtime>now+3600)
			fmt="%b %e  %Y";
		strftime(buf,sizeof(buf),fmt,localtime(&e.mtime));
	}
	else if(k==2)
	{
		mode_t m=e.mode;
		char t='-';
		if(S_ISDIR(m))t='d';
		else if(S_ISLNK(m))t='l';
		else if(S_ISFIFO(m))t='p';
		else if(S_ISSOCK(m))t='s';
		else if(S_ISCHR(m))t='c';
		else if(S_ISBLK(m))t='b';
		const char* r="rwxrwxrwx";
		buf[0]=t;
		for(int b=0;b<9;b++)
			buf[1+b]=(m&(0400>>b))?r[b]:'-';
		buf[10]=0;
	}
	else if(k==3)
	{
		mode_t m=e.mode;
		char c=' ';
		if(S_ISLNK(m))c=e.isDir?'~':'@';
		else if(e.isDir)c='/';
		else if(S_ISFIFO(m))c='|';
		else if(S_ISSOCK(m))c='=';
		else if(S_ISCHR(m)||S_ISBLK(m))c='#';
		else if(m&0111)c='*';
		buf[0]=c;
		buf[1]=0;
	}
	copyOut(out,max,buf);
}

// Marks: v 1 sets, 0 clears, -1 toggles. ".." is never marked. Returns the mark.
static int fs_mark(int i, int v)
{
	if(!validEntry(i))
		return 0;
	DirEntry& e=listing[i];
	if(e.name=="..")
		return 0;
	if(v<0)
		v=!e.marked;
	e.marked=v?1:0;
	return e.marked;
}

static int fs_marked(int i)
{
	if(!validEntry(i))
		return 0;
	return listing[i].marked;
}

// Marks (v 1), unmarks (0) or inverts (2) the entries whose names match a shell
// pattern such as *.cpp; files only unless dirsToo. Returns how many changed.
static int fs_mark_match(int v, const char* pattern, int dirsToo)
{
	int n=0;
	for(DirEntry& e:listing)
	{
		if(e.name=="..")
			continue;
		if(e.isDir&&!dirsToo)
			continue;
		if(pattern[0]&&fnmatch(pattern,e.name.c_str(),FNM_PERIOD))
			continue;
		int m=v==2?!e.marked:(v?1:0);
		if(m!=e.marked)
			n++;
		e.marked=m;
	}
	return n;
}

static std::string bytesText(long long v)
{
	char buf[48];
	if(v<1000000)
		snprintf(buf,sizeof(buf),"%lld B",v);
	else if(v<1000LL*1048576)
		snprintf(buf,sizeof(buf),"%.1f MiB",v/1048576.0);
	else
		snprintf(buf,sizeof(buf),"%.2f GiB",v/1073741824.0);
	return buf;
}

// Describes the marked entries, as "3 files, 12.5 MiB" (directory sizes are not
// counted). Returns how many are marked.
static int fs_mark_info(char* out, int max)
{
	int n=0;
	long long bytes=0;
	for(const DirEntry& e:listing)
		if(e.marked)
		{
			n++;
			if(!e.isDir)
				bytes+=e.size;
		}
	std::string s=bytesText(bytes)+" in "+std::to_string(n)+(n==1?" entry":" entries");
	copyOut(out,max,s);
	return n;
}

// The free and total space of the file system holding path, as "12G/50G (24%)".
static int fs_disk(const char* path, char* out, int max)
{
	out[0]=0;
	struct statvfs v;
	if(statvfs(path,&v))
		return 0;
	double total=(double)v.f_blocks*v.f_frsize;
	double avail=(double)v.f_bavail*v.f_frsize;
	if(total<=0)
		return 0;
	const char* unit[5]={"","K","M","G","T"};
	double t=total;
	double a=avail;
	int u=0;
	while(t>=10000&&u<4)
	{
		t/=1024;
		a/=1024;
		u++;
	}
	char buf[64];
	snprintf(buf,sizeof(buf),"%.0f%s/%.0f%s (%d%%)",a,unit[u],t,unit[u],(int)(avail*100/total));
	copyOut(out,max,buf);
	return 1;
}

// Writes the entry's name. Returns 1 for a directory, 0 for anything else.
static int fs_name(int i, char* out, int max)
{
	out[0]=0;
	if(i<0||i>=(int)listing.size())
		return 0;
	copyOut(out,max,listing[i].name);
	return listing[i].isDir;
}

static void fs_info(int i, char* out, int max)
{
	out[0]=0;
	if(i>=0&&i<(int)listing.size())
		copyOut(out,max,listing[i].info);
}

static int fs_index(const char* name)
{
	for(int i=0;i<(int)listing.size();i++)
		if(listing[i].name==name)
			return i;
	return -1;
}

// The next entry after start whose name begins with this letter, wrapping around.
static int fs_find(int letter, int start)
{
	int n=(int)listing.size();
	for(int k=1;k<=n;k++)
	{
		int i=(start+k)%n;
		unsigned char c=listing[i].name[0];
		if(c<128&&tolower(c)==tolower(letter))
			return i;
	}
	return -1;
}

static void fs_home(char* out, int max)
{
	const char* home=getenv("HOME");
	copyOut(out,max,home?home:"/");
}

// ---------------------------------------------------------------- reading and saving files

static std::string lastError;

static void noteError(const char* what)
{
	lastError=std::string(what)+": "+strerror(errno);
}

static void fs_error(char* out, int max)
{
	copyOut(out,max,lastError);
}

// Writes the absolute path. Returns 1 if the file exists, 0 if only its directory
// does, and -1 if that is missing too.
static int fs_abspath(const char* path, char* out, int max)
{
	char full[PATH_MAX];
	if(realpath(path,full))
	{
		copyOut(out,max,full);
		return 1;
	}
	std::string p=path;
	size_t slash=p.rfind('/');
	std::string dir=".";
	std::string base=p;
	if(slash!=std::string::npos)
	{
		dir=slash?p.substr(0,slash):"/";
		base=p.substr(slash+1);
	}
	if(base.empty()||!realpath(dir.c_str(),full))
		return -1;
	std::string joined=full;
	if(joined!="/")
		joined+="/";
	copyOut(out,max,joined+base);
	return 0;
}

// The size of a regular file, or -1 if it is not one. Saturates, as Scinc has only int.
static int fs_size(const char* path)
{
	struct stat st;
	if(stat(path,&st))
	{
		noteError("Cannot read the file");
		return -1;
	}
	if(!S_ISREG(st.st_mode))
	{
		errno=EINVAL;
		noteError("Not a regular file");
		return -1;
	}
	if(st.st_size>INT_MAX)
		return INT_MAX;
	return (int)st.st_size;
}

// Reads at most max bytes. Returns the count, or -1.
static int fs_load(const char* path, char* buf, int max)
{
	if(fs_size(path)<0)
		return -1;
	FILE* f=fopen(path,"rb");
	if(!f)
	{
		noteError("Cannot open the file");
		return -1;
	}
	size_t n=fread(buf,1,max,f);
	int bad=ferror(f);
	if(bad)
		noteError("Cannot read the file");
	fclose(f);
	return bad?-1:(int)n;
}

// A value that changes when the file does, or -1 if there is no such file.
static int fs_stamp(const char* path)
{
	struct stat st;
	if(stat(path,&st))
		return -1;
	unsigned v=(unsigned)st.st_mtim.tv_sec*1000u+(unsigned)(st.st_mtim.tv_nsec/1000000)+(unsigned)st.st_size*31u;
	return (int)(v&0x7fffffffu);
}

static int fs_isdir(const char* path)
{
	struct stat st;
	if(stat(path,&st))
		return 0;
	return S_ISDIR(st.st_mode)?1:0;
}

// A save goes to a temporary file next to the target and replaces it only once all
// of it is safely on disk, so that a full disk or a crash cannot cost the old file.
static int saveFd=-1;
static std::string saveTmp;
static std::string saveTarget;

static int fs_save_begin(const char* path)
{
	char full[PATH_MAX];
	// a symbolic link stays a link: the file it points to is the one replaced
	saveTarget=realpath(path,full)?full:path;
	struct stat st;
	int exists=!stat(saveTarget.c_str(),&st);
	if(exists&&access(saveTarget.c_str(),W_OK))
	{
		noteError("Cannot write the file");
		return 0;
	}
	mode_t mode;
	if(exists)
		mode=st.st_mode&07777;
	else
	{
		mode_t mask=umask(0);
		umask(mask);
		mode=0666&~mask;
	}
	size_t slash=saveTarget.rfind('/');
	std::string dir=slash==std::string::npos?"":saveTarget.substr(0,slash+1);
	std::string base=slash==std::string::npos?saveTarget:saveTarget.substr(slash+1);
	saveTmp=dir+"."+base+".XXXXXX";
	std::vector<char> name(saveTmp.begin(),saveTmp.end());
	name.push_back(0);
	saveFd=mkstemp(name.data());
	if(saveFd<0)
	{
		noteError("Cannot create a file in this directory");
		return 0;
	}
	saveTmp=name.data();
	if(fchmod(saveFd,mode))
	{
		noteError("Cannot set the file's permissions");
		close(saveFd);
		saveFd=-1;
		unlink(saveTmp.c_str());
		return 0;
	}
	return 1;
}

static int fs_save_write(const char* data, int n)
{
	while(n>0)
	{
		ssize_t w=write(saveFd,data,n);
		if(w<0)
		{
			if(errno==EINTR)
				continue;
			noteError("Cannot write the file");
			return 0;
		}
		data+=w;
		n-=(int)w;
	}
	return 1;
}

// Replaces the target if commit is set, otherwise throws the temporary file away.
static int fs_save_end(int commit)
{
	int ok=commit;
	if(ok&&fsync(saveFd))
	{
		noteError("Cannot write the file");
		ok=0;
	}
	if(close(saveFd)&&ok)
	{
		noteError("Cannot write the file");
		ok=0;
	}
	saveFd=-1;
	if(ok&&rename(saveTmp.c_str(),saveTarget.c_str()))
	{
		noteError("Cannot replace the file");
		ok=0;
	}
	if(!ok)
		unlink(saveTmp.c_str());
	return ok;
}

// ---------------------------------------------------------------- file operations

static int fs_mkdir(const char* path)
{
	if(mkdir(path,0777))
	{
		noteError("Cannot make the directory");
		return 0;
	}
	return 1;
}

// A copy, move or delete runs as a job: fs_job_add names the sources, fs_job_start
// plans the work as a list of actions, and fs_job_step does some of it each call, so
// that the program can show progress and stay responsive. A step stops early to ask
// when an action fails or its target exists; fs_job_answer says what to do.

enum { A_MKDIR, A_COPY, A_LINK, A_UNLINK, A_RMDIR, A_CHMOD, A_RENAME, A_UNLINKOK, A_RMDIRQ };

struct JobAct
{
	int kind;
	std::string src;
	std::string dst;
	long long size;
	mode_t mode;
	time_t mtime;
	int ok;
};

static std::vector<std::string> jobSrc;
static std::vector<JobAct> jobActs;
static size_t jobAt=0;
static int jobOp=0;
static int jobState=0;       // 0 working, -1 an error to answer, -2 a target exists
static int jobOverAll=0;     // 1 overwrite all, 2 skip all existing targets
static int jobSkipErrs=0;
static int jobForce=0;       // the answer was overwrite, for the action now waiting
static int jobIn=-1;
static int jobOut=-1;
static long long jobCopied=0;   // of the file being copied
static long long jobBytes=0;    // of whole files done
static long long jobTotal=0;
static int jobFiles=0;
static int jobFilesDone=0;

static void jobAct(int kind, const std::string& src, const std::string& dst, long long size, mode_t mode, time_t mtime)
{
	JobAct a;
	a.kind=kind;
	a.src=src;
	a.dst=dst;
	a.size=size;
	a.mode=mode;
	a.mtime=mtime;
	a.ok=0;
	jobActs.push_back(a);
	if(kind==A_COPY)
	{
		jobTotal+=size;
		jobFiles++;
	}
	else if(kind!=A_CHMOD&&kind!=A_RMDIRQ&&kind!=A_UNLINKOK)
		jobFiles++;
}

static std::string baseOf(const std::string& p)
{
	std::string s=p;
	while(s.size()>1&&s.back()=='/')
		s.pop_back();
	size_t slash=s.rfind('/');
	return slash==std::string::npos?s:s.substr(slash+1);
}

// The actions that copy src to dst, deleting each source once it is copied if del.
static void planCopy(const std::string& src, const std::string& dst, int del, std::vector<JobAct>* into)
{
	struct stat st;
	std::vector<JobAct> saved;
	if(into)
		saved.swap(jobActs);
	if(lstat(src.c_str(),&st))
		jobAct(A_COPY,src,dst,0,0644,0);   // fails when run, with the reason
	else if(S_ISDIR(st.st_mode))
	{
		jobAct(A_MKDIR,src,dst,0,st.st_mode&07777,st.st_mtime);
		DIR* d=opendir(src.c_str());
		if(d)
		{
			std::vector<std::string> names;
			while(dirent* e=readdir(d))
			{
				std::string n=e->d_name;
				if(n!="."&&n!="..")
					names.push_back(n);
			}
			closedir(d);
			std::sort(names.begin(),names.end());
			for(const std::string& n:names)
				planCopy(src+"/"+n,dst+"/"+n,del,0);
		}
		jobAct(A_CHMOD,src,dst,0,st.st_mode&07777,st.st_mtime);
		if(del)
			jobAct(A_RMDIRQ,src,dst,0,0,0);
	}
	else
	{
		if(S_ISLNK(st.st_mode))
			jobAct(A_LINK,src,dst,0,0,0);
		else
			jobAct(A_COPY,src,dst,st.st_size,st.st_mode&07777,st.st_mtime);
		if(del)
			jobAct(A_UNLINKOK,src,dst,0,0,0);
	}
	if(into)
	{
		into->swap(jobActs);
		jobActs.swap(saved);
	}
}

static void planDelete(const std::string& path)
{
	struct stat st;
	if(!lstat(path.c_str(),&st)&&S_ISDIR(st.st_mode))
	{
		DIR* d=opendir(path.c_str());
		if(d)
		{
			std::vector<std::string> names;
			while(dirent* e=readdir(d))
			{
				std::string n=e->d_name;
				if(n!="."&&n!="..")
					names.push_back(n);
			}
			closedir(d);
			std::sort(names.begin(),names.end());
			for(const std::string& n:names)
				planDelete(path+"/"+n);
		}
		jobAct(A_RMDIR,path,"",0,0,0);
	}
	else
		jobAct(A_UNLINK,path,"",0,0,0);
}

// dst lies inside src, or is src.
static int inside(const std::string& src, const std::string& dst)
{
	char a[PATH_MAX];
	char b[PATH_MAX];
	if(!realpath(src.c_str(),a))
		return 0;
	std::string dir=dst;
	std::string rest;
	// the target need not exist yet: resolve its nearest existing parent
	while(!realpath(dir.c_str(),b))
	{
		size_t slash=dir.rfind('/');
		if(slash==std::string::npos||slash==0)
			return 0;
		rest=dir.substr(slash)+rest;
		dir=dir.substr(0,slash);
	}
	std::string full=std::string(b)+rest;
	std::string s=a;
	return full==s||full.compare(0,s.size()+1,s+"/")==0;
}

static void jobCloseFiles()
{
	if(jobIn>=0)
		close(jobIn);
	if(jobOut>=0)
		close(jobOut);
	jobIn=-1;
	jobOut=-1;
}

static void fs_job_clear()
{
	jobCloseFiles();
	jobSrc.clear();
	jobActs.clear();
	jobAt=0;
	jobState=0;
	jobOverAll=0;
	jobSkipErrs=0;
	jobForce=0;
	jobCopied=0;
	jobBytes=0;
	jobTotal=0;
	jobFiles=0;
	jobFilesDone=0;
}

static void fs_job_add(const char* path)
{
	jobSrc.push_back(path);
}

// op 0 copies, 1 moves, 2 deletes the sources added. A copy or move into an existing
// directory puts each source in it under its own name; one source can also go to a
// new name. Returns 0, with fs_error saying why, if the job cannot start.
static int fs_job_start(int op, const char* dstPath)
{
	jobOp=op;
	jobActs.clear();
	jobAt=0;
	jobState=0;
	if(op==2)
	{
		for(const std::string& s:jobSrc)
			planDelete(s);
		return 1;
	}
	std::string dst=dstPath;
	struct stat st;
	int intoDir=!stat(dst.c_str(),&st)&&S_ISDIR(st.st_mode);
	if(!intoDir&&jobSrc.size()!=1)
	{
		lastError="The target must be an existing directory for more than one source.";
		return 0;
	}
	for(const std::string& s:jobSrc)
	{
		std::string to=intoDir?dst+"/"+baseOf(s):dst;
		struct stat a;
		struct stat b;
		if(!lstat(s.c_str(),&a)&&!lstat(to.c_str(),&b)&&a.st_dev==b.st_dev&&a.st_ino==b.st_ino)
		{
			lastError="Cannot "+std::string(op?"move":"copy")+" \""+baseOf(s)+"\" onto itself.";
			return 0;
		}
		if(!lstat(s.c_str(),&a)&&S_ISDIR(a.st_mode)&&inside(s,to))
		{
			lastError="Cannot "+std::string(op?"move":"copy")+" the directory \""+baseOf(s)+"\" into itself.";
			return 0;
		}
		if(op==1)
		{
			jobAct(A_RENAME,s,to,0,0,0);
			// a move within one file system is a rename; its size is not copied
			if(!lstat(s.c_str(),&a)&&!S_ISDIR(a.st_mode))
			{
				jobActs.back().size=a.st_size;
				jobTotal+=a.st_size;
			}
		}
		else
			planCopy(s,to,0,0);
	}
	return 1;
}

static int jobFail(const char* what)
{
	noteError(what);
	jobCloseFiles();
	jobState=-1;
	return -1;
}

static void jobNext()
{
	JobAct& a=jobActs[jobAt];
	if(a.kind!=A_CHMOD&&a.kind!=A_RMDIRQ&&a.kind!=A_UNLINKOK)
		jobFilesDone++;
	if(a.kind==A_COPY)
		jobBytes+=a.size;
	jobCopied=0;
	jobForce=0;
	jobAt++;
}

// The target exists: ask unless an earlier answer covers it. Returns 1 to go on and
// overwrite, 0 to skip, -2 to ask.
static int jobExists(const std::string& dst)
{
	struct stat st;
	if(lstat(dst.c_str(),&st))
		return 1;
	if(jobForce||jobOverAll==1)
		return 1;
	if(jobOverAll==2)
		return 0;
	jobState=-2;
	return -2;
}

// Skips a directory's actions, its own and those of everything in it.
static void jobSkipDir()
{
	std::string top=jobActs[jobAt].src;
	std::string pre=top+"/";
	size_t end=jobAt+1;
	while(end<jobActs.size()&&(jobActs[end].src==top||jobActs[end].src.compare(0,pre.size(),pre)==0))
		end++;
	while(jobAt<end)
		jobNext();
}

// Does one action, or part of a file copy, and returns 1 when it may go on.
static int jobRun(std::chrono::steady_clock::time_point until)
{
	JobAct& a=jobActs[jobAt];
	struct stat st;
	if(a.kind==A_MKDIR)
	{
		if(!lstat(a.dst.c_str(),&st))
		{
			if(S_ISDIR(st.st_mode))
			{
				a.ok=1;      // merge into the directory there
				jobNext();
				return 1;
			}
			int r=jobExists(a.dst);
			if(r<0)return r;
			if(!r)
			{
				jobSkipDir();
				return 1;
			}
			if(unlink(a.dst.c_str()))
				return jobFail("Cannot replace");
		}
		if(mkdir(a.dst.c_str(),a.mode|0700))
			return jobFail("Cannot make the directory");
		a.ok=1;
		jobNext();
		return 1;
	}
	if(a.kind==A_CHMOD)
	{
		chmod(a.dst.c_str(),a.mode);
		struct timespec ts[2];
		ts[0].tv_sec=a.mtime;
		ts[0].tv_nsec=0;
		ts[1]=ts[0];
		utimensat(AT_FDCWD,a.dst.c_str(),ts,0);
		a.ok=1;
		jobNext();
		return 1;
	}
	if(a.kind==A_LINK)
	{
		char target[PATH_MAX];
		ssize_t n=readlink(a.src.c_str(),target,sizeof(target)-1);
		if(n<0)
			return jobFail("Cannot read the link");
		target[n]=0;
		int r=jobExists(a.dst);
		if(r<0)return r;
		if(!r)
		{
			jobNext();
			return 1;
		}
		if(!lstat(a.dst.c_str(),&st))
		{
			if(S_ISDIR(st.st_mode))
			{
				errno=EISDIR;
				return jobFail("A directory is in the way");
			}
			if(unlink(a.dst.c_str()))
				return jobFail("Cannot replace");
		}
		if(symlink(target,a.dst.c_str()))
			return jobFail("Cannot make the link");
		a.ok=1;
		jobNext();
		return 1;
	}
	if(a.kind==A_COPY)
	{
		if(jobIn<0)
		{
			if(!lstat(a.dst.c_str(),&st)&&S_ISDIR(st.st_mode))
			{
				errno=EISDIR;
				return jobFail("A directory is in the way");
			}
			int r=jobExists(a.dst);
			if(r<0)return r;
			if(!r)
			{
				jobNext();
				return 1;
			}
			if(lstat(a.src.c_str(),&st))
				return jobFail("Cannot read");
			if(!S_ISREG(st.st_mode))
			{
				errno=EINVAL;
				return jobFail("Not a regular file");
			}
			jobIn=open(a.src.c_str(),O_RDONLY);
			if(jobIn<0)
				return jobFail("Cannot open");
			jobOut=open(a.dst.c_str(),O_WRONLY|O_CREAT|O_TRUNC,a.mode|0600);
			if(jobOut<0)
				return jobFail("Cannot create");
			jobCopied=0;
		}
		static char buf[1<<20];
		while(std::chrono::steady_clock::now()<until)
		{
			ssize_t n=read(jobIn,buf,sizeof(buf));
			if(n<0)
			{
				if(errno==EINTR)continue;
				unlink(a.dst.c_str());
				return jobFail("Cannot read");
			}
			if(n==0)
			{
				jobCloseFiles();
				chmod(a.dst.c_str(),a.mode);
				struct timespec ts[2];
				ts[0].tv_sec=a.mtime;
				ts[0].tv_nsec=0;
				ts[1]=ts[0];
				utimensat(AT_FDCWD,a.dst.c_str(),ts,0);
				a.ok=1;
				jobNext();
				return 1;
			}
			char* p=buf;
			while(n>0)
			{
				ssize_t w=write(jobOut,p,n);
				if(w<0)
				{
					if(errno==EINTR)continue;
					int e=errno;
					jobCloseFiles();
					unlink(a.dst.c_str());
					errno=e;
					return jobFail("Cannot write");
				}
				p+=w;
				n-=w;
				jobCopied+=w;
			}
		}
		return 1;
	}
	if(a.kind==A_UNLINKOK)
	{
		// the source goes only if its copy was made
		if(jobAt>0&&jobActs[jobAt-1].ok)
			if(unlink(a.src.c_str()))
				return jobFail("Cannot delete");
		jobNext();
		return 1;
	}
	if(a.kind==A_RMDIRQ)
	{
		rmdir(a.src.c_str());     // stays if something in it was skipped
		jobNext();
		return 1;
	}
	if(a.kind==A_UNLINK)
	{
		if(unlink(a.src.c_str())&&errno!=ENOENT)
			return jobFail("Cannot delete");
		jobNext();
		return 1;
	}
	if(a.kind==A_RMDIR)
	{
		if(rmdir(a.src.c_str())&&errno!=ENOENT)
			return jobFail("Cannot delete the directory");
		jobNext();
		return 1;
	}
	if(a.kind==A_RENAME)
	{
		struct stat s1;
		if(lstat(a.src.c_str(),&s1))
			return jobFail("Cannot read");
		int srcDir=S_ISDIR(s1.st_mode);
		int expand=0;
		if(!lstat(a.dst.c_str(),&st))
		{
			if(srcDir&&S_ISDIR(st.st_mode))
				expand=1;    // merge into the directory there, a file at a time
			else if(S_ISDIR(st.st_mode))
			{
				errno=EISDIR;
				return jobFail("A directory is in the way");
			}
			else
			{
				int r=jobExists(a.dst);
				if(r<0)return r;
				if(!r)
				{
					jobNext();
					return 1;
				}
				if(srcDir&&unlink(a.dst.c_str()))
					return jobFail("Cannot replace");
			}
		}
		if(!expand)
		{
			if(!rename(a.src.c_str(),a.dst.c_str()))
			{
				a.ok=1;
				jobFilesDone++;
				jobBytes+=a.size;
				jobAt++;
				jobForce=0;
				return 1;
			}
			if(errno!=EXDEV)
				return jobFail("Cannot move");
		}
		// another file system, or a merge: copy, then delete what was copied
		std::vector<JobAct> more;
		planCopy(a.src,a.dst,1,&more);
		jobTotal-=a.size;
		jobFiles--;
		jobActs.erase(jobActs.begin()+jobAt);
		// planCopy has counted the new actions in the totals
		jobActs.insert(jobActs.begin()+jobAt,more.begin(),more.end());
		jobForce=jobForce&&!srcDir;
		return 1;
	}
	jobNext();
	return 1;
}

// Works for about ms milliseconds. Returns 1 while there is more to do, 0 when the
// job is done, -1 when an action failed (fs_error says why) and -2 when a target
// exists; both wait for fs_job_answer.
static int fs_job_step(int ms)
{
	if(jobState<0)
		return jobState;
	auto until=std::chrono::steady_clock::now()+std::chrono::milliseconds(ms);
	while(jobAt<jobActs.size())
	{
		int r=jobRun(until);
		if(r<0)
		{
			if(r==-1&&jobSkipErrs)
			{
				jobState=0;
				jobNext();
				continue;
			}
			return r;
		}
		if(std::chrono::steady_clock::now()>=until)
			return jobAt<jobActs.size()?1:0;
	}
	return 0;
}

// The answer to a question: 0 retry or overwrite, 1 skip, 2 overwrite all or skip
// all errors, 3 skip all existing targets, 4 abort (the job ends).
static void fs_job_answer(int k)
{
	int was=jobState;
	jobState=0;
	if(k==4)
	{
		// a file left half copied goes
		if(jobOut>=0&&jobAt<jobActs.size())
		{
			jobCloseFiles();
			unlink(jobActs[jobAt].dst.c_str());
		}
		jobCloseFiles();
		jobAt=jobActs.size();
		return;
	}
	if(was==-2)
	{
		if(k==0)
			jobForce=1;
		else if(k==2)
			jobOverAll=1;
		else if(k==1||k==3)
		{
			if(k==3)
				jobOverAll=2;
			// skipping a directory skips its contents
			if(jobActs[jobAt].kind==A_MKDIR)
				jobSkipDir();
			else
				jobNext();
		}
		return;
	}
	if(k==1||k==2)
	{
		if(k==2)
			jobSkipErrs=1;
		jobNext();
	}
}

// Progress: 0 per mille of the whole job, 1 entries done, 2 entries in all, 3 per mille
// of the file being copied.
static int fs_job_info(int k)
{
	if(k==1)return jobFilesDone;
	if(k==2)return jobFiles;
	if(k==3)
	{
		if(jobAt<jobActs.size()&&jobActs[jobAt].size>0)
			return (int)(jobCopied*1000/jobActs[jobAt].size);
		return 0;
	}
	double whole=(double)jobTotal+jobFiles*4096.0;
	if(whole<=0)
		return 1000;
	double done=(double)(jobBytes+jobCopied)+jobFilesDone*4096.0;
	int v=(int)(done*1000/whole);
	return v>1000?1000:v;
}

// The current action's source (k 0) or target (k 1).
static void fs_job_name(int k, char* out, int max)
{
	out[0]=0;
	if(jobAt<jobActs.size())
		copyOut(out,max,k?jobActs[jobAt].dst:jobActs[jobAt].src);
}

// ---------------------------------------------------------------- running programs

// Closes every descriptor above stderr, so that a started program does not hold on
// to the interpreter's files, sockets or display connection.
static void closeExtra()
{
	for(int fd=3;fd<1024;fd++)
		close(fd);
}

// Runs a shell command in dir on this terminal and waits for it, then for a key.
// The program calls it with the terminal given back (TvSuspend), so the command
// reads and writes the real terminal. Ctrl+C and Ctrl+\ reach the command only.
// Returns its exit status, 128+n if signal n ended it, or -1 if it could not start.
static int run_here(const char* cmd, const char* dir)
{
	printf("\n%s$ %s\n",dir,cmd);
	fflush(stdout);
	struct sigaction ign;
	struct sigaction oldInt;
	struct sigaction oldQuit;
	struct sigaction oldTstp;
	memset(&ign,0,sizeof(ign));
	ign.sa_handler=SIG_IGN;
	sigaction(SIGINT,&ign,&oldInt);
	sigaction(SIGQUIT,&ign,&oldQuit);
	sigaction(SIGTSTP,&ign,&oldTstp);
	int status=-1;
	pid_t pid=fork();
	if(pid==0)
	{
		signal(SIGINT,SIG_DFL);
		signal(SIGQUIT,SIG_DFL);
		signal(SIGTSTP,SIG_DFL);
		signal(SIGTERM,SIG_DFL);
		signal(SIGHUP,SIG_DFL);
		closeExtra();
		if(chdir(dir))
			_exit(126);
		execl("/bin/sh","sh","-c",cmd,(char*)0);
		_exit(127);
	}
	if(pid<0)
		noteError("Cannot start the command");
	else
	{
		int st=0;
		while(1)
		{
			pid_t r=waitpid(pid,&st,WUNTRACED);
			if(r<0&&errno==EINTR)
				continue;
			if(r<0)
				break;
			if(WIFSTOPPED(st))
			{
				// Ctrl+Z: there is no shell here to resume it later, so go on
				kill(pid,SIGCONT);
				continue;
			}
			if(WIFEXITED(st))
				status=WEXITSTATUS(st);
			else if(WIFSIGNALED(st))
				status=128+WTERMSIG(st);
			break;
		}
	}
	sigaction(SIGINT,&oldInt,0);
	sigaction(SIGQUIT,&oldQuit,0);
	sigaction(SIGTSTP,&oldTstp,0);
	if(status>=0)
		printf("\n[exit code %d] Press any key to return to t2fc",status);
	else
		printf("\n[%s] Press any key to return to t2fc",lastError.c_str());
	fflush(stdout);
	termios now;
	if(!tcgetattr(0,&now))
	{
		termios one=now;
		one.c_lflag&=~(ICANON|ECHO);
		one.c_cc[VMIN]=1;
		one.c_cc[VTIME]=0;
		tcflush(0,TCIFLUSH);
		tcsetattr(0,TCSANOW,&one);
		unsigned char c;
		ssize_t ignored=read(0,&c,1);
		(void)ignored;
		tcsetattr(0,TCSANOW,&now);
	}
	printf("\n");
	fflush(stdout);
	return status;
}

// Starts a shell command in dir and does not wait: it runs in a session of its own,
// with no terminal and its output thrown away, and outlives the program. Returns 1
// if it was started.
static int run_detached(const char* cmd, const char* dir)
{
	pid_t pid=fork();
	if(pid<0)
	{
		noteError("Cannot start the command");
		return 0;
	}
	if(pid==0)
	{
		// the grandchild runs the command; init reaps it, so no zombie is left
		setsid();
		if(fork()!=0)
			_exit(0);
		int null=open("/dev/null",O_RDWR);
		if(null>=0)
		{
			dup2(null,0);
			dup2(null,1);
			dup2(null,2);
		}
		closeExtra();
		signal(SIGINT,SIG_DFL);
		signal(SIGQUIT,SIG_DFL);
		signal(SIGTERM,SIG_DFL);
		signal(SIGHUP,SIG_DFL);
		if(chdir(dir))
			_exit(126);
		execl("/bin/sh","sh","-c",cmd,(char*)0);
		_exit(127);
	}
	int st=0;
	while(waitpid(pid,&st,0)<0&&errno==EINTR)
		;
	return 1;
}

// 1 if there is a graphic desktop to open windows on.
static int run_has_display()
{
	const char* x=getenv("DISPLAY");
	const char* w=getenv("WAYLAND_DISPLAY");
	return (x&&*x)||(w&&*w);
}

static int inPath(const std::string& prog)
{
	const char* path=getenv("PATH");
	if(!path)
		return 0;
	std::string p=path;
	size_t start=0;
	while(start<=p.size())
	{
		size_t end=p.find(':',start);
		if(end==std::string::npos)
			end=p.size();
		std::string dir=p.substr(start,end-start);
		if(dir.empty())
			dir=".";
		if(!access((dir+"/"+prog).c_str(),X_OK))
			return 1;
		start=end+1;
	}
	return 0;
}

// The start of a command that opens a terminal window and runs the rest of it, as
// "xterm -e": $TERMINAL first, then the usual terminal programs. Returns 0, with
// out empty, if none is found.
static int run_terminal(char* out, int max)
{
	out[0]=0;
	const char* t=getenv("TERMINAL");
	if(t&&*t&&inPath(t))
	{
		copyOut(out,max,std::string(t)+" -e");
		return 1;
	}
	static const char* known[][2]={
		{"x-terminal-emulator","-e"},{"gnome-terminal","--"},{"konsole","-e"},
		{"xfce4-terminal","-x"},{"mate-terminal","-x"},{"kitty",""},{"alacritty","-e"},
		{"foot",""},{"wezterm","start --"},{"xterm","-e"}};
	for(auto& k:known)
		if(inPath(k[0]))
		{
			std::string s=k[0];
			if(k[1][0])
				s+=std::string(" ")+k[1];
			copyOut(out,max,s);
			return 1;
		}
	return 0;
}

// Quotes s for the shell, unless it is made of characters that need no quoting.
static void run_quote(const char* s, char* out, int max)
{
	std::string in=s;
	int plain=!in.empty();
	for(unsigned char c:in)
		if(!isalnum(c)&&!strchr("._/+-,:=@%",c))
			plain=0;
	if(plain)
	{
		copyOut(out,max,in);
		return;
	}
	std::string q="'";
	for(char c:in)
	{
		if(c=='\'')
			q+="'\\''";
		else
			q+=c;
	}
	q+="'";
	copyOut(out,max,q);
}

// ---------------------------------------------------------------- the desktop's clipboard

// Runs argv[0] from PATH with in (n bytes) on its stdin, and collects its stdout in
// out if out is given, for at most ms milliseconds. Returns its exit status, or -1
// if it could not run or took too long. A clipboard program that stays behind to
// serve the text (xclip, wl-copy) gets /dev/null for its output, so it never holds
// the pipe or the terminal.
static int clipRun(const char* const* argv, const char* in, int n, std::string* out, int ms)
{
	int inP[2];
	int outP[2];
	if(pipe(inP))
		return -1;
	if(pipe(outP))
	{
		close(inP[0]);
		close(inP[1]);
		return -1;
	}
	pid_t pid=fork();
	if(pid<0)
	{
		close(inP[0]);
		close(inP[1]);
		close(outP[0]);
		close(outP[1]);
		return -1;
	}
	if(pid==0)
	{
		int null=open("/dev/null",O_RDWR);
		dup2(inP[0],0);
		if(out)
			dup2(outP[1],1);
		else if(null>=0)
			dup2(null,1);
		if(null>=0)
			dup2(null,2);
		closeExtra();
		setsid();
		signal(SIGPIPE,SIG_DFL);
		execvp(argv[0],(char* const*)argv);
		_exit(127);
	}
	close(inP[0]);
	close(outP[1]);
	struct sigaction ign;
	struct sigaction oldPipe;
	memset(&ign,0,sizeof(ign));
	ign.sa_handler=SIG_IGN;
	sigaction(SIGPIPE,&ign,&oldPipe);
	fcntl(inP[1],F_SETFL,O_NONBLOCK);
	int wfd=inP[1];
	int rfd=outP[0];
	if(n<=0)
	{
		close(wfd);
		wfd=-1;
	}
	int sent=0;
	auto start=std::chrono::steady_clock::now();
	int timedOut=0;
	while(wfd>=0||rfd>=0)
	{
		int left=ms-(int)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();
		if(left<=0)
		{
			timedOut=1;
			break;
		}
		pollfd p[2];
		int np=0;
		if(wfd>=0)
		{
			p[np].fd=wfd;
			p[np].events=POLLOUT;
			p[np].revents=0;
			np++;
		}
		if(rfd>=0)
		{
			p[np].fd=rfd;
			p[np].events=POLLIN;
			p[np].revents=0;
			np++;
		}
		int r=poll(p,np,left);
		if(r<0&&errno==EINTR)
			continue;
		if(r<=0)
		{
			timedOut=1;
			break;
		}
		for(int i=0;i<np;i++)
		{
			if(!p[i].revents)
				continue;
			if(p[i].fd==wfd)
			{
				ssize_t w=write(wfd,in+sent,n-sent);
				if(w>0)
					sent+=w;
				if((w<0&&errno!=EAGAIN&&errno!=EINTR)||sent>=n)
				{
					close(wfd);
					wfd=-1;
				}
			}
			else
			{
				char buf[65536];
				ssize_t got=read(rfd,buf,sizeof(buf));
				if(got>0)
				{
					if(out)
						out->append(buf,got);
				}
				else if(got==0||(errno!=EAGAIN&&errno!=EINTR))
				{
					close(rfd);
					rfd=-1;
				}
			}
		}
	}
	if(wfd>=0)
		close(wfd);
	if(rfd>=0)
		close(rfd);
	sigaction(SIGPIPE,&oldPipe,0);
	int st=0;
	if(timedOut)
	{
		kill(pid,SIGKILL);
		while(waitpid(pid,&st,0)<0&&errno==EINTR)
			;
		return -1;
	}
	// the program has closed its output; it may still be finishing
	for(int i=0;i<200;i++)
	{
		pid_t r=waitpid(pid,&st,WNOHANG);
		if(r==pid)
			return WIFEXITED(st)?WEXITSTATUS(st):-1;
		if(r<0&&errno!=EINTR)
			return -1;
		usleep(5000);
	}
	kill(pid,SIGKILL);
	while(waitpid(pid,&st,0)<0&&errno==EINTR)
		;
	return -1;
}

static int inPath(const std::string& prog);

// Which clipboard program this desktop has: 1 wl-copy/wl-paste (Wayland), 2 xclip,
// 3 xsel (X11), 0 none.
static int clipTool()
{
	const char* w=getenv("WAYLAND_DISPLAY");
	const char* x=getenv("DISPLAY");
	if(w&&*w&&inPath("wl-copy")&&inPath("wl-paste"))
		return 1;
	if(x&&*x)
	{
		if(inPath("xclip"))
			return 2;
		if(inPath("xsel"))
			return 3;
	}
	return 0;
}

static const char b64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Puts n bytes of s on the desktop's clipboard. Returns 1 if a clipboard program
// took it, 2 if it went to the terminal as an OSC 52 sequence (over ssh, in tmux
// with set-clipboard on, and in terminals that allow it), 0 if neither.
static int clip_set(const char* s, int n)
{
	int tool=clipTool();
	int r=-1;
	if(tool==1)
	{
		const char* a[]={"wl-copy",0};
		r=clipRun(a,s,n,0,3000);
	}
	else if(tool==2)
	{
		const char* a[]={"xclip","-selection","clipboard","-i",0};
		r=clipRun(a,s,n,0,3000);
	}
	else if(tool==3)
	{
		const char* a[]={"xsel","-b","-i",0};
		r=clipRun(a,s,n,0,3000);
	}
	if(r==0)
		return 1;
	if(!rawActive||n>100000)
	{
		noteError("No clipboard program (wl-copy, xclip or xsel) could take the text");
		return 0;
	}
	std::string seq="\x1b]52;c;";
	for(int i=0;i<n;i+=3)
	{
		unsigned v=(unsigned char)s[i]<<16;
		if(i+1<n)
			v|=(unsigned char)s[i+1]<<8;
		if(i+2<n)
			v|=(unsigned char)s[i+2];
		seq+=b64[(v>>18)&63];
		seq+=b64[(v>>12)&63];
		seq+=(i+1<n)?b64[(v>>6)&63]:'=';
		seq+=(i+2<n)?b64[v&63]:'=';
	}
	seq+="\x07";
	fflush(stdout);
	size_t at=0;
	while(at<seq.size())
	{
		ssize_t w=write(1,seq.data()+at,seq.size()-at);
		if(w<0&&errno==EINTR)
			continue;
		if(w<=0)
			break;
		at+=w;
	}
	return 2;
}

// Reads the desktop's clipboard into out (up to max bytes), with CR LF made LF.
// Returns its length, -1 if there is no clipboard program to ask or it failed (as
// when the clipboard is empty), or -2 if the text was longer than max (out then
// holds its start).
static int clip_get(char* out, int max)
{
	int tool=clipTool();
	std::string got;
	int r=-1;
	if(tool==1)
	{
		const char* a[]={"wl-paste","-n",0};
		r=clipRun(a,0,0,&got,3000);
	}
	else if(tool==2)
	{
		const char* a[]={"xclip","-selection","clipboard","-o",0};
		r=clipRun(a,0,0,&got,3000);
	}
	else if(tool==3)
	{
		const char* a[]={"xsel","-b","-o",0};
		r=clipRun(a,0,0,&got,3000);
	}
	if(r!=0)
	{
		noteError("The desktop's clipboard could not be read");
		return -1;
	}
	int o=0;
	for(size_t i=0;i<got.size();i++)
	{
		if(got[i]=='\r'&&i+1<got.size()&&got[i+1]=='\n')
			continue;
		if(o>=max)
			return -2;
		out[o++]=got[i];
	}
	return o;
}

// ---------------------------------------------------------------- counting what a directory holds

// A scan walks a directory tree on a thread of its own: files, subdirectories, other
// entries and the bytes of the files, staying on the directory's file system (as
// du -x) and not following symbolic links. A file with several hard links counts
// once. It stops at its time limit, or when the slot is given another directory;
// a scan stuck on a slow disk is simply left behind.
struct DirScan
{
	std::atomic<int> cancel{0};
	std::atomic<int> state{1};   // 1 running, 2 done, 3 stopped at the time limit, 4 error
	std::atomic<long long> files{0};
	std::atomic<long long> dirs{0};
	std::atomic<long long> other{0};
	std::atomic<long long> bytes{0};
	std::atomic<long long> denied{0};   // directories that could not be read
	std::atomic<int> topFiles{0};
	std::atomic<int> topDirs{0};
	std::atomic<int> topOther{0};
	std::atomic<int> topDone{0};
	std::atomic<int> ms{0};
};

#define SCAN_SLOTS 16
static std::shared_ptr<DirScan> scans[SCAN_SLOTS];

static void scanRun(std::shared_ptr<DirScan> sc, std::string root, int limitMs)
{
	auto start=std::chrono::steady_clock::now();
	auto elapsed=[&](){return (int)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();};
	struct stat rs;
	if(lstat(root.c_str(),&rs)||!S_ISDIR(rs.st_mode))
	{
		sc->state=4;
		return;
	}
	dev_t dev=rs.st_dev;
	std::unordered_set<unsigned long long> seen;   // inodes with several links
	std::vector<std::string> todo;
	todo.push_back(root);
	int top=1;
	int count=0;
	while(!todo.empty())
	{
		std::string dir=todo.back();
		todo.pop_back();
		DIR* d=opendir(dir.c_str());
		if(!d)
		{
			if(top)
			{
				sc->state=4;
				return;
			}
			sc->denied++;
			continue;
		}
		int fd=dirfd(d);
		while(dirent* e=readdir(d))
		{
			if(sc->cancel)
				break;
			if(!strcmp(e->d_name,".")||!strcmp(e->d_name,".."))
				continue;
			struct stat st;
			if(fstatat(fd,e->d_name,&st,AT_SYMLINK_NOFOLLOW))
				continue;
			if(S_ISDIR(st.st_mode))
			{
				sc->dirs++;
				if(top)
					sc->topDirs++;
				if(st.st_dev==dev)
				{
					std::string sub=dir;
					if(sub.empty()||sub.back()!='/')
						sub+='/';
					todo.push_back(sub+e->d_name);
				}
			}
			else if(S_ISREG(st.st_mode))
			{
				sc->files++;
				if(top)
					sc->topFiles++;
				int count1=1;
				if(st.st_nlink>1)
				{
					unsigned long long key=((unsigned long long)st.st_dev<<40)^(unsigned long long)st.st_ino;
					if(!seen.insert(key).second)
						count1=0;
				}
				if(count1)
					sc->bytes+=st.st_size;
			}
			else
			{
				sc->other++;
				if(top)
					sc->topOther++;
			}
			if(((++count)&255)==0)
			{
				int now=elapsed();
				sc->ms=now;
				if(now>limitMs)
					sc->cancel=2;
			}
		}
		closedir(d);
		if(top)
			sc->topDone=1;
		top=0;
		if(sc->cancel)
			break;
		int now=elapsed();
		sc->ms=now;
		if(now>limitMs)
			sc->cancel=2;
		if(sc->cancel)
			break;
	}
	sc->ms=elapsed();
	sc->state=(sc->cancel==2)?3:(sc->cancel?3:2);
}

// Starts counting what path holds in slot (0..15), for at most limitMs, dropping
// any scan the slot had. Returns 1 if it started.
static int dir_scan(int slot, const char* path, int limitMs)
{
	if(slot<0||slot>=SCAN_SLOTS)
		return 0;
	if(scans[slot])
		scans[slot]->cancel=1;
	auto sc=std::make_shared<DirScan>();
	scans[slot]=sc;
	try
	{
		std::thread(scanRun,sc,std::string(path),limitMs).detach();
	}
	catch(...)
	{
		sc->state=4;
		return 0;
	}
	return 1;
}

static void dir_scan_stop(int slot)
{
	if(slot<0||slot>=SCAN_SLOTS||!scans[slot])
		return;
	scans[slot]->cancel=1;
	scans[slot].reset();
}

static int sat(long long v)
{
	return v>INT_MAX?INT_MAX:(int)v;
}

// What the scan in slot found so far: k 0 its state (0 none, 1 running, 2 done,
// 3 stopped at the time limit, 4 the directory could not be read), 1 files,
// 2 directories, 3 other entries, 4 KiB in the files (rounded up), 5 milliseconds
// spent, 6 directories that could not be read, 7 8 9 the files, directories and
// other entries directly in it, 10 whether those three are complete.
static int dir_scan_info(int slot, int k)
{
	if(slot<0||slot>=SCAN_SLOTS||!scans[slot])
		return 0;
	DirScan& s=*scans[slot];
	switch(k)
	{
	case 0: return s.state;
	case 1: return sat(s.files);
	case 2: return sat(s.dirs);
	case 3: return sat(s.other);
	case 4: return sat((s.bytes+1023)/1024);
	case 5: return s.ms;
	case 6: return sat(s.denied);
	case 7: return s.topFiles;
	case 8: return s.topDirs;
	case 9: return s.topOther;
	case 10: return s.topDone;
	}
	return 0;
}

// The bytes in the files, written out ("123456789"), as int cannot hold them all.
static void dir_scan_bytes(int slot, char* out, int max)
{
	long long b=0;
	if(slot>=0&&slot<SCAN_SLOTS&&scans[slot])
		b=scans[slot]->bytes;
	copyOut(out,max,std::to_string(b));
}

extern "C" int term_open_wrapper(ScincVM* s, void*){ScincBoundCall(s,term_open);return 0;}
extern "C" int term_close_wrapper(ScincVM* s, void*){ScincBoundCall(s,term_close);return 0;}
extern "C" int term_read_wrapper(ScincVM* s, void*){ScincBoundCall(s,term_read);return 0;}
extern "C" int term_cols_wrapper(ScincVM* s, void*){ScincBoundCall(s,term_cols);return 0;}
extern "C" int term_rows_wrapper(ScincVM* s, void*){ScincBoundCall(s,term_rows);return 0;}
extern "C" int term_time_wrapper(ScincVM* s, void*){ScincBoundCall(s,term_time);return 0;}
extern "C" int term_ms_wrapper(ScincVM* s, void*){ScincBoundCall(s,term_ms);return 0;}
extern "C" int term_pending_wrapper(ScincVM* s, void*){ScincBoundCall(s,term_pending);return 0;}
extern "C" int term_truecolor_wrapper(ScincVM* s, void*){ScincBoundCall(s,term_truecolor);return 0;}
extern "C" int fs_error_wrapper(ScincVM* s, void*){ScincBoundCall(s,fs_error);return 0;}
extern "C" int fs_abspath_wrapper(ScincVM* s, void*){ScincBoundCall(s,fs_abspath);return 0;}
extern "C" int fs_size_wrapper(ScincVM* s, void*){ScincBoundCall(s,fs_size);return 0;}
extern "C" int fs_load_wrapper(ScincVM* s, void*){ScincBoundCall(s,fs_load);return 0;}
extern "C" int fs_stamp_wrapper(ScincVM* s, void*){ScincBoundCall(s,fs_stamp);return 0;}
extern "C" int fs_isdir_wrapper(ScincVM* s, void*){ScincBoundCall(s,fs_isdir);return 0;}
extern "C" int fs_save_begin_wrapper(ScincVM* s, void*){ScincBoundCall(s,fs_save_begin);return 0;}
extern "C" int fs_save_write_wrapper(ScincVM* s, void*){ScincBoundCall(s,fs_save_write);return 0;}
extern "C" int fs_save_end_wrapper(ScincVM* s, void*){ScincBoundCall(s,fs_save_end);return 0;}
extern "C" int fs_list_wrapper(ScincVM* s, void*){ScincBoundCall(s,fs_list);return 0;}
extern "C" int fs_name_wrapper(ScincVM* s, void*){ScincBoundCall(s,fs_name);return 0;}
extern "C" int fs_info_wrapper(ScincVM* s, void*){ScincBoundCall(s,fs_info);return 0;}
extern "C" int fs_index_wrapper(ScincVM* s, void*){ScincBoundCall(s,fs_index);return 0;}
extern "C" int fs_find_wrapper(ScincVM* s, void*){ScincBoundCall(s,fs_find);return 0;}
extern "C" int fs_home_wrapper(ScincVM* s, void*){ScincBoundCall(s,fs_home);return 0;}
extern "C" int fs_slot_wrapper(ScincVM* s, void*){ScincBoundCall(s,fs_slot);return 0;}
extern "C" int fs_sort_wrapper(ScincVM* s, void*){ScincBoundCall(s,fs_sort);return 0;}
extern "C" int fs_field_wrapper(ScincVM* s, void*){ScincBoundCall(s,fs_field);return 0;}
extern "C" int fs_mark_wrapper(ScincVM* s, void*){ScincBoundCall(s,fs_mark);return 0;}
extern "C" int fs_marked_wrapper(ScincVM* s, void*){ScincBoundCall(s,fs_marked);return 0;}
extern "C" int fs_mark_match_wrapper(ScincVM* s, void*){ScincBoundCall(s,fs_mark_match);return 0;}
extern "C" int fs_mark_info_wrapper(ScincVM* s, void*){ScincBoundCall(s,fs_mark_info);return 0;}
extern "C" int fs_disk_wrapper(ScincVM* s, void*){ScincBoundCall(s,fs_disk);return 0;}
extern "C" int fs_mkdir_wrapper(ScincVM* s, void*){ScincBoundCall(s,fs_mkdir);return 0;}
extern "C" int fs_job_clear_wrapper(ScincVM* s, void*){ScincBoundCall(s,fs_job_clear);return 0;}
extern "C" int fs_job_add_wrapper(ScincVM* s, void*){ScincBoundCall(s,fs_job_add);return 0;}
extern "C" int fs_job_start_wrapper(ScincVM* s, void*){ScincBoundCall(s,fs_job_start);return 0;}
extern "C" int fs_job_step_wrapper(ScincVM* s, void*){ScincBoundCall(s,fs_job_step);return 0;}
extern "C" int fs_job_answer_wrapper(ScincVM* s, void*){ScincBoundCall(s,fs_job_answer);return 0;}
extern "C" int fs_job_info_wrapper(ScincVM* s, void*){ScincBoundCall(s,fs_job_info);return 0;}
extern "C" int fs_job_name_wrapper(ScincVM* s, void*){ScincBoundCall(s,fs_job_name);return 0;}
extern "C" int run_here_wrapper(ScincVM* s, void*){ScincBoundCall(s,run_here);return 0;}
extern "C" int run_detached_wrapper(ScincVM* s, void*){ScincBoundCall(s,run_detached);return 0;}
extern "C" int run_has_display_wrapper(ScincVM* s, void*){ScincBoundCall(s,run_has_display);return 0;}
extern "C" int run_terminal_wrapper(ScincVM* s, void*){ScincBoundCall(s,run_terminal);return 0;}
extern "C" int run_quote_wrapper(ScincVM* s, void*){ScincBoundCall(s,run_quote);return 0;}
extern "C" int clip_set_wrapper(ScincVM* s, void*){ScincBoundCall(s,clip_set);return 0;}
extern "C" int clip_get_wrapper(ScincVM* s, void*){ScincBoundCall(s,clip_get);return 0;}
extern "C" int dir_scan_wrapper(ScincVM* s, void*){ScincBoundCall(s,dir_scan);return 0;}
extern "C" int dir_scan_stop_wrapper(ScincVM* s, void*){ScincBoundCall(s,dir_scan_stop);return 0;}
extern "C" int dir_scan_info_wrapper(ScincVM* s, void*){ScincBoundCall(s,dir_scan_info);return 0;}
extern "C" int dir_scan_bytes_wrapper(ScincVM* s, void*){ScincBoundCall(s,dir_scan_bytes);return 0;}
