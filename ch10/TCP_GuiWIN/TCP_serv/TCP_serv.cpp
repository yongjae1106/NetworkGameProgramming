// TCP_serv.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "framework.h"
#include "TCP_serv.h"
#include <iostream> // for std::string in ClearLine

// 전역 변수:
HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

SOCKET listen_sock;
std::vector<ClientInfo*> client_list;
HANDLE hAcceptThread;
CRITICAL_SECTION cs;
HANDLE hConsole;
int client_id_counter = 0;

// 함수 선언
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void ClearLine(int line);
bool recvAll(SOCKET s, char* buf, int len);


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    printf("===========================================\n");
    printf("Multi-Client File Transfer Server Started\n");
    printf("Port: 8080\n");
    printf("Waiting for connections...\n");
    printf("===========================================\n");

    InitializeCriticalSection(&cs);

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        DisplayError("WSAStartup()");
        return -1;
    }

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_TCPSERV, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TCPSERV));
    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    for (ClientInfo* client : client_list) {
        closesocket(client->sock);
        delete client;
    }
    client_list.clear();
    closesocket(listen_sock);
    if(hAcceptThread) WaitForSingleObject(hAcceptThread, 1000);
    CloseHandle(hAcceptThread);
    DeleteCriticalSection(&cs);
    WSACleanup();
    if (fp) fclose(fp);
    FreeConsole();

    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TCPSERV));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_TCPSERV);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance;
   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 500, 300, nullptr, nullptr, hInstance, nullptr);
   if (!hWnd) return FALSE;
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);
   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        listen_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_sock == INVALID_SOCKET) { DisplayError("socket()"); return -1; }

        SOCKADDR_IN server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        server_addr.sin_port = htons(8080); // ch6 포트

        if (bind(listen_sock, (SOCKADDR*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            DisplayError("bind()"); closesocket(listen_sock); return -1;
        }
        if (listen(listen_sock, SOMAXCONN) == SOCKET_ERROR) {
            DisplayError("listen()"); closesocket(listen_sock); return -1;
        }
        hAcceptThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, (void*)hWnd, 0, NULL);
        if (hAcceptThread == 0) {
            DisplayError("_beginthreadex()"); closesocket(listen_sock); return -1;
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            EnterCriticalSection(&cs);
            char buffer[256];
            sprintf_s(buffer, "TCP 파일 서버 실행 중... %d명 접속 중.", (int)client_list.size());
            LeaveCriticalSection(&cs);
            wchar_t w_buffer[256];
            MultiByteToWideChar(CP_ACP, 0, buffer, -1, w_buffer, 256);
            TextOut(hdc, 10, 10, w_buffer, wcslen(w_buffer));
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

unsigned __stdcall AcceptThread(void* arg) {
    HWND hWnd = (HWND)arg;
    SOCKADDR_IN client_addr;
    int addr_len = sizeof(client_addr);
    while (true) {
        SOCKET client_sock = accept(listen_sock, (SOCKADDR*)&client_addr, &addr_len);
        if (client_sock == INVALID_SOCKET) { DisplayError("accept()"); break; }

        EnterCriticalSection(&cs);
        ClientInfo* client = new ClientInfo;
        client->sock = client_sock;
        client->id = client_id_counter++;
        client_list.push_back(client);
        LeaveCriticalSection(&cs);

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        strcpy_s(client->ip_addr, client_ip); // Store IP in ClientInfo

        printf("클라이언트 접속: ID=%d, IP=%s\n", client->id, client_ip);
        InvalidateRect(hWnd, NULL, TRUE);
        HANDLE hClientThread = (HANDLE)_beginthreadex(NULL, 0, ClientThread, (void*)client, 0, NULL);
        CloseHandle(hClientThread);
    }
    return 0;
}

unsigned __stdcall ClientThread(void* arg) {
    ClientInfo* client = (ClientInfo*)arg;
    SOCKET sock = client->sock;
    int id = client->id;
    
    // 모든 지역 변수를 함수 맨 위에 선언 및 초기화
    char buf[BUF_SIZE];
    long long total_bytes = 0;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    char sanitized_filename[MAX_PATH] = { 0 };
    long long file_size = 0;
    int filename_len = 0;
    char filename[MAX_PATH] = { 0 };
    const char* last_slash = NULL;
    const char* last_bslash = NULL;
    std::string file_path;

    // 1. 파일 크기 수신
    if (recv(sock, (char*)&file_size, sizeof(file_size), MSG_WAITALL) <= 0) {
        goto cleanup;
    }

    // 2. 파일 이름 길이 수신
    if (recv(sock, (char*)&filename_len, sizeof(filename_len), MSG_WAITALL) <= 0) {
        goto cleanup;
    }
    if (filename_len <= 0 || filename_len >= MAX_PATH) {
        goto cleanup;
    }

    // 3. 파일 이름 수신
    if (recv(sock, filename, filename_len, MSG_WAITALL) <= 0) {
        goto cleanup;
    }
    filename[filename_len] = '\0';

    // 4. 파일 이름에서 경로 정보 제거
    last_slash = strrchr(filename, '\\');
    last_bslash = strrchr(filename, '/');
    if (last_slash == NULL && last_bslash == NULL) {
        strcpy_s(sanitized_filename, filename);
    } else {
        const char* start_pos = max(last_slash, last_bslash) + 1;
        strcpy_s(sanitized_filename, start_pos);
    }

    // 5. received 폴더 생성 및 파일 생성
    CreateDirectoryA("received", NULL);
    file_path = "received\\" + std::string(sanitized_filename);
    
    hFile = CreateFileA(file_path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        goto cleanup;
    }

    // 6. 파일 내용 수신 및 저장
    while (total_bytes < file_size) {
        int bytes_to_recv = min(BUF_SIZE, (int)(file_size - total_bytes));
        int retval = recv(sock, buf, bytes_to_recv, 0);
        if (retval <= 0) { break; }
        
        DWORD written;
        WriteFile(hFile, buf, retval, &written, NULL);
        total_bytes += retval;

        EnterCriticalSection(&cs);
        ClearLine(id + 5); // 5줄 아래부터 시작

        printf("[ID:%d, IP:%s] '%s' 수신 중: %.2f %%", id, client->ip_addr, sanitized_filename, (double)total_bytes / file_size * 100.0);

        LeaveCriticalSection(&cs);
    }

cleanup:
    EnterCriticalSection(&cs);
    ClearLine(id + 5);
    if (total_bytes == file_size && file_size > 0) {
        printf("[ID:%d, IP:%s] '%s' 파일 수신 완료! (크기: %lld 바이트)\n", id, client->ip_addr, sanitized_filename, total_bytes);
    } else {
        printf("[ID:%d, IP:%s] 파일 수신 중단 또는 실패.\n", id, client->ip_addr);
    }
    LeaveCriticalSection(&cs);

    if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
    closesocket(sock);
    
    EnterCriticalSection(&cs);
    for (auto it = client_list.begin(); it != client_list.end(); ++it) {
        if ((*it)->sock == sock) {
            delete* it;
            client_list.erase(it);
            break;
        }
    }
    LeaveCriticalSection(&cs);
    return 0;
}

void ClearLine(int line) {
    COORD coord = { 0, (SHORT)line };
    DWORD count;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    FillConsoleOutputCharacter(GetStdHandle(STD_OUTPUT_HANDLE), ' ', csbi.dwSize.X, coord, &count);
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void DisplayError(const char* msg) {
    LPVOID lpMsgBuf;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
    printf("Error %s: %s\n", msg, (char*)lpMsgBuf);
    LocalFree(lpMsgBuf);
}