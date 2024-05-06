#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include "../include/strn.h"


int listdir(const char *path) 
{
	printf("Directory list for dir %s\n", path);	
	printf("          Size        Name\n");	
	dirent *entry;
	DIR *dp;
	dp=opendir(path);
	if(dp==0)
	{
		fprintf(stderr,"opendir error\n");
		return -1;
	}
	int files=0;
	int folders=0;
	while(true)
	{
		entry=readdir(dp);
		if(!entry)
		{
			printf("%i files %i folders\n", files, folders);
			break;
		}
	
		struct stat st;
		char namebuf[256]={0};
		if(strnlast(path,256)!='/')
			strncat(namebuf,path,"/",256);
		else
			strncat(namebuf,path,"",256);
		strncat(namebuf,namebuf,entry->d_name,256);
		if(stat(namebuf, &st)!=0)
		{
			fprintf(stderr, "stat error\n");
		}
		else
		{
			if(S_ISDIR(st.st_mode)){printf(" d ");folders++;}
			else                   {printf(" - ");files++;}
			printf(" % 10i bytes ", (int)st.st_size);
		}
		printf("%s\n",entry->d_name);
	}
	closedir(dp);
	return 0;
}

typedef char* pchar;

int main(int argc, const pchar* argv)
{
	if(argc<2)
	{
		listdir(".");
	}
	else
	{
		listdir(argv[1]);
	}
	return 0;
}
