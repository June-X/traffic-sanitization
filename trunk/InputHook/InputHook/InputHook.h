// InputHook.h : main header file for the InputHook DLL
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols

// CInputHookApp
// See InputHook.cpp for the implementation of this class
//

class CInputHookApp : public CWinApp
{
public:
	CInputHookApp();

// Overrides
public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};
