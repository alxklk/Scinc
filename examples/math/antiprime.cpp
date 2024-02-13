// from Rosetta code: http://rosettacode.org/wiki/Anti-primes#C
#include <stdio.h>

int countDivisors(int n) {
	int i;
	int count;
	if (n < 2) return 1;
	count = 2; // 1 and n
	for (i = 2; i <= n/2; ++i) {
		if (n%i == 0) ++count;
	}
	return count;
}

int main() {
	int n;
	int d;
	int maxDiv = 0;
	int count = 0;
	printf("The first 40 anti-primes are:\n");
	for (n = 1; count < 40; ++n) {
		d = countDivisors(n); 
		if (d > maxDiv) {
			printf("%d ", n);
			maxDiv = d;
			count++;
		}
	}
	printf("\n"); 
	return 0;
}