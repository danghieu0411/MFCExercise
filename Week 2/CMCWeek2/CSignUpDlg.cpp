// CSignUpDlg.cpp : implementation file
//

#include "pch.h"
#include "CMCWeek2.h"
#include "CSignUpDlg.h"
#include "afxdialogex.h"


// CSignUpDlg dialog

IMPLEMENT_DYNAMIC(CSignUpDlg, CDialogEx)

CSignUpDlg::CSignUpDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SIGNUP_DIALOG, pParent)
{

}

CSignUpDlg::~CSignUpDlg()
{
}

void CSignUpDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSignUpDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CSignUpDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CSignUpDlg message handlers


void CSignUpDlg::OnBnClickedOk()
{
	CString username;
	CString password;
	CString confirmPassword;

	GetDlgItemText(IDC_USERNAME, username);
	GetDlgItemText(IDC_PASSWORD, password);
	GetDlgItemText(IDC_CONFIRM_PASSWORD, confirmPassword);

	username.Trim();
	if (username.IsEmpty() || password.IsEmpty()) {
		AfxMessageBox(L"Username/password cannot be empty");
		return;
	}

	if (password != confirmPassword) {
		AfxMessageBox(L"Password not match");
		return;
	}

	if (!theApp.m_database.RegisterUser(username, password)) {
		AfxMessageBox(L"Cannot create account. The username may already exist.");
		return;
	}

	CDialogEx::OnOK();
}
