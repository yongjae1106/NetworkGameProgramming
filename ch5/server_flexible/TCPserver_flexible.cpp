#include "../../Common.h"
#include "../../TCPserver_flexible.h"
#include <iostream>

#define PORT 9000

using namespace std;

int main(int argc, char* argv[])
{
	int retval;

	WSAData wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return 1;
	}

	// socket 생성
	SOCKET listen_sock;
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET)
	{
		cerr << "socket() err!" << endl;
	}

	sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	serv_addr.sin_addr.s_addr = htonl(ADDR_ANY);
	retval = bind(listen_sock, (sockaddr*)&serv_addr, sizeof(serv_addr));
	if (retval == SOCKET_ERROR)
	{
		cerr << "bind() err!" << endl;
	}

	// listen 함수
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR)
	{
		cerr << "listen() err!" << endl;
	}

	SOCKET client_sock;
	sockaddr_in client_addr;
	int addrlen;
	flexible_buf recv_buf;
	
	// accept
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

		// recv
		while (1)
		{
			recv(client_sock, (char*)&recv_buf.bufsize, sizeof(recv_buf.bufsize), 0);
			retval = recv(client_sock, recv_buf.buf, recv_buf.bufsize, 0);
			if (retval <= 0)
			{
				cerr << "recv() err!" << endl;
				break;
			}
			recv_buf.buf[retval] = '\0';
			cout << "chat: " << recv_buf.buf << endl;
			cout << "서버가 " << retval << "바이트를 받았습니다." << endl;
		}
		closesocket(client_sock);
		cout << "클라이언트 소켓 종료" << endl;
	}

	closesocket(listen_sock);
	WSACleanup();
}