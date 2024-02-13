#include <stdio.h>
#define FIXED_DIGITS 2600
#define FIXED_FRACTS 4

#include "fixed_digistream.h"
#include "fixed.h"

int main(
#ifndef __SCINC__
int argc, char** argv
#endif
)
{
	CDigitStream out;
	fixed n[2];n[0].Seti(1);n[1].Seti(1);
	printf("1 :");n[0].Print(out);printf("\n");
	printf("2 :");n[1].Print(out);printf("\n");
	for(int i=3;i<=2000;i++)
	{
		n[i%2]=n[0]+n[1];
		printf("%i :",i);
		n[i%2].Print(out);
		printf("\n");
	}
	return 0;
}
