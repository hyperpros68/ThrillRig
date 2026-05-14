#pragma once
#include "framework.h"

class CXmppClient {
public:
    CXmppClient();
    ~CXmppClient();

    bool Connect(const std::wstring& host, int port, const std::wstring& user, const std::wstring& pass);
    void Disconnect();
    bool SendMessage(const std::wstring& to, const std::wstring& body);

    // Callback/Event handling can be added here
    void SetLogCallback(void (*callback)(const wchar_t*)) { m_pLogCallback = callback; }
    void SetMessageCallback(void (*callback)(const wchar_t*, const wchar_t*)) { m_pMessageCallback = callback; }

private:
    static DWORD WINAPI NetworkThread(LPVOID lpParam);
    void Run();

    void Log(const wchar_t* format, ...);

    SOCKET m_socket;
    std::wstring m_host;
    int m_port;
    std::wstring m_user;
    std::wstring m_pass;
    std::wstring m_domain;

    HANDLE m_hThread;
    bool m_bRunning;
    bool m_bAuthenticated;

    void (*m_pLogCallback)(const wchar_t*);
    void (*m_pMessageCallback)(const wchar_t*, const wchar_t*);

    // XMPP State machine and simple XML builders
    bool SendRaw(const std::string& data);
    bool HandleAuth();
    bool HandleStreamHeader();
};
