/***************************************************************************
 * ROM Properties Page shell extension. (librpsecure)                      *
 * os-secure_openbsd.c: OS security functions. (OpenBSD)                   *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.librpsecure.h"
#include "os-secure.h"

// C includes.
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef HAVE_TAME
# include <sys/tame.h>
#endif /* HAVE_TAME */

/**
 * Enable OS-specific security functionality.
 * @param param OS-specific parameter.
 * @return 0 on success; negative POSIX error code on error.
 */
int rp_secure_enable(rp_secure_param_t param)
{
	int ret;

#if defined(HAVE_PLEDGE)
# ifdef HAVE_PLEDGE_EXECPROMISES
	// OpenBSD 6.3+: Second parameter is `const char *execpromises`.
	ret = pledge(param.promises, "");
# else /* !HAVE_PLEDGE_EXECPROMISES */
	// OpenBSD 5.9-6.2: Second parameter is `const char *paths[]`.
	ret = pledge(param.promises, NULL);
# endif /* HAVE_PLEDGE_EXECPROMISES */

#elif defined(HAVE_TAME)
	// OpenBSD 5.8: tame() function.
	// Similar to pledge(), but it takes a bitfield instead of
	// a string of pledges.
	// NOTE: stdio includes fattr, e.g. utimes().
	// Parameter must be `int flags`.
	ret = tame(param.tame_flags);
#else
# error Cannot compile os-secure_openbsd.c with no implementation.
#endif

	return (ret == 0 ? 0 : -errno);
}
