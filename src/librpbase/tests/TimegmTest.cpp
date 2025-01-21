/***************************************************************************
 * ROM Properties Page shell extension. (librpbase/tests)                  *
 * TimegmTest.cpp: timegm() test.                                          *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Google Test
#include "gtest/gtest.h"
#include "tcharx.h"

// timegm() and/or replacement function.
#include "time_r.h"

// libfmt
#include <fmt/core.h>
#include <fmt/format.h>
#define FSTR FMT_STRING

// NOTE: MSVCRT's documentation for _mkgmtime64() says it has a limited range:
// - Documented: [1970/01/01, 3000/12/31]
// - Actual:     [1969/01/01, 3001/01/01] ??? (may need more testing)

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

TEST(TimegmTest, unixEpochMinusTwoTest)
{
	struct tm tm_unix_epochMinusTwo = TM_INIT(1969, 12, 31, 23, 59, 58);
#ifdef USING_MSVCRT_MKGMTIME
	EXPECT_EQ(-2LL, timegm(&tm_unix_epochMinusTwo));
#else /* !USING_MSVCRT_MKGMTIME */
	EXPECT_EQ(-2LL, timegm(&tm_unix_epochMinusTwo));
#endif /* USING_MSVCRT_MKGMTIME */
}

TEST(TimegmTest, unixEpochMinusOneTest)
{
	struct tm tm_unix_epochMinusOne = TM_INIT(1969, 12, 31, 23, 59, 59);
	EXPECT_EQ(-1LL, timegm(&tm_unix_epochMinusOne));
}

TEST(TimegmTest, unixEpochTest)
{
	struct tm tm_unix_epoch = TM_INIT(1970, 1, 1, 0, 0, 0);
	EXPECT_EQ(0LL, timegm(&tm_unix_epoch));
}

TEST(TimegmTest, unix32bitMinMinusOneTest)
{
	struct tm tm_unix_32bit_minMinusOne = TM_INIT(1901, 12, 13, 20, 45, 51);
#ifdef USING_MSVCRT_MKGMTIME
	EXPECT_EQ(-1LL, timegm(&tm_unix_32bit_minMinusOne));
#else /* !USING_MSVCRT_MKGMTIME */
	if (sizeof(time_t) < 8) {
		EXPECT_EQ(-1LL, timegm(&tm_unix_32bit_minMinusOne));
	} else {
		EXPECT_EQ(-2147483649LL, timegm(&tm_unix_32bit_minMinusOne));
	}
#endif /* USING_MSVCRT_MKGMTIME */
}

TEST(TimegmTest, unix32bitMinTest)
{
	struct tm tm_unix_32bit_min = TM_INIT(1901, 12, 13, 20, 45, 52);
#ifdef USING_MSVCRT_MKGMTIME
	EXPECT_EQ(-1LL, timegm(&tm_unix_32bit_min));
#else /* !USING_MSVCRT_MKGMTIME */
	EXPECT_EQ(-2147483648LL, timegm(&tm_unix_32bit_min));
#endif /* USING_MSVCRT_MKGMTIME */
}

TEST(TimegmTest, unix32bitMaxTest)
{
	struct tm tm_unix_32bit_max = TM_INIT(2038, 1, 19, 3, 14, 7);
	EXPECT_EQ(2147483647LL, timegm(&tm_unix_32bit_max));
}

TEST(TimegmTest, unix32bitMaxPlusOne)
{
	struct tm tm_unix_32bit_maxPlusOne = TM_INIT(2038, 1, 19, 3, 14, 8);
	if (sizeof(time_t) < 8) {
		EXPECT_EQ(-1LL, timegm(&tm_unix_32bit_maxPlusOne));
	} else {
		EXPECT_EQ(2147483648LL, timegm(&tm_unix_32bit_maxPlusOne));
	}
}

TEST(TimegmTest, msdosEpochTest)
{
	struct tm tm_msdos_epoch = TM_INIT(1980, 1, 1, 0, 0, 0);
	EXPECT_EQ(315532800LL, timegm(&tm_msdos_epoch));
}

// FIXME: Broken on Mac OS X on travis-ci. (Returns -1 instead of the correct value.)
#ifndef __APPLE__
TEST(TimegmTest, winEpochTest)
{
	struct tm tm_win_epoch = TM_INIT(1601, 1, 1, 0, 0, 0);
#ifdef USING_MSVCRT_MKGMTIME
	EXPECT_EQ(-1LL, timegm(&tm_win_epoch));
#else /* !USING_MSVCRT_MKGMTIME */
	if (sizeof(time_t) < 8) {
		EXPECT_EQ(-1LL, timegm(&tm_win_epoch));
	} else {
		EXPECT_EQ(-11644473600LL, timegm(&tm_win_epoch));
	}
#endif /* USING_MSVCRT_MKGMTIME */
}
#endif /* __APPLE__ */

// TODO: Figure this out. (It seems to be correct for MSVC 2010, but not MSVC 2019.)
#if 0
TEST(TimegmTest, mkgmtime64RealMinTest)
{
	// Real minimum value for MSVCRT _mkgmtime64().
	struct tm tm_mkgmtime_realMin = TM_INIT(1969, 1, 1, 0, 0, 0);
	EXPECT_EQ(-31536000LL, timegm(&tm_mkgmtime_realMin));
}

TEST(TimegmTest, mkgmtime64RealMinMinusOneTest)
{
	// Real minimum value for MSVCRT _mkgmtime64(), minus one.
	struct tm tm_mkgmtime_realMinMinusOne = TM_INIT(1968, 12, 31, 23, 59, 59);
#ifdef USING_MSVCRT_MKGMTIME
	EXPECT_EQ(-1LL, timegm(&tm_mkgmtime_realMinMinusOne));
#else /* !USING_MSVCRT_MKGMTIME */
	if (sizeof(time_t) < 8) {
		EXPECT_EQ(-1LL, timegm(&tm_mkgmtime_realMinMinusOne));
	} else {
		EXPECT_EQ(-31536001LL, timegm(&tm_mkgmtime_realMinMinusOne));
	}
#endif /* USING_MSVCRT_MKGMTIME */
}
#endif

TEST(TimegmTest, mkgmtime64DocMaxTest)
{
	// Documented maximum value for MSVCRT _mkgmtime64().
	struct tm tm_mkgmtime_docMax = TM_INIT(3000, 12, 31, 23, 59, 59);
	if (sizeof(time_t) < 8) {
		EXPECT_EQ(-1LL, timegm(&tm_mkgmtime_docMax));
	} else {
		EXPECT_EQ(32535215999LL, timegm(&tm_mkgmtime_docMax));
	}
}

TEST(TimegmTest, mkgmtime64DocMaxPlusOneTest)
{
	// Documented maximum value for MSVCRT _mkgmtime64(), plus one.
	struct tm tm_mkgmtime_docMaxPlusOne = TM_INIT(3001, 1, 1, 0, 0, 0);
	if (sizeof(time_t) < 8) {
		EXPECT_EQ(-1LL, timegm(&tm_mkgmtime_docMaxPlusOne));
	} else {
		EXPECT_EQ(32535216000LL, timegm(&tm_mkgmtime_docMaxPlusOne));
	}
}

#if 0
TEST(TimegmTest, mkgmtime64RealMaxTest)
{
	// Real maximum value for MSVCRT _mkgmtime64().
	// FIXME: Figure this out. It's between [3001/01/01, 3001/01/02].
	struct tm tm_mkgmtime_realMax = TM_INIT(3001, 12, 31, 23, 59, 59);
	if (sizeof(time_t) < 8) {
		EXPECT_EQ(-1LL, timegm(&tm_mkgmtime_realMax));
	} else {
		EXPECT_EQ(32566751999LL, timegm(&tm_mkgmtime_realMax));
	}
}
#endif

TEST(TimegmTest, mkgmtime64RealMaxPlusOneTest)
{
	// Real maximum value for MSVCRT _mkgmtime64(), plus one.
	struct tm tm_mkgmtime_realMaxPlusOne = TM_INIT(3002, 1, 1, 0, 0, 0);
#ifdef USING_MSVCRT_MKGMTIME
	EXPECT_EQ(-1LL, timegm(&tm_mkgmtime_realMaxPlusOne));
#else /* !USING_MSVCRT_MKGMTIME */
	if (sizeof(time_t) < 8) {
		EXPECT_EQ(-1LL, timegm(&tm_mkgmtime_realMaxPlusOne));
	} else {
		EXPECT_EQ(32566752000LL, timegm(&tm_mkgmtime_realMaxPlusOne));
	}
#endif /* USING_MSVCRT_MKGMTIME */
}

TEST(TimegmTest, winMaxTimeTest)
{
	struct tm tm_win_maxTime = TM_INIT(30828, 9, 14, 2, 48, 5);
#ifdef USING_MSVCRT_MKGMTIME
	EXPECT_EQ(-1LL, timegm(&tm_win_maxTime));
#else /* !USING_MSVCRT_MKGMTIME */
	if (sizeof(time_t) < 8) {
		EXPECT_EQ(-1LL, timegm(&tm_win_maxTime));
	} else {
		EXPECT_EQ(910692730085LL, timegm(&tm_win_maxTime));
	}
#endif /* USING_MSVCRT_MKGMTIME */
}

TEST(TimegmTest, gcnEpochTest)
{
	struct tm tm_gcn_epoch = TM_INIT(2000, 1, 1, 0, 0, 0);
	EXPECT_EQ(0x386D4380LL, timegm(&tm_gcn_epoch));
}

} }

/**
 * Test suite main function.
 */
extern "C" int gtest_main(int argc, TCHAR *argv[])
{
#if defined(USING_MSVCRT_MKGMTIME)
# if defined(HAVE__MKGMTIME64)
	static constexpr char func_name[] = "_mkgmtime64() (MSVCRT)";
# elif defined(HAVE__MKGMTIME32)
	static constexpr char func_name[] = "_mkgmtime32() (MSVCRT)";
# else /*elif defined(HAVE__MKGMTIME)*/
	static constexpr char func_name[] = "_mkgmtime() (MSVCRT)";
# endif
#elif defined(HAVE_TIMEGM)
	static constexpr char func_name[] = "timegm() (libc)";
#else
	static constexpr char func_name[] = "timegm() (internal)";
#endif

	fmt::print(stderr, FSTR("LibRpBase test suite: timegm() tests.\n"));
	fmt::print(stderr, FSTR("Time conversion function in use: {:s}\n"), func_name);
	if (sizeof(time_t) < 8) {
		fmt::print(stderr,
			FSTR("*** WARNING: 32-bit time_t is in use.\n"
			     "*** Disabling tests known to fail with 32-bit time_t.\n"));
	}
	fmt::print(stderr, FSTR("\n"));
		
	fflush(nullptr);

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
