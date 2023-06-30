/***************************************************************************
 * ROM Properties Page shell extension. (librpbase/tests)                  *
 * MD5HashTest.cpp: MD5Hash class test.                                    *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Google Test
#include "gtest/gtest.h"
#include "tcharx.h"

// MD5Hash
#include "../crypto/MD5Hash.hpp"

// C includes. (C++ namespace)
#include <cstdio>

// C++ includes.
#include <iostream>
#include <sstream>
#include <string>
using std::ostringstream;
using std::string;

namespace LibRpBase { namespace Tests {

struct MD5HashTest_mode
{
	// String to hash.
	const char *str;

	// Hash data. (16 bytes)
	const uint8_t *md5;

	MD5HashTest_mode(const char *str, const uint8_t *md5)
		: str(str), md5(md5)
	{ }
};

class MD5HashTest : public ::testing::TestWithParam<MD5HashTest_mode>
{
	public:
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
void MD5HashTest::CompareByteArrays(
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
 * Run an MD5Hash test.
 */
TEST_P(MD5HashTest, md5HashTest)
{
	const MD5HashTest_mode &mode = GetParam();

	uint8_t md5[16];
	EXPECT_EQ(0, MD5Hash::calcHash(md5, sizeof(md5), mode.str, strlen(mode.str)));

	// Compare the hash to the expected hash.
	CompareByteArrays(mode.md5, md5, sizeof(md5), "MD5 hash");
}

/** MD5 hash tests. **/

static const uint8_t md5_exp[][16] = {
	{0xD4,0x1D,0x8C,0xD9,0x8F,0x00,0xB2,0x04,
	 0xE9,0x80,0x09,0x98,0xEC,0xF8,0x42,0x7E},

	{0xE4,0xD9,0x09,0xC2,0x90,0xD0,0xFB,0x1C,
	 0xA0,0x68,0xFF,0xAD,0xDF,0x22,0xCB,0xD0},

	{0x39,0xA1,0x08,0x7C,0x44,0x3E,0xAB,0xCB,
	 0x69,0xBF,0x9F,0xC4,0xD8,0xA3,0x49,0x96},

	{0x81,0x8C,0x6E,0x60,0x1A,0x24,0xF7,0x27,
	 0x50,0xDA,0x0F,0x6C,0x9B,0x8E,0xBE,0x28},

	{0xB6,0xBD,0x99,0xD7,0xB2,0x10,0xAB,0x3B,
	 0x09,0x89,0xD1,0x12,0x8D,0xF9,0x26,0x47},

	{0xFE,0x96,0x0B,0x7E,0x81,0xAE,0x74,0xF0,
	 0xC1,0x05,0xE9,0x0A,0x88,0x40,0x77,0xA0},
};

INSTANTIATE_TEST_SUITE_P(MD5StringHashTest, MD5HashTest,
	::testing::Values(
		MD5HashTest_mode("", md5_exp[0]),
		MD5HashTest_mode("The quick brown fox jumps over the lazy dog.", md5_exp[1]),
		MD5HashTest_mode("▁▂▃▄▅▆▇█▉▊▋▌▍▎▏", md5_exp[2]),
		MD5HashTest_mode("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.", md5_exp[3]),
		MD5HashTest_mode("ＳＰＹＲＯ　ＴＨＥ　ＤＲＡＧＯＮ", md5_exp[4]),
		MD5HashTest_mode("ソニック カラーズ", md5_exp[5])
		)
	);

} }
