#include <stdio.h>

int main()
{
	puts("Enter s:");
	char s[100];
	fgets(s,100,stdin);
	for(int i=0;i<100;i++)if(s[i]=='\n')s[i]='\0';
	for(int i=0;i<5;i++)
		printf("'%s' ", s);
	puts("\n");
	return 0;
}