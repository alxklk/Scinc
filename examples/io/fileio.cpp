#include <stdio.h>
#include "../include/strn.h"

typedef char* pchar;

int main(int argc, const pchar* argv)
{
	FILE* f=0;
	bool needClose=true;
	bool useStdfile=false;
	char* fn;
	if(argc<2)
	{
		fn="fileio.txt";
		printf("Defaulting file name to '%s'\n", fn);
	}
	else
	{
		if(strneq(argv[1],"stderr",10))
		{
			printf("Writing to standard error stream\n");
			f=stderr;
			needClose=false;
			useStdfile=true;
		}
		else if(strneq(argv[1],"stdout",10))
		{
			printf("Writing to standard output stream\n");
			f=stdout;
			needClose=false;
			useStdfile=true;
		}
		else
		{
			fn=argv[1];
		}
	}
	if(!useStdfile)
	{
		f=fopen(fn,"rb");
		if(f)
		{
			printf("File %s exists. Bye.\n", fn);
			fclose(f);
			return 12;
		}
		printf("Writing to %s\n", fn);
		f=fopen(fn,"wb");
		if(!f)
		{
			printf("Failed to open file %s for write. Bye.\n", fn);
			fclose(f);
			return 14;
		}
	}
	fprintf(f,"Check fprintf. int 123=%i, float 3.14=%4.2f, string 'abc'='%s'\n",123,3.14,"abc");
	int pos=ftell(f);
	fwrite((void*)"0000000000",1,10,f);
	fputs("\nWritten 10 zeros\n",f);
	fprintf(f,"Now seek back to pos %i and rewrite first 4 of them with 1s\n",pos);
	fseek(f,pos,SEEK_SET);
	fwrite((void*)"1111",1,4,f);

	if(needClose)
	{
		fclose(f);
	}
	return 0;
}
