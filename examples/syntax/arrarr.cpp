#include <stdio.h>

struct S0
{
	int type;
};

//typedef S0 A0[4];
using A0 = S0[4];

int main()
{
	A0 a0;
	for (int i = 0; i < 4; i++)
	{
		a0[i].type = i;
	}
	for (int i = 0; i < 4; i++)
	{
		printf("%d\n", a0[i].type);
	}
	
	return 0; 
}
