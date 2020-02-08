/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * os-secure_posix.c: OS security functions. (OpenBSD)                     *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "os-secure.h"

// C includes.
#include <errno.h>
#include <unistd.h>

/**
 * Enable OS-specific security functionality.
 * @return 0 on success; negative POSIX error code on error.
 */
int rp_download_os_secure(void)
{
	// Promises:
	// - stdio: General stdio functionality.
	// - rpath: Read from ~/.config/rom-properties/ and ~/.cache/rom-properties/
	// - wpath: Write to ~/.cache/rom-properties/
	// - cpath: Create ~/.cache/rom-properties/ if it doesn't exist.
	// - inet: Internet access.
	// - fattr: Modify file attributes, e.g. mtime.
	// - dns: Resolve hostnames.
	int ret = pledge("stdio rpath wpath cpath inet fattr dns", "");
	return (ret == 0 ? 0 : -errno);
}
