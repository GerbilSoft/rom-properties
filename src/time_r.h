/***************************************************************************
 * ROM Properties Page shell extension.                                    *
 * time_r.h: Workaround for missing time functions.                        *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "config.libc.h"

// _POSIX_C_SOURCE is required for *_r() on MinGW-w64.
// However, this breaks snprintf() on FreeBSD when using clang/libc++,
// so only define it on Windows.
// Reference: https://github.com/pocoproject/poco/issues/1045#issuecomment-245987081
#ifdef _WIN32
#  ifndef _POSIX_C_SOURCE
#    define _POSIX_C_SOURCE 1
#  endif
#endif /* _WIN32 */
#include <time.h>

#ifndef HAVE_GMTIME_R
#  ifdef gmtime_r
     // Old MinGW-w64 (3.1.0, Ubuntu 14.04) has incompatible *_r() macros.
#    undef gmtime_r
#  endif
static inline struct tm *gmtime_r(const time_t *timep, struct tm *result)
{
#ifdef HAVE_GMTIME_S
	return (gmtime_s(result, timep) == 0 ? result : NULL);
#else /* !HAVE_GMTIME_S */
	// cppcheck-suppress gmtimeCalled
	struct tm *tm = gmtime(timep);
	if (tm && result) {
		*result = *tm;
		return result;
	}
	return NULL;
#endif /* GMTIME_S */
}
#endif /* HAVE_GMTIME_R */

#ifndef HAVE_LOCALTIME_R
#  ifdef localtime_r
     // Old MinGW-w64 (3.1.0, Ubuntu 14.04) has incompatible *_r() macros.
#    undef localtime_r
#  endif
static inline struct tm *localtime_r(const time_t *timep, struct tm *result)
{
#ifdef HAVE_LOCALTIME_S
	return (localtime_s(result, timep) == 0 ? result : NULL);
#else /* !HAVE_LOCALTIME_S */
	// cppcheck-suppress localtimeCalled
	struct tm *tm = localtime(timep);
	if (tm && result) {
		*result = *tm;
		return result;
	}
	return NULL;
#endif /* HAVE_LOCALTIME_S */
}
#endif /* HAVE_LOCALTIME_R */

/** timegm() **/

/**
 * Linux, Mac OS X, and other Unix-like operating systems have a
 * function timegm() that converts `struct tm` to `time_t`.
 *
 * MSVCRT's equivalent function is _mkgmtime(). Note that it might
 * write to the original `struct tm`, so we'll need to make a copy.
 *
 * NOTE: timegm() is NOT part of *any* standard!
 */
#if !defined(HAVE_TIMEGM)
static inline time_t timegm(struct tm *tm)
{
	struct tm my_tm;
	my_tm = *tm;
#if defined(HAVE__MKGMTIME64)
#  define USING_MSVCRT_MKGMTIME 1
	return _mkgmtime64(&my_tm);
#elif defined(HAVE__MKGMTIME32)
#  define USING_MSVCRT_MKGMTIME 1
	return _mkgmtime32(&my_tm);
#elif defined(HAVE__MKGMTIME)
#  define USING_MSVCRT_MKGMTIME 1
	return _mkgmtime(&my_tm);
#else
#  error timegm() or equivalent function not found.
#endif
}
#endif /* !HAVE_TIMEGM */
