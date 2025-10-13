// 순서 중요: WinSock2를 windows.h보다 먼저 포함
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <string>
#pragma comment(lib, "Ws2_32.lib")
using namespace std;

// 전체 버퍼 전송 완료까지 send 반복
static bool sendAll(SOCKET s, const char* buf, int len)
{
    int sentTotal = 0;
    while (sentTotal < len)
    {
        int sent = send(s, buf + sentTotal, len - sentTotal, 0);
        if (sent == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            cerr << "send() failed with WSA error " << err << "\n";
            return false;
        }
        sentTotal += sent;
    }
    return true;
}

// 경로에서 파일 이름만 추출
static string extractFilename(const string& path)
{
    size_t pos = path.find_last_of("\\/");
    if (pos != string::npos)
    {
        return path.substr(pos + 1);
    }
    return path;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        cerr << "Usage: TCPaviClnt <filename>\n";
        return 1;
    }
    const char* filePath = argv[1];

    // 파일 이름 추출
    string filename = extractFilename(filePath);

    // 파일 열기
    HANDLE hFile = CreateFileA(filePath, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        cerr << "Failed to open file.\n";
        return 1;
    }

    // 파일 크기 구하기
    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize))
    {
        cerr << "GetFileSizeEx failed.\n";
        CloseHandle(hFile);
        return 1;
    }

    // WinSock 초기화
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        cerr << "WSAStartup failed.\n";
        CloseHandle(hFile);
        return 1;
    }

    // TCP 소켓 생성
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        cerr << "socket() failed.\n";
        CloseHandle(hFile);
        WSACleanup();
        return 1;
    }

    // 서버 연결
    sockaddr_in servAddr = {};
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(9090);
    inet_pton(AF_INET, "127.0.0.1", &servAddr.sin_addr);
    if (connect(sock, (sockaddr*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
    {
        cerr << "connect() failed. WSA error " << WSAGetLastError() << "\n";
        closesocket(sock);
        CloseHandle(hFile);
        WSACleanup();
        return 1;
    }

    // 헤더 전송 1: 파일 크기 (8바이트)
    long long fileSizeToSend = fileSize.QuadPart;
    if (!sendAll(sock, (char*)&fileSizeToSend, sizeof(fileSizeToSend)))
    {
        cerr << "Failed to send file size.\n";
        closesocket(sock);
        CloseHandle(hFile);
        WSACleanup();
        return 1;
    }

    cout << "Sending file size: " << fileSizeToSend << " bytes\n";

    // 헤더 전송 2: 파일 이름 길이 (4바이트)
    int filenameLen = (int)filename.length();
    if (!sendAll(sock, (char*)&filenameLen, sizeof(filenameLen)))
    {
        cerr << "Failed to send filename length.\n";
        closesocket(sock);
        CloseHandle(hFile);
        WSACleanup();
        return 1;
    }

    // 헤더 전송 3: 파일 이름
    cout << "Sending filename: " << filename << " (" << filenameLen << " bytes)\n";
    if (!sendAll(sock, filename.c_str(), filenameLen))
    {
        cerr << "Failed to send filename.\n";
        closesocket(sock);
        CloseHandle(hFile);
        WSACleanup();
        return 1;
    }
    cout << "Headers sent successfully.\n";

    // 파일 데이터 전송
    const DWORD BUF_SIZE = 64 * 1024;
    char* buffer = new char[BUF_SIZE];
    long long sentBytes = 0;
    long long totalBytes = fileSize.QuadPart;
    DWORD bytesRead = 0;

    while (ReadFile(hFile, buffer, BUF_SIZE, &bytesRead, NULL) && bytesRead > 0)
    {
        if (!sendAll(sock, buffer, bytesRead))
        {
            cerr << "send() failed during data transfer.\n";
            delete[] buffer;
            closesocket(sock);
            CloseHandle(hFile);
            WSACleanup();
            return 1;
        }

        sentBytes += bytesRead;
        double progress = (double)sentBytes / totalBytes * 100.0;
        cout << "\rsend percent: " << (int)progress << "%";
        cout.flush();
    }

    delete[] buffer;
    cout << "\nsend complete.\n";
    closesocket(sock);
    CloseHandle(hFile);
    WSACleanup();
    return 0;
}