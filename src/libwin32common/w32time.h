/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * w32time.h: Windows time conversion functions.                           *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: https://support.microsoft.com/en-us/topic/bf03df72-96e4-59f3-1d02-b6781002dc7f

#pragma once

#include "RpWin32_sdk.h"
#include "librpbase/timeconv.h"

/**
 * Convert from Unix time to Win32 FILETIME.
 * @param unixtime Unix time
 * @return FILETIME
 */
static inline FILETIME UnixTimeToFileTime(_In_ rp_time_t unixtime)
{
	FILETIME ft;
	LARGE_INTEGER li;
	li.QuadPart = UnixTimeToWindowsTime(unixtime);
	ft.dwLowDateTime = li.LowPart;
	ft.dwHighDateTime = (DWORD)li.HighPart;
	return ft;
}

/**
 * Convert from Unix time to Win32 SYSTEMTIME.
 * @param unixtime Unix time
 * @return Win32 SYSTEMTIME
 */
static inline SYSTEMTIME UnixTimeToSystemTime(_In_ rp_time_t unixtime)
{
	SYSTEMTIME st;
	const FILETIME ft = UnixTimeToFileTime(unixtime);
	FileTimeToSystemTime(&ft, &st);
	return st;
}

/**
 * Convert from Win32 FILETIME to Unix time.
 * @param pFileTime Win32 FILETIME
 * @return Unix time.
 */
static inline int64_t FileTimeToUnixTime(_In_ FILETIME ft)
{
	LARGE_INTEGER li;
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = (LONG)ft.dwHighDateTime;
	return WindowsTimeToUnixTime(li.QuadPart);
}

/**
 * Convert from Win32 SYSTEMTIME to Unix time.
 * @param pFileTime Win32 SYSTEMTIME
 * @return Unix time.
 */
static inline int64_t SystemTimeToUnixTime(_In_ const SYSTEMTIME *pSystemTime)
{
	FILETIME ft;
	SystemTimeToFileTime(pSystemTime, &ft);
	return FileTimeToUnixTime(ft);
}
