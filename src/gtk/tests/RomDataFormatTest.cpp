/***************************************************************************
 * ROM Properties Page shell extension. (gtk/tests)                        *
 * RomDataFormatTest.cpp: RomDataFormat tests (glib)                       *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.librpbase.h"	// for TIME64_FOUND

// Google Test
#include "gtest/gtest.h"
#include "tcharx.h"

// RomFields (for RFT_DATETIME format flags)
#include "librpbase/RomFields.hpp"
using LibRpBase::RomFields;

// RomDataFormat
#include "RomDataFormat.hpp"

// C++ STL classes
using std::array;
using std::string;

namespace LibRomData { namespace Tests {

class RomDataFormatTest : public ::testing::Test
{};

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
		{0, nullptr},
		{-2, nullptr},
		{1, nullptr},
		{0x7FFFFFFF, nullptr},
#ifdef TIME64_FOUND
		{0x80000000, nullptr},
#endif /* TIME64_FOUND */
	}};

	for (const auto &test : dateTimeTestData) {
		gchar *str = nullptr;
		// FIXME: "\\d" isn't working...
		// FIXME: With "threadsafe", SIGABRT is caught and assert()'s message doesn't print.
		// FIXME: With "fast", the message *does* print, but the subprocess doesn't exit properly...
		//"RomDataFormat\\.cpp:[0-9]+: gchar\\* rom_data_format_datetime\\(time_t, unsigned int\\): Assertion `format\\[0\\] != '\\\\0'' failed\\."
		EXPECT_DEBUG_DEATH(str = rom_data_format_datetime(test.timestamp, flags), "");
		EXPECT_STREQ(test.str, str);
		g_free(str);
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
		{0, "01/01/70"},
		{-2, "12/31/69"},
		{1, "01/01/70"},
		{0x7FFFFFFF, "01/19/38"},
#ifdef TIME64_FOUND
		{0x80000000, "01/19/38"},
#endif /* TIME64_FOUND */
	}};

	for (const auto &test : dateTimeTestData) {
		gchar *const str = rom_data_format_datetime(test.timestamp, flags);
		EXPECT_STREQ(test.str, str);
		g_free(str);
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
#ifdef TIME64_FOUND
		{0x80000000, "03:14:08"},
#endif /* TIME64_FOUND */
	}};

	for (const auto &test : dateTimeTestData) {
		gchar *const str = rom_data_format_datetime(test.timestamp, flags);
		EXPECT_STREQ(test.str, str);
		g_free(str);
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
		{0, "01/01/70 00:00:00"},
		{-2, "12/31/69 23:59:58"},
		{1, "01/01/70 00:00:01"},
		{0x7FFFFFFF, "01/19/38 03:14:07"},
#ifdef TIME64_FOUND
		{0x80000000, "01/19/38 03:14:08"},
#endif /* TIME64_FOUND */
	}};

	for (const auto &test : dateTimeTestData) {
		gchar *const str = rom_data_format_datetime(test.timestamp, flags);
		EXPECT_STREQ(test.str, str);
		g_free(str);
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
		{0, nullptr},
		{-2, nullptr},
		{1, nullptr},
		{0x7FFFFFFF, nullptr},
#ifdef TIME64_FOUND
		{0x80000000, nullptr},
#endif /* TIME64_FOUND */
	}};

	for (const auto &test : dateTimeTestData) {
		gchar *str = nullptr;
		// FIXME: "\\d" isn't working...
		// FIXME: With "threadsafe", SIGABRT is caught and assert()'s message doesn't print.
		// FIXME: With "fast", the message *does* print, but the subprocess doesn't exit properly...
		//"RomDataFormat\\.cpp:[0-9]+: gchar\\* rom_data_format_datetime\\(time_t, unsigned int\\): Assertion `format\\[0\\] != '\\\\0'' failed\\."
		EXPECT_DEBUG_DEATH(str = rom_data_format_datetime(test.timestamp, flags), "");
		EXPECT_STREQ(test.str, str);
		g_free(str);
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
		{0, "Jan 01"},
		{-2, "Dec 31"},
		{1, "Jan 01"},
		{0x7FFFFFFF, "Jan 19"},
#ifdef TIME64_FOUND
		{0x80000000, "Jan 19"},
#endif /* TIME64_FOUND */
	}};

	for (const auto &test : dateTimeTestData) {
		gchar *const str = rom_data_format_datetime(test.timestamp, flags);
		EXPECT_STREQ(test.str, str);
		g_free(str);
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
#ifdef TIME64_FOUND
		{0x80000000, "03:14:08"},
#endif /* TIME64_FOUND */
	}};

	for (const auto &test : dateTimeTestData) {
		gchar *const str = rom_data_format_datetime(test.timestamp, flags);
		EXPECT_STREQ(test.str, str);
		g_free(str);
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
		{0, "Jan 01 00:00:00"},
		{-2, "Dec 31 23:59:58"},
		{1, "Jan 01 00:00:01"},
		{0x7FFFFFFF, "Jan 19 03:14:07"},
#ifdef TIME64_FOUND
		{0x80000000, "Jan 19 03:14:08"},
#endif /* TIME64_FOUND */
	}};

	for (const auto &test : dateTimeTestData) {
		gchar *const str = rom_data_format_datetime(test.timestamp, flags);
		EXPECT_STREQ(test.str, str);
		g_free(str);
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
		const string str = rom_data_format_dimensions(test.dimensions);
		EXPECT_EQ(test.str, str);
	}
}

} }

/**
 * Test suite main function.
 */
extern "C" int gtest_main(int argc, TCHAR *argv[])
{
	fputs("glib (GTK) UI frontend test suite: RomDataFormat tests.\n\n", stderr);
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
