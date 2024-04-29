#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>

void strncat(char* res, const char* left, const char* right, int n)
{
	const char* src=left;
	int srci=0;
	bool first=true;
	int i=0;
	while(true)
	{
		if(i==n)
		{
			break;
		}
		if(src[srci]==0)
		{
			if(first)
			{
				first=false;
				src=right;
				srci=0;
			}
			else
			{
				break;
			}
		}
		res[i]=src[srci];
		i++;
		srci++;
	}
	res[i]=0;
}

int strnlen(const char* s, int n)
{
	for(int i=0;i<n;i++)
	{
		if(s[i]==0)
			return i;
	}
	return n;
}

char strnlast(const char* s, int n)
{
	if(s[0]==0)return s[0];

	for(int i=1;i<n;i++)
	{
		if(s[i]==0)
			return s[i-1];
	}
	return s[n-1];
}

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
