#include <stdio.h>

int main()
{
	bool x=false;
	if(x!) // should catch syntax error!
	{
		printf("This is weird\n");
	}
    return 0;
}

