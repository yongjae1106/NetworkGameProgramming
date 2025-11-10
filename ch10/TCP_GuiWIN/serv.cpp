#include <WinSock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <map>
#pragma comment(lib, "Ws2_32.lib")
using namespace std;

// 콘솔 출력용 mutex
HANDLE g_mutex;
// 스레드별 줄 번호 관리
map<DWORD, int> g_threadLines;
int g_nextLine = 6; // 헤더 다음부터 시작

// 특정 줄로 커서 이동
void SetCursorPosition(int line, int col = 0)
{
    COORD coord;
    coord.X = col;
    coord.Y = line;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

// 줄 지우기
void ClearLine(int line)
{
    SetCursorPosition(line, 0);
    cout << string(120, ' '); // 120칸 공백으로 지우기 (콘솔 한줄에 120칸)
    SetCursorPosition(line, 0);
}

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
    DWORD threadId = GetCurrentThreadId();

    // 이 스레드에 줄 번호 할당
    WaitForSingleObject(g_mutex, INFINITE);
    int myLine = g_nextLine++;
    g_threadLines[threadId] = myLine;
    ReleaseMutex(g_mutex);

    // 클라이언트 IP 주소 가져오기
    sockaddr_in clientAddr;
    int addrLen = sizeof(clientAddr);
    getpeername(clientSock, (sockaddr*)&clientAddr, &addrLen);
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN); // IPv4 주소를 문자열로 변환

    WaitForSingleObject(g_mutex, INFINITE);
    ClearLine(myLine);
    cout << "[Thread " << threadId << "] [" << clientIP << "] Connecting..." << flush;
    ReleaseMutex(g_mutex);

    // 헤더 수신: 파일 크기 (8바이트)
    long long totalFileSize = 0;
    if (!recvAll(clientSock, (char*)&totalFileSize, sizeof(totalFileSize)))
    {
        WaitForSingleObject(g_mutex, INFINITE);
        ClearLine(myLine);
        cout << "[Thread " << threadId << "] [" << clientIP << "] Failed to receive file size." << flush;
        ReleaseMutex(g_mutex);
        closesocket(clientSock);
        return 1;
    }

    // 헤더 수신: 파일 이름 길이 (4바이트)
    int filenameLen = 0;
    if (!recvAll(clientSock, (char*)&filenameLen, sizeof(filenameLen)))
    {
        WaitForSingleObject(g_mutex, INFINITE);
        ClearLine(myLine);
        cout << "[Thread " << threadId << "] [" << clientIP << "] Failed to receive filename length." << flush;
        ReleaseMutex(g_mutex);
        closesocket(clientSock);
        return 1;
    }

    // 파일 이름 길이 유효성 검사
    if (filenameLen <= 0 || filenameLen > 260)
    {
        WaitForSingleObject(g_mutex, INFINITE);
        ClearLine(myLine);
        cout << "[Thread " << threadId << "] [" << clientIP << "] Invalid filename length: " << filenameLen << flush;
        ReleaseMutex(g_mutex);
        closesocket(clientSock);
        return 1;
    }

    // 헤더 수신: 파일 이름
    char* filename = new char[filenameLen + 1];
    if (!recvAll(clientSock, filename, filenameLen))
    {
        WaitForSingleObject(g_mutex, INFINITE);
        ClearLine(myLine);
        cout << "[Thread " << threadId << "] [" << clientIP << "] Failed to receive filename." << flush;
        ReleaseMutex(g_mutex);
        delete[] filename;
        closesocket(clientSock);
        return 1;
    }
    filename[filenameLen] = '\0';

    WaitForSingleObject(g_mutex, INFINITE);
    ClearLine(myLine);
    cout << "[Thread " << threadId << "] [" << clientIP << "] " << filename << " (" << totalFileSize << " bytes)" << flush;
    ReleaseMutex(g_mutex);

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
        DWORD err = GetLastError();
        WaitForSingleObject(g_mutex, INFINITE);
        ClearLine(myLine);
        cout << "[Thread " << threadId << "] [" << clientIP << "] Failed to create file (Error: " << err << ")" << flush;
        ReleaseMutex(g_mutex);
        closesocket(clientSock);
        return 1;
    }

    // 파일 데이터 수신
    char buffer[64 * 1024];
    int bytesRecv = 0;
    long long totalRecv = 0;
    int lastProgress = -1;

    while ((bytesRecv = recv(clientSock, buffer, sizeof(buffer), 0)) > 0)// recvall은 크기만큼 반복완료될때까지 블락되지만 위와같은 경우 64KB 받을때마다 잠깐 블락이 풀림
    {
        // 파일에 데이터 쓰기
        DWORD written;
        WriteFile(hFile, buffer, bytesRecv, &written, NULL);
        totalRecv += bytesRecv;

        // 진행률 표시 (10% 단위)
        double progress = (double)totalRecv / totalFileSize * 100.0;
        int currentProgress = (int)(progress / 10);

        if (currentProgress != lastProgress)
        {
            WaitForSingleObject(g_mutex, INFINITE);
            ClearLine(myLine);
            cout << "[Thread " << threadId << "] [" << clientIP << "] "
                << filePath << " - " << currentProgress * 10 << "%" << flush;
            ReleaseMutex(g_mutex);
            lastProgress = currentProgress;
        }
    }

    WaitForSingleObject(g_mutex, INFINITE);
    ClearLine(myLine);
    cout << "[Thread " << threadId << "] [" << clientIP << "] "
        << filePath << " - Complete!" << flush;
    ReleaseMutex(g_mutex);

    CloseHandle(hFile);
    closesocket(clientSock);
    return 0;
}

int main()
{
    // 콘솔 커서 숨기기
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);

    // Mutex 생성
    g_mutex = CreateMutex(NULL, FALSE, NULL);
    if (g_mutex == NULL)
    {
        cerr << "Failed to create mutex\n";
        return 1;
    }

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        cerr << "WSAStartup failed\n";
        CloseHandle(g_mutex);
        return 1;
    }

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET)
    {
        cerr << "socket() failed with error: " << WSAGetLastError() << "\n";
        WSACleanup();
        CloseHandle(g_mutex);
        return 1;
    }

    sockaddr_in servAddr = {};
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(8080);
    servAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSock, (sockaddr*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
    {
        cerr << "bind() failed with error: " << WSAGetLastError() << "\n";
        closesocket(listenSock);
        WSACleanup();
        CloseHandle(g_mutex);
        return 1;
    }

    if (listen(listenSock, SOMAXCONN) == SOCKET_ERROR)
    {
        cerr << "listen() failed with error: " << WSAGetLastError() << "\n";
        closesocket(listenSock);
        WSACleanup();
        CloseHandle(g_mutex);
        return 1;
    }

    // 헤더 출력
    cout << "===========================================\n";
    cout << "Multi-Client File Transfer Server Started\n";
    cout << "Port: 8080\n";
    cout << "Waiting for connections...\n";
    cout << "===========================================\n";
    cout << flush;

    // 무한 루프로 클라이언트 연결 대기
    while (1)
    {
        SOCKET clientSock = accept(listenSock, NULL, NULL);
        if (clientSock == INVALID_SOCKET)
        {
            continue;
        }

        // 새 스레드에서 클라이언트 처리
        HANDLE hThread = CreateThread(NULL, 0, ClientHandler,
            (LPVOID)clientSock, 0, NULL);
        if (hThread == NULL)
        {
            closesocket(clientSock);
        }
        else
        {
            CloseHandle(hThread);
        }
    }

    closesocket(listenSock);
    WSACleanup();
    CloseHandle(g_mutex);
    return 0;
}