#include "pch.h"
#include "ChatClient.h"
#include "CRegisterDlg.h"
#include "TextUtil.h"
#include "afxdialogex.h"

IMPLEMENT_DYNAMIC(CRegisterDlg, CDialogEx)

CRegisterDlg::CRegisterDlg(TcpClient& client, CWnd* parent)
    : CDialogEx(IDD_REGISTER_DIALOG, parent), m_client(client) {
}

CRegisterDlg::~CRegisterDlg() {}

void CRegisterDlg::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CRegisterDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_REGISTER_SUBMIT, &CRegisterDlg::OnRegister)
END_MESSAGE_MAP()

void CRegisterDlg::OnRegister() {
    CString username, password, confirm;
    GetDlgItemText(IDC_EDIT_REG_USERNAME, username);
    GetDlgItemText(IDC_EDIT_REG_PASSWORD, password);
    GetDlgItemText(IDC_EDIT_REG_CONFIRM, confirm);

    if (username.IsEmpty() || password.IsEmpty()) {
        AfxMessageBox(L"Enter username and password.");
        return;
    }
    if (password != confirm) {
        AfxMessageBox(L"Passwords do not match.");
        return;
    }

    std::string payload = ToUtf8(username) + "|||" + ToUtf8(password);
    if (!m_client.Send({ Opcode::Register, payload })) {
        AfxMessageBox(L"Cannot send registration request.");
        return;
    }

    m_waiting = true;
    GetDlgItem(IDC_BTN_REGISTER_SUBMIT)->EnableWindow(FALSE);
    GetDlgItem(IDCANCEL)->EnableWindow(FALSE);
}

void CRegisterDlg::HandleResponse(const Packet& packet) {
    if (!m_waiting)
        return;

    m_waiting = false;
    GetDlgItem(IDC_BTN_REGISTER_SUBMIT)->EnableWindow(TRUE);
    GetDlgItem(IDCANCEL)->EnableWindow(TRUE);

    if (packet.opcode == Opcode::Success) {
        AfxMessageBox(L"Account created. Please log in.");
        EndDialog(IDOK);
    }
    else if (packet.opcode == Opcode::Error) {
        AfxMessageBox(L"Registration failed: " + FromUtf8(packet.payload));
    }
}

void CRegisterDlg::OnCancel() {
    if (!m_waiting)
        CDialogEx::OnCancel();
}
