#include <WinSock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#pragma comment(lib, "Ws2_32.lib")
using namespace std;

// 정확히 len 바이트를 수신할 때까지 반복
static bool recvAll(SOCKET s, char* buf, int len)
{
    int recvTotal = 0;
    while (recvTotal < len)
    {
        int recvd = recv(s, buf + recvTotal, len - recvTotal, 0);
        if (recvd <= 0)
        {
            return false;
        }
        recvTotal += recvd;
    }
    return true;
}

// 클라이언트 처리를 위한 스레드 함수
DWORD WINAPI ClientHandler(LPVOID arg)
{
    SOCKET clientSock = (SOCKET)arg;

    cout << "[Thread " << GetCurrentThreadId() << "] Client connected. Receiving file...\n";

    // 헤더 수신: 파일 크기 (8바이트)
    long long totalFileSize = 0;
    if (!recvAll(clientSock, (char*)&totalFileSize, sizeof(totalFileSize)))
    {
        cerr << "[Thread " << GetCurrentThreadId() << "] Failed to receive file size.\n";
        closesocket(clientSock);
        return 1;
    }
    cout << "[Thread " << GetCurrentThreadId() << "] File size: " << totalFileSize << " bytes\n";

    // 헤더 수신: 파일 이름 길이 (4바이트)
    int filenameLen = 0;
    if (!recvAll(clientSock, (char*)&filenameLen, sizeof(filenameLen)))
    {
        cerr << "[Thread " << GetCurrentThreadId() << "] Failed to receive filename length.\n";
        closesocket(clientSock);
        return 1;
    }
    cout << "[Thread " << GetCurrentThreadId() << "] Filename length: " << filenameLen << " bytes\n";

    // 파일 이름 길이 유효성 검사
    if (filenameLen <= 0 || filenameLen > 260)
    {
        cerr << "[Thread " << GetCurrentThreadId() << "] Invalid filename length: " << filenameLen << "\n";
        closesocket(clientSock);
        return 1;
    }

    // 헤더 수신: 파일 이름
    char* filename = new char[filenameLen + 1];
    if (!recvAll(clientSock, filename, filenameLen))
    {
        cerr << "[Thread " << GetCurrentThreadId() << "] Failed to receive filename.\n";
        delete[] filename;
        closesocket(clientSock);
        return 1;
    }
    filename[filenameLen] = '\0';
    cout << "[Thread " << GetCurrentThreadId() << "] Filename: " << filename << "\n";

    // received 폴더 생성 (이미 있으면 무시)
    CreateDirectoryA("received", NULL);

    // 파일 경로 생성
    string filePath = string("received\\") + filename;
    delete[] filename;

    // 파일 생성
    HANDLE hFile = CreateFileA(filePath.c_str(), GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        cerr << "[Thread " << GetCurrentThreadId() << "] Failed to create file: "
            << filePath << " (Error: " << GetLastError() << ")\n";
        closesocket(clientSock);
        return 1;
    }
    cout << "[Thread " << GetCurrentThreadId() << "] File created: " << filePath << "\n";

    // 파일 데이터 수신
    char buffer[64 * 1024];
    int bytesRecv = 0;
    long long totalRecv = 0;

    while ((bytesRecv = recv(clientSock, buffer, sizeof(buffer), 0)) > 0)
    {
        // 파일에 데이터 쓰기
        DWORD written;
        WriteFile(hFile, buffer, bytesRecv, &written, NULL);
        totalRecv += bytesRecv;

        // 진행률 표시
        double progress = (double)totalRecv / totalFileSize * 100.0;
        cout << "[Thread " << GetCurrentThreadId() << "] Received: " << (int)progress << "%\r";
        cout.flush();
    }

    cout << "\n[Thread " << GetCurrentThreadId() << "] Transfer complete: " << filePath << "\n";
    CloseHandle(hFile);
    closesocket(clientSock);

    return 0;
}

int main()
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        cerr << "WSAStartup failed\n";
        return 1;
    }

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET)
    {
        cerr << "socket() failed with error: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in servAddr = {};
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(9090);
    servAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSock, (sockaddr*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
    {
        cerr << "bind() failed with error: " << WSAGetLastError() << "\n";
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    if (listen(listenSock, SOMAXCONN) == SOCKET_ERROR)
    {
        cerr << "listen() failed with error: " << WSAGetLastError() << "\n";
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    cout << "===========================================\n";
    cout << "Multi-Client File Transfer Server Started\n";
    cout << "Port: 9090\n";
    cout << "Waiting for connections...\n";
    cout << "===========================================\n\n";

    // 무한 루프로 클라이언트 연결 대기
    while (1)
    {
        SOCKET clientSock = accept(listenSock, NULL, NULL);
        if (clientSock == INVALID_SOCKET)
        {
            cerr << "accept failed with error: " << WSAGetLastError() << "\n";
            continue; // 에러 발생해도 서버는 계속 실행
        }

        // 새 스레드에서 클라이언트 처리
        HANDLE hThread = CreateThread(NULL, 0, ClientHandler,
            (LPVOID)clientSock, 0, NULL);
        if (hThread == NULL)
        {
            cerr << "Failed to create thread for client\n";
            closesocket(clientSock);
        }
        else
        {
            // 스레드 핸들 해제 (자동 정리)
            CloseHandle(hThread);
        }
    }

    // 정상 종료 시 (실제로는 무한루프로 도달 안함)
    closesocket(listenSock);
    WSACleanup();
    return 0;
}