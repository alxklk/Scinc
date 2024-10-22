#define __SCINC__

#include "cliser.cpp"
#include "../include/strn.h"

typedef char* pchar;

int main(int argc, pchar* argv)
{
	printf("cli start\n");
	int cliSock=CliInit("127.0.0.1",2777);
	printf("sock=%i\n", cliSock);
	char buf[4096];

	char ask[128]="Client asks";
	CliSend(cliSock, ask, strnlen(ask,128));

	while(true)
	{
		int cnt=CliPoll(cliSock, buf, 4096);
		printf("poll: res=%i\n", cnt);
		if(cnt>0)
		{
			printf("Serv responded '%s'\n", buf);
		}
		Wait(.1);
	}
	CliDone(cliSock);
    return 0;
}