#include <stdio.h>
typedef int(*pf)(int x, float y, char* z);

int mf(int x, pf f)
{
    return f(x,0,"2");
}

int sq(int x, float y, char* z);

int main()
{
	// Here Scinc differs from c++, probably because 
	// the priorities of & and () relate differently
	// &sq(123,2,""); 

	// Here Scinc fails to compile, c++ works
	//(&sq)(0,2,""); 

	pf fp0=&sq;
	pf fp1=[](int x, float y, char* z)->int{printf("fp1: %i\n",x);return x*x;};
	fp0(1,2,"");
	fp1(2,3,"");

	// Here Scinc fails, c++ works
	// [](int x, float y, char* z)->int{printf("lam: %i\n",x);return x*x;}(3,3,"");

	printf("%i\n", mf(4, &sq));
	printf("%i\n", mf(5, [](int x, float y, char* z)->int{printf("fp1: %i\n",x);return x*x;}));
	printf("%i\n", mf(6, fp1));
	return 0;
}

int sq(int x, float y, char* z)
{
	printf("sq: %i\n",x);
	return x*x;
}
