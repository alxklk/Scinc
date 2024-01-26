#include <stdio.h>

int main()
{
	FILE* fout=((FILE*)3);
	fputs("12345", fout);
	return 0;
}