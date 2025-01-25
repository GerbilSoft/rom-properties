/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RomDataFormat.hpp: Common RomData string formatting functions.          *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RomDataFormat.hpp"

// librpbase
#include "librpbase/RomFields.hpp"
using LibRpBase::RomFields;

// C++ STL classes
using std::tstring;

// Windows 10 and later
#ifndef DATE_MONTHDAY
#  define DATE_MONTHDAY 0x00000080
#endif /* DATE_MONTHDAY */

/**
 * Format an RFT_DATETIME.
 * @param date_time	[in] Date/Time
 * @param flags		[in] RFT_DATETIME flags
 * @return Formatted RFT_DATETIME, or empty on error.
 */
tstring formatDateTime(time_t date_time, unsigned int flags)
{
	// Format the date/time using the system locale.
	tstring tstr;
	TCHAR buf[128];

	// Convert from Unix time to Win32 SYSTEMTIME.
	SYSTEMTIME st;
	UnixTimeToSystemTime(date_time, &st);

	// At least one of Date and/or Time must be set.
	assert((flags &
		(RomFields::RFT_DATETIME_HAS_DATE | RomFields::RFT_DATETIME_HAS_TIME)) != 0);

	if (!(flags & RomFields::RFT_DATETIME_IS_UTC)) {
		// Convert to the current timezone.
		const SYSTEMTIME st_utc = st;
		BOOL ret = SystemTimeToTzSpecificLocalTime(nullptr, &st_utc, &st);
		if (!ret) {
			// Conversion failed.
			return {};
		}
	}

	if (flags & RomFields::RFT_DATETIME_HAS_DATE) {
		// Format the date.
		int ret;
		if (flags & RomFields::RFT_DATETIME_NO_YEAR) {
			// Try Windows 10's DATE_MONTHDAY first.
			ret = GetDateFormat(
				MAKELCID(LOCALE_USER_DEFAULT, SORT_DEFAULT),
				DATE_MONTHDAY,
				&st, nullptr, buf, _countof(buf));
			if (!ret) {
				// DATE_MONTHDAY failed.
				// Fall back to a hard-coded format string.
				// TODO: Localization.
				ret = GetDateFormat(
					MAKELCID(LOCALE_USER_DEFAULT, SORT_DEFAULT),
					0, &st, _T("MMM d"), buf, _countof(buf));
			}
		} else {
			ret = GetDateFormat(
				MAKELCID(LOCALE_USER_DEFAULT, SORT_DEFAULT),
				DATE_SHORTDATE,
				&st, nullptr, buf, _countof(buf));
		}
		if (!ret) {
			// Error!
			return {};
		}

		// Add to the tstring.
		tstr += buf;
	}

	if (flags & RomFields::RFT_DATETIME_HAS_TIME) {
		// Format the time.
		if (!tstr.empty()) {
			// Add a space.
			tstr += _T(' ');
		}

		int ret = GetTimeFormat(
			MAKELCID(LOCALE_USER_DEFAULT, SORT_DEFAULT),
			0, &st, nullptr, buf, _countof(buf));
		if (!ret) {
			// Error!
			return 0;
		}

		// Add to the tstring.
		tstr += buf;
	}

	return tstr;
}

/**
 * Format an RFT_DIMENSIONS.
 * @param dimensions	[in] Dimensions
 * @return Formatted RFT_DIMENSIONS, or nullptr on error. (allocated string; free with g_free)
 */
tstring formatDimensions(const int dimensions[3])
{
	// TODO: 'x' or '×'? Using 'x' for now.
	tstring tstr;

	if (dimensions[1] > 0) {
		if (dimensions[2] > 0) {
			tstr = fmt::format(FSTR(_T("{:d}x{:d}x{:d}")),
				dimensions[0], dimensions[1], dimensions[2]);
		} else {
			tstr = fmt::format(FSTR(_T("{:d}x{:d}")),
				dimensions[0], dimensions[1]);
		}
	} else {
		// FIXME: fmt::to_tstring()?
		tstr = fmt::to_wstring(dimensions[0]);
	}

	return tstr;
}
