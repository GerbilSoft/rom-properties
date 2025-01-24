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
