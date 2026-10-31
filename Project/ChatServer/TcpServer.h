#pragma once

#include <winsock2.h>
#include <unordered_map>
#include "Protocol.h"
#include "Database.h"

class TcpServer {
public:
    TcpServer();
    ~TcpServer();

    bool Start(unsigned short port);
    Database& GetDatabase();

    bool AddOnlineUser(int userId, SOCKET socket);
    void RemoveOnlineUser(int userId, SOCKET socket);
    bool SendToClient(SOCKET socket, const Packet& packet);
    bool NotifyUser(int userId, const Packet& packet);
    void Disconnect(SOCKET socket, int userId);

private:
    SOCKET m_listenSocket = INVALID_SOCKET;
    Database m_database;
    std::unordered_map<int, SOCKET> m_onlineUsers;
    CRITICAL_SECTION m_onlineLock;
};
