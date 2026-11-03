// CLoginDlg.cpp : implementation file
//

#include "pch.h"
#include "CMCWeek2.h"
#include "CLoginDlg.h"
#include "afxdialogex.h"
#include "CSignUpDlg.h"
#include "sqlite3.h"


// CLoginDlg dialog

IMPLEMENT_DYNAMIC(CLoginDlg, CDialogEx)

CLoginDlg::CLoginDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_LOGIN_DIALOG, pParent), m_userId(-1)
{

}

CLoginDlg::~CLoginDlg()
{
}

void CLoginDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_USERNAME, m_username);
	DDX_Text(pDX, IDC_PASSWORD, m_password);
}


BEGIN_MESSAGE_MAP(CLoginDlg, CDialogEx)
	ON_BN_CLICKED(IDCANCEL, &CLoginDlg::OnBnClickedCancel)
	ON_BN_CLICKED(ID_LOGIN, &CLoginDlg::OnBnClickedLogin)
	ON_BN_CLICKED(IDC_BUTTON1, &CLoginDlg::OnBnClickedButton1)
END_MESSAGE_MAP()


// CLoginDlg message handlers


void CLoginDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	CDialogEx::OnCancel();
}


void CLoginDlg::OnBnClickedLogin()
{
	if (!UpdateData(TRUE))
		return;
	m_username.Trim();
	if (m_username.IsEmpty() || m_password.IsEmpty()) {
		AfxMessageBox(L"Username/password cannot be empty");
		return;
	}

	if (theApp.m_database.VerifyUser(m_username, m_password, m_userId)) {
		EndDialog(IDOK);
	}
	else {
		MessageBox(L"Wrong username or password", L"Error", MB_OK);
	}
}


void CLoginDlg::OnBnClickedButton1()
{
	CSignUpDlg dlg;

	if (dlg.DoModal() == IDOK) {
		AfxMessageBox(L"Account created");
	}
}

int CLoginDlg::GetUserId() const {
	return m_userId;
}
