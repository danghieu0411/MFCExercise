#include <winsock2.h>
#include <ws2tcpip.h>
// CMCWeek4Dlg.h : header file
//

#pragma once


// CCMCWeek4Dlg dialog
class CCMCWeek4Dlg : public CDialogEx
{
// Construction
public:
	CCMCWeek4Dlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CMCWEEK4_DIALOG };
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
	SOCKET m_listenSocket = INVALID_SOCKET;
	SOCKET m_chatSocket = INVALID_SOCKET;
	afx_msg void OnDestroy();
	HANDLE m_hServerThread = NULL;
	volatile bool m_stopping = false;
	bool m_wsaStarted = false;

	bool StartServer();
	static DWORD WINAPI ServerThreadProc(LPVOID lpParam);
	afx_msg LRESULT OnChatReceived(WPARAM wParam, LPARAM lParam);
	void AppendHistory(const CString& line);

	bool IsAutorunEnable();
	bool SetAutorun(bool enable);
public:
	afx_msg void OnBnClickedSend();
	afx_msg void OnBnClickedCheckAutorun();
};
