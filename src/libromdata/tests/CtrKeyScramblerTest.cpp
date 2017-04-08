/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * CtrKeyScramblerTest.cpp: CtrKeyScrambler class test.                    *
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

#include "../TextFuncs.hpp"

// CtrKeyScrambler
#include "../crypto/KeyManager.hpp"
#include "../crypto/CtrKeyScrambler.hpp"

// C includes. (C++ namespace)
#include <cstdio>

// C++ includes.
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using std::endl;
using std::ostringstream;
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
		CtrKeyScramblerTest() { }

		virtual void SetUp(void) override final;
		virtual void TearDown(void) override final;

	public:
		// TODO: Split into a separate file, since this is
		// also implemented by AesCipherTest.
		/**
		 * Compare two byte arrays.
		 * The byte arrays are converted to hexdumps and then
		 * compared using EXPECT_EQ().
		 * @param expected Expected data.
		 * @param actual Actual data.
		 * @param size Size of both arrays.
		 * @param data_type Data type.
		 */
		void CompareByteArrays(
			const uint8_t *expected,
			const uint8_t *actual,
			unsigned int size,
			const char *data_type);
};

/**
 * Compare two byte arrays.
 * The byte arrays are converted to hexdumps and then
 * compared using EXPECT_EQ().
 * @param expected Expected data.
 * @param actual Actual data.
 * @param size Size of both arrays.
 * @param data_type Data type.
 */
void CtrKeyScramblerTest::CompareByteArrays(
	const uint8_t *expected,
	const uint8_t *actual,
	unsigned int size,
	const char *data_type)
{
	// Output format: (assume ~64 bytes per line)
	// 0000: 01 23 45 67 89 AB CD EF  01 23 45 67 89 AB CD EF
	const unsigned int bufSize = ((size / 16) + !!(size % 16)) * 64;
	char printf_buf[16];
	string s_expected, s_actual;
	s_expected.reserve(bufSize);
	s_actual.reserve(bufSize);

	// TODO: Use stringstream instead?
	const uint8_t *pE = expected, *pA = actual;
	for (unsigned int i = 0; i < size; i++, pE++, pA++) {
		if (i % 16 == 0) {
			// New line.
			if (i > 0) {
				// Append newlines.
				s_expected += '\n';
				s_actual += '\n';
			}

			snprintf(printf_buf, sizeof(printf_buf), "%04X: ", i);
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
		"Expected " << data_type << ":" << endl << s_expected << endl <<
		"Actual " << data_type << ":" << endl << s_actual << endl;
}

/**
 * SetUp() function.
 * Run before each test.
 */
void CtrKeyScramblerTest::SetUp(void)
{ }

/**
 * TearDown() function.
 * Run after each test.
 */
void CtrKeyScramblerTest::TearDown(void)
{ }

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

INSTANTIATE_TEST_CASE_P(ctrScrambleTest, CtrKeyScramblerTest,
	::testing::Values(
		CtrKeyScramblerTest_mode(test_CtrScramble, test_KeyX, test_KeyY)
	));
} }

/**
 * Test suite main function.
 */
extern "C" int gtest_main(int argc, char *argv[])
{
	fprintf(stderr, "LibRomData test suite: CtrKeyScrambler tests.\n\n");
	fflush(nullptr);

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
