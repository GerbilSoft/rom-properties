/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * TextFuncsTest.cpp: TextFuncs class test.                                *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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

// TextFuncs
#include "../TextFuncs.hpp"

// C includes. (C++ namespace)
#include <cstdio>

// C++ includes.
#include <string>
using std::string;
using std::u16string;

namespace LibRomData { namespace Tests {

class TextFuncsTest : public ::testing::Test
{
	protected:
		TextFuncsTest() { }

	public:
		// NOTE: Test strings are uint8_t[] in order to prevent
		// narrowing conversion warnings from appearing.

		/**
		 * cp1252 test string.
		 * Contains all possible cp1252 characters.
		 */
		static const uint8_t cp1252_data[250];

		/**
		 * cp1252 to UTF-8 test string.
		 * Contains the expected result from:
		 * - cp1252_to_utf8(cp1252_data, sizeof(cp1252_data))
		 * - cp1252_sjis_to_utf8(cp1252_data, sizeof(cp1252_data))
		 */
		static const uint8_t cp1252_utf8_data[388];
};

// Test strings are located in TextFuncsTest_data.hpp.
#include "TextFuncsTest_data.hpp"

/**
 * Test cp1252_to_utf8().
 */
TEST_F(TextFuncsTest, cp1252_to_utf8)
{
	// Test with implicit length.
	string u8str = cp1252_to_utf8((const char*)cp1252_data, -1);
	EXPECT_EQ(u8str.size(), sizeof(cp1252_utf8_data)-1);
	EXPECT_EQ((const char*)cp1252_utf8_data, u8str);

	// Test with explicit length.
	u8str = cp1252_to_utf8((const char*)cp1252_data, sizeof(cp1252_data)-1);
	EXPECT_EQ(u8str.size(), sizeof(cp1252_utf8_data)-1);
	EXPECT_EQ((const char*)cp1252_utf8_data, u8str);

	// Test with explicit length and trimmed NULLs.
	u8str = cp1252_to_utf8((const char*)cp1252_data, sizeof(cp1252_data));
	EXPECT_EQ(u8str.size(), sizeof(cp1252_utf8_data)-1);
	EXPECT_EQ((const char*)cp1252_utf8_data, u8str);
}

/**
 * Test cp1252_sjis_to_utf8() fallback functionality.
 * These strings should be detected as cp1252 due to
 * Shift-JIS decoding errors.
 */
TEST_F(TextFuncsTest, cp1252_sjis_to_utf8_fallback)
{
	// Test with implicit length.
	string u8str = cp1252_sjis_to_utf8((const char*)cp1252_data, -1);
	EXPECT_EQ(u8str.size(), sizeof(cp1252_utf8_data)-1);
	EXPECT_EQ((const char*)cp1252_utf8_data, u8str);

	// Test with explicit length.
	u8str = cp1252_sjis_to_utf8((const char*)cp1252_data, sizeof(cp1252_data)-1);
	EXPECT_EQ(u8str.size(), sizeof(cp1252_utf8_data)-1);
	EXPECT_EQ((const char*)cp1252_utf8_data, u8str);

	// Test with explicit length and trimmed NULLs.
	u8str = cp1252_sjis_to_utf8((const char*)cp1252_data, sizeof(cp1252_data));
	EXPECT_EQ(u8str.size(), sizeof(cp1252_utf8_data)-1);
	EXPECT_EQ((const char*)cp1252_utf8_data, u8str);
}

/**
 * Test cp1252_sjis_to_utf8() with ASCII strings.
 * Note that backslashes will *not* be converted to
 * yen symbols, so this should be a no-op.
 *
 * FIXME: Backslash may be converted to yen symbols
 * on Windows if the system has a Japanese locale.
 */
TEST_F(TextFuncsTest, cp1252_sjis_to_utf8_ascii)
{
	static const char cp1252_in[] = "C:\\Windows\\System32";

	// Test with implicit length.
	string u8str = cp1252_sjis_to_utf8(cp1252_in, -1);
	EXPECT_EQ(u8str.size(), sizeof(cp1252_in)-1);
	EXPECT_EQ(cp1252_in, u8str);

	// Test with explicit length.
	u8str = cp1252_sjis_to_utf8(cp1252_in, sizeof(cp1252_in)-1);
	EXPECT_EQ(u8str.size(), sizeof(cp1252_in)-1);
	EXPECT_EQ(cp1252_in, u8str);

	// Test with explicit length and trimmed NULLs.
	u8str = cp1252_sjis_to_utf8(cp1252_in, sizeof(cp1252_in));
	EXPECT_EQ(u8str.size(), sizeof(cp1252_in)-1);
	EXPECT_EQ(cp1252_in, u8str);
}

/**
 * Test cp1252_sjis_to_utf8() with Japanese text.
 */
TEST_F(TextFuncsTest, cp1252_sjis_to_utf8_japanese)
{
	// TODO: Longer test string.
	// This string is from the JP Pokemon Colosseum (GCN) save file.
	static const uint8_t sjis_in[34] = {
		0x83,0x7C,0x83,0x50,0x83,0x82,0x83,0x93,
		0x83,0x52,0x83,0x8D,0x83,0x56,0x83,0x41,
		0x83,0x80,0x0A,0x83,0x5A,0x81,0x5B,0x83,
		0x75,0x83,0x74,0x83,0x40,0x83,0x43,0x83,
		0x8B,0x00
	};

	static const uint8_t utf8_out[50] = {
		0xE3,0x83,0x9D,0xE3,0x82,0xB1,0xE3,0x83,
		0xA2,0xE3,0x83,0xB3,0xE3,0x82,0xB3,0xE3,
		0x83,0xAD,0xE3,0x82,0xB7,0xE3,0x82,0xA2,
		0xE3,0x83,0xA0,0x0A,0xE3,0x82,0xBB,0xE3,
		0x83,0xBC,0xE3,0x83,0x96,0xE3,0x83,0x95,
		0xE3,0x82,0xA1,0xE3,0x82,0xA4,0xE3,0x83,
		0xAB,0x00
	};

	// Test with implicit length.
	string u8str = cp1252_sjis_to_utf8((const char*)sjis_in, -1);
	EXPECT_EQ(u8str.size(), sizeof(utf8_out)-1);
	EXPECT_EQ((const char*)utf8_out, u8str);

	// Test with explicit length.
	u8str = cp1252_sjis_to_utf8((const char*)sjis_in, sizeof(sjis_in)-1);
	EXPECT_EQ(u8str.size(), sizeof(utf8_out)-1);
	EXPECT_EQ((const char*)utf8_out, u8str);

	// Test with explicit length and trimmed NULLs.
	u8str = cp1252_sjis_to_utf8((const char*)sjis_in, sizeof(sjis_in));
	EXPECT_EQ(u8str.size(), sizeof(utf8_out)-1);
	EXPECT_EQ((const char*)utf8_out, u8str);
}

} }

/**
 * Test suite main function.
 */
extern "C" int gtest_main(int argc, char *argv[])
{
	fprintf(stderr, "LibRomData test suite: TextFuncs tests.\n\n");
	fflush(nullptr);

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
