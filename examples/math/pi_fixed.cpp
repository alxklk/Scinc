#define FIXED_DIGITS 100

#include "fixed.h"

int main(
#ifndef __SCINC__
int argc, char** argv
#endif
)
{
	printf("Pi method 1\n");
	fixed Pi;
	Pi.PI();
	printf("Pi method 0\n");
	fixed Pi0;
	Pi0.PI0();
	printf("Pi1:");Pi.Print();printf("\n");
	printf("Pi0:");Pi0.Print();printf("\n");
	return 0;
}
