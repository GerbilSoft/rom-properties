/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * time_r.h: Workaround for missing reentrant time functions.              *
 *                                                                         *
 * Copyright (c) 2017-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_TIME_R_H__
#define __ROMPROPERTIES_LIBRPBASE_TIME_R_H__

#include "config.librpbase.h"

// _POSIX_C_SOURCE is required for *_r() on MinGW-w64.
// However, this breaks snprintf() on FreeBSD when using clang/libc++,
// so only define it on Windows.
// Reference: https://github.com/pocoproject/poco/issues/1045#issuecomment-245987081
#ifdef _WIN32
# ifndef _POSIX_C_SOURCE
#  define _POSIX_C_SOURCE 1
# endif
#endif /* _WIN32 */
#include <time.h>

#ifndef HAVE_GMTIME_R
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
#if !defined(HAVE_TIMEGM) && (defined(HAVE__MKGMTIME64) || defined(HAVE__MKGMTIME))
static inline time_t timegm(struct tm *tm)
{
	struct tm my_tm;
	my_tm = *tm;
#if defined(HAVE__MKGMTIME64)
# define USING_MSVCRT_MKGMTIME 1
	return _mkgmtime64(&my_tm);
#elif defined(HAVE__MKGMTIME)
# define USING_MSVCRT_MKGMTIME 1
	return _mkgmtime(&my_tm);
#endif
}
#elif !defined(HAVE_TIMEGM)
// timegm() or equivalent is not available.
// Use the version in timegm.c.
#ifdef __cplusplus
extern "C"
#endif /* __cplusplus */
time_t timegm(const struct tm *t);
#endif /* HAVE_TIMEGM */

#endif /* __ROMPROPERTIES_LIBRPBASE_TIME_R_H__ */
