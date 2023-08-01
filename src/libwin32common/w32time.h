/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * w32time.h: Windows time conversion functions.                           *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: https://support.microsoft.com/en-us/topic/bf03df72-96e4-59f3-1d02-b6781002dc7f

#pragma once

#include "RpWin32_sdk.h"
#include "librpbase/timeconv.h"

/**
 * Convert from Unix time to Win32 FILETIME.
 * @param unixtime Unix time
 * @param pFileTime FILETIME
 */
static inline void UnixTimeToFileTime(_In_ time_t unixtime, _Out_ FILETIME *pFileTime)
{
	LARGE_INTEGER li;
	li.QuadPart = UnixTimeToWindowsTime(unixtime);
	pFileTime->dwLowDateTime = li.LowPart;
	pFileTime->dwHighDateTime = (DWORD)li.HighPart;
}

/**
 * Convert from Unix time to Win32 SYSTEMTIME.
 * @param unixtime Unix time
 * @param pSystemTime Win32 SYSTEMTIME
 */
static inline void UnixTimeToSystemTime(_In_ time_t unixtime, _Out_ SYSTEMTIME *pSystemTime)
{
	FILETIME ft;
	UnixTimeToFileTime(unixtime, &ft);
	FileTimeToSystemTime(&ft, pSystemTime);
}

/**
 * Convert from Win32 FILETIME to Unix time.
 * @param pFileTime Win32 FILETIME
 * @return Unix time.
 */
static inline int64_t FileTimeToUnixTime(_In_ const FILETIME *pFileTime)
{
	LARGE_INTEGER li;
	li.LowPart = pFileTime->dwLowDateTime;
	li.HighPart = (LONG)pFileTime->dwHighDateTime;
	return WindowsTimeToUnixTime(li.QuadPart);
}

/**
 * Convert from Win32 SYSTEMTIME to Unix time.
 * @param pFileTime Win32 SYSTEMTIME
 * @return Unix time.
 */
static inline int64_t SystemTimeToUnixTime(_In_ const SYSTEMTIME *pSystemTime)
{
	FILETIME fileTime;
	SystemTimeToFileTime(pSystemTime, &fileTime);
	return FileTimeToUnixTime(&fileTime);
}
