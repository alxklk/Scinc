#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>

void strglue(char* res, const char* left, const char* right, int n)
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

int listdir(const char *path) 
{
	dirent *entry;
	DIR *dp;
	dp=opendir(path);
	if(dp==0){
		printf("opendir error\n");
		return -1;
	}
	while((entry=readdir(dp)))
	{
		struct stat st;
		char namebuf[256];
		strglue(namebuf,path,"/",256);
		strglue(namebuf,namebuf,entry->d_name,256);
		if(stat(namebuf, &st)!=0)
		{
			//perror("Stat error ");
		}
		else
		{
			printf("%i ",st.st_mode);
			if(S_ISDIR(st.st_mode))printf(" + ");

		}
		printf("%s\n",namebuf);
	}
	closedir(dp);
	return 0;
}

int main()
{
	listdir("../");
	return 0;
}
