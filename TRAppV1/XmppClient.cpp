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
    
    // Extract domain from user if present (e.g. user@domain)
    size_t pos = user.find(L'@');
    if (pos != std::wstring::npos) {
        m_domain = user.substr(pos + 1);
        m_user = user.substr(0, pos);
    } else {
        m_domain = host; // Fallback
    }

    struct addrinfo hints = {0}, *result = NULL;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    char portStr[10];
    sprintf_s(portStr, "%d", m_port);

    char hostAnsi[256];
    WideCharToMultiByte(CP_ACP, 0, m_host.c_str(), -1, hostAnsi, sizeof(hostAnsi), NULL, NULL);

    if (getaddrinfo(hostAnsi, portStr, &hints, &result) != 0) {
        Log(L"Failed to resolve host.");
        return false;
    }

    m_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (m_socket == INVALID_SOCKET) {
        freeaddrinfo(result);
        return false;
    }

    if (connect(m_socket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        Log(L"Connection failed.");
        closesocket(m_socket);
        freeaddrinfo(result);
        return false;
    }

    freeaddrinfo(result);
    Log(L"Connected to %s:%d", m_host.c_str(), m_port);

    m_bRunning = true;
    m_hThread = CreateThread(NULL, 0, NetworkThread, this, 0, NULL);

    // Start XMPP Stream
    std::string streamHeader = "<stream:stream xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams' to='";
    char domainAnsi[256];
    WideCharToMultiByte(CP_ACP, 0, m_domain.c_str(), -1, domainAnsi, sizeof(domainAnsi), NULL, NULL);
    streamHeader += domainAnsi;
    streamHeader += "' version='1.0'>";
    SendRaw(streamHeader);

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
    char buffer[4096];
    while (m_bRunning) {
        int bytes = recv(m_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            
            wchar_t wbuf[4096];
            MultiByteToWideChar(CP_UTF8, 0, buffer, -1, wbuf, 4096);
            Log(L"RECV: %s", wbuf);

            // Basic State Handling (Simplified)
            if (strstr(buffer, "<stream:features") && !m_bAuthenticated) {
                // Perform Auth
                char userA[256], passA[256];
                WideCharToMultiByte(CP_UTF8, 0, m_user.c_str(), -1, userA, sizeof(userA), NULL, NULL);
                WideCharToMultiByte(CP_UTF8, 0, m_pass.c_str(), -1, passA, sizeof(passA), NULL, NULL);

                std::string authData;
                authData.push_back('\0');
                authData.append(userA);
                authData.push_back('\0');
                authData.append(passA);

                std::string authBase64 = Base64Encode(authData);
                std::string authMsg = "<auth xmlns='urn:ietf:params:xml:ns:xmpp-sasl' mechanism='PLAIN'>" + authBase64 + "</auth>";
                SendRaw(authMsg);
            }
            else if (strstr(buffer, "<success")) {
                Log(L"Authenticated successfully.");
                m_bAuthenticated = true;
                // Restart stream after auth
                std::string streamHeader = "<stream:stream xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams' to='";
                char domainAnsi[256];
                WideCharToMultiByte(CP_ACP, 0, m_domain.c_str(), -1, domainAnsi, sizeof(domainAnsi), NULL, NULL);
                streamHeader += domainAnsi;
                streamHeader += "' version='1.0'>";
                SendRaw(streamHeader);
            }
            else if (strstr(buffer, "<stream:features") && strstr(buffer, "<bind")) {
                // Resource Binding
                SendRaw("<iq type='set' id='bind_1'><bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'><resource>TRAdapter</resource></bind></iq>");
            }
            else if (strstr(buffer, "<iq") && strstr(buffer, "type='result'") && strstr(buffer, "id='bind_1'")) {
                Log(L"Resource bound. Session started.");
                SendRaw("<presence/>");
            }
            else if (strstr(buffer, "<message")) {
                // Very simple body extraction
                const char* bodyStart = strstr(buffer, "<body>");
                const char* bodyEnd = strstr(buffer, "</body>");
                const char* fromStart = strstr(buffer, "from='");
                
                if (bodyStart && bodyEnd) {
                    bodyStart += 6;
                    std::string body(bodyStart, bodyEnd - bodyStart);
                    
                    std::string from = "unknown";
                    if (fromStart) {
                        fromStart += 6;
                        const char* fromEnd = strchr(fromStart, '\'');
                        if (fromEnd) from.assign(fromStart, fromEnd - fromStart);
                    }

                    wchar_t wBody[4096], wFrom[256];
                    MultiByteToWideChar(CP_UTF8, 0, body.c_str(), -1, wBody, 4096);
                    MultiByteToWideChar(CP_UTF8, 0, from.c_str(), -1, wFrom, 256);
                    
                    if (m_pMessageCallback) m_pMessageCallback(wFrom, wBody);
                }
            }
        } else {
            break;
        }
    }
    m_bRunning = false;
    Log(L"Network thread stopped.");
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
