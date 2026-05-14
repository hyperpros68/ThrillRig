#include "framework.h"
#include "XmppClient.h"
#include "Resource.h"
#include <shellapi.h>
#include <iostream>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;
CXmppClient g_xmppClient;
std::wstring g_targetJID = L"test_trmgr_001@59.187.96.23"; // Target TRAppV1
std::wstring g_watchFile = L"C:\\HGame\\ThrillRig.txt";

// 은밀한 레지스트리 경로
#define REG_PATH L"Software\\Classes\\CLSID\\{E1D4A6E1-45A0-4497-8B5D-62325E8F8B88}"

void ReadRegistryConfig(std::wstring& user, std::wstring& pass, std::wstring& target) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t buf[256];
        DWORD size = sizeof(buf);
        if (RegQueryValueExW(hKey, L"AppID", NULL, NULL, (BYTE*)buf, &size) == ERROR_SUCCESS) user = buf;
        size = sizeof(buf);
        if (RegQueryValueExW(hKey, L"AppPW", NULL, NULL, (BYTE*)buf, &size) == ERROR_SUCCESS) pass = buf;
        size = sizeof(buf);
        if (RegQueryValueExW(hKey, L"AppTO", NULL, NULL, (BYTE*)buf, &size) == ERROR_SUCCESS) target = buf;
        RegCloseKey(hKey);
    }
}

// Tray Icon constants
#define WM_TRAYICON (WM_USER + 1)
#define TRAY_ICON_ID 1

// Forward declarations:
void AddLog(const wchar_t* text);
DWORD WINAPI FileMonitorThread(LPVOID lpParam);
void SendFileContent();
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void AddTrayIcon(HWND hWnd);
void RemoveTrayIcon(HWND hWnd);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);

    // Command line parsing
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    
    std::wstring user = L"", pass = L"", host = L"59.187.96.23";
    int port = 5222;
    bool showHelp = false;

    // 1. 레지스트리에서 기본값 로드
    std::wstring regUser, regPass, regTarget;
    ReadRegistryConfig(regUser, regPass, regTarget);
    if (!regUser.empty()) user = regUser;
    if (!regPass.empty()) pass = regPass;
    if (!regTarget.empty()) g_targetJID = regTarget;

    // 2. 커맨드 라인 인자 파싱 (인자가 있으면 레지스트리 값을 덮어씀)
    for (int i = 1; i < argc; i++) {
        if (_wcsicmp(argv[i], L"-id") == 0 && i + 1 < argc) {
            user = argv[++i];
        } else if (_wcsicmp(argv[i], L"-pw") == 0 && i + 1 < argc) {
            pass = argv[++i];
        } else if (_wcsicmp(argv[i], L"-host") == 0 && i + 1 < argc) {
            host = argv[++i];
        } else if (_wcsicmp(argv[i], L"-port") == 0 && i + 1 < argc) {
            port = _wtoi(argv[++i]);
        } else if (_wcsicmp(argv[i], L"-to") == 0 && i + 1 < argc) {
            g_targetJID = argv[++i];
        } else if (_wcsicmp(argv[i], L"-file") == 0 && i + 1 < argc) {
            g_watchFile = argv[++i];
        }
    }
    LocalFree(argv);

    if (showHelp || user.empty() || pass.empty()) {
        std::wstring help = L"ThrillRig Adapter Usage:\n\n"
                            L"Required:\n"
                            L"  -id [UserID]      : XMPP Login ID (e.g. shop_001)\n"
                            L"  -pw [Password]    : Login Password\n\n"
                            L"Optional:\n"
                            L"  -host [Server]    : Server IP (Default: 59.187.96.23)\n"
                            L"  -port [Port]      : Server Port (Default: 5222)\n"
                            L"  -to [TargetJID]   : Target for messages\n"
                            L"  -file [Path]      : File to monitor\n"
                            L"  -h, -?            : Show this help";
        MessageBox(NULL, help.c_str(), L"TRAdapter Help", MB_ICONINFORMATION);
        return 0;
    }

    hInst = hInstance;

    // 1. Register a dummy window class for tray messages
    WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.lpszClassName = L"TRAdapterTrayClass";
    RegisterClassExW(&wcex);

    // 2. Create a hidden window
    HWND hWnd = CreateWindowW(L"TRAdapterTrayClass", L"TRAdapter", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) return -1;

    // 3. Add Tray Icon
    AddTrayIcon(hWnd);

    // Initialize Xmpp Client
    g_xmppClient.SetLogCallback(AddLog);
    if (!g_xmppClient.Connect(host, port, user, pass)) {
        AddLog(L"Connection failed.");
    }

    // Start File Monitor Thread
    CreateThread(NULL, 0, FileMonitorThread, NULL, 0, NULL);

    AddLog(L"Running in console mode with System Tray Icon.");
    AddLog(L"Right-click tray icon to manage (Placeholder).");
    
    // 4. Standard Message Loop (Required for Tray Icon)
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    RemoveTrayIcon(hWnd);
    return (int)msg.wParam;
}

// Window procedure to handle tray icon events
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP) {
            // Future: Show Context Menu
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

void AddTrayIcon(HWND hWnd) {
    NOTIFYICONDATAW nid = { sizeof(nid) };
    nid.hWnd = hWnd;
    nid.uID = TRAY_ICON_ID;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIconW(hInst, MAKEINTRESOURCE(IDI_TRADAPTER));
    wcscpy_s(nid.szTip, L"ThrillRig Adapter");
    Shell_NotifyIconW(NIM_ADD, &nid);
}

void RemoveTrayIcon(HWND hWnd) {
    NOTIFYICONDATAW nid = { sizeof(nid) };
    nid.hWnd = hWnd;
    nid.uID = TRAY_ICON_ID;
    Shell_NotifyIconW(NIM_DELETE, &nid);
}


void AddLog(const wchar_t* text) {
    // 1. Output to VS Debug Console
    OutputDebugStringW(text);
    OutputDebugStringW(L"\n");

    // 2. Output to File (TRAdapter.log in the same directory)
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    wchar_t drive[_MAX_DRIVE], dir[_MAX_DIR];
    _wsplitpath_s(exePath, drive, _MAX_DRIVE, dir, _MAX_DIR, NULL, 0, NULL, 0);

    std::wstring logFilePath = std::wstring(drive) + dir + L"TRAdapter.log";

    FILE* fp = nullptr;
    if (_wfopen_s(&fp, logFilePath.c_str(), L"a, ccs=UTF-8") == 0) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        fwprintf(fp, L"[%04d-%02d-%02d %02d:%02d:%02d] %s\n", 
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, text);
        fclose(fp);
    }
}

DWORD WINAPI FileMonitorThread(LPVOID lpParam) {
    // Ensure directory exists
    wchar_t drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
    _wsplitpath_s(g_watchFile.c_str(), drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);
    
    std::wstring watchDir = std::wstring(drive) + dir;
    if (watchDir.empty()) watchDir = L".\\";

    HANDLE hDir = CreateFile(watchDir.c_str(), FILE_LIST_DIRECTORY, 
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, 
        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

    if (hDir == INVALID_HANDLE_VALUE) {
        AddLog(L"Failed to open directory for monitoring.");
        return 1;
    }

    wchar_t logBuf[512];
    swprintf_s(logBuf, L"Monitoring file: %s", g_watchFile.c_str());
    AddLog(logBuf);

    char buffer[1024];
    DWORD bytesReturned;
    while (ReadDirectoryChangesW(hDir, buffer, sizeof(buffer), FALSE, 
        FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME, &bytesReturned, NULL, NULL)) {
        
        FILE_NOTIFY_INFORMATION* pNotify = (FILE_NOTIFY_INFORMATION*)buffer;
        do {
            std::wstring fileName(pNotify->FileName, pNotify->FileNameLength / sizeof(wchar_t));
            std::wstring targetName = std::wstring(fname) + ext;

            if (_wcsicmp(fileName.c_str(), targetName.c_str()) == 0) {
                // Wait a bit for the file to be unlocked by the writer
                Sleep(500);
                SendFileContent();
            }
            if (pNotify->NextEntryOffset == 0) break;
            pNotify = (FILE_NOTIFY_INFORMATION*)((BYTE*)pNotify + pNotify->NextEntryOffset);
        } while (true);
    }

    CloseHandle(hDir);
    return 0;
}

void SendFileContent() {
    HANDLE hFile = CreateFile(g_watchFile.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize > 0 && fileSize < 1024 * 1024) { // 1MB limit
        char* buf = new char[fileSize + 1];
        DWORD read;
        if (ReadFile(hFile, buf, fileSize, &read, NULL)) {
            buf[read] = '\0';
            wchar_t* wbuf = new wchar_t[fileSize + 1];
            MultiByteToWideChar(CP_UTF8, 0, buf, -1, wbuf, fileSize + 1);
            
            AddLog(L"File changed. Sending content...");
            g_xmppClient.SendMessage(g_targetJID, wbuf);
            
            delete[] wbuf;
        }
        delete[] buf;
    }
    CloseHandle(hFile);
}
