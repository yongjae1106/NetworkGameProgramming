#pragma once

#include "resource.h"
#define MAX_LOADSTRING 100
#include <winsock2.h>
#include <ws2tcpip.h>
#include <commctrl.h>
#include <commdlg.h>
#include <stdio.h>
#include <process.h>
#include <shlwapi.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")

#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 8080
#define BUF_SIZE    1000

#define IDC_BUTTON_SELECT   101
#define IDC_BUTTON_SEND     102
#define IDC_PROGRESS        103
#define IDC_STATIC_STATUS   104
#define IDC_BUTTON_RESET    106

// 스레드 함수 선언
unsigned __stdcall SendFileThread(void* arg);
void DisplayError(const char* msg);
