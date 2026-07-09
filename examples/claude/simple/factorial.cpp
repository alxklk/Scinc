#include <stdio.h>

// Recursive factorial - exercises user-defined functions and recursion.
// Stops at 12 because 13! overflows 32-bit int (Scinc has no long).
int factorial(int n)
{
	if (n <= 1)
		return 1;
	return n * factorial(n - 1);
}

int main()
{
	for (int n = 1; n <= 12; n++)
		printf("%2d! = %d\n", n, factorial(n));
	return 0;
}
