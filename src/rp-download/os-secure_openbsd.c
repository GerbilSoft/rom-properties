/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * os-secure_posix.c: OS security functions. (OpenBSD)                     *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include "config.rp-download.h"
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
	int ret;

	// Promises:
	// - stdio: General stdio functionality.
	// - rpath: Read from ~/.config/rom-properties/ and ~/.cache/rom-properties/
	// - wpath: Write to ~/.cache/rom-properties/
	// - cpath: Create ~/.cache/rom-properties/ if it doesn't exist.
	// - inet: Internet access.
	// - fattr: Modify file attributes, e.g. mtime.
	// - dns: Resolve hostnames.
#ifdef HAVE_PLEDGE_EXECPROMISES
	// OpenBSD 6.3+: Second parameter is `const char *execpromises`.
	ret = pledge("stdio rpath wpath cpath inet fattr dns", "");
#else /* !HAVE_PLEDGE_EXECPROMISES */
	// OpenBSD 5.9-6.2: Second parameter is `const char *paths[]`.
	ret = pledge("stdio rpath wpath cpath inet fattr dns", NULL);
#endif /* HAVE_PLEDGE_EXECPROMISES */

	return (ret == 0 ? 0 : -errno);
}
