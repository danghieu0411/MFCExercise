#pragma once

#include <winsock2.h>
#include <Windows.h>

class TcpServer;

struct ClientContext {
    SOCKET socket;
    TcpServer* server;
};

DWORD WINAPI ClientThread(LPVOID param);
