/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * RomDataFormat.hpp: Common RomData string formatting functions.          *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RomDataFormat.hpp"

// librpbase
#include "librpbase/RomFields.hpp"
using LibRpBase::RomFields;

// C++ STL classes
using std::array;
using std::string;

/**
 * Format an RFT_DATETIME.
 * @param date_time	[in] Date/Time
 * @param flags		[in] RFT_DATETIME flags
 * @return Formatted RFT_DATETIME, or nullptr on error. (allocated string; free with g_free)
 */
gchar *
rom_data_format_datetime(time_t date_time, unsigned int flags)
{
	if (date_time == -1) {
		// Invalid date/time.
		return nullptr;
	}

	GDateTime *const dateTime = (flags & RomFields::RFT_DATETIME_IS_UTC)
		? g_date_time_new_from_unix_utc(date_time)
		: g_date_time_new_from_unix_local(date_time);
	assert(dateTime != nullptr);
	if (!dateTime) {
		// Unable to convert the timestamp.
		return nullptr;
	}

	static constexpr char formats_strtbl[] =
		"\0"		// [0] No date or time
		"%x\0"		// [1] Date
		"%X\0"		// [4] Time
		"%x %X\0"	// [7] Date Time

		// TODO: Better localization here.
		"\0"		// [13] No date or time
		"%b %d\0"	// [14] Date (no year)
		"%X\0"		// [20] Time
		"%b %d %X\0";	// [23] Date Time (no year)
	static constexpr array<uint8_t, 8> formats_offtbl = {{0, 1, 4, 7, 13, 14, 20, 23}};
	static_assert(sizeof(formats_strtbl) == 33, "formats_offtbl[] needs to be recalculated");

	const unsigned int offset = (flags & RomFields::RFT_DATETIME_HAS_DATETIME_NO_YEAR_MASK);
	const char *const format = &formats_strtbl[formats_offtbl[offset]];
	assert(format[0] != '\0');

	gchar *str = nullptr;
	if (format[0] != '\0') {
		str = g_date_time_format(dateTime, format);
	}

	g_date_time_unref(dateTime);
	return str;
}

/**
 * Format an RFT_DIMENSIONS.
 * @param dimensions	[in] Dimensions
 * @return Formatted RFT_DIMENSIONS, or nullptr on error.
 */
std::string
rom_data_format_dimensions(const int dimensions[3])
{
	string str;

	// TODO: 'x' or '×'? Using 'x' for now.
	if (dimensions[1] > 0) {
		if (dimensions[2] > 0) {
			str = fmt::format(FSTR("{:d}x{:d}x{:d}"),
				dimensions[0], dimensions[1], dimensions[2]);
		} else {
			str = fmt::format(FSTR("{:d}x{:d}"),
				dimensions[0], dimensions[1]);
		}
	} else {
		str = fmt::to_string(dimensions[0]);
	}

	return str;
}
