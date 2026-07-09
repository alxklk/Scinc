#include <stdio.h>

// Sieve of Eratosthenes: print primes below 100.
// Array sizes in Scinc must be literal/macro constants (not `const int`),
// so N is a preprocessor macro. We use an int array rather than char:
// a `char` array element read directly in a boolean context (`if(sieve[i])`)
// is currently always truthy in Scinc, so `int` avoids that pitfall.
#define N 100

int main()
{
	int sieve[N];
	for (int i = 0; i < N; i++)
		sieve[i] = 1;
	sieve[0] = 0;
	sieve[1] = 0;

	for (int i = 2; i * i < N; i++)
		if (sieve[i])
			for (int j = i * i; j < N; j += i)
				sieve[j] = 0;

	int count = 0;
	for (int i = 2; i < N; i++)
		if (sieve[i])
		{
			printf("%d ", i);
			count++;
		}
	printf("\n%d primes below %d\n", count, N);
	return 0;
}
