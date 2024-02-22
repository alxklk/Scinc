#include <stdio.h>
#include <math.h>

#define M(x) B(x)

int main()
{
#define B(a) sin(a)
	printf("%f\n", M(.1));
#define B(c) cos(c)
	printf("%f\n", M(.1));
	return 0;
}
