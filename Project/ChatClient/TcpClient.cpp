#include "pch.h"
#include "TcpClient.h"
#include <WS2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

TcpClient::~TcpClient() {
    Disconnect();
}

bool TcpClient::Connect(const char* ip, unsigned short port, HWND hWnd) {
    if (m_socket != INVALID_SOCKET) {
        return false;
    }

    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        return false;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1 ||
        connect(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(sock);
        WSACleanup();
        return false;
    }

    u_long nonBlocking = 1;
    if (ioctlsocket(sock, FIONBIO, &nonBlocking) == SOCKET_ERROR) {
        closesocket(sock);
        WSACleanup();
        return false;
    }

    HANDLE stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (stopEvent == nullptr) {
        closesocket(sock);
        WSACleanup();
        return false;
    }

    m_socket = sock;
    m_hWnd = hWnd;
    m_stopEvent = stopEvent;

    m_thread = CreateThread(
        nullptr, 0, ReceiveThread, this, 0, nullptr
    );

    if (m_thread == nullptr) {
        closesocket(m_socket);
        CloseHandle(m_stopEvent);
        m_socket = INVALID_SOCKET;
        m_stopEvent = nullptr;
        m_hWnd = nullptr;
        WSACleanup();
        return false;
    }

    return true;
}

bool TcpClient::Send(const Packet& packet) {
    if (m_socket == INVALID_SOCKET) {
        return false;
    }
    return SendPacket(m_socket, packet, m_stopEvent);
}

DWORD WINAPI TcpClient::ReceiveThread(LPVOID param) {
    TcpClient* client = (TcpClient*)param;
    client->ReceiveLoop();
    return 0;
}

void TcpClient::ReceiveLoop() {
    Packet packet;

    while (RecvPacket(m_socket, packet, m_stopEvent)) {
        Packet* received = new Packet(packet);

        if (!PostMessage(m_hWnd, WM_TCP_PACKET, 0,
            (LPARAM)received)) {
            delete received;
        }
    }

    PostMessage(m_hWnd, WM_TCP_DISCONNECTED, 0, 0);
}

void TcpClient::Disconnect() {
    if (m_socket == INVALID_SOCKET)
        return;

    SetEvent(m_stopEvent);

    if (m_thread != nullptr) {
        WaitForSingleObject(m_thread, INFINITE);
        CloseHandle(m_thread);
        m_thread = nullptr;
    }

    shutdown(m_socket, SD_BOTH);
    closesocket(m_socket);
    CloseHandle(m_stopEvent);

    m_socket = INVALID_SOCKET;
    m_stopEvent = nullptr;
    m_hWnd = nullptr;

    WSACleanup();
}