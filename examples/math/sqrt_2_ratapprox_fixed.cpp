#define FIXED_DIGITS 250

#include "fixed.h"

int main(
#ifndef __SCINC__
int argc, char** argv
#endif
)
{
	printf("Sqrt 2 rational approximation without divisions:\n");
	fixed x0;x0.Seti(1);
    fixed x1;x1.Seti(1);
	for(int i=0;i<160;i++)
	{
		x1.Print();printf("\n---------------\n");x0.Print();printf("\n\n");
		fixed s=x1/x0;
		printf("=\n");s.Print();printf("\n\n");
		printf("Check #%i:",i+1);(s*s).Print();printf("\n\n");
		fixed oldx0=x0;
		x0=x1+x0;
		x1=x0+oldx0;
	}
	return 0;
}
