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

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual void OnCancel();
    afx_msg void OnLogin();
    afx_msg void OnRegister();
    DECLARE_MESSAGE_MAP()

private:
    TcpClient& m_client;
    CRegisterDlg* m_register = nullptr;
    std::string m_attemptUsername;
    std::string m_username;
    bool m_waiting = false;
};
