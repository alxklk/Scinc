#define FIXED_DIGITS 250

#include "fixed.h"

int main(
#ifndef __SCINC__
int argc, char** argv
#endif
)
{
	printf("Sqrt(200)\n");
	fixed two;
	two.Seti(200);
	fixed stwo=two.Sqrt();
	stwo.Print();printf("\n");
	printf("Check:");(stwo*stwo).Print();printf("\n");
	return 0;
}
