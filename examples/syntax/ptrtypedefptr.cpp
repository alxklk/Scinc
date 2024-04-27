#include <stdio.h>

typedef char* pchar;

int PrintAll(int cnt, pchar* lines)
{
	for(int i=0;i<cnt;i++)
	{
		printf("Line %i: \"%s\"\n", i, lines[i]);
	}
	return 12;
}

int main()
{
	char *lines[5]={"One","Two","Three","Four","Five"};
	int res=PrintAll(sizeof(lines)/sizeof(pchar), lines);
	printf("Printall returned %i\n", res);
	return 14;
}