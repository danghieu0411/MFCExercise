#pragma once

#include "sqlite3.h"
#include <vector>
#include "Contact.h"
#include "Message.h"

class Database {

private:
	sqlite3* m_db;

	bool GenerateSalt(std::vector<BYTE>& salt);

	bool HashPassword(
		const CString& password,
		const std::vector<BYTE>& salt,
		int iterations,
		std::vector<BYTE>& hash
	);

public:
	Database();
	~Database();

	bool Open();
	void Close();
	bool Initialize();

	bool RegisterUser(const CString& username, const CString& password);
	bool VerifyUser(const CString& username, const CString& password, int& userId);

	bool InsertMessage(
		int senderId,
		int receiverId,
		const CString& content,
		__int64 sendAt
	);

	bool LoadContact(int ownerUserId, std::vector<Contact>& contacts);
	bool LoadMessages(int userId, int contactId, int lastMessageId,
		std::vector<Message>& messages);
};
