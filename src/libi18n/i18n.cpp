/***************************************************************************
 * ROM Properties Page shell extension. (libi18n)                          *
 * i18n.cpp: Internationalization support code.                            *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// NOTE: We need to have at least one file compiled in order to
// build a static library. Otherwise, CMake will complain.

// Since we don't want any code actually compiled here if NLS is disabled,
// wrap the whole thing in #ifdef ENABLE_NLS.
#include "libi18n/config.libi18n.h"

#ifdef ENABLE_NLS

#include "i18n.hpp"

// Initialized?
static bool i18n_is_init = false;

// C++ STL classes
#include <mutex>

#ifdef _WIN32
#  include "libwin32common/RpWin32_sdk.h"
#  include <stdlib.h>	// _countof() on 32-bit MinGW-w64
#  include <string.h>
#  include <tchar.h>

// Architecture name for the arch-specific subdirectory.
#if defined(_M_ARM) || defined(__arm__) || \
      defined(_M_ARMT) || defined(__thumb__)
#  define ARCH_NAME _T("arm")
#elif defined(_M_ARM64EC)
#  define ARCH_NAME _T("arm64ec")
#elif defined(_M_ARM64) || defined(__aarch64__)
#  define ARCH_NAME _T("arm64")
#elif defined(_M_IX86) || defined(__i386__)
#  define ARCH_NAME _T("i386")
#elif defined(_M_X64) || defined(_M_AMD64) || defined(__amd64__) || defined(__x86_64__)
#  define ARCH_NAME _T("amd64")
#elif defined(_M_IA64) || defined(__ia64__)
#  define ARCH_NAME _T("ia64")
#elif defined(__riscv) && (__riscv_xlen == 32)
// TODO: MSVC riscv32 preprocessor macro, if one ever gets defined.
#  define ARCH_NAME _T("riscv32")
#elif defined(__riscv) && (__riscv_xlen == 64)
// TODO: MSVC riscv64 preprocessor macro, if one ever gets defined.
#  define ARCH_NAME _T("riscv64")
#else
#  error Unsupported CPU architecture.
#endif

#ifdef _UNICODE
#  define tbindtextdomain(domainname, dirname) wbindtextdomain((domainname), (dirname))
#else /* !_UNICODE */
#  define tbindtextdomain(domainname, dirname) bindtextdomain((domainname), (dirname))
#endif /* _UNICODE */

/**
 * Initialize the internationalization subsystem.
 * (Windows version)
 *
 * Called by std::call_once().
 */
static void rp_i18n_init_int(void)
{
	// Windows: Use the application-specific locale directory.
	TCHAR tpathname[MAX_PATH+16];

	// Get the current module filename.
	// NOTE: Delay-load only supports ANSI module names.
	// We'll assume it's ASCII and do a simple conversion to Unicode.
	SetLastError(ERROR_SUCCESS);	// required for XP
	DWORD dwResult = GetModuleFileName(HINST_THISCOMPONENT,
		tpathname, _countof(tpathname));
	if (dwResult == 0 || dwResult >= _countof(tpathname) || GetLastError() != ERROR_SUCCESS) {
		// Cannot get the current module filename.
		// TODO: Windows XP doesn't SetLastError() if the
		// filename is too big for the buffer.
		i18n_is_init = false;
		return;
	}

	// Find the last backslash in tpathname[].
	TCHAR *bs = _tcsrchr(tpathname, _T('\\'));
	if (!bs) {
		// No backslashes...
		i18n_is_init = false;
		return;
	}

	// Append the "locale" subdirectory.
	_tcscpy(bs+1, _T("locale"));
	DWORD dwAttrs = GetFileAttributes(tpathname);
	if (dwAttrs == INVALID_FILE_ATTRIBUTES ||
	    !(dwAttrs & FILE_ATTRIBUTE_DIRECTORY))
	{
		// Not found, or not a directory.
		// Try one level up.
		*bs = 0;
		bs = _tcsrchr(tpathname, _T('\\'));
		if (!bs) {
			// No backslashes...
			i18n_is_init = false;
			return;
		}

		// Make sure the current subdirectory matches
		// the DLL architecture.
#ifdef _M_ARM64EC
		// Windows, ARM64EC: Subdirectory could also be "arm64".
		if (_tcsicmp(bs+1, _T("arm64")) != 0)
#endif /* _M_ARM64EC */
		{
			if (_tcsicmp(bs+1, ARCH_NAME) != 0) {
				// Not a match.
				i18n_is_init = false;
				return;
			}
		}

		// Append the "locale" subdirectory.
		_tcscpy(bs+1, _T("locale"));
		dwAttrs = GetFileAttributes(tpathname);
		if (dwAttrs == INVALID_FILE_ATTRIBUTES ||
		    !(dwAttrs & FILE_ATTRIBUTE_DIRECTORY))
		{
			// Not found, or not a subdirectory.
			i18n_is_init = false;
			return;
		}
	}

	// Found the locale subdirectory.
	// Bind the gettext domain.
	LPCTSTR base = tbindtextdomain(RP_I18N_DOMAIN, tpathname);
	i18n_is_init = (base != NULL);
}

#else /* !_WIN32 */

/**
 * Initialize the internationalization subsystem.
 * (Unix/Linux version)
 *
 * Called by std::call_once().
 */
static void rp_i18n_init_int(void)
{
	// Unix/Linux: Use the system-wide locale directory.
	const char *const base = bindtextdomain(RP_I18N_DOMAIN, DIR_INSTALL_LOCALE);
	i18n_is_init = (base != NULL);
}
#endif /* _WIN32 */

/**
 * Initialize the internationalization subsystem.
 * (Unix/Linux version)
 *
 * @return 0 on success; non-zero on error.
 */
int rp_i18n_init(void)
{
	static std::once_flag i18n_once_flag;
	std::call_once(i18n_once_flag, rp_i18n_init_int);
	return (i18n_is_init ? 0 : -1);
}

#endif /* ENABLE_NLS */
