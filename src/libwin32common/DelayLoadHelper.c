/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * DelayLoadHelper.c: DelayLoad helper functions and macros.               *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: http://otb.manusoft.com/2013/01/using-delayload-to-specify-dependent-dll-path.htm
#include "stdafx.h"
#include "DelayLoadHelper.h"
#include "tcharx.h"

#include "libromdata/config.libromdata.h"

// C includes.
#include <stdlib.h>

// stdbool
#include "stdboolx.h"

// ImageBase for GetModuleFileName().
// Reference: https://devblogs.microsoft.com/oldnewthing/20041025-00/?p=37483
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

// Architecture-specific subdirectory.
#if defined(_M_ARM) || defined(__arm__) || \
      defined(_M_ARMT) || defined(__thumb__)
static const TCHAR rp_subdir[] = _T("arm\\");
#elif defined(_M_ARM64EC)
static const TCHAR rp_subdir[] = _T("arm64ec\\");
#elif defined(_M_ARM64) || defined(__aarch64__)
static const TCHAR rp_subdir[] = _T("arm64\\");
#elif defined(_M_IX86) || defined(__i386__)
static const TCHAR rp_subdir[] = _T("i386\\");
#elif defined(_M_X64) || defined(_M_AMD64) || defined(__amd64__) || defined(__x86_64__)
static const TCHAR rp_subdir[] = _T("amd64\\");
#elif defined(_M_IA64) || defined(__ia64__)
static const TCHAR rp_subdir[] = _T("ia64\\");
#elif defined(__riscv) && (__riscv_xlen == 32)
// TODO: MSVC riscv32 preprocessor macro, if one ever gets defined.
static const TCHAR rp_subdir[] = _T("riscv32\\");
#elif defined(__riscv) && (__riscv_xlen == 64)
// TODO: MSVC riscv64 preprocessor macro, if one ever gets defined.
static const TCHAR rp_subdir[] = _T("riscv64\\");
#else
#  error Unsupported CPU architecture.
#endif

// TODO: Are we using a debug suffix for MinGW-w64?

// libromdata DLL is "romdata-X.dll" (MSVC) or "libromdata-X.dll" (MinGW).
#ifdef _MSC_VER
#  define ROMDATA_PREFIX
#  ifdef NDEBUG
#    define DEBUG_SUFFIX ""
#  else
#    define DEBUG_SUFFIX "d"
#  endif
#else
#  define ROMDATA_PREFIX "lib"
#  define DEBUG_SUFFIX ""
#endif

static const char *const dll_whitelist[] = {
#ifdef RP_LIBROMDATA_IS_DLL
	ROMDATA_PREFIX "romdata-" LIBROMDATA_SOVERSION_STR DEBUG_SUFFIX ".dll",
#endif /* RP_LIBROMDATA_IS_DLL */
	"zlib1" DEBUG_SUFFIX ".dll",
	"libpng16" DEBUG_SUFFIX ".dll",
	"tinyxml2-10" DEBUG_SUFFIX ".dll",
	"zstd" DEBUG_SUFFIX ".dll",
	"lz4" DEBUG_SUFFIX ".dll",
	"minilzo" DEBUG_SUFFIX ".dll",
	"libgnuintl-8.dll",
};

/**
 * Explicit LoadLibrary() for delay-load.
 * @param pdli Delay-load info.
 * @return Library handle, or NULL on error.
 */
static HMODULE WINAPI rp_LoadLibrary(LPCSTR pszModuleName)
{
	// We only want to handle DLLs included with rom-properties.
	// System DLLs should be handled normally.
	TCHAR dll_fullpath[MAX_PATH+64];
	const TCHAR *bs;
	TCHAR *dest;
	LPCSTR pszDll;
	HMODULE hDll;

	DWORD dwResult;
	unsigned int path_len;
	unsigned int i;
	bool match = false;

	for (i = 0; i < _countof(dll_whitelist); i++) {
		if (!strcasecmp(pszModuleName, dll_whitelist[i])) {
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
	SetLastError(ERROR_SUCCESS);	// required for XP
	dwResult = GetModuleFileName(HINST_THISCOMPONENT,
		dll_fullpath, _countof(dll_fullpath));
	if (dwResult == 0 || dwResult >= _countof(dll_fullpath) || GetLastError() != ERROR_SUCCESS) {
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
	hDll = LoadLibraryEx(dll_fullpath, NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (hDll != NULL) {
		// DLL loaded successfully.
		return hDll;
	}

	// Check the architecture-specific subdirectory.
	_tcscpy(&dll_fullpath[path_len], rp_subdir);
	path_len += _countof(rp_subdir) - 1;

	// Copy the DLL name into the fullpath.
	// TODO: Use strcpy_s() or similar?
	dest = &dll_fullpath[path_len];
	for (pszDll = pszModuleName;
	     *pszDll != 0 && dest != &dll_fullpath[_countof(dll_fullpath)];
	     dest++, pszDll++)
	{
		*dest = (TCHAR)(unsigned int)*pszDll;
	}
	*dest = 0;

	// Attempt to load the DLL.
	return LoadLibraryEx(dll_fullpath, NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
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
			return (FARPROC)rp_LoadLibrary(pdli->szDll);
		default:
			break;
	}

	return NULL;
}

// Set the delay-load notification hook.
// NOTE: MSVC 2015 Update 3 makes this a const variable.
// References:
// - https://docs.microsoft.com/en-us/cpp/build/reference/notification-hooks?view=vs-2019
// - https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros?view=vs-2019
#if _MSC_VER > 1900 || (_MSC_VER == 1900 && _MSC_FULL_VER >= 190024210)
const PfnDliHook __pfnDliNotifyHook2 = rp_dliNotifyHook;
#else
PfnDliHook __pfnDliNotifyHook2 = rp_dliNotifyHook;
#endif
