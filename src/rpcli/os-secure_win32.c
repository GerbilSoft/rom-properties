/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
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
int rpcli_os_secure(void)
{
	// Set Win32 security options.
	rp_secoptions_init(TRUE);

	// NOTE: We're not reducing the process integrity level here,
	// since rpcli might be used to extract images to somewhere
	// within the user's home directory.
	return 0;
}
