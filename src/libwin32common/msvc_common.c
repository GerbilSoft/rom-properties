/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * msvc_common.c: MSVC compatibility functions.                            *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
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
