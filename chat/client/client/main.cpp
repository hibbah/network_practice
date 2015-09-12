#pragma comment(lib, "ws2_32.lib")

#include <stdio.h>
#include <WinSock2.h>
#include <process.h>
#include <Windows.h>
#include <conio.h>
#include <string>

const char * IP = "127.0.0.1";
const int PORT = 3057;
const int BUF = 2001;

class ITEM
{
public:
	SOCKET sock;
	int idx;

	ITEM() {}
	ITEM(const SOCKET & s, const int & i) : sock(s), idx(i) {}
};

void errorHandling(const char *);
int setNickName();
SOCKET initSocket(SOCKADDR_IN &);
unsigned WINAPI sendMsg(void * arg); // send MSG to server
unsigned WINAPI recvMsg(void * arg); // recv MSG from server

char recvbuf[BUF];
char buf[BUF];
int cursor = 4;

int main()
{
	WSADATA wsaData;
	SOCKET sock;
	SOCKADDR_IN servaddr;
	HANDLE sendThread, recvThread;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		errorHandling("WSAStartup() error");

	int msgidx = setNickName(); // chatting NICKNAME
	if((sock = initSocket(servaddr)) == SOCKET_ERROR)
		errorHandling("initSocket() error");
	
	if (connect(sock, (SOCKADDR*)&servaddr, sizeof(servaddr)) == SOCKET_ERROR) // connect to server
		errorHandling("connect() error");

	puts("[[ CHATBOT ]] : connection complete");
	puts("[[ CHATBOT ]] : if you want Quit - input \'q\'/\'Q\'");

	// send & recv routine start
	ITEM item(sock, msgidx); // data for msg send/recv
	sendThread = (HANDLE)_beginthreadex(NULL, 0, sendMsg, (void*)&item, 0, NULL);
	recvThread = (HANDLE)_beginthreadex(NULL, 0, recvMsg, (void*)&item, 0, NULL);

	// resource release
	WaitForSingleObject(sendThread, INFINITE);
	WaitForSingleObject(recvThread, INFINITE);
	closesocket(sock);
	WSACleanup();
	puts("[[ CHATBOT ]] : program terminate");

	return 0;
}

unsigned WINAPI sendMsg(void * arg)
{
	ITEM item = *((ITEM*)arg);
	SOCKET sock = item.sock;
	const int startidx = item.idx;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { 0, cursor + 3 });

	while (1)
	{
		fgets(&buf[startidx], BUF - startidx, stdin);
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { 0, cursor + 3 });
		printf("                                                                                ");
		if (buf[startidx] == 'q' || buf[startidx] == 'Q')
		{
			closesocket(sock);
			break;
		}
		send(sock, buf, strlen(buf), 0);
	}
	return 0;
}

unsigned WINAPI recvMsg(void * arg)
{
	ITEM item = *((ITEM*)arg);
	SOCKET sock = item.sock;
	int idx = item.idx;

	while (1)
	{
		int msglen = recv(sock, recvbuf, 2000, 0);
		if (msglen == -1) break;
		recvbuf[msglen] = 0;
		
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { 0, cursor++ });
		fputs(recvbuf, stdout);
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { 0, cursor + 3 });
	}
	return 0;
}

SOCKET initSocket(SOCKADDR_IN & addr)
{
	SOCKET sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(IP);
	addr.sin_port = PORT;
	return sock == INVALID_SOCKET ? SOCKET_ERROR : sock;
}

int setNickName()
{
	char tmp[200];
	printf("[[ CHATBOT ]] : input the NICKNAME : ");
	gets(tmp);
	sprintf(buf, "[ %s ] : ", tmp);
	return strlen(buf);
}

void errorHandling(const char * msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}