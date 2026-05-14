#include "XmppClient.h"
#include <iostream>
#include <sstream>

// Simple Base64 encoder for SASL PLAIN
static std::string Base64Encode(const std::string& in) {
    static const char lookup[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int val = 0, valb = -6;
    for (unsigned char c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(lookup[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(lookup[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

CXmppClient::CXmppClient() : m_socket(INVALID_SOCKET), m_hThread(NULL), m_bRunning(false), m_bAuthenticated(false), m_pLogCallback(nullptr), m_pMessageCallback(nullptr) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
}

CXmppClient::~CXmppClient() {
    Disconnect();
    WSACleanup();
}

bool CXmppClient::Connect(const std::wstring& host, int port, const std::wstring& user, const std::wstring& pass) {
    m_host = host;
    m_port = port;
    m_user = user;
    m_pass = pass;
    
    size_t pos = user.find(L'@');
    if (pos != std::wstring::npos) {
        m_domain = user.substr(pos + 1);
        m_user = user.substr(0, pos);
    } else {
        m_domain = host;
    }

    m_bRunning = true;
    m_bAuthenticated = false;
    m_socket = INVALID_SOCKET; // Run loop will handle the first connection

    if (m_hThread == NULL) {
        m_hThread = CreateThread(NULL, 0, NetworkThread, this, 0, NULL);
    }
    return true;
}

void CXmppClient::Disconnect() {
    m_bRunning = false;
    if (m_socket != INVALID_SOCKET) {
        SendRaw("</stream:stream>");
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
    if (m_hThread) {
        WaitForSingleObject(m_hThread, 2000);
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }
}

bool CXmppClient::SendRaw(const std::string& data) {
    if (m_socket == INVALID_SOCKET) return false;
    int sent = send(m_socket, data.c_str(), (int)data.length(), 0);
    return sent != SOCKET_ERROR;
}

bool CXmppClient::SendMessage(const std::wstring& to, const std::wstring& body) {
    if (!m_bAuthenticated || m_socket == INVALID_SOCKET) {
        Log(L"Cannot send message: Not connected.");
        return false;
    }
    char toA[256], bodyA[1024];
    WideCharToMultiByte(CP_UTF8, 0, to.c_str(), -1, toA, sizeof(toA), NULL, NULL);
    WideCharToMultiByte(CP_UTF8, 0, body.c_str(), -1, bodyA, sizeof(bodyA), NULL, NULL);

    std::stringstream ss;
    ss << "<message to='" << toA << "' type='chat'><body>" << bodyA << "</body></message>";
    return SendRaw(ss.str());
}

DWORD WINAPI CXmppClient::NetworkThread(LPVOID lpParam) {
    ((CXmppClient*)lpParam)->Run();
    return 0;
}

void CXmppClient::Run() {
    while (m_bRunning) {
        if (m_socket == INVALID_SOCKET) {
            struct addrinfo hints = {0}, *result = NULL;
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;
            char portStr[10]; sprintf_s(portStr, "%d", m_port);
            char hostAnsi[256]; WideCharToMultiByte(CP_ACP, 0, m_host.c_str(), -1, hostAnsi, sizeof(hostAnsi), NULL, NULL);

            if (getaddrinfo(hostAnsi, portStr, &hints, &result) == 0) {
                m_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
                if (connect(m_socket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
                    closesocket(m_socket); m_socket = INVALID_SOCKET;
                } else {
                    Log(L"Connected to %s:%d", m_host.c_str(), m_port);
                    std::string streamHeader = "<stream:stream xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams' to='";
                    char domainAnsi[256]; WideCharToMultiByte(CP_ACP, 0, m_domain.c_str(), -1, domainAnsi, sizeof(domainAnsi), NULL, NULL);
                    streamHeader += domainAnsi; streamHeader += "' version='1.0'>";
                    SendRaw(streamHeader);
                }
                freeaddrinfo(result);
            }
            if (m_socket == INVALID_SOCKET) {
                Sleep(1000); continue;
            }
        }

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(m_socket, &fds);
        struct timeval tv = { 60, 0 };

        int sel = select(0, &fds, NULL, NULL, &tv);
        if (sel == 0) {
            SendRaw(" "); continue;
        } else if (sel == SOCKET_ERROR) {
            closesocket(m_socket); m_socket = INVALID_SOCKET; continue;
        }

        char buffer[4096];
        int bytes = recv(m_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            wchar_t wbuf[4096];
            MultiByteToWideChar(CP_UTF8, 0, buffer, -1, wbuf, 4096);
            Log(L"RECV: %s", wbuf);

            if (strstr(buffer, "<stream:features") && !m_bAuthenticated) {
                char userA[256], passA[256];
                WideCharToMultiByte(CP_UTF8, 0, m_user.c_str(), -1, userA, sizeof(userA), NULL, NULL);
                WideCharToMultiByte(CP_UTF8, 0, m_pass.c_str(), -1, passA, sizeof(passA), NULL, NULL);
                std::string authData;
                authData.push_back('\0'); authData.append(userA); authData.push_back('\0'); authData.append(passA);
                std::string authMsg = "<auth xmlns='urn:ietf:params:xml:ns:xmpp-sasl' mechanism='PLAIN'>" + Base64Encode(authData) + "</auth>";
                SendRaw(authMsg);
            }
            else if (strstr(buffer, "<success")) {
                Log(L"!!! [인증 성공] Authenticated successfully.");
                m_bAuthenticated = true;
                std::string streamHeader = "<stream:stream xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams' to='";
                char domainAnsi[256]; WideCharToMultiByte(CP_ACP, 0, m_domain.c_str(), -1, domainAnsi, sizeof(domainAnsi), NULL, NULL);
                streamHeader += domainAnsi; streamHeader += "' version='1.0'>";
                SendRaw(streamHeader);
            }
            else if (strstr(buffer, "<stream:features") && strstr(buffer, "<bind")) {
                SendRaw("<iq type='set' id='bind_1'><bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'><resource>TRAdapter</resource></bind></iq>");
            }
            else if (strstr(buffer, "<iq") && strstr(buffer, "type='result'") && strstr(buffer, "id='bind_1'")) {
                Log(L"Resource bound. Session started.");
                SendRaw("<presence/>");
            }
        } else {
            Log(L"Connection lost. Reconnecting...");
            m_bAuthenticated = false;
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            Sleep(500);
        }
    }
}

void CXmppClient::Log(const wchar_t* format, ...) {
    if (!m_pLogCallback) return;
    wchar_t buffer[1024];
    va_list args;
    va_start(args, format);
    vswprintf_s(buffer, format, args);
    va_end(args);
    m_pLogCallback(buffer);
}
