/***************************************************************************
 * ROM Properties Page shell extension. (rp-download/tests)                *
 * CacheKeyVerifyTest.cpp: CacheKeyVerify test.                            *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Google Test
#include "gtest_init.hpp"

// CacheKeyVerify
#include "../CacheKeyVerify.hpp"

// C includes (C++ namespace)
#include <cstdio>

// C++ STL classes
#include <string>
using std::tstring;

namespace RpDownload { namespace Tests {

struct CacheKeyVerifyTest_mode
{
	const TCHAR *cache_key;	// Cache key
	const TCHAR *full_url;	// Expected URL
	bool check_newer;	// Expected check_newer value
	CacheKeyError ckerr;	// Expected error code

	CacheKeyVerifyTest_mode(const TCHAR *cache_key, const TCHAR *full_url, bool check_newer, CacheKeyError ckerr)
		: cache_key(cache_key)
		, full_url(full_url)
		, check_newer(check_newer)
		, ckerr(ckerr)
	{}
};

class CacheKeyVerifyTest : public ::testing::TestWithParam<CacheKeyVerifyTest_mode>
{};

/**
 * Run a verifyCacheKey() test.
 */
TEST_P(CacheKeyVerifyTest, verifyCacheKeyTest)
{
	const CacheKeyVerifyTest_mode &mode = GetParam();

	tstring full_url;
	bool check_newer = false;
	CacheKeyError ckerr = verifyCacheKey(full_url, check_newer, mode.cache_key);
	EXPECT_EQ(ckerr, mode.ckerr);
	EXPECT_EQ(check_newer, mode.check_newer);
	if (ckerr == CacheKeyError::OK) {
		EXPECT_EQ(full_url, mode.full_url);
	}
}

INSTANTIATE_TEST_SUITE_P(ValidCacheKeys, CacheKeyVerifyTest,
	::testing::Values(
		CacheKeyVerifyTest_mode(_T("wii/disc/US/GALE01.png"), _T("https://art.gametdb.com/wii/disc/US/GALE01.png"), false, CacheKeyError::OK),
		CacheKeyVerifyTest_mode(_T("wiiu/disc/US/ARPE01.png"), _T("https://art.gametdb.com/wiiu/disc/US/ARPE01.png"), false, CacheKeyError::OK),
		CacheKeyVerifyTest_mode(_T("3ds/cover/JA/AREJ.jpg"), _T("https://art.gametdb.com/3ds/cover/JA/AREJ.jpg"), false, CacheKeyError::OK),
		CacheKeyVerifyTest_mode(_T("ds/cover/US/ASCE.jpg"), _T("https://art.gametdb.com/ds/cover/US/ASCE.jpg"), false, CacheKeyError::OK),
		CacheKeyVerifyTest_mode(_T("amiibo/00000000-00000002.png"), _T("https://amiibo.life/nfc/00000000-00000002/image"), false, CacheKeyError::OK),
		CacheKeyVerifyTest_mode(_T("gba/title/E/ASOE78.png"), _T("https://rpdb.gerbilsoft.com/gba/title/E/ASOE78.png"), false, CacheKeyError::OK),
		CacheKeyVerifyTest_mode(_T("gb/title/SGB/POKEMON RED-01.png"), _T("https://rpdb.gerbilsoft.com/gb/title/SGB/POKEMON%20RED-01.png"), false, CacheKeyError::OK),
		CacheKeyVerifyTest_mode(_T("gb/title/CGB/NoID/POKEMON YELLOW-01.png"), _T("https://rpdb.gerbilsoft.com/gb/title/CGB/NoID/POKEMON%20YELLOW-01.png"), false, CacheKeyError::OK),
		CacheKeyVerifyTest_mode(_T("gb/title/CGB/E/BRNE5M.png"), _T("https://rpdb.gerbilsoft.com/gb/title/CGB/E/BRNE5M.png"), false, CacheKeyError::OK),
		CacheKeyVerifyTest_mode(_T("snes/title/E/SNS-SUPER MARIOWORLD-USA.png"), _T("https://rpdb.gerbilsoft.com/snes/title/E/SNS-SUPER%20MARIOWORLD-USA.png"), false, CacheKeyError::OK),
		CacheKeyVerifyTest_mode(_T("snes/title/E/SNS-YI-USA.png"), _T("https://rpdb.gerbilsoft.com/snes/title/E/SNS-YI-USA.png"), false, CacheKeyError::OK),
		CacheKeyVerifyTest_mode(_T("snes/title/J/SHVC-ARWJ-JPN.png"), _T("https://rpdb.gerbilsoft.com/snes/title/J/SHVC-ARWJ-JPN.png"), false, CacheKeyError::OK),
		CacheKeyVerifyTest_mode(_T("ngpc/title/NEOP0059.png"), _T("https://rpdb.gerbilsoft.com/ngpc/title/NEOP0059.png"), false, CacheKeyError::OK),
		CacheKeyVerifyTest_mode(_T("ws/title/M/SWJ-AAE001.png"), _T("https://rpdb.gerbilsoft.com/ws/title/M/SWJ-AAE001.png"), false, CacheKeyError::OK),
		CacheKeyVerifyTest_mode(_T("ws/title/M/SWJ-BAN00F.png"), _T("https://rpdb.gerbilsoft.com/ws/title/M/SWJ-BAN00F.png"), false, CacheKeyError::OK),
		CacheKeyVerifyTest_mode(_T("ws/title/C/SWJ-BANC0D.png"), _T("https://rpdb.gerbilsoft.com/ws/title/C/SWJ-BANC0D.png"), false, CacheKeyError::OK),
		CacheKeyVerifyTest_mode(_T("ws/title/C/SWJ-BANC02.png"), _T("https://rpdb.gerbilsoft.com/ws/title/C/SWJ-BANC02.png"), false, CacheKeyError::OK),
		CacheKeyVerifyTest_mode(_T("md/title/F/GM 00001009-00.png"), _T("https://rpdb.gerbilsoft.com/md/title/F/GM%2000001009-00.png"), false, CacheKeyError::OK),
		CacheKeyVerifyTest_mode(_T("mcd/title/4/GM MK-4407 -00.png"), _T("https://rpdb.gerbilsoft.com/mcd/title/4/GM%20MK-4407%20-00.png"), false, CacheKeyError::OK),
		CacheKeyVerifyTest_mode(_T("32x/title/A/GM MK-84503-00.png"), _T("https://rpdb.gerbilsoft.com/32x/title/A/GM%20MK-84503-00.png"), false, CacheKeyError::OK),
		CacheKeyVerifyTest_mode(_T("mcd32x/title/4/GM T-16202F-00.png"), _T("https://rpdb.gerbilsoft.com/mcd32x/title/4/GM%20T-16202F-00.png"), false, CacheKeyError::OK),
		CacheKeyVerifyTest_mode(_T("pico/title/4/MK-49049-00.png"), _T("https://rpdb.gerbilsoft.com/pico/title/4/MK-49049-00.png"), false, CacheKeyError::OK),
		// NOTE: No known Teradrive ROM dumps yet...
		CacheKeyVerifyTest_mode(_T("tera/title/F/GM 00000000-00.png"), _T("https://rpdb.gerbilsoft.com/tera/title/F/GM%2000000000-00.png"), false, CacheKeyError::OK),
		CacheKeyVerifyTest_mode(_T("c64/title/crt/0/0ae6ec18.png"), _T("https://rpdb.gerbilsoft.com/c64/title/crt/0/0ae6ec18.png"), false, CacheKeyError::OK),
		CacheKeyVerifyTest_mode(_T("c128/title/crt/ca3ba492.png"), _T("https://rpdb.gerbilsoft.com/c128/title/crt/ca3ba492.png"), false, CacheKeyError::OK),

		// NOTE: No cart images for cbmII, vic20, or plus4 yet...
		CacheKeyVerifyTest_mode(_T("cbmII/title/crt/00000000.png"), _T("https://rpdb.gerbilsoft.com/cbmII/title/crt/00000000.png"), false, CacheKeyError::OK),
		CacheKeyVerifyTest_mode(_T("vic20/title/crt/00000000.png"), _T("https://rpdb.gerbilsoft.com/vic20/title/crt/00000000.png"), false, CacheKeyError::OK),
		CacheKeyVerifyTest_mode(_T("plus4/title/crt/00000000.png"), _T("https://rpdb.gerbilsoft.com/plus4/title/crt/00000000.png"), false, CacheKeyError::OK),

		CacheKeyVerifyTest_mode(_T("ps1/cover/SCPS/SCPS-10031.jpg"), _T("https://rpdb.gerbilsoft.com/ps1/cover/SCPS/SCPS-10031.jpg"), false, CacheKeyError::OK),
		CacheKeyVerifyTest_mode(_T("ps1/cover3D/SCPS/SCPS-10031.png"), _T("https://rpdb.gerbilsoft.com/ps1/cover3D/SCPS/SCPS-10031.png"), false, CacheKeyError::OK),
		CacheKeyVerifyTest_mode(_T("ps2/cover/SLUS/SLUS-20917.jpg"), _T("https://rpdb.gerbilsoft.com/ps2/cover/SLUS/SLUS-20917.jpg"), false, CacheKeyError::OK),
		CacheKeyVerifyTest_mode(_T("ps2/cover3D/SLUS/SLUS-20917.png"), _T("https://rpdb.gerbilsoft.com/ps2/cover3D/SLUS/SLUS-20917.png"), false, CacheKeyError::OK),

		CacheKeyVerifyTest_mode(_T("sys/version.txt"), _T("https://rpdb.gerbilsoft.com/sys/version.txt"), true, CacheKeyError::OK)
		)
	);

INSTANTIATE_TEST_SUITE_P(InvalidCacheKeys, CacheKeyVerifyTest,
	::testing::Values(
		// Empty cache key
		CacheKeyVerifyTest_mode(_T(""), nullptr, false, CacheKeyError::Invalid),

		// No slashes
		CacheKeyVerifyTest_mode(_T("GALE01.png"), nullptr, false, CacheKeyError::Invalid),

		// No prefix
		CacheKeyVerifyTest_mode(_T("/disc/US/GALE01.png"), nullptr, false, CacheKeyError::Invalid),

		// No file extension
		CacheKeyVerifyTest_mode(_T("wii/disc/US/GALE01"), nullptr, false, CacheKeyError::Invalid),

		// Invalid file extension (.txt is only valid for [sys])
		CacheKeyVerifyTest_mode(_T("wii/disc/US/GALE01.tiff"), nullptr, false, CacheKeyError::Invalid),
		CacheKeyVerifyTest_mode(_T("wii/disc/US/GALE01.txt"), nullptr, false, CacheKeyError::Invalid),

		// Invalid prefix
		CacheKeyVerifyTest_mode(_T("blahblah/quack/ducks.jpg"), nullptr, false, CacheKeyError::PrefixNotSupported)
		)
	);

} }

#ifdef HAVE_SECCOMP
const unsigned int rp_gtest_syscall_set = 0;
#endif /* HAVE_SECCOMP */

/**
 * Test suite main function
 */
extern "C" int gtest_main(int argc, TCHAR *argv[])
{
	fputs("rp-download test suite: CacheKeyVerify tests\n\n", stderr);
	fflush(nullptr);

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
