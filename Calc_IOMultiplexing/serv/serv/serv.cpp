/**********************************************************************************

2015.08.28

- recv : [ int, int, sum{char, int} ] == [ 연산개수, 초기값, sum{연산자, 숫자} ]
- calc : 초기값으로부터 {연산자, 숫자} 받는 순서대로 계산 (곱셈,나눗셈 우선순위 무시)
- send : [ int, int, string ] == [ 결과값, 메시지길이, 메시지 ]
		결과값(음수, 양수, 0)에 따른 메시지 전송
		만약, 클라이언트로부터 종료요청을 받은 경우, 서버가 직접 입력하는 메시지를 전송

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

const char pos[] = "result is positive INT";
const char neg[] = "result is negative INT";
const char zero[] = "result is ZERO";

int setSendMsg(const int & result, char buf[])
{
	*((int*)&buf[0]) = result;

	int ret = 0;
	if (result == 0)
	{
		*((int*)&buf[4]) = ret = strlen(zero);
		strcpy(&buf[8], zero);
	}
	else if (result < 0)
	{
		*((int*)&buf[4]) = ret = strlen(neg);
		strcpy(&buf[8], neg);
	}
	else
	{
		*((int*)&buf[4]) = ret = strlen(pos);
		strcpy(&buf[8], pos);
	}
	return ret + 8;
}

int main()
{
	WSADATA wsaData;
	SOCKET serv, sock;
	SOCKADDR_IN servaddr, clntaddr;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error");

	serv = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serv == INVALID_SOCKET) ErrorHandling("socket() error");

	ZeroMemory(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = 3057;

	int option = true;
	setsockopt(serv, SOL_SOCKET, SO_REUSEADDR, (char*)&option, 4);

	if (bind(serv, (SOCKADDR*)&servaddr, sizeof(servaddr)) == SOCKET_ERROR)
		ErrorHandling("bind() error");

	if (listen(serv, 5) == SOCKET_ERROR)
		ErrorHandling("listen() error");

	puts("server ready");

	char buf[501];
	TIMEVAL timeout;
	FD_SET set, cpy;
	FD_ZERO(&set);
	FD_SET(serv, &set);

	while (1)
	{
		cpy = set;
		timeout.tv_sec = 10;
		timeout.tv_usec = 0;

		int ret = select(cpy.fd_count, &cpy, NULL, NULL, &timeout);

		if (ret == -1) ErrorHandling("select() error");
		if (ret == 0)
		{
			puts("[Calc BOT : select() :: timeout!!");
			continue;
		}

		for (int i = 0; i < cpy.fd_count; ++i)
		{
			if (cpy.fd_array[i] == serv) // client connection
			{
				int clntaddrsize = sizeof(clntaddr);
				sock = accept(serv, (SOCKADDR*)&clntaddr, &clntaddrsize);
				if (sock == INVALID_SOCKET) ErrorHandling("accept() error");
				printf("[Calc BOT : new client %d connect\n", sock);
				FD_SET(sock, &set);
			}
			else // client service
			{
				sock = cpy.fd_array[i];
				int cnt = 0;
				int recvlen = recv(sock, (char*)&cnt, 4, 0);

				if (recvlen == 0) // recv EOF. connect termination request
				{
					printf("input the last MSG to client %d : ", sock);
					gets(buf);
					int sendlen = send(sock, buf, strlen(buf), 0);
					
					// for debugging
					if (sendlen == 0) printf("send length == 0\n");

					shutdown(sock, SD_SEND);
					FD_CLR(sock, &set);
					printf("connection terminate with client %d\n", sock);
					closesocket(sock);
				}
				else // clac & send result to client
				{
					int result, num, msglen;
					char op;
					recv(sock, (char*)&result, 4, 0); // recv initial value

					printf("client request & result : %d", result);

					// printf("init value : %d\n", result);
					for (int i = 0; i < cnt; ++i)
					{
						recv(sock, &op, 1, 0); // recv operator
						recv(sock, (char*)&num, 4, 0); // recv number

						printf(" %c %d", op, num);

						switch (op)
						{
						case '+': result += num; break;
						case '-': result -= num; break;
						case '*': result *= num; break;
						case '/': result /= num; break;
						}
						// printf("%d\n", result);
					}
					printf(" == %d\n", result);
					int sendLen = setSendMsg(result, buf); // send result to client
					send(sock, buf, sendLen, 0);
				}
			}
		}
	}
	closesocket(serv);
	WSACleanup();

	return 0;
}