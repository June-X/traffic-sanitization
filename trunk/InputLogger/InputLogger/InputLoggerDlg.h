// InputLoggerDlg.h : header file
//

#pragma once

#define WM_NC (WM_USER + 1001)
#define IDC_NC (WM_USER + 1002) 
// CInputLoggerDlg dialog
class CInputLoggerDlg : public CDialog
{
// Construction
public:
	CInputLoggerDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_INPUTLOGGER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	virtual BOOL DestroyWindow();
	void OnMini();
// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	afx_msg LRESULT OnNc(WPARAM wParam, LPARAM lParam);
	afx_msg void OnMenuQuit();
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg void OnOK();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
};
