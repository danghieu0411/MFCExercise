#pragma once

#include "TcpClient.h"
#include <set>
#include <string>
#include <vector>

class CLoginDlg;

struct ChatContact {
    int id;
    std::string username;
    int lastMessageId;
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
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnDestroy();
    afx_msg LRESULT OnStartLogin(WPARAM, LPARAM);
    afx_msg LRESULT OnTcpPacket(WPARAM, LPARAM);
    afx_msg LRESULT OnTcpDisconnected(WPARAM, LPARAM);
    afx_msg void OnSend();
    afx_msg void OnNewChat();
    afx_msg void OnContactChanged(NMHDR* hdr, LRESULT* result);
    afx_msg void OnContactDraw(NMHDR* hdr, LRESULT* result);
    DECLARE_MESSAGE_MAP()

private:
    enum class Request { None, Contacts, Messages, Send };

    void GetContacts();
    void GetMessages(const std::string& username);
    void UpdateContacts();
    void ShowMessages();
    void AppendHistory(const CString& text, COLORREF color, WORD align);
    void RequestFinished();
    void UpdateControls();
    int ContactId(const std::string& username) const;
    void ProcessPacket(const Packet& packet);

    HICON m_hIcon = nullptr;
    CListCtrl m_contacts;
    CRichEditCtrl m_history;
    CEdit m_messageInput;
    CFont m_boldFont;
    TcpClient m_client;
    CLoginDlg* m_loginDlg = nullptr;

    Request m_request = Request::None;
    std::string m_myUsername;
    std::string m_currentUsername;
    std::string m_loadingUsername;
    std::vector<ChatContact> m_contactList;
    std::vector<ChatContact> m_incomingContacts;
    std::vector<ChatEntry> m_messages;
    std::set<int> m_unread;

    int m_lastMessageId = 0;
    int m_batchCount = 0;
    bool m_rebuilding = false;
    bool m_refreshContacts = false;
    bool m_refreshMessagesAfterContacts = false;
    bool m_closing = false;
};
