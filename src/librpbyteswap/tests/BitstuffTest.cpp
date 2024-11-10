/***************************************************************************
 * ROM Properties Page shell extension. (librpbyteswap/tests)              *
 * BitstuffTest.cpp: bitstuff.h functions test                             *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Google Test
#include "gtest/gtest.h"
#include "tcharx.h"

// bit stuff
#include "librpbyteswap/bitstuff.h"

// C includes (C++ namespace)
#include <cstdio>
#include <cstdlib>
#include <ctime>

namespace LibRpByteswap { namespace Tests {

class BitstuffTest : public ::testing::Test
{ };

/**
 * Test uilog2()
 */
TEST_F(BitstuffTest, uilog2)
{
	// Test bits 0 through 31.
	for (unsigned int i = 0; i < 32; i++) {
		EXPECT_EQ(i, uilog2(1U << i));
	}

	// Test doubled bits, e.g. 00000011, 00000110, etc.
	// The result should be the highest bit.
	for (unsigned int i = 1; i < 32; i++) {
		const unsigned int test_val = (1U << i) | (1U << (i-1));
		EXPECT_EQ(i, uilog2(test_val));
	}

	// Test values with bit 31 always set.
	// The result should always be 31.
	unsigned int test_val = 0;
	for (unsigned int i = 0; i < 32; i++) {
		test_val <<= (i & 1);
		test_val |= (1U << 31);
		EXPECT_EQ(31U, uilog2(test_val));
	}

	// NOTE: uilog2() returns 0 here, which is technically wrong,
	// but it's better to return a defined value than an undefined one.
	EXPECT_EQ(0U, uilog2(0));
}

/**
 * Test popcount() [inline version that may use a compiler built-in]
 */
TEST_F(BitstuffTest, popcount)
{
	// Empty and filled
	EXPECT_EQ(0U, popcount(0U));
	EXPECT_EQ(32U, popcount(~0U));

	// Various test patterns
	EXPECT_EQ(16U, popcount(0x55555555U));
	EXPECT_EQ(16U, popcount(0xAAAAAAAAU));
	EXPECT_EQ(16U, popcount(0x33333333U));
	EXPECT_EQ(16U, popcount(0x0F0F0F0FU));

	EXPECT_EQ(8U, popcount(0x05050505U));
	EXPECT_EQ(8U, popcount(0x50505050U));
	EXPECT_EQ(8U, popcount(0x0A0A0A0AU));
	EXPECT_EQ(8U, popcount(0xA0A0A0A0U));
	EXPECT_EQ(8U, popcount(0x03030303U));
	EXPECT_EQ(8U, popcount(0x30303030U));
	EXPECT_EQ(8U, popcount(0x0C0C0C0CU));
	EXPECT_EQ(8U, popcount(0xC0C0C0C0U));

	EXPECT_EQ(12U, popcount(0x07070707U));
	EXPECT_EQ(12U, popcount(0x70707070U));
	EXPECT_EQ(12U, popcount(0x0E0E0E0EU));
	EXPECT_EQ(12U, popcount(0xE0E0E0E0U));

	// Single bit
	for (unsigned int i = 0; i < 32; i++) {
		EXPECT_EQ(1U, popcount(1U << i));
	}
}

/**
 * Test popcount() using random values [inline version that may use a compiler built-in]
 */
TEST_F(BitstuffTest, rand_popcount)
{
	// Testing 16,384 random values.
	// NOTE: MSVC RAND_MAX is 32,767, so we only get 15 bits of random data per rand() call.
	for (unsigned int i = 0; i < 16384; i++) {
		unsigned int testval = 0;
		unsigned int popcnt_expected = 0;

		int rnd = 0;
		for (unsigned int j = 0; j < 32; j++, rnd >>= 1) {
			if (j == 0 || j == 15 || j == 30) {
				rnd = rand();
			}
			const unsigned int bit = (rnd & 1);
			testval <<= 1;
			testval |= bit;
			popcnt_expected += bit;
		}

		// Test the popcount() function.
		EXPECT_EQ(popcnt_expected, popcount(testval)) << "Test value: 0x" <<
			std::setw(8) << std::setfill('0') << std::hex << testval <<
			std::setw(0) << std::setfill(' ') << std::dec;
	}
}

/**
 * Test popcount_c() [C version]
 */
TEST_F(BitstuffTest, popcount_c)
{
	// Empty and filled
	EXPECT_EQ(0U, popcount_c(0U));
	EXPECT_EQ(32U, popcount_c(~0U));

	// Various test patterns
	EXPECT_EQ(16U, popcount_c(0x55555555U));
	EXPECT_EQ(16U, popcount_c(0xAAAAAAAAU));
	EXPECT_EQ(16U, popcount_c(0x33333333U));
	EXPECT_EQ(16U, popcount_c(0x0F0F0F0FU));

	EXPECT_EQ(8U, popcount_c(0x05050505U));
	EXPECT_EQ(8U, popcount_c(0x50505050U));
	EXPECT_EQ(8U, popcount_c(0x0A0A0A0AU));
	EXPECT_EQ(8U, popcount_c(0xA0A0A0A0U));
	EXPECT_EQ(8U, popcount_c(0x03030303U));
	EXPECT_EQ(8U, popcount_c(0x30303030U));
	EXPECT_EQ(8U, popcount_c(0x0C0C0C0CU));
	EXPECT_EQ(8U, popcount_c(0xC0C0C0C0U));

	EXPECT_EQ(12U, popcount_c(0x07070707U));
	EXPECT_EQ(12U, popcount_c(0x70707070U));
	EXPECT_EQ(12U, popcount_c(0x0E0E0E0EU));
	EXPECT_EQ(12U, popcount_c(0xE0E0E0E0U));

	// Single bit
	for (unsigned int i = 0; i < 32; i++) {
		EXPECT_EQ(1U, popcount_c(1U << i));
	}
}

/**
 * Test popcount_c() using random values [C version]
 */
TEST_F(BitstuffTest, rand_popcount_c)
{
	// Testing 16,384 random values.
	// NOTE: MSVC RAND_MAX is 32,767, so we only get 15 bits of random data per rand() call.
	for (unsigned int i = 0; i < 16384; i++) {
		unsigned int testval = 0;
		unsigned int popcnt_expected = 0;

		int rnd = 0;
		for (unsigned int j = 0; j < 32; j++, rnd >>= 1) {
			if (j == 0 || j == 15 || j == 30) {
				rnd = rand();
			}
			const unsigned int bit = (rnd & 1);
			testval <<= 1;
			testval |= bit;
			popcnt_expected += bit;
		}

		// Test the popcount_c() function.
		EXPECT_EQ(popcnt_expected, popcount_c(testval)) << "Test value: 0x" <<
			std::setw(8) << std::setfill('0') << std::hex << testval <<
			std::setw(0) << std::setfill(' ') << std::dec;
	}
}

/**
 * Test isPow2()
 */
TEST_F(BitstuffTest, isPow2)
{
	// Zero is NOT considered a power of two by this function.
	EXPECT_FALSE(isPow2(0U));
	// ...and neither should ~0U.
	EXPECT_FALSE(isPow2(~0U));

	// Single bits should all be considered powers of two
	for (unsigned int i = 0; i < 32; i++) {
		EXPECT_TRUE(1U << i);
	}

	// Doubled bits should NOT be powers of two.
	// 00000011, 00000110, etc.
	for (unsigned int i = 1; i < 32; i++) {
		const unsigned int test_val = (1U << i) | (1U << (i-1));
		EXPECT_FALSE(isPow2(test_val));
	}
}

/**
 * Test nextPow2()
 */
TEST_F(BitstuffTest, nextPow2)
{
	// Single bits should result in the next bit.
	for (unsigned int i = 0; i < 31; i++) {
		EXPECT_EQ(1U << (i+1), nextPow2(1U << i));
	}

	// Bit 31 will overflow.
	// FIXME: On gcc, it becomes 0. On MSVC, it becomes 1.
	//EXPECT_EQ(0U, nextPow2(1U << 31));

	// Doubled bits should go to the next bit.
	// 00000011, 00000110, etc.
	for (unsigned int i = 1; i < 31; i++) {
		const unsigned int test_val = (1U << i) | (1U << (i-1));
		EXPECT_EQ(1U << (i+1), nextPow2(test_val));
	}
}

} }

/**
 * Test suite main function
 */
extern "C" int gtest_main(int argc, TCHAR *argv[])
{
	fputs("LibRpByteswap test suite: bitstuff.h tests\n\n", stderr);
	fflush(nullptr);

	// Initialize the random number generator.
	srand(time(nullptr));

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
