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

#pragma once

#include "RpWin32_sdk.h"
#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

#ifdef __cplusplus
extern "C" {
#endif

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
RP_LIBROMDATA_PUBLIC
HMODULE rp_LoadLibraryExA(_In_ LPCSTR lpLibFileName, _Reserved_ HANDLE hFile, _In_ DWORD dwFlags);

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
RP_LIBROMDATA_PUBLIC
HMODULE rp_LoadLibraryExW(_In_ LPCWSTR lpLibFileName, _Reserved_ HANDLE hFile, _In_ DWORD dwFlags);

#ifdef UNICODE
#  define rp_LoadLibraryEx rp_LoadLibraryExW
#else
#  define rp_LoadLibraryEx rp_LoadLibraryExA
#endif

#ifdef __cplusplus
}
#endif
