#pragma once

#include <string>
#include <atlconv.h>

// MFC controls are Unicode (UTF-16); SQLite/text packets use UTF-8 bytes.
// Keep the two necessary conversions in one place.
inline std::string ToUtf8(const CString& text) {
    CStringA bytes(CW2A(text.GetString(), CP_UTF8));
    return std::string(bytes.GetString());
}

inline CString FromUtf8(const std::string& bytes) {
    return CString(CA2W(bytes.c_str(), CP_UTF8));
}
