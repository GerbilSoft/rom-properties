/***************************************************************************
 * ROM Properties Page shell extension. (kde/tests)                        *
 * RomDataFormatTest.cpp: RomDataFormat tests                              *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Google Test
#include "gtest/gtest.h"
#include "tcharx.h"

// RomFields (for RFT_DATETIME format flags)
#include "librpbase/RomFields.hpp"
using LibRpBase::RomFields;

// RomDataFormat
#include "RomDataFormat.hpp"

// for RP_KDE_UPPER
#include "RpQtNS.hpp"

// C++ STL classes
using std::array;

namespace LibRomData { namespace Tests {

class RomDataFormatTest : public ::testing::Test
{
};

#ifndef NDEBUG
using RomDataFormatDeathTest = RomDataFormatTest;
#else
#define RomDataFormatDeathTest RomDataFormatTest
#endif

// NOTE: -1 is considered an invalid date/time by libromdata,
// so use -2 to test "before 1970/01/01 00:00:00".
struct DateTimeTestData {
	time_t timestamp;
	const char *str;	// using "C" locale
};

struct DimensionsTestData {
	int dimensions[3];
	const char *str;
};

// NOTE: To make debugging easier, EXPECT_STREQ() will be used with
// QString::toUtf8().constData(). This means that invalid values will
// return an empty string ("") instead of nullptr.

/**
 * formatDateTime format 0: Invalid format
 * glib version uses g_date_time_format().
 *
 * NOTE: In debug builds, this will call assert(),
 * which eventually calls abort().
 */
TEST_F(RomDataFormatDeathTest, formatDateTime_0_invalid)
{
	// UTC is used to prevent issues caused by differing timezones on build and test systems.
	static const unsigned int flags = RomFields::RFT_DATETIME_IS_UTC;

	static const array<DateTimeTestData, 5> dateTimeTestData = {{
		{0, ""},
		{-2, ""},
		{1, ""},
		{0x7FFFFFFF, ""},
		{0x80000000, ""},
	}};

	for (const auto &test : dateTimeTestData) {
		QString str;
		// FIXME: "\\d" isn't working...
		// FIXME: With "threadsafe", SIGABRT is caught and assert()'s message doesn't print.
		// FIXME: With "fast", the message *does* print, but the subprocess doesn't exit properly...
		//"RomDataFormat\\.cpp:[0-9]+: gchar\\* rom_data_format_datetime\\(time_t, unsigned int\\): Assertion `format\\[0\\] != '\\\\0'' failed\\."
		EXPECT_DEBUG_DEATH(str = formatDateTime(test.timestamp, flags), "");
		EXPECT_STREQ(test.str, str.toUtf8().constData());
	}
}

/**
 * formatDateTime format 1: Date only
 * glib version uses g_date_time_format().
 */
TEST_F(RomDataFormatTest, formatDateTime_1_dateOnly)
{
	// UTC is used to prevent issues caused by differing timezones on build and test systems.
	static const unsigned int flags = RomFields::RFT_DATETIME_IS_UTC |
	                                  RomFields::RFT_DATETIME_HAS_DATE;

	static const array<DateTimeTestData, 5> dateTimeTestData = {{
		{0, "1 Jan 1970"},
		{-2, "31 Dec 1969"},
		{1, "1 Jan 1970"},
		{0x7FFFFFFF, "19 Jan 2038"},
		{0x80000000, "19 Jan 2038"},
	}};

	for (const auto &test : dateTimeTestData) {
		QString str = formatDateTime(test.timestamp, flags);
		EXPECT_STREQ(test.str, str.toUtf8().constData());
	}
}

/**
 * formatDateTime format 2: Time only
 * glib version uses g_date_time_format().
 */
TEST_F(RomDataFormatTest, formatDateTime_2_timeOnly)
{
	// UTC is used to prevent issues caused by differing timezones on build and test systems.
	static const unsigned int flags = RomFields::RFT_DATETIME_IS_UTC |
	                                  RomFields::RFT_DATETIME_HAS_TIME;

	static const array<DateTimeTestData, 5> dateTimeTestData = {{
		{0, "00:00:00"},
		{-2, "23:59:58"},
		{1, "00:00:01"},
		{0x7FFFFFFF, "03:14:07"},
		{0x80000000, "03:14:08"},
	}};

	for (const auto &test : dateTimeTestData) {
		QString str = formatDateTime(test.timestamp, flags);
		EXPECT_STREQ(test.str, str.toUtf8().constData());
	}
}

/**
 * formatDateTime format 3: Date and time
 * glib version uses g_date_time_format().
 */
TEST_F(RomDataFormatTest, formatDateTime_3_dateAndTime)
{
	// UTC is used to prevent issues caused by differing timezones on build and test systems.
	static const unsigned int flags = RomFields::RFT_DATETIME_IS_UTC |
	                                  RomFields::RFT_DATETIME_HAS_DATE |
	                                  RomFields::RFT_DATETIME_HAS_TIME;

	static const array<DateTimeTestData, 5> dateTimeTestData = {{
		{0, "1 Jan 1970 00:00:00"},
		{-2, "31 Dec 1969 23:59:58"},
		{1, "1 Jan 1970 00:00:01"},
		{0x7FFFFFFF, "19 Jan 2038 03:14:07"},
		{0x80000000, "19 Jan 2038 03:14:08"},
	}};

	for (const auto &test : dateTimeTestData) {
		QString str = formatDateTime(test.timestamp, flags);
		EXPECT_STREQ(test.str, str.toUtf8().constData());
	}
}

/**
 * formatDateTime format 4: Invalid format
 * glib version uses g_date_time_format().
 *
 * NOTE: In debug builds, this will call assert(),
 * which eventually calls abort().
 */
TEST_F(RomDataFormatDeathTest, formatDateTime_4_invalid)
{
	// UTC is used to prevent issues caused by differing timezones on build and test systems.
	static const unsigned int flags = RomFields::RFT_DATETIME_IS_UTC |
	                                  RomFields::RFT_DATETIME_NO_YEAR;

	static const array<DateTimeTestData, 5> dateTimeTestData = {{
		{0, ""},
		{-2, ""},
		{1, ""},
		{0x7FFFFFFF, ""},
		{0x80000000, ""},
	}};

	for (const auto &test : dateTimeTestData) {
		QString str;
		// FIXME: "\\d" isn't working...
		// FIXME: With "threadsafe", SIGABRT is caught and assert()'s message doesn't print.
		// FIXME: With "fast", the message *does* print, but the subprocess doesn't exit properly...
		//"RomDataFormat\\.cpp:[0-9]+: gchar\\* rom_data_format_datetime\\(time_t, unsigned int\\): Assertion `format\\[0\\] != '\\\\0'' failed\\."
		EXPECT_DEBUG_DEATH(str = formatDateTime(test.timestamp, flags), "");
		EXPECT_STREQ(test.str, str.toUtf8().constData());
	}
}

/**
 * formatDateTime format 5: Date only (no year)
 * glib version uses g_date_time_format().
 */
TEST_F(RomDataFormatTest, formatDateTime_5_dateOnly)
{
	// UTC is used to prevent issues caused by differing timezones on build and test systems.
	static const unsigned int flags = RomFields::RFT_DATETIME_IS_UTC |
	                                  RomFields::RFT_DATETIME_NO_YEAR |
	                                  RomFields::RFT_DATETIME_HAS_DATE;

	static const array<DateTimeTestData, 5> dateTimeTestData = {{
		{0, "Jan 1"},
		{-2, "Dec 31"},
		{1, "Jan 1"},
		{0x7FFFFFFF, "Jan 19"},
		{0x80000000, "Jan 19"},
	}};

	for (const auto &test : dateTimeTestData) {
		QString str = formatDateTime(test.timestamp, flags);
		EXPECT_STREQ(test.str, str.toUtf8().constData());
	}
}

/**
 * formatDateTime format 6: Time only (no year) [technically redundant...]
 * glib version uses g_date_time_format().
 */
TEST_F(RomDataFormatTest, formatDateTime_6_timeOnly_noYear)
{
	// UTC is used to prevent issues caused by differing timezones on build and test systems.
	static const unsigned int flags = RomFields::RFT_DATETIME_IS_UTC |
	                                  RomFields::RFT_DATETIME_NO_YEAR |
	                                  RomFields::RFT_DATETIME_HAS_TIME;

	static const array<DateTimeTestData, 5> dateTimeTestData = {{
		{0, "00:00:00"},
		{-2, "23:59:58"},
		{1, "00:00:01"},
		{0x7FFFFFFF, "03:14:07"},
		{0x80000000, "03:14:08"},
	}};

	for (const auto &test : dateTimeTestData) {
		QString str = formatDateTime(test.timestamp, flags);
		EXPECT_STREQ(test.str, str.toUtf8().constData());
	}
}

/**
 * formatDateTime format 7: Date and time (no year)
 * glib version uses g_date_time_format().
 */
TEST_F(RomDataFormatTest, formatDateTime_7_dateAndTime_noYear)
{
	// UTC is used to prevent issues caused by differing timezones on build and test systems.
	static const unsigned int flags = RomFields::RFT_DATETIME_IS_UTC |
	                                  RomFields::RFT_DATETIME_NO_YEAR |
	                                  RomFields::RFT_DATETIME_HAS_DATE |
	                                  RomFields::RFT_DATETIME_HAS_TIME;

	static const array<DateTimeTestData, 5> dateTimeTestData = {{
		{0, "Jan 1 00:00:00"},
		{-2, "Dec 31 23:59:58"},
		{1, "Jan 1 00:00:01"},
		{0x7FFFFFFF, "Jan 19 03:14:07"},
		{0x80000000, "Jan 19 03:14:08"},
	}};

	for (const auto &test : dateTimeTestData) {
		QString str = formatDateTime(test.timestamp, flags);
		EXPECT_STREQ(test.str, str.toUtf8().constData());
	}
}

/**
 * formatDimensions test
 */
TEST_F(RomDataFormatTest, formatDimensions)
{
	static const array<DimensionsTestData, 10> dimensionsTestData = {{
		{{0, 0, 0}, "0"},
		{{1, 0, 0}, "1"},
		{{32, 0, 0}, "32"},
		{{1048576, 0, 0}, "1048576"},

		{{1, 1, 0}, "1x1"},
		{{32, 24, 0}, "32x24"},
		{{1048576, 524288, 0}, "1048576x524288"},

		{{1, 1, 1}, "1x1x1"},
		{{32, 24, 16}, "32x24x16"},
		{{1048576, 524288, 262144}, "1048576x524288x262144"},
	}};

	for (const auto &test : dimensionsTestData) {
		QString str = formatDimensions(test.dimensions);
		EXPECT_STREQ(test.str, str.toUtf8().constData());
	}
}

} }

/**
 * Test suite main function.
 */
extern "C" int gtest_main(int argc, TCHAR *argv[])
{
	fputs("KDE (" RP_KDE_UPPER ") UI frontend test suite: RomDataFormat tests.\n\n", stderr);
	fflush(nullptr);

	// Use "threadsafe" tests on Linux for proper assert() handling in death tests.
	// References:
	// - https://stackoverflow.com/questions/71251067/gtest-expect-death-fails-with-sigabrt
	// - https://stackoverflow.com/a/71257678
	GTEST_FLAG_SET(death_test_style, "threadsafe");

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
