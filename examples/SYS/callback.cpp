#include <stdio.h>

int n=0;

int CallBack()
{
	if(!n%50)printf("Callback %i\n", n);
	n++;
	return 123;
}


int main()
{
	while(true)
	{
		Poll();
	}
	return 0;
}