/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * w32time.h: Windows time conversion functions.                           *
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

#ifndef __ROMPROPERTIES_LIBWIN32COMMON_WINTIME_H__
#define __ROMPROPERTIES_LIBWIN32COMMON_WINTIME_H__

#include "RpWin32_sdk.h"

// Macros from MinGW-w64's gettimeofday.c.
#define FILETIME_1970 116444736000000000LL	// Seconds between 1/1/1601 and 1/1/1970.
#define HECTONANOSEC_PER_SEC 10000000LL

#if defined(_MSC_VER) && !defined(__cplusplus)
#define TIME_INLINE __inline
#else
#define TIME_INLINE inline
#endif

/**
 * Convert from Unix time to Win32 SYSTEMTIME.
 * @param unix_time Unix time.
 * @param pSystemTime Win32 SYSTEMTIME.
 */
static TIME_INLINE void UnixTimeToSystemTime(int64_t unix_time, SYSTEMTIME *pSystemTime)
{
	LARGE_INTEGER li;
	FILETIME ft;

	// Reference: https://support.microsoft.com/en-us/kb/167296
	li.QuadPart = (unix_time * HECTONANOSEC_PER_SEC) + FILETIME_1970;
	ft.dwLowDateTime = li.LowPart;
	ft.dwHighDateTime = (DWORD)li.HighPart;
	FileTimeToSystemTime(&ft, pSystemTime);
}

/**
 * Convert from Win32 FILETIME to Unix time.
 * @param pFileTime Win32 FILETIME.
 * @return Unix time.
 */
static TIME_INLINE int64_t FileTimeToUnixTime(const FILETIME *pFileTime)
{
	// Reference: https://support.microsoft.com/en-us/kb/167296
	LARGE_INTEGER li;
	li.LowPart = pFileTime->dwLowDateTime;
	li.HighPart = (LONG)pFileTime->dwHighDateTime;
	return (li.QuadPart - FILETIME_1970) / HECTONANOSEC_PER_SEC;
}

/**
 * Convert from Win32 SYSTEMTIME to Unix time.
 * @param pFileTime Win32 SYSTEMTIME.
 * @return Unix time.
 */
static TIME_INLINE int64_t SystemTimeToUnixTime(const SYSTEMTIME *pSystemTime)
{
	FILETIME fileTime;
	SystemTimeToFileTime(pSystemTime, &fileTime);
	return FileTimeToUnixTime(&fileTime);
}

#endif /* __ROMPROPERTIES_LIBWIN32COMMON_WINTIME_H__ */
