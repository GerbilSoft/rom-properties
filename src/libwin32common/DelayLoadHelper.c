/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * DelayLoadHelper.c: DelayLoad helper functions and macros.               *
 *                                                                         *
 * Copyright (c) 2017-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: http://otb.manusoft.com/2013/01/using-delayload-to-specify-dependent-dll-path.htm
#include "stdafx.h"
#include "DelayLoadHelper.h"

// C includes.
#include <stdlib.h>

// TODO: Move to our own stdboolx.h file.
#ifndef __cplusplus
# if defined(_MSC_VER) && _MSC_VER >= 1800
#  include <stdbool.h>
# else
typedef unsigned char bool;
#  define true 1
#  define false 0
# endif
#endif /* __cplusplus */

// ImageBase for GetModuleFileName().
// Reference: https://blogs.msdn.microsoft.com/oldnewthing/20041025-00/?p=37483
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

// Architecture-specific subdirectory.
#if defined(__i386__) || defined(_M_IX86)
static const TCHAR rp_subdir[] = _T("i386\\");
#elif defined(__amd64__) || defined(_M_X64)
static const TCHAR rp_subdir[] = _T("amd64\\");
#else
# error Unsupported CPU architecture.
#endif

/**
 * Explicit LoadLibrary() for delay-load.
 * @param pdli Delay-load info.
 * @return Library handle, or NULL on error.
 */
static HMODULE WINAPI rp_loadLibrary(LPCSTR pszModuleName)
{
	// We only want to handle DLLs included with rom-properties.
	// System DLLs should be handled normally.
	// DLL whitelist: First byte is an explicit length.
	static const char prefix_whitelist[][16] = {
		"\x05" "zlib1",
		"\x06" "libpng",
		"\x04" "jpeg",
		"\x08" "tinyxml2",
		"\x0C" "libgnuintl-8",
	};

	TCHAR dll_fullpath[MAX_PATH+64];
	const TCHAR *bs;
	TCHAR *dest;
	LPCSTR pszDll;
	HMODULE hDll;
	
	DWORD dwResult;
	unsigned int path_len;
	unsigned int i;
	bool match = false;

	for (i = 0; i < _countof(prefix_whitelist); i++) {
		if (!strncasecmp(pszModuleName, &prefix_whitelist[i][1],
		     (unsigned int)prefix_whitelist[i][0]))
		{
			// Found a match.
			match = true;
			break;
		}
	}
	if (!match) {
		// Not a match. Use standard delay-load.
		return NULL;
	}

	// NOTE: Delay-load only supports ANSI module names.
	// We'll assume it's ASCII and do a simple conversion to Unicode.
	SetLastError(ERROR_SUCCESS);
	dwResult = GetModuleFileName(HINST_THISCOMPONENT,
		dll_fullpath, _countof(dll_fullpath));
	if (dwResult == 0 || GetLastError() != ERROR_SUCCESS) {
		// Cannot get the current module filename.
		// TODO: Windows XP doesn't SetLastError() if the
		// filename is too big for the buffer.
		return NULL;
	}

	// Find the last backslash in dll_fullpath[].
	bs = _tcsrchr(dll_fullpath, _T('\\'));
	if (!bs) {
		// No backslashes...
		return NULL;
	}

	// Append the module name.
	path_len = (unsigned int)(bs - &dll_fullpath[0] + 1);
	dest = &dll_fullpath[path_len];
	for (pszDll = pszModuleName;
	     *pszDll != 0 && dest != &dll_fullpath[_countof(dll_fullpath)];
	     dest++, pszDll++)
	{
		*dest = (TCHAR)(unsigned int)*pszDll;
	}
	*dest = 0;

	// Attempt to load the DLL.
	hDll = LoadLibrary(dll_fullpath);
	if (hDll != NULL) {
		// DLL loaded successfully.
		return hDll;
	}

	// Check the architecture-specific subdirectory.
	_tcscpy(&dll_fullpath[path_len], rp_subdir);
	path_len += _countof(rp_subdir) - 1;

	dest = &dll_fullpath[path_len];
	for (pszDll = pszModuleName;
	     *pszDll != 0 && dest != &dll_fullpath[_countof(dll_fullpath)];
	     dest++, pszDll++)
	{
		*dest = (TCHAR)(unsigned int)*pszDll;
	}
	*dest = 0;

	// Attempt to load the DLL.
	return LoadLibrary(dll_fullpath);
}

/**
 * Delay-load notification hook.
 * @param dliNotify Notification code.
 * @param pdli Delay-load info.
 * @return
 */
static FARPROC WINAPI rp_dliNotifyHook(unsigned int dliNotify, PDelayLoadInfo pdli)
{
	switch (dliNotify) {
		case dliNotePreLoadLibrary:
			return (FARPROC)rp_loadLibrary(pdli->szDll);
		default:
			break;
	}

	return NULL;
}

// Set the delay-load notification hook.
// NOTE: MSVC 2015 Update 3 makes this a const variable.
// Reference: https://msdn.microsoft.com/en-us/library/b0084kay.aspx
#if _MSC_VER > 1900 || (_MSC_VER == 1900 && _MSC_FULL_VER >= 190024210)
const PfnDliHook __pfnDliNotifyHook2 = rp_dliNotifyHook;
#else
PfnDliHook __pfnDliNotifyHook2 = rp_dliNotifyHook;
#endif
