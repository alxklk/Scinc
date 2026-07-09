#include <stdio.h>

// Print the first 20 Fibonacci numbers (iterative).
// Note: Scinc supports int/float/double/char but not long/short/unsigned,
// so we stay within int range (fib(19) = 4181 fits comfortably).
int main()
{
	int a = 0;
	int b = 1;
	for (int i = 0; i < 20; i++)
	{
		printf("%d ", a);
		int next = a + b;
		a = b;
		b = next;
	}
	printf("\n");
	return 0;
}
