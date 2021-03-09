#include <stdio.h>

#ifdef __SCINC__
typedef __native__(8) FILE_ptr;
__dyn_lib_import__("fileio_dl","fopen_wrapper")FILE_ptr fopen(char* fname, char* mode);
__dyn_lib_import__("fileio_dl","fclose_wrapper")int fclose(FILE_ptr f);
__dyn_lib_import__("fileio_dl","fwrite_wrapper")int fwrite(void* ptr, int size, int n, FILE_ptr f);
#else

typedef FILE* FILE_ptr;

#endif

int main()
{
	char buf[16];
	for(int i=0;i<16;i++)buf[i]='a'+i;
	FILE_ptr f=fopen("01234.txt","wb");
	int written=fwrite(buf,1,16,f);
	printf("Written %i bytes\n", written);
	fclose(f);
	return 0;
}