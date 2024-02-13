#include <stdio.h>

//#ifdef __SCINC__
//class __native__(4) NativeClass;

class NativeClass{
	public:
	//[[scinc::dynlink("nativeclass_dl","NativeClassMethod_wrapper")]]int Method(int x);
};

//__dyn_lib_import__("nativeclass_dl","NativeClassMethod_wrapper")int NativeClass::Method(int x);
[[scinc::dynlink("nativeclass_dl","NativeClassMethod_wrapper")]] int Method(int x);

//#else

//#endif

int main()
{
	//NativeClass nc;nc.Method(46);
	Method(46);
	return 0;
}