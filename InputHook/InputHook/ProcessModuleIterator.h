#pragma once

class CProcessModuleIterator
{
protected:
	HANDLE	m_hProcess;			// process handle
	HMODULE*	m_hModules;			// array of module handles
	DWORD		m_count;				// size of array
	DWORD		m_current;			// next module handle
public:
	CProcessModuleIterator(DWORD pid);
	~CProcessModuleIterator();

	HANDLE GetProcessHandle()	{ return m_hProcess; }
	DWORD GetCount()				{ return m_count; }
	HMODULE First();
	HMODULE Next() {
		return m_hProcess && m_current < m_count ? m_hModules[m_current++] : 0;
	}
};
