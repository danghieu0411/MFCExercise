// CMCWeek5.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>

#pragma comment(lib, "Ws2_32.lib")

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
        ServiceCtrlHandler,
        nullptr
    );
    if (g_statusHandle == nullptr) return;
    g_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_status.dwCurrentState = SERVICE_START_PENDING;
    g_status.dwControlsAccepted = 0;
    g_status.dwWin32ExitCode = NO_ERROR;
    g_status.dwCheckPoint = 0;
    g_status.dwWaitHint = 3000;

    SetServiceStatus(g_statusHandle, &g_status);

    g_stopEvent = CreateEventW(
        nullptr,
        TRUE,
        FALSE,
        nullptr
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

    if (ioctlsocket(listenSocket, FIONBIO, &nonBlocking)
        == SOCKET_ERROR)
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

    while (WaitForSingleObject(g_stopEvent, 0) == WAIT_TIMEOUT)
    {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(listenSocket, &readSet);

        timeval timeout{ 0, 200000 };

        int ready = select(
            0,
            &readSet,
            nullptr,
            nullptr,
            &timeout
        );

        if (ready == SOCKET_ERROR)
            break;

        if (ready == 0)
            continue;

        SOCKET client = accept(listenSocket, nullptr, nullptr);

        if (client == INVALID_SOCKET)
            continue;

        // Client socket is also non-blocking
        char buffer[1024];

        while (WaitForSingleObject(g_stopEvent, 0) == WAIT_TIMEOUT)
        {
            fd_set clientReadSet;
            FD_ZERO(&clientReadSet);
            FD_SET(client, &clientReadSet);

            timeval clientTimeout{ 0, 200000 };

            int clientReady = select(
                0,
                &clientReadSet,
                nullptr,
                nullptr,
                &clientTimeout
            );

            if (clientReady == SOCKET_ERROR)
                break;

            if (clientReady == 0)
                continue;

            int received = recv(client, buffer, sizeof(buffer), 0);

            if (received <= 0)
                break;

            // Echo the bytes back to the client
            int total = 0;

            while (total < received)
            {
                fd_set writeSet;
                FD_ZERO(&writeSet);
                FD_SET(client, &writeSet);

                timeval writeTimeout{ 0, 200000 };

                int writable = select(
                    0,
                    nullptr,
                    &writeSet,
                    nullptr,
                    &writeTimeout
                );

                if (writable == SOCKET_ERROR)
                    break;

                if (writable == 0)
                {
                    if (WaitForSingleObject(g_stopEvent, 0)
                        != WAIT_TIMEOUT)
                        break;

                    continue;
                }

                int sent = send(
                    client,
                    buffer + total,
                    received - total,
                    0
                );

                if (sent <= 0)
                    break;

                total += sent;
            }

            if (total != received)
                break;
        }

        closesocket(client);
    }

    closesocket(listenSocket);
    WSACleanup();

    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
