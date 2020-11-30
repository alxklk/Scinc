#pragma STACK_SIZE 6553600

#include <stdio.h>
#define FIXED_DIGITS 2600
#define FIXED_FRACTS 2
#include "fixed.h"

int main(
#ifndef __SCINC__
int argc, char** argv
#endif
)
{
	fixed n;n.Seti(1);
	printf("1 :");n.Print();printf("\n");
	for(int i=2;i<=1000;i++)
	{
		fixed n1;n1.Seti(i);
		n=n*n1;
		printf("%i :",i);
		n.Print();
		printf("\n");
	}
	return 0;
}
