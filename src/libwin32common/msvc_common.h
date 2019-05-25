/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * msvc_common.h: MSVC compatibility functions.                            *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBWIN32COMMON_MSVC_COMMON_H__
#define __ROMPROPERTIES_LIBWIN32COMMON_MSVC_COMMON_H__

#ifdef _MSC_VER

// MSVC doesn't have gettimeofday().
// NOTE: MSVC does have struct timeval in winsock.h,
// but it uses long, which is 32-bit.
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define timeval rp_timeval
struct timeval {
	time_t       tv_sec;	// seconds
	unsigned int tv_usec;	// microseconds
};

#define timezone rp_timezone
struct timezone {
	int tz_minuteswest;	// minutes west of Greenwich
	int tz_dsttime;		// type of DST correction
};

int gettimeofday(struct timeval *tv, struct timezone *tz);

#ifdef __cplusplus
}
#endif

#endif /* _MSC_VER */

#endif /* __ROMPROPERTIES_LIBWIN32COMMON_MSVC_COMMON_H__ */
