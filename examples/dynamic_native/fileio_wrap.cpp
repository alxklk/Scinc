#ifndef __SCINC__
#include <stdio.h>
#include "../../include/ScincVM.h"
#include "../../include/ScincBinder.h"

struct FILE_ptr{
	FILE*f;
	static const int size_of=sizeof(FILE*);
};

#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

#else

typedef __native__<8> FILE_ptr;

#endif

#ifdef __SCINC__
#define NAME(name) #name"_wrapper"
#define WRAP(rettype, name, args, ...) [[scinc::dynlink("fileio_wrap", NAME(name) )]]rettype name args;
#else
#define WRAP(rettype, name, args, ...) extern "C" EXPORT int name##_wrapper(ScincVM* s, void*){ScincBoundCall(s,+[]args ->rettype __VA_ARGS__);return 0;}
#endif

WRAP(FILE_ptr ,Fopen, (const char* fname, const char* mode), {return {fopen(fname, mode)};})
WRAP(int, Fclose, (FILE_ptr f), {return fclose(f.f);})
WRAP(int, Fwrite, (void* ptr, int size, int n, FILE_ptr f), {return fwrite(ptr, size, n, f.f);})

#ifdef __SCINC__

int main()
{
	FILE_ptr f = Fopen("result.txt", "wb");
#define BUFSIZE 16
	char buf[BUFSIZE];
	for (int i = 0; i < BUFSIZE; i++)
		buf[i] = 'a' + i;
	int written = Fwrite((void *)&(buf[0]), 1, BUFSIZE, f);
	printf("Written %i bytes\n", written);
	for (int i = 0; i < BUFSIZE; i++)
		buf[i] = ' ';
	int count = snprintf(buf, BUFSIZE, "\n%12.10f", sqrt(2.));
	written = Fwrite((void *)&(buf[0]), 1, count, f);
	printf("Written %i bytes\n", written);
	Fclose(f);
	return 0;
}

#endif