class CDigitStream
{
public:
	void begin(){};
	void put(int c){printf("%c",c);}
	void end(){};
};

/*

#pragma STACK_SIZE 6553600

#include <stdio.h>
#define FIXED_DIGITS 2800
#define FIXED_FRACTS 1
class CDigitStream
{
public:
	void begin(){};
	void put(int c){printf("%c",c);}
	void end(){};
};
#include "fixed.h"


int main(
#ifndef __SCINC__
int argc, char** argv
#endif
)
{
	CDigitStream ds;
	fixed n;n.Seti(1);
	printf("1 :");n.Print(ds);printf("\n");
	for(int i=2;i<=1000;i++)
	{
		fixed n1;n1.Seti(i);
		n=n*n1;
		printf("%i :",i);
		n.Print(ds);
		printf("\n");
	}
	return 0;
}
*/