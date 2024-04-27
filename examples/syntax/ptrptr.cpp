#include <stdio.h>


void PrintAll(int cnt, char** lines)
{
	for(int i=0;i<cnt;i++)
	{
		printf("Line %i: \"%s\"\n", i, lines[i]);
	}
}


int main()
{
	char *lines[5]={"One","Two","Three","Four","Five"};
	PrintAll(sizeof(lines)/sizeof(char*), lines);
	return 0;
}