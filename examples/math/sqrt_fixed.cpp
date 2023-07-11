#define FIXED_DIGITS 150
#define FIXED_FRACTS 130

#include "fixed.h"

int main(
#ifndef __SCINC__
int argc, char** argv
#endif
)
{
	char* s="10000000000000000000.000000000000000000000000000000000000000001";
	printf("Sqrt(%s)\n",s);
	fixed two;
	two.Sets(s);
	fixed stwo=two.Sqrt();
	stwo.Print();printf("\n");
	printf("Check:");(stwo*stwo).Print();printf("\n");
	return 0;
}
