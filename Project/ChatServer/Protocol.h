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
    AddContact = 0x07,

    Success = 0x80,
    Error = 0x81,
    MessageItem = 0x82,
    MessagesEnd = 0x83,
    NewMessage = 0x84,
    ContactItem = 0x85,
    ContactsEnd = 0x86
};

struct Packet {
    Opcode opcode;
    std::string payload;
};

bool SendAll(SOCKET sock, const char* buffer, int length);
bool RecvAll(SOCKET sock, char* buffer, int length);

bool SendPacket(SOCKET sock, const Packet& packet);
bool RecvPacket(SOCKET sock, Packet& packet);