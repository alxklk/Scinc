#include <stdio.h>

// Adopted example from Rosetta Code

void roman(char *s, int n)
{
	if(!n)
	{
		puts("Roman numeral for zero requested.");
		return;
	}
	int i=0;
	#define digit(loop, num, c) loop(n>=num){s[i++]=c;n-=num;}
	#define digits(loop, num, c1, c2) loop(n>=num){s[i++]=c1;s[i++]=c2;n-=num;}

	digit(while, 1000, 'M');
	digits(if, 900, 'C', 'M');
	digit(if, 500, 'D');
	digits(if, 400, 'C', 'D');
	digit(while, 100, 'C');
	digits(if, 90, 'X', 'C');
	digit(if, 50, 'L');
	digits(if, 40, 'X', 'L');
	digit(while, 10, 'X');
	digits(if, 9, 'I', 'X');
	digit(if, 5, 'V');
	digits(if, 4, 'I', 'V');
	digit(while, 1, 'I');

	#undef digit
	#undef digits

	s[i]=0;
}

int main()
{
	char buffer[16];
	int i;
	for(i=1;i<100;++i)
	{
		roman(buffer, i);
		printf("%4u: % 8s ",i,buffer);
		if (i % 6 == 0)
			puts("");
	}
	return 0;
}