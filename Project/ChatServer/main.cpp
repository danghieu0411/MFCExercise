#include <winsock2.h>
#include "TcpServer.h"

#pragma comment(lib, "Ws2_32.lib")

int main() {
	WSADATA wsa{};

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		return 1;
	}

	TcpServer server;
	server.Start(1206);

	WSACleanup();
	return 0;
}