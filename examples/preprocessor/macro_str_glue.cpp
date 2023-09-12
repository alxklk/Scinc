#include <stdio.h>

#define STR0(x) #x
#define M1(b) STR0(b)

int main()
{
	printf("'%s'\n", M1(2));
	return 0;
}
