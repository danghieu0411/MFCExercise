
// CMCWeek2Dlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "CMCWeek2.h"
#include "CMCWeek2Dlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CCMCWeek2Dlg dialog



CCMCWeek2Dlg::CCMCWeek2Dlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CMCWEEK2_DIALOG, pParent), m_currentUserId(-1)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCMCWeek2Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_CONTACT_LIST, m_contactList);
	DDX_Control(pDX, IDC_CHAT_HISTORY, m_chatHistory);
	DDX_Control(pDX, IDC_MESSAGE_INPUT, m_messageInput);
}

BEGIN_MESSAGE_MAP(CCMCWeek2Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_CONTACT_LIST, &CCMCWeek2Dlg::OnLvnItemchangedContactList)
	ON_BN_CLICKED(IDC_SEND, &CCMCWeek2Dlg::OnBnClickedSend)
END_MESSAGE_MAP()


// CCMCWeek2Dlg message handlers

BOOL CCMCWeek2Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	m_chatHistory.InsertColumn(
		0,
		L"Contact",
		LVCFMT_LEFT,
		350
	);

	m_chatHistory.InsertColumn(
		1,
		L"Time",
		LVCFMT_CENTER,
		140
	);

	m_chatHistory.InsertColumn(
		2,
		L"Me",
		LVCFMT_RIGHT,
		350
	);

	m_chatHistory.SetExtendedStyle(
		m_chatHistory.GetExtendedStyle() | // include current extended styles
		LVS_EX_FULLROWSELECT // when click a row, highlight the whole row, not only the chosen column
	);

	m_contactList.ModifyStyle(0, LVS_SINGLESEL);
	m_contactList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	m_chatHistory.SetExtendedStyle(m_chatHistory.GetExtendedStyle() | LVS_EX_DOUBLEBUFFER);
	CRect historyRect;
	m_chatHistory.GetClientRect(&historyRect);
	int historyWidth = historyRect.Width() - GetSystemMetrics(SM_CXVSCROLL) - 4;
	m_chatHistory.SetColumnWidth(0, historyWidth * 40 / 100);
	m_chatHistory.SetColumnWidth(1, historyWidth * 20 / 100);
	m_chatHistory.SetColumnWidth(2, historyWidth * 40 / 100);
	SetWindowText(L"CMCWeek2 - Local chat");
	GetDlgItem(IDC_SEND)->EnableWindow(FALSE);

	m_contactList.InsertColumn(
		0, // column's index
		L"Contacts", // column header
		LVCFMT_LEFT, // left-aligned
		150 // width, in pixels
	);


	if (!RefreshContacts())
		AfxMessageBox(L"Cannot load contacts.");
	SetTimer(1, 1000, nullptr);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CCMCWeek2Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CCMCWeek2Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CCMCWeek2Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CCMCWeek2Dlg::OnLvnItemchangedContactList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	if (!(pNMLV->uChanged & LVIF_STATE) ||
		!(pNMLV->uNewState & LVIS_SELECTED) || pNMLV->iItem < 0)
		return;

	int contactId = static_cast<int>(m_contactList.GetItemData(pNMLV->iItem));
	if (contactId == m_selectedContactId)
		return;
	m_selectedContactId = contactId;
	m_lastMessageId = 0;
	m_chatHistory.DeleteAllItems();
	GetDlgItem(IDC_SEND)->EnableWindow(TRUE);
	if (!LoadChatHistory())
		AfxMessageBox(L"Cannot load messages.");
}

void CCMCWeek2Dlg::OnBnClickedSend()
{
	if (m_selectedContactId == -1) {
		AfxMessageBox(L"Select a contact first.");
		return;
	}
	CString content;
	m_messageInput.GetWindowText(content);
	CString trimmed(content);
	trimmed.Trim();
	if (trimmed.IsEmpty())
		return;

	if (!theApp.m_database.InsertMessage(m_currentUserId, m_selectedContactId,
		content, CTime::GetCurrentTime().GetTime())) {
		AfxMessageBox(L"Cannot send message. Please try again.");
		return;
	}
	m_messageInput.SetWindowText(L"");
	if (!LoadChatHistory())
		AfxMessageBox(L"Message saved, but cannot refresh history.");
	m_messageInput.SetFocus();
}

void CCMCWeek2Dlg::OnOK()
{
	OnBnClickedSend();
}

bool CCMCWeek2Dlg::RefreshContacts()
{
	std::vector<Contact> contacts;
	if (!theApp.m_database.LoadContact(m_currentUserId, contacts))
		return false;
	bool changed = contacts.size() != m_contacts.size();
	for (size_t i = 0; !changed && i < contacts.size(); ++i)
		changed = contacts[i].id != m_contacts[i].id || contacts[i].name != m_contacts[i].name;
	if (!changed)
		return true;

	m_contacts = contacts;
	m_contactList.DeleteAllItems();
	int selectedRow = -1;
	for (size_t i = 0; i < m_contacts.size(); ++i) {
		int row = m_contactList.InsertItem(static_cast<int>(i), m_contacts[i].name);
		m_contactList.SetItemData(row, m_contacts[i].id);
		if (m_contacts[i].id == m_selectedContactId)
			selectedRow = row;
	}
	if (selectedRow == -1) {
		m_selectedContactId = -1;
		m_lastMessageId = 0;
		m_chatHistory.DeleteAllItems();
		GetDlgItem(IDC_SEND)->EnableWindow(FALSE);
		if (!m_contacts.empty())
			selectedRow = 0;
	}
	if (selectedRow != -1)
		m_contactList.SetItemState(selectedRow, LVIS_SELECTED | LVIS_FOCUSED,
			LVIS_SELECTED | LVIS_FOCUSED);
	return true;
}

bool CCMCWeek2Dlg::LoadChatHistory()
{
	if (m_selectedContactId == -1)
		return true;
	std::vector<Message> messages;
	if (!theApp.m_database.LoadMessages(m_currentUserId, m_selectedContactId,
		m_lastMessageId, messages))
		return false;
	for (const Message& message : messages) {
		int row = m_chatHistory.InsertItem(m_chatHistory.GetItemCount(), L"");
		m_chatHistory.SetItemText(row, message.senderId == m_currentUserId ? 2 : 0,
			message.content);
		m_chatHistory.SetItemText(row, 1, CTime(message.timeStamp).Format(L"%d/%m %H:%M"));
		m_lastMessageId = message.id;
	}
	if (!messages.empty())
		m_chatHistory.EnsureVisible(m_chatHistory.GetItemCount() - 1, FALSE);
	return true;
}

void CCMCWeek2Dlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 1) {
		RefreshContacts();
		LoadChatHistory();
	}
	CDialogEx::OnTimer(nIDEvent);
}

void CCMCWeek2Dlg::OnDestroy()
{
	KillTimer(1);
	CDialogEx::OnDestroy();
}

void CCMCWeek2Dlg::SetCurrentUserId(int userId) {
	m_currentUserId = userId;
}