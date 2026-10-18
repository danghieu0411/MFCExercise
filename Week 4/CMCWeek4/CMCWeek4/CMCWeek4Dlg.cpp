
// CMCWeek4Dlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "CMCWeek4.h"
#include "CMCWeek4Dlg.h"
#include "afxdialogex.h"

#pragma comment(lib, "Ws2_32.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static bool SendAll(SOCKET s, const char* data, int size)
{
	int total = 0;
	while (total < size) {
		int n = send(s, data + total, size - total, 0);
		if (n <= 0) return false;
		total += n;
	}
	return true;
}

static bool ReceiveAll(SOCKET s, char* data, int size)
{
	int total = 0;
	while (total < size) {
		int n = recv(s, data + total, size - total, 0);
		if (n <= 0) return false;
		total += n;
	}
	return true;
}

namespace {
	const wchar_t RUN_KEY[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
	const wchar_t VALUE[] = L"ChatAppServer";

	const int PORT = 8888;
	const UINT WM_CHAT_RECEIVED = WM_APP + 1;  // WM_APP marks the beginning of user-defined message value, values prior to it are reserved for system message
}
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


// CCMCWeek4Dlg dialog



CCMCWeek4Dlg::CCMCWeek4Dlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CMCWEEK4_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCMCWeek4Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CCMCWeek4Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_SEND, &CCMCWeek4Dlg::OnBnClickedSend)
	ON_MESSAGE(WM_CHAT_RECEIVED, &CCMCWeek4Dlg::OnChatReceived)
	ON_BN_CLICKED(IDC_CHECK_AUTORUN, &CCMCWeek4Dlg::OnBnClickedCheckAutorun)
END_MESSAGE_MAP()


// CCMCWeek4Dlg message handlers

BOOL CCMCWeek4Dlg::OnInitDialog()
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

	CheckDlgButton(
		IDC_CHECK_AUTORUN,
		IsAutorunEnable() ? BST_CHECKED : BST_UNCHECKED
	);

	int result = WSAStartup(MAKEWORD(2, 2), &m_wsaData);
	if (result != 0) {
		MessageBox(L"Cannot initialize Winsock!", L"Error");
		EndDialog(IDCANCEL);
		return TRUE;
	}
	m_wsaStarted = true;

	if (!StartServer()) {
		EndDialog(IDCANCEL);
		return TRUE;
	}

	m_hServerThread = CreateThread(NULL, 0, ServerThreadProc, this, 0, NULL);
	if (m_hServerThread == NULL) {
		MessageBox(L"Failed to create server thread!", L"Error");
		EndDialog(IDCANCEL);
		return TRUE;
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CCMCWeek4Dlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CCMCWeek4Dlg::OnPaint()
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
HCURSOR CCMCWeek4Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CCMCWeek4Dlg::OnDestroy()
{
	m_stopping = true; // tell the thread that UI is going to close so it stops working
	if (m_chatSocket != INVALID_SOCKET)
		shutdown(m_chatSocket, SD_BOTH);  // disable send - receive on this socket, and wake recv() up if it is blocking

	if (m_hServerThread != NULL) {
		WaitForSingleObject(m_hServerThread, INFINITE);
		CloseHandle(m_hServerThread);
		m_hServerThread = NULL;
	}

	if (m_listenSocket != INVALID_SOCKET) {
		closesocket(m_listenSocket);
		m_listenSocket = INVALID_SOCKET;
	}

	if (m_wsaStarted)
		WSACleanup();

	CDialogEx::OnDestroy();
}

bool CCMCWeek4Dlg::StartServer() {

	m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (m_listenSocket == INVALID_SOCKET) {
		MessageBox(L"Cannot create socket", L"Error");
		return false;
	}

	sockaddr_in serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(8888);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(m_listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		
		MessageBox(L"Bind socket failed", L"Error");
		closesocket(m_listenSocket);
		m_listenSocket = INVALID_SOCKET;
		return false;
	}

	if (listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR) {
		MessageBox(L"Listen failed.", L"Error");
		closesocket(m_listenSocket);
		m_listenSocket = INVALID_SOCKET;
		return false;
	}
	return true;
}

DWORD WINAPI CCMCWeek4Dlg::ServerThreadProc(LPVOID lpParam)
{
	CCMCWeek4Dlg* pDlg = (CCMCWeek4Dlg*)lpParam;

	while (!pDlg->m_stopping) { // work when m_stopping is still false
		fd_set readSet;  // set of sockets to check
		FD_ZERO(&readSet); // clear the set
		FD_SET(pDlg->m_listenSocket, &readSet); // add our listening socket to the set
		timeval timeout = { 0, 200000 }; // waiting time
		int ready = select(0, &readSet, NULL, NULL, &timeout); // select() check the socket status
		if (ready == SOCKET_ERROR) return 1;
		if (ready == 0) continue; // no client connected yet, re-check m_stopping
		break; // positive value means there are connection waiting to be accepted
	}
	if (pDlg->m_stopping) return 0;

	SOCKET clientSocket = accept(pDlg->m_listenSocket, NULL, NULL);
	if (clientSocket == INVALID_SOCKET) return 1;
	if (pDlg->m_stopping) {
		closesocket(clientSocket);
		return 0;
	}

	pDlg->m_chatSocket = clientSocket;
	CString* connected = new CString(L"Client connected.");
	if (!pDlg->PostMessage(WM_CHAT_RECEIVED, 0, (LPARAM)connected))
		delete connected;

	while (!pDlg->m_stopping) {
		int size = 0;
		if (!ReceiveAll(clientSocket, (char*)&size, sizeof(size))) break;
		if (size <= 0 || size > 1024 * 1024 || size % sizeof(wchar_t) != 0) break;

		int count = size / sizeof(wchar_t);
		wchar_t* buffer = new wchar_t[count + 1];
		if (!ReceiveAll(clientSocket, (char*)buffer, size)) {
			delete[] buffer;
			break;
		}
		buffer[count] = L'\0';

		CString* message = new CString(L"Peer: ");
		*message += buffer;
		delete[] buffer;
		if (!pDlg->PostMessage(WM_CHAT_RECEIVED, 0, (LPARAM)message))
			delete message;
	}

	pDlg->m_chatSocket = INVALID_SOCKET;
	closesocket(clientSocket);
	return 0;
}


void CCMCWeek4Dlg::OnBnClickedSend()
{
	if (m_chatSocket == INVALID_SOCKET) {
		MessageBox(L"No client connected!", L"Info");
		return;
	}

	CString text;
	GetDlgItemText(IDC_MESSAGE_INPUT, text);
	if (text.IsEmpty()) return;

	int size = text.GetLength() * sizeof(wchar_t);
	if (size > 1024 * 1024) {
		MessageBox(L"Message too long!", L"Error");
		return;
	}
	if (!SendAll(m_chatSocket, (char*)&size, sizeof(size)) ||
		!SendAll(m_chatSocket, (const char*)text.GetString(), size)) {
		MessageBox(L"Send failed!", L"Error");
		return;
	}

	AppendHistory(L"Me: " + text);
	SetDlgItemText(IDC_MESSAGE_INPUT, L"");
}

void CCMCWeek4Dlg::AppendHistory(const CString& line)
{
	CString history;
	GetDlgItemText(IDC_CHAT_HISTORY, history);
	history += line + L"\r\n";
	SetDlgItemText(IDC_CHAT_HISTORY, history);
}

LRESULT CCMCWeek4Dlg::OnChatReceived(WPARAM wParam, LPARAM lParam)
{
	CString* message = (CString*)lParam;
	AppendHistory(*message);
	delete message;
	return 0;
}

bool CCMCWeek4Dlg::IsAutorunEnable() {
	HKEY hKey = nullptr;

	LONG result = RegOpenKeyExW(
		HKEY_CURRENT_USER, // qurey HKCU
		RUN_KEY, // autorun
		0,
		KEY_QUERY_VALUE, // set this when we only query value
		&hKey // pointer to the handle
	);
	if (result != ERROR_SUCCESS) return false;

	wchar_t registeredPath[32768] = {};
	DWORD type = 0;
	DWORD size = sizeof(registeredPath);

	result = RegQueryValueExW(
		hKey, // key handle
		VALUE, // value to query
		nullptr, // reserved
		&type, // pointer to a variable to get the type of data stored in value
		(BYTE*)registeredPath, // pointer to a buffer to hold tde queried data
		&size // pointer to a variable holding the buffer size
	);

	RegCloseKey(hKey);

	if (result != ERROR_SUCCESS || type != REG_SZ) {
		return false;
	}

	if (size < sizeof(wchar_t) || size>sizeof(registeredPath) || size % sizeof(wchar_t) != 0) return false;

	registeredPath[size / sizeof(wchar_t) - 1] = L'\0';

	wchar_t exePath[32768] = {};
	DWORD length = GetModuleFileNameW(
		nullptr, // null denotes that we want to specify path of current process
		exePath, // buffer to receive the path
		_countof(exePath) // size of the buffer in TCHAR, that is why i use _countof() to get number of elements, not sizeof()
	);
	if (length == 0 || length >= _countof(exePath)) return false;

	CString expected;
	expected.Format(L"\"%s\"", exePath);

	return expected.CompareNoCase(registeredPath)==0;
}

bool CCMCWeek4Dlg::SetAutorun(bool enable) {
	HKEY hKey = nullptr;

	LONG result = RegCreateKeyExW( // create if not exist, open if exist
		HKEY_CURRENT_USER,
		RUN_KEY,
		0,
		nullptr,
		REG_OPTION_NON_VOLATILE, // value remains as hive unloads
		KEY_SET_VALUE,
		nullptr,
		&hKey,
		nullptr
	);

	if (result != ERROR_SUCCESS) return false;

	if (enable) {
		wchar_t exePath[32768] = {};

		DWORD length = GetModuleFileNameW(nullptr, exePath, _countof(exePath));
		if (length == 0 || length >= _countof(exePath)) {
			RegCloseKey(hKey);
			return false;
		}

		CString path;
		path.Format(L"\"%s\"", exePath);

		result = RegSetValueExW(
			hKey,
			VALUE,
			0,
			REG_SZ,
			(const BYTE*)(LPCWSTR)path,
			(DWORD)(path.GetLength() + 1) * sizeof(wchar_t)
		);
	}
	else {
		result = RegDeleteValueW(hKey, VALUE);
		if (result == ERROR_FILE_NOT_FOUND) {
			result = ERROR_SUCCESS;
		}
	}

	RegCloseKey(hKey);
	return result == ERROR_SUCCESS;
}


void CCMCWeek4Dlg::OnBnClickedCheckAutorun()
{
	bool enable = IsDlgButtonChecked(IDC_CHECK_AUTORUN) == BST_CHECKED;
	if (!SetAutorun(enable)) {
		MessageBox(L"Fail to update autorun settings", L"Error");
		CheckDlgButton(
			IDC_CHECK_AUTORUN,
			IsAutorunEnable() ? BST_CHECKED : BST_UNCHECKED
		);
	}
}
