// clangd mock of what Scinc makes available to a program. Force-included by
// compile_flags.txt, never read by Scinc. Signatures come from `Scinc -hdr`.
//
// graphics.h, sound.h and svg.h are pulled in here because Scinc resolves names
// regardless of order, and headers such as ws.h use Graph without including it.
// The cost: clangd will not notice a forgotten #include "graphics.h".
//
// Scinc also predefines `const` as empty. That is left out on purpose: with real
// const, temporaries bind to `const T&` parameters, which plain C++ refuses for `T&`.
#ifndef SCINC_BASE_H
#define SCINC_BASE_H

#define __SCINC__
#define SEEK_CUR 1
#define SEEK_END 2
#define SEEK_SET 0

struct FILE;

// Scinc converts void* to any pointer implicitly, C++ does not.
struct __scinc_void_ptr
{
	void* p;
	template<class T> operator T*() const;
};

// An opaque N-byte native value, spelled __native__(8) or __native__<8>.
template<int N> struct __native__ { char content[N]; };
#define __native__(n) __native__<n>

[[noreturn]] void __scinc_finish(int);

void Interrupt();
int _Inp_GetSP();
float Time();
__scinc_void_ptr malloc(int);
void free(const void*);
int printf(const char*,...);
int snprintf(char*,int,const char*,...);
int puts(const char*);
char* gets_s(char*,int);
float sin(float);
float cos(float);
float sqrt(float);
float exp(float);
float tan(float);
float atan2(float,float);
float pow(float,float);
void* memcpy(void*,const void*,int);
void* memset(void*,int,int);
void* memmove(void*,const void*,int);
void SetHostInt(const char*,int);
void SetHostFloat(const char*,float);
float GetHostFloat(const char*,float);
int GetHostInt(const char*,int);
FILE* fopen(const char*,const char*);
int fclose(FILE*);
int fflush(FILE*);
int fgetc(FILE*);
int fwrite(const void*,int,int,FILE*);
int fread(void*,int,int,FILE*);
char* fgets(char*,int,FILE*);
const char* fputs(const char*,FILE*);
int fprintf(FILE*,const char*,...);
int fseek(FILE*,int,int);
int ftell(FILE*);
void Wait(float);

#include "graphics.h"
#include "sound.h"
#include "svg.h"

// float and double are one 8-byte type in Scinc.
#define double float
#define bool int
#define exit(x) __scinc_finish(x)
#define false 0
#define true 1
#define stdin ((FILE*)1)
#define stdout ((FILE*)2)
#define stderr ((FILE*)3)

#endif // SCINC_BASE_H
