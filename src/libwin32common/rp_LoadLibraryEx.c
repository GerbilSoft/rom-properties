/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * rp_LoadLibraryEx.c: LoadLibraryEx() wrapper function.                   *
 *                                                                         *
 * Copyright (c) 2026 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// NOTE: LoadLibraryEx() Search flags are not supported prior to Windows Vista.
// Windows Vista, Server 2008 R2, and 7 require KB2533623 for proper functionality.
// (For Windows 7, KB3063858 supercedes KB2533623.)

#include "rp_LoadLibraryEx.h"
#include "rp_versionhelpers.h"

#define LOAD_LIBRARY_SEARCH_FLAGS \
	(LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | \
	 LOAD_LIBRARY_SEARCH_APPLICATION_DIR | \
	 LOAD_LIBRARY_SEARCH_USER_DIRS | \
	 LOAD_LIBRARY_SEARCH_SYSTEM32 | \
	 LOAD_LIBRARY_SEARCH_DEFAULT_DIRS)

/**
 * LoadLibraryExA() wrapper function.
 *
 * If the OS version is earlier than Windows XP, all LOAD_LIBRARY_SEARCH_*
 * flags will be filtered out.
 *
 * @param lpLibFileName
 * @param hFile
 * @param dwFlags
 * @return HMODULE
 */
HMODULE rp_LoadLibraryExA(_In_ LPCSTR lpLibFileName, _Reserved_ HANDLE hFile, _In_ DWORD dwFlags)
{
	if (!IsWindowsVistaOrGreater()) {
		dwFlags &= ~LOAD_LIBRARY_SEARCH_FLAGS;
	}

	return LoadLibraryExA(lpLibFileName, hFile, dwFlags);
}

/**
 * LoadLibraryExW() wrapper function.
 *
 * If the OS version is earlier than Windows XP, all LOAD_LIBRARY_SEARCH_*
 * flags will be filtered out.
 *
 * @param lpLibFileName
 * @param hFile
 * @param dwFlags
 * @return HMODULE
 */
HMODULE rp_LoadLibraryExW(_In_ LPCWSTR lpLibFileName, _Reserved_ HANDLE hFile, _In_ DWORD dwFlags)
{
	if (!IsWindowsVistaOrGreater()) {
		dwFlags &= ~LOAD_LIBRARY_SEARCH_FLAGS;
	}

	return LoadLibraryExW(lpLibFileName, hFile, dwFlags);
}
