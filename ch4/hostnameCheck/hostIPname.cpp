#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")  // include <winsock2.h> 에는 실제로 winsock2 함수의 선언만 있기 때문에 정의가 필요함 "ws2_32.lib"에 정의가 있음

using namespace std;

int main(int argc, char* argv[])
{
    WSADATA wsa;
    hostent* host;
    char** alias;
    in_addr addr;   // in_addr 은 IPv4를 담는 구조체

    if (argc != 2)
    {
        cout << "Usage: " << argv[0] << " <domain name>" << endl;
        return 1;
    }

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        cerr << "WSAStartup failed" << endl;
        return 1;
    }

    host = gethostbyname(argv[1]);  // 도메인 이름(호스트 이름)을 IP 주소로 변환하는 윈속 API 함수

    if (host == NULL)
    {
        cerr << "gethostbyname failed" << endl;
        WSACleanup();
        return 1;
    }

    cout << "Official name: " << host->h_name << endl;

    cout << "Aliases:" << endl;

    alias = host->h_aliases;

    while (*alias != NULL)
    {
        cout << "  " << *alias << endl;
        alias++;    // 다음 별명으로 이동
    }

    cout << "IPv4 Addresses:" << endl;

    int i = 0;

    while (host->h_addr_list[i] != NULL) 
    {
        addr.s_addr = *(u_long*)host->h_addr_list[i];   // u_long 의 크기 4바이트 / 
        cout << "  " << inet_ntoa(addr) << endl;    // inet_ntoa 는 IPv4를 사람이 읽을 수 있는 문자열로 변환해주는 함수 / inet_ntop 는 IPv6를 사람이 읽을 수 있는 문자열로 변환해주는 함수
        // 2진수를 10진수로 사람이 읽을 수 있는 문자열로 변환해줌
        i++;
    }

    WSACleanup();
    return 0;
}


// IPv4의 사이즈는 4바이트
// IPv6의 사이즈는 16바이트
// char = 1바이트 / int = 4바이트 / long = 4바이트 / double = 8바이트 / float = 8바이트 / 