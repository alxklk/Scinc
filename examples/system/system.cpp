#include <stdio.h>

#include "system.h"

int main()
{
	const char* command = "ls / ; sleep 2 ; pwd ; echo Finished";
	int rc = system(command);
	printf("system(\"%s\") returned %d\n", command, rc);
	return 0;
}
