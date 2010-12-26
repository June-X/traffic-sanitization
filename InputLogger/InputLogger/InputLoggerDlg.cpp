// InputLoggerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "InputLogger.h"
#include "InputLoggerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	ON_WM_DRAWITEM()
END_MESSAGE_MAP()


// CInputLoggerDlg dialog




CInputLoggerDlg::CInputLoggerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CInputLoggerDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CInputLoggerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CInputLoggerDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(ID_OK, OnOK)
	ON_COMMAND(ID_MENU_QUIT, OnMenuQuit)
	ON_MESSAGE(WM_NC, OnNc)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


// CInputLoggerDlg message handlers

BOOL CInputLoggerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CreateMutex(NULL, FALSE, "InputLogger");
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		CDialog::OnOK();
	}
	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	static HMODULE hDLL;
	typedef BOOL (CALLBACK *HookFunctionType)();
	HookFunctionType installHook;
	if (hDLL = LoadLibrary((LPCTSTR)"InputHook.dll")) {
		installHook = (HookFunctionType)GetProcAddress(hDLL, "installHook");
		installHook();
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

BOOL CInputLoggerDlg::DestroyWindow()
{
	NOTIFYICONDATA nc;
	nc.cbSize = sizeof(NOTIFYICONDATA);
	nc.hWnd = m_hWnd;
	nc.uCallbackMessage = WM_NC;
	nc.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nc.uID = IDC_NC;
	Shell_NotifyIcon(NIM_DELETE,&nc);
	Shell_NotifyIcon(NIM_DELETE, &nc);
	return CDialog::DestroyWindow();
}

void CInputLoggerDlg::OnMini()
{
		NOTIFYICONDATA nc;
		nc.cbSize = sizeof(NOTIFYICONDATA);
		nc.hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
		nc.hWnd = m_hWnd;
		lstrcpy(nc.szTip, "InputLogger");
		nc.uCallbackMessage = WM_NC;
		nc.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		nc.uID = IDC_NC;
		Shell_NotifyIcon(NIM_ADD, &nc);
		ShowWindow(false);
		SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | WS_EX_TOPMOST);
}

LRESULT CInputLoggerDlg::OnNc(WPARAM wParam, LPARAM lParam)
{
	if (lParam == WM_LBUTTONDOWN) {
		ShowWindow(SW_SHOWNORMAL);
		NOTIFYICONDATA nc;
		nc.cbSize = sizeof(NOTIFYICONDATA);
		nc.hWnd = m_hWnd;
		nc.uCallbackMessage = WM_NC;
		nc.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		nc.uID = IDC_NC;
		Shell_NotifyIcon(NIM_DELETE, &nc);
		::SetWindowPos(GetSafeHwnd(), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
	return NULL;
}

void CInputLoggerDlg::OnMenuQuit()
{
	CDialog::OnOK();
}

void CInputLoggerDlg::OnOK()
{
	OnMini();
}

void CInputLoggerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CInputLoggerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CInputLoggerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

