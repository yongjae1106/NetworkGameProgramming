#pragma once

#include "resource.h"
#define MAX_LOADSTRING 100
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <conio.h>
#include <process.h>
#include <vector>
#include <string>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_PORT 8080
#define BUF_SIZE    1000

// 스레드에 전달할 데이터 구조체
struct ClientInfo {
    SOCKET sock;
    int id;
    char ip_addr[INET_ADDRSTRLEN];
};

// 함수 선언
unsigned __stdcall AcceptThread(void* arg);
unsigned __stdcall ClientThread(void* arg);
void DisplayError(const char* msg);

