/***************************************************************************
 * ROM Properties Page shell extension. (librpsecure/win32)                *
 * restrict-dll.c: Restrict DLL lookups.                                   *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "restrict-dll.h"

// C includes
#include <assert.h>

// Windows SDK
#include <windows.h>
#include <tchar.h>

typedef BOOL (WINAPI *pfnSetDefaultDllDirectories_t)(DWORD DirectoryFlags);
typedef BOOL (WINAPI *pfnSetDllDirectoryW_t)(LPCWSTR lpPathName);

#ifndef LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR
#  define LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR    0x00000100
#endif
#ifndef LOAD_LIBRARY_SEARCH_APPLICATION_DIR
#  define LOAD_LIBRARY_SEARCH_APPLICATION_DIR 0x00000200
#endif
#ifndef LOAD_LIBRARY_SEARCH_USER_DIRS
#  define LOAD_LIBRARY_SEARCH_USER_DIRS       0x00000400
#endif
#ifndef LOAD_LIBRARY_SEARCH_SYSTEM32
#  define LOAD_LIBRARY_SEARCH_SYSTEM32        0x00000800
#endif
#ifndef LOAD_LIBRARY_SEARCH_DEFAULT_DIRS
#  define LOAD_LIBRARY_SEARCH_DEFAULT_DIRS    0x00001000
#endif

/**
 * Restrict DLL lookups to system directories.
 *
 * After calling this function, any DLLs located in the application
 * directory will need to be loaded using LoadLibrary() with an
 * absolute path.
 *
 * @return 0 on success; non-zero on error.
 */
int rp_secure_restrict_dll_lookups(void)
{
	// Reference: https://support.microsoft.com/en-gb/topic/secure-loading-of-libraries-to-prevent-dll-preloading-attacks-d41303ec-0748-9211-f317-2edc819682e1
	HANDLE hKernel32;

	union {
		pfnSetDefaultDllDirectories_t pfnSetDefaultDllDirectories;
		pfnSetDllDirectoryW_t pfnSetDllDirectoryW;
	} pfn;

	hKernel32 = GetModuleHandle(_T("kernel32.dll"));
	assert(hKernel32 != NULL);
	if (!hKernel32) {
		// Something is seriously wrong if kernel32 isn't loaded...
		DebugBreak();
		return -420;
	}

	// Attempt to use SetDefaultDllDirectories().
	// Reference: https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-setdefaultdlldirectories
	// - Available on Windows 8 and later.
	// - Also available on Windows 7 with KB2533623 installed.
	pfn.pfnSetDefaultDllDirectories = (pfnSetDefaultDllDirectories_t)GetProcAddress(hKernel32, "SetDefaultDllDirectories");
	if (pfn.pfnSetDefaultDllDirectories) {
		// Found SetDefaultDlLDirectories().
		if (pfn.pfnSetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32)) {
			// Success!
			return 0;
		}
	}

	// Attempt to use SetDllDirectory().
	// This will remove the current working directory from the search path.
	// It's not quite as good as SetDefaultDllDirectories(), though...
	// Reference: https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setdlldirectorya
	// - Available on Windows Vista and later.
	// - Also available on Windows XP with SP1.
	pfn.pfnSetDllDirectoryW = (pfnSetDllDirectoryW_t)GetProcAddress(hKernel32, "SetDllDirectoryW");
	if (pfn.pfnSetDllDirectoryW) {
		// Found SetDllDirectory().
		if (pfn.pfnSetDllDirectoryW(L"")) {
			// Success!
			return 0;
		}
	}

	// Failed to restrict the DLL lookup path...
	return -1;
}
