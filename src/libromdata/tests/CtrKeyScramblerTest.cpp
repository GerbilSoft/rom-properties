/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * CtrKeyScramblerTest.cpp: CtrKeyScrambler class test.                    *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Google Test
#include "gtest/gtest.h"
#include "tcharx.h"

// CtrKeyScrambler
#include "librpbase/crypto/KeyManager.hpp"
#include "../crypto/CtrKeyScrambler.hpp"

// C includes (C++ namespace)
#include <cstdio>

// C++ includes
#include <iostream>
#include <string>
#include <vector>
using std::string;
using std::vector;

namespace LibRomData { namespace Tests {

// Parameters for CtrKeyScrambler tests.
struct CtrKeyScramblerTest_mode
{
	const uint8_t *keyNormal;	// Expected KeyNormal.
	const uint8_t *keyX;		// KeyX.
	const uint8_t *keyY;		// KeyY.

	CtrKeyScramblerTest_mode(
		const uint8_t *keyNormal,
		const uint8_t *keyX,
		const uint8_t *keyY)
		: keyNormal(keyNormal)
		, keyX(keyX)
		, keyY(keyY)
	{ }

	// May be required for MSVC 2010?
	CtrKeyScramblerTest_mode(const CtrKeyScramblerTest_mode &other)
		: keyNormal(other.keyNormal)
		, keyX(other.keyX)
		, keyY(other.keyY)
	{ }

	// Required for MSVC 2010.
	CtrKeyScramblerTest_mode &operator=(const CtrKeyScramblerTest_mode &other)
	{
		keyNormal = other.keyNormal;
		keyX = other.keyX;
		keyY = other.keyY;
		return *this;
	}
};

class CtrKeyScramblerTest : public ::testing::TestWithParam<CtrKeyScramblerTest_mode>
{
	protected:
		CtrKeyScramblerTest() = default;

	public:
		// TODO: Split into a separate file, since this is
		// also implemented by AesCipherTest.
		/**
		 * Compare two byte arrays.
		 * The byte arrays are converted to hexdumps and then
		 * compared using EXPECT_EQ().
		 * @param expected	[in] Expected data.
		 * @param actual	[in] Actual data.
		 * @param size		[in] Size of both arrays.
		 * @param data_type	[in] Data type.
		 */
		void CompareByteArrays(
			const uint8_t *expected,
			const uint8_t *actual,
			size_t size,
			const char *data_type);
};

/**
 * Compare two byte arrays.
 * The byte arrays are converted to hexdumps and then
 * compared using EXPECT_EQ().
 * @param expected	[in] Expected data.
 * @param actual	[in] Actual data.
 * @param size		[in] Size of both arrays.
 * @param data_type	[in] Data type.
 */
void CtrKeyScramblerTest::CompareByteArrays(
	const uint8_t *expected,
	const uint8_t *actual,
	size_t size,
	const char *data_type)
{
	// Output format: (assume ~64 bytes per line)
	// 0000: 01 23 45 67 89 AB CD EF  01 23 45 67 89 AB CD EF
	const size_t bufSize = ((size / 16) + !!(size % 16)) * 64;
	char printf_buf[16];
	string s_expected, s_actual;
	s_expected.reserve(bufSize);
	s_actual.reserve(bufSize);

	const uint8_t *pE = expected, *pA = actual;
	for (size_t i = 0; i < size; i++, pE++, pA++) {
		if (i % 16 == 0) {
			// New line.
			if (i > 0) {
				// Append newlines.
				s_expected += '\n';
				s_actual += '\n';
			}

			// TODO: Print a 64-bit value.
			snprintf(printf_buf, sizeof(printf_buf), "%04X: ", static_cast<unsigned int>(i));
			s_expected += printf_buf;
			s_actual += printf_buf;
		}

		// Print the byte.
		snprintf(printf_buf, sizeof(printf_buf), "%02X", *pE);
		s_expected += printf_buf;
		snprintf(printf_buf, sizeof(printf_buf), "%02X", *pA);
		s_actual += printf_buf;

		if (i % 16 == 7) {
			s_expected += "  ";
			s_actual += "  ";
		} else if (i % 16  < 15) {
			s_expected += ' ';
			s_actual += ' ';
		}
	}

	// Compare the byte arrays, and
	// print the strings on failure.
	EXPECT_EQ(0, memcmp(expected, actual, size)) <<
		"Expected " << data_type << ":" << '\n' << s_expected << '\n' <<
		"Actual " << data_type << ":" << '\n' << s_actual << '\n';
}

/**
 * Run a CtrKeyScrambler test.
 */
TEST_P(CtrKeyScramblerTest, ctrScrambleTest)
{
	const CtrKeyScramblerTest_mode &mode = GetParam();
	ASSERT_TRUE(mode.keyNormal != nullptr);
	ASSERT_TRUE(mode.keyX != nullptr);
	ASSERT_TRUE(mode.keyY != nullptr);

	// Use a rather bland scrambling key.
	static const uint8_t ctr_scrambler[16] = {
		0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
		0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F
	};

	u128_t keyNormal;
	ASSERT_EQ(0, CtrKeyScrambler::CtrScramble(&keyNormal,
		reinterpret_cast<const u128_t*>(mode.keyX),
		reinterpret_cast<const u128_t*>(mode.keyY),
		reinterpret_cast<const u128_t*>(&ctr_scrambler)));

	// Compare the generated KeyNormal to the expected KeyNormal.
	CompareByteArrays(mode.keyNormal, keyNormal.u8, 16, "KeyNormal");
}

/** CtrKeyScrambler tests. **/

// Example KeyX.
static const uint8_t test_KeyX[16] = {
	0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,
	0xFE,0xDC,0xBA,0x98,0x76,0x54,0x32,0x10
};

// Example KeyY.
static const uint8_t test_KeyY[16] = {
	0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,
	0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA
};

// Expected CtrScramble(test_KeyX, test_KeyY).
static const uint8_t test_CtrScramble[16] = {
	0xEB,0x4C,0x83,0xD5,0xFC,0xA8,0x94,0x21,
	0x1B,0xBB,0x85,0x34,0x0E,0x5B,0x70,0xE4
};

INSTANTIATE_TEST_SUITE_P(ctrScrambleTest, CtrKeyScramblerTest,
	::testing::Values(
		CtrKeyScramblerTest_mode(test_CtrScramble, test_KeyX, test_KeyY)
	));
} }

/**
 * Test suite main function.
 */
extern "C" int gtest_main(int argc, TCHAR *argv[])
{
	fprintf(stderr, "LibRomData test suite: CtrKeyScrambler tests.\n\n");
	fflush(nullptr);

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
