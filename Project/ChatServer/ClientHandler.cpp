#include "ClientHandler.h"
#include "TcpServer.h"
#include "Protocol.h"

#include <string>
#include <charconv>
#include <ctime>
#include <system_error>

DWORD WINAPI ClientThread(LPVOID param) {
    ClientContext* context = (ClientContext*)param;
    SOCKET sock = context->socket;
    TcpServer* server = context->server;
    delete context;

    Database& db = server->GetDatabase();
    int currentUserId = -1;
    Packet packet;

    while (RecvPacket(sock, packet)) {
        bool sent = true;

        switch (packet.opcode) {
        case Opcode::Register: {
            size_t pos = packet.payload.find("|||");
            if (pos == std::string::npos) {
                sent = server->SendToClient(sock, { Opcode::Error, "INVALID_PAYLOAD" });
                break;
            }

            std::string username = packet.payload.substr(0, pos);
            std::string password = packet.payload.substr(pos + 3);
            if (username.empty() || password.empty() ||
                username.find('\0') != std::string::npos ||
                password.find('\0') != std::string::npos) {
                sent = server->SendToClient(sock, { Opcode::Error, "INVALID_PAYLOAD" });
                break;
            }

            bool success = db.RegisterUser(username, password);
            sent = server->SendToClient(sock, {
                success ? Opcode::Success : Opcode::Error,
                success ? "" : "REGISTER_FAILED"
            });
            break;
        }

        case Opcode::Login: {
            if (currentUserId != -1) {
                sent = server->SendToClient(sock, { Opcode::Error, "ALREADY_AUTHENTICATED" });
                break;
            }

            size_t pos = packet.payload.find("|||");
            if (pos == std::string::npos) {
                sent = server->SendToClient(sock, { Opcode::Error, "INVALID_PAYLOAD" });
                break;
            }

            std::string username = packet.payload.substr(0, pos);
            std::string password = packet.payload.substr(pos + 3);
            int userId = -1;
            if (username.empty() || password.empty() ||
                username.find('\0') != std::string::npos ||
                password.find('\0') != std::string::npos ||
                !db.VerifyUser(username, password, userId)) {
                sent = server->SendToClient(sock, { Opcode::Error, "INVALID_CREDENTIALS" });
                break;
            }

            if (!server->AddOnlineUser(userId, sock)) {
                sent = server->SendToClient(sock, { Opcode::Error, "ALREADY_LOGGED_IN" });
                break;
            }

            currentUserId = userId;
            sent = server->SendToClient(sock, { Opcode::Success, "" });
            break;
        }

        case Opcode::Logout: {
            if (currentUserId == -1) {
                sent = server->SendToClient(sock, { Opcode::Error, "NOT_AUTHENTICATED" });
                break;
            }

            server->RemoveOnlineUser(currentUserId, sock);
            currentUserId = -1;
            sent = server->SendToClient(sock, { Opcode::Success, "" });
            break;
        }

        case Opcode::SendMessage: {
            if (currentUserId == -1) {
                sent = server->SendToClient(sock, { Opcode::Error, "NOT_AUTHENTICATED" });
                break;
            }

            size_t pos = packet.payload.find("|||");
            if (pos == std::string::npos) {
                sent = server->SendToClient(sock, { Opcode::Error, "INVALID_PAYLOAD" });
                break;
            }

            std::string receiver = packet.payload.substr(0, pos);
            std::string content = packet.payload.substr(pos + 3);
            if (receiver.empty() || content.empty() ||
                receiver.find('\0') != std::string::npos ||
                content.find('\0') != std::string::npos) {
                sent = server->SendToClient(sock, { Opcode::Error, "INVALID_PAYLOAD" });
                break;
            }

            int receiverId = -1;
            if (!db.FindUserId(receiver, receiverId)) {
                sent = server->SendToClient(sock, { Opcode::Error, "USER_NOT_FOUND" });
                break;
            }

            if (!db.InsertMessage(currentUserId, receiverId, content, (int64_t)time(nullptr))) {
                sent = server->SendToClient(sock, { Opcode::Error, "DATABASE_ERROR" });
                break;
            }

            sent = server->SendToClient(sock, { Opcode::Success, "" });
            server->NotifyUser(receiverId, { Opcode::NewMessage, std::to_string(currentUserId) });
            break;
        }

        case Opcode::GetMessages: {
            if (currentUserId == -1) {
                sent = server->SendToClient(sock, { Opcode::Error, "NOT_AUTHENTICATED" });
                break;
            }

            size_t pos = packet.payload.find("|||");
            if (pos == std::string::npos) {
                sent = server->SendToClient(sock, { Opcode::Error, "INVALID_PAYLOAD" });
                break;
            }

            std::string otherUser = packet.payload.substr(0, pos);
            std::string idText = packet.payload.substr(pos + 3);
            int lastId = 0;
            auto parsed = std::from_chars(idText.data(), idText.data() + idText.size(), lastId);
            if (otherUser.empty() || parsed.ec != std::errc{} ||
                parsed.ptr != idText.data() + idText.size() || lastId < 0) {
                sent = server->SendToClient(sock, { Opcode::Error, "INVALID_PAYLOAD" });
                break;
            }

            int otherUserId = -1;
            if (!db.FindUserId(otherUser, otherUserId)) {
                sent = server->SendToClient(sock, { Opcode::Error, "USER_NOT_FOUND" });
                break;
            }

            auto messages = db.GetMessages(currentUserId, otherUserId, lastId);
            for (const auto& msg : messages) {
                std::string text = std::to_string(msg.id) + "|||" +
                    std::to_string(msg.senderId) + "|||" +
                    std::to_string(msg.receiverId) + "|||" +
                    std::to_string(msg.sentAt) + "|||" + msg.content;

                if (!server->SendToClient(sock, { Opcode::MessageItem, text })) {
                    sent = false;
                    break;
                }
            }

            if (sent)
                sent = server->SendToClient(sock, { Opcode::MessagesEnd, "" });
            break;
        }

        case Opcode::AddContact: {
            if (currentUserId == -1) {
                sent = server->SendToClient(sock, { Opcode::Error, "NOT_AUTHENTICATED" });
                break;
            }

            std::string username = packet.payload;
            if (username.empty() || username.find('\0') != std::string::npos) {
                sent = server->SendToClient(sock, { Opcode::Error, "INVALID_PAYLOAD" });
                break;
            }

            int contactId = -1;
            if (!db.FindUserId(username, contactId)) {
                sent = server->SendToClient(sock, { Opcode::Error, "USER_NOT_FOUND" });
                break;
            }

            if (contactId == currentUserId) {
                sent = server->SendToClient(sock, { Opcode::Error, "INVALID_CONTACT" });
                break;
            }

            bool success = db.AddContact(currentUserId, contactId);
            sent = server->SendToClient(sock, {
                success ? Opcode::Success : Opcode::Error,
                success ? "" : "DATABASE_ERROR"
            });
            break;
        }

        case Opcode::GetContacts: {
            if (currentUserId == -1) {
                sent = server->SendToClient(sock, { Opcode::Error, "NOT_AUTHENTICATED" });
                break;
            }

            auto contacts = db.GetContacts(currentUserId);
            for (const auto& contact : contacts) {
                std::string text = std::to_string(contact.userId) + "|||" +
                    contact.username + "|||" + std::to_string(contact.lastMessageId);

                if (!server->SendToClient(sock, { Opcode::ContactItem, text })) {
                    sent = false;
                    break;
                }
            }

            if (sent)
                sent = server->SendToClient(sock, { Opcode::ContactsEnd, "" });
            break;
        }

        default:
            sent = server->SendToClient(sock, { Opcode::Error, "UNKNOWN_OPCODE" });
            break;
        }

        if (!sent)
            break;
    }

    server->Disconnect(sock, currentUserId);
    return 0;
}
