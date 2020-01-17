/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * os-secure_win32.c: OS security functions. (Win32)                       *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "os-secure.h"

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

	// Set Win32 security options.
	secoptions_init(TRUE);

	// Check the process integrity level.
	// TODO: If it's higher than low, relaunch the program with low integrity if supported.
	level = GetIntegrityLevel();
	if (level > INTEGRITY_LOW) {
		_ftprintf(stderr, _T("WARNING: Not running as low integrity!!!\n"));
	}

	return 0;
}
