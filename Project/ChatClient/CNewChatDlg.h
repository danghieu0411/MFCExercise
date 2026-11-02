#pragma once
#include <string>

class CNewChatDlg : public CDialogEx {
    DECLARE_DYNAMIC(CNewChatDlg)
public:
    CNewChatDlg(CWnd* parent = nullptr);
    virtual ~CNewChatDlg();
    const CString& Username() const { return m_username; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual void OnOK();
    DECLARE_MESSAGE_MAP()

private:
    CString m_username;
};
