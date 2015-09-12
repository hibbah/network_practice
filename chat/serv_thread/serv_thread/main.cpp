#pragma comment(lib, "ws2_32.lib")

#include <stdio.h>
#include <WinSock2.h>
#include <process.h>
#include <set>
#include <vector>

using namespace std;

const int PORT = 3057;
const int BUF = 2001;

void errorHandling(const char *);
SOCKET initSocket(SOCKADDR_IN &);
unsigned WINAPI clntHandler(void *);

set <SOCKET> clntsock; // client socket handle set
HANDLE mutex;
char buf[BUF];

int main()
{
	WSADATA wsaData;
	SOCKET serv, sock;
	SOCKADDR_IN servaddr, clntaddr;
	
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		errorHandling("WSAStartup() error");

	if ((serv = initSocket(servaddr)) == SOCKET_ERROR)
		errorHandling("initSocket() error");
	if (bind(serv, (SOCKADDR*)&servaddr, sizeof(servaddr)) == SOCKET_ERROR)
		errorHandling("bind() error");
	if (listen(serv, 5) == SOCKET_ERROR)
		errorHandling("listen() error");

	mutex = CreateMutex(NULL, FALSE, NULL);

	puts("[[ CHATBOT ]] : server ready");

	while (1)
	{
		int clntaddrsize = sizeof(clntaddr);
		sock = accept(serv, (SOCKADDR*)&clntaddr, &clntaddrsize);
		if (sock == INVALID_SOCKET) errorHandling("accept() error");

		WaitForSingleObject(mutex, INFINITE); // enter CriticalSection (use mutex)
		clntsock.insert(sock); // insert to newSocketHandle in client socket set
		ReleaseMutex(mutex); // leave CriticalSection (release mutex)

		printf("[[ CHATBOT ]] : new client %d connected\n", sock);
		printf("[[ CHATBOT ]] : new client %d IP : %s\n", sock, inet_ntoa(clntaddr.sin_addr));
		printf("[[ CHATBOT ]] : current user : %d\n", clntsock.size());

		_beginthreadex(NULL, 0, clntHandler, (void*)&sock, 0, NULL); // service
	}

	// resource release
	// WaitForMultipleObjects(threads.size(), &threads[0], TRUE, INFINITE); // Windows의 쓰레드는 함수 종료와 함께 자동 소멸
	CloseHandle(mutex);
	closesocket(serv);
	WSACleanup();

	return 0;
}

unsigned WINAPI clntHandler(void * arg)
{
	SOCKET sock = *((SOCKET*)arg);
	int msglen = 0;

	while (1)
	{
		msglen = recv(sock, buf, 2000, 0);
		if (msglen == -1) break;

		WaitForSingleObject(mutex, INFINITE); // enter CS
		set<SOCKET>::iterator it;
		for (it = clntsock.begin(); it != clntsock.end(); ++it)
			send(*it, buf, msglen, 0);
		ReleaseMutex(mutex); // leave CS
	}

	WaitForSingleObject(mutex, INFINITE); // enter CS
	clntsock.erase(sock);
	ReleaseMutex(mutex);

	printf("[[ CHATBOT ]] : client %d is terminated\n", sock);
	printf("[[ CHATBOT ]] : current user : %d\n", clntsock.size());

	// resource(client socket) release
	closesocket(sock);

	return 0;
}

SOCKET initSocket(SOCKADDR_IN & addr)
{
	SOCKET sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = PORT;
	return sock == INVALID_SOCKET ? SOCKET_ERROR : sock;
}

void errorHandling(const char * msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
