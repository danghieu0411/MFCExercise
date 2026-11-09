#pragma once

#include "TcpClient.h"
#include <set>
#include <string>
#include <vector>

class CLoginDlg;

struct ChatContact {
    int id;
    std::string username;
    std::string avatar;
};

struct ChatEntry {
    int id;
    int senderId;
    long long sentAt;
    std::string text;
};

class CChatClientDlg : public CDialogEx {
public:
    CChatClientDlg(CWnd* parent = nullptr);

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_CHATCLIENT_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnDestroy();
    afx_msg LRESULT OnStartLogin(WPARAM, LPARAM);
    afx_msg LRESULT OnTcpPacket(WPARAM, LPARAM);
    afx_msg LRESULT OnTcpDisconnected(WPARAM, LPARAM);
    afx_msg void OnBnClickedBtnSend();
    afx_msg void OnBnClickedBtnNewChat();
    afx_msg void OnLvnItemchangedListContacts(NMHDR* hdr, LRESULT* result);
    afx_msg void OnNMCustomdrawListContacts(NMHDR* hdr, LRESULT* result);
    DECLARE_MESSAGE_MAP()

private:
    enum class Request { None, Contacts, Messages, Send };

    void GetContacts();
    void GetMessages(const std::string& username);
    void UpdateContacts();
    void ShowMessages(size_t firstIndex = 0);
    void ShowMessage(const ChatEntry& message);
    void AppendHistory(const CString& text, COLORREF color, WORD align, COLORREF background = RGB(255, 255, 255));
    void RequestFinished();
    void UpdateControls();
    void ProcessPacket(const Packet& packet);
    void RefreshChat();
    HICON AvatarIcon(const std::string& avatar) const;

    HICON m_hIcon = nullptr;
    CListCtrl m_contacts;
    CImageList m_avatarImages;
    CStatic m_myAvatar;
    CStatic m_chatAvatar;
    CRichEditCtrl m_history;
    CEdit m_messageInput;
    CFont m_boldFont;
    TcpClient m_client;
    CLoginDlg* m_loginDlg = nullptr;

    Request m_request = Request::None; // specify WHICH response it is waiting from the server
    std::string m_myUsername;
    std::string m_currentUsername;
    std::string m_loadingUsername;
    std::vector<ChatContact> m_contactList;
    std::vector<ChatContact> m_incomingContacts;
    std::vector<ChatEntry> m_messages;
    std::vector<ChatEntry> m_incomingMessages;
    std::set<int> m_unread;

    int m_lastMessageId = 0;
    int m_myUserId = -1;
    int m_currentContactId = -1;
    bool m_rebuilding = false;
    bool m_refreshPending = false;
    bool m_closing = false;
};
