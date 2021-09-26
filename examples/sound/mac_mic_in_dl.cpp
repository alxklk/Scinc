#include "../../include/ScincVM.h"
#include "../../include/ScincBinder.h"

#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

int bufLen=0;

int MIC_In_Init(int buflen)
{
	bufLen=buflen;
	return 0;
}

int MIC_In_Record(double* buf)
{
	for(int i=0;i<bufLen;i++)
		buf[i]=0.;
	return 0;
}

int MIC_In_Done()
{
	return 0;
}

extern "C" EXPORT int MIC_In_Init_Wrapper  (ScincVM* s, void*){ScincBoundCall(s,MIC_In_Init);return 0;}
extern "C" EXPORT int MIC_In_Record_Wrapper(ScincVM* s, void*){ScincBoundCall(s,MIC_In_Record);return 0;}
extern "C" EXPORT int MIC_In_Done_Wrapper  (ScincVM* s, void*){ScincBoundCall(s,MIC_In_Done);return 0;}
