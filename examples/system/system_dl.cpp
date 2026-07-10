#include "../../include/ScincVM.h"
#include "../../include/ScincBinder.h"

#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

#include <cstdlib>

int system(const char* command)
{
	return std::system(command);
}

extern "C" EXPORT int system_Wrapper(ScincVM* s, void*)
{
	ScincBoundCall(s, system);
	return 0;
}
