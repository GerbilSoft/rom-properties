/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * DelayLoad.cpp: Delay-load helper.                                       *
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
#include <delayimp.h>

// Our DLL name from DllMain.
extern wchar_t dll_filename[];

/**
 * Explicit LoadLibrary() for delay-load.
 * @param pdli Delay-load info.
 * @return Library handle, or NULL on error.
 */
static HMODULE rp_loadLibrary(LPCSTR pszModuleName)
{
	// NOTE: Delay-load only supports ANSI module names.
	// We'll assume it's ASCII and do a simple conversion to Unicode.
	wchar_t dll_fullpath[MAX_PATH+32];

	// Find the last backslash in dll_filename[].
	const wchar_t *bs = wcsrchr(dll_filename, L'\\');
	if (!bs) {
		// No backslashes...
		return nullptr;
	}

	// Copy up to (and including) the last backslash, then
	// append the module name.
	unsigned int path_len = (unsigned int)(bs - dll_filename + 1);
	memcpy(dll_fullpath, dll_filename, path_len * sizeof(wchar_t));
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
