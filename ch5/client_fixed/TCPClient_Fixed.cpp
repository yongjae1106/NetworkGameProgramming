#include "../../Common.h"
#include <iostream>
#define PORT 9000
#define SIZE 50

char* serverIP = (char*)"127.0.0.1";

using namespace std;

int main(int argc, char* argv[])
{
	int retval;

	if (argc > 1)
	{
		serverIP = argv[1];
	}

	WSAData wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return 1;
	}

	SOCKET sock;
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		cerr << "socket() err!" << endl;
	}

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	inet_pton(AF_INET, serverIP, &addr.sin_addr);
	retval = connect(sock, (sockaddr*)&addr, sizeof(addr));
	if (retval != 0)
	{
		cerr << "connect() err!" << endl;
	}

	char buf[SIZE];
	const char* chat[] =
	{
		"안녕하세요",
		"10월 2일입니다.",
		"2025년이네요",
		"안녕히가십쇼"
	};

	for (int i = 0; i < 4; i++)
	{
		memset(buf, '#', sizeof(buf));
		strncpy(buf, chat[i], strlen(chat[i]));

		retval = send(sock, buf, SIZE, 0);
		if (retval != 0)
		{
			cerr << "send() err!" << endl;
		}

		cout << "클라이언트가 " << retval << "바이트를 보냈습니다." << endl;
	}

	closesocket(sock);
	WSACleanup();
}