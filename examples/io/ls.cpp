#include <stdio.h>
#include <dirent.h>

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
		printf("%s\n",entry->d_name);
	}
	closedir(dp);
	return 0;
}

int main()
{
	listdir("../");
	return 0;
}
