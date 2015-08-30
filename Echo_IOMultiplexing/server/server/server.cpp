#include <iostream>
#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

void ErrorHandling(char * msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

int main()
{
	WSADATA wsaData;
	SOCKET serv, sock;
	SOCKADDR_IN servaddr, clntaddr;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error");

	ZeroMemory(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = 3357;

	serv = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serv == INVALID_SOCKET) ErrorHandling("socket() error");

	int option, optlen;
	option = 1;
	optlen = sizeof(option);
	setsockopt(serv, SOL_SOCKET, SO_REUSEADDR, (char*)&option, optlen);

	if (bind(serv, (SOCKADDR*)&servaddr, sizeof(servaddr)) == SOCKET_ERROR)
		ErrorHandling("bind() error");

	if (listen(serv, 2) == SOCKET_ERROR)
		ErrorHandling("listen() error");
	printf("listenning sock : %d\n", serv);

	char buf[501] = { 0 };
	TIMEVAL timeout;
	fd_set set, tmp;
	FD_ZERO(&set);
	FD_SET(serv, &set);

	puts("---------------------------------------------------");

	while (1)
	{
		tmp = set;
		timeout.tv_sec = 10;
		timeout.tv_usec = 0;

		int ret = select(tmp.fd_count, &tmp, 0, 0, &timeout);
		if (ret == SOCKET_ERROR)
		{
			puts("select() error");
			break;
		}
		if (ret == 0)
		{
			puts("[CHAT BOT] : select :: timeout");
			continue;
		}

		for (int i = 0; i < tmp.fd_count; ++i)
		{
			if (FD_ISSET(tmp.fd_array[i], &tmp) == false) continue;

			if (tmp.fd_array[i] == serv) // new client arrival
			{
				int clntaddrsize = sizeof(clntaddr);
				sock = accept(serv, (SOCKADDR*)&clntaddr, &clntaddrsize);
				printf("[CHAT BOT] : connection complete client %d\n", sock);
				if (sock == INVALID_SOCKET) ErrorHandling("accept() error");
				FD_SET(sock, &set);
			}
			else // echo service
			{
				sock = tmp.fd_array[i]; // clnt socket

				int len = 0;
				recv(sock, (char*)&len, 4, 0); // recv string length
				if (len == 0)
				{
					printf("[CHAT BOT] : bye bye client %d\n", sock);
					FD_CLR(sock, &set);
					closesocket(sock); // close client socket (release the resource to OS)
					continue;
				}

				int total = 0;
				while (total < len)
				{
					int recvlen = recv(sock, &buf[total], 500, 0);
					if (recvlen == -1) ErrorHandling("recv() error");
					total += recvlen;
				}
				buf[total] = NULL;
				printf("%s\n", buf);
				send(sock, buf, strlen(buf), 0);
			}
		}
	}
	puts("---------------------------------------------------");
	puts("[CHAT BOT] : server terminate...");
	closesocket(serv);
	WSACleanup();

	return 0;
}
