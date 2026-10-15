
// MFCApplication1Dlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "MFCApplication1.h"
#include "MFCApplication1Dlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMFCApplication1Dlg dialog



CMFCApplication1Dlg::CMFCApplication1Dlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MFCAPPLICATION1_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFCApplication1Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMFCApplication1Dlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_SAVE, &CMFCApplication1Dlg::OnBnClickedSave)
	ON_BN_CLICKED(IDC_BROWSE_FILE, &CMFCApplication1Dlg::OnBnClickedBrowseFile)
	ON_BN_CLICKED(IDC_SAVE_AS, &CMFCApplication1Dlg::OnBnClickedSaveAs)
	ON_BN_CLICKED(IDC_LOAD, &CMFCApplication1Dlg::OnBnClickedLoad)
END_MESSAGE_MAP()


// CMFCApplication1Dlg message handlers

BOOL CMFCApplication1Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMFCApplication1Dlg::OnPaint()
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
HCURSOR CMFCApplication1Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CMFCApplication1Dlg::SaveToFile(const CString& path) {
	CString content;
	GetDlgItemText(IDC_CONTENT, content);

	int size = (content.GetLength() + 1) * sizeof(wchar_t);

	HANDLE hFile = CreateFileW(
		path,
		GENERIC_WRITE, // open to write
		0, // share mode, 0 prevents subsequent open operation
		nullptr, // security attribute, too abstract for me now, but can be null to be default
		CREATE_ALWAYS, // create if not exist, overwrite if exist
		FILE_ATTRIBUTE_NORMAL, // normal file
		nullptr // don't understand, just let it null
	);

	if (hFile == INVALID_HANDLE_VALUE) {
		MessageBox(L"Cannot open file", L"Error", MB_OK);
		return;
	}

	DWORD bytesWritten;
	WriteFile(
		hFile, // handle to file to write
		content, // buffer holding the content to write
		size, // size of buffer
		&bytesWritten, // out, to hold number of bytes successfully written
		nullptr // don't know what is this
	);

	CloseHandle(hFile);
}

void CMFCApplication1Dlg::ReadFromFile(const CString& path) {
	HANDLE hFile = CreateFileW(
		path,
		GENERIC_READ,
		FILE_SHARE_READ, // any subsequent process can read, revisit OS course to understand...
		nullptr,
		OPEN_EXISTING, // open only if it exists
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);

	if (hFile == INVALID_HANDLE_VALUE) {
		MessageBox(L"Cannot open file to read", L"Error", MB_OK);
		return;
	}

	DWORD fileSize = GetFileSize(hFile, nullptr); // don't know what the second param is, can be null so let it be

	BYTE* buffer = new BYTE[fileSize];

	DWORD bytesRead;

	if (!ReadFile(
		hFile,
		buffer,
		fileSize,
		&bytesRead,
		nullptr
	)) {
		delete[] buffer;
		CloseHandle(hFile);
		MessageBox(L"Cannot read file", L"Error", MB_OK);
		return;
	}

	WCHAR* pszContent = (WCHAR*)buffer;
	SetDlgItemTextW(IDC_CONTENT, pszContent);

	delete[] buffer;
	CloseHandle(hFile);
}

void CMFCApplication1Dlg::OnBnClickedSave()
{
	CString path;
	GetDlgItemText(IDC_PATH, path);

	if (path.IsEmpty()) {
		AfxMessageBox(_T("Please specify a path first!"));
		return;
	}
	SaveToFile(path);
}


void CMFCApplication1Dlg::OnBnClickedBrowseFile()
{
	CFileDialog dlg(
		TRUE,
		nullptr,
		nullptr,
		OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
		_T("Text Files (*.txt)|*.txt|All Files (*.*)|*.*||")
	);

	if (dlg.DoModal() == IDOK)
	{
		CString path = dlg.GetPathName();

		SetDlgItemText(IDC_PATH, path);
	}
}


void CMFCApplication1Dlg::OnBnClickedSaveAs()
{
	CFileDialog dlg(
		FALSE,
		_T("txt"),
		nullptr,
		OFN_OVERWRITEPROMPT,
		_T("Text Files (*.txt)|*.txt|All Files (*.*)|*.*||")
	);

	if (dlg.DoModal() == IDOK) {
		CString path = dlg.GetPathName();
		SetDlgItemText(IDC_PATH, path);
		SaveToFile(path);
	}
}


void CMFCApplication1Dlg::OnBnClickedLoad()
{
	CString path;
	GetDlgItemText(IDC_PATH, path);

	if (path.IsEmpty())
	{
		AfxMessageBox(_T("Please specify a file first."));
		return;
	}

	ReadFromFile(path);
}
