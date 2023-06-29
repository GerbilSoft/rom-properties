/***************************************************************************
 * ROM Properties Page shell extension. (librptext/tests)                  *
 * TextFuncsTest.cpp: Text conversion functions test                       *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Google Test
#include "gtest/gtest.h"
#include "tcharx.h"

// TextFuncs
#include "../conversion.hpp"
#include "../utf8_strlen.hpp"
#include "librpcpu/byteorder.h"
using namespace LibRpText;

// C includes. (C++ namespace)
#include <cstdio>
#include <cstring>

// C++ includes.
#include <string>
using std::string;
using std::u16string;

#define C8(x) reinterpret_cast<const char*>(x)
#define C16(x) reinterpret_cast<const char16_t*>(x)

#define C16_ARRAY_SIZE(x) (sizeof(x)/sizeof(char16_t))
#define C16_ARRAY_SIZE_I(x) static_cast<int>(sizeof(x)/sizeof(char16_t))

namespace LibRpBase { namespace Tests {

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
		 * - cp1252_to_utf8(cp1252_data, ARRAY_SIZE_I(cp1252_data))
		 * - cp1252_sjis_to_utf8(cp1252_data, ARRAY_SIZE_I(cp1252_data))
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
		 * - cp1252_sjis_to_utf8(sjis_data, ARRAY_SIZE_I(sjis_data))
		 */
		static const uint8_t sjis_utf8_data[53];

		/**
		 * Shift-JIS to UTF-16 test string.
		 * Contains the expected result from:
		 * - cp1252_sjis_to_utf16(sjis_data, ARRAY_SIZE_I(sjis_data))
		 */
		static const char16_t sjis_utf16_data[19];

		/**
		 * Shift-JIS test string with a cp1252 copyright symbol. (0xA9)
		 * This string is incorrectly detected as Shift-JIS because
		 * all bytes are valid.
		 */
		static const uint8_t sjis_copyright_in[16];

		/**
		 * UTF-8 result from:
		 * - cp1252_sjis_to_utf8(sjis_copyright_in, sizeof(sjis_copyright_in))
		 */
		static const uint8_t sjis_copyright_out_utf8[18];

		/**
		 * UTF-16 result from:
		 * - cp1252_sjis_to_utf16(sjis_copyright_in, sizeof(sjis_copyright_in))
		 */
		static const char16_t sjis_copyright_out_utf16[16];

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
		 * Latin-1 to UTF-8 test string.
		 * Contains the expected result from:
		 * - latin1_to_utf8(cp1252_data, ARRAY_SIZE_I(cp1252_data))
		 *
		 * This includes the C1 control codes, as per the Unicode Latin-1 Supplement:
		 * https://en.wikipedia.org/wiki/Latin-1_Supplement_(Unicode_block)
		 */
		static const uint8_t latin1_utf8_data[371+1];

		/**
		 * Latin-1 to UTF-16 test string.
		 * Contains the expected result from:
		 * - latin1_to_utf16(cp1252_data, ARRAY_SIZE_I(cp1252_data))
		 *
		 * This includes the C1 control codes, as per the Unicode Latin-1 Supplement:
		 * https://en.wikipedia.org/wiki/Latin-1_Supplement_(Unicode_block)
		 */
		static const char16_t latin1_utf16_data[249+1];

		/** Specialized code page functions. **/

		/**
		 * Atari ST to UTF-8 test string.
		 * Contains all Atari ST characters that can be converted to Unicode.
		 */
		static const char atariST_data[236+1];

		/**
		 * Atari ST to UTF-16 test string.
		 * Contains the expected result from:
		 * - utf8_to_utf16(cpN_to_utf8(CP_RP_ATARIST, atariST_data, ARRAY_SIZE_I(atariST_data)))
		 */
		static const char16_t atariST_utf16_data[236+1];

		/**
		 * Atari ATASCII to UTF-8 test string.
		 * Contains all Atari ATASCII characters that can be converted to Unicode.
		 */
		static const char atascii_data[229+1];

		/**
		 * Atari ATASCII to UTF-16 test string.
		 * Contains the expected result from:
		 * - utf8_to_utf16(cpN_to_utf8(CP_RP_ATASCII, atascii_data, ARRAY_SIZE_I(atascii_data)-1))
		 */
		static const char16_t atascii_utf16_data[229+1];
};

// Test strings are located in TextFuncsTest_data.hpp.
#include "TextFuncsTest_data.hpp"

/** Code Page 1252 **/

/**
 * Test cp1252_to_utf8().
 */
TEST_F(TextFuncsTest, cp1252_to_utf8)
{
	// Test with implicit length.
	string str = cp1252_to_utf8(C8(cp1252_data), -1);
	EXPECT_EQ(ARRAY_SIZE(cp1252_utf8_data)-1, str.size());
	EXPECT_EQ(C8(cp1252_utf8_data), str);

	// Test with explicit length.
	str = cp1252_to_utf8(C8(cp1252_data), ARRAY_SIZE_I(cp1252_data)-1);
	EXPECT_EQ(ARRAY_SIZE(cp1252_utf8_data)-1, str.size());
	EXPECT_EQ(C8(cp1252_utf8_data), str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cp1252_to_utf8(C8(cp1252_data), ARRAY_SIZE_I(cp1252_data));
	EXPECT_EQ(ARRAY_SIZE(cp1252_utf8_data)-1, str.size());
	EXPECT_EQ(C8(cp1252_utf8_data), str);
}

/**
 * Test cp1252_to_utf16().
 */
TEST_F(TextFuncsTest, cp1252_to_utf16)
{
	// Test with implicit length.
	u16string str = cp1252_to_utf16(C8(cp1252_data), -1);
	EXPECT_EQ(ARRAY_SIZE(cp1252_utf16_data)-1, str.size());
	EXPECT_EQ(cp1252_utf16_data, str);

	// Test with explicit length.
	str = cp1252_to_utf16(C8(cp1252_data), ARRAY_SIZE_I(cp1252_data)-1);
	EXPECT_EQ(ARRAY_SIZE(cp1252_utf16_data)-1, str.size());
	EXPECT_EQ(cp1252_utf16_data, str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cp1252_to_utf16(C8(cp1252_data), ARRAY_SIZE_I(cp1252_data));
	EXPECT_EQ(ARRAY_SIZE(cp1252_utf16_data)-1, str.size());
	EXPECT_EQ(cp1252_utf16_data, str);
}

/** Code Page 1252 + Shift-JIS (932) **/

/**
 * Test cp1252_sjis_to_utf8() fallback functionality.
 * This string should be detected as cp1252 due to
 * Shift-JIS decoding errors.
 */
TEST_F(TextFuncsTest, cp1252_sjis_to_utf8_fallback)
{
	// Test with implicit length.
	string str = cp1252_sjis_to_utf8(C8(cp1252_data), -1);
	EXPECT_EQ(ARRAY_SIZE(cp1252_utf8_data)-1, str.size());
	EXPECT_EQ(C8(cp1252_utf8_data), str);

	// Test with explicit length.
	str = cp1252_sjis_to_utf8(C8(cp1252_data), ARRAY_SIZE_I(cp1252_data)-1);
	EXPECT_EQ(ARRAY_SIZE(cp1252_utf8_data)-1, str.size());
	EXPECT_EQ(C8(cp1252_utf8_data), str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cp1252_sjis_to_utf8(C8(cp1252_data), ARRAY_SIZE_I(cp1252_data));
	EXPECT_EQ(ARRAY_SIZE(cp1252_utf8_data)-1, str.size());
	EXPECT_EQ(C8(cp1252_utf8_data), str);
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

	// Test with implicit length.
	string str = cp1252_sjis_to_utf8(C8(sjis_copyright_in), -1);
	EXPECT_EQ(ARRAY_SIZE(sjis_copyright_out_utf8)-1, str.size());
	EXPECT_EQ(C8(sjis_copyright_out_utf8), str);

	// Test with explicit length.
	str = cp1252_sjis_to_utf8(C8(sjis_copyright_in), ARRAY_SIZE_I(sjis_copyright_in)-1);
	EXPECT_EQ(ARRAY_SIZE(sjis_copyright_out_utf8)-1, str.size());
	EXPECT_EQ(C8(sjis_copyright_out_utf8), str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cp1252_sjis_to_utf8(C8(sjis_copyright_in), ARRAY_SIZE_I(sjis_copyright_in));
	EXPECT_EQ(ARRAY_SIZE(sjis_copyright_out_utf8)-1, str.size());
	EXPECT_EQ(C8(sjis_copyright_out_utf8), str);
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
	EXPECT_EQ(ARRAY_SIZE(cp1252_in)-1, str.size());
	EXPECT_EQ(cp1252_in, str);

	// Test with explicit length.
	str = cp1252_sjis_to_utf8(cp1252_in, ARRAY_SIZE_I(cp1252_in)-1);
	EXPECT_EQ(ARRAY_SIZE(cp1252_in)-1, str.size());
	EXPECT_EQ(cp1252_in, str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cp1252_sjis_to_utf8(cp1252_in, ARRAY_SIZE_I(cp1252_in));
	EXPECT_EQ(ARRAY_SIZE(cp1252_in)-1, str.size());
	EXPECT_EQ(cp1252_in, str);
}

/**
 * Test cp1252_sjis_to_utf8() with Japanese text.
 * This includes a wave dash character (8160).
 */
TEST_F(TextFuncsTest, cp1252_sjis_to_utf8_japanese)
{
	// Test with implicit length.
	string str = cp1252_sjis_to_utf8(C8(sjis_data), -1);
	EXPECT_EQ(ARRAY_SIZE(sjis_utf8_data)-1, str.size());
	EXPECT_EQ(C8(sjis_utf8_data), str);

	// Test with explicit length.
	str = cp1252_sjis_to_utf8(C8(sjis_data), ARRAY_SIZE_I(sjis_data)-1);
	EXPECT_EQ(ARRAY_SIZE(sjis_utf8_data)-1, str.size());
	EXPECT_EQ(C8(sjis_utf8_data), str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cp1252_sjis_to_utf8(C8(sjis_data), ARRAY_SIZE_I(sjis_data));
	EXPECT_EQ(ARRAY_SIZE(sjis_utf8_data)-1, str.size());
	EXPECT_EQ(C8(sjis_utf8_data), str);
}

/**
 * Test cp1252_sjis_to_utf16() fallback functionality.
 * This strings should be detected as cp1252 due to
 * Shift-JIS decoding errors.
 */
TEST_F(TextFuncsTest, cp1252_sjis_to_utf16_fallback)
{
	// Test with implicit length.
	u16string str = cp1252_sjis_to_utf16(C8(cp1252_data), -1);
	EXPECT_EQ(ARRAY_SIZE(cp1252_utf16_data)-1, str.size());
	EXPECT_EQ(cp1252_utf16_data, str);

	// Test with explicit length.
	str = cp1252_sjis_to_utf16(C8(cp1252_data), ARRAY_SIZE_I(cp1252_data)-1);
	EXPECT_EQ(ARRAY_SIZE(cp1252_utf16_data)-1, str.size());
	EXPECT_EQ(cp1252_utf16_data, str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cp1252_sjis_to_utf16(C8(cp1252_data), ARRAY_SIZE_I(cp1252_data));
	EXPECT_EQ(ARRAY_SIZE(cp1252_utf16_data)-1, str.size());
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

	// Test with implicit length.
	u16string str = cp1252_sjis_to_utf16(C8(sjis_copyright_in), -1);
	EXPECT_EQ(ARRAY_SIZE(sjis_copyright_out_utf16)-1, str.size());
	EXPECT_EQ(sjis_copyright_out_utf16, str);

	// Test with explicit length.
	str = cp1252_sjis_to_utf16(C8(sjis_copyright_in), ARRAY_SIZE_I(sjis_copyright_in)-1);
	EXPECT_EQ(ARRAY_SIZE(sjis_copyright_out_utf16)-1, str.size());
	EXPECT_EQ(sjis_copyright_out_utf16, str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cp1252_sjis_to_utf16(C8(sjis_copyright_in), ARRAY_SIZE_I(sjis_copyright_in));
	EXPECT_EQ(ARRAY_SIZE(sjis_copyright_out_utf16)-1, str.size());
	EXPECT_EQ(sjis_copyright_out_utf16, str);
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

	// NOTE: Need to manually initialize the char16_t[] array
	// due to the way _RP() is implemented for versions of
	// MSVC older than 2015.
	static const char16_t utf16_out[] = {
		'C',':','\\','W','i','n','d','o',
		'w','s','\\','S','y','s','t','e',
		'm','3','2',0
	};

	// Test with implicit length.
	u16string str = cp1252_sjis_to_utf16(cp1252_in, -1);
	EXPECT_EQ(ARRAY_SIZE(utf16_out)-1, str.size());
	EXPECT_EQ(utf16_out, str);

	// Test with explicit length.
	str = cp1252_sjis_to_utf16(cp1252_in, ARRAY_SIZE_I(cp1252_in)-1);
	EXPECT_EQ(ARRAY_SIZE(utf16_out)-1, str.size());
	EXPECT_EQ(utf16_out, str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cp1252_sjis_to_utf16(cp1252_in, ARRAY_SIZE_I(cp1252_in));
	EXPECT_EQ(ARRAY_SIZE(utf16_out)-1, str.size());
	EXPECT_EQ(utf16_out, str);
}

/**
 * Test cp1252_sjis_to_utf16() with Japanese text.
 * This includes a wave dash character (8160).
 */
TEST_F(TextFuncsTest, cp1252_sjis_to_utf16_japanese)
{
	// Test with implicit length.
	u16string str = cp1252_sjis_to_utf16(C8(sjis_data), -1);
	EXPECT_EQ(ARRAY_SIZE(sjis_utf16_data)-1, str.size());
	EXPECT_EQ(sjis_utf16_data, str);

	// Test with explicit length.
	str = cp1252_sjis_to_utf16(C8(sjis_data), ARRAY_SIZE_I(sjis_data)-1);
	EXPECT_EQ(ARRAY_SIZE(sjis_utf16_data)-1, str.size());
	EXPECT_EQ(sjis_utf16_data, str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cp1252_sjis_to_utf16(C8(sjis_data), ARRAY_SIZE_I(sjis_data));
	EXPECT_EQ(ARRAY_SIZE(sjis_utf16_data)-1, str.size());
	EXPECT_EQ(sjis_utf16_data, str);
}

/** UTF-8 to UTF-16 and vice-versa **/

/**
 * Test utf8_to_utf16() with regular text and special characters.
 */
TEST_F(TextFuncsTest, utf8_to_utf16)
{
	// NOTE: The UTF-16 test strings are stored as
	// uint8_t arrays in order to prevent byteswapping
	// by the compiler.

	// Test with implicit length.
	u16string str = utf8_to_utf16(C8(utf8_data), -1);
	EXPECT_EQ(C16_ARRAY_SIZE(utf16_data)-1, str.size());
	EXPECT_EQ(C16(utf16_data), str);

	// Test with explicit length.
	str = utf8_to_utf16(C8(utf8_data), ARRAY_SIZE_I(utf8_data)-1);
	EXPECT_EQ(C16_ARRAY_SIZE(utf16_data)-1, str.size());
	EXPECT_EQ(C16(utf16_data), str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = utf8_to_utf16(C8(utf8_data), ARRAY_SIZE_I(utf8_data));
	EXPECT_EQ(C16_ARRAY_SIZE(utf16_data)-1, str.size());
	EXPECT_EQ(C16(utf16_data), str);
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
	string str = utf16le_to_utf8(C16(utf16le_data), -1);
	EXPECT_EQ(ARRAY_SIZE(utf8_data)-1, str.size());
	EXPECT_EQ(C8(utf8_data), str);

	// Test with explicit length.
	str = utf16le_to_utf8(C16(utf16le_data), C16_ARRAY_SIZE_I(utf16_data)-1);
	EXPECT_EQ(ARRAY_SIZE(utf8_data)-1, str.size());
	EXPECT_EQ(C8(utf8_data), str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = utf16le_to_utf8(C16(utf16le_data), C16_ARRAY_SIZE_I(utf16_data));
	EXPECT_EQ(ARRAY_SIZE(utf8_data)-1, str.size());
	EXPECT_EQ(C8(utf8_data), str);
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
	string str = utf16be_to_utf8(C16(utf16be_data), -1);
	EXPECT_EQ(ARRAY_SIZE(utf8_data)-1, str.size());
	EXPECT_EQ(C8(utf8_data), str);

	// Test with explicit length.
	str = utf16be_to_utf8(C16(utf16be_data), C16_ARRAY_SIZE_I(utf16_data)-1);
	EXPECT_EQ(ARRAY_SIZE(utf8_data)-1, str.size());
	EXPECT_EQ(C8(utf8_data), str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = utf16be_to_utf8(C16(utf16be_data), C16_ARRAY_SIZE_I(utf16_data));
	EXPECT_EQ(ARRAY_SIZE(utf8_data)-1, str.size());
	EXPECT_EQ(C8(utf8_data), str);
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
	string str = utf16_to_utf8(C16(utf16_data), -1);
	EXPECT_EQ(ARRAY_SIZE(utf8_data)-1, str.size());
	EXPECT_EQ(C8(utf8_data), str);

	// Test with explicit length.
	str = utf16_to_utf8(C16(utf16_data), C16_ARRAY_SIZE_I(utf16_data)-1);
	EXPECT_EQ(ARRAY_SIZE(utf8_data)-1, str.size());
	EXPECT_EQ(C8(utf8_data), str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = utf16_to_utf8(C16(utf16_data), C16_ARRAY_SIZE_I(utf16_data));
	EXPECT_EQ(ARRAY_SIZE(utf8_data)-1, str.size());
	EXPECT_EQ(C8(utf8_data), str);
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
	u16string str = utf16_bswap(C16(utf16be_data), -1);
	EXPECT_EQ(C16_ARRAY_SIZE(utf16le_data)-1, str.size());
	EXPECT_EQ(C16(utf16le_data), str);

	// Test with explicit length.
	str = utf16_bswap(C16(utf16be_data), C16_ARRAY_SIZE_I(utf16be_data)-1);
	EXPECT_EQ(C16_ARRAY_SIZE(utf16be_data)-1, str.size());
	EXPECT_EQ(C16(utf16le_data), str);

	// Test with explicit length and an extra NULL.
	// NOTE: utf16_bswap does NOT trim NULLs.
	str = utf16_bswap(C16(utf16be_data), C16_ARRAY_SIZE_I(utf16be_data));
	EXPECT_EQ(C16_ARRAY_SIZE(utf16le_data), str.size());
	// Remove the extra NULL before comparing.
	str.resize(str.size()-1);
	EXPECT_EQ(C16(utf16le_data), str);
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
	u16string str = utf16_bswap(C16(utf16le_data), -1);
	EXPECT_EQ(C16_ARRAY_SIZE(utf16be_data)-1, str.size());
	EXPECT_EQ(C16(utf16be_data), str);

	// Test with explicit length.
	str = utf16_bswap(C16(utf16le_data), C16_ARRAY_SIZE_I(utf16le_data)-1);
	EXPECT_EQ(C16_ARRAY_SIZE(utf16be_data)-1, str.size());
	EXPECT_EQ(C16(utf16be_data), str);

	// Test with explicit length and an extra NULL.
	// NOTE: utf16_bswap does NOT trim NULLs.
	str = utf16_bswap(C16(utf16le_data), C16_ARRAY_SIZE_I(utf16le_data));
	EXPECT_EQ(C16_ARRAY_SIZE(utf16be_data), str.size());
	// Remove the extra NULL before comparing.
	str.resize(str.size()-1);
	EXPECT_EQ(C16(utf16be_data), str);
}

/** Latin-1 (ISO-8859-1) **/

// NOTE: latin1_to_*() functions now act like cp1252.
// Use the cpN_to_*() functions instead.

/**
 * Test latin1_to_utf8().
 */
TEST_F(TextFuncsTest, latin1_to_utf8)
{
	// Test with implicit length.
	string str = cpN_to_utf8(CP_LATIN1, C8(cp1252_data), -1);
	EXPECT_EQ(ARRAY_SIZE(latin1_utf8_data)-1, str.size());
	EXPECT_EQ(C8(latin1_utf8_data), str);

	// Test with explicit length.
	str = cpN_to_utf8(CP_LATIN1, C8(cp1252_data), ARRAY_SIZE_I(cp1252_data)-1);
	EXPECT_EQ(ARRAY_SIZE(latin1_utf8_data)-1, str.size());
	EXPECT_EQ(C8(latin1_utf8_data), str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cpN_to_utf8(CP_LATIN1, C8(cp1252_data), ARRAY_SIZE_I(cp1252_data));
	EXPECT_EQ(ARRAY_SIZE(latin1_utf8_data)-1, str.size());
	EXPECT_EQ(C8(latin1_utf8_data), str);
}

/**
 * Test latin1_to_utf16().
 */
TEST_F(TextFuncsTest, latin1_to_utf16)
{
	// Test with implicit length.
	u16string str = cpN_to_utf16(CP_LATIN1, C8(cp1252_data), -1);
	EXPECT_EQ(ARRAY_SIZE(latin1_utf16_data)-1, str.size());
	EXPECT_EQ(latin1_utf16_data, str);

	// Test with explicit length.
	str = cpN_to_utf16(CP_LATIN1, C8(cp1252_data), ARRAY_SIZE_I(cp1252_data)-1);
	EXPECT_EQ(ARRAY_SIZE(latin1_utf16_data)-1, str.size());
	EXPECT_EQ(latin1_utf16_data, str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cpN_to_utf16(CP_LATIN1, C8(cp1252_data), ARRAY_SIZE_I(cp1252_data));
	EXPECT_EQ(ARRAY_SIZE(latin1_utf16_data)-1, str.size());
	EXPECT_EQ(latin1_utf16_data, str);
}

/**
 * Test utf8_to_latin1().
 */
TEST_F(TextFuncsTest, utf8_to_latin1)
{
	// Test with implicit length.
	string str = utf8_to_latin1(C8(latin1_utf8_data), -1);
	EXPECT_EQ(ARRAY_SIZE(cp1252_data)-1, str.size());
	EXPECT_EQ(C8(cp1252_data), str);

	// Test with explicit length.
	str = utf8_to_latin1(C8(latin1_utf8_data), ARRAY_SIZE_I(latin1_utf8_data)-1);
	EXPECT_EQ(ARRAY_SIZE(cp1252_data)-1, str.size());
	EXPECT_EQ(C8(cp1252_data), str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = utf8_to_latin1(C8(latin1_utf8_data), ARRAY_SIZE_I(latin1_utf8_data));
	EXPECT_EQ(ARRAY_SIZE(cp1252_data)-1, str.size());
	EXPECT_EQ(C8(cp1252_data), str);
}

/**
 * Test utf16_to_latin1().
 */
TEST_F(TextFuncsTest, utf16_to_latin1)
{
	// Test with implicit length.
	string str = utf16_to_latin1(C16(latin1_utf16_data), -1);
	EXPECT_EQ(ARRAY_SIZE(cp1252_data)-1, str.size());
	EXPECT_EQ(C8(cp1252_data), str);

	// Test with explicit length.
	str = utf16_to_latin1(C16(latin1_utf16_data), ARRAY_SIZE_I(latin1_utf16_data)-1);
	EXPECT_EQ(ARRAY_SIZE(cp1252_data)-1, str.size());
	EXPECT_EQ(C8(cp1252_data), str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = utf16_to_latin1(C16(latin1_utf16_data), ARRAY_SIZE(latin1_utf16_data));
	EXPECT_EQ(ARRAY_SIZE(cp1252_data)-1, str.size());
	EXPECT_EQ(C8(cp1252_data), str);
}

/** Miscellaneous functions. **/

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
	EXPECT_EQ(strlen(ascii_in), u16_strlen(u16_in));

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

/**
 * Test u16_strdup().
 */
TEST_F(TextFuncsTest, u16_strdup)
{
	// NOTE: u16_strdup() is a wrapper for wcsdup() on Windows.
	// On all other systems, it's a simple implementation.

	// Test string.
	static const char16_t u16_str[] = {
		'T','h','e',' ','q','u','i','c','k',' ','b','r',
		'o','w','n',' ','f','o','x',' ','j','u','m','p',
		's',' ','o','v','e','r',' ','t','h','e',' ','l',
		'a','z','y',' ','d','o','g','.',0
	};

	char16_t *const u16_dup = u16_strdup(u16_str);
	ASSERT_TRUE(u16_dup != nullptr);

	// Verify the NULL terminator.
	EXPECT_EQ(0, u16_dup[ARRAY_SIZE(u16_str)-1]);
	if (u16_dup[ARRAY_SIZE(u16_str)-1] != 0) {
		// NULL terminator not found.
		// u16_strlen() and u16_strcmp() may crash,
		// so exit early.
		// NOTE: We can't use ASSERT_EQ() because we
		// have to free the u16_strdup()'d string.
		free(u16_dup);
		return;
	}

	// Verify the string length.
	EXPECT_EQ(ARRAY_SIZE(u16_str)-1, u16_strlen(u16_dup));

	// Verify the string contents.
	// NOTE: EXPECT_STREQ() supports const wchar_t*,
	// but not const char16_t*.
	EXPECT_EQ(0, u16_strcmp(u16_str, u16_dup));

	free(u16_dup);
}

/**
 * Test u16_strcmp().
 */
TEST_F(TextFuncsTest, u16_strcmp)
{
	// NOTE: u16_strcmp() is a wrapper for wcscmp() on Windows.
	// On all other systems, it's a simple implementation.

	// Three test strings.
	// TODO: Update these strings so they would fail if tested
	// using u16_strcasecmp().
	static const char16_t u16_str1[] = {'a','b','c','d','e','f','g',0};
	static const char16_t u16_str2[] = {'a','b','d','e','f','g','h',0};
	static const char16_t u16_str3[] = {'d','e','f','g','h','i','j',0};

	// Compare strings to themselves.
	EXPECT_EQ(0, u16_strcmp(u16_str1, u16_str1));
	EXPECT_EQ(0, u16_strcmp(u16_str2, u16_str2));
	EXPECT_EQ(0, u16_strcmp(u16_str3, u16_str3));

	// Compare strings to each other.
	EXPECT_LT(u16_strcmp(u16_str1, u16_str2), 0);
	EXPECT_LT(u16_strcmp(u16_str1, u16_str3), 0);
	EXPECT_GT(u16_strcmp(u16_str2, u16_str1), 0);
	EXPECT_LT(u16_strcmp(u16_str2, u16_str3), 0);
	EXPECT_GT(u16_strcmp(u16_str3, u16_str1), 0);
	EXPECT_GT(u16_strcmp(u16_str3, u16_str2), 0);
}

/**
 * Test u16_strcasecmp().
 */
TEST_F(TextFuncsTest, u16_strcasecmp)
{
	// NOTE: u16_strcasecmp() is a wrapper for wcsicmp() on Windows.
	// On all other systems, it's a simple implementation.

	// Three test strings.
	static const char16_t u16_str1[] = {'A','b','C','d','E','f','G',0};
	static const char16_t u16_str2[] = {'a','B','d','E','f','G','h',0};
	static const char16_t u16_str3[] = {'D','e','F','g','H','i','J',0};

	// Compare strings to themselves.
	EXPECT_EQ(0, u16_strcasecmp(u16_str1, u16_str1));
	EXPECT_EQ(0, u16_strcasecmp(u16_str2, u16_str2));
	EXPECT_EQ(0, u16_strcasecmp(u16_str3, u16_str3));

	// Compare strings to each other.
	EXPECT_LT(u16_strcasecmp(u16_str1, u16_str2), 0);
	EXPECT_LT(u16_strcasecmp(u16_str1, u16_str3), 0);
	EXPECT_GT(u16_strcasecmp(u16_str2, u16_str1), 0);
	EXPECT_LT(u16_strcasecmp(u16_str2, u16_str3), 0);
	EXPECT_GT(u16_strcasecmp(u16_str3, u16_str1), 0);
	EXPECT_GT(u16_strcasecmp(u16_str3, u16_str2), 0);
}

/** Specialized code page functions. **/

TEST_F(TextFuncsTest, atariST_to_utf8)
{
	// This tests all code points that can be converted from the
	// Atari ST character set to Unicode.
	// Reference: https://en.wikipedia.org/wiki/Atari_ST_character_set

	// Test with implicit length.
	string str = cpN_to_utf8(CP_RP_ATARIST, atariST_data, -1);
	u16string u16str = utf8_to_utf16(str);
	EXPECT_EQ(ARRAY_SIZE(atariST_utf16_data)-1, u16str.size());
	EXPECT_EQ(atariST_utf16_data, u16str);

	// Test with explicit length.
	str = cpN_to_utf8(CP_RP_ATARIST, atariST_data, ARRAY_SIZE(atariST_data)-1);
	u16str = utf8_to_utf16(str);
	EXPECT_EQ(ARRAY_SIZE(atariST_utf16_data)-1, u16str.size());
	EXPECT_EQ(atariST_utf16_data, u16str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cpN_to_utf8(CP_RP_ATARIST, atariST_data, ARRAY_SIZE(atariST_data));
	u16str = utf8_to_utf16(str);
	EXPECT_EQ(ARRAY_SIZE(atariST_utf16_data)-1, u16str.size());
	EXPECT_EQ(atariST_utf16_data, u16str);
}

TEST_F(TextFuncsTest, atascii_to_utf8)
{
	// This tests all code points that can be converted from the
	// Atari ATASCII character set to Unicode.
	// Reference: https://en.wikipedia.org/wiki/ATASCII

	// Test with implicit length.
	// NOTE: We have to skip the first character, 0x00, because
	// implicit length mode would interpret that as an empty string.
	string str = cpN_to_utf8(CP_RP_ATASCII, &atascii_data[1], -1);
	u16string u16str = utf8_to_utf16(str);
	EXPECT_EQ(ARRAY_SIZE(atascii_utf16_data)-2, u16str.size());
	EXPECT_EQ(&atascii_utf16_data[1], u16str);

	// Test with explicit length.
	str = cpN_to_utf8(CP_RP_ATASCII, atascii_data, ARRAY_SIZE(atascii_data)-1);
	u16str = utf8_to_utf16(str);
	EXPECT_EQ(ARRAY_SIZE(atascii_utf16_data)-1, u16str.size());
	EXPECT_EQ(atascii_utf16_data, u16str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cpN_to_utf8(CP_RP_ATASCII, C8(atascii_data), ARRAY_SIZE(atascii_data));
	u16str = utf8_to_utf16(str);
	EXPECT_EQ(ARRAY_SIZE(atascii_utf16_data)-1, u16str.size());
	EXPECT_EQ(atascii_utf16_data, u16str);
}

/** Other text functions **/

/**
 * Test utf8_disp_strlen().
 */
TEST_F(TextFuncsTest, utf8_disp_strlen)
{
       // utf8_disp_strlen() should be identical to strlen() for ASCII text.
       static const char ascii_text[] = "abc123xyz789";
       EXPECT_EQ(strlen(ascii_text), utf8_disp_strlen(ascii_text));

       // Test string with 2-byte UTF-8 code points. (U+0080 - U+07FF)
       static const char utf8_2byte_text[] = "Ακρόπολη";
       EXPECT_EQ(16, strlen(utf8_2byte_text));
       EXPECT_EQ(8, utf8_disp_strlen(utf8_2byte_text));

       // Test string with 3-byte UTF-8 code points. (U+0800 - U+FFFF)
       static const char utf8_3byte_text[] = "╔╗╚╝┼";
       EXPECT_EQ(15, strlen(utf8_3byte_text));
       EXPECT_EQ(5, utf8_disp_strlen(utf8_3byte_text));

       // Test string with 4-byte UTF-8 code points. (U+10000 - U+10FFFF)
       static const char utf8_4byte_text[] = "😂🙄💾🖬";
       EXPECT_EQ(16, strlen(utf8_4byte_text));
       EXPECT_EQ(4, utf8_disp_strlen(utf8_4byte_text));
}

/**
 * Test dos2unix().
 */
TEST_F(TextFuncsTest, dos2unix)
{
	int lf_count;

	static const char expected_lf[] = "The quick brown fox\njumps over\nthe lazy dog.";
	static const char test1[] = "The quick brown fox\r\njumps over\r\nthe lazy dog.";
	static const char expected_lf2[] = "The quick brown fox\njumps over\nthe lazy dog.\n";
	static const char test2[] = "The quick brown fox\r\njumps over\r\nthe lazy dog.\r\n";
	static const char test3[] = "The quick brown fox\r\njumps over\r\nthe lazy dog.\r";
	static const char test4[] = "The quick brown fox\rjumps over\rthe lazy dog.\r";
	static const char test5[] = "The quick brown fox\njumps over\rthe lazy dog.\r";

	// Basic conversion. (no trailing newline sequence)
	lf_count = 0;
	EXPECT_EQ(expected_lf, dos2unix(test1, -1, &lf_count));
	EXPECT_EQ(2, lf_count);

	// Trailing "\r\n"
	lf_count = 0;
	EXPECT_EQ(expected_lf2, dos2unix(test2, -1, &lf_count));
	EXPECT_EQ(3, lf_count);

	// Trailing '\r' should be converted to '\n'.
	lf_count = 0;
	EXPECT_EQ(expected_lf2, dos2unix(test3, -1, &lf_count));
	EXPECT_EQ(3, lf_count);

	// All standalone '\r' characters should be converted to '\n'.
	lf_count = 0;
	EXPECT_EQ(expected_lf2, dos2unix(test4, -1, &lf_count));
	EXPECT_EQ(3, lf_count);

	// Existing standalone '\n' should be counted but not changed.
	lf_count = 0;
	EXPECT_EQ(expected_lf2, dos2unix(test5, -1, &lf_count));
	EXPECT_EQ(3, lf_count);
}

/** Audio functions **/

/**
 * Test formatSampleAsTime().
 */
TEST_F(TextFuncsTest, formatSampleAsTime)
{
	// TODO: More variations?

	// Do whole seconds conversion for: 11, 16, 22, 24, 44, 48 kHz
	EXPECT_EQ("0:03.00", formatSampleAsTime(11025U*3U, 11025U));
	EXPECT_EQ("0:03.00", formatSampleAsTime(16000U*3U, 16000U));
	EXPECT_EQ("0:03.00", formatSampleAsTime(22050U*3U, 22050U));
	EXPECT_EQ("0:03.00", formatSampleAsTime(24000U*3U, 24000U));
	EXPECT_EQ("0:03.00", formatSampleAsTime(44100U*3U, 44100U));
	EXPECT_EQ("0:03.00", formatSampleAsTime(48000U*3U, 48000U));

	// Add a quarter second and see how things go.
	// NOTE: A few of these end up returning "0:03:24" due to rounding issues.
	EXPECT_EQ("0:03.24", formatSampleAsTime(11025U*3.25, 11025U));
	EXPECT_EQ("0:03.25", formatSampleAsTime(16000U*3.25, 16000U));
	EXPECT_EQ("0:03.24", formatSampleAsTime(22050U*3.25, 22050U));
	EXPECT_EQ("0:03.25", formatSampleAsTime(24000U*3.25, 24000U));
	EXPECT_EQ("0:03.25", formatSampleAsTime(44100U*3.25, 44100U));
	EXPECT_EQ("0:03.25", formatSampleAsTime(48000U*3.25, 48000U));

	/// Add two minutes

	// Do whole seconds conversion for: 11, 16, 22, 24, 44, 48 kHz
	EXPECT_EQ("2:03.00", formatSampleAsTime(11025U*123U, 11025U));
	EXPECT_EQ("2:03.00", formatSampleAsTime(16000U*123U, 16000U));
	EXPECT_EQ("2:03.00", formatSampleAsTime(22050U*123U, 22050U));
	EXPECT_EQ("2:03.00", formatSampleAsTime(24000U*123U, 24000U));
	EXPECT_EQ("2:03.00", formatSampleAsTime(44100U*123U, 44100U));
	EXPECT_EQ("2:03.00", formatSampleAsTime(48000U*123U, 48000U));

	// Add a quarter second and see how things go.
	// NOTE: A few of these end up returning "0:03:24" due to rounding issues.
	EXPECT_EQ("2:03.24", formatSampleAsTime(11025U*123.25, 11025U));
	EXPECT_EQ("2:03.25", formatSampleAsTime(16000U*123.25, 16000U));
	EXPECT_EQ("2:03.24", formatSampleAsTime(22050U*123.25, 22050U));
	EXPECT_EQ("2:03.25", formatSampleAsTime(24000U*123.25, 24000U));
	EXPECT_EQ("2:03.25", formatSampleAsTime(44100U*123.25, 44100U));
	EXPECT_EQ("2:03.25", formatSampleAsTime(48000U*123.25, 48000U));
}

/**
 * Test convSampleToMs().
 */
TEST_F(TextFuncsTest, convSampleToMs)
{
	// Do whole seconds conversion for: 11, 16, 22, 24, 44, 48 kHz
	EXPECT_EQ(3000U, convSampleToMs(11025U*3U, 11025U));
	EXPECT_EQ(3000U, convSampleToMs(16000U*3U, 16000U));
	EXPECT_EQ(3000U, convSampleToMs(22050U*3U, 22050U));
	EXPECT_EQ(3000U, convSampleToMs(24000U*3U, 24000U));
	EXPECT_EQ(3000U, convSampleToMs(44100U*3U, 44100U));
	EXPECT_EQ(3000U, convSampleToMs(48000U*3U, 48000U));

	// Add a quarter second and see how things go.
	// NOTE: A few of these end up returning 3249 due to rounding issues.
	EXPECT_EQ(3249U, convSampleToMs(11025U*3.25, 11025U));
	EXPECT_EQ(3250U, convSampleToMs(16000U*3.25, 16000U));
	EXPECT_EQ(3249U, convSampleToMs(22050U*3.25, 22050U));
	EXPECT_EQ(3250U, convSampleToMs(24000U*3.25, 24000U));
	EXPECT_EQ(3250U, convSampleToMs(44100U*3.25, 44100U));
	EXPECT_EQ(3250U, convSampleToMs(48000U*3.25, 48000U));
}

} }

/**
 * Test suite main function.
 */
extern "C" int gtest_main(int argc, TCHAR *argv[])
{
	fputs("LibRpText test suite: TextFuncs tests.\n\n", stderr);
	fflush(nullptr);

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
