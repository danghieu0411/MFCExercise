// CMCWeek5.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <vector>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

struct ClientState {
    SOCKET socket;
    std::string pendingData;
};

wchar_t SERVICE_NAME[] = L"CMCWeek5";
SERVICE_STATUS_HANDLE g_statusHandle = nullptr;
SERVICE_STATUS g_status = {};
HANDLE g_stopEvent = nullptr;
HANDLE g_workerThread = nullptr;


void WINAPI ServiceMain(DWORD argc, LPWSTR* argv);

DWORD WINAPI ServiceCtrlHandler(
    DWORD control,
    DWORD eventType,
    LPVOID eventData,
    LPVOID context
);

DWORD WINAPI ServerThreadProc(LPVOID param);

int main()
{
    SERVICE_TABLE_ENTRYW serviceTable[] = {
        {SERVICE_NAME, ServiceMain},
        {nullptr, nullptr}
    };

    if (!StartServiceCtrlDispatcherW(serviceTable)) return 1;

    return 0;
}

DWORD WINAPI ServiceCtrlHandler(
    DWORD control,
    DWORD eventType,
    LPVOID eventData,
    LPVOID context
) {
    if (control == SERVICE_CONTROL_STOP) {
        g_status.dwCurrentState = SERVICE_STOP_PENDING;
        g_status.dwControlsAccepted = 0;

        SetServiceStatus(g_statusHandle, &g_status);
        SetEvent(g_stopEvent);
    }
    return NO_ERROR;
}

void WINAPI ServiceMain(DWORD agrc, LPWSTR* argv) {
    g_statusHandle = RegisterServiceCtrlHandlerExW(
        SERVICE_NAME,
        ServiceCtrlHandler, // pointer to the handler function
        nullptr // extra data to pass to the handler function
    );
    if (g_statusHandle == nullptr) return;

    g_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS; // service type, here: the service runs its own process
    g_status.dwCurrentState = SERVICE_START_PENDING;
    g_status.dwControlsAccepted = 0;
    g_status.dwWin32ExitCode = NO_ERROR; // use to report error occurring during start or stop, set to NO_ERROR when running or in normal termination
    g_status.dwCheckPoint = 0; // used to report progress in lengthy operation, not neccessary in this case
    g_status.dwWaitHint = 3000; // estimated time for a pending operation

    SetServiceStatus(g_statusHandle, &g_status);

    g_stopEvent = CreateEventW(
        nullptr, // security attribute, null means childs cannot inherit the handle
        TRUE, // manual reset is enabled
        FALSE,  // initial state is non-signaled
        nullptr // pointer to name of the event object, not used here
    );

    if (g_stopEvent == nullptr)
    {
        g_status.dwCurrentState = SERVICE_STOPPED;
        g_status.dwWin32ExitCode = GetLastError();
        g_status.dwControlsAccepted = 0;
        SetServiceStatus(g_statusHandle, &g_status);
        return;
    }

    g_workerThread = CreateThread(
        nullptr,
        0,
        ServerThreadProc,
        nullptr,
        0,
        nullptr
    );

    if (g_workerThread == nullptr)
    {
        DWORD error = GetLastError();

        CloseHandle(g_stopEvent);
        g_stopEvent = nullptr;

        g_status.dwCurrentState = SERVICE_STOPPED;
        g_status.dwControlsAccepted = 0;
        g_status.dwWin32ExitCode = error;
        g_status.dwWaitHint = 0;

        SetServiceStatus(g_statusHandle, &g_status);
        return;
    }

    g_status.dwCurrentState = SERVICE_RUNNING;
    g_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_status.dwCheckPoint = 0;
    g_status.dwWaitHint = 0;

    SetServiceStatus(g_statusHandle, &g_status);

    // wait for STOP
    WaitForSingleObject(g_stopEvent, INFINITE);

    g_status.dwCurrentState = SERVICE_STOP_PENDING;
    g_status.dwControlsAccepted = 0;
    g_status.dwWaitHint = 3000;
    SetServiceStatus(g_statusHandle, &g_status);

    WaitForSingleObject(g_workerThread, INFINITE);

    CloseHandle(g_workerThread);
    g_workerThread = nullptr;

    CloseHandle(g_stopEvent);
    g_stopEvent = nullptr;

    g_status.dwCurrentState = SERVICE_STOPPED;
    g_status.dwControlsAccepted = 0;
    g_status.dwWaitHint = 0;
    SetServiceStatus(g_statusHandle, &g_status);

}

DWORD WINAPI ServerThreadProc(LPVOID param)
{
    WSADATA wsaData{};

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return 1;

    SOCKET listenSocket = socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP
    );

    if (listenSocket == INVALID_SOCKET) {
        WSACleanup();
        return 1;
    }

    // Set socket to non-blocking mode
    u_long nonBlocking = 1;

    if (ioctlsocket(
        listenSocket, 
        FIONBIO, 
        &nonBlocking
    )== SOCKET_ERROR)
    {
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9999);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(
        listenSocket,
        reinterpret_cast<sockaddr*>(&serverAddr),
        sizeof(serverAddr)
    ) == SOCKET_ERROR)
    {
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    std::vector<ClientState> clients;
    const size_t MAX_PENDING = 64 * 1024;

    while (WaitForSingleObject(g_stopEvent, 0) == WAIT_TIMEOUT)
    {
        fd_set readSet;
        fd_set writeSet;

        FD_ZERO(&readSet);
        FD_ZERO(&writeSet);

        FD_SET(listenSocket, &readSet);

        for (const auto& client : clients) {
            FD_SET(client.socket, &readSet);

            if (!(client.pendingData.empty())) FD_SET(client.socket, &writeSet);
        }

        timeval timeout{ 0, 200000 };

        int ready = select(
            0,
            &readSet,
            &writeSet,
            nullptr,
            &timeout
        );

        if (ready == SOCKET_ERROR)
            break;

        if (ready == 0)
            continue;

        if (FD_ISSET(listenSocket, &readSet))
        {
            SOCKET newClient = accept(listenSocket, nullptr, nullptr);

            if (newClient != INVALID_SOCKET)
            {
                u_long clientMode = 1;

                if (clients.size() >= FD_SETSIZE - 1 ||
                    ioctlsocket(newClient, FIONBIO, &clientMode)
                    == SOCKET_ERROR)
                {
                    closesocket(newClient);
                }
                else
                {
                    clients.push_back({ newClient, {} });
                }
            }
        }
        
        for (size_t i = 0; i < clients.size(); )
        {
            ClientState& client = clients[i];
            bool disconnect = false;

            if (FD_ISSET(client.socket, &readSet))
            {
                char buffer[1024];

                int received = recv(
                    client.socket,
                    buffer,
                    sizeof(buffer),
                    0
                );

                if (received > 0)
                {
                    client.pendingData.append(buffer, received);

                    if (client.pendingData.size() > MAX_PENDING)
                        disconnect = true;
                }
                else if (received == 0)
                {
                    disconnect = true;
                }
                else if (WSAGetLastError() != WSAEWOULDBLOCK)
                {
                    disconnect = true;
                }
            }

            // Send pending echo data when writable
            if (!disconnect &&
                !client.pendingData.empty() &&
                FD_ISSET(client.socket, &writeSet))
            {
                int sent = send(
                    client.socket,
                    client.pendingData.data(),
                    (int)(client.pendingData.size()),
                    0
                );

                if (sent > 0)
                {
                    client.pendingData.erase(0, sent);
                }
                else if (sent == 0 ||
                    WSAGetLastError() != WSAEWOULDBLOCK)
                {
                    disconnect = true;
                }
            }

            if (disconnect)
            {
                closesocket(client.socket);
                clients.erase(clients.begin() + i);
            }
            else
            {
                ++i;
            }
        }
    }

    // Service is stopping: close all connections
    for (const auto& client : clients)
        closesocket(client.socket);

    closesocket(listenSocket);
    WSACleanup();

    return 0;
}
