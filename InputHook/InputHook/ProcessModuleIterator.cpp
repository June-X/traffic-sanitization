#include "StdAfx.h"
#include "ProcessModuleIterator.h"
#include <Psapi.h>

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