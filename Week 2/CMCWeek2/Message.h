#pragma once

#include <afxstr.h>

struct Message {
	int id;
	int senderId;
	int receiverId;
	CString content;
	__int64 timeStamp;
};
