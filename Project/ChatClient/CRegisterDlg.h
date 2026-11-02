#pragma once

#include "TcpClient.h"

class CRegisterDlg : public CDialogEx {
    DECLARE_DYNAMIC(CRegisterDlg)
public:
    CRegisterDlg(TcpClient& client, CWnd* parent = nullptr);
    virtual ~CRegisterDlg();
    void HandleResponse(const Packet& packet);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual void OnCancel();
    afx_msg void OnRegister();
    DECLARE_MESSAGE_MAP()

private:
    TcpClient& m_client;
    bool m_waiting = false;
};
