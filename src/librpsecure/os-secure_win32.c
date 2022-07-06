/***************************************************************************
 * ROM Properties Page shell extension. (librpsecure)                      *
 * os-secure_win32.c: OS security functions. (Win32)                       *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "os-secure.h"

// C includes.
#ifndef NDEBUG
# include <stdio.h>
#endif /* !NDEBUG */

// Windows includes.
#include <windows.h>
#include <tchar.h>

// Integrity level and security options.
#include "win32/integrity_level.h"
#include "win32/secoptions.h"

/**
 * Reduce the process integrity level to Low.
 * (Windows only; no-op on other platforms.)
 * @return 0 on success; negative POSIX error code on error.
 */
int rp_secure_reduce_integrity(void)
{
	DWORD dwRet;

	// Check the process integrity level.
	// If it's not low, adjust it.
	// TODO: Just adjust it without checking?
	int level = GetProcessIntegrityLevel();
	if (level <= SECURITY_MANDATORY_LOW_RID) {
		// We're already at low integrity.
		return 0;
	}

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
		_ftprintf(stderr, _T("*** DEBUG: SetProcessIntegrityLevel() failed: %lu\n"), dwRet);
#endif /* !NDEBUG */
	}

	return 0;
}

/**
 * Enable OS-specific security functionality.
 * @param param OS-specific parameter.
 * @return 0 on success; negative POSIX error code on error.
 */
int rp_secure_enable(rp_secure_param_t param)
{
	// Set Win32 security options.
	// NOTE: This Must be done *after* reducing the process integrity level.
	// TODO: Return error code?
	rp_secure_win32_secoptions_init(param.bHighSec);

	return 0;
}
