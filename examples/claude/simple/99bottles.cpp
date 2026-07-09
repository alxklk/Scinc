#include <stdio.h>

// Classic "99 Bottles of Beer" song.
int main()
{
	for (int n = 99; n > 0; n--)
	{
		printf("%d bottles of beer on the wall, %d bottles of beer.\n", n, n);
		if (n == 1)
			printf("Take one down and pass it around, no more bottles of beer on the wall.\n\n");
		else if (n == 2)
			printf("Take one down and pass it around, 1 bottle of beer on the wall.\n\n");
		else
			printf("Take one down and pass it around, %d bottles of beer on the wall.\n\n", n - 1);
	}
	printf("No more bottles of beer on the wall, no more bottles of beer.\n");
	printf("Go to the store and buy some more, 99 bottles of beer on the wall.\n");
	return 0;
}
