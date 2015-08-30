#include <iostream>
#include <WinSock2.h>
#include <Windows.h>
#include <signal.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

void ErrorHandling(char * msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

void exitSignal(int a)
{
	if (a == SIGINT) puts("signal == SIGINT");
	Sleep(1500);
}

int main()
{
	bool connected = false;
	WSADATA wsaData;
	SOCKET sock;
	SOCKADDR_IN servaddr;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error");

	ZeroMemory(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servaddr.sin_port = 3357;

	sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) ErrorHandling("socket() error");

	if (connect(sock, (SOCKADDR*)&servaddr, sizeof(sockaddr)) == SOCKET_ERROR)
		ErrorHandling("connect() error");

	char buf[501], recvbuf[501]; buf[4] = '[';

	connected = true;
	puts("connection complete.....");

	signal(SIGINT, exitSignal);

	printf("input the nickName : ");
	gets(&buf[5]);
	strcpy(&buf[5 + strlen(&buf[5])], "] : ");
	puts("-----------------------------------------");

	int nicklen = strlen(&buf[4]);

	while (1)
	{
		printf("input MSG (exit q/Q) : ");
		gets(&buf[4 + nicklen]);

		if (buf[4 + nicklen] == 'q' || buf[4 + nicklen] == 'Q') break;

		int msglen = strlen(&buf[4]);
		int len = *((int*)&buf[0]) = msglen; // MSG(nick + msg) length

		send(sock, buf, 4 + msglen, 0); // len(4) + msglen + null(1)

		int total = 0;
		while (total < len)
		{
			int recvlen = recv(sock, &recvbuf[total], 500, 0);
			if (recvlen == -1) ErrorHandling("recv() error");
			total += recvlen;
		}
		recvbuf[total] = NULL;
		printf("MSG from server : %s\n", recvbuf);
	}
	*((int*)&buf[0]) = 0;
	send(sock, buf, 4, 0); // send 'Q/q' to server
	puts("terminate...");
	//shutdown(sock, SD_SEND);
	// 서버와 데이터 통신을 하는 위의 while loop에서
	// 클라이언트는 자신이 응답받을 MSG의 길이를 알고있으므로 (echo서비스 특성상)
	// 자신이 데이터를 어디까지 받아야 하는지(len) 정확하게 알고있음.
	// 따라서, 서버로부터 데이터를 모두 전송받는 메커니즘까지 위의 while문에 포함됨
	// 즉, 클라이언트가 종료하려 한다는것은 서버로부터 받을 데이터도 없음을 의미.
	// => shutdown을 통한 half-close과정을 굳이 할 필요 없이 바로 종료해버리면 됨.

	closesocket(sock);
	WSACleanup();
	system("pause");

	return 0;
}
