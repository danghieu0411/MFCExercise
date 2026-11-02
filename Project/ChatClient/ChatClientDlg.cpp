#include "pch.h"
#include "framework.h"
#include "ChatClient.h"
#include "ChatClientDlg.h"
#include "CLoginDlg.h"
#include "CNewChatDlg.h"
#include "TextUtil.h"
#include "afxdialogex.h"
#include <ctime>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

constexpr UINT WM_START_LOGIN = WM_APP + 3;

CChatClientDlg::CChatClientDlg(CWnd* parent)
    : CDialogEx(IDD_CHATCLIENT_DIALOG, parent) {
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CChatClientDlg::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_CONTACTS, m_contacts);
    DDX_Control(pDX, IDC_RICH_HISTORY, m_history);
    DDX_Control(pDX, IDC_EDIT_MESSAGE, m_messageInput);
}

BEGIN_MESSAGE_MAP(CChatClientDlg, CDialogEx)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_WM_DESTROY()
    ON_MESSAGE(WM_START_LOGIN, &CChatClientDlg::OnStartLogin)
    ON_MESSAGE(WM_TCP_PACKET, &CChatClientDlg::OnTcpPacket)
    ON_MESSAGE(WM_TCP_DISCONNECTED, &CChatClientDlg::OnTcpDisconnected)
    ON_BN_CLICKED(IDC_BTN_SEND, &CChatClientDlg::OnSend)
    ON_BN_CLICKED(IDC_BTN_NEW_CHAT, &CChatClientDlg::OnNewChat)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_CONTACTS, &CChatClientDlg::OnContactChanged)
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST_CONTACTS, &CChatClientDlg::OnContactDraw)
END_MESSAGE_MAP()

BOOL CChatClientDlg::OnInitDialog() {
    CDialogEx::OnInitDialog();

    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);

    m_contacts.SetExtendedStyle(m_contacts.GetExtendedStyle() | LVS_EX_FULLROWSELECT);
    CRect rect;
    m_contacts.GetClientRect(&rect);
    m_contacts.InsertColumn(0, L"Username", LVCFMT_LEFT, rect.Width() - 4);

    LOGFONT lf{};
    CFont* font = m_contacts.GetFont();
    if (font && font->GetLogFont(&lf)) {
        lf.lfWeight = FW_BOLD;
        m_boldFont.CreateFontIndirect(&lf);
    }

    m_history.SetReadOnly(TRUE);
    m_history.SetBackgroundColor(FALSE, RGB(255, 255, 255));
    UpdateControls();

    // Start after the main dialog has finished initializing.
    PostMessage(WM_START_LOGIN);
    return TRUE;
}

LRESULT CChatClientDlg::OnStartLogin(WPARAM, LPARAM) {
    if (!m_client.Connect("127.0.0.1", 1206, m_hWnd)) {
        AfxMessageBox(L"Cannot connect to server at 127.0.0.1:1206.");
        PostMessage(WM_CLOSE);
        return 0;
    }

    CLoginDlg login(m_client, this);
    m_loginDlg = &login;
    INT_PTR result = login.DoModal();
    m_loginDlg = nullptr;

    if (result != IDOK) {
        PostMessage(WM_CLOSE);
        return 0;
    }

    m_myUsername = login.Username();
    SetDlgItemText(IDC_USERNAME, L"username: " + FromUtf8(m_myUsername));
    GetContacts();
    return 0;
}

void CChatClientDlg::GetContacts() {
    if (!m_client.Send({ Opcode::GetContacts, "" })) {
        AfxMessageBox(L"Cannot request contacts.");
        return;
    }
    m_request = Request::Contacts;
    m_incomingContacts.clear();
    UpdateControls();
}

void CChatClientDlg::GetMessages(const std::string& username) {
    if (username.empty())
        return;

    if (!m_client.Send({ Opcode::GetMessages, username + "|||0" })) {
        AfxMessageBox(L"Cannot request messages.");
        return;
    }
    m_request = Request::Messages;
    m_loadingUsername = username;
    m_messages.clear();
    m_lastMessageId = 0;
    m_batchCount = 0;
    UpdateControls();
}

int CChatClientDlg::ContactId(const std::string& username) const {
    for (const auto& contact : m_contactList) {
        if (contact.username == username)
            return contact.id;
    }
    return -1;
}

void CChatClientDlg::UpdateContacts() {
    m_rebuilding = true;
    m_contacts.DeleteAllItems();
    for (int i = 0; i < (int)m_contactList.size(); ++i) {
        const auto& contact = m_contactList[i];
        int row = m_contacts.InsertItem(i, FromUtf8(contact.username));
        m_contacts.SetItemData(row, (DWORD_PTR)contact.id);
        if (contact.username == m_currentUsername)
            m_contacts.SetItemState(row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }
    m_rebuilding = false;
    m_contacts.Invalidate();
}

void CChatClientDlg::UpdateControls() {
    bool ready = !m_myUsername.empty() && m_request == Request::None;
    m_contacts.EnableWindow(ready);
    GetDlgItem(IDC_BTN_NEW_CHAT)->EnableWindow(ready);
    GetDlgItem(IDC_BTN_SEND)->EnableWindow(ready && !m_currentUsername.empty());
    m_messageInput.EnableWindow(ready && !m_currentUsername.empty());
}

void CChatClientDlg::RequestFinished() {
    m_request = Request::None;
    if (m_refreshContacts) {
        m_refreshContacts = false;
        GetContacts();
    }
    else {
        UpdateControls();
    }
}

void CChatClientDlg::ProcessPacket(const Packet& packet) {
    // Server pushes a sender ID. The next contacts query provides the username.
    if (packet.opcode == Opcode::NewMessage) {
        int senderId = std::stoi(packet.payload);
        if (senderId == ContactId(m_currentUsername)) {
            m_refreshMessagesAfterContacts = true;
            m_unread.erase(senderId);
        }
        else {
            m_unread.insert(senderId);
        }
        m_refreshContacts = true;
        if (m_request == Request::None)
            RequestFinished();
        return;
    }

    if (m_request == Request::Contacts) {
        if (packet.opcode == Opcode::ContactItem) {
            size_t a = packet.payload.find("|||");
            size_t b = packet.payload.find("|||", a + 3);
            ChatContact contact{};
            contact.id = std::stoi(packet.payload.substr(0, a));
            contact.username = packet.payload.substr(a + 3, b - a - 3);
            contact.lastMessageId = std::stoi(packet.payload.substr(b + 3));
            m_incomingContacts.push_back(contact);
        }
        else if (packet.opcode == Opcode::ContactsEnd) {
            m_contactList = m_incomingContacts; // Already sorted by the server
            UpdateContacts();
            if (m_refreshMessagesAfterContacts && !m_currentUsername.empty()) {
                m_refreshMessagesAfterContacts = false;
                GetMessages(m_currentUsername);
            }
            else {
                RequestFinished();
            }
        }
    }
    else if (m_request == Request::Messages) {
        if (packet.opcode == Opcode::MessageItem) {
            const std::string& s = packet.payload;
            size_t a = s.find("|||");
            size_t b = s.find("|||", a + 3);
            size_t c = s.find("|||", b + 3);
            size_t d = s.find("|||", c + 3);
            ChatEntry msg{};
            msg.id = std::stoi(s.substr(0, a));
            msg.senderId = std::stoi(s.substr(a + 3, b - a - 3));
            msg.sentAt = std::stoll(s.substr(c + 3, d - c - 3));
            msg.text = s.substr(d + 3); // Content itself may contain "|||".
            m_messages.push_back(msg);
            m_lastMessageId = msg.id;
            ++m_batchCount;
        }
        else if (packet.opcode == Opcode::MessagesEnd) {
            // The server sends at most 100 messages per request.
            if (m_batchCount == 100) {
                m_batchCount = 0;
                m_client.Send({ Opcode::GetMessages,
                    m_loadingUsername + "|||" + std::to_string(m_lastMessageId) });
                return;
            }
            m_currentUsername = m_loadingUsername;
            m_unread.erase(ContactId(m_currentUsername));
            ShowMessages();
            UpdateContacts();
            RequestFinished();
        }
        else if (packet.opcode == Opcode::Error) {
            AfxMessageBox(L"Cannot open conversation: " + FromUtf8(packet.payload));
            UpdateContacts();
            RequestFinished();
        }
    }
    else if (m_request == Request::Send) {
        if (packet.opcode == Opcode::Success) {
            m_messageInput.SetWindowText(L"");
            m_refreshMessagesAfterContacts = true;
            GetContacts();
        }
        else if (packet.opcode == Opcode::Error) {
            AfxMessageBox(L"Send failed: " + FromUtf8(packet.payload));
            RequestFinished();
        }
    }
}

LRESULT CChatClientDlg::OnTcpPacket(WPARAM, LPARAM lParam) {
    Packet* packet = (Packet*)lParam;
    if (m_loginDlg)
        m_loginDlg->HandleResponse(*packet);
    else
        ProcessPacket(*packet);
    delete packet;
    return 0;
}

LRESULT CChatClientDlg::OnTcpDisconnected(WPARAM, LPARAM) {
    if (m_closing)
        return 0;
    m_closing = true;
    AfxMessageBox(L"Connection to server lost.");
    if (m_loginDlg)
        m_loginDlg->CloseOnDisconnect();
    PostMessage(WM_CLOSE);
    return 0;
}

void CChatClientDlg::OnContactChanged(NMHDR* hdr, LRESULT* result) {
    *result = 0;
    NMLISTVIEW* item = (NMLISTVIEW*)hdr;
    if (m_rebuilding || m_request != Request::None || m_myUsername.empty())
        return;
    if (!(item->uNewState & LVIS_SELECTED) || (item->uOldState & LVIS_SELECTED))
        return;
    if (item->iItem < 0)
        return;

    int userId = (int)m_contacts.GetItemData(item->iItem);
    for (const auto& c : m_contactList) {
        if (c.id == userId) {
            GetMessages(c.username);
            return;
        }
    }
}

void CChatClientDlg::OnContactDraw(NMHDR* hdr, LRESULT* result) {
    NMLVCUSTOMDRAW* draw = (NMLVCUSTOMDRAW*)hdr;
    if (draw->nmcd.dwDrawStage == CDDS_PREPAINT) {
        *result = CDRF_NOTIFYITEMDRAW;
    }
    else if (draw->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
        int row = (int)draw->nmcd.dwItemSpec;
        if (m_unread.count((int)m_contacts.GetItemData(row)) && m_boldFont.GetSafeHandle()) {
            ::SelectObject(draw->nmcd.hdc, m_boldFont.GetSafeHandle());
            *result = CDRF_NEWFONT;
        }
        else {
            *result = CDRF_DODEFAULT;
        }
    }
    else {
        *result = CDRF_DODEFAULT;
    }
}

void CChatClientDlg::OnNewChat() {
    if (m_request != Request::None)
        return;
    CNewChatDlg dlg(this);
    if (dlg.DoModal() == IDOK) {
        std::string username = ToUtf8(dlg.Username());
        if (username == m_myUsername) {
            AfxMessageBox(L"Choose a different user.");
            return;
        }
        // GetMessages checks whether this username actually exists.
        GetMessages(username);
    }
}

void CChatClientDlg::OnSend() {
    if (m_request != Request::None || m_currentUsername.empty())
        return;

    CString message;
    m_messageInput.GetWindowText(message);
    CString trimmed = message;
    trimmed.Trim();
    if (trimmed.IsEmpty())
        return;

    if (!m_client.Send({ Opcode::SendMessage,
        m_currentUsername + "|||" + ToUtf8(message) })) {
        AfxMessageBox(L"Cannot send message.");
        return;
    }

    m_request = Request::Send;
    UpdateControls();
}

void CChatClientDlg::AppendHistory(const CString& text, COLORREF color, WORD align) {
    m_history.SetSel(-1, -1);
    PARAFORMAT pf{};
    pf.cbSize = sizeof(pf);
    pf.dwMask = PFM_ALIGNMENT;
    pf.wAlignment = align;
    m_history.SetParaFormat(pf);

    CHARFORMAT cf{};
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = color;
    m_history.SetSelectionCharFormat(cf);
    m_history.ReplaceSel(text);
}

void CChatClientDlg::ShowMessages() {
    m_history.SetReadOnly(FALSE);
    m_history.SetWindowText(L"");
    SetDlgItemText(IDC_STATIC_CHAT_TITLE, L"Chat with: " + FromUtf8(m_currentUsername));
    int otherId = ContactId(m_currentUsername);

    for (const auto& msg : m_messages) {
        bool fromOther = msg.senderId == otherId;
        CString sender = fromOther ? FromUtf8(m_currentUsername) : L"You";
        CTime time((time_t)msg.sentAt);
        CString header = sender + L"  " + time.Format(L"%d/%m/%Y %H:%M");
        WORD alignment = fromOther ? PFA_LEFT : PFA_RIGHT;

        AppendHistory(header + L"\r\n", RGB(120, 120, 120), alignment);
        AppendHistory(FromUtf8(msg.text) + L"\r\n\r\n", RGB(30, 30, 30), alignment);
    }
    m_history.SetSel(-1, -1);
    m_history.LineScroll(m_history.GetLineCount());
    m_history.SetReadOnly(TRUE);
}

void CChatClientDlg::OnDestroy() {
    m_closing = true;
    m_client.Disconnect();

    // Receive thread posts heap-allocated packets; free unprocessed ones.
    MSG msg{};
    while (::PeekMessage(&msg, m_hWnd, WM_TCP_PACKET, WM_TCP_PACKET, PM_REMOVE))
        delete (Packet*)msg.lParam;

    CDialogEx::OnDestroy();
}

void CChatClientDlg::OnPaint() {
    if (IsIconic()) {
        CPaintDC dc(this);
        SendMessage(WM_ICONERASEBKGND, (WPARAM)dc.GetSafeHdc(), 0);
        CRect rect;
        GetClientRect(&rect);
        dc.DrawIcon((rect.Width() - GetSystemMetrics(SM_CXICON)) / 2,
                    (rect.Height() - GetSystemMetrics(SM_CYICON)) / 2, m_hIcon);
    }
    else {
        CDialogEx::OnPaint();
    }
}

HCURSOR CChatClientDlg::OnQueryDragIcon() {
    return (HCURSOR)m_hIcon;
}
