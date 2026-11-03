#include "pch.h"
#include "Database.h"
#include <bcrypt.h>
#pragma comment (lib, "bcrypt.lib")

Database::Database() {
	m_db = nullptr;
}

Database::~Database() {
	Close();
}

bool Database::Open() {
	wchar_t exePath[MAX_PATH]{};
	DWORD length = GetModuleFileNameW(nullptr, exePath, MAX_PATH);
	if (length == 0 || length == MAX_PATH)
		return false;
	CString path(exePath);
	path = path.Left(path.ReverseFind(L'\\') + 1) + L"chat.db";
	int result = sqlite3_open16(path.GetString(), &m_db);

	if (result != SQLITE_OK) {
		return false;
	}

	return sqlite3_exec(m_db, "PRAGMA foreign_keys = ON;",
		nullptr, nullptr, nullptr) == SQLITE_OK;
}

void Database::Close() {
	if (m_db != nullptr) {
		sqlite3_close(m_db);
		m_db = nullptr;
	}
}

bool Database::Initialize() {

	const char* sql =
		"CREATE TABLE IF NOT EXISTS users ("
		"id INTEGER PRIMARY KEY AUTOINCREMENT,"
		"username TEXT NOT NULL UNIQUE,"
		"password_hash BLOB NOT NULL,"
		"salt BLOB NOT NULL,"
		"iterations INTEGER NOT NULL,"
		"avatar TEXT"
		");";

	char* errorMessage = nullptr;

	int result = sqlite3_exec(
		m_db, // an open database
		sql, // sql statement to be evaluated
		nullptr, // callback function, let it null
		nullptr, // 1st argument to callback function
		&errorMessage 
	);

	if (result != SQLITE_OK) {
		if (errorMessage != nullptr) {
			sqlite3_free(errorMessage);
		}
		return false;
	}

	const char* messageSql = 
		"CREATE TABLE IF NOT EXISTS messages ("
		"id INTEGER PRIMARY KEY AUTOINCREMENT,"
		"sender_id INTEGER NOT NULL,"
		"receiver_id INTEGER NOT NULL,"
		"content TEXT NOT NULL,"
		"sent_at INTEGER NOT NULL,"
		"FOREIGN KEY(sender_id) REFERENCES users(id),"
		"FOREIGN KEY(receiver_id) REFERENCES users(id)"
		");";

	result = sqlite3_exec(
		m_db,
		messageSql,
		nullptr,
		nullptr,
		&errorMessage
	);

	if (result != SQLITE_OK) {
		if (errorMessage != nullptr) {
			sqlite3_free(errorMessage);
		}
		return false;
	}

	return true;
}

bool Database::HashPassword(
	const CString& password,
	const std::vector<BYTE>& salt,
	int iterations,
	std::vector<BYTE>& hash
) {
	if (iterations <= 0 || salt.empty())
		return false;

	BCRYPT_ALG_HANDLE hAlg = nullptr;

	NTSTATUS status = BCryptOpenAlgorithmProvider(
		&hAlg, // pointer to an algorithm handle
		BCRYPT_SHA256_ALGORITHM, // the algorithm to use
		nullptr, // default provided
		BCRYPT_ALG_HANDLE_HMAC_FLAG
	);

	if (status != 0) return false;

	hash.resize(32);

	status = BCryptDeriveKeyPBKDF2(
		hAlg, // handle to an algorithm provider, with the flag set in above operation, it can be re-used here
		(PUCHAR)(LPCWSTR)password, // pointer to the buffer holding the password
		password.GetLength() * sizeof(wchar_t), // size of that buffer
		(PUCHAR)salt.data(), // pointer to salt buffer
		(ULONG)salt.size(), // its size
		iterations, // number of iterations
		hash.data(), //pointer to buffer holding the result key
		(ULONG)hash.size(), // its size, 32 in this case
		0
	);

	BCryptCloseAlgorithmProvider(hAlg, 0);

	return status == 0;
}

bool Database::GenerateSalt(std::vector<BYTE>& salt) {
	salt.resize(16);

	NTSTATUS status = BCryptGenRandom(
		nullptr, // hAlg
		salt.data(),
		(ULONG)salt.size(),
		BCRYPT_USE_SYSTEM_PREFERRED_RNG // when using this flag, the hAlg must be null
	);

	return status == 0;
}

bool Database::RegisterUser(const CString& username, const CString& password) {

	if (username.IsEmpty() || password.IsEmpty())
		return false;

	std::vector<BYTE> hash;
	std::vector<BYTE> salt;

	const int iterations = 200000;

	if (!GenerateSalt(salt)) {
		return false;
	}

	if (!HashPassword(password, salt, iterations, hash)) {
		return false;
	}

	const char* sql =
		"INSERT INTO users "
		"(username, password_hash, salt, iterations) "
		"VALUES (?, ?, ?, ?);";

	sqlite3_stmt* stmt = nullptr;

	int result = sqlite3_prepare_v2(
		m_db, // database handle
		sql, // the sql statement
		-1, // maximum length of the sql statement in bytes
		&stmt, // the statement handle
		nullptr
	);

	if (result != SQLITE_OK) {
		return false;
	}

	sqlite3_bind_text16(stmt, 1, username.GetString(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_blob(stmt, 2, hash.data(), (int)hash.size(), SQLITE_TRANSIENT);
	sqlite3_bind_blob(stmt, 3, salt.data(), (int)salt.size(), SQLITE_TRANSIENT);
	sqlite3_bind_int(stmt, 4, iterations);

	result = sqlite3_step(stmt);


	sqlite3_finalize(stmt);

	return result == SQLITE_DONE;
}

bool Database::VerifyUser(const CString& username, const CString& password, int& userId) {
	userId = -1;
	const char* sql =
		"SELECT id, password_hash, salt, iterations "
		"FROM users WHERE username = ?;";
	
	sqlite3_stmt* stmt = nullptr;
	int result = sqlite3_prepare_v2(
		m_db,
		sql,
		-1,
		&stmt,
		nullptr
	);

	if (result != SQLITE_OK) {
		return false;
	}

	sqlite3_bind_text16(stmt, 1, username.GetString(), -1, SQLITE_TRANSIENT);

	result = sqlite3_step(stmt);

	if (result != SQLITE_ROW) {
		sqlite3_finalize(stmt);
		return false;
	}

	int id = sqlite3_column_int(stmt, 0);

	const BYTE* storedHash = (const BYTE*)sqlite3_column_blob(stmt, 1); // get raw blob from row result
	int hashSize = sqlite3_column_bytes(stmt, 1); // this func returns size of a blob/utf-8 text in bytes

	const BYTE* storedSalt = (const BYTE*)sqlite3_column_blob(stmt, 2);
	int saltSize = sqlite3_column_bytes(stmt, 2);

	int iterations = sqlite3_column_int(stmt, 3);

	if (!storedHash || hashSize != 32 || !storedSalt || saltSize != 16 || iterations <= 0) {
		sqlite3_finalize(stmt);
		return false;
	}

	std::vector<BYTE> salt(storedSalt, storedSalt + saltSize);

	std::vector<BYTE> calculatedHash;

	if (!HashPassword(password, salt, iterations, calculatedHash)) {
		sqlite3_finalize(stmt);
		return false;
	}

	bool match =
		calculatedHash.size() == hashSize &&
		memcmp(calculatedHash.data(), storedHash, hashSize)==0;

	sqlite3_finalize(stmt);
	if (match)
		userId = id;
	return match;
}

bool Database::InsertMessage(
	int senderId,
	int receiverId,
	const CString& content,
	__int64 sendAt
) {
	if (senderId <= 0 || receiverId <= 0 || senderId == receiverId || content.IsEmpty())
		return false;

	const char* sql =
		"INSERT INTO messages "
		"(sender_id, receiver_id, content, sent_at) "
		"VALUES (?, ?, ?, ?);";

	sqlite3_stmt* stmt = nullptr;

	int result = sqlite3_prepare_v2(
		m_db,
		sql,
		-1,
		&stmt,
		nullptr
	);

	if (result != SQLITE_OK) {
		return false;
	}

	sqlite3_bind_int(stmt, 1, senderId);
	sqlite3_bind_int(stmt, 2, receiverId);

	sqlite3_bind_text16(stmt, 3, content.GetString(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int64(stmt, 4, sendAt);

	result = sqlite3_step(stmt);

	sqlite3_finalize(stmt);

	return result == SQLITE_DONE;

}

bool Database::LoadContact(int ownerUserId, std::vector<Contact>& contacts) {
	contacts.clear();
	const char* sql = "SELECT id, username FROM users WHERE id <> ? ORDER BY username;";
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
		return false;
	sqlite3_bind_int(stmt, 1, ownerUserId);
	int result;
	while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
		Contact contact{};
		contact.id = sqlite3_column_int(stmt, 0);
		contact.name = static_cast<const wchar_t*>(sqlite3_column_text16(stmt, 1));
		contacts.push_back(contact);
	}
	sqlite3_finalize(stmt);
	return result == SQLITE_DONE;
}

bool Database::LoadMessages(int userId, int contactId, int lastMessageId,
	std::vector<Message>& messages) {
	messages.clear();
	const char* sql =
		"SELECT id, sender_id, receiver_id, content, sent_at FROM messages "
		"WHERE ((sender_id = ? AND receiver_id = ?) "
		"OR (sender_id = ? AND receiver_id = ?)) AND id > ? ORDER BY id;";
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
		return false;

	sqlite3_bind_int(stmt, 1, userId);
	sqlite3_bind_int(stmt, 2, contactId);
	sqlite3_bind_int(stmt, 3, contactId);
	sqlite3_bind_int(stmt, 4, userId);
	sqlite3_bind_int(stmt, 5, lastMessageId);
	int result;
	while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
		Message message{};
		message.id = sqlite3_column_int(stmt, 0);
		message.senderId = sqlite3_column_int(stmt, 1);
		message.receiverId = sqlite3_column_int(stmt, 2);
		message.content = static_cast<const wchar_t*>(sqlite3_column_text16(stmt, 3));
		message.timeStamp = sqlite3_column_int64(stmt, 4);
		messages.push_back(message);
	}

	sqlite3_finalize(stmt);
	return result == SQLITE_DONE;
}
