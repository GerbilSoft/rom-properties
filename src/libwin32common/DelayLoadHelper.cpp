/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * DelayLoadHelper.cpp: DelayLoad helper functions and macros.             *
 *                                                                         *
 * Copyright (c) 2017 by David Korth.                                      *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

// Reference: http://otb.manusoft.com/2013/01/using-delayload-to-specify-dependent-dll-path.htm
#include "stdafx.h"
#include "DelayLoadHelper.hpp"

// C includes.
#include <stdlib.h>

// ImageBase for GetModuleFileName().
// Reference: https://blogs.msdn.microsoft.com/oldnewthing/20041025-00/?p=37483
extern "C" IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

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
	static const char prefix_whitelist[][12] = {
		"\x05" "zlib1",
		"\x06" "libpng",
		"\x04" "jpeg",
		"\x08" "tinyxml2",
	};

	bool match = false;
	for (unsigned int i = 0; i < _countof(prefix_whitelist); i++) {
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
		return nullptr;
	}

	// NOTE: Delay-load only supports ANSI module names.
	// We'll assume it's ASCII and do a simple conversion to Unicode.
	wchar_t dll_fullpath[MAX_PATH+32];
	SetLastError(ERROR_SUCCESS);
	DWORD dwResult = GetModuleFileName(HINST_THISCOMPONENT,
		dll_fullpath, _countof(dll_fullpath));
	if (dwResult == 0 || GetLastError() != ERROR_SUCCESS) {
		// Cannot get the current module filename.
		// TODO: Windows XP doesn't SetLastError() if the
		// filename is too big for the buffer.
		return nullptr;
	}

	// Find the last backslash in dll_fullpath[].
	const wchar_t *bs = wcsrchr(dll_fullpath, L'\\');
	if (!bs) {
		// No backslashes...
		return nullptr;
	}

	// Append the module name.
	// append the module name.
	unsigned int path_len = (unsigned int)(bs - &dll_fullpath[0] + 1);
	wchar_t *dest = &dll_fullpath[path_len];
	for (; *pszModuleName != 0 && dest != &dll_fullpath[_countof(dll_fullpath)];
	     dest++, pszModuleName++)
	{
		*dest = (wchar_t)(unsigned int)*pszModuleName;
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

	return nullptr;
}

// Set the delay-load notification hook.
// NOTE: MSVC 2015 Update 3 makes this a const variable.
// This version also introduced the _MSVC_LANG macro,
// so we can use that to determine if const is needed.
// Reference: https://msdn.microsoft.com/en-us/library/b0084kay.aspx
#if _MSC_VER > 1900 || (_MSC_VER == 1900 && defined(_MSVC_LANG))
extern "C" const PfnDliHook __pfnDliNotifyHook2 = rp_dliNotifyHook;
#else
extern "C" PfnDliHook __pfnDliNotifyHook2 = rp_dliNotifyHook;
#endif
