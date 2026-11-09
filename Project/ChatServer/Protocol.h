#pragma once

#include <cstdint>
#include <string>
#include <winsock2.h>

enum class Opcode : uint8_t {
    Register = 0x01,
    Login = 0x02,
    Logout = 0x03,
    SendMessage = 0x04,
    GetMessages = 0x05,
    GetContacts = 0x06,

    RegisterOk = 0x80,
    Error = 0x81,
    MessageItem = 0x82,
    MessagesEnd = 0x83,
    NewMessage = 0x84,
    ContactItem = 0x85,
    ContactsEnd = 0x86,
    LoginOk = 0x87,
    MessageSent = 0x88,
    LogoutOk = 0x89
};

struct Packet {
    Opcode opcode;
    std::string payload;
};

bool SendAll(SOCKET sock, const char* buffer, int length, HANDLE stopEvent = nullptr);
bool RecvAll(SOCKET sock, char* buffer, int length, HANDLE stopEvent = nullptr);

bool SendPacket(SOCKET sock, const Packet& packet, HANDLE stopEvent = nullptr);
bool RecvPacket(SOCKET sock, Packet& packet, HANDLE stopEvent = nullptr);
