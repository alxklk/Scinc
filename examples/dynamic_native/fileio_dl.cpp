#include <cstdio>
#include "../../include/ScincVM.h"
#include "../../include/ScincBinder.h"

struct FILE_ptr{
	FILE*f;
	static const int size_of=sizeof(FILE*);
};

#ifdef MSVC
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

FILE_ptr fopen_impl(const char* fname, const char* mode){return {fopen(fname, mode)};}
extern "C" EXPORT int fopen_wrapper(ScincVM* s, void*){ScincBoundCall(s,fopen_impl);return 0;}

int fclose_impl(FILE_ptr f){return fclose(f.f);}
extern "C" EXPORT int fclose_wrapper(ScincVM* s, void*){ScincBoundCall(s,fclose_impl);return 0;}

int fwrite_impl(char* ptr, int size, int n, FILE_ptr f){return fwrite((void*)ptr, size, n, f.f);};
extern "C" EXPORT int fwrite_wrapper(ScincVM* s, void*){ScincBoundCall(s,fwrite_impl);return 0;}
