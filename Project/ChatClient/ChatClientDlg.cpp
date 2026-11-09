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
    DDX_Control(pDX, IDC_PIC_MY_AVATAR, m_myAvatar);
    DDX_Control(pDX, IDC_PIC_CHAT_AVATAR, m_chatAvatar);
}

BEGIN_MESSAGE_MAP(CChatClientDlg, CDialogEx)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_WM_DESTROY()
    ON_MESSAGE(WM_START_LOGIN, &CChatClientDlg::OnStartLogin)
    ON_MESSAGE(WM_TCP_PACKET, &CChatClientDlg::OnTcpPacket)
    ON_MESSAGE(WM_TCP_DISCONNECTED, &CChatClientDlg::OnTcpDisconnected)
    ON_BN_CLICKED(IDC_BTN_SEND, &CChatClientDlg::OnBnClickedBtnSend)
    ON_BN_CLICKED(IDC_BTN_NEW_CHAT, &CChatClientDlg::OnBnClickedBtnNewChat)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_CONTACTS, &CChatClientDlg::OnLvnItemchangedListContacts)
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST_CONTACTS, &CChatClientDlg::OnNMCustomdrawListContacts)
END_MESSAGE_MAP()


BOOL CChatClientDlg::OnInitDialog() {
    CDialogEx::OnInitDialog();

    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);

    m_avatarImages.Create(32, 32, ILC_COLOR32 | ILC_MASK, 5, 0);
    m_avatarImages.Add(AfxGetApp()->LoadIcon(IDI_AVATAR_ROBOT));
    m_avatarImages.Add(AfxGetApp()->LoadIcon(IDI_AVATAR_DOG));
    m_avatarImages.Add(AfxGetApp()->LoadIcon(IDI_AVATAR_CAT));
    m_avatarImages.Add(AfxGetApp()->LoadIcon(IDI_AVATAR_LEAF));
    m_avatarImages.Add(AfxGetApp()->LoadIcon(IDI_AVATAR_SUN));
    m_contacts.SetImageList(&m_avatarImages, LVSIL_SMALL);

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

    m_history.SetBackgroundColor(FALSE, RGB(255, 255, 255));
    UpdateControls();

    // Start after the main dialog has finished initializing.
    PostMessage(WM_START_LOGIN);
    return TRUE;
}

void CChatClientDlg::OnOK() {}

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
    m_myUserId = login.UserId();
    SetDlgItemText(IDC_USERNAME, L"username: " + FromUtf8(m_myUsername));
    m_myAvatar.SetIcon(AvatarIcon(login.Avatar()));
    GetContacts();
    return 0;
}

void CChatClientDlg::GetContacts() {
    if (!m_client.Send({ Opcode::GetContacts, "" })) {
        OnTcpDisconnected(0, 0);
        return;
    }
    m_request = Request::Contacts;
    m_incomingContacts.clear();
    UpdateControls();
}

void CChatClientDlg::GetMessages(const std::string& username) {
    if (username.empty())
        return;

    int lastId = username == m_currentUsername ? m_lastMessageId : 0;

    std::string payload = username + "|||" + std::to_string(lastId);

    if (!m_client.Send({ Opcode::GetMessages, payload })) {
        OnTcpDisconnected(0, 0);
        return;
    }
    m_request = Request::Messages;
    m_loadingUsername = username;
    m_incomingMessages.clear();
    UpdateControls();
}

void CChatClientDlg::UpdateContacts() {
    m_rebuilding = true;
    m_contacts.DeleteAllItems();
    for (int i = 0; i < (int)m_contactList.size(); ++i) {
        const auto& contact = m_contactList[i];
        int image = 0;
        if (contact.avatar == "dog") image = 1;
        else if (contact.avatar == "cat") image = 2;
        else if (contact.avatar == "leaf") image = 3;
        else if (contact.avatar == "sun") image = 4;

        int row = m_contacts.InsertItem(i, FromUtf8(contact.username), image);
        m_contacts.SetItemData(row, (DWORD_PTR)contact.id);
        if (contact.username == m_currentUsername)
            m_contacts.SetItemState(row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }
    m_rebuilding = false;
    m_contacts.Invalidate(); // tell windows to re-draw the list control
}

void CChatClientDlg::UpdateControls() {
    bool ready = m_myUserId > 0 &&
        m_request == Request::None && !m_closing;
    bool chatting = ready && m_currentContactId > 0;
    m_contacts.EnableWindow(ready);
    GetDlgItem(IDC_BTN_NEW_CHAT)->EnableWindow(ready);
    GetDlgItem(IDC_BTN_SEND)->EnableWindow(chatting);
    m_messageInput.EnableWindow(chatting);
}

void CChatClientDlg::RequestFinished() {
    m_request = Request::None;

    if (m_closing) {
        return;
    }
    if (m_refreshPending && IsWindowEnabled()) {
        RefreshChat();
    }
    else {
        UpdateControls();
    }
}

void CChatClientDlg::ProcessPacket(const Packet& packet) {
    if (m_closing) return;

    if (packet.opcode == Opcode::NewMessage) {
        int senderId = std::stoi(packet.payload);
        m_unread.insert(senderId);
        m_contacts.Invalidate();
        RefreshChat();
        return;
    }

    if (packet.opcode == Opcode::Error) {
        m_incomingMessages.clear();
        UpdateContacts();
        AfxMessageBox(L"Request failed: " + FromUtf8(packet.payload));
        RequestFinished();
        return;
    }

    if (m_request == Request::Contacts) {
        if (packet.opcode == Opcode::ContactItem) {
            size_t first = packet.payload.find("|||");
            size_t second = packet.payload.find("|||", first + 3);
            ChatContact contact{};
            contact.id = std::stoi(packet.payload.substr(0, first));
            contact.username = packet.payload.substr(first + 3, second - first - 3);
            contact.avatar = packet.payload.substr(second + 3);
            m_incomingContacts.push_back(contact);
        }
        else if (packet.opcode == Opcode::ContactsEnd) {
            m_contactList = m_incomingContacts; // already sorted by the server
            UpdateContacts();
            if (!m_currentUsername.empty()) {
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
            m_incomingMessages.push_back(msg);
        }
        else if (packet.opcode == Opcode::MessagesEnd) {
            size_t pos = packet.payload.find("|||");
            int contactId = std::stoi(packet.payload.substr(0, pos));
            std::string avatar = packet.payload.substr(pos + 3);

            bool changed = m_loadingUsername != m_currentUsername;
            size_t firstIndex = changed ? 0 : m_messages.size();

            if (changed) {
                m_messages.clear();
                m_lastMessageId = 0;
            }

            m_messages.insert(
                m_messages.end(),
                m_incomingMessages.begin(),
                m_incomingMessages.end()
            );

            m_currentContactId = contactId;
            m_currentUsername = m_loadingUsername;
            m_chatAvatar.SetIcon(AvatarIcon(avatar));

            if (!m_messages.empty()) {
                m_lastMessageId = m_messages.back().id;
            }

            if (changed || !m_incomingMessages.empty()) {
                ShowMessages(firstIndex);
            }
            m_incomingMessages.clear();
            m_unread.erase(m_currentContactId);
            UpdateContacts();
            RequestFinished();
        }
    }
    else if (m_request == Request::Send) {
        if (packet.opcode == Opcode::MessageSent) {
            m_messageInput.SetWindowText(L"");
            m_refreshPending = true;
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

void CChatClientDlg::OnLvnItemchangedListContacts(NMHDR* hdr, LRESULT* result) {
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

void CChatClientDlg::OnNMCustomdrawListContacts(NMHDR* hdr, LRESULT* result) {
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

void CChatClientDlg::OnBnClickedBtnNewChat() {
    if ( m_closing || m_request != Request::None)
        return;
    CNewChatDlg dlg(this);

    INT_PTR result = dlg.DoModal();

    if (m_closing) return;

    if (result != IDOK) {
        RequestFinished();
        return;
    }

    std::string username = ToUtf8(dlg.Username());
    if (username == m_myUsername) {
        AfxMessageBox(L"Choose a different user.");
        RequestFinished();
        return;
    }
    GetMessages(username);
}

void CChatClientDlg::OnBnClickedBtnSend() {
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
        OnTcpDisconnected(0, 0);
        return;
    }

    m_request = Request::Send;
    UpdateControls();
}

void CChatClientDlg::AppendHistory(const CString& text, COLORREF color, WORD align, COLORREF background) {
    m_history.SetSel(-1, -1);
    PARAFORMAT pf{};
    pf.cbSize = sizeof(pf);
    pf.dwMask = PFM_ALIGNMENT;
    pf.wAlignment = align;
    m_history.SetParaFormat(pf);

    CHARFORMAT2 cf{};
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR | CFM_BACKCOLOR;
    cf.crTextColor = color;
    cf.crBackColor = background;
    m_history.SetSelectionCharFormat(cf);
    m_history.ReplaceSel(text);
}

void CChatClientDlg::ShowMessage(const ChatEntry& message) {
    bool mine = message.senderId == m_myUserId;
    CString sender = mine ? L"You" : FromUtf8(m_currentUsername);

    CTime time((time_t)message.sentAt);
    CString header = sender + L"  " + time.Format(L"%d/%m/%Y %H:%M");
    WORD alignment = mine ? PFA_RIGHT : PFA_LEFT;

    AppendHistory(header + L"\r\n", RGB(120, 120, 120), alignment);
    COLORREF background = mine ? RGB(220, 240, 255) : RGB(240, 240, 240);
    AppendHistory(
        FromUtf8(message.text) + L"\r\n\r\n",
        RGB(30, 30, 30),
        alignment,
        background
    );
    AppendHistory(L"\r\n\r\n", RGB(30, 30, 30), alignment);
}

void CChatClientDlg::ShowMessages(size_t firstIndex) {
    m_history.SetReadOnly(FALSE);

    if (firstIndex == 0) {
        m_history.SetWindowText(L"");
        SetDlgItemText(
            IDC_STATIC_CHAT_TITLE,
            L"Chat with: " + FromUtf8(m_currentUsername)
        );
    }

    for (size_t i = firstIndex; i < m_messages.size(); ++i)
        ShowMessage(m_messages[i]);

    m_history.SetSel(-1, -1);
    m_history.LineScroll(m_history.GetLineCount());
    m_history.SetReadOnly(TRUE);
}

HICON CChatClientDlg::AvatarIcon(const std::string& avatar) const {
    UINT resource = IDI_AVATAR_ROBOT;
    if (avatar == "dog") resource = IDI_AVATAR_DOG;
    else if (avatar == "cat") resource = IDI_AVATAR_CAT;
    else if (avatar == "leaf") resource = IDI_AVATAR_LEAF;
    else if (avatar == "sun") resource = IDI_AVATAR_SUN;
    return AfxGetApp()->LoadIcon(resource);
}

void CChatClientDlg::RefreshChat() {
    if (m_closing || m_myUserId <= 0) {
        return;
    }
    m_refreshPending = true;

    if (m_request != Request::None || !IsWindowEnabled()) {
        return;
    }

    m_refreshPending = false;
    GetContacts();
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
