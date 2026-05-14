#include "framework.h"
#include "XmppClient.h"
#include <shellapi.h>
#include <iostream>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;
CXmppClient g_xmppClient;

// Forward declarations:
void AddLog(const wchar_t* text);
void OnMessageReceived(const wchar_t* from, const wchar_t* body);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    
    // Create console for logging
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    std::wcout.clear();
    std::cout.clear();

    // Command line parsing
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    
    std::wstring user = L"test_trmgr_001@59.187.96.23", pass = L"trmgr!@#$", host = L"59.187.96.23";
    int port = 5222;

    for (int i = 1; i < argc; i++) {
        if (_wcsicmp(argv[i], L"-id") == 0 && i + 1 < argc) {
            user = argv[++i];
        } else if (_wcsicmp(argv[i], L"-pw") == 0 && i + 1 < argc) {
            pass = argv[++i];
        } else if (_wcsicmp(argv[i], L"-host") == 0 && i + 1 < argc) {
            host = argv[++i];
        } else if (_wcsicmp(argv[i], L"-port") == 0 && i + 1 < argc) {
            port = _wtoi(argv[++i]);
        }
    }
    LocalFree(argv);

    hInst = hInstance;

    // Initialize Xmpp Client
    g_xmppClient.SetLogCallback(AddLog);
    g_xmppClient.SetMessageCallback(OnMessageReceived);
    if (!g_xmppClient.Connect(host, port, user, pass)) {
        AddLog(L"Connection failed.");
    }

    AddLog(L"Running in console mode. Press Ctrl+C to exit.");

    // Keep alive
    while (true) {
        Sleep(1000);
    }

    return 0;
}


void AddLog(const wchar_t* text) {
    // 1. Output to VS Debug Console
    OutputDebugStringW(text);
    OutputDebugStringW(L"\n");

    // 2. Output to DOS Console
    std::wcout << text << std::endl;
}

void OnMessageReceived(const wchar_t* from, const wchar_t* body) {
    wchar_t buf[4096];
    swprintf_s(buf, L"[%s]: %s", from, body);
    AddLog(buf);
}
