/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * os-secure_win32.c: OS security functions. (Win32)                       *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "os-secure.h"

// C includes.
#include <stdio.h>
#include "tcharx.h"

// libwin32common
#include "libwin32common/RpWin32_sdk.h"
#include "libwin32common/secoptions.h"
#include "libwin32common/integrity_level.h"

/**
 * Enable OS-specific security functionality.
 * @return 0 on success; negative POSIX error code on error.
 */
int rp_download_os_secure(void)
{
	int level;

	// Check the process integrity level.
	// If it's not low, adjust it.
	// TODO: Just adjust it without checking?
	level = GetProcessIntegrityLevel();
	if (level > SECURITY_MANDATORY_LOW_RID) {
		DWORD dwRet;
#ifndef NDEBUG
		_ftprintf(stderr, _T("*** DEBUG: Integrity level is %d (NOT LOW). Adjusting to low...\n"), level);
#endif /* NDEBUG */
		dwRet = SetProcessIntegrityLevel(SECURITY_MANDATORY_LOW_RID);
		if (dwRet == 0) {
			// Verify that it succeeded.
			// TODO: Return an error code if it fails?
			level = GetProcessIntegrityLevel();
#ifndef NDEBUG
			if (level <= SECURITY_MANDATORY_LOW_RID) {
				_ftprintf(stderr, _T("*** DEBUG: Integrity level reduced to: %d\n"), level);
			} else {
				_ftprintf(stderr, _T("*** DEBUG: Integrity level NOT reduced: %d\n"), level);
			}
#endif /* !NDEBUG */
		} else {
			// TODO: Return an error code?
#ifndef NDEBUG
			_ftprintf(stderr, _T("*** DEBUG: SetProcessIntegrityLevel() failed: %u\n"), dwRet);
#endif /* !NDEBUG */
		}
	}

	// Set Win32 security options.
	// NOTE: Must be done *after* reducing the process integrity level.
	// FIXME: Enabling high-security (Win32k syscall disable) requires
	// eliminating anything that links to GDI, e.g. ole32.dll and shell32.dll.
	rp_secoptions_init(FALSE);

	return 0;
}
