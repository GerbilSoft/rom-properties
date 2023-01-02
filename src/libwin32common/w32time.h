/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * w32time.h: Windows time conversion functions.                           *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "RpWin32_sdk.h"

// C includes.
#include <stdint.h>

// Macros from MinGW-w64's gettimeofday.c.
#define FILETIME_1970 116444736000000000LL	// Seconds between 1/1/1601 and 1/1/1970.
#define HECTONANOSEC_PER_SEC 10000000LL

/**
 * Convert from Unix time to Win32 FILETIME.
 * @param unix_time Unix time.
 * @param pFileTime FILETIME.
 */
static inline void UnixTimeToFileTime(_In_ time_t unix_time, _Out_ FILETIME *pFileTime)
{
	// Reference: https://support.microsoft.com/en-us/kb/167296
	LARGE_INTEGER li;
	li.QuadPart = ((int64_t)unix_time * HECTONANOSEC_PER_SEC) + FILETIME_1970;
	pFileTime->dwLowDateTime = li.LowPart;
	pFileTime->dwHighDateTime = (DWORD)li.HighPart;
}

/**
 * Convert from Unix time to Win32 SYSTEMTIME.
 * @param unix_time Unix time.
 * @param pSystemTime Win32 SYSTEMTIME.
 */
static inline void UnixTimeToSystemTime(_In_ time_t unix_time, _Out_ SYSTEMTIME *pSystemTime)
{
	// Reference: https://support.microsoft.com/en-us/kb/167296
	FILETIME ft;
	UnixTimeToFileTime(unix_time, &ft);
	FileTimeToSystemTime(&ft, pSystemTime);
}

/**
 * Convert from Win32 FILETIME to Unix time.
 * @param pFileTime Win32 FILETIME.
 * @return Unix time.
 */
static inline int64_t FileTimeToUnixTime(_In_ const FILETIME *pFileTime)
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
static inline int64_t SystemTimeToUnixTime(_In_ const SYSTEMTIME *pSystemTime)
{
	FILETIME fileTime;
	SystemTimeToFileTime(pSystemTime, &fileTime);
	return FileTimeToUnixTime(&fileTime);
}
