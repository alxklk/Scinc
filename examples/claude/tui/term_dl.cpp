// The services Scinc has no intrinsics for: raw mode, reading a key with a timeout,
// the terminal size, the local time, listing directories and saving files safely. All drawing is plain
// ANSI output from the Scinc side.
#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <csignal>
#include <cstdlib>
#include <string>
#include <vector>
#include <dirent.h>
#include <fcntl.h>
#include <poll.h>
#include <strings.h>
#include <sys/stat.h>
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
	static const char leave[]="\x1b[?2004l\x1b[?1006l\x1b[?1002l\x1b[?1000l\x1b[?7h\x1b[?25h\x1b[0m\x1b[?1049l";
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

// ---------------------------------------------------------------- directory listing

struct DirEntry
{
	std::string name;
	int isDir;
	std::string info;
};

static std::vector<DirEntry> listing;

static void copyOut(char* out, int max, const std::string& s)
{
	snprintf(out,max,"%s",s.c_str());
}

static bool entryLess(const DirEntry& a, const DirEntry& b)
{
	if(a.name=="..")return b.name!="..";
	if(b.name=="..")return false;
	if(a.isDir!=b.isDir)return a.isDir>b.isDir;
	int c=strcasecmp(a.name.c_str(),b.name.c_str());
	if(c)return c<0;
	return a.name<b.name;
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
// returns the entry count, or -1 with the previous listing kept.
static int fs_list(const char* path, char* resolved, int max, int showHidden)
{
	char full[PATH_MAX];
	if(!realpath(path,full))
		return -1;
	DIR* d=opendir(full);
	if(!d)
		return -1;
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
		std::string target=std::string(full)+"/"+name;
		struct stat st;
		if(!stat(target.c_str(),&st))
		{
			entry.isDir=S_ISDIR(st.st_mode)?1:0;
			entry.info=describe(st);
		}
		else
			entry.info="unreadable";
		entries.push_back(entry);
	}
	closedir(d);
	std::sort(entries.begin(),entries.end(),entryLess);
	listing.swap(entries);
	copyOut(resolved,max,full);
	return (int)listing.size();
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

extern "C" int term_open_wrapper(ScincVM* s, void*){ScincBoundCall(s,term_open);return 0;}
extern "C" int term_close_wrapper(ScincVM* s, void*){ScincBoundCall(s,term_close);return 0;}
extern "C" int term_read_wrapper(ScincVM* s, void*){ScincBoundCall(s,term_read);return 0;}
extern "C" int term_cols_wrapper(ScincVM* s, void*){ScincBoundCall(s,term_cols);return 0;}
extern "C" int term_rows_wrapper(ScincVM* s, void*){ScincBoundCall(s,term_rows);return 0;}
extern "C" int term_time_wrapper(ScincVM* s, void*){ScincBoundCall(s,term_time);return 0;}
extern "C" int term_pending_wrapper(ScincVM* s, void*){ScincBoundCall(s,term_pending);return 0;}
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
