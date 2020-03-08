/***************************************************************************
 * ROM Properties Page shell extension. (librpbase/tests)                  *
 * TimegmTest.cpp: timegm() test.                                          *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Google Test
#include "gtest/gtest.h"
#include "tcharx.h"

// timegm() and/or replacement function.
#include "time_r.h"

namespace LibRpBase { namespace Tests {

// `struct tm` initialization macro using ISO date format.
static inline struct tm TM_INIT(int year, int month, int day, int hour, int minute, int second)
{
	// NOTE: We can't simply use struct initialization because
	// BSD/GNU systems have extra tm_gmtoff and tm_zone fields.
	struct tm tm_ret;
	memset(&tm_ret, 0, sizeof(tm_ret));
	tm_ret.tm_sec = second;
	tm_ret.tm_min = minute;
	tm_ret.tm_hour = hour;
	tm_ret.tm_mday = day;
	tm_ret.tm_mon = month - 1;
	tm_ret.tm_year = year - 1900;
	return tm_ret;
}

TEST(TimegmTest, unixEpochTest)
{
	struct tm tm_unix_epoch = TM_INIT(1970, 1, 1, 0, 0, 0);
	EXPECT_EQ((time_t)0, timegm(&tm_unix_epoch));
}

TEST(TimegmTest, unix32bitMinMinusOneTest)
{
	struct tm tm_unix_32bit_minMinusOne = TM_INIT(1901, 12, 13, 20, 45, 51);
	EXPECT_EQ((time_t)-2147483649, timegm(&tm_unix_32bit_minMinusOne));
}

TEST(TimegmTest, unix32bitMinTest)
{
	struct tm tm_unix_32bit_min = TM_INIT(1901, 12, 13, 20, 45, 52);
	EXPECT_EQ((time_t)-2147483648, timegm(&tm_unix_32bit_min));
}

TEST(TimegmTest, unix32bitMaxTest)
{
	struct tm tm_unix_32bit_max = TM_INIT(2038, 1, 19, 3, 14, 7);
	EXPECT_EQ((time_t)2147483647, timegm(&tm_unix_32bit_max));
}

TEST(TimegmTest, unix32bitMaxPlusOne)
{
	struct tm tm_unix_32bit_maxPlusOne = TM_INIT(2038, 1, 19, 3, 14, 8);
	EXPECT_EQ((time_t)2147483648, timegm(&tm_unix_32bit_maxPlusOne));
}

TEST(TimegmTest, msdosEpochTest)
{
	struct tm tm_msdos_epoch = TM_INIT(1980, 1, 1, 0, 0, 0);
	EXPECT_EQ((time_t)315532800, timegm(&tm_msdos_epoch));
}

TEST(TimegmTest, winEpochTest)
{
	struct tm tm_win_epoch = TM_INIT(1601, 1, 1, 0, 0, 0);
	EXPECT_EQ((time_t)-11644473600, timegm(&tm_win_epoch));
}

TEST(TimegmTest, winMaxTimeTest)
{
	struct tm tm_win_maxTime = TM_INIT(30828, 9, 14, 2, 48, 5);
	EXPECT_EQ((time_t)910692730085, timegm(&tm_win_maxTime));
}

TEST(TimegmTest, gcnEpochTest)
{
	struct tm tm_gcn_epoch = TM_INIT(2000, 1, 1, 0, 0, 0);
	EXPECT_EQ((time_t)0x386D4380, timegm(&tm_gcn_epoch));
}

} }

/**
 * Test suite main function.
 */
extern "C" int gtest_main(int argc, TCHAR *argv[])
{
	fprintf(stderr, "LibRpBase test suite: timegm() tests.\n\n");
	fflush(nullptr);

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
