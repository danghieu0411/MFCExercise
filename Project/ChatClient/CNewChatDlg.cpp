#include "pch.h"
#include "ChatClient.h"
#include "CNewChatDlg.h"
#include "afxdialogex.h"

IMPLEMENT_DYNAMIC(CNewChatDlg, CDialogEx)

CNewChatDlg::CNewChatDlg(CWnd* parent)
    : CDialogEx(IDD_NEWCHAT_DIALOG, parent) {
}

CNewChatDlg::~CNewChatDlg() {}

void CNewChatDlg::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CNewChatDlg, CDialogEx)
END_MESSAGE_MAP()

void CNewChatDlg::OnOK() {
    GetDlgItemText(IDC_EDIT_NEWCHAT_USERNAME, m_username);
    m_username.Trim();
    if (m_username.IsEmpty()) {
        AfxMessageBox(L"Enter a username.");
        return;
    }
    CDialogEx::OnOK();
}
