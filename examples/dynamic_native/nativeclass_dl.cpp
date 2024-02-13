#include <cstdio>
#include "../../include/ScincVM.h"
#include "../../include/ScincBinder.h"

struct NativeClass{
	int id;
	static const int size_of=sizeof(int);
};

#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

int NativeClassMethod_impl(int x)
{
	printf(" Native class method called with arg %i\n", x);
	return 13;
};
extern "C" EXPORT int NativeClassMethod_wrapper(ScincVM* s, void*){ScincBoundCall(s,NativeClassMethod_impl);return 0;}
