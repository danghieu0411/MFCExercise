
// CMCWeek3Dlg.h : header file
//

#pragma once


// CCMCWeek3Dlg dialog
class CCMCWeek3Dlg : public CDialogEx
{
// Construction
public:
	CCMCWeek3Dlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CMCWEEK3_DIALOG };
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

public:
	CListBox m_chatList;
	afx_msg void OnBnClickedButtonSendMsg();
	afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
	static DWORD WINAPI PipeServerThread(LPVOID pParam);  // controlling function
	afx_msg void OnBnClickedButtonStartServer();
	afx_msg void OnBnClickedButtonSendPipe();
};
