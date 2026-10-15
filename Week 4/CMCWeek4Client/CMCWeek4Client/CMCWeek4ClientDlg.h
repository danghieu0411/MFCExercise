#include <winsock2.h>
#include <ws2tcpip.h>

// CMCWeek4ClientDlg.h : header file
//

#pragma once


// CCMCWeek4ClientDlg dialog
class CCMCWeek4ClientDlg : public CDialogEx
{
// Construction
public:
	CCMCWeek4ClientDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CMCWEEK4CLIENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

private:
    WSADATA m_wsaData;
    SOCKET m_chatSocket = INVALID_SOCKET;
    HANDLE m_hReceiveThread = NULL;
    bool m_wsaStarted = false;

    bool ConnectToServer();
    static DWORD WINAPI ReceiveThreadProc(LPVOID lpParam);
    void AppendHistory(const CString& line);
    afx_msg LRESULT OnChatReceived(WPARAM wParam, LPARAM lParam);
    afx_msg void OnDestroy();

public:
    afx_msg void OnBnClickedSend();
};
