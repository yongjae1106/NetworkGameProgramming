#include "../../Common.h"
#include <iostream>
using namespace std;

#define PORT 9000
#define SIZE 50

char* serverIP = (char*)"127.0.0.1";

int main(int argc, char argv[])
{
	int retval;

	WSAData wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return 1;
	}

	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET)
	{
		cerr << "socket() err!" << endl;
	}

	sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.s_addr = htonl(ADDR_ANY);
	retval = bind(listen_sock, (sockaddr*)&server_addr, sizeof(server_addr));
	if (retval == SOCKET_ERROR)
	{
		cerr << "bind() err!" << endl;
	}

	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR)
	{
		cerr << "listen() err!" << endl;
	}

	SOCKET client_sock;
	sockaddr_in client_addr;
	int addrlen;
	char buf[SIZE + 1];

	while (1)
	{
		addrlen = sizeof(client_addr);
		client_sock = accept(listen_sock, (sockaddr*)&client_addr, &addrlen);
		if (client_sock == INVALID_SOCKET)
		{
			cerr << "accept() err!" << endl;
			break;
		}

		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &client_addr, addr, sizeof(addr));
		cout << "TCP 클라이언트 | IP 주소: " << addr << " | PORT 번호: " << ntohs(client_addr.sin_port) << endl;

		while (1)
		{
			retval = recv(client_sock, buf, SIZE, MSG_WAITALL);
			if (retval == SOCKET_ERROR)
			{
				cerr << "recv() err!" << endl;
				break;
			}
			else if (retval == 0)
			{
				break;
			}

			buf[retval] = '\0';
			cout << "TCP|" << addr << "/"<< ntohs(client_addr.sin_port) << "|: " << buf << endl;

		}

		closesocket(client_sock);
		cout << "TCP|" << addr << "/" << ntohs(client_addr.sin_port) << "| 클라이언트 종료" << endl;
	}

	closesocket(listen_sock);
	WSACleanup();
	return 0;
}