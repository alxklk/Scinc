#include <stdio.h>

#define L(x) # x 
#define N(x) L(x)

int main()
{
	printf("1 '%s'\n", L(1));
	printf("2 '%s'\n", L(3));
	printf("3 '%s'\n", L(__LINE__));
	printf("4 '%s'\n", N(__LINE__));
	printf("5 '%s'\n", L(a __LINE__ c));
	printf("6 '%s'\n", N(a __LINE__ c));
	printf("7 '%s'\n", N(a-b	));
	printf("8 '%s'\n", N(     a -  		 b));
	printf("9 '%s'\n", L(L(a)));
	printf("10 '%s'\n", N(L(a	)));
	printf("11 '%s'\n", N(N(a)));
	printf("12 '%s'\n", L("123"));
	printf("13 '%s'\n", N("123"));
	printf("14 '%s'\n", L(L("234")));
	printf("15 '%s'\n", N(L("234")));
	printf("16 '%s'\n", N(N("234")));
	printf("17 '%s'\n", L("1\"\'\"\'23"));
	printf("18 '%s'\n", N("1\"\'\"\'23"));
	printf("19 '%s'\n", N(N(L(L("2\t\n\t\0\0\"\'\"\'34")))));
	printf("20 '%s'\n", N(L("2\"\'\"\'34")));
	printf("20 '%s'\n", N(N("2\"\'\"\'34")));
	return 0;
}
