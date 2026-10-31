#pragma comment(lib, "Ws2_32.lib")

#include "Protocol.h"
#include <utility>

const uint16_t MAX_PACKET_SIZE = 65535;

bool SendAll(SOCKET sock, const char* buffer, int length) {
    int total = 0;
    while (total < length) {
        int sent = send(sock, buffer + total, length - total, 0);
        if (sent == SOCKET_ERROR || sent == 0) {
            return false;
        }
        total += sent;
    }
    return true;
}

bool RecvAll(SOCKET sock, char* buffer, int length) {
    int total = 0;
    while (total < length) {
        int received = recv(sock, buffer + total, length - total, 0);
        if (received == SOCKET_ERROR || received == 0) {
            return false;
        }
        total += received;
    }
    return true;
}

bool SendPacket(SOCKET sock, const Packet& packet) {
    if (packet.payload.size() > MAX_PACKET_SIZE - 1)
        return false;

    uint16_t length = 1 + (uint16_t)packet.payload.size();
    uint16_t networkLength = htons(length); // em giữ cái này để sau thử test với kiến trúc khác 

    if (!SendAll(sock, (const char*)&networkLength, sizeof(networkLength)))
        return false;

    uint8_t opcode = (uint8_t)packet.opcode;
    if (!SendAll(sock, (const char*)&opcode, 1))
        return false;

    return SendAll(sock, packet.payload.data(), (int)length - 1);
}

bool RecvPacket(SOCKET sock, Packet& packet) {
    uint16_t networkLength = 0;
    if (!RecvAll(sock, (char*)&networkLength, sizeof(networkLength)))
        return false;

    uint16_t length = ntohs(networkLength);
    if (length < 1)
        return false;

    uint8_t opcode = 0;
    if (!RecvAll(sock, (char*)&opcode, 1))
        return false;

    std::string payload(length - 1, '\0'); // omit the 1-byte opcode
    if (!RecvAll(sock, payload.data(), (int)length - 1))
        return false;

    packet.opcode = (Opcode)opcode;
    packet.payload = std::move(payload);
    return true;
}
