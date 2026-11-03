#include "TcpServer.h"
#include "ClientHandler.h"
#include <iostream>
#include <vector>

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
    auto it = m_onlineUsers.find(userId); // the type of 'it' is indeed an iterator, we use it like a pointer with ->
    if (it != m_onlineUsers.end() && it->second == socket)
        m_onlineUsers.erase(it);
}

bool TcpServer::SendToClient(SOCKET socket, const Packet& packet) {
    CriticalSectionGuard lock(m_onlineLock);
    return SendPacket(socket, packet, m_stopEvent);
}

bool TcpServer::NotifyUser(int userId, const Packet& packet) {
    CriticalSectionGuard lock(m_onlineLock);
    auto it = m_onlineUsers.find(userId);
    if (it == m_onlineUsers.end())
        return false;
    return SendPacket(it->second, packet, m_stopEvent);
}

HANDLE TcpServer::GetStopEvent() const {
    return m_stopEvent;
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

    u_long nonBlocking = 1;
    return ioctlsocket(m_listenSocket, FIONBIO, &nonBlocking) != SOCKET_ERROR;
}

bool TcpServer::Run(HANDLE stopEvent) {
    m_stopEvent = stopEvent;
    std::vector<HANDLE> clientThreads;
    bool success = true;

    while (WaitForSingleObject(stopEvent, 0) == WAIT_TIMEOUT) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(m_listenSocket, &readSet);
        timeval timeout{ 0, 200000 }; 
        int ready = select(0, &readSet, nullptr, nullptr, &timeout);
        if (ready == SOCKET_ERROR) {
            success = false;
            break;
        }
        if (ready == 0)
            continue;
        if (WaitForSingleObject(stopEvent, 0) != WAIT_TIMEOUT)
            break;

        SOCKET clientSock = accept(m_listenSocket, nullptr, nullptr);
        if (clientSock == INVALID_SOCKET) {
            if (WSAGetLastError() == WSAEWOULDBLOCK)
                continue;
            success = false;
            break;
        }

        u_long nonBlocking = 1;
        if (ioctlsocket(clientSock, FIONBIO, &nonBlocking) == SOCKET_ERROR) {
            closesocket(clientSock);
            continue;
        }

        ClientContext* context = new ClientContext{ clientSock, this };
        HANDLE hThread = CreateThread(
            nullptr, 0, ClientThread, context, 0, nullptr
        );

        if (hThread == nullptr) {
            Disconnect(clientSock, -1);
            delete context;
        }
        else {
            clientThreads.push_back(hThread);
        }
    }

    SetEvent(stopEvent); // stop client thread if some error happens during the network loop

    for (HANDLE thread : clientThreads) {
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
    }
    return success;
}
