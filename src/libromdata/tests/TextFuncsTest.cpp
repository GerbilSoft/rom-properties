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
#include "../byteorder.h"

// C includes. (C++ namespace)
#include <cstdio>

// C++ includes.
#include <string>
using std::string;
using std::u16string;

// NOTE: We're redefining ARRAY_SIZE() here in order to
// get a size_t instead of an int.
#ifdef ARRAY_SIZE
#unfed ARRAY_SIZE
#endif

/**
 * Number of elements in an array.
 *
 * Includes a static check for pointers to make sure
 * a dynamically-allocated array wasn't specified.
 * Reference: http://stackoverflow.com/questions/8018843/macro-definition-array-size
 */
#define ARRAY_SIZE(x) \
	(((sizeof(x) / sizeof(x[0]))) / \
		(size_t)(!(sizeof(x) % sizeof(x[0]))))

namespace LibRomData { namespace Tests {

class TextFuncsTest : public ::testing::Test
{
	protected:
		TextFuncsTest() { }

	public:
		// NOTE: 8-bit test strings are unsigned in order to prevent
		// narrowing conversion warnings from appearing.
		// char16_t is defined as unsigned, so this isn't a problem
		// for 16-bit strings.

		/**
		 * cp1252 test string.
		 * Contains all possible cp1252 characters.
		 */
		static const uint8_t cp1252_data[250];

		/**
		 * cp1252 to UTF-8 test string.
		 * Contains the expected result from:
		 * - cp1252_to_utf8(cp1252_data, ARRAY_SIZE(cp1252_data))
		 * - cp1252_sjis_to_utf8(cp1252_data, ARRAY_SIZE(cp1252_data))
		 */
		static const uint8_t cp1252_utf8_data[388];

		/**
		 * cp1252 to UTF-16 test string.
		 * Contains the expected result from:
		 * - cp1252_to_utf16(cp1252_data, sizeof(cp1252_data))
		 * - cp1252_sjis_to_utf16(cp1252_data, sizeof(cp1252_data))
		 */
		static const char16_t cp1252_utf16_data[250];

		/**
		 * Shift-JIS test string.
		 *
		 * TODO: Get a longer test string.
		 * This string is from the JP Pokemon Colosseum (GCN) save file,
		 * plus a wave dash character (8160).
		 */
		static const uint8_t sjis_data[36];

		/**
		 * Shift-JIS to UTF-8 test string.
		 * Contains the expected result from:
		 * - cp1252_sjis_to_utf8(sjis_data, ARRAY_SIZE(sjis_data))
		 */
		static const uint8_t sjis_utf8_data[53];

		/**
		 * Shift-JIS to UTF-16 test string.
		 * Contains the expected result from:
		 * - cp1252_sjis_to_utf16(sjis_data, ARRAY_SIZE(sjis_data))
		 */
		static const char16_t sjis_utf16_data[19];

		/**
		 * UTF-8 test string.
		 * Contains Latin-1, BMP, and SMP characters.
		 *
		 * This contains the same string as
		 * utf16le_data[] and utf16be_data[].
		 */
		static const uint8_t utf8_data[325];

		/**
		 * UTF-16LE test string.
		 * Contains Latin-1, BMP, and SMP characters.
		 *
		 * This contains the same string as
		 * utf8_data[] and utf16be_data[].
		 *
		 * NOTE: This is encoded as uint8_t to prevent
		 * byteswapping issues.
		 */
		static const uint8_t utf16le_data[558];

		/**
		 * UTF-16BE test string.
		 * Contains Latin-1, BMP, and SMP characters.
		 *
		 * This contains the same string as
		 * utf8_data[] and utf16le_data[].
		 *
		 * NOTE: This is encoded as uint8_t to prevent
		 * byteswapping issues.
		 */
		static const uint8_t utf16be_data[558];

		// Host-endian UTF-16 data for functions
		// that convert to/from host-endian UTF-16.
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
		#define utf16_data utf16le_data
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
		#define utf16_data utf16be_data
#endif

		/**
		 * Latin1 to UTF-8 test string.
		 * Contains the expected result from:
		 * - latin1_to_utf8(cp1252_data, ARRAY_SIZE(cp1252_data))
		 * (NOTE: Unsupported characters are replaced with '?'.)
		 */
		static const uint8_t latin1_utf8_data[346];

		/**
		 * Latin1 to UTF-16 test string.
		 * Contains the expected result from:
		 * - latin1_to_utf16(cp1252_data, ARRAY_SIZE(cp1252_data))
		 * (NOTE: Unsupported characters are replaced with '?'.)
		 */
		static const char16_t latin1_utf16_data[250];
};

// Test strings are located in TextFuncsTest_data.hpp.
#include "TextFuncsTest_data.hpp"

/**
 * Test cp1252_to_utf8().
 */
TEST_F(TextFuncsTest, cp1252_to_utf8)
{
	// Test with implicit length.
	string str = cp1252_to_utf8((const char*)cp1252_data, -1);
	EXPECT_EQ(str.size(), ARRAY_SIZE(cp1252_utf8_data)-1);
	EXPECT_EQ((const char*)cp1252_utf8_data, str);

	// Test with explicit length.
	str = cp1252_to_utf8((const char*)cp1252_data, ARRAY_SIZE(cp1252_data)-1);
	EXPECT_EQ(str.size(), ARRAY_SIZE(cp1252_utf8_data)-1);
	EXPECT_EQ((const char*)cp1252_utf8_data, str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cp1252_to_utf8((const char*)cp1252_data, ARRAY_SIZE(cp1252_data));
	EXPECT_EQ(str.size(), ARRAY_SIZE(cp1252_utf8_data)-1);
	EXPECT_EQ((const char*)cp1252_utf8_data, str);
}

/**
 * Test cp1252_to_utf16().
 */
TEST_F(TextFuncsTest, cp1252_to_utf16)
{
	// Test with implicit length.
	u16string str = cp1252_to_utf16((const char*)cp1252_data, -1);
	EXPECT_EQ(str.size(), ARRAY_SIZE(cp1252_utf16_data)-1);
	EXPECT_EQ(cp1252_utf16_data, str);

	// Test with explicit length.
	str = cp1252_to_utf16((const char*)cp1252_data, ARRAY_SIZE(cp1252_data)-1);
	EXPECT_EQ(str.size(), ARRAY_SIZE(cp1252_utf16_data)-1);
	EXPECT_EQ(cp1252_utf16_data, str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cp1252_to_utf16((const char*)cp1252_data, ARRAY_SIZE(cp1252_data));
	EXPECT_EQ(str.size(), ARRAY_SIZE(cp1252_utf16_data)-1);
	EXPECT_EQ(cp1252_utf16_data, str);
}

/**
 * Test cp1252_sjis_to_utf8() fallback functionality.
 * This string should be detected as cp1252 due to
 * Shift-JIS decoding errors.
 */
TEST_F(TextFuncsTest, cp1252_sjis_to_utf8_fallback)
{
	// Test with implicit length.
	string str = cp1252_sjis_to_utf8((const char*)cp1252_data, -1);
	EXPECT_EQ(str.size(), ARRAY_SIZE(cp1252_utf8_data)-1);
	EXPECT_EQ((const char*)cp1252_utf8_data, str);

	// Test with explicit length.
	str = cp1252_sjis_to_utf8((const char*)cp1252_data, ARRAY_SIZE(cp1252_data)-1);
	EXPECT_EQ(str.size(), ARRAY_SIZE(cp1252_utf8_data)-1);
	EXPECT_EQ((const char*)cp1252_utf8_data, str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cp1252_sjis_to_utf8((const char*)cp1252_data, ARRAY_SIZE(cp1252_data));
	EXPECT_EQ(str.size(), ARRAY_SIZE(cp1252_utf8_data)-1);
	EXPECT_EQ((const char*)cp1252_utf8_data, str);
}

/**
 * Test cp1252_sjis_to_utf8() fallback functionality.
 * This string is incorrectly detected as Shift-JIS because
 * all bytes are valid.
 */
TEST_F(TextFuncsTest, cp1252_sjis_to_utf8_copyright)
{
	// cp1252 code point 0xA9 is the copyright symbol,
	// but it's also halfwidth katakana "U" in Shift-JIS.
	const uint8_t sjis_in[16] = {
		0xA9,0x20,0x32,0x30,0x30,0x32,0x20,0x4E,
		0x69,0x6E,0x74,0x65,0x6E,0x64,0x6F,0x00
	};
	const uint8_t utf8_out[18] = {
		0xEF,0xBD,0xA9,0x20,0x32,0x30,0x30,0x32,
		0x20,0x4E,0x69,0x6E,0x74,0x65,0x6E,0x64,
		0x6F,0x00
	};

	// Test with implicit length.
	string str = cp1252_sjis_to_utf8((const char*)sjis_in, -1);
	EXPECT_EQ(str.size(), ARRAY_SIZE(utf8_out)-1);
	EXPECT_EQ((const char*)utf8_out, str);

	// Test with explicit length.
	str = cp1252_sjis_to_utf8((const char*)sjis_in, ARRAY_SIZE(sjis_in)-1);
	EXPECT_EQ(str.size(), ARRAY_SIZE(utf8_out)-1);
	EXPECT_EQ((const char*)utf8_out, str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cp1252_sjis_to_utf8((const char*)sjis_in, ARRAY_SIZE(sjis_in));
	EXPECT_EQ(str.size(), ARRAY_SIZE(utf8_out)-1);
	EXPECT_EQ((const char*)utf8_out, str);
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
	string str = cp1252_sjis_to_utf8(cp1252_in, -1);
	EXPECT_EQ(str.size(), ARRAY_SIZE(cp1252_in)-1);
	EXPECT_EQ(cp1252_in, str);

	// Test with explicit length.
	str = cp1252_sjis_to_utf8(cp1252_in, ARRAY_SIZE(cp1252_in)-1);
	EXPECT_EQ(str.size(), ARRAY_SIZE(cp1252_in)-1);
	EXPECT_EQ(cp1252_in, str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cp1252_sjis_to_utf8(cp1252_in, ARRAY_SIZE(cp1252_in));
	EXPECT_EQ(str.size(), ARRAY_SIZE(cp1252_in)-1);
	EXPECT_EQ(cp1252_in, str);
}

/**
 * Test cp1252_sjis_to_utf8() with Japanese text.
 * This includes a wave dash character (8160).
 */
TEST_F(TextFuncsTest, cp1252_sjis_to_utf8_japanese)
{
	// Test with implicit length.
	string str = cp1252_sjis_to_utf8((const char*)sjis_data, -1);
	EXPECT_EQ(str.size(), ARRAY_SIZE(sjis_utf8_data)-1);
	EXPECT_EQ((const char*)sjis_utf8_data, str);

	// Test with explicit length.
	str = cp1252_sjis_to_utf8((const char*)sjis_data, ARRAY_SIZE(sjis_data)-1);
	EXPECT_EQ(str.size(), ARRAY_SIZE(sjis_utf8_data)-1);
	EXPECT_EQ((const char*)sjis_utf8_data, str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cp1252_sjis_to_utf8((const char*)sjis_data, ARRAY_SIZE(sjis_data));
	EXPECT_EQ(str.size(), ARRAY_SIZE(sjis_utf8_data)-1);
	EXPECT_EQ((const char*)sjis_utf8_data, str);
}

/**
 * Test cp1252_sjis_to_utf16() fallback functionality.
 * This strings should be detected as cp1252 due to
 * Shift-JIS decoding errors.
 */
TEST_F(TextFuncsTest, cp1252_sjis_to_utf16_fallback)
{
	// Test with implicit length.
	u16string str = cp1252_sjis_to_utf16((const char*)cp1252_data, -1);
	EXPECT_EQ(str.size(), ARRAY_SIZE(cp1252_utf16_data)-1);
	EXPECT_EQ(cp1252_utf16_data, str);

	// Test with explicit length.
	str = cp1252_sjis_to_utf16((const char*)cp1252_data, ARRAY_SIZE(cp1252_data)-1);
	EXPECT_EQ(str.size(), ARRAY_SIZE(cp1252_utf16_data)-1);
	EXPECT_EQ(cp1252_utf16_data, str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cp1252_sjis_to_utf16((const char*)cp1252_data, ARRAY_SIZE(cp1252_data));
	EXPECT_EQ(str.size(), ARRAY_SIZE(cp1252_utf16_data)-1);
	EXPECT_EQ(cp1252_utf16_data, str);
}

/**
 * Test cp1252_sjis_to_utf8() fallback functionality.
 * This string is incorrectly detected as Shift-JIS because
 * all bytes are valid.
 */
TEST_F(TextFuncsTest, cp1252_sjis_to_utf16_copyright)
{
	// cp1252 code point 0xA9 is the copyright symbol,
	// but it's also halfwidth katakana "U" in Shift-JIS.
	const uint8_t sjis_in[16] = {
		0xA9,0x20,0x32,0x30,0x30,0x32,0x20,0x4E,
		0x69,0x6E,0x74,0x65,0x6E,0x64,0x6F,0x00
	};
	const char16_t utf16_out[16] = {
		0xFF69,0x0020,0x0032,0x0030,0x0030,0x0032,0x0020,0x004E,
		0x0069,0x006E,0x0074,0x0065,0x006E,0x0064,0x006F,0x0000
	};

	// Test with implicit length.
	u16string str = cp1252_sjis_to_utf16((const char*)sjis_in, -1);
	EXPECT_EQ(str.size(), ARRAY_SIZE(utf16_out)-1);
	EXPECT_EQ(utf16_out, str);

	// Test with explicit length.
	str = cp1252_sjis_to_utf16((const char*)sjis_in, ARRAY_SIZE(sjis_in)-1);
	EXPECT_EQ(str.size(), ARRAY_SIZE(utf16_out)-1);
	EXPECT_EQ(utf16_out, str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cp1252_sjis_to_utf16((const char*)sjis_in, ARRAY_SIZE(sjis_in));
	EXPECT_EQ(str.size(), ARRAY_SIZE(utf16_out)-1);
	EXPECT_EQ(utf16_out, str);
}

/**
 * Test cp1252_sjis_to_utf16() with ASCII strings.
 * Note that backslashes will *not* be converted to
 * yen symbols, so this should be a no-op.
 *
 * FIXME: Backslash may be converted to yen symbols
 * on Windows if the system has a Japanese locale.
 */
TEST_F(TextFuncsTest, cp1252_sjis_to_utf16_ascii)
{
	static const char cp1252_in[] = "C:\\Windows\\System32";
	// TODO: Hex and/or _RP_CHR()?
	static const char16_t u16_out[] = {
		'C',':','\\','W','i','n','d','o',
		'w','s','\\','S','y','s','t','e',
		'm','3','2',0
	};

	// Test with implicit length.
	u16string str = cp1252_sjis_to_utf16(cp1252_in, -1);
	EXPECT_EQ(str.size(), ARRAY_SIZE(u16_out)-1);
	EXPECT_EQ(u16_out, str);

	// Test with explicit length.
	str = cp1252_sjis_to_utf16(cp1252_in, ARRAY_SIZE(cp1252_in)-1);
	EXPECT_EQ(str.size(), ARRAY_SIZE(u16_out)-1);
	EXPECT_EQ(u16_out, str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cp1252_sjis_to_utf16(cp1252_in, ARRAY_SIZE(cp1252_in));
	EXPECT_EQ(str.size(), ARRAY_SIZE(u16_out)-1);
	EXPECT_EQ(u16_out, str);
}

/**
 * Test cp1252_sjis_to_utf16() with Japanese text.
 * This includes a wave dash character (8160).
 */
TEST_F(TextFuncsTest, cp1252_sjis_to_utf16_japanese)
{
	// Test with implicit length.
	u16string str = cp1252_sjis_to_utf16((const char*)sjis_data, -1);
	EXPECT_EQ(str.size(), ARRAY_SIZE(sjis_utf16_data)-1);
	EXPECT_EQ(sjis_utf16_data, str);

	// Test with explicit length.
	str = cp1252_sjis_to_utf16((const char*)sjis_data, ARRAY_SIZE(sjis_data)-1);
	EXPECT_EQ(str.size(), ARRAY_SIZE(sjis_utf16_data)-1);
	EXPECT_EQ(sjis_utf16_data, str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cp1252_sjis_to_utf16((const char*)sjis_data, ARRAY_SIZE(sjis_data));
	EXPECT_EQ(str.size(), ARRAY_SIZE(sjis_utf16_data)-1);
	EXPECT_EQ(sjis_utf16_data, str);
}

/**
 * Test utf8_to_utf16() with regular text and special characters.
 */
TEST_F(TextFuncsTest, utf8_to_utf16)
{
	// NOTE: The UTF-16 test strings are stored as
	// uint8_t arrays in order to prevent byteswapping
	// by the compiler.

	// Test with implicit length.
	u16string str = utf8_to_utf16((const char*)utf8_data, -1);
	EXPECT_EQ(str.size(), (sizeof(utf16_data)/sizeof(char16_t))-1);
	EXPECT_EQ((const char16_t*)utf16_data, str);

	// Test with explicit length.
	str = utf8_to_utf16((const char*)utf8_data, ARRAY_SIZE(utf8_data)-1);
	EXPECT_EQ(str.size(), (sizeof(utf16_data)/sizeof(char16_t))-1);
	EXPECT_EQ((const char16_t*)utf16_data, str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = utf8_to_utf16((const char*)utf8_data, ARRAY_SIZE(utf8_data));
	EXPECT_EQ(str.size(), (sizeof(utf16_data)/sizeof(char16_t))-1);
	EXPECT_EQ((const char16_t*)utf16_data, str);
}

/**
 * Test utf16le_to_utf8() with regular text and special characters.
 */
TEST_F(TextFuncsTest, utf16le_to_utf8)
{
	// NOTE: The UTF-16 test strings are stored as
	// uint8_t arrays in order to prevent byteswapping
	// by the compiler.

	// Test with implicit length.
	string str = utf16le_to_utf8((const char16_t*)utf16le_data, -1);
	EXPECT_EQ(str.size(), ARRAY_SIZE(utf8_data)-1);
	EXPECT_EQ((const char*)utf8_data, str);

	// Test with explicit length.
	str = utf16le_to_utf8((const char16_t*)utf16le_data, (sizeof(utf16_data)/sizeof(char16_t))-1);
	EXPECT_EQ(str.size(), ARRAY_SIZE(utf8_data)-1);
	EXPECT_EQ((const char*)utf8_data, str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = utf16le_to_utf8((const char16_t*)utf16le_data, (sizeof(utf16_data)/sizeof(char16_t)));
	EXPECT_EQ(str.size(), ARRAY_SIZE(utf8_data)-1);
	EXPECT_EQ((const char*)utf8_data, str);
}

/**
 * Test utf16be_to_utf8() with regular text and special characters.
 */
TEST_F(TextFuncsTest, utf16be_to_utf8)
{
	// NOTE: The UTF-16 test strings are stored as
	// uint8_t arrays in order to prevent byteswapping
	// by the compiler.

	// Test with implicit length.
	string str = utf16be_to_utf8((const char16_t*)utf16be_data, -1);
	EXPECT_EQ(str.size(), ARRAY_SIZE(utf8_data)-1);
	EXPECT_EQ((const char*)utf8_data, str);

	// Test with explicit length.
	str = utf16be_to_utf8((const char16_t*)utf16be_data, (sizeof(utf16_data)/sizeof(char16_t))-1);
	EXPECT_EQ(str.size(), ARRAY_SIZE(utf8_data)-1);
	EXPECT_EQ((const char*)utf8_data, str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = utf16be_to_utf8((const char16_t*)utf16be_data, (sizeof(utf16_data)/sizeof(char16_t)));
	EXPECT_EQ(str.size(), ARRAY_SIZE(utf8_data)-1);
	EXPECT_EQ((const char*)utf8_data, str);
}

/**
 * Test utf16_to_utf8() with regular text and special characters.
 * NOTE: This is effectively the same as the utf16le_to_utf8()
 * or utf16be_to_utf8() test, depending on system architecture.
 * This test ensures the byteorder macros are working correctly.
 */
TEST_F(TextFuncsTest, utf16_to_utf8)
{
	// NOTE: The UTF-16 test strings are stored as
	// uint8_t arrays in order to prevent byteswapping
	// by the compiler.

	// Test with implicit length.
	string str = utf16_to_utf8((const char16_t*)utf16_data, -1);
	EXPECT_EQ(str.size(), ARRAY_SIZE(utf8_data)-1);
	EXPECT_EQ((const char*)utf8_data, str);

	// Test with explicit length.
	str = utf16_to_utf8((const char16_t*)utf16_data, (sizeof(utf16_data)/sizeof(char16_t))-1);
	EXPECT_EQ(str.size(), ARRAY_SIZE(utf8_data)-1);
	EXPECT_EQ((const char*)utf8_data, str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = utf16_to_utf8((const char16_t*)utf16_data, (sizeof(utf16_data)/sizeof(char16_t)));
	EXPECT_EQ(str.size(), ARRAY_SIZE(utf8_data)-1);
	EXPECT_EQ((const char*)utf8_data, str);
}

/**
 * Test utf16_bswap() with regular text and special characters.
 * This function converts from BE to LE.
 */
TEST_F(TextFuncsTest, utf16_bswap_BEtoLE)
{
	// NOTE: The UTF-16 test strings are stored as
	// uint8_t arrays in order to prevent byteswapping
	// by the compiler.

	// Test with implicit length.
	u16string str = utf16_bswap((const char16_t*)utf16be_data, -1);
	EXPECT_EQ(str.size(), (sizeof(utf16le_data)/sizeof(char16_t))-1);
	EXPECT_EQ((const char16_t*)utf16le_data, str);

	// Test with explicit length.
	str = utf16_bswap((const char16_t*)utf16be_data, (sizeof(utf16be_data)/sizeof(char16_t))-1);
	EXPECT_EQ(str.size(), (sizeof(utf16le_data)/sizeof(char16_t))-1);
	EXPECT_EQ((const char16_t*)utf16le_data, str);

	// Test with explicit length and an extra NULL.
	// NOTE: utf16_bswap does NOT trim NULLs.
	str = utf16_bswap((const char16_t*)utf16be_data, (sizeof(utf16be_data)/sizeof(char16_t)));
	EXPECT_EQ(str.size(), (sizeof(utf16le_data)/sizeof(char16_t)));
	// Remove the extra NULL before comparing.
	str.resize(str.size()-1);
	EXPECT_EQ((const char16_t*)utf16le_data, str);
}

/**
 * Test utf16_bswap() with regular text and special characters.
 * This function converts from LE to BE.
 */
TEST_F(TextFuncsTest, utf16_bswap_LEtoBE)
{
	// NOTE: The UTF-16 test strings are stored as
	// uint8_t arrays in order to prevent byteswapping
	// by the compiler.

	// Test with implicit length.
	u16string str = utf16_bswap((const char16_t*)utf16le_data, -1);
	EXPECT_EQ(str.size(), (sizeof(utf16be_data)/sizeof(char16_t))-1);
	EXPECT_EQ((const char16_t*)utf16be_data, str);

	// Test with explicit length.
	str = utf16_bswap((const char16_t*)utf16le_data, (sizeof(utf16le_data)/sizeof(char16_t))-1);
	EXPECT_EQ(str.size(), (sizeof(utf16be_data)/sizeof(char16_t))-1);
	EXPECT_EQ((const char16_t*)utf16be_data, str);

	// Test with explicit length and an extra NULL.
	// NOTE: utf16_bswap does NOT trim NULLs.
	str = utf16_bswap((const char16_t*)utf16le_data, (sizeof(utf16le_data)/sizeof(char16_t)));
	EXPECT_EQ(str.size(), (sizeof(utf16be_data)/sizeof(char16_t)));
	// Remove the extra NULL before comparing.
	str.resize(str.size()-1);
	EXPECT_EQ((const char16_t*)utf16be_data, str);
}

/**
 * Test latin1_to_utf8().
 */
TEST_F(TextFuncsTest, latin1_to_utf8)
{
	// Test with implicit length.
	string str = latin1_to_utf8((const char*)cp1252_data, -1);
	EXPECT_EQ(str.size(), ARRAY_SIZE(latin1_utf8_data)-1);
	EXPECT_EQ((const char*)latin1_utf8_data, str);

	// Test with explicit length.
	str = latin1_to_utf8((const char*)cp1252_data, ARRAY_SIZE(cp1252_data)-1);
	EXPECT_EQ(str.size(), ARRAY_SIZE(latin1_utf8_data)-1);
	EXPECT_EQ((const char*)latin1_utf8_data, str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = latin1_to_utf8((const char*)cp1252_data, ARRAY_SIZE(cp1252_data));
	EXPECT_EQ(str.size(), ARRAY_SIZE(latin1_utf8_data)-1);
	EXPECT_EQ((const char*)latin1_utf8_data, str);
}

/**
 * Test latin1_to_utf16().
 */
TEST_F(TextFuncsTest, latin1_to_utf16)
{
	// Test with implicit length.
	u16string str = latin1_to_utf16((const char*)cp1252_data, -1);
	EXPECT_EQ(str.size(), ARRAY_SIZE(latin1_utf16_data)-1);
	EXPECT_EQ(latin1_utf16_data, str);

	// Test with explicit length.
	str = latin1_to_utf16((const char*)cp1252_data, ARRAY_SIZE(cp1252_data)-1);
	EXPECT_EQ(str.size(), ARRAY_SIZE(latin1_utf16_data)-1);
	EXPECT_EQ(latin1_utf16_data, str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = latin1_to_utf16((const char*)cp1252_data, ARRAY_SIZE(cp1252_data));
	EXPECT_EQ(str.size(), ARRAY_SIZE(latin1_utf16_data)-1);
	EXPECT_EQ(latin1_utf16_data, str);
}

/**
 * Test u16_strlen().
 */
TEST_F(TextFuncsTest, u16_strlen)
{
	// NOTE: u16_strlen() is a wrapper for wcslen() on Windows.
	// On all other systems, it's a simple implementation.

	// Compare to 8-bit strlen() with ASCII.
	static const char ascii_in[] = "abcdefghijklmnopqrstuvwxyz";
	static const char16_t u16_in[] = {
		'a','b','c','d','e','f','g','h','i','j','k','l',
		'm','n','o','p','q','r','s','t','u','v','w','x',
		'y','z',0
	};

	EXPECT_EQ(ARRAY_SIZE(ascii_in)-1, strlen(ascii_in));
	EXPECT_EQ(ARRAY_SIZE(u16_in)-1, u16_strlen(u16_in));
	EXPECT_EQ(u16_strlen(u16_in), strlen(ascii_in));

	// Test u16_strlen() with SMP characters.
	// u16_strlen() will return the number of 16-bit characters,
	// NOT the number of code points.
	static const char16_t u16smp_in[] = {
		0xD83C,0xDF4C,0xD83C,0xDF59,
		0xD83C,0xDF69,0xD83D,0xDCB5,
		0xD83D,0xDCBE,0x0000
	};
	EXPECT_EQ(ARRAY_SIZE(u16smp_in)-1, u16_strlen(u16smp_in));
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
