#include "Resource.h"
#include "XmppClient.h"
#include "framework.h"
#include "../TR_Protocol.h"
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <shellapi.h>
#include <wincrypt.h>
#include <bcrypt.h>
#include <vector>

#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "bcrypt.lib")

// Forward declaration
void AddLog(const wchar_t *text);

// --- Base64 Decoding Routine ---
static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

inline bool is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_decode(const std::string& encoded_string) {
    int in_len = (int)encoded_string.size();
    int i = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::string ret;

    while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = (unsigned char)base64_chars.find(char_array_4[i]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++)
                ret += char_array_3[i];
            i = 0;
        }
    }

    if (i) {
        for (int j = i; j < 4; j++)
            char_array_4[j] = 0;

        for (int j = 0; j < 4; j++)
            char_array_4[j] = (unsigned char)base64_chars.find(char_array_4[j]);

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (int j = 0; (j < i - 1); j++) ret += char_array_3[j];
    }

    return ret;
}

std::string DecodeThrillRig(std::string line) {
    if (line.empty()) return "";
    int n = line[0] - '0';
    if (n + 1 >= (int)line.length()) return "";
    line = line.substr(n + 1);
    if (!line.empty()) {
        std::swap(line.front(), line.back());
    }
    return base64_decode(line);
}

void TestThrillRigDecoding() {
    std::vector<std::string> testStrings = {
        "7GO3mSYSQjE0LFMsMSxGMTR",
        "6Lkx8ep0jE0LFMsMTAsRjER",
        "7G9hvzfkQjE0LFMsMSxGMTR",
        "9yCH6cwINPMjEzLFMsMSxGMTR",
        "5eI7gOQjE0LFMsNSxGMTR"
    };

    AddLog(L"--- ThrillRig Startup Decode Test ---");
    for (const auto& s : testStrings) {
        std::string decoded = DecodeThrillRig(s);
        std::wstring wDecoded(decoded.begin(), decoded.end());
        
        wchar_t logBuf[512];
        swprintf_s(logBuf, L"Input: %S", s.c_str());
        AddLog(logBuf);
        swprintf_s(logBuf, L" -> Decoded: %s (Structure: Key, Type, Value, Key)", wDecoded.c_str());
        AddLog(logBuf);
    }
    AddLog(L"-------------------------------------");
}


#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;
CXmppClient g_xmppClient;
std::wstring g_targetJID = L"test_trmgr_001@59.187.96.23"; // Target TRAppV1
std::wstring g_serverJID = L"trwww@59.187.96.23"; // Central Server (TRService)
std::wstring g_gameID = L"ThrillRig";                      // Game Identifier
std::wstring g_watchFile =
    L"C:\\Project\\ThrillRig\\Program\\ThrillRigV1\\Data\\SaveData.bin";
std::wstring g_aesKey = L"ThrillRigV1_SecretKey_2026_0515"; // Default

// --- ACK 추적용 구조체 ---
struct PendingMsg {
    std::wstring jid;
    std::wstring body;
    DWORD sendTime;
    int retryCount;
};
std::map<std::wstring, PendingMsg> g_pendingQueue;
CRITICAL_SECTION g_csQueue;

// --- Transaction 관리용 변수 ---
int g_lastValues[4] = { -1, -1, -1, -1 }; // 마지막으로 전송 성공한 값
int g_transSeq = 0;                        // Transaction 일련번호

// 은밀한 레지스트리 경로
#define REG_PATH                                                               \
  L"Software\\Classes\\CLSID\\{E1D4A6E1-45A0-4497-8B5D-62325E8F8B88}"

void ReadRegistryConfig(std::wstring &user, std::wstring &pass,
                        std::wstring &target, std::wstring &gameId, int &seq, std::wstring &aesKey) {
  HKEY hKey;
  if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, KEY_READ, &hKey) ==
      ERROR_SUCCESS) {
    wchar_t buf[256];
    DWORD size = sizeof(buf);
    if (RegQueryValueExW(hKey, L"AppID", NULL, NULL, (BYTE *)buf, &size) == ERROR_SUCCESS) user = buf;
    size = sizeof(buf);
    if (RegQueryValueExW(hKey, L"AppPW", NULL, NULL, (BYTE *)buf, &size) == ERROR_SUCCESS) pass = buf;
    size = sizeof(buf);
    if (RegQueryValueExW(hKey, L"AppTO", NULL, NULL, (BYTE *)buf, &size) == ERROR_SUCCESS) target = buf;
    size = sizeof(buf);
    if (RegQueryValueExW(hKey, L"GameID", NULL, NULL, (BYTE *)buf, &size) == ERROR_SUCCESS) gameId = buf;
    
    DWORD dwSeq = 0;
    DWORD dwSize = sizeof(dwSeq);
    if (RegQueryValueExW(hKey, L"LastSeq", NULL, NULL, (BYTE *)&dwSeq, &dwSize) == ERROR_SUCCESS) seq = (int)dwSeq;
    
    size = sizeof(buf);
    if (RegQueryValueExW(hKey, L"AppKey", NULL, NULL, (BYTE *)buf, &size) == ERROR_SUCCESS) aesKey = buf;
    
    RegCloseKey(hKey);
  }
}

void WriteRegistrySeq(int seq) {
  HKEY hKey;
  if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
    DWORD dwSeq = (DWORD)seq;
    RegSetValueExW(hKey, L"LastSeq", 0, REG_DWORD, (BYTE *)&dwSeq, sizeof(dwSeq));
    RegCloseKey(hKey);
  }
}

// Tray Icon constants
#define WM_TRAYICON (WM_USER + 1)
#define TRAY_ICON_ID 1

// Forward declarations:
DWORD WINAPI FileMonitorThread(LPVOID lpParam);
DWORD WINAPI FileTouchThread(LPVOID lpParam);
void SendFileContent();
void ReadAndDisplayBinaryData();
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void AddTrayIcon(HWND hWnd);
void RemoveTrayIcon(HWND hWnd);
void OnMessageReceived(const wchar_t* from, const wchar_t* body);
DWORD WINAPI AckMonitorThread(LPVOID lpParam);

// --- AES-256-GCM Encryption Wrapper ---
std::string Base64Encode(const std::vector<BYTE>& data) {
    DWORD size = 0;
    CryptBinaryToStringA(data.data(), (DWORD)data.size(), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &size);
    std::string res(size, '\0');
    CryptBinaryToStringA(data.data(), (DWORD)data.size(), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, &res[0], &size);
    res.resize(size - 1);
    return res;
}

std::vector<BYTE> Base64Decode(const std::string& base64) {
    DWORD size = 0;
    CryptStringToBinaryA(base64.c_str(), 0, CRYPT_STRING_BASE64, NULL, &size, NULL, NULL);
    std::vector<BYTE> res(size);
    CryptStringToBinaryA(base64.c_str(), 0, CRYPT_STRING_BASE64, res.data(), &size, NULL, NULL);
    return res;
}

std::string EncryptMessage(const std::wstring& plaintext, const std::wstring& keyStr) {
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_KEY_HANDLE hKey = NULL;
    std::string result = "";

    if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, NULL, 0) != 0) return "";
    BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE, (PBYTE)BCRYPT_CHAIN_MODE_GCM, sizeof(BCRYPT_CHAIN_MODE_GCM), 0);

    std::string keyA(keyStr.begin(), keyStr.end());
    if (keyA.length() < 32) keyA.append(32 - keyA.length(), '0');
    else keyA = keyA.substr(0, 32);

    if (BCryptGenerateSymmetricKey(hAlg, &hKey, NULL, 0, (PBYTE)keyA.c_str(), 32, 0) != 0) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return "";
    }

    BYTE iv[12];
    BCryptGenRandom(NULL, iv, 12, BCRYPT_USE_SYSTEM_PREFERRED_RNG);

    std::string ptA(plaintext.begin(), plaintext.end());
    DWORD cbCipher = (DWORD)ptA.length();
    std::vector<BYTE> cipher(cbCipher);
    
    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
    BCRYPT_INIT_AUTH_MODE_INFO(authInfo);
    BYTE tag[16];
    authInfo.pbNonce = iv;
    authInfo.cbNonce = 12;
    authInfo.pbTag = tag;
    authInfo.cbTag = 16;

    DWORD cbResult = 0;
    if (BCryptEncrypt(hKey, (PUCHAR)ptA.c_str(), (DWORD)ptA.length(), &authInfo, NULL, 0, cipher.data(), cbCipher, &cbResult, 0) == 0) {
        // GCM in CNG: cipher contains the encrypted data, and tag is filled.
        // We combine cipher + tag for Python compatibility (cryptography library expects ciphertext + tag)
        std::vector<BYTE> combined = cipher;
        combined.insert(combined.end(), tag, tag + 16);
        
        char buf[2048];
        sprintf_s(buf, "{\"enc\":true, \"data\":\"%s\", \"iv\":\"%s\"}", 
                  Base64Encode(combined).c_str(), Base64Encode(std::vector<BYTE>(iv, iv+12)).c_str());
        result = buf;
    }

    BCryptDestroyKey(hKey);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return result;
}

std::wstring DecryptMessage(const std::string& jsonBody, const std::wstring& keyStr) {
    // Simple JSON parse for enc, data, iv
    if (jsonBody.find("\"enc\":true") == std::string::npos) return L"";
    
    auto getVal = [&](std::string key) {
        size_t pos = jsonBody.find("\"" + key + "\":\"");
        if (pos == std::string::npos) return std::string("");
        pos += key.length() + 4;
        size_t end = jsonBody.find("\"", pos);
        return jsonBody.substr(pos, end - pos);
    };

    std::string dataB64 = getVal("data");
    std::string ivB64 = getVal("iv");
    if (dataB64.empty() || ivB64.empty()) return L"";

    std::vector<BYTE> combined = Base64Decode(dataB64);
    std::vector<BYTE> iv = Base64Decode(ivB64);
    if (combined.size() < 16) return L"";

    std::vector<BYTE> cipher(combined.begin(), combined.end() - 16);
    BYTE tag[16];
    memcpy(tag, combined.data() + cipher.size(), 16);

    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_KEY_HANDLE hKey = NULL;
    std::wstring result = L"";

    if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, NULL, 0) == 0) {
        BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE, (PBYTE)BCRYPT_CHAIN_MODE_GCM, sizeof(BCRYPT_CHAIN_MODE_GCM), 0);
        std::string keyA(keyStr.begin(), keyStr.end());
        if (keyA.length() < 32) keyA.append(32 - keyA.length(), '0'); else keyA = keyA.substr(0, 32);

        if (BCryptGenerateSymmetricKey(hAlg, &hKey, NULL, 0, (PBYTE)keyA.c_str(), 32, 0) == 0) {
            BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
            BCRYPT_INIT_AUTH_MODE_INFO(authInfo);
            authInfo.pbNonce = iv.data();
            authInfo.cbNonce = (DWORD)iv.size();
            authInfo.pbTag = tag;
            authInfo.cbTag = 16;

            std::vector<BYTE> plain(cipher.size());
            DWORD cbResult = 0;
            if (BCryptDecrypt(hKey, cipher.data(), (DWORD)cipher.size(), &authInfo, NULL, 0, plain.data(), (DWORD)plain.size(), &cbResult, 0) == 0) {
                std::string resA((char*)plain.data(), cbResult);
                result = std::wstring(resA.begin(), resA.end());
            }
            BCryptDestroyKey(hKey);
        }
        BCryptCloseAlgorithmProvider(hAlg, 0);
    }
    return result;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow) {
  UNREFERENCED_PARAMETER(hPrevInstance);

  // Command line parsing
  int argc;
  LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);

  std::wstring user = L"test_tradt_001", pass = L"tradt!@#$",
               host = L"59.187.96.23", target = L"test_trmgr_001@59.187.96.23";
  int port = 5222;
  bool showHelp = false;

  // 1. 레지스트리에서 설정 및 마지막 시퀀스 번호 로드
  ReadRegistryConfig(user, pass, target, g_gameID, g_transSeq, g_aesKey);
  g_targetJID = target;

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
    std::wstring help =
        L"ThrillRig Adapter Usage:\n\n"
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
  WNDCLASSEXW wcex = {sizeof(WNDCLASSEX)};
  wcex.lpfnWndProc = WndProc;
  wcex.hInstance = hInstance;
  wcex.lpszClassName = L"TRAdapterTrayClass";
  RegisterClassExW(&wcex);

  // 2. Create a hidden window
  HWND hWnd = CreateWindowW(
      L"TRAdapterTrayClass", L"TRAdapter", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
      0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

  if (!hWnd)
    return -1;

  // 3. Add Tray Icon
  AddTrayIcon(hWnd);

  // Initialize Xmpp Client
  InitializeCriticalSection(&g_csQueue);
  g_xmppClient.SetLogCallback(AddLog);
  g_xmppClient.SetMessageCallback(OnMessageReceived);
  
  if (!g_xmppClient.Connect(host, port, user, pass)) {
    AddLog(L"Connection failed.");
  }

  // Start Threads
  CreateThread(NULL, 0, FileMonitorThread, NULL, 0, NULL);
  CreateThread(NULL, 0, FileTouchThread, NULL, 0, NULL);
  CreateThread(NULL, 0, AckMonitorThread, NULL, 0, NULL);

  // 초기 바이너리 데이터 읽기 및 출력
  ReadAndDisplayBinaryData();

  AddLog(L"Running in console mode with System Tray Icon.");
  TestThrillRigDecoding();
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
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam,
                         LPARAM lParam) {
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
  NOTIFYICONDATAW nid = {sizeof(nid)};
  nid.hWnd = hWnd;
  nid.uID = TRAY_ICON_ID;
  nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  nid.uCallbackMessage = WM_TRAYICON;
  nid.hIcon = LoadIconW(hInst, MAKEINTRESOURCE(IDI_TRADAPTER));
  wcscpy_s(nid.szTip, L"ThrillRig Adapter");
  Shell_NotifyIconW(NIM_ADD, &nid);
}

void RemoveTrayIcon(HWND hWnd) {
  NOTIFYICONDATAW nid = {sizeof(nid)};
  nid.hWnd = hWnd;
  nid.uID = TRAY_ICON_ID;
  Shell_NotifyIconW(NIM_DELETE, &nid);
}

void ReadAndDisplayBinaryData() {
  HANDLE hFile = CreateFileW(g_watchFile.c_str(), GENERIC_READ, FILE_SHARE_READ,
                            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile != INVALID_HANDLE_VALUE) {
    int values[4] = {0};
    DWORD bytesRead = 0;
    if (ReadFile(hFile, values, sizeof(values), &bytesRead, NULL) &&
        bytesRead == sizeof(values)) {
      wchar_t logBuf[256];
      AddLog(L"--- Initial Binary Data ---");
      swprintf_s(logBuf, L"Credit: %d", values[0]);
      AddLog(logBuf);
      swprintf_s(logBuf, L"Bank: %d", values[1]);
      AddLog(logBuf);
      swprintf_s(logBuf, L"Bankuse: %d", values[2]);
      AddLog(logBuf);
      swprintf_s(logBuf, L"Bankdelete: %d", values[3]);
      AddLog(logBuf);
      AddLog(L"---------------------------");
    } else {
      AddLog(L"Failed to read 16 bytes from SaveData.bin");
    }
    CloseHandle(hFile);
  } else {
    AddLog(L"Failed to open SaveData.bin for initial reading.");
  }
}

void AddLog(const wchar_t *text) {
  // 1. Output to VS Debug Console
  OutputDebugStringW(text);
  OutputDebugStringW(L"\n");

  // 2. Output to File (TRAdapter.log in the same directory)
  wchar_t exePath[MAX_PATH];
  GetModuleFileNameW(NULL, exePath, MAX_PATH);
  wchar_t drive[_MAX_DRIVE], dir[_MAX_DIR];
  _wsplitpath_s(exePath, drive, _MAX_DRIVE, dir, _MAX_DIR, NULL, 0, NULL, 0);

  std::wstring logFilePath = std::wstring(drive) + dir + L"TRAdapter.log";

  FILE *fp = nullptr;
  if (_wfopen_s(&fp, logFilePath.c_str(), L"a, ccs=UTF-8") == 0) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    fwprintf(fp, L"[%04d-%02d-%02d %02d:%02d:%02d] %s\n", st.wYear, st.wMonth,
             st.wDay, st.wHour, st.wMinute, st.wSecond, text);
    fclose(fp);
  }
}

DWORD WINAPI FileMonitorThread(LPVOID lpParam) {
  // Ensure directory exists
  wchar_t drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
  _wsplitpath_s(g_watchFile.c_str(), drive, _MAX_DRIVE, dir, _MAX_DIR, fname,
                _MAX_FNAME, ext, _MAX_EXT);

  std::wstring watchDir = std::wstring(drive) + dir;
  if (watchDir.empty())
    watchDir = L".\\";

  HANDLE hDir =
      CreateFile(watchDir.c_str(), FILE_LIST_DIRECTORY,
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
                               FILE_NOTIFY_CHANGE_LAST_WRITE |
                                   FILE_NOTIFY_CHANGE_FILE_NAME,
                               &bytesReturned, NULL, NULL)) {

    FILE_NOTIFY_INFORMATION *pNotify = (FILE_NOTIFY_INFORMATION *)buffer;
    do {
      std::wstring fileName(pNotify->FileName,
                            pNotify->FileNameLength / sizeof(wchar_t));
      std::wstring targetName = std::wstring(fname) + ext;

      if (_wcsicmp(fileName.c_str(), targetName.c_str()) == 0) {
        // Wait a bit for the file to be unlocked by the writer
        Sleep(500);
        SendFileContent();
      }
      if (pNotify->NextEntryOffset == 0)
        break;
      pNotify = (FILE_NOTIFY_INFORMATION *)((BYTE *)pNotify +
                                            pNotify->NextEntryOffset);
    } while (true);
  }

  CloseHandle(hDir);
  return 0;
}

void SendFileContent() {
  HANDLE hFile = CreateFile(g_watchFile.c_str(), GENERIC_READ, FILE_SHARE_READ,
                            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile == INVALID_HANDLE_VALUE)
    return;

  int values[4] = {0};
  DWORD read = 0;
  if (ReadFile(hFile, values, sizeof(values), &read, NULL) &&
      read == sizeof(values)) {
    
    // 1. 이전 데이터와 동일한지 체크 (변동이 없으면 전송 안 함)
    if (values[0] == g_lastValues[0] && values[1] == g_lastValues[1] &&
        values[2] == g_lastValues[2] && values[3] == g_lastValues[3]) {
        // 데이터 변동 없음, 무시
        CloseHandle(hFile);
        return;
    }

    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t timeBuf[32];
    swprintf_s(timeBuf, L"%04d-%02d-%02d %02d:%02d:%02d", 
               st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    // 2. 새로운 Transaction 생성 및 시퀀스 증가 및 저장
    g_transSeq++;
    WriteRegistrySeq(g_transSeq);
    
    wchar_t msgId[64];
    swprintf_s(msgId, L"TRN_%04d_%d_%d_%d_%d", g_transSeq, values[0], values[1], values[2], values[3]);

    // 3. 현재 값을 마지막 전송 값으로 업데이트
    memcpy(g_lastValues, values, sizeof(values));

    wchar_t jsonBuf[1024];
    swprintf_s(jsonBuf, 
               L"{\"msg_id\": \"%s\", \"version\": \"%S\", \"gameid\": \"%s\", \"credit\": %d, \"bank\": %d, \"bankuse\": %d, \"bankdelete\": %d, \"sendtime\": \"%s\"}",
               msgId, THR_PROT::APP_VERSION, g_gameID.c_str(), values[0], values[1], values[2], values[3], timeBuf);

    // --- Encryption Layer ---
    std::string encBody = EncryptMessage(jsonBuf, g_aesKey);
    std::wstring wEncBody(encBody.begin(), encBody.end());

    wchar_t logMsg[2048];
    swprintf_s(logMsg, L"[SEND] New Transaction ID: %s\n - Original: %s\n - Encrypted: %S", msgId, jsonBuf, encBody.c_str());
    AddLog(logMsg);

    // 큐에 보관 (ACK 대기 - 재전송 시에도 동일한 암호문 사용)
    EnterCriticalSection(&g_csQueue);
    PendingMsg pm = { g_serverJID, wEncBody, GetTickCount(), 0 };
    g_pendingQueue[msgId] = pm;
    LeaveCriticalSection(&g_csQueue);

    g_xmppClient.SendMessage(g_targetJID, wEncBody);
    g_xmppClient.SendMessage(g_serverJID, wEncBody);
  } else {
    AddLog(L"Failed to read binary data for JSON encoding.");
  }

  CloseHandle(hFile);
}

// --- 메시지 수신 처리 (ACK 확인) ---
void OnMessageReceived(const wchar_t* from, const wchar_t* body) {
    std::wstring wBody(body);
    std::string sBody(wBody.begin(), wBody.end());

    // --- Decryption Layer ---
    std::wstring decrypted = DecryptMessage(sBody, g_aesKey);
    if (!decrypted.empty()) {
        wBody = decrypted;
    }

    // 형식 예: ACK:MSG_2024...
    std::wstring msg = wBody;
    std::wstring prefix = std::wstring(L"ACK:");
    if (msg.find(prefix) == 0) {
        std::wstring ackId = msg.substr(prefix.length());
        
        EnterCriticalSection(&g_csQueue);
        if (g_pendingQueue.erase(ackId)) {
            wchar_t logBuf[256];
            swprintf_s(logBuf, L"[RECV] ACK Received for ID: %s. (Transaction Complete)", ackId.c_str());
            AddLog(logBuf);
        }
        LeaveCriticalSection(&g_csQueue);
    }
}

// --- ACK 모니터링 및 재전송 쓰레드 ---
DWORD WINAPI AckMonitorThread(LPVOID lpParam) {
    AddLog(L"AckMonitorThread started.");
    while (true) {
        Sleep(1000); // 1초마다 체크
        
        DWORD now = GetTickCount();
        std::vector<std::wstring> toRemove;

        EnterCriticalSection(&g_csQueue);
        for (auto& pair : g_pendingQueue) {
            PendingMsg& pm = pair.second;
            // 3초 이상 경과 시
            if (now - pm.sendTime > 3000) {
                if (pm.retryCount < 3) {
                    pm.retryCount++;
                    pm.sendTime = now;
                    wchar_t logBuf[256];
                    swprintf_s(logBuf, L"[RETRY #%d] No ACK yet. Resending ID: %s", pm.retryCount, pair.first.c_str());
                    AddLog(logBuf);
                    g_xmppClient.SendMessage(pm.jid, pm.body.c_str());
                } else {
                    wchar_t logBuf[256];
                    swprintf_s(logBuf, L"[TIMEOUT] FAILED: Delivery failed for ID: %s after 3 retries.", pair.first.c_str());
                    AddLog(logBuf);
                    toRemove.push_back(pair.first);
                }
            }
        }
        for (const auto& id : toRemove) g_pendingQueue.erase(id);
        LeaveCriticalSection(&g_csQueue);
    }
    return 0;
}

DWORD WINAPI FileTouchThread(LPVOID lpParam) {
  AddLog(L"Test: FileTouchThread started (10s interval).");
  while (true) {
    Sleep(10000); // 10 seconds
    HANDLE hFile =
        CreateFileW(g_watchFile.c_str(), FILE_WRITE_ATTRIBUTES,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
      FILETIME ft;
      SYSTEMTIME st;
      GetSystemTime(&st);
      SystemTimeToFileTime(&st, &ft);
      if (SetFileTime(hFile, NULL, NULL, &ft)) {
        AddLog(L"Test: Touched watch file to trigger monitor.");
      } else {
        AddLog(L"Test: Failed to touch file (SetFileTime error).");
      }
      CloseHandle(hFile);
    } else {
      AddLog(L"Test: Failed to open file for touch (Maybe in use).");
    }
  }
  return 0;
}
