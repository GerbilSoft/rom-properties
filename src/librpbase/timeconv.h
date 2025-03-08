/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * timeconv.h: Conversion between Unix time and other formats.             *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: https://support.microsoft.com/en-us/topic/bf03df72-96e4-59f3-1d02-b6781002dc7f

#pragma once

// C includes
#include <stdint.h>
#include <time.h>

// Macros from MinGW-w64's gettimeofday.c.
#define FILETIME_1970 116444736000000000LL	// Seconds between 1/1/1601 and 1/1/1970.
#define HECTONANOSEC_PER_SEC 10000000LL

// const: Function does not have any effects except on the return value,
// and it only depends on the input parameters. (similar to constexpr)
#ifndef ATTR_CONST
#  ifdef __GNUC__
#    define ATTR_CONST __attribute__((const))
#  else /* !__GNUC__ */
#    define ATTR_CONST
#  endif /* __GNUC__ */
#endif /* ATTR_CONST */

// constexpr is not valid in C.
#ifndef CONSTEXPR
#  ifdef __cplusplus
#    define CONSTEXPR constexpr
#  else
#    define CONSTEXPR
#  endif /* !__cplusplus */
#endif /* !CONSTEXPR */

/**
 * Convert from Windows time to Unix time.
 * Windows time is hectonanoseconds since 1601/01/01 00:00:00 GMT.
 * Unix time is seconds since 1970/01/01 00:00:00 GMT.
 * @param wintime Windows time
 * @return Unix time
 */
ATTR_CONST
static inline CONSTEXPR time_t WindowsTimeToUnixTime(int64_t wintime)
{
	return (wintime - FILETIME_1970) / HECTONANOSEC_PER_SEC;
}

/**
 * Convert from Windows time (split format) to Unix time.
 * Windows time is hectonanoseconds since 1601/01/01 00:00:00 GMT.
 * Unix time is seconds since 1970/01/01 00:00:00 GMT.
 * @param wintime_hi Windows time, high DWORD
 * @param wintime_lo Windows time, low DWORD
 * @return Unix time
 */
ATTR_CONST
static inline CONSTEXPR time_t WindowsSplitTimeToUnixTime(uint32_t wintime_hi, uint32_t wintime_lo)
{
	return WindowsTimeToUnixTime(((int64_t)wintime_hi << 32U) | wintime_lo);
}

/**
 * Convert from Unix time to Windows time.
 * Unix time is seconds since 1970/01/01 00:00:00 GMT.
 * Windows time is hectonanoseconds since 1601/01/01 00:00:00 GMT.
 * @param wintime Windows time
 * @return Unix time
 */
ATTR_CONST
static inline CONSTEXPR int64_t UnixTimeToWindowsTime(time_t unixtime)
{
	return ((int64_t)unixtime * HECTONANOSEC_PER_SEC) + FILETIME_1970;
}
