#include <iostream>
#include "../../Common.h"
#include "../../TCPserver_flexible.h"
#define PORT 9000

char* serverIP = (char*)"127.0.0.1";

using namespace std;

int main(int argc, char* argv[])
{
	int retval;
	flexible_buf send_buf;

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
	addr.sin_port =htons(PORT);
	inet_pton(AF_INET, serverIP, &addr.sin_addr);

	retval = connect(sock, (sockaddr*)&addr, sizeof(addr));
	if (retval == SOCKET_ERROR)
	{
		cerr << "connect() err!" << endl;
	}

	while (1)
	{

		cout << "대화 내용 입력: ";
		cin >> send_buf.buf;

		send_buf.bufsize = strlen(send_buf.buf);

		retval = send(sock, (char*)&send_buf, sizeof(int) + strlen(send_buf.buf), 0);
		if (retval <= 0)
		{
			cerr << "send() err!" << endl;
			break;
		}

		cout << "클라이언트가 " << retval << "바이트를 보냈습니다." << endl;
	}
	closesocket(sock);
	WSACleanup();
}