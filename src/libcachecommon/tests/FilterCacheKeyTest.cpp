/***************************************************************************
 * ROM Properties Page shell extension. (libcachecommon/tests)             *
 * FilterCacheKeyTest.cpp: CacheManager::filterCacheKey() test.            *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Google Test
#include "gtest_init.hpp"

// libcachecommon
#include "../CacheKeys.hpp"

#ifdef RP_WIS16
#  include "librptext/wchar.hpp"
#endif /* RP_WIS16 */

// C includes (C++ namespace)
#include <cerrno>
#include <cstdio>

// C++ includes
#include <string>
using std::string;
#ifdef RP_WIS16
using std::wstring;
#endif /* RP_WIS16 */

namespace LibCacheCommon { namespace Tests {

struct FilterCacheKeyTest_mode
{
	// Cache keys
	const char *keyOrig;		// Original key
	const char *keyFilteredPosix;	// Filtered key (POSIX)
	const char *keyFilteredWin32;	// Filtered key (Win32)
	bool canTestAsUTF16;		// Overlong UTF-8 sequences can't be tested as UTF-16

	FilterCacheKeyTest_mode(
		bool canTestAsUTF16,
		const char *keyOrig,
		const char *keyFilteredPosix,
		const char *keyFilteredWin32)
		: keyOrig(keyOrig)
		, keyFilteredPosix(keyFilteredPosix)
		, keyFilteredWin32(keyFilteredWin32)
		, canTestAsUTF16(canTestAsUTF16)
	{ }

	// May be required for MSVC 2010?
	FilterCacheKeyTest_mode(const FilterCacheKeyTest_mode &other)
		: keyOrig(other.keyOrig)
		, keyFilteredPosix(other.keyFilteredPosix)
		, keyFilteredWin32(other.keyFilteredWin32)
		, canTestAsUTF16(other.canTestAsUTF16)
	{ }

	// Required for MSVC 2010.
	FilterCacheKeyTest_mode &operator=(const FilterCacheKeyTest_mode &other)
	{
		if (this != &other) {
			keyOrig = other.keyOrig;
			keyFilteredPosix = other.keyFilteredPosix;
			keyFilteredWin32 = other.keyFilteredWin32;
			canTestAsUTF16 = other.canTestAsUTF16;
		}
		return *this;
	}
};

class FilterCacheKeyTest : public ::testing::TestWithParam<FilterCacheKeyTest_mode>
{
	protected:
		FilterCacheKeyTest() = default;
};

/**
 * Run a CacheManager::filterCacheKey() test.
 */
TEST_P(FilterCacheKeyTest, filterCacheKey)
{
	const FilterCacheKeyTest_mode &mode = GetParam();

	string keyFiltered = mode.keyOrig;
	int ret = LibCacheCommon::filterCacheKey(keyFiltered);

	// If it starts with certain invalid characters, we should expect -EINVAL.
	// Otherwise, we'll expect 0.
	if (mode.keyOrig[0] == '/' ||
	    mode.keyOrig[0] == '\\' ||
	    mode.keyOrig[0] == '.' ||
	    mode.keyOrig[1] == ':')
	{
		EXPECT_EQ(-EINVAL, ret);
		return;
	}

	// Expecting success.
	EXPECT_EQ(0, ret);
	if (ret != 0) {
		return;
	}

#ifdef _WIN32
	EXPECT_EQ(mode.keyFilteredWin32, keyFiltered);
#else /* !_WIN32 */
	EXPECT_EQ(mode.keyFilteredPosix, keyFiltered);
#endif /* _WIN32 */
}

#ifdef RP_WIS16
/**
 * Run a CacheManager::filterCacheKey() test.
 * wchar_t version; converts UTF-8 strings to UTF-16 prior to testing.
 */
TEST_P(FilterCacheKeyTest, filterCacheKey_wchar_t)
{
	const FilterCacheKeyTest_mode &mode = GetParam();
	if (!mode.canTestAsUTF16)
		return;

	wstring wkeyFiltered = U82W_s(mode.keyOrig);
	int ret = LibCacheCommon::filterCacheKey(wkeyFiltered);

	// If it starts with certain invalid characters, we should expect -EINVAL.
	// Otherwise, we'll expect 0.
	if (mode.keyOrig[0] == '/' ||
	    mode.keyOrig[0] == '\\' ||
	    mode.keyOrig[0] == '.' ||
	    mode.keyOrig[1] == ':')
	{
		EXPECT_EQ(-EINVAL, ret);
		return;
	}

	// Expecting success.
	EXPECT_EQ(0, ret);
	if (ret != 0) {
		return;
	}

	const string keyFiltered = W2U8(wkeyFiltered);
#ifdef _WIN32
	EXPECT_EQ(mode.keyFilteredWin32, keyFiltered);
#else /* !_WIN32 */
	EXPECT_EQ(mode.keyFilteredPosix, keyFiltered);
#endif /* _WIN32 */
}
#endif /* RP_WIS16 */

// TODO: Add more test cases.
INSTANTIATE_TEST_SUITE_P(CacheManagerTest, FilterCacheKeyTest,
	::testing::Values(
		// Known-good cache key.
		FilterCacheKeyTest_mode(true,
			"wii/disc/US/GALE01.png",
			"wii/disc/US/GALE01.png",
			"wii\\disc\\US\\GALE01.png"),

		// Simple ".." traversal.
		FilterCacheKeyTest_mode(true,
			"../../../../etc/passwd",
			"",
			""),

		// "..." traversal, which isn't actually traversal,
		// but is filtered out anyway.
		FilterCacheKeyTest_mode(true,
			".../.../.../.../etc/passwd",
			"",
			""),

		// Unix-style absolute path. (blocked due to leading '/')
		FilterCacheKeyTest_mode(true,
			"/etc/passwd",
			"",
			""),

		// Windows-style absolute path. (blocked due to ':')
		FilterCacheKeyTest_mode(true,
			"C:/Windows/System32/config/SAM",
			"",
			""),

		// Filter out bad characters.
		// These characters are converted to '_', unlike '\\' and ':',
		// which abort processing and return an empty string.
		FilterCacheKeyTest_mode(true,
			"lol/\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\"*<>?|_!",
			"lol/_______________________________ _______!",
			"lol\\_______________________________ _______!"),

		// Allow UTF-8 characters.
		FilterCacheKeyTest_mode(true,
			"\xC2\xA9\xC2\xAE\xE2\x99\xAA\xE2\x98\x83\xF0\x9F\x92\xBE",
			"\xC2\xA9\xC2\xAE\xE2\x99\xAA\xE2\x98\x83\xF0\x9F\x92\xBE",
			"\xC2\xA9\xC2\xAE\xE2\x99\xAA\xE2\x98\x83\xF0\x9F\x92\xBE"),

		// Allow UTF-8 characters while filtering bad ASCII characters.
		FilterCacheKeyTest_mode(true,
			"\xC2\xA9\xC2\xAE\xE2\x99\xAA\xE2\x98\x83\xF0\x9F\x92\xBE\x01\x02",
			"\xC2\xA9\xC2\xAE\xE2\x99\xAA\xE2\x98\x83\xF0\x9F\x92\xBE__",
			"\xC2\xA9\xC2\xAE\xE2\x99\xAA\xE2\x98\x83\xF0\x9F\x92\xBE__"),

		// Disallow invalid UTF-8 sequences.
		// Reference: https://en.wikipedia.org/wiki/UTF-8
		// - Invalid sequence: \x80\xC0\xE0\xF0\xF8
		// - Overlong encoding: U+0000 -> \xC0\x80 (Modified UTF-8)
		// - Overlong encoding: U+0020 -> \xE0\x80\xA0
		// - Overlong encoding: U+20AC -> \xF0\x82\x82\xAC
		// NOTE: Disabled for UTF-16 testing due to conversion issues,
		// and the fact that it's useless for UTF-16.
		FilterCacheKeyTest_mode(false,
			"\xC2\xA9\x80\xC0\xE0\xF0\xF8\xC0\x80\xE0\x80\xA0\xF0\x82\x82\xAC",
			"\xC2\xA9______________",
			"\xC2\xA9______________"),

		// Allow SMP characters. (>U+FFFF)
		// For UTF-8, this tests 4-byte sequences.
		// For UTF-16, this tests surrogate pairs.
		FilterCacheKeyTest_mode(true,
			"\xF0\x9F\x91\x80\xF0\x9F\x91\xA2\xF0\x9F\x92\xBE\xF0\x9F\xA6\x86",
			"\xF0\x9F\x91\x80\xF0\x9F\x91\xA2\xF0\x9F\x92\xBE\xF0\x9F\xA6\x86",
			"\xF0\x9F\x91\x80\xF0\x9F\x91\xA2\xF0\x9F\x92\xBE\xF0\x9F\xA6\x86")

		// TODO: UTF-16 test for invalid surrogate pairs.
	));

/**
 * Test CacheManager::filterCacheKey() with invalid parameters.
 * (UTF-8 version)
 */
TEST_F(FilterCacheKeyTest, filterCacheKey_EINVAL)
{
	char cache_key[2] = {'\0', '\0'};

	// Test NULL pointer.
	EXPECT_EQ(-EINVAL, LibCacheCommon::filterCacheKey(static_cast<char*>(nullptr)));

	// Test an empty string.
	EXPECT_EQ(-EINVAL, LibCacheCommon::filterCacheKey(cache_key));

	// Test a string containing: '/'
	cache_key[0] = '/';
	EXPECT_EQ(-EINVAL, LibCacheCommon::filterCacheKey(cache_key));

	// Test a string containing: '\\'
	cache_key[0] = '\\';
	EXPECT_EQ(-EINVAL, LibCacheCommon::filterCacheKey(cache_key));
}

#ifdef RP_WIS16
/**
 * Test CacheManager::filterCacheKey() with invalid parameters.
 * (UTF-16 version)
 */
TEST_F(FilterCacheKeyTest, filterCacheKey_EINVAL_wchar_t)
{
	wchar_t cache_key[2] = {L'\0', L'\0'};

	// Test NULL pointer.
	EXPECT_EQ(-EINVAL, LibCacheCommon::filterCacheKey(static_cast<wchar_t*>(nullptr)));

	// Test an empty string.
	EXPECT_EQ(-EINVAL, LibCacheCommon::filterCacheKey(cache_key));

	// Test a string containing: '/'
	cache_key[0] = L'/';
	EXPECT_EQ(-EINVAL, LibCacheCommon::filterCacheKey(cache_key));

	// Test a string containing: '\\'
	cache_key[0] = L'\\';
	EXPECT_EQ(-EINVAL, LibCacheCommon::filterCacheKey(cache_key));
}
#endif /* RP_WIS16 */

/**
 * Test CacheManager::filterCacheKey() with invalid UTF-8 sequences.
 * (UTF-8 version)
 */
TEST_F(FilterCacheKeyTest, filterCacheKey_invalid_UTF8)
{
	// NOTE: Only the first byte of invalid sequences is
	// overwritten with '_'.
	char cache_key[8];

	// Two-byte UTF-8 sequence: invalid second byte
	cache_key[0] = '\xC0';
	cache_key[1] = 'A';
	cache_key[2] = 'B';
	cache_key[3] = '\x0';
	EXPECT_EQ(0, LibCacheCommon::filterCacheKey(cache_key));
	EXPECT_STREQ("_AB", cache_key);

	// Three-byte UTF-8 sequence: invalid second byte
	cache_key[0] = '\xE0';
	cache_key[1] = 'A';
	cache_key[2] = 'B';
	cache_key[3] = 'C';
	cache_key[4] = '\x0';
	EXPECT_EQ(0, LibCacheCommon::filterCacheKey(cache_key));
	EXPECT_STREQ("_ABC", cache_key);

	// Three-byte UTF-8 sequence: invalid third byte
	cache_key[0] = '\xE0';
	cache_key[1] = '\x80';
	cache_key[2] = 'A';
	cache_key[3] = 'B';
	cache_key[4] = '\x0';
	EXPECT_EQ(0, LibCacheCommon::filterCacheKey(cache_key));
	EXPECT_STREQ("__AB", cache_key);

	// Four-byte UTF-8 sequence: invalid second byte
	cache_key[0] = '\xF0';
	cache_key[1] = 'A';
	cache_key[2] = 'B';
	cache_key[3] = 'C';
	cache_key[4] = 'D';
	cache_key[5] = '\x0';
	EXPECT_EQ(0, LibCacheCommon::filterCacheKey(cache_key));
	EXPECT_STREQ("_ABCD", cache_key);

	// Four-byte UTF-8 sequence: invalid third byte
	cache_key[0] = '\xF0';
	cache_key[1] = '\x80';
	cache_key[2] = 'A';
	cache_key[3] = 'B';
	cache_key[4] = 'C';
	cache_key[5] = '\x0';
	EXPECT_EQ(0, LibCacheCommon::filterCacheKey(cache_key));
	EXPECT_STREQ("__ABC", cache_key);

	// Four-byte UTF-8 sequence: invalid fourth byte
	cache_key[0] = '\xF0';
	cache_key[1] = '\x80';
	cache_key[2] = '\x80';
	cache_key[3] = 'A';
	cache_key[4] = 'B';
	cache_key[5] = '\x0';
	EXPECT_EQ(0, LibCacheCommon::filterCacheKey(cache_key));
	EXPECT_STREQ("___AB", cache_key);

	// Five-byte UTF-8 sequence (invalid in general)
	cache_key[0] = '\xF8';
	cache_key[1] = 'A';
	cache_key[2] = 'B';
	cache_key[3] = 'C';
	cache_key[4] = 'D';
	cache_key[5] = 'E';
	cache_key[6] = '\x0';
	EXPECT_EQ(0, LibCacheCommon::filterCacheKey(cache_key));
	EXPECT_STREQ("_ABCDE", cache_key);

	// Six-byte UTF-8 sequence (invalid in general)
	cache_key[0] = '\xFC';
	cache_key[1] = 'A';
	cache_key[2] = 'B';
	cache_key[3] = 'C';
	cache_key[4] = 'D';
	cache_key[5] = 'E';
	cache_key[6] = 'F';
	cache_key[7] = '\x0';
	EXPECT_EQ(0, LibCacheCommon::filterCacheKey(cache_key));
	EXPECT_STREQ("_ABCDEF", cache_key);
}

// TODO: Invalid UTF-16 sequence test for Windows.

} } //namespace LibCacheCommon::Tests

#ifdef HAVE_SECCOMP
const unsigned int rp_gtest_syscall_set = 0;
#endif /* HAVE_SECCOMP */

/**
 * Test suite main function.
 */
extern "C" int gtest_main(int argc, TCHAR *argv[])
{
	fputs("LibCacheCommon test suite: LibCacheCommon::filterCacheKey() tests.\n\n", stderr);
	fflush(nullptr);

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
