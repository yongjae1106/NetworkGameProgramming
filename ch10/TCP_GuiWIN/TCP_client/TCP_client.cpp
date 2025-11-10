// TCP_client.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "framework.h"
#include "TCP_client.h"
#define MAX_LOADSTRING 100

// 전역 변수:
HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

HWND hStatus, hProgress, hSendButton, hSelectButton, hResetButton;
WCHAR filePath[MAX_PATH];

// 함수 선언
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // 윈속 초기화
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        MessageBox(NULL, L"WSAStartup() failed", L"Error", MB_OK);
        return -1;
    }

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_TCPCLIENT, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TCPCLIENT));
    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    WSACleanup();
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
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TCPCLIENT));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_TCPCLIENT);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance;
   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 450, 200, nullptr, nullptr, hInstance, nullptr);

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
    {
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_PROGRESS_CLASS;
        InitCommonControlsEx(&icex);

        hSelectButton = CreateWindow(L"button", L"파일 선택", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            10, 10, 100, 25, hWnd, (HMENU)IDC_BUTTON_SELECT, hInst, NULL);

        hSendButton = CreateWindow(L"button", L"파일 전송", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            120, 10, 100, 25, hWnd, (HMENU)IDC_BUTTON_SEND, hInst, NULL);
        EnableWindow(hSendButton, FALSE);

        hResetButton = CreateWindow(L"button", L"초기화", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            230, 10, 100, 25, hWnd, (HMENU)IDC_BUTTON_RESET, hInst, NULL);

        hStatus = CreateWindow(L"static", L"전송할 파일을 선택하세요.", WS_CHILD | WS_VISIBLE | SS_LEFT,
            10, 50, 400, 25, hWnd, (HMENU)IDC_STATIC_STATUS, hInst, NULL);

        hProgress = CreateWindowEx(0, PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
            10, 80, 400, 25, hWnd, (HMENU)IDC_PROGRESS, hInst, NULL);
    }
    break;
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case IDC_BUTTON_SELECT:
        {
            OPENFILENAME ofn;
            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hWnd;
            ofn.lpstrFile = filePath;
            ofn.lpstrFile[0] = '\0';
            ofn.nMaxFile = sizeof(filePath);
            ofn.lpstrFilter = L"All Files\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

            if (GetOpenFileName(&ofn) == TRUE) {
                SetWindowText(hStatus, filePath);
                EnableWindow(hSendButton, TRUE);
            }
        }
        break;
        case IDC_BUTTON_SEND:
        {
            EnableWindow(hSendButton, FALSE);
            EnableWindow(hSelectButton, FALSE);
            HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, SendFileThread, (void*)hWnd, 0, NULL);
            CloseHandle(hThread);
        }
        break;
                case IDC_BUTTON_RESET:
                {
                    EnableWindow(hSelectButton, TRUE);
                    SendMessage(hProgress, PBM_SETPOS, 0, 0);
                    SetWindowText(hStatus, L"전송할 파일을 선택하세요.");
                    filePath[0] = '\0';
                    EnableWindow(hSendButton, FALSE);
                }
                break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
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

unsigned __stdcall SendFileThread(void* arg) {
    HWND hWnd = (HWND)arg;
    SOCKET sock = INVALID_SOCKET;
    SOCKADDR_IN server_addr;
    long long file_size = 0;
    long long total_sent = 0;
    FILE* fp = NULL;
    const WCHAR* filename_w = NULL;
    char filename_mb[MAX_PATH] = { 0 };
    int filename_len = 0;
    bool success = false;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        DisplayError("socket()");
        goto cleanup;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
    server_addr.sin_port = htons(8080);

    if (connect(sock, (SOCKADDR*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        DisplayError("connect()");
        goto cleanup;
    }

    if (_wfopen_s(&fp, filePath, L"rb") != 0) {
        DisplayError("fopen()");
        goto cleanup;
    }

    fseek(fp, 0, SEEK_END);
    file_size = _ftelli64(fp);
    fseek(fp, 0, SEEK_SET);

    // 1. 파일 크기 전송 (long long, 8바이트)
    if (send(sock, (char*)&file_size, sizeof(file_size), 0) == SOCKET_ERROR) {
        DisplayError("send(file_size)");
        goto cleanup;
    }

    // 2. 파일 이름 전송
    filename_w = PathFindFileNameW(filePath);
    WideCharToMultiByte(CP_ACP, 0, filename_w, -1, filename_mb, MAX_PATH, NULL, NULL);
    filename_len = strlen(filename_mb);
    
    // 2.1. 파일 이름 길이 전송 (int, 4바이트)
    if (send(sock, (char*)&filename_len, sizeof(filename_len), 0) == SOCKET_ERROR) {
        DisplayError("send(filename_len)");
        goto cleanup;
    }

    // 2.2. 파일 이름 문자열 전송
    if (send(sock, filename_mb, filename_len, 0) == SOCKET_ERROR) {
        DisplayError("send(filename)");
        goto cleanup;
    }

    // 3. 파일 내용 전송
    char buf[BUF_SIZE];
    int bytes_read;
    while ((bytes_read = fread(buf, 1, BUF_SIZE, fp)) > 0) {
        int retval = send(sock, buf, bytes_read, 0);
        if (retval == SOCKET_ERROR) {
            DisplayError("send()");
            break;
        }
        total_sent += retval;
        if (file_size > 0) {
            int percent = (int)(total_sent * 100 / file_size);
            SendMessage(hProgress, PBM_SETPOS, percent, 0);
        }
    }

    if (total_sent == file_size) {
        success = true;
    }

cleanup:
    if(fp) fclose(fp);
    if(sock != INVALID_SOCKET) closesocket(sock);

    if (success) {
        SetWindowText(hStatus, L"전송 완료!");
    } else {
        SetWindowText(hStatus, L"전송 실패. 초기화 버튼을 누르세요.");
        EnableWindow(hSelectButton, TRUE); // 실패 시 다시 시도할 수 있도록
    }
    return 0;
}

void DisplayError(const char* msg) {
    LPWSTR lpMsgBuf;
    DWORD dw = GetLastError();
    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&lpMsgBuf, 0, NULL);

    wchar_t formatted_msg[512];
    wsprintfW(formatted_msg, L"%S failed with error %d: %s", msg, dw, lpMsgBuf);
    MessageBoxW(NULL, formatted_msg, L"Error", MB_OK);

    LocalFree(lpMsgBuf);
}