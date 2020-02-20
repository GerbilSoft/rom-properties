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
	IntegrityLevel level;

	// Check the process integrity level.
	// If it's not low, adjust it.
	// TODO: Just adjust it without checking?
	level = GetIntegrityLevel();
	if (level > INTEGRITY_LOW) {
#ifndef NDEBUG
		_ftprintf(stderr, _T("*** Integrity level is %u (NOT LOW). Adjusting to low...\n"), level);
#endif /* NDEBUG */
		SetIntegrityLevel(INTEGRITY_LOW);
		// NOTE: GetIntegrityLevel() fails attempting to open the process token.
		// This might be a side effect of switching to low integrity...
		// TODO: Verify with procexp.
		// TODO: Consolidate the token adjustment code in integrity_level.c.
		level = GetIntegrityLevel();
#ifndef NDEBUG
		if (level <= INTEGRITY_LOW) {
			_ftprintf(stderr, _T("*** Integrity level reduced to: %u\n"), level);
		} else {
			_ftprintf(stderr, _T("*** Integrity level NOT reduced: %u\n"), level);
		}
#endif /* !NDEBUG */
	}

	// Set Win32 security options.
	// NOTE: Must be done *after* reducing the process integrity level.
	// FIXME: Enabling high-security (Win32k syscall disable) requires
	// eliminating anything that links to GDI, e.g. ole32.dll and shell32.dll.
	rp_secoptions_init(FALSE);

	return 0;
}
