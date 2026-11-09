// Archives for t2fc, as read-only folders: zip, tar (plain, gz, bz2, xz, zstd, lz),
// 7z, rar, iso, cpio, deb, rpm, cab and what else libarchive reads. libarchive is
// opened at run time (libarchive.so.13), so this library needs nothing else to build.
//
// A path that runs through an archive file, "/home/me/a.zip/docs/x.txt", names
// something in it. The calls here mirror term_dl's: a listing in each of 16 slots
// with its fields and marks (arc_list ... arc_mark_info), what a path is
// (arc_where, arc_isdir, arc_size, arc_load, arc_stamp), a file taken out to a
// temporary file for the programs that need a real one (arc_temp), the whole tree of
// an archive for the archive window (arc_tree, arc_row), and a copy job that takes
// files and directories out, a step at a time (arc_job_*), answered as fs_job is.
#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <chrono>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include <dlfcn.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <ftw.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../../../include/ScincVM.h"
#include "../../../include/ScincBinder.h"

// ---------------------------------------------------------------- libarchive

struct archive;
struct archive_entry;
#define AR_OK 0
#define AR_EOF 1
#define AR_WARN (-20)

static archive* (*pReadNew)();
static int (*pFilterAll)(archive*);
static int (*pFormatAll)(archive*);
static int (*pOpenFile)(archive*,const char*,size_t);
static int (*pNextHeader)(archive*,archive_entry**);
static long (*pReadData)(archive*,void*,size_t);
static int (*pReadFree)(archive*);
static const char* (*pErrorString)(archive*);
static const char* (*pPathname)(archive_entry*);
static const char* (*pPathnameUtf8)(archive_entry*);
static long long (*pSize)(archive_entry*);
static time_t (*pMtime)(archive_entry*);
static unsigned (*pFiletype)(archive_entry*);
static unsigned (*pPerm)(archive_entry*);
static const char* (*pSymlink)(archive_entry*);
static int libState=0;

static std::string lastError;

static bool libLoad()
{
	if(libState)return libState>0;
	libState=-1;
	void* h=dlopen("libarchive.so.13",RTLD_NOW|RTLD_LOCAL);
	if(!h)h=dlopen("libarchive.so",RTLD_NOW|RTLD_LOCAL);
	if(!h)return false;
	pReadNew=(archive*(*)())dlsym(h,"archive_read_new");
	pFilterAll=(int(*)(archive*))dlsym(h,"archive_read_support_filter_all");
	pFormatAll=(int(*)(archive*))dlsym(h,"archive_read_support_format_all");
	pOpenFile=(int(*)(archive*,const char*,size_t))dlsym(h,"archive_read_open_filename");
	pNextHeader=(int(*)(archive*,archive_entry**))dlsym(h,"archive_read_next_header");
	pReadData=(long(*)(archive*,void*,size_t))dlsym(h,"archive_read_data");
	pReadFree=(int(*)(archive*))dlsym(h,"archive_read_free");
	pErrorString=(const char*(*)(archive*))dlsym(h,"archive_error_string");
	pPathname=(const char*(*)(archive_entry*))dlsym(h,"archive_entry_pathname");
	pPathnameUtf8=(const char*(*)(archive_entry*))dlsym(h,"archive_entry_pathname_utf8");
	pSize=(long long(*)(archive_entry*))dlsym(h,"archive_entry_size");
	pMtime=(time_t(*)(archive_entry*))dlsym(h,"archive_entry_mtime");
	pFiletype=(unsigned(*)(archive_entry*))dlsym(h,"archive_entry_filetype");
	pPerm=(unsigned(*)(archive_entry*))dlsym(h,"archive_entry_perm");
	pSymlink=(const char*(*)(archive_entry*))dlsym(h,"archive_entry_symlink");
	if(!pReadNew||!pFilterAll||!pFormatAll||!pOpenFile||!pNextHeader||!pReadData||!pReadFree||
		!pErrorString||!pPathname||!pSize||!pMtime||!pFiletype||!pPerm||!pSymlink)
		return false;
	libState=1;
	return true;
}

static archive* openArchive(const std::string& file)
{
	if(!libLoad())
	{
		lastError="Archives need libarchive (libarchive.so.13), which this machine does not have.";
		return 0;
	}
	archive* a=pReadNew();
	pFilterAll(a);
	pFormatAll(a);
	if(pOpenFile(a,file.c_str(),65536)!=AR_OK)
	{
		const char* e=pErrorString(a);
		lastError=std::string("Cannot read the archive: ")+(e?e:"unknown format");
		pReadFree(a);
		return 0;
	}
	return a;
}

// ---------------------------------------------------------------- paths

static const char* arcExts[]={".zip",".jar",".war",".apk",".xpi",".tar",".tgz",".tbz",".tbz2",
	".txz",".tzst",".tlz",".tar.gz",".tar.bz2",".tar.xz",".tar.zst",".tar.lz",".tar.lzma",
	".tar.z",".7z",".rar",".iso",".cpio",".deb",".rpm",".cab",".lha",".lzh",".xar",0};

static bool arcName(const std::string& name)
{
	for(int i=0;arcExts[i];i++)
	{
		size_t n=strlen(arcExts[i]);
		if(name.size()>n&&!strcasecmp(name.c_str()+name.size()-n,arcExts[i]))
			return true;
	}
	return false;
}

// An absolute path with "." and ".." taken out by the letters, not by the disk.
static std::string lexical(const std::string& in)
{
	std::string p=in;
	if(p.empty()||p[0]!='/')
	{
		char cwd[PATH_MAX];
		if(getcwd(cwd,sizeof cwd))
			p=std::string(cwd)+"/"+p;
	}
	std::vector<std::string> parts;
	size_t i=0;
	while(i<=p.size())
	{
		size_t j=p.find('/',i);
		if(j==std::string::npos)j=p.size();
		std::string s=p.substr(i,j-i);
		if(s==".."){if(!parts.empty())parts.pop_back();}
		else if(!s.empty()&&s!=".")parts.push_back(s);
		i=j+1;
	}
	std::string r;
	for(auto& s:parts)r+="/"+s;
	return r.empty()?"/":r;
}

// Splits a path at the archive file in it. Returns false if there is none.
static bool split(const std::string& path, std::string& file, std::string& inner)
{
	std::string p=lexical(path);
	size_t i=1;
	while(i<=p.size())
	{
		size_t j=p.find('/',i);
		if(j==std::string::npos)j=p.size();
		std::string pre=p.substr(0,j);
		struct stat st;
		if(stat(pre.c_str(),&st))
			return false;
		if(S_ISREG(st.st_mode))
		{
			if(!arcName(pre))
				return false;
			char full[PATH_MAX];
			file=realpath(pre.c_str(),full)?full:pre;
			inner=j<p.size()?p.substr(j+1):"";
			return true;
		}
		i=j+1;
	}
	return false;
}

static void copyOut(char* out, int max, const std::string& s)
{
	snprintf(out,max,"%s",s.c_str());
}

// ---------------------------------------------------------------- the index of an archive

struct AEnt
{
	std::string path;     // without a leading or trailing slash
	int isDir;
	long long size;
	time_t mtime;
	mode_t mode;
	std::string link;
};

struct AIndex
{
	std::string file;
	time_t mt;
	off_t sz;
	std::vector<AEnt> e;
	std::unordered_map<std::string,int> at;
	std::unordered_map<std::string,std::vector<int>> kids;  // by the parent's path
	long long bytes;
	int files;
	int dirs;
};

static std::vector<AIndex*> cache;

static std::string cleanName(const char* s)
{
	std::string p=s;
	while(p.compare(0,2,"./")==0)p=p.substr(2);
	while(!p.empty()&&p[0]=='/')p=p.substr(1);
	while(!p.empty()&&p.back()=='/')p.pop_back();
	return lexical("/"+p).substr(1);
}

static std::string parentOf(const std::string& p)
{
	size_t s=p.rfind('/');
	return s==std::string::npos?"":p.substr(0,s);
}

static void addEntry(AIndex* x, const AEnt& en)
{
	auto it=x->at.find(en.path);
	if(it!=x->at.end())
	{
		// the later one wins, but a directory stays one
		AEnt& old=x->e[it->second];
		int dir=old.isDir||en.isDir;
		old=en;
		old.isDir=dir;
		return;
	}
	// make the parents first
	std::string par=parentOf(en.path);
	if(!en.path.empty()&&!par.empty()&&!x->at.count(par))
	{
		AEnt d;
		d.path=par;
		d.isDir=1;
		d.size=0;
		d.mtime=en.mtime;
		d.mode=S_IFDIR|0755;
		addEntry(x,d);
	}
	x->at[en.path]=(int)x->e.size();
	x->kids[par].push_back((int)x->e.size());
	x->e.push_back(en);
}

static AIndex* indexOf(const std::string& file)
{
	struct stat st;
	if(stat(file.c_str(),&st))
	{
		lastError="Cannot read the archive";
		return 0;
	}
	for(size_t i=0;i<cache.size();i++)
		if(cache[i]->file==file)
		{
			AIndex* x=cache[i];
			if(x->mt==st.st_mtime&&x->sz==st.st_size)
			{
				cache.erase(cache.begin()+i);
				cache.push_back(x);
				return x;
			}
			delete x;
			cache.erase(cache.begin()+i);
			break;
		}
	archive* a=openArchive(file);
	if(!a)return 0;
	AIndex* x=new AIndex;
	x->file=file;
	x->mt=st.st_mtime;
	x->sz=st.st_size;
	x->bytes=0;
	x->files=0;
	x->dirs=0;
	archive_entry* en;
	int r;
	while((r=pNextHeader(a,&en))==AR_OK||r==AR_WARN)
	{
		const char* name=pPathname(en);
		if(!name&&pPathnameUtf8)name=pPathnameUtf8(en);
		if(!name)continue;
		AEnt e;
		e.path=cleanName(name);
		if(e.path.empty())continue;
		unsigned t=pFiletype(en);
		e.isDir=(t==S_IFDIR);
		e.size=e.isDir?0:pSize(en);
		e.mtime=pMtime(en);
		e.mode=(t?t:S_IFREG)|(pPerm(en)&07777);
		if(t==S_IFLNK&&pSymlink(en))e.link=pSymlink(en);
		addEntry(x,e);
	}
	if(r!=AR_EOF&&x->e.empty())
	{
		const char* m=pErrorString(a);
		lastError=std::string("Cannot read the archive: ")+(m?m:"");
		pReadFree(a);
		delete x;
		return 0;
	}
	pReadFree(a);
	for(auto& e:x->e)
	{
		if(e.isDir)x->dirs++;
		else
		{
			x->files++;
			x->bytes+=e.size;
		}
	}
	cache.push_back(x);
	if(cache.size()>6)
	{
		delete cache[0];
		cache.erase(cache.begin());
	}
	return x;
}

// The entry a path names, with its index; -1 for the archive's top, -2 for none.
static int entryOf(const std::string& path, AIndex*& x, std::string* innerOut=0)
{
	std::string file;
	std::string inner;
	x=0;
	if(!split(path,file,inner))
	{
		lastError="Not in an archive";
		return -2;
	}
	x=indexOf(file);
	if(!x)return -2;
	if(innerOut)*innerOut=inner;
	if(inner.empty())return -1;
	auto it=x->at.find(inner);
	if(it==x->at.end())
	{
		lastError="No such file in the archive";
		return -2;
	}
	return it->second;
}

// An archive reader at the start of the data of the entry with this path, or 0.
static archive* openAt(AIndex* x, const std::string& inner)
{
	archive* a=openArchive(x->file);
	if(!a)return 0;
	archive_entry* en;
	int r;
	while((r=pNextHeader(a,&en))==AR_OK||r==AR_WARN)
	{
		const char* name=pPathname(en);
		if(!name&&pPathnameUtf8)name=pPathnameUtf8(en);
		if(name&&cleanName(name)==inner)
			return a;
	}
	lastError="The file is gone from the archive";
	pReadFree(a);
	return 0;
}

// ---------------------------------------------------------------- what a path is

// 0 not in an archive, 1 an archive file (its top), 2 something in an archive.
// file and inner get the two parts.
static int arc_where(const char* path, char* file, int fmax, char* inner, int imax)
{
	std::string f;
	std::string in;
	file[0]=0;
	inner[0]=0;
	if(!split(path,f,in))
		return 0;
	copyOut(file,fmax,f);
	copyOut(inner,imax,in);
	return in.empty()?1:2;
}

// 1 if the path names an archive file by its name.
static int arc_is(const char* path)
{
	struct stat st;
	if(stat(path,&st)||!S_ISREG(st.st_mode))
		return 0;
	return arcName(path)?1:0;
}

static int arc_isdir(const char* path)
{
	AIndex* x;
	int i=entryOf(path,x);
	if(i==-1)return 1;
	if(i<0)return 0;
	return x->e[i].isDir;
}

static int arc_size(const char* path)
{
	AIndex* x;
	int i=entryOf(path,x);
	if(i<0)return -1;
	if(x->e[i].isDir)
	{
		lastError="Not a regular file";
		return -1;
	}
	long long s=x->e[i].size;
	return s>INT_MAX?INT_MAX:(int)s;
}

// Reads at most max bytes of a file in an archive. Returns the count, or -1.
static int arc_load(const char* path, char* buf, int max)
{
	AIndex* x;
	std::string inner;
	int i=entryOf(path,x,&inner);
	if(i<0)return -1;
	if(x->e[i].isDir)
	{
		lastError="Not a regular file";
		return -1;
	}
	archive* a=openAt(x,inner);
	if(!a)return -1;
	int got=0;
	while(got<max)
	{
		long n=pReadData(a,buf+got,max-got);
		if(n<0)
		{
			const char* m=pErrorString(a);
			lastError=std::string("Cannot read from the archive: ")+(m?m:"");
			pReadFree(a);
			return -1;
		}
		if(n==0)break;
		got+=(int)n;
	}
	pReadFree(a);
	return got;
}

// A value that changes when the archive does.
static int arc_stamp(const char* path)
{
	std::string f;
	std::string in;
	if(!split(path,f,in))return -1;
	struct stat st;
	if(stat(f.c_str(),&st))return -1;
	unsigned v=(unsigned)st.st_mtim.tv_sec*1000u+(unsigned)(st.st_mtim.tv_nsec/1000000)+(unsigned)st.st_size*31u;
	return (int)(v&0x7fffffffu);
}

static void arc_error(char* out, int max)
{
	copyOut(out,max,lastError);
}

// ---------------------------------------------------------------- temporary files

static std::string tmpDir;
static std::map<std::string,std::string> tmpMade;   // archive path -> its file
static int tmpN=0;

static int rmOne(const char* p, const struct stat*, int, struct FTW*)
{
	remove(p);
	return 0;
}

static void tmpClean()
{
	if(!tmpDir.empty())
		nftw(tmpDir.c_str(),rmOne,16,FTW_DEPTH|FTW_PHYS);
}

// Writes the open reader's data to a new file. Returns false, with lastError, if not.
static bool writeData(archive* a, const std::string& to, mode_t mode)
{
	int fd=open(to.c_str(),O_WRONLY|O_CREAT|O_TRUNC,(mode&0777)|0600);
	if(fd<0)
	{
		lastError=std::string("Cannot create: ")+strerror(errno);
		return false;
	}
	static char buf[1<<16];
	for(;;)
	{
		long n=pReadData(a,buf,sizeof buf);
		if(n<0)
		{
			const char* m=pErrorString(a);
			lastError=std::string("Cannot read from the archive: ")+(m?m:"");
			close(fd);
			unlink(to.c_str());
			return false;
		}
		if(n==0)break;
		if(write(fd,buf,n)!=n)
		{
			lastError=std::string("Cannot write: ")+strerror(errno);
			close(fd);
			unlink(to.c_str());
			return false;
		}
	}
	close(fd);
	return true;
}

// Takes a file out of an archive into a temporary directory, removed when the
// program ends, and writes that file's path. Returns 0 with arc_error if it cannot.
static int arc_temp(const char* path, char* out, int max)
{
	out[0]=0;
	AIndex* x;
	std::string inner;
	int i=entryOf(path,x,&inner);
	if(i<0)return 0;
	if(x->e[i].isDir)
	{
		lastError="That is a directory";
		return 0;
	}
	char stamp[64];
	snprintf(stamp,sizeof stamp,"|%ld|%ld",(long)x->mt,(long)x->sz);
	std::string key=x->file+"/"+inner+stamp;
	auto it=tmpMade.find(key);
	struct stat st;
	if(it!=tmpMade.end()&&!stat(it->second.c_str(),&st))
	{
		copyOut(out,max,it->second);
		return 1;
	}
	if(tmpDir.empty())
	{
		const char* t=getenv("TMPDIR");
		std::string tpl=std::string(t&&*t?t:"/tmp")+"/t2fc-arc-XXXXXX";
		std::vector<char> b(tpl.begin(),tpl.end());
		b.push_back(0);
		if(!mkdtemp(b.data()))
		{
			lastError=std::string("Cannot make a temporary directory: ")+strerror(errno);
			return 0;
		}
		tmpDir=b.data();
		atexit(tmpClean);
	}
	std::string dir=tmpDir+"/"+std::to_string(++tmpN);
	mkdir(dir.c_str(),0700);
	size_t s=inner.rfind('/');
	std::string to=dir+"/"+(s==std::string::npos?inner:inner.substr(s+1));
	archive* a=openAt(x,inner);
	if(!a)return 0;
	bool ok=writeData(a,to,x->e[i].mode);
	pReadFree(a);
	if(!ok)return 0;
	struct timespec ts[2];
	ts[0].tv_sec=x->e[i].mtime;
	ts[0].tv_nsec=0;
	ts[1]=ts[0];
	utimensat(AT_FDCWD,to.c_str(),ts,0);
	tmpMade[key]=to;
	copyOut(out,max,to);
	return 1;
}

// The temporary directory, empty before the first arc_temp.
static void arc_tempdir(char* out, int max)
{
	copyOut(out,max,tmpDir);
}

// ---------------------------------------------------------------- listings

struct LEnt
{
	std::string name;
	int isDir;
	long long size;
	time_t mtime;
	mode_t mode;
	int marked;
};

#define SLOTS 16
static std::vector<LEnt> lists[SLOTS];
static std::string listed[SLOTS];
static int sorts[SLOTS]={0};
static int slot=0;
#define L lists[slot]

static const char* extOf(const std::string& name)
{
	size_t dot=name.rfind('.');
	if(dot==std::string::npos||dot==0)
		return "";
	return name.c_str()+dot+1;
}

static int sortNow=0;

static bool lessEnt(const LEnt& a, const LEnt& b)
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

static void sortList()
{
	sortNow=sorts[slot];
	std::stable_sort(L.begin(),L.end(),lessEnt);
}

static int arc_slot(int k)
{
	int old=slot;
	if(k>=0&&k<SLOTS)
		slot=k;
	return old;
}

static void arc_sort(int mode)
{
	sorts[slot]=mode&7;
	sortList();
}

// Lists a directory in an archive (or its top), with ".." to go up. Writes the path
// as it should be shown to resolved, and returns the entry count. Returns -2, with
// the path with "." and ".." taken out in resolved, when the path is not in an
// archive (the caller lists it as a directory), and -1 when the archive cannot be
// read or has no such directory.
static int arc_list(const char* path, char* resolved, int max, int showHidden)
{
	std::string file;
	std::string inner;
	if(!split(path,file,inner))
	{
		copyOut(resolved,max,lexical(path));
		return -2;
	}
	AIndex* x=indexOf(file);
	if(!x)return -1;
	if(!inner.empty())
	{
		auto it=x->at.find(inner);
		if(it==x->at.end()||!x->e[it->second].isDir)
		{
			lastError="No such directory in the archive";
			return -1;
		}
	}
	std::string full=file+(inner.empty()?"":"/"+inner);
	std::vector<std::string> keep;
	if(listed[slot]==full)
		for(const LEnt& e:L)
			if(e.marked)
				keep.push_back(e.name);
	std::sort(keep.begin(),keep.end());
	std::vector<LEnt> out;
	LEnt up;
	up.name="..";
	up.isDir=1;
	up.size=0;
	up.mtime=x->mt;
	up.mode=S_IFDIR|0755;
	up.marked=0;
	out.push_back(up);
	auto k=x->kids.find(inner);
	if(k!=x->kids.end())
		for(int i:k->second)
		{
			const AEnt& a=x->e[i];
			LEnt e;
			size_t s=a.path.rfind('/');
			e.name=s==std::string::npos?a.path:a.path.substr(s+1);
			if(e.name[0]=='.'&&!showHidden)
				continue;
			e.isDir=a.isDir;
			e.size=a.size;
			e.mtime=a.mtime;
			e.mode=a.mode;
			e.marked=std::binary_search(keep.begin(),keep.end(),e.name)?1:0;
			out.push_back(e);
		}
	L.swap(out);
	listed[slot]=full;
	sortList();
	copyOut(resolved,max,full);
	return (int)L.size();
}

static int valid(int i)
{
	return i>=0&&i<(int)L.size();
}

static int arc_name(int i, char* out, int max)
{
	out[0]=0;
	if(!valid(i))return 0;
	copyOut(out,max,L[i].name);
	return L[i].isDir;
}

static int arc_index(const char* name)
{
	for(size_t i=0;i<L.size();i++)
		if(L[i].name==name)
			return (int)i;
	return -1;
}

static int arc_find(int letter, int start)
{
	int n=(int)L.size();
	for(int k=1;k<=n;k++)
	{
		int i=(start+k)%n;
		if(!L[i].name.empty()&&tolower((unsigned char)L[i].name[0])==tolower(letter))
			return i;
	}
	return -1;
}

static std::string sizeText(long long v, int isDir, bool up)
{
	char buf[32];
	if(up)snprintf(buf,sizeof buf,"UP--DIR");
	else if(isDir)snprintf(buf,sizeof buf,"<DIR>");
	else if(v<10000000)snprintf(buf,sizeof buf,"%lld",v);
	else if(v<10000000LL*1024)snprintf(buf,sizeof buf,"%lldK",(v+1023)/1024);
	else if(v<10000000LL*1024*1024)snprintf(buf,sizeof buf,"%lldM",(v+1048575)/1048576);
	else snprintf(buf,sizeof buf,"%lldG",v/1073741824LL);
	return buf;
}

static std::string timeText(time_t t)
{
	char buf[32];
	time_t now=time(0);
	const char* fmt="%b %e %H:%M";
	if(t<now-183*86400||t>now+3600)
		fmt="%b %e  %Y";
	strftime(buf,sizeof buf,fmt,localtime(&t));
	return buf;
}

// As term_dl's fs_field: 0 size, 1 time, 2 permissions, 3 type mark.
static void arc_field(int i, int k, char* out, int max)
{
	out[0]=0;
	if(!valid(i))return;
	const LEnt& e=L[i];
	char buf[32];
	buf[0]=0;
	if(k==0)
	{
		copyOut(out,max,sizeText(e.size,e.isDir,e.name==".."));
		return;
	}
	if(k==1)
	{
		copyOut(out,max,timeText(e.mtime));
		return;
	}
	if(k==2)
	{
		mode_t m=e.mode;
		char t='-';
		if(S_ISDIR(m))t='d';
		else if(S_ISLNK(m))t='l';
		const char* r="rwxrwxrwx";
		buf[0]=t;
		for(int b=0;b<9;b++)
			buf[1+b]=(m&(0400>>b))?r[b]:'-';
		buf[10]=0;
	}
	else if(k==3)
	{
		char c=' ';
		if(S_ISLNK(e.mode))c='@';
		else if(e.isDir)c='/';
		else if(e.mode&0111)c='*';
		buf[0]=c;
		buf[1]=0;
	}
	copyOut(out,max,buf);
}

static int arc_mark(int i, int v)
{
	if(!valid(i)||L[i].name=="..")return 0;
	if(v<0)L[i].marked=!L[i].marked;
	else L[i].marked=v?1:0;
	return L[i].marked;
}

static int arc_marked(int i)
{
	if(!valid(i))return 0;
	return L[i].marked;
}

// v 1 marks, 0 unmarks the names matching pattern, 2 inverts every mark. Directories
// only if dirsToo. Returns how many changed.
static int arc_mark_match(int v, const char* pattern, int dirsToo)
{
	int n=0;
	for(LEnt& e:L)
	{
		if(e.name=="..")continue;
		if(e.isDir&&!dirsToo&&v!=2)continue;
		if(v==2)
		{
			if(e.isDir)continue;
			e.marked=!e.marked;
			n++;
		}
		else if(!fnmatch(pattern,e.name.c_str(),0))
		{
			if(e.marked!=v)n++;
			e.marked=v;
		}
	}
	return n;
}

static std::string bytesText(long long v)
{
	char buf[32];
	if(v<10000)snprintf(buf,sizeof buf,"%lld B",v);
	else if(v<10000LL*1024)snprintf(buf,sizeof buf,"%lld K",(v+1023)/1024);
	else if(v<10000LL*1024*1024)snprintf(buf,sizeof buf,"%lld M",(v+1048575)/1048576);
	else snprintf(buf,sizeof buf,"%lld G",(v+1073741823LL)/1073741824LL);
	return buf;
}

static int arc_mark_info(char* out, int max)
{
	int n=0;
	long long bytes=0;
	for(const LEnt& e:L)
		if(e.marked)
		{
			n++;
			if(!e.isDir)bytes+=e.size;
		}
	copyOut(out,max,bytesText(bytes)+" in "+std::to_string(n)+(n==1?" entry":" entries"));
	return n;
}

// ---------------------------------------------------------------- the whole tree

struct TRow
{
	int ent;
	std::string prefix;
};

#define TREES 8
static std::vector<TRow> trees[TREES];
static AIndex* treeOf[TREES];
static std::string treeFile[TREES];

static void treeWalk(int id, AIndex* x, const std::string& dir, const std::string& prefix)
{
	auto k=x->kids.find(dir);
	if(k==x->kids.end())return;
	std::vector<int> kids=k->second;
	std::sort(kids.begin(),kids.end(),[x](int a, int b)
	{
		const AEnt& p=x->e[a];
		const AEnt& q=x->e[b];
		if(p.isDir!=q.isDir)return p.isDir>q.isDir;
		return strcasecmp(p.path.c_str(),q.path.c_str())<0;
	});
	for(size_t i=0;i<kids.size();i++)
	{
		bool last=i+1==kids.size();
		TRow r;
		r.ent=kids[i];
		r.prefix=prefix+(last?"\xe2\x94\x94\xe2\x94\x80 ":"\xe2\x94\x9c\xe2\x94\x80 ");
		trees[id].push_back(r);
		if(x->e[kids[i]].isDir)
			treeWalk(id,x,x->e[kids[i]].path,prefix+(last?"   ":"\xe2\x94\x82  "));
	}
}

// Reads an archive's whole tree into tree id (0-7) and returns its row count, or -1.
static int arc_tree(int id, const char* path)
{
	if(id<0||id>=TREES)return -1;
	trees[id].clear();
	std::string file;
	std::string inner;
	if(!split(path,file,inner))
	{
		lastError="Not an archive";
		return -1;
	}
	AIndex* x=indexOf(file);
	if(!x)return -1;
	treeOf[id]=x;
	treeFile[id]=file;
	treeWalk(id,x,"","");
	return (int)trees[id].size();
}

// A row of tree id: k 0 the tree lines and the name, 1 the size, 2 the time, 3 the
// path to give the other calls. Returns 1 for a directory, 0 for a file, -1 if none.
static int arc_row(int id, int i, int k, char* out, int max)
{
	out[0]=0;
	if(id<0||id>=TREES||i<0||i>=(int)trees[id].size())return -1;
	// the index may have gone from the cache; read it again if so
	AIndex* x=0;
	for(AIndex* c:cache)
		if(c==treeOf[id])x=c;
	if(!x)
	{
		x=indexOf(treeFile[id]);
		if(!x||x!=treeOf[id])
		{
			// the archive changed under the window
			if(x)
			{
				treeOf[id]=x;
				trees[id].clear();
				treeWalk(id,x,"","");
				if(i>=(int)trees[id].size())return -1;
			}
			else return -1;
		}
	}
	const TRow& r=trees[id][i];
	const AEnt& e=x->e[r.ent];
	if(k==0)
	{
		size_t s=e.path.rfind('/');
		std::string name=s==std::string::npos?e.path:e.path.substr(s+1);
		copyOut(out,max,r.prefix+name+(e.isDir?"/":""));
	}
	else if(k==1)copyOut(out,max,e.isDir?"":sizeText(e.size,0,false));
	else if(k==2)copyOut(out,max,timeText(e.mtime));
	else if(k==3)copyOut(out,max,treeFile[id]+"/"+e.path);
	return e.isDir;
}

// "N files, N directories, size" for tree id.
static void arc_summary(int id, char* out, int max)
{
	out[0]=0;
	if(id<0||id>=TREES||!treeOf[id])return;
	AIndex* x=0;
	for(AIndex* c:cache)
		if(c==treeOf[id])x=c;
	if(!x)return;
	copyOut(out,max,std::to_string(x->files)+(x->files==1?" file, ":" files, ")+
		std::to_string(x->dirs)+(x->dirs==1?" directory, ":" directories, ")+bytesText(x->bytes));
}

// ---------------------------------------------------------------- the copy job

enum { J_MKDIR, J_COPY, J_LINK, J_TIME };

struct JAct
{
	int kind;
	std::string inner;   // in the archive
	std::string dst;
	long long size;
	mode_t mode;
	time_t mtime;
	std::string link;
};

static std::vector<std::string> jobSrc;
static std::vector<JAct> acts;
static AIndex* jobX=0;
static std::string jobFile;
static size_t at=0;
static int jobState=0;      // 0 working, -1 an error to answer, -2 a target exists
static int overAll=0;       // 1 overwrite all, 2 skip all
static int skipErrs=0;
static int force=0;
static archive* jobAr=0;
static int jobOut=-1;
static long long copied=0;
static long long bytesDone=0;
static long long total=0;
static int files=0;
static int filesDone=0;

static void jobClose()
{
	if(jobAr)pReadFree(jobAr);
	if(jobOut>=0)close(jobOut);
	jobAr=0;
	jobOut=-1;
}

static void arc_job_clear()
{
	jobClose();
	jobSrc.clear();
	acts.clear();
	at=0;
	jobState=0;
	overAll=0;
	skipErrs=0;
	force=0;
	copied=0;
	bytesDone=0;
	total=0;
	files=0;
	filesDone=0;
}

static void arc_job_add(const char* path)
{
	jobSrc.push_back(path);
}

static void plan(int i, const std::string& dst)
{
	const AEnt& e=jobX->e[i];
	JAct a;
	a.inner=e.path;
	a.dst=dst;
	a.size=e.size;
	a.mode=e.mode&07777;
	a.mtime=e.mtime;
	a.link=e.link;
	if(e.isDir)
	{
		a.kind=J_MKDIR;
		acts.push_back(a);
		files++;
		auto k=jobX->kids.find(e.path);
		if(k!=jobX->kids.end())
			for(int c:k->second)
			{
				const std::string& p=jobX->e[c].path;
				size_t s=p.rfind('/');
				plan(c,dst+"/"+(s==std::string::npos?p:p.substr(s+1)));
			}
		a.kind=J_TIME;
		acts.push_back(a);
		return;
	}
	a.kind=S_ISLNK(e.mode)?J_LINK:J_COPY;
	acts.push_back(a);
	files++;
	total+=e.size;
}

// Copies the sources added, all from one archive, into the directory dst (or, for
// one source, to the name dst). Returns 0 with arc_error if it cannot start.
static int arc_job_start(const char* dstPath)
{
	acts.clear();
	at=0;
	jobState=0;
	std::string dst=dstPath;
	std::string f;
	std::string in;
	if(split(dst,f,in))
	{
		lastError="Archives are read-only: nothing can be copied into one.";
		return 0;
	}
	struct stat st;
	int intoDir=!stat(dst.c_str(),&st)&&S_ISDIR(st.st_mode);
	if(!intoDir&&jobSrc.size()!=1)
	{
		lastError="The target must be an existing directory for more than one source.";
		return 0;
	}
	for(const std::string& s:jobSrc)
	{
		AIndex* x;
		std::string inner;
		int i=entryOf(s,x,&inner);
		if(i==-1)
		{
			lastError="Copy the archive itself from the panel above it.";
			return 0;
		}
		if(i<0)return 0;
		if(jobX&&jobX!=x&&!acts.empty())
		{
			lastError="The sources must be in one archive.";
			return 0;
		}
		jobX=x;
		jobFile=x->file;
		size_t sl=inner.rfind('/');
		std::string base=sl==std::string::npos?inner:inner.substr(sl+1);
		plan(i,intoDir?dst+"/"+base:dst);
	}
	return 1;
}

static int fail(const std::string& what)
{
	lastError=what+(errno?std::string(": ")+strerror(errno):"");
	jobClose();
	jobState=-1;
	return -1;
}

static void next()
{
	JAct& a=acts[at];
	if(a.kind!=J_TIME)filesDone++;
	if(a.kind==J_COPY)bytesDone+=a.size;
	copied=0;
	force=0;
	at++;
}

static int exists(const std::string& dst)
{
	struct stat st;
	if(lstat(dst.c_str(),&st))return 1;
	if(force||overAll==1)return 1;
	if(overAll==2)return 0;
	jobState=-2;
	return -2;
}

static void skipDir()
{
	std::string pre=acts[at].dst+"/";
	std::string top=acts[at].dst;
	size_t end=at+1;
	while(end<acts.size()&&(acts[end].dst==top||acts[end].dst.compare(0,pre.size(),pre)==0))
		end++;
	while(at<end)next();
}

static int run(std::chrono::steady_clock::time_point until)
{
	JAct& a=acts[at];
	struct stat st;
	errno=0;
	if(a.kind==J_MKDIR)
	{
		if(!lstat(a.dst.c_str(),&st))
		{
			if(S_ISDIR(st.st_mode))
			{
				next();
				return 1;
			}
			int r=exists(a.dst);
			if(r<0)return r;
			if(!r)
			{
				skipDir();
				return 1;
			}
			if(unlink(a.dst.c_str()))return fail("Cannot replace");
		}
		if(mkdir(a.dst.c_str(),a.mode|0700))return fail("Cannot make the directory");
		next();
		return 1;
	}
	if(a.kind==J_TIME)
	{
		chmod(a.dst.c_str(),a.mode|0700);
		struct timespec ts[2];
		ts[0].tv_sec=a.mtime;
		ts[0].tv_nsec=0;
		ts[1]=ts[0];
		utimensat(AT_FDCWD,a.dst.c_str(),ts,0);
		next();
		return 1;
	}
	if(a.kind==J_LINK)
	{
		int r=exists(a.dst);
		if(r<0)return r;
		if(!r)
		{
			next();
			return 1;
		}
		if(!lstat(a.dst.c_str(),&st))
		{
			if(S_ISDIR(st.st_mode))
			{
				errno=EISDIR;
				return fail("A directory is in the way");
			}
			unlink(a.dst.c_str());
		}
		if(symlink(a.link.c_str(),a.dst.c_str()))return fail("Cannot make the link");
		next();
		return 1;
	}
	// J_COPY
	if(!jobAr)
	{
		if(!lstat(a.dst.c_str(),&st)&&S_ISDIR(st.st_mode))
		{
			errno=EISDIR;
			return fail("A directory is in the way");
		}
		int r=exists(a.dst);
		if(r<0)return r;
		if(!r)
		{
			next();
			return 1;
		}
		jobX=indexOf(jobFile);
		if(!jobX)return fail("Cannot read the archive");
		jobAr=openAt(jobX,a.inner);
		if(!jobAr)
		{
			errno=0;
			return fail(lastError);
		}
		jobOut=open(a.dst.c_str(),O_WRONLY|O_CREAT|O_TRUNC,a.mode|0600);
		if(jobOut<0)return fail("Cannot create");
		copied=0;
	}
	static char buf[1<<20];
	while(std::chrono::steady_clock::now()<until)
	{
		long n=pReadData(jobAr,buf,sizeof buf);
		if(n<0)
		{
			const char* m=pErrorString(jobAr);
			std::string why=std::string("Cannot read from the archive: ")+(m?m:"");
			jobClose();
			unlink(a.dst.c_str());
			errno=0;
			return fail(why);
		}
		if(n==0)
		{
			jobClose();
			chmod(a.dst.c_str(),a.mode);
			struct timespec ts[2];
			ts[0].tv_sec=a.mtime;
			ts[0].tv_nsec=0;
			ts[1]=ts[0];
			utimensat(AT_FDCWD,a.dst.c_str(),ts,0);
			next();
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
				jobClose();
				unlink(a.dst.c_str());
				errno=e;
				return fail("Cannot write");
			}
			p+=w;
			n-=w;
			copied+=w;
		}
	}
	return 1;
}

// As fs_job_step: 1 more to do, 0 done, -1 an error and -2 a target that exists,
// both waiting for arc_job_answer.
static int arc_job_step(int ms)
{
	if(jobState<0)return jobState;
	auto until=std::chrono::steady_clock::now()+std::chrono::milliseconds(ms);
	while(at<acts.size())
	{
		int r=run(until);
		if(r<0)
		{
			if(r==-1&&skipErrs)
			{
				jobState=0;
				next();
				continue;
			}
			return r;
		}
		if(std::chrono::steady_clock::now()>=until)break;
	}
	return at<acts.size()?1:0;
}

// 0 retry or overwrite, 1 skip, 2 overwrite all or skip all errors, 3 skip all
// existing targets, 4 abort.
static void arc_job_answer(int k)
{
	int was=jobState;
	jobState=0;
	if(k==4)
	{
		if(jobOut>=0&&at<acts.size())
		{
			jobClose();
			unlink(acts[at].dst.c_str());
		}
		jobClose();
		at=acts.size();
		return;
	}
	if(was==-2)
	{
		if(k==0)force=1;
		else if(k==2)overAll=1;
		else if(k==1||k==3)
		{
			if(k==3)overAll=2;
			if(acts[at].kind==J_MKDIR)skipDir();
			else next();
		}
		return;
	}
	if(k==1||k==2)
	{
		if(k==2)skipErrs=1;
		next();
	}
}

static int arc_job_info(int k)
{
	if(k==1)return filesDone;
	if(k==2)return files;
	if(k==3)
	{
		if(at<acts.size()&&acts[at].size>0)
			return (int)(copied*1000/acts[at].size);
		return 0;
	}
	double whole=(double)total+files*4096.0;
	if(whole<=0)return 1000;
	double done=(double)(bytesDone+copied)+filesDone*4096.0;
	int v=(int)(done*1000/whole);
	return v>1000?1000:v;
}

static void arc_job_name(int k, char* out, int max)
{
	out[0]=0;
	if(at<acts.size())
		copyOut(out,max,k?acts[at].dst:jobFile+"/"+acts[at].inner);
}

// ---------------------------------------------------------------- Scinc bindings

extern "C" int arc_where_wrapper(ScincVM* s, void*){ScincBoundCall(s,arc_where);return 0;}
extern "C" int arc_is_wrapper(ScincVM* s, void*){ScincBoundCall(s,arc_is);return 0;}
extern "C" int arc_isdir_wrapper(ScincVM* s, void*){ScincBoundCall(s,arc_isdir);return 0;}
extern "C" int arc_size_wrapper(ScincVM* s, void*){ScincBoundCall(s,arc_size);return 0;}
extern "C" int arc_load_wrapper(ScincVM* s, void*){ScincBoundCall(s,arc_load);return 0;}
extern "C" int arc_stamp_wrapper(ScincVM* s, void*){ScincBoundCall(s,arc_stamp);return 0;}
extern "C" int arc_error_wrapper(ScincVM* s, void*){ScincBoundCall(s,arc_error);return 0;}
extern "C" int arc_temp_wrapper(ScincVM* s, void*){ScincBoundCall(s,arc_temp);return 0;}
extern "C" int arc_tempdir_wrapper(ScincVM* s, void*){ScincBoundCall(s,arc_tempdir);return 0;}
extern "C" int arc_slot_wrapper(ScincVM* s, void*){ScincBoundCall(s,arc_slot);return 0;}
extern "C" int arc_sort_wrapper(ScincVM* s, void*){ScincBoundCall(s,arc_sort);return 0;}
extern "C" int arc_list_wrapper(ScincVM* s, void*){ScincBoundCall(s,arc_list);return 0;}
extern "C" int arc_name_wrapper(ScincVM* s, void*){ScincBoundCall(s,arc_name);return 0;}
extern "C" int arc_index_wrapper(ScincVM* s, void*){ScincBoundCall(s,arc_index);return 0;}
extern "C" int arc_find_wrapper(ScincVM* s, void*){ScincBoundCall(s,arc_find);return 0;}
extern "C" int arc_field_wrapper(ScincVM* s, void*){ScincBoundCall(s,arc_field);return 0;}
extern "C" int arc_mark_wrapper(ScincVM* s, void*){ScincBoundCall(s,arc_mark);return 0;}
extern "C" int arc_marked_wrapper(ScincVM* s, void*){ScincBoundCall(s,arc_marked);return 0;}
extern "C" int arc_mark_match_wrapper(ScincVM* s, void*){ScincBoundCall(s,arc_mark_match);return 0;}
extern "C" int arc_mark_info_wrapper(ScincVM* s, void*){ScincBoundCall(s,arc_mark_info);return 0;}
extern "C" int arc_tree_wrapper(ScincVM* s, void*){ScincBoundCall(s,arc_tree);return 0;}
extern "C" int arc_row_wrapper(ScincVM* s, void*){ScincBoundCall(s,arc_row);return 0;}
extern "C" int arc_summary_wrapper(ScincVM* s, void*){ScincBoundCall(s,arc_summary);return 0;}
extern "C" int arc_job_clear_wrapper(ScincVM* s, void*){ScincBoundCall(s,arc_job_clear);return 0;}
extern "C" int arc_job_add_wrapper(ScincVM* s, void*){ScincBoundCall(s,arc_job_add);return 0;}
extern "C" int arc_job_start_wrapper(ScincVM* s, void*){ScincBoundCall(s,arc_job_start);return 0;}
extern "C" int arc_job_step_wrapper(ScincVM* s, void*){ScincBoundCall(s,arc_job_step);return 0;}
extern "C" int arc_job_answer_wrapper(ScincVM* s, void*){ScincBoundCall(s,arc_job_answer);return 0;}
extern "C" int arc_job_info_wrapper(ScincVM* s, void*){ScincBoundCall(s,arc_job_info);return 0;}
extern "C" int arc_job_name_wrapper(ScincVM* s, void*){ScincBoundCall(s,arc_job_name);return 0;}
