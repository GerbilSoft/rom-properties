/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * FilterCacheKeyTest.cpp: CacheManager::filterCacheKey() test.            *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

// Google Test
#include "gtest/gtest.h"

// libromdata
#include "librpbase/TextFuncs.hpp"
using LibRpBase::rp_string;

// Cache Manager
#include "../CacheManager.hpp"

namespace LibCacheMgr { namespace Tests {

struct FilterCacheKeyTest_mode
{
	// Cache keys.
	const rp_char *keyOrig;			// Original key.
	const rp_char *keyFilteredPosix;	// Filtered key. (POSIX)
	const rp_char *keyFilteredWin32;	// Filtered key. (Win32)

	FilterCacheKeyTest_mode(
		const rp_char *keyOrig,
		const rp_char *keyFilteredPosix,
		const rp_char *keyFilteredWin32)
		: keyOrig(keyOrig)
		, keyFilteredPosix(keyFilteredPosix)
		, keyFilteredWin32(keyFilteredWin32)
	{ }

	// May be required for MSVC 2010?
	FilterCacheKeyTest_mode(const FilterCacheKeyTest_mode &other)
		: keyOrig(other.keyOrig)
		, keyFilteredPosix(other.keyFilteredPosix)
		, keyFilteredWin32(other.keyFilteredWin32)
	{ }

	// Required for MSVC 2010.
	FilterCacheKeyTest_mode &operator=(const FilterCacheKeyTest_mode &other)
	{
		keyOrig = other.keyOrig;
		keyFilteredPosix = other.keyFilteredPosix;
		keyFilteredWin32 = other.keyFilteredWin32;
		return *this;
	}
};

class FilterCacheKeyTest : public ::testing::TestWithParam<FilterCacheKeyTest_mode>
{
	protected:
		FilterCacheKeyTest() { }
};

/**
 * Run a CacheManager::filterCacheKey() test.
 */
TEST_P(FilterCacheKeyTest, filterCacheKey)
{
	const FilterCacheKeyTest_mode &mode = GetParam();

	rp_string keyFiltered = CacheManager::filterCacheKey(mode.keyOrig);
#ifdef _WIN32
	EXPECT_EQ(mode.keyFilteredWin32, keyFiltered);
#else /* !_WIN32 */
	EXPECT_EQ(mode.keyFilteredPosix, keyFiltered);
#endif /* _WIN32 */
}

// TODO: Add more test cases.
INSTANTIATE_TEST_CASE_P(CacheManagerTest, FilterCacheKeyTest,
	::testing::Values(
		// Known-good cache key.
		FilterCacheKeyTest_mode(
			_RP("wii/disc/US/GALE01.png"),
			_RP("wii/disc/US/GALE01.png"),
			_RP("wii\\disc\\US\\GALE01.png")),

		// Simple ".." traversal.
		FilterCacheKeyTest_mode(
			_RP("../../../../etc/passwd"),
			_RP(""),
			_RP("")),

		// "..." traversal, which isn't actually traversal,
		// but is filtered out anyway.
		FilterCacheKeyTest_mode(
			_RP(".../.../.../.../etc/passwd"),
			_RP(""),
			_RP("")),

		// Unix-style absolute path. (blocked due to leading '/')
		FilterCacheKeyTest_mode(
			_RP("/etc/passwd"),
			_RP(""),
			_RP("")),

		// Windows-style absolute path. (blocked due to ':')
		FilterCacheKeyTest_mode(
			_RP("C:/Windows/System32/config/SAM"),
			_RP(""),
			_RP("")),

		// Filter out bad characters.
		// These characters are converted to '_', unlike '\\' and ':',
		// which abort processing and return an empty string.
		FilterCacheKeyTest_mode(
			_RP("lol/\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\"*<>?|_!"),
			_RP("lol/_______________________________ _______!"),
			_RP("lol\\_______________________________ _______!"))
	));
} }

/**
 * Test suite main function.
 */
extern "C" int gtest_main(int argc, char *argv[])
{
	fprintf(stderr, "LibCacheMgr test suite: CacheManager::filterCacheKey() tests.\n\n");
	fflush(nullptr);

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
