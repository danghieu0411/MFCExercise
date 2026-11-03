
// CMCWeek2Dlg.h : header file
//

#pragma once
#include <vector>
#include "Contact.h"
// CCMCWeek2Dlg dialog


class CCMCWeek2Dlg : public CDialogEx
{
// Construction
public:
	CCMCWeek2Dlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CMCWEEK2_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

private:
	CListCtrl m_contactList;
	CListCtrl m_chatHistory;
	CEdit m_messageInput;
	int m_currentUserId;

	std::vector<Contact> m_contacts;
	int m_selectedContactId = -1;
	int m_lastMessageId = 0;
	bool RefreshContacts();
	bool LoadChatHistory();
public:
	afx_msg void OnLvnItemchangedContactList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedSend();
	void SetCurrentUserId(int userId);
};
