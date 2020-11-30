#include <stdio.h>
#define FIXED_DIGITS 600
#define FIXED_FRACTS 4
#include "fixed.h"

int main(
#ifndef __SCINC__
int argc, char** argv
#endif
)
{
	fixed n[2];n[0].Seti(1);n[1].Seti(1);
	printf("1 :");n[0].Print();printf("\n");
	printf("2 :");n[1].Print();printf("\n");
	for(int i=3;i<=1000;i++)
	{
		n[i%2]=n[0]+n[1];
		printf("%i :",i);
		n[i%2].Print();
		printf("\n");
	}
	return 0;
}
