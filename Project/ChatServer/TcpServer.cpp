#include "TcpServer.h"
#include "ClientHandler.h"
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")

TcpServer::TcpServer() {
    InitializeCriticalSection(&m_onlineLock);
}

TcpServer::~TcpServer() {
    if (m_listenSocket != INVALID_SOCKET)
        closesocket(m_listenSocket);
    DeleteCriticalSection(&m_onlineLock);
}

Database& TcpServer::GetDatabase() {
    return m_database;
}

bool TcpServer::AddOnlineUser(int userId, SOCKET socket) {
    CriticalSectionGuard lock(m_onlineLock);
    if (m_onlineUsers.count(userId))
        return false;
    m_onlineUsers[userId] = socket;
    return true;
}

void TcpServer::RemoveOnlineUser(int userId, SOCKET socket) {
    CriticalSectionGuard lock(m_onlineLock);
    auto it = m_onlineUsers.find(userId);
    if (it != m_onlineUsers.end() && it->second == socket)
        m_onlineUsers.erase(it);
}

bool TcpServer::SendToClient(SOCKET socket, const Packet& packet) {
    CriticalSectionGuard lock(m_onlineLock);
    return SendPacket(socket, packet);
}

bool TcpServer::NotifyUser(int userId, const Packet& packet) {
    CriticalSectionGuard lock(m_onlineLock);
    auto it = m_onlineUsers.find(userId);
    if (it == m_onlineUsers.end())
        return false;
    return SendPacket(it->second, packet);
}

void TcpServer::Disconnect(SOCKET socket, int userId) {
    CriticalSectionGuard lock(m_onlineLock);
    if (userId != -1) {
        auto it = m_onlineUsers.find(userId);
        if (it != m_onlineUsers.end() && it->second == socket)
            m_onlineUsers.erase(it);
    }
    closesocket(socket);
}

bool TcpServer::Start(unsigned short port) {
    if (!m_database.Open() || !m_database.Initialize()) {
        std::cout << "Cannot initialize database!\n";
        return false;
    }

    m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listenSocket == INVALID_SOCKET)
        return false;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(m_listenSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
        return false;

    if (listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR)
        return false;

    std::cout << "Server is listening on port " << port << "\n";

    while (true) {
        SOCKET clientSock = accept(m_listenSocket, nullptr, nullptr);
        if (clientSock == INVALID_SOCKET) {
            Sleep(100);
            continue;
        }

        ClientContext* context = new ClientContext{ clientSock, this };
        HANDLE hThread = CreateThread(
            nullptr, 0, ClientThread, context, 0, nullptr
        );

        if (hThread == nullptr) {
            closesocket(clientSock);
            delete context;
        }
        else {
            CloseHandle(hThread);
        }
    }
}
