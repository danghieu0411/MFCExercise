#pragma once

#include "Protocol.h"

const UINT WM_TCP_PACKET = WM_APP + 1;
const UINT WM_TCP_DISCONNECTED = WM_APP + 2;

class TcpClient {
public:
    TcpClient() = default;
    ~TcpClient();

    bool Connect(const char* serverIp, unsigned short port, HWND hWnd);
    bool Send(const Packet& packet);
    void Disconnect();

private:
    static DWORD WINAPI ReceiveThread(LPVOID param);
    void ReceiveLoop();

    SOCKET m_socket = INVALID_SOCKET;
    HANDLE m_thread = nullptr;
    HWND m_hWnd = nullptr;
};
