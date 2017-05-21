/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * msvc_common.h: MSVC compatibility functions.                            *
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
	time_t   tv_sec;	// seconds
	uint32_t tv_usec;	// microseconds
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
