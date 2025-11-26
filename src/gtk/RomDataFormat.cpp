/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * RomDataFormat.hpp: Common RomData string formatting functions.          *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RomDataFormat.hpp"

// for pango_version() and PANGO_VERSION_ENCODE()
#include <pango/pango.h>

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
string
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

/**
 * Format multi-line text for Achievements display using Pango markup.
 * Used for AchievementsTab and Achievements-like RFT_LISTDATA fields.
 * @param text Multi-line text
 * @return Text formatted for Achievements display using Pango markup.
 */
string
rom_data_format_RFT_LISTDATA_text_as_achievements(const char *text)
{
	string s_ret;

	// Escape the text for Pango markup.
	gchar *s_esc = g_markup_escape_text(text, -1);

	// Find the first newline.
	const char *nl = strchr(s_esc, '\n');
	if (!nl) {
		// No newline...
		// Return the escaped markup as-is.
		s_ret.assign(s_esc);
		g_free(s_esc);
		return s_ret;
	}

	s_ret.reserve(strlen(s_esc) + 32);
	s_ret.assign(s_esc, 0, nl - s_esc);

	// Pango 1.49.0 [2021/08/22] added percentage sizes.
	// For older versions, we'll need to use 'smaller' instead.
	// Note that compared to the KDE version, 'smaller' is slightly
	// big, and 'smaller'+'smaller' is too small.
	static const char *const span_start_line2 = (pango_version() >= PANGO_VERSION_ENCODE(1,49,0))
		? "\n<span size='75%'>"
		: "\n<span size='smaller'>";
	s_ret += span_start_line2;

	// Add the rest of the string.
	s_ret.append(nl + 1);

	g_free(s_esc);
	s_ret += "</span>";
	return s_ret;
}
