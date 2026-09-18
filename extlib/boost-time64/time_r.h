/***************************************************************************
 * ROM Properties Page shell extension. (boost-time64)                     *
 * time_r.h: Workaround for missing time functions.                        *
 *                                                                         *
 * Copyright (c) 2017-2026 by David Korth.                                 *
 * SPDX-License-Identifier: BSL-1.0                                        *
 ***************************************************************************/

#pragma once

#include "config.boost-time64.h"

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

// Ensure we use 64-bit (or greater) time_t throughout rom-properties.
#if SIZEOF_TIME_T >= 8
// time_t is 64-bit (or greater).
typedef time_t rp_time_t;
#else /* SIZEOF_TIME_T < 8 */
// time_t is not 64-bit. Use a 64-bit int type for time64_t.
#  ifdef _MSC_VER
typedef __int64 rp_time_t;
#  else /* !_MSC_VER */
#    include <stdint.h>
typedef int64_t rp_time_t;
#  endif /* _MSC_VER */
#endif /* SIZEOF_TIME_T >= 8 */

#ifdef USING_INTERNAL_RP_GMTIME

// Use the internal rp_gmtime() function. (in boost-time64)
#ifdef __cplusplus
extern "C"
#endif
struct tm *rp_gmtime(const rp_time_t *timep, struct tm *result);

#else

static inline struct tm *rp_gmtime(const rp_time_t *timep, struct tm *result)
{
#if defined(HAVE_GMTIME_R)
	return gmtime_r(timep, result);
#elif defined(HAVE_GMTIME_S)
	// NOTE: gmtime_s() takes a non-const pointer, but it doesn't
	// actually modify the value.
	return (gmtime_s(result, (time_t*)timep) == 0) ? result : NULL;
#else
	// FIXME: Might not be thread-safe!
	// cppcheck-suppress gmtimeCalled
	struct tm *tm = gmtime(timep);
	if (tm && result) {
		*result = *tm;
		return result;
	}
	return NULL;
#endif
}

#endif /* USING_INTERNAL_RP_GMTIME */

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
 * MSVCRT's equivalent function is _mkgmtime64(). Note that it might
 * write to the original `struct tm`, so we'll need to make a copy.
 *
 * NOTE: timegm() is NOT part of *any* standard!
 */

#ifdef USING_INTERNAL_RP_TIMEGM

// Use the internal rp_gmtime() function. (in boost-time64)
// (boost's implementation of timegm() does not modify tm.)
#ifdef __cplusplus
extern "C"
#endif
rp_time_t rp_timegm(struct tm const *tm);

#else /* !USING_INTERNAL_RP_TIMEGM */

static inline rp_time_t rp_timegm(struct tm *tm)
{
	// NOTE: _mkgmtime64() and timegm() take a non-const pointer.
	// MSVC's _mkgmtime64() may modify struct tm if it has an invalid value.
	// glibc's timegm() makes a copy of tm.
#if defined(HAVE__MKGMTIME64)
#  define USING_MSVCRT__MKGMTIME64 1
	return _mkgmtime64(tm);
#elif defined(HAVE_TIMEGM)
	return timegm(tm);
#else
#  error timegm() or equivalent function not found.
#endif
}

#endif /* !USING_INTERNAL_RP_TIMEGM */
