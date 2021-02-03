/***************************************************************************
 * ROM Properties Page shell extension. (libi18n)                          *
 * i18n.c: Internationalization support code.                              *
 *                                                                         *
 * Copyright (c) 2017-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// NOTE: We need to have at least one file compiled in order to
// build a static library. Otherwise, CMake will complain.

// Since we don't want any code actually compiled here if NLS is disabled,
// wrap the whole thing in #ifdef ENABLE_NLS.

#include "libi18n/config.libi18n.h"
#ifdef ENABLE_NLS

#include "i18n.h"
#ifdef _WIN32
# include "libwin32common/RpWin32_sdk.h"
#endif

#include "common.h"

#ifdef _WIN32
// Architecture name.
#if defined(_M_X64) || defined(__amd64__)
# define ARCH_NAME _T("amd64")
#elif defined(_M_IX86) || defined(__i386__)
# define ARCH_NAME _T("i386")
#else
# error Unsupported CPU architecture.
#endif

/**
 * Initialize the internationalization subsystem.
 * (Windows version)
 *
 * @return 0 on success; non-zero on error.
 */
int rp_i18n_init(void)
{
	// Windows: Use the application-specific locale directory.
	DWORD dwResult, dwAttrs;
	TCHAR tpathname[MAX_PATH+16];

	TCHAR *bs;
	LPCTSTR base;

	// Get the current module filename.
	// NOTE: Delay-load only supports ANSI module names.
	// We'll assume it's ASCII and do a simple conversion to Unicode.
	SetLastError(ERROR_SUCCESS);
	dwResult = GetModuleFileName(HINST_THISCOMPONENT,
		tpathname, ARRAY_SIZE(tpathname));
	if (dwResult == 0 || GetLastError() != ERROR_SUCCESS) {
		// Cannot get the current module filename.
		// TODO: Windows XP doesn't SetLastError() if the
		// filename is too big for the buffer.
		return -1;
	}

	// Find the last backslash in tpathname[].
	bs = _tcsrchr(tpathname, _T('\\'));
	if (!bs) {
		// No backslashes...
		return -1;
	}

	// Append the "locale" subdirectory.
	_tcscpy(bs+1, _T("locale"));
	dwAttrs = GetFileAttributes(tpathname);
	if (dwAttrs == INVALID_FILE_ATTRIBUTES ||
	    !(dwAttrs & FILE_ATTRIBUTE_DIRECTORY))
	{
		// Not found, or not a directory.
		// Try one level up.
		*bs = 0;
		bs = _tcsrchr(tpathname, _T('\\'));
		if (!bs) {
			// No backslashes...
			return -1;
		}

		// Make sure the current subdirectory matches
		// the DLL architecture.
		if (_tcscmp(bs+1, ARCH_NAME) != 0) {
			// Not a match.
			return -1;
		}

		// Append the "locale" subdirectory.
		_tcscpy(bs+1, _T("locale"));
		dwAttrs = GetFileAttributes(tpathname);
		if (dwAttrs == INVALID_FILE_ATTRIBUTES ||
		    !(dwAttrs & FILE_ATTRIBUTE_DIRECTORY))
		{
			// Not found, or not a subdirectory.
			return -1;
		}
	}

	// Found the locale subdirectory.
	// Bind the gettext domain.
	// NOTE: gettext-0.21 supports Unicode paths using wbindtextdomain().
#ifdef UNICODE
	base = wbindtextdomain(RP_I18N_DOMAIN, tpathname);
#else /* !UNICODE */
	base = bindtextdomain(RP_I18N_DOMAIN, tpathname);
#endif /* UNICODE */
	if (!base) {
		// bindtextdomain() failed.
		return -1;
	}

	// Initialized.
	return 0;
}

#else /* !_WIN32 */

/**
 * Initialize the internationalization subsystem.
 * (Unix/Linux version)
 *
 * @return 0 on success; non-zero on error.
 */
int rp_i18n_init(void)
{
	// Unix/Linux: Use the system-wide locale directory.
	const char *const base = bindtextdomain(RP_I18N_DOMAIN, DIR_INSTALL_LOCALE);
	if (!base) {
		// bindtextdomain() failed.
		return -1;
	}
	return 0;
}
#endif /* _WIN32 */

#endif /* ENABLE_NLS */
