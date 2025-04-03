/***************************************************************************
 * ROM Properties Page shell extension. (librpsecure)                      *
 * os-secure.h: OS security functions.                                     *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "config.librpsecure.h"
#include "stdboolx.h"

#ifdef ENABLE_EXTRA_SECURITY
#  ifdef _WIN32
#    include <windows.h>
#  else /* !_WIN32 */
#    include <unistd.h>
#    ifdef HAVE_SECCOMP
#      include <linux/unistd.h>
#      include <seccomp.h>
#      include <stdint.h>
#    elif HAVE_TAME
#      include <sys/tame.h>
#    endif
#  endif /* _WIN32 */
#endif /* ENABLE_EXTRA_SECURITY */


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Reduce the process integrity level to Low.
 * (Windows only; no-op on other platforms.)
 * @return 0 on success; negative POSIX error code on error.
 */
#if defined(_WIN32) && defined(ENABLE_EXTRA_SECURITY)
int rp_secure_reduce_integrity(void);
#else /* !_WIN32 */
static inline int rp_secure_reduce_integrity(void)
{
	return 0;
}
#endif /* _WIN32 */

/**
 * OS-specific security parameter.
 *
 * NOTE: This should be sizeof(void*) or less so it can be
 * passed by value.
 */
typedef struct _rp_secure_param_t {
#if defined(_WIN32)
	int bHighSec;		// High security mode
#elif defined(HAVE_SECCOMP)
	const int16_t *syscall_wl;	// Array of allowed syscalls. (-1 terminated)
	bool threading;			// Set to true to enable multi-threading.
#elif defined(HAVE_PLEDGE)
	const char *promises;	// pledge() promises
#elif defined(HAVE_TAME)
	int tame_flags;		// tame() flags
#else
#  ifdef ENABLE_SANDBOX
#    warning rp_secure_enable() not implemented for this OS
#  endif /* ENABLE_SANDBOX */
	int dummy;		// to prevent having an empty struct
#endif
} rp_secure_param_t;

/**
 * Enable OS-specific security functionality.
 * @param param OS-specific parameter.
 * @return 0 on success; negative POSIX error code on error.
 */
#if defined(ENABLE_EXTRA_SECURITY)
int rp_secure_enable(rp_secure_param_t param);
#else /* !ENABLE_EXTRA_SECURITY */
static inline int rp_secure_enable(rp_secure_param_t param)
{
	((void)param);
	return 0;
}
#endif /* ENABLE_EXTRA_SECURITY */

#ifdef __cplusplus
}
#endif
