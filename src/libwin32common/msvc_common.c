/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * msvc_common.c: MSVC compatibility functions.                            *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef _MSC_VER
#error msvc_common.c should only be compiled in MSVC builds.
#endif /* _MSC_VER */

#include "msvc_common.h"
#include "w32time.h"

// MSVC doesn't have struct timeval or gettimeofday().
int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	union {
		uint64_t ns100; // Time since 1/1/1601 in 100ns units.
		FILETIME ft;
	} _now;
	TIME_ZONE_INFORMATION TimeZoneInformation;
	DWORD tzi;

	if (tz) {
		// Get the timezone information.
		if ((tzi = GetTimeZoneInformation(&TimeZoneInformation)) != TIME_ZONE_ID_INVALID) {
			tz->tz_minuteswest = TimeZoneInformation.Bias;
			if (tzi == TIME_ZONE_ID_DAYLIGHT) {
				tz->tz_dsttime = 1;
			} else {
				tz->tz_dsttime = 0;
			}
		} else {
			tz->tz_minuteswest = 0;
			tz->tz_dsttime = 0;
		}
	}

	if (tv) {
		GetSystemTimeAsFileTime(&_now.ft);      // 100ns units since 1/1/1601
		// Windows XP's accuracy seems to be ~125,000ns == 125us == 0.125ms
		_now.ns100 -= FILETIME_1970;
		tv->tv_sec = _now.ns100 / HECTONANOSEC_PER_SEC;                 // seconds since 1/1/1970
		tv->tv_usec = (_now.ns100 % HECTONANOSEC_PER_SEC) / 10LL;       // microseconds
	}

	return 0;
}
