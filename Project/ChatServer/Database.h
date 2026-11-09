#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "sqlite3.h"
#include <Windows.h>

struct ChatMessage {
    int id;
    int senderId;
    int receiverId;
    std::string content;
    int64_t sentAt;
};

struct ContactInfo {
    int userId;
    std::string username;
    std::string avatar;
};

class Database {
public:
    Database();
    ~Database();

    bool Open();
    void Close();
    bool Initialize();

    bool RegisterUser(const std::string& username,
        const std::string& password, const std::string& avatar);

    bool VerifyUser(const std::string& username,
        const std::string& password,
        int& userId);

    bool FindUserId(const std::string& username,
        int& userId);

    std::string GetAvatar(int userId);

    bool InsertMessage(int senderId, int receiverId,
        const std::string& content,
        int64_t sentAt);

    bool GetMessages(
        int userId, int otherUserId, int lastId, std::vector<ChatMessage>& messages);

    bool GetContacts(int userId, std::vector<ContactInfo>& contacts);

private:
    sqlite3* m_db = nullptr;
    CRITICAL_SECTION m_dbLock;
};

// as we may fprget to call LeaveCriticalSection() when multiple branchs exist, this helper class helps
class CriticalSectionGuard {
public:
    explicit CriticalSectionGuard(CRITICAL_SECTION& cs) // explicit prevents implicit cast, the argument passed to the constructor must be like declared
        : m_cs(cs) { // this is member initializer list, we cannot assign it in the constructor's body, as the reference type must be initialized with value
        EnterCriticalSection(&m_cs);
    }

    ~CriticalSectionGuard() {
        LeaveCriticalSection(&m_cs);
    }

private:
    CRITICAL_SECTION& m_cs;
};
