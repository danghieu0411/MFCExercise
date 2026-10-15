
// CMCWeek4ClientDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "CMCWeek4Client.h"
#include "CMCWeek4ClientDlg.h"
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

const int PORT = 8888;
const char* SERVER_IP = "192.168.234.1";
const UINT WM_CHAT_RECEIVED = WM_APP + 1;

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


// CCMCWeek4ClientDlg dialog



CCMCWeek4ClientDlg::CCMCWeek4ClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CMCWEEK4CLIENT_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCMCWeek4ClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CCMCWeek4ClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_SEND, &CCMCWeek4ClientDlg::OnBnClickedSend)
	ON_MESSAGE(WM_CHAT_RECEIVED, &CCMCWeek4ClientDlg::OnChatReceived)
END_MESSAGE_MAP()


// CCMCWeek4ClientDlg message handlers

BOOL CCMCWeek4ClientDlg::OnInitDialog()
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
    int result = WSAStartup(MAKEWORD(2, 2), &m_wsaData);
    if (result != 0) {
        MessageBox(L"Cannot initialize Winsock!", L"Error");
        EndDialog(IDCANCEL);
        return TRUE;
    }
    m_wsaStarted = true;

    if (!ConnectToServer()) {
        EndDialog(IDCANCEL);
        return TRUE;
    }

    AppendHistory(L"Connected to server.\r\n");
    m_hReceiveThread = CreateThread(NULL, 0, ReceiveThreadProc, this, 0, NULL);
    if (m_hReceiveThread == NULL) {
        MessageBox(L"Failed to create receive thread!", L"Error");
        EndDialog(IDCANCEL);
        return TRUE;
    }

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CCMCWeek4ClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CCMCWeek4ClientDlg::OnPaint()
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
HCURSOR CCMCWeek4ClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CCMCWeek4ClientDlg::OnBnClickedSend()
{
    // TODO: Add your control notification handler code here
    if (m_chatSocket == INVALID_SOCKET) {
        MessageBox(L"Not connected to server!", L"Info");
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

bool CCMCWeek4ClientDlg::ConnectToServer()
{
    m_chatSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_chatSocket == INVALID_SOCKET) {
        MessageBox(L"Cannot create client socket!", L"Error");
        return false;
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr) != 1) {
        MessageBox(L"Invalid server IP!", L"Error");
        return false;
    }

    if (connect(m_chatSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        CString message;
        message.Format(L"Connect failed! Error: %d", WSAGetLastError());
        MessageBox(message, L"Error");
        return false;
    }
    return true;
}

DWORD WINAPI CCMCWeek4ClientDlg::ReceiveThreadProc(LPVOID lpParam)
{
    CCMCWeek4ClientDlg* pDlg = (CCMCWeek4ClientDlg*)lpParam;

    while (true) {
        int size = 0;
        if (!ReceiveAll(pDlg->m_chatSocket, (char*)&size, sizeof(size))) break;
        if (size <= 0 || size > 1024 * 1024 || size % sizeof(wchar_t) != 0) break;

        int count = size / sizeof(wchar_t);
        wchar_t* buffer = new wchar_t[count + 1];
        if (!ReceiveAll(pDlg->m_chatSocket, (char*)buffer, size)) {
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
    return 0;
}

void CCMCWeek4ClientDlg::AppendHistory(const CString& line)
{
    CString history;
    GetDlgItemText(IDC_CHAT_HISTORY, history);
    history += line + L"\r\n";
    SetDlgItemText(IDC_CHAT_HISTORY, history);
}

LRESULT CCMCWeek4ClientDlg::OnChatReceived(WPARAM wParam, LPARAM lParam)
{
    CString* message = (CString*)lParam;
    AppendHistory(*message);
    delete message;
    return 0;
}

void CCMCWeek4ClientDlg::OnDestroy()
{
    if (m_chatSocket != INVALID_SOCKET)
        shutdown(m_chatSocket, SD_BOTH);

    if (m_hReceiveThread != NULL) {
        WaitForSingleObject(m_hReceiveThread, INFINITE);
        CloseHandle(m_hReceiveThread);
        m_hReceiveThread = NULL;
    }

    if (m_chatSocket != INVALID_SOCKET) {
        closesocket(m_chatSocket);
        m_chatSocket = INVALID_SOCKET;
    }

    if (m_wsaStarted)
        WSACleanup();

    CDialogEx::OnDestroy();
}
