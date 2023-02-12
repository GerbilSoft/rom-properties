/***************************************************************************
 * ROM Properties Page shell extension. (librpcpu/tests)                   *
 * BitstuffTest.cpp: bitstuff.h functions test                             *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Google Test
#include "gtest/gtest.h"
#include "tcharx.h"

// bit stuff
#include "librpcpu/bitstuff.h"

// C includes. (C++ namespace)
#include <cstdio>

namespace LibRpCpu { namespace Tests {

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
 * Test popcount()
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
	fputs("LibRpCpu test suite: bitstuff.h tests\n\n", stderr);
	fflush(nullptr);

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
