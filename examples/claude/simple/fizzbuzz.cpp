#include <stdio.h>

// FizzBuzz: numbers 1..100, but multiples of 3 -> "Fizz",
// multiples of 5 -> "Buzz", multiples of both -> "FizzBuzz".
int main()
{
	for (int i = 1; i <= 100; i++)
	{
		if (i % 15 == 0)
			printf("FizzBuzz\n");
		else if (i % 3 == 0)
			printf("Fizz\n");
		else if (i % 5 == 0)
			printf("Buzz\n");
		else
			printf("%d\n", i);
	}
	return 0;
}
