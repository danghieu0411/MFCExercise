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

    DDX_Control(pDX, IDC_COMBO_AVATAR, m_avatarCombo);
    DDX_Control(pDX, IDC_PIC_AVATAR, m_avatarPreview);
}

BEGIN_MESSAGE_MAP(CRegisterDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_REGISTER_SUBMIT, &CRegisterDlg::OnBnClickedBtnRegisterSubmit)
    ON_CBN_SELCHANGE(IDC_COMBO_AVATAR, &CRegisterDlg::OnCbnSelchangeComboAvatar)
END_MESSAGE_MAP()

void CRegisterDlg::OnOK() {
    OnBnClickedBtnRegisterSubmit();
}

void CRegisterDlg::OnBnClickedBtnRegisterSubmit() {
    if (m_waiting) return;
    CString username, password, confirm;
    GetDlgItemText(IDC_EDIT_REG_USERNAME, username);
    GetDlgItemText(IDC_EDIT_REG_PASSWORD, password);
    GetDlgItemText(IDC_EDIT_REG_CONFIRM, confirm);
    username.Trim();

    if (username.IsEmpty() || password.IsEmpty()) {
        AfxMessageBox(L"Enter username and password.");
        return;
    }
    if (username.Find(L'|') >= 0) {
        AfxMessageBox(L"Username cannot contain '|'.");
        return;
    }
    if (password != confirm) {
        AfxMessageBox(L"Passwords do not match.");
        return;
    }

    const char* avatars[] = {
        "", "dog", "cat", "robot", "leaf", "sun"
    };
    std::string avatar = avatars[m_avatarCombo.GetCurSel()];
    std::string payload = ToUtf8(username) + "|||" +
        avatar + "|||" + ToUtf8(password);
    if (!m_client.Send({ Opcode::Register, payload })) {
        AfxMessageBox(L"Cannot send registration request.");
        return;
    }

    m_waiting = true;
    m_avatarCombo.EnableWindow(FALSE);
    GetDlgItem(IDC_BTN_REGISTER_SUBMIT)->EnableWindow(FALSE);
    GetDlgItem(IDCANCEL)->EnableWindow(FALSE);
}

void CRegisterDlg::HandleResponse(const Packet& packet) {
    if (!m_waiting)
        return;

    m_waiting = false;
    m_avatarCombo.EnableWindow(TRUE);
    GetDlgItem(IDC_BTN_REGISTER_SUBMIT)->EnableWindow(TRUE);
    GetDlgItem(IDCANCEL)->EnableWindow(TRUE);

    if (packet.opcode == Opcode::RegisterOk) {
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


BOOL CRegisterDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // TODO:  Add extra initialization here

    m_avatarCombo.SetCurSel(0);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


void CRegisterDlg::OnCbnSelchangeComboAvatar()
{
    const UINT icons[] = {
        IDI_AVATAR_ROBOT,
        IDI_AVATAR_DOG,
        IDI_AVATAR_CAT,
        IDI_AVATAR_ROBOT,
        IDI_AVATAR_LEAF,
        IDI_AVATAR_SUN
    };

    int selected = m_avatarCombo.GetCurSel();
    HICON icon = AfxGetApp()->LoadIcon(icons[selected]);
    m_avatarPreview.SetIcon(icon);
}
