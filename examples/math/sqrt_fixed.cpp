#define FIXED_DIGITS 250

#include "fixed.h"

int main(
#ifndef __SCINC__
int argc, char** argv
#endif
)
{
	printf("Sqrt 2\n");
	fixed two;
	two.Seti(2);
    fixed stwo=two.Sqrt();
	stwo.Print();printf("\n");
	printf("Check:");(stwo*stwo).Print();printf("\n");
	return 0;
}
