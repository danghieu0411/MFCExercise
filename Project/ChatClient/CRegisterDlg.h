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
    virtual void OnOK();
    afx_msg void OnBnClickedBtnRegisterSubmit();
    DECLARE_MESSAGE_MAP()

private:
    TcpClient& m_client;
    bool m_waiting = false;

    CComboBox m_avatarCombo;
    CStatic m_avatarPreview;
public:
    virtual BOOL OnInitDialog();
    afx_msg void OnCbnSelchangeComboAvatar();
};
