#ifdef __SCINC__

#define NAME(name) #name"_wrapper"
#define WRAP(rettype, name, args, ...) [[scinc::dynlink("cliser_dl", NAME(name) )]]rettype name args;

#else

#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

#include "../../include/ScincVM.h"
#include "../../include/ScincBinder.h"
#define WRAP(rettype, name, args, ...) extern "C" EXPORT int name##_wrapper(ScincVM* s, void*){ScincBoundCall(s,+[]args ->rettype __VA_ARGS__);return 0;}

#include <stdio.h>
#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

	int ServInit_Impl(int port)
	{
		int sock=socket(AF_INET, SOCK_STREAM, 0);
		if(sock==-1)
		{
			return -1;
		}
		int reuseAddressOpt=1;
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&reuseAddressOpt, sizeof(reuseAddressOpt));
		int keeAliveOpt=0;
		setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char*)&keeAliveOpt, sizeof(keeAliveOpt));

		struct sockaddr_in serverAddr;
		memset(&serverAddr, 0, sizeof(serverAddr));
		serverAddr.sin_family=AF_INET;
		serverAddr.sin_addr.s_addr=INADDR_ANY;
		serverAddr.sin_port=htons(port);

		if(bind(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr))==-1)
		{
			close(sock);
			fprintf(stderr,"Error in bind\n");
			return -1;
		}

		if(listen(sock, 5)==-1)
		{
			fprintf(stderr,"Error in listen\n");
			return -1;
		}


		return sock;
	}

	int ServPoll_Impl(int servSock, int& clientSock, std::string& request)
	{
		if(clientSock==-1)
		{
			clientSock=accept(servSock, 0, 0);
			if(clientSock==-1)
				return 0;
		}
		printf("poll\n");

		request.clear();
		char buffer[4096];
		//while(true)
		{
			int bytesRead=recv(clientSock, buffer, sizeof(buffer), 0);
			if (bytesRead <= 0)
			{
				close(clientSock);
				clientSock=-1;
				return bytesRead;
			}
			request.append(std::string_view(buffer,bytesRead));
		}
		return request.size();
	}

	int ServSend_Impl(int clientSock, const char* data, int size)
	{
		ssize_t bytesSent=send(clientSock, data, size, 0);
		return bytesSent;
	}


	int CliInit_Impl(const char* ipaddr, int port)
	{
		int sock=socket(AF_INET, SOCK_STREAM, 0);
		if(sock==-1)
		{
			fprintf(stderr, "Error creating socket");
			return -1;
		}

		if(0)
		{
			int reuseAddressOpt=0;
			setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&reuseAddressOpt, sizeof(reuseAddressOpt));
			int keeAliveOpt=0;
			setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char*)&keeAliveOpt, sizeof(keeAliveOpt));
			// Set the socket to non-blocking mode
			int flags=fcntl(sock, F_GETFL, 0);
			if(flags==-1)
			{
				fprintf(stderr, "Error getting socket flags.");
				return 1;
			}
			fcntl(sock, F_SETFL, flags|O_NONBLOCK);
		}

		struct sockaddr_in server_addr;
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family=AF_INET;
		server_addr.sin_port=htons(port);
		if(inet_pton(AF_INET, ipaddr, &server_addr.sin_addr) <= 0)
		{
			fprintf(stderr, "Invalid server address");
			close(sock);
			return -1;
		}
		if(connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr))==-1)
		{
			fprintf(stderr, "Error in connect\n");
			return -1;
		}
		fprintf(stderr, "Init OK\n");
		return sock;
	}

	int CliSend_Impl(int cliSock, const char* data, int size)
	{
		fprintf(stderr, "send(%i, '%.*s', %i)\n", cliSock, size, data, size);
		int res=send(cliSock, data, size, 0);
		if(res==-1)
		{
			fprintf(stderr, "Error sending message\n");
			close(cliSock);
			return -1;
		}
		return res;
	}

	int CliPoll_Impl(int cliSock, std::vector<char>& data)
	{
		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 1;

		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(cliSock, &readfds);

		int ready=select(cliSock+1, &readfds, nullptr, nullptr, &timeout);
		if(ready==-1)
		{
			fprintf(stderr, "Error in select().");
			close(cliSock);
			cliSock=-1; // probably need to reopen
			return -1;
		} else if (ready == 0) {
			return 0;
		}

		char response[1024];
		//socklen_t server_addr_len=sizeof(server_addr);
		int bytes_received=recv(cliSock, response, sizeof(response)-1, 0);
		if(bytes_received==-1)
		{
			fprintf(stderr, "Error receiving response");
			close(cliSock);
			cliSock=-1; // probably need to reopen
			return -1;
		}

		data.clear();
		for(int i=0;i<bytes_received;i++)
		{
			data.push_back(response[i]);
		}
		return bytes_received;
	}

#endif

WRAP(int, ServInit, (int port), {return ServInit_Impl(port);})
WRAP(int, ServPoll, (int servSock, int* clientSock, char* buf, int maxSize), {std::string s; int res=ServPoll_Impl(servSock, *clientSock, s); int cnt=std::min(maxSize,int(s.size()));for(int i=0;i<cnt-1;i++){buf[i]=s[i];}buf[cnt]=0;return cnt;} )
WRAP(int, ServSend, (int clientSock, char* data, int size), {return ServSend_Impl(clientSock, data, size);})
WRAP(int, ServDone,(int servSock),{close(servSock);return 0;})
WRAP(int, CliInit,(char* addr, int port),{return CliInit_Impl(addr, port);})
WRAP(int, CliPoll,(int cliSock, char* buf, int maxSize), {std::vector<char>data;int res=CliPoll_Impl(cliSock, data); int cnt=std::min(maxSize,int(data.size()));for(int i=0;i<cnt-1;i++){buf[i]=data[i];}buf[cnt]=0;return cnt;})
WRAP(int, CliSend,(int cliSock, char* data, int size),{return CliSend_Impl(cliSock, data, size);})
WRAP(int, CliDone,(int cliSock),{close(cliSock);return 0;})
