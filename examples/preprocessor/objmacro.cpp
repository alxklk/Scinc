#include <stdio.h>


#define M N "%" O "&" B "-" O N
#define N O "+" O "*" B 
#define O " " B "$" B " "

int main()
{
#define B "1"
	printf("'%s'\n", M);
#define B "2"
	printf("'%s'\n", M);
	return 0;
}
