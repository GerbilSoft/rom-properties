/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * NtDllDlopen.hpp: NTDLL.DLL functions using dlopen().                    *
 *                                                                         *
 * Copyright (c) 2026 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "NtDllDlopen.hpp"

// C includes
#include <tchar.h>

// C includes (C++ namespace)
#include <cassert>
#include <cerrno>

namespace LibWin32Common {

/**
 * Initialize NTDLL.DLL function pointers.
 * @return 0 on success; negative POSIX error code on error.
 */
int NtDllDlopen::init(void)
{
	if (hNtDll) {
		// Already initialized.
		return 0;
	}

	// NTDLL.DLL should always be loaded by default,
	// unless maybe we're using Win9x?
	hNtDll = GetModuleHandle(_T("ntdll.dll"));
	if (!hNtDll) {
		// NTDLL not found?
		clearFnPtrs();
		return -ENOENT;
	}

	// Load the function pointers.
#define NTDLL_LOAD_PTR(fn) pfn_##fn = reinterpret_cast<__typeof__(fn)*>(GetProcAddress(hNtDll, #fn))
	NTDLL_LOAD_PTR(NtClose);
	NTDLL_LOAD_PTR(NtCreateFile);
	NTDLL_LOAD_PTR(NtDeleteFile);
	NTDLL_LOAD_PTR(NtOpenFile);
	NTDLL_LOAD_PTR(NtQueryDirectoryFile);
	NTDLL_LOAD_PTR(NtQueryInformationFile);
	NTDLL_LOAD_PTR(NtSetInformationFile);
	NTDLL_LOAD_PTR(NtWriteFile);

	if (!pfn_NtClose ||
	    !pfn_NtCreateFile ||
	    !pfn_NtDeleteFile ||
	    !pfn_NtOpenFile ||
	    !pfn_NtQueryDirectoryFile ||
	    !pfn_NtQueryInformationFile ||
	    !pfn_NtSetInformationFile ||
	    !pfn_NtWriteFile)
	{
		// At least one function pointer is missing...
		assert(!"Missing NTDLL.DLL function pointer???");
		clearFnPtrs();
		return -EIO;	// FIXME: No POSIX error code for "missing symbol"...
	}

	return 0;
}

void NtDllDlopen::clearFnPtrs(void)
{
#define NTDLL_CLEAR_PTR(fn) pfn_##fn = nullptr
	NTDLL_CLEAR_PTR(NtClose);
	NTDLL_CLEAR_PTR(NtCreateFile);
	NTDLL_CLEAR_PTR(NtDeleteFile);
	NTDLL_CLEAR_PTR(NtOpenFile);
	NTDLL_CLEAR_PTR(NtQueryDirectoryFile);
	NTDLL_CLEAR_PTR(NtQueryInformationFile);
	NTDLL_CLEAR_PTR(NtSetInformationFile);
	NTDLL_CLEAR_PTR(NtWriteFile);

	hNtDll = nullptr;
}

} // namespace LibWin32Common
