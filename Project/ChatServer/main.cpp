#include <winsock2.h>
#include <Windows.h>
#include "TcpServer.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Advapi32.lib")

namespace {
    wchar_t serviceName[] = L"ChatServer";
    SERVICE_STATUS_HANDLE statusHandle = nullptr;
    HANDLE stopEvent = nullptr;

    void ReportStatus(DWORD state, DWORD error = NO_ERROR) {
        SERVICE_STATUS status{};
        status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
        status.dwCurrentState = state;
        status.dwControlsAccepted = (state == SERVICE_RUNNING) ? SERVICE_ACCEPT_STOP : 0;
        status.dwWin32ExitCode = error;
        if (state == SERVICE_START_PENDING || state == SERVICE_STOP_PENDING) {
            status.dwCheckPoint = 1;
            status.dwWaitHint = 5000;
        }
        SetServiceStatus(statusHandle, &status);
    }

    void WINAPI ServiceHandler(DWORD control) {
        if (control == SERVICE_CONTROL_STOP) {
            ReportStatus(SERVICE_STOP_PENDING);
            SetEvent(stopEvent);
        }
    }

    void WINAPI ServiceMain(DWORD, LPWSTR*) {
        statusHandle = RegisterServiceCtrlHandlerW(serviceName, ServiceHandler);
        if (!statusHandle)
            return;

        ReportStatus(SERVICE_START_PENDING);
        stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!stopEvent) {
            ReportStatus(SERVICE_STOPPED, GetLastError());
            return;
        }

        WSADATA wsa{};
        int wsaError = WSAStartup(MAKEWORD(2, 2), &wsa);
        if (wsaError != 0) {
            CloseHandle(stopEvent);
            ReportStatus(SERVICE_STOPPED, wsaError);
            return;
        }

        DWORD error = NO_ERROR;
        {
            TcpServer server;
            if (!server.Start(1206)) {
                error = ERROR_GEN_FAILURE;
            }
            else {
                ReportStatus(SERVICE_RUNNING);
                if (!server.Run(stopEvent))
                    error = ERROR_GEN_FAILURE;
                ReportStatus(SERVICE_STOP_PENDING);
            }
        }

        WSACleanup();
        CloseHandle(stopEvent);
        ReportStatus(SERVICE_STOPPED, error);
    }
}

int main() {
    SERVICE_TABLE_ENTRYW table[] = {
        { serviceName, ServiceMain },
        { nullptr, nullptr }
    };
    return StartServiceCtrlDispatcherW(table) ? 0 : static_cast<int>(GetLastError());
}
