
// CMCWeek3Dlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "CMCWeek3.h"
#include "CMCWeek3Dlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

const wchar_t* PEER_PIPE = L"\\\\.\\pipe\\CMCWeek3_Pipe";
const wchar_t* MY_PIPE = L"\\\\.\\pipe\\CMCWeek3_1_Pipe";

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


// CCMCWeek3Dlg dialog



CCMCWeek3Dlg::CCMCWeek3Dlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CMCWEEK3_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCMCWeek3Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_CHAT, m_chatList);
}

BEGIN_MESSAGE_MAP(CCMCWeek3Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_SEND_MSG, &CCMCWeek3Dlg::OnBnClickedButtonSendMsg)
	ON_WM_COPYDATA()
	ON_BN_CLICKED(IDC_BUTTON_START_SERVER, &CCMCWeek3Dlg::OnBnClickedButtonStartServer)
	ON_BN_CLICKED(IDC_BUTTON_SEND_PIPE, &CCMCWeek3Dlg::OnBnClickedButtonSendPipe)
END_MESSAGE_MAP()


// CCMCWeek3Dlg message handlers

BOOL CCMCWeek3Dlg::OnInitDialog()
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

	SetWindowText(L"CMCWeek3_1");

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CCMCWeek3Dlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CCMCWeek3Dlg::OnPaint()
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
HCURSOR CCMCWeek3Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CCMCWeek3Dlg::OnBnClickedButtonSendMsg()
{
	CString msg;
	GetDlgItemText(IDC_EDIT_MESSAGE, msg);

	if (msg.IsEmpty()) return;

	HWND hTarget = ::FindWindow(NULL, L"CMCWeek3");  // use :: to specify global win32, not the CWnd function
	if (hTarget == NULL) {
		AfxMessageBox(L"Cannot find the other app");
		return;
	}

	COPYDATASTRUCT cds{};
	cds.dwData = 1; // data type, assume 1 is 'chat app' here for convenience
	cds.cbData = (msg.GetLength() + 1) * sizeof(wchar_t); // size of the buffer
	cds.lpData = (PVOID)(LPCWSTR)msg; // pointer to the buffer

	::SendMessage(
		hTarget, // window receiving the message
		WM_COPYDATA,
		(WPARAM)GetSafeHwnd(), // handle to the window sending the message
		(LPARAM)&cds // pointer to the COPYDATASTRUCT
	);

	CString line = L"Me: " + msg;
	m_chatList.AddString(line);
	SetDlgItemText(IDC_EDIT_MESSAGE, L"");
}

BOOL CCMCWeek3Dlg::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct) {
	LPCWSTR received = (LPCWSTR)pCopyDataStruct->lpData;

	CString line = L"Peer: ";
	line += received;

	m_chatList.AddString(line);

	return TRUE;
}


void CCMCWeek3Dlg::OnBnClickedButtonStartServer()
{
	AfxBeginThread(PipeServerThread, this);
}

UINT CCMCWeek3Dlg::PipeServerThread(LPVOID pParam) {

	CCMCWeek3Dlg* pDlg = (CCMCWeek3Dlg*)pParam;

	HANDLE hPipe = CreateNamedPipe(
		MY_PIPE,
		PIPE_ACCESS_INBOUND,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
		1,
		1024,
		1024,
		0,
		NULL
	);

	if (hPipe == INVALID_HANDLE_VALUE) return 1;

	while (true) {

		BOOL connected = ConnectNamedPipe(hPipe, NULL);

		if (!connected && GetLastError() != ERROR_PIPE_CONNECTED) {
			CloseHandle(hPipe);
			return 1;
		}

		wchar_t buffer[512]{};
		DWORD byteRead = 0;

		if (ReadFile(
			hPipe,
			buffer,
			sizeof(buffer),
			&byteRead,
			NULL
		)) {
			COPYDATASTRUCT cds{};
			cds.dwData = 2;
			cds.cbData = byteRead;
			cds.lpData = buffer;

			::SendMessage(
				pDlg->GetSafeHwnd(),
				WM_COPYDATA,
				0,
				(LPARAM)&cds
			);
		}

		DisconnectNamedPipe(hPipe);
	}
	CloseHandle(hPipe);

	return 0;
}


void CCMCWeek3Dlg::OnBnClickedButtonSendPipe()
{
	CString msg;
	GetDlgItemText(IDC_EDIT_MESSAGE, msg);

	if (msg.IsEmpty()) return;

	HANDLE hPipe = CreateFile(
		PEER_PIPE,
		GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL
	);

	if (hPipe == INVALID_HANDLE_VALUE) {
		AfxMessageBox(L"Pipe Server is not running");
		return;
	}

	DWORD bytesWritten = 0;

	WriteFile(
		hPipe,
		(LPCWSTR)msg,
		(msg.GetLength() + 1) * sizeof(wchar_t),
		&bytesWritten,
		NULL
	);

	CloseHandle(hPipe);

	CString line = L"Me: " + msg;
	m_chatList.AddString(line);

	SetDlgItemText(IDC_EDIT_MESSAGE, L"");
}
