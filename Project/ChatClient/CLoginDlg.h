#pragma once

#include "TcpClient.h"

class CRegisterDlg;

class CLoginDlg : public CDialogEx {
    DECLARE_DYNAMIC(CLoginDlg)
public:
    CLoginDlg(TcpClient& client, CWnd* parent = nullptr);
    virtual ~CLoginDlg();
    void HandleResponse(const Packet& packet);
    void CloseOnDisconnect();
    const std::string& Username() const { return m_username; }
    const std::string& Avatar() const { return m_avatar; }
    int UserId() const {
        return m_userId;
    }

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual void OnCancel();
    virtual void OnOK();
    afx_msg void OnBnClickedBtnLogin();
    afx_msg void OnBnClickedBtnRegister();
    DECLARE_MESSAGE_MAP()

private:
    TcpClient& m_client;
    CRegisterDlg* m_register = nullptr;
    std::string m_username;
    std::string m_avatar;
    bool m_waiting = false;
    int m_userId = -1;
};
