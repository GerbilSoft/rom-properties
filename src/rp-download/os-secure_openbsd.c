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
#ifdef HAVE_TAME
# include <sys/tame.h>
#endif /* HAVE_TAME */

/**
 * Enable OS-specific security functionality.
 * @return 0 on success; negative POSIX error code on error.
 */
int rp_download_os_secure(void)
{
	int ret;

#if defined(HAVE_PLEDGE)
	// Promises:
	// - stdio: General stdio functionality.
	// - rpath: Read from ~/.config/rom-properties/ and ~/.cache/rom-properties/
	// - wpath: Write to ~/.cache/rom-properties/
	// - cpath: Create ~/.cache/rom-properties/ if it doesn't exist.
	// - inet: Internet access.
	// - fattr: Modify file attributes, e.g. mtime.
	// - dns: Resolve hostnames.
	// - getpw: Get user's home directory if HOME is empty.
# ifdef HAVE_PLEDGE_EXECPROMISES
	// OpenBSD 6.3+: Second parameter is `const char *execpromises`.
	ret = pledge("stdio rpath wpath cpath inet fattr dns getpw", "");
# else /* !HAVE_PLEDGE_EXECPROMISES */
	// OpenBSD 5.9-6.2: Second parameter is `const char *paths[]`.
	ret = pledge("stdio rpath wpath cpath inet fattr dns getpw", NULL);
# endif /* HAVE_PLEDGE_EXECPROMISES */

#elif defined(HAVE_TAME)
	// OpenBSD 5.8: tame() function.
	// Similar to pledge(), but it takes a bitfield instead of
	// a string of pledges.
	// NOTE: stdio includes fattr, e.g. utimes().
	tame(TAME_STDIO | TAME_RPATH | TAME_WPATH | TAME_CPATH |
	     TAME_INET | TAME_DNS | TAME_GETPW);
#endif

	return (ret == 0 ? 0 : -errno);
}
