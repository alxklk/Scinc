#include <stdio.h>

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

int main()
{
	char buf[128];
	strglue(buf,"1","2", 128)      ;printf("%s\n", buf);
	strglue(buf,"","2", 128)       ;printf("%s\n", buf);
	strglue(buf,"1","", 128)       ;printf("%s\n", buf);
	strglue(buf,"1111","2222", 128);printf("%s\n", buf);
	strglue(buf,"11","22", 3)      ;printf("%s\n", buf);
	return 0;
}
