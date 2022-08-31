#include <stdio.h>

#ifdef __SCINC__
typedef __native__(8) FILE_ptr;
__dyn_lib_import__("fileio_dl","fopen_wrapper")FILE_ptr fopen_(char* fname, char* mode);
__dyn_lib_import__("fileio_dl","fclose_wrapper")int fclose_(FILE_ptr f);
__dyn_lib_import__("fileio_dl","fwrite_wrapper")int fwrite_(void* ptr, int size, int n, FILE_ptr f);
#else

typedef FILE* FILE_ptr;

#endif

int main()
{
	char buf[16];
	//for(int i=0;i<16;i++)buf[i]='a'+i;
	printf("Hi %f\n", sqrt(2.));
	FILE_ptr f=fopen_("01234.txt","wb");
	int written=fwrite_((void*)&(buf[0]),1,16,f);
	printf("Written %i bytes\n", written);
	fclose_(f);
	return 0;
}