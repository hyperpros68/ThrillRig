#include <windows.h>
#include <string>
#include <shlobj.h>

// 은밀한 레지스트리 경로 (CLSID 하위로 위장)
#define REG_PATH L"Software\\Classes\\CLSID\\{E1D4A6E1-45A0-4497-8B5D-62325E8F8B88}"
#define INSTALL_DIR L"C:\\ProgramData\\ThrillRig"
#define EXE_NAME L"TRAdapter.exe"

// UI 컨트롤 ID
#define ID_EDIT_ID 101
#define ID_EDIT_PW 102
#define ID_EDIT_TO 103
#define ID_BTN_INSTALL 104
#define ID_CHK_SHOW_PW 105
#define ID_EDIT_GAMEID 106

HWND hEditGameId, hEditId, hEditPw, hEditTo;

// 레지스트리에 설정 저장 함수
bool SaveToRegistry(const std::wstring& gameId, const std::wstring& id, const std::wstring& pw, const std::wstring& to) {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, L"GameID", 0, REG_SZ, (BYTE*)gameId.c_str(), (DWORD)(gameId.length() + 1) * sizeof(wchar_t));
        RegSetValueExW(hKey, L"AppID", 0, REG_SZ, (BYTE*)id.c_str(), (DWORD)(id.length() + 1) * sizeof(wchar_t));
        RegSetValueExW(hKey, L"AppPW", 0, REG_SZ, (BYTE*)pw.c_str(), (DWORD)(pw.length() + 1) * sizeof(wchar_t));
        RegSetValueExW(hKey, L"AppTO", 0, REG_SZ, (BYTE*)to.c_str(), (DWORD)(to.length() + 1) * sizeof(wchar_t));
        RegCloseKey(hKey);
        return true;
    }
    return false;
}

// 파일 복사 함수
bool InstallFiles() {
    CreateDirectoryW(INSTALL_DIR, NULL);
    wchar_t currentPath[MAX_PATH];
    GetModuleFileNameW(NULL, currentPath, MAX_PATH);
    
    std::wstring srcPath = currentPath;
    size_t pos = srcPath.find_last_of(L"\\");
    srcPath = srcPath.substr(0, pos + 1) + EXE_NAME;
    
    std::wstring destPath = std::wstring(INSTALL_DIR) + L"\\" + EXE_NAME;
    return CopyFileW(srcPath.c_str(), destPath.c_str(), FALSE);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
    {
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(NULL, exePath, MAX_PATH);
        std::wstring configPath = exePath;
        size_t pos = configPath.find_last_of(L"\\");
        if (pos != std::wstring::npos) {
            configPath = configPath.substr(0, pos + 1) + L"AdtInstall.cfg";
        }

        wchar_t defGameId[256] = { 0 }, defId[256] = { 0 }, defPw[256] = { 0 }, defTo[256] = { 0 };
        GetPrivateProfileStringW(L"System", L"GameID", L"ThrillRig", defGameId, 256, configPath.c_str());
        GetPrivateProfileStringW(L"System", L"AppID", L"", defId, 256, configPath.c_str());
        GetPrivateProfileStringW(L"System", L"AppPW", L"", defPw, 256, configPath.c_str());
        GetPrivateProfileStringW(L"System", L"ToMsgJID", L"trwww@59.187.96.23", defTo, 256, configPath.c_str());

        CreateWindowW(L"Static", L"Game:", WS_VISIBLE | WS_CHILD, 20, 20, 50, 20, hWnd, NULL, NULL, NULL);
        hEditGameId = CreateWindowW(L"Edit", defGameId, WS_VISIBLE | WS_CHILD | WS_BORDER, 80, 20, 200, 20, hWnd, (HMENU)ID_EDIT_GAMEID, NULL, NULL);

        CreateWindowW(L"Static", L"ID:", WS_VISIBLE | WS_CHILD, 20, 50, 50, 20, hWnd, NULL, NULL, NULL);
        hEditId = CreateWindowW(L"Edit", defId, WS_VISIBLE | WS_CHILD | WS_BORDER, 80, 50, 200, 20, hWnd, (HMENU)ID_EDIT_ID, NULL, NULL);
        
        CreateWindowW(L"Static", L"PW:", WS_VISIBLE | WS_CHILD, 20, 80, 50, 20, hWnd, NULL, NULL, NULL);
        hEditPw = CreateWindowW(L"Edit", defPw, WS_VISIBLE | WS_CHILD | WS_BORDER | ES_PASSWORD, 80, 80, 200, 20, hWnd, (HMENU)ID_EDIT_PW, NULL, NULL);
        CreateWindowW(L"Button", L"Show", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 290, 80, 60, 20, hWnd, (HMENU)ID_CHK_SHOW_PW, NULL, NULL);
        
        CreateWindowW(L"Static", L"TO:", WS_VISIBLE | WS_CHILD, 20, 110, 50, 20, hWnd, NULL, NULL, NULL);
        hEditTo = CreateWindowW(L"Edit", defTo, WS_VISIBLE | WS_CHILD | WS_BORDER, 80, 110, 200, 20, hWnd, (HMENU)ID_EDIT_TO, NULL, NULL);
        
        CreateWindowW(L"Button", L"Install & Start", WS_VISIBLE | WS_CHILD, 100, 150, 100, 30, hWnd, (HMENU)ID_BTN_INSTALL, NULL, NULL);
    }
    break;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_CHK_SHOW_PW) {
            LRESULT checked = SendMessage((HWND)lParam, BM_GETCHECK, 0, 0);
            SendMessage(hEditPw, EM_SETPASSWORDCHAR, (checked == BST_CHECKED) ? 0 : L'*', 0);
            SetFocus(hEditPw); // 포커스를 다시 줘서 강제 리프레시 유도
            InvalidateRect(hEditPw, NULL, TRUE);
        }

        if (LOWORD(wParam) == ID_BTN_INSTALL) {
            wchar_t gameId[256], id[256], pw[256], to[256];
            GetWindowTextW(hEditGameId, gameId, 256);
            GetWindowTextW(hEditId, id, 256);
            GetWindowTextW(hEditPw, pw, 256);
            GetWindowTextW(hEditTo, to, 256);
            
            if (SaveToRegistry(gameId, id, pw, to)) {
                if (InstallFiles()) {
                    MessageBoxW(hWnd, L"Installation Successful!", L"Success", MB_OK);
                    ShellExecuteW(NULL, L"open", (std::wstring(INSTALL_DIR) + L"\\" + EXE_NAME).c_str(), NULL, NULL, SW_SHOW);
                    PostQuitMessage(0);
                } else {
                    MessageBoxW(hWnd, L"Failed to copy files.", L"Error", MB_ICONERROR);
                }
            } else {
                MessageBoxW(hWnd, L"Failed to save configuration.", L"Error", MB_ICONERROR);
            }
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

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"TRAdtInstallerClass";
    RegisterClassExW(&wcex);

    // 화면 중앙 좌표 계산
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int windowWidth = 400;
    int windowHeight = 240;
    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;

    HWND hWnd = CreateWindowW(L"TRAdtInstallerClass", L"ThrillRig Adapter Installer", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        x, y, windowWidth, windowHeight, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) return FALSE;
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
