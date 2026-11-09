#include "pch.h"
#include "ChatClient.h"
#include "CLoginDlg.h"
#include "CRegisterDlg.h"
#include "TextUtil.h"
#include "afxdialogex.h"

IMPLEMENT_DYNAMIC(CLoginDlg, CDialogEx)

CLoginDlg::CLoginDlg(TcpClient& client, CWnd* parent)
    : CDialogEx(IDD_LOGIN_DIALOG, parent), m_client(client) {
}

CLoginDlg::~CLoginDlg() {}

void CLoginDlg::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CLoginDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_LOGIN, &CLoginDlg::OnBnClickedBtnLogin)
    ON_BN_CLICKED(IDC_BTN_REGISTER, &CLoginDlg::OnBnClickedBtnRegister)
END_MESSAGE_MAP()

void CLoginDlg::OnOK() {
    OnBnClickedBtnLogin();
}

void CLoginDlg::OnBnClickedBtnLogin() {
    if (m_waiting) return;
    CString username, password;
    GetDlgItemText(IDC_EDIT_USERNAME, username);
    GetDlgItemText(IDC_EDIT_PASSWORD, password);
    username.Trim();

    if (username.IsEmpty() || password.IsEmpty()) {
        AfxMessageBox(L"Enter username and password.");
        return;
    }

    if (username.Find(L'|') >= 0) {
        AfxMessageBox(L"Username cannot contain '|'.");
        return;
    }

    std::string name = ToUtf8(username);
    if (!m_client.Send({ Opcode::Login, name + "|||" + ToUtf8(password) })) {
        AfxMessageBox(L"Cannot send login request.");
        return;
    }

    m_username = name;
    m_waiting = true;

    // temporarily disable these buttons, prevent user from repeatedly press login
    GetDlgItem(IDC_BTN_LOGIN)->EnableWindow(FALSE);
    GetDlgItem(IDC_BTN_REGISTER)->EnableWindow(FALSE);
}

void CLoginDlg::OnBnClickedBtnRegister() {
    CRegisterDlg dlg(m_client, this);
    m_register = &dlg;
    dlg.DoModal();
    m_register = nullptr;
}

void CLoginDlg::HandleResponse(const Packet& packet) {
    if (m_register) {
        m_register->HandleResponse(packet);
        return;
    }
    if (!m_waiting)
        return;

    if (packet.opcode != Opcode::LoginOk && packet.opcode != Opcode::Error) {
        return;
    }

    m_waiting = false;
    GetDlgItem(IDC_BTN_LOGIN)->EnableWindow(TRUE);
    GetDlgItem(IDC_BTN_REGISTER)->EnableWindow(TRUE);

    if (packet.opcode == Opcode::Error) {
        AfxMessageBox(L"Login failed: " + FromUtf8(packet.payload));
        return;
    }

    size_t pos = packet.payload.find("|||");
    m_userId = std::stoi(packet.payload.substr(0, pos));
    m_avatar = packet.payload.substr(pos + 3);
    EndDialog(IDOK);
}

void CLoginDlg::CloseOnDisconnect() {
    if (m_register)
        m_register->EndDialog(IDCANCEL);
    EndDialog(IDCANCEL);
}

void CLoginDlg::OnCancel() {
    if (!m_waiting)
        CDialogEx::OnCancel();
}
