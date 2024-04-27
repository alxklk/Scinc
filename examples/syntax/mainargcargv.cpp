#include <stdio.h>

typedef char* pchar;

int main(int argc, pchar* argv)
{
	// printf("__crt00_argc @%i __crt00_argvBuf @%i __crt00_argv @%i\n", &__crt00_argc, __crt00_argvBuf, __crt00_argv);
	// printf("argc=%i\n",argc);
	// printf("__crt00_argc=%i\n",__crt00_argc);
	// printf("argv=%i\n",argv);
	// for(int i=0;i<5;i++)
	// {
	// 	printf("argv[%i]=%i\n",i, argv[i]);
	// }
	// printf("buf:\n");
	// for(int i=0;i<512;i++)
	// {
	// 	printf("%c",__crt00_argvBuf[i]);
	// }
	// printf("\n");

	for(int i=0;i<argc;i++)
	{
		printf("Line %i: \"%s\"\n", i, argv[i]);
	}
	return 12;
}
