#include "Database.h"
#include "sha256.h"

#pragma comment(lib, "winsqlite3.lib")

Database::Database() {
    InitializeCriticalSection(&m_dbLock);
}

Database::~Database() {
    Close();
    DeleteCriticalSection(&m_dbLock);
}

bool Database::Open() {
    wchar_t exePath[MAX_PATH]{};
    DWORD len = GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    if (len == 0 || len == MAX_PATH)
        return false;

    std::wstring path(exePath, len);
    path.resize(path.find_last_of(L"\\/") + 1);
    path += L"chat.db";
    return sqlite3_open16(path.c_str(), &m_db) == SQLITE_OK;
}

void Database::Close() {
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

bool Database::Initialize() {
    const char* sql =
        "PRAGMA foreign_keys = ON;"

        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "username TEXT NOT NULL UNIQUE,"
        "password_hash TEXT NOT NULL," // the sha256 lib returns text instead of raw bytes
        "avatar TEXT"
        ");"

        "CREATE TABLE IF NOT EXISTS messages ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "sender_id INTEGER NOT NULL,"
        "receiver_id INTEGER NOT NULL,"
        "content TEXT NOT NULL,"
        "sent_at INTEGER NOT NULL,"
        "FOREIGN KEY(sender_id) REFERENCES users(id),"
        "FOREIGN KEY(receiver_id) REFERENCES users(id)"
        ");"

        "CREATE TABLE IF NOT EXISTS contacts ("
        "owner_user_id INTEGER NOT NULL,"
        "contact_user_id INTEGER NOT NULL,"
        "PRIMARY KEY(owner_user_id, contact_user_id),"
        "FOREIGN KEY(owner_user_id) REFERENCES users(id),"
        "FOREIGN KEY(contact_user_id) REFERENCES users(id)"
        ");";

    return sqlite3_exec(m_db, sql, nullptr, nullptr, nullptr) == SQLITE_OK;
}

bool Database::RegisterUser(const std::string& username, const std::string& password) {
    CriticalSectionGuard lock(m_dbLock);
    SHA256 sha256;
    std::string hash = sha256(password);

    const char* sql =
        "INSERT INTO users (username, password_hash) "
        "VALUES (?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, hash.c_str(), -1, SQLITE_TRANSIENT);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

bool Database::VerifyUser(const std::string& username,
    const std::string& password, int& userId)
{
    CriticalSectionGuard lock(m_dbLock);
    const char* sql =
        "SELECT id, password_hash FROM users WHERE username = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    bool valid = false;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char* storedHash = (const char*)sqlite3_column_text(stmt, 1);

        SHA256 sha256;
        std::string calculatedHash = sha256(password);
        if (storedHash && calculatedHash == storedHash) {
            userId = id;
            valid = true;
        }
    }

    sqlite3_finalize(stmt);
    return valid;
}

bool Database::FindUserId(const std::string& username, int& userId)
{
    CriticalSectionGuard lock(m_dbLock);
    const char* sql = "SELECT id FROM users WHERE username = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    bool found = false;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        userId = sqlite3_column_int(stmt, 0);
        found = true;
    }

    sqlite3_finalize(stmt);
    return found;
}

bool Database::InsertMessage(int senderId, int receiverId,
    const std::string& content, int64_t sentAt)
{
    CriticalSectionGuard lock(m_dbLock);
    const char* sql =
        "INSERT INTO messages (sender_id, receiver_id, content, sent_at) "
        "VALUES (?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(stmt, 1, senderId);
    sqlite3_bind_int(stmt, 2, receiverId);
    sqlite3_bind_text(stmt, 3, content.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, sentAt);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

std::vector<ChatMessage> Database::GetMessages(int userId, int otherUserId, int lastId) {
    CriticalSectionGuard lock(m_dbLock);
    std::vector<ChatMessage> messages;

    const char* sql =
        "SELECT id, sender_id, receiver_id, content, sent_at "
        "FROM messages "
        "WHERE ((sender_id = ? AND receiver_id = ?) "
        "OR (sender_id = ? AND receiver_id = ?)) "
        "AND id > ? "
        "ORDER BY id ASC LIMIT 100;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return messages;

    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int(stmt, 2, otherUserId);
    sqlite3_bind_int(stmt, 3, otherUserId);
    sqlite3_bind_int(stmt, 4, userId);
    sqlite3_bind_int(stmt, 5, lastId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ChatMessage msg{};
        msg.id = sqlite3_column_int(stmt, 0);
        msg.senderId = sqlite3_column_int(stmt, 1);
        msg.receiverId = sqlite3_column_int(stmt, 2);

        const char* text = (const char*)sqlite3_column_text(stmt, 3);
        if (text)
            msg.content = text;

        msg.sentAt = sqlite3_column_int64(stmt, 4);
        messages.push_back(msg);
    }

    sqlite3_finalize(stmt);
    return messages;
}

bool Database::AddContact(int userId, int contactId) {
    CriticalSectionGuard lock(m_dbLock);
    const char* sql =
        "INSERT OR IGNORE INTO contacts (owner_user_id, contact_user_id) VALUES (?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int(stmt, 2, contactId);
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

std::vector<ContactInfo> Database::GetContacts(int userId) {
    CriticalSectionGuard lock(m_dbLock);
    std::vector<ContactInfo> contacts;

    const char* sql =
        "SELECT u.id, u.username, "
        "COALESCE((SELECT MAX(m.id) FROM messages m "
        "WHERE (m.sender_id = ? AND m.receiver_id = u.id) "
        "OR (m.receiver_id = ? AND m.sender_id = u.id)), 0) AS last_id "
        "FROM users u WHERE u.id IN ("
        "SELECT contact_user_id FROM contacts WHERE owner_user_id = ? "
        "UNION SELECT receiver_id FROM messages WHERE sender_id = ? "
        "UNION SELECT sender_id FROM messages WHERE receiver_id = ?) "
        "ORDER BY last_id DESC, u.username COLLATE NOCASE;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return contacts;

    for (int i = 1; i <= 5; ++i)
        sqlite3_bind_int(stmt, i, userId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ContactInfo contact{};
        contact.userId = sqlite3_column_int(stmt, 0);
        const char* name = (const char*)sqlite3_column_text(stmt, 1);
        if (name)
            contact.username = name;
        contact.lastMessageId = sqlite3_column_int(stmt, 2);
        contacts.push_back(contact);
    }

    sqlite3_finalize(stmt);
    return contacts;
}
