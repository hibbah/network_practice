/**********************************************************************************

2015.08.28

- send : [ int, int, sum{char, int} ] == [ 연산개수, 초기값, sum{연산자, 숫자} ]
- recv : [ int, int, string ] == [ 결과값, 메시지길이, 메시지 ]
- exit condition : 연산개수에 -1 입력

**********************************************************************************/

#include <iostream>
#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

void ErrorHandling(char * msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

void exitProgram(const SOCKET & sock, char buf[])
{
	shutdown(sock, SD_SEND); // send EOF to server
	
	int idx = 0, recvlen;
	while ((recvlen = recv(sock, &buf[idx], 500 - idx, 0)) != 0)
		idx += recvlen;
	buf[idx] = NULL;

	printf("last MSG from server : %s\n", buf);
	puts("Half-close complete");
	puts("connection terminate");
}

int main()
{
	WSADATA wsaData;
	SOCKET sock;
	SOCKADDR_IN servaddr;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error");

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) ErrorHandling("socket() error");

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servaddr.sin_port = 3057;

	if (connect(sock, (SOCKADDR*)&servaddr, sizeof(servaddr)) == SOCKET_ERROR)
		ErrorHandling("connect() error");

	char buf[501];
	while (1)
	{
		printf("input the Cnt (EXIT : -1) : ");
		scanf("%d", (int*)&buf[0]);
		int cnt = *((int*)&buf[0]);
		
		if (cnt == -1) // program terminate
		{
			exitProgram(sock, buf);
			break;
		}

		printf("input the initval : ");
		scanf("%d", (int*)&buf[4]); getchar();

		int idx = 8;
		for (int i = 0; i < cnt; ++i)
		{
			printf("input the %dth [operator, num] : ", i + 1);
			scanf("%c%d", &buf[idx], (int*)&buf[idx + 1]); getchar();
			idx += 5;
		}

		int sendlen = send(sock, buf, 8 + 5 * cnt, 0);
		
		// for debugging
		if (sendlen == 0) printf("send length == 0\n");

		int result, recvlen = 0, total = 0, msglen = 0;
		recv(sock, (char*)&result, 4, 0);
		recv(sock, (char*)&msglen, 4, 0);
		while (total < msglen)
		{
			recvlen = recv(sock, &buf[total], 500 - idx, 0);
			total += recvlen;
		}
		buf[total] = NULL;
		printf("result & MSG : [ %d, %s ]\n", result, buf);
		puts("-----------------------------------------------------");
	}
	closesocket(sock);
	WSACleanup();

	return 0;
}