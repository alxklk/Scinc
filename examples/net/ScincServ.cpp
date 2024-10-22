#define __SCINC__

#include "cliser.cpp"
#include "../include/strn.h"

typedef char* pchar;

int main(int argc, pchar* argv)
{
	printf("Serv start\n");
	int sock=ServInit(2777);
	printf("sock=%i\n", sock);
	char buf[4096];
	int cliSock=-1;

	while(true)
	{
		int cnt=ServPoll(sock, &cliSock, buf,4096);
		printf("poll: res=%i, clisock=%i\n", cnt, cliSock);
		if(cnt>0)
		{
			printf("Serv received '%s'\n", buf);
			char response[128]="Server responce";
			ServSend(cliSock, response, strnlen(response,4096));
		}
		Wait(.1);
	}
	ServDone(sock);
    return 0;
}