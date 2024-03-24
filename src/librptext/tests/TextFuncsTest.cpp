/***************************************************************************
 * ROM Properties Page shell extension. (librptext/tests)                  *
 * TextFuncsTest.cpp: Text conversion functions test                       *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Google Test
#include "gtest/gtest.h"
#include "tcharx.h"

// TextFuncs
#include "../conversion.hpp"
#include "../utf8_strlen.hpp"
#include "librpbyteswap/byteorder.h"
using namespace LibRpText;

// C includes (C++ namespace)
#include <cstdio>
#include <cstring>

// C++ includes
#include <array>
#include <string>
using std::array;
using std::string;
using std::u16string;

#define C8(x) reinterpret_cast<const char*>(x.data())
#define C16(x) reinterpret_cast<const char16_t*>(x.data())

#define C16_ARRAY_SIZE(x) (sizeof(x)/sizeof(char16_t))
#define C16_ARRAY_SIZE_I(x) static_cast<int>(sizeof(x)/sizeof(char16_t))

namespace LibRpBase { namespace Tests {

class TextFuncsTest : public ::testing::Test
{
	protected:
		TextFuncsTest() = default;

	public:
		// NOTE: 8-bit test strings are unsigned in order to prevent
		// narrowing conversion warnings from appearing.
		// char16_t is defined as unsigned, so this isn't a problem
		// for 16-bit strings.

		/**
		 * cp1252 test string.
		 * Contains all possible cp1252 characters.
		 */
		static const array<uint8_t, 250> cp1252_data;

		/**
		 * cp1252 to UTF-8 test string.
		 * Contains the expected result from:
		 * - cp1252_to_utf8(cp1252_data, ARRAY_SIZE_I(cp1252_data))
		 * - cp1252_sjis_to_utf8(cp1252_data, ARRAY_SIZE_I(cp1252_data))
		 */
		static const array<uint8_t, 388> cp1252_utf8_data;

		/**
		 * cp1252 to UTF-16 test string.
		 * Contains the expected result from:
		 * - cp1252_to_utf16(cp1252_data, sizeof(cp1252_data))
		 * - cp1252_sjis_to_utf16(cp1252_data, sizeof(cp1252_data))
		 */
		static const array<char16_t, 250> cp1252_utf16_data;

		/**
		 * Shift-JIS test string.
		 *
		 * TODO: Get a longer test string.
		 * This string is from the JP Pokemon Colosseum (GCN) save file,
		 * plus a wave dash character (8160).
		 */
		static const array<uint8_t, 36> sjis_data;

		/**
		 * Shift-JIS to UTF-8 test string.
		 * Contains the expected result from:
		 * - cp1252_sjis_to_utf8(sjis_data, ARRAY_SIZE_I(sjis_data))
		 */
		static const array<uint8_t, 53> sjis_utf8_data;

		/**
		 * Shift-JIS to UTF-16 test string.
		 * Contains the expected result from:
		 * - cp1252_sjis_to_utf16(sjis_data, ARRAY_SIZE_I(sjis_data))
		 */
		static const array<char16_t, 19> sjis_utf16_data;

		/**
		 * Shift-JIS test string with a cp1252 copyright symbol. (0xA9)
		 * This string is incorrectly detected as Shift-JIS because
		 * all bytes are valid.
		 */
		static const array<uint8_t, 16> sjis_copyright_in;

		/**
		 * UTF-8 result from:
		 * - cp1252_sjis_to_utf8(sjis_copyright_in, sizeof(sjis_copyright_in))
		 */
		static const array<uint8_t, 18> sjis_copyright_out_utf8;

		/**
		 * UTF-16 result from:
		 * - cp1252_sjis_to_utf16(sjis_copyright_in, sizeof(sjis_copyright_in))
		 */
		static const array<char16_t, 16> sjis_copyright_out_utf16;

		/**
		 * UTF-8 test string.
		 * Contains Latin-1, BMP, and SMP characters.
		 *
		 * This contains the same string as
		 * utf16le_data[] and utf16be_data[].
		 */
		static const array<uint8_t, 325> utf8_data;

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
		static const array<uint8_t, 558> utf16le_data;

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
		static const array<uint8_t, 558> utf16be_data;

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
		static const array<uint8_t, 371+1> latin1_utf8_data;

		/**
		 * Latin-1 to UTF-16 test string.
		 * Contains the expected result from:
		 * - latin1_to_utf16(cp1252_data, ARRAY_SIZE_I(cp1252_data))
		 *
		 * This includes the C1 control codes, as per the Unicode Latin-1 Supplement:
		 * https://en.wikipedia.org/wiki/Latin-1_Supplement_(Unicode_block)
		 */
		static const array<char16_t, 249+1> latin1_utf16_data;

		/** Specialized code page functions. **/

		/**
		 * Atari ST to UTF-8 test string.
		 * Contains all Atari ST characters that can be converted to Unicode.
		 */
		static const array<uint8_t, 236+1> atariST_data;

		/**
		 * Atari ST to UTF-16 test string.
		 * Contains the expected result from:
		 * - utf8_to_utf16(cpN_to_utf8(CP_RP_ATARIST, atariST_data, ARRAY_SIZE_I(atariST_data)))
		 */
		static const array<char16_t, 236+1> atariST_utf16_data;

		/**
		 * Atari ATASCII to UTF-8 test string.
		 * Contains all Atari ATASCII characters that can be converted to Unicode.
		 */
		static const array<uint8_t, 229+1> atascii_data;

		/**
		 * Atari ATASCII to UTF-16 test string.
		 * Contains the expected result from:
		 * - utf8_to_utf16(cpN_to_utf8(CP_RP_ATASCII, atascii_data, ARRAY_SIZE_I(atascii_data)-1))
		 */
		static const array<char16_t, 229+1> atascii_utf16_data;
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
	EXPECT_EQ(cp1252_utf8_data.size()-1, str.size());
	EXPECT_EQ(C8(cp1252_utf8_data), str);

	// Test with explicit length.
	str = cp1252_to_utf8(C8(cp1252_data), cp1252_data.size()-1);
	EXPECT_EQ(cp1252_utf8_data.size()-1, str.size());
	EXPECT_EQ(C8(cp1252_utf8_data), str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cp1252_to_utf8(C8(cp1252_data), cp1252_data.size());
	EXPECT_EQ(cp1252_utf8_data.size()-1, str.size());
	EXPECT_EQ(C8(cp1252_utf8_data), str);

	// Test with std::string source data.
	string src(C8(cp1252_data));
	EXPECT_EQ(cp1252_data.size()-1, src.size());
	str = cp1252_to_utf8(src);
	EXPECT_EQ(cp1252_utf8_data.size()-1, str.size());
	EXPECT_EQ(C8(cp1252_utf8_data), str);

	// Test with std::string source data and an extra NULL.
	// The extra NULL should be trimmed.
	src.assign(C8(cp1252_data), cp1252_data.size());
	EXPECT_EQ(cp1252_data.size(), src.size());
	str = cp1252_to_utf8(src);
	EXPECT_EQ(cp1252_utf8_data.size()-1, str.size());
	EXPECT_EQ(C8(cp1252_utf8_data), str);
}

/**
 * Test cp1252_to_utf16().
 */
TEST_F(TextFuncsTest, cp1252_to_utf16)
{
	// Test with implicit length.
	u16string str = cp1252_to_utf16(C8(cp1252_data), -1);
	EXPECT_EQ(cp1252_utf16_data.size()-1, str.size());
	EXPECT_EQ(cp1252_utf16_data.data(), str);

	// Test with explicit length.
	str = cp1252_to_utf16(C8(cp1252_data), cp1252_data.size()-1);
	EXPECT_EQ(cp1252_utf16_data.size()-1, str.size());
	EXPECT_EQ(cp1252_utf16_data.data(), str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cp1252_to_utf16(C8(cp1252_data), cp1252_data.size());
	EXPECT_EQ(cp1252_utf16_data.size()-1, str.size());
	EXPECT_EQ(cp1252_utf16_data.data(), str);
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
	EXPECT_EQ(cp1252_utf8_data.size()-1, str.size());
	EXPECT_EQ(C8(cp1252_utf8_data), str);

	// Test with explicit length.
	str = cp1252_sjis_to_utf8(C8(cp1252_data), cp1252_data.size()-1);
	EXPECT_EQ(cp1252_utf8_data.size()-1, str.size());
	EXPECT_EQ(C8(cp1252_utf8_data), str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cp1252_sjis_to_utf8(C8(cp1252_data), cp1252_data.size());
	EXPECT_EQ(cp1252_utf8_data.size()-1, str.size());
	EXPECT_EQ(C8(cp1252_utf8_data), str);

	// Test with std::string source data.
	string src(C8(cp1252_data));
	EXPECT_EQ(cp1252_data.size()-1, src.size());
	str = cp1252_sjis_to_utf8(src);
	EXPECT_EQ(cp1252_utf8_data.size()-1, str.size());
	EXPECT_EQ(C8(cp1252_utf8_data), str);

	// Test with std::string source data and an extra NULL.
	// The extra NULL should be trimmed.
	src.assign(C8(cp1252_data), cp1252_data.size());
	EXPECT_EQ(cp1252_data.size(), src.size());
	str = cp1252_sjis_to_utf8(src);
	EXPECT_EQ(cp1252_utf8_data.size()-1, str.size());
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
	EXPECT_EQ(sjis_copyright_out_utf8.size()-1, str.size());
	EXPECT_EQ(C8(sjis_copyright_out_utf8), str);

	// Test with explicit length.
	str = cp1252_sjis_to_utf8(C8(sjis_copyright_in), sjis_copyright_in.size()-1);
	EXPECT_EQ(sjis_copyright_out_utf8.size()-1, str.size());
	EXPECT_EQ(C8(sjis_copyright_out_utf8), str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cp1252_sjis_to_utf8(C8(sjis_copyright_in), sjis_copyright_in.size());
	EXPECT_EQ(sjis_copyright_out_utf8.size()-1, str.size());
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
	EXPECT_EQ(sjis_utf8_data.size()-1, str.size());
	EXPECT_EQ(C8(sjis_utf8_data), str);

	// Test with explicit length.
	str = cp1252_sjis_to_utf8(C8(sjis_data), sjis_data.size()-1);
	EXPECT_EQ(sjis_utf8_data.size()-1, str.size());
	EXPECT_EQ(C8(sjis_utf8_data), str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cp1252_sjis_to_utf8(C8(sjis_data), sjis_data.size());
	EXPECT_EQ(sjis_utf8_data.size()-1, str.size());
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
	EXPECT_EQ(cp1252_utf16_data.size()-1, str.size());
	EXPECT_EQ(cp1252_utf16_data.data(), str);

	// Test with explicit length.
	str = cp1252_sjis_to_utf16(C8(cp1252_data), cp1252_data.size()-1);
	EXPECT_EQ(cp1252_utf16_data.size()-1, str.size());
	EXPECT_EQ(cp1252_utf16_data.data(), str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cp1252_sjis_to_utf16(C8(cp1252_data), cp1252_data.size());
	EXPECT_EQ(cp1252_utf16_data.size()-1, str.size());
	EXPECT_EQ(cp1252_utf16_data.data(), str);
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
	EXPECT_EQ(sjis_copyright_out_utf16.size()-1, str.size());
	EXPECT_EQ(sjis_copyright_out_utf16.data(), str);

	// Test with explicit length.
	str = cp1252_sjis_to_utf16(C8(sjis_copyright_in), sjis_copyright_in.size()-1);
	EXPECT_EQ(sjis_copyright_out_utf16.size()-1, str.size());
	EXPECT_EQ(sjis_copyright_out_utf16.data(), str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cp1252_sjis_to_utf16(C8(sjis_copyright_in), sjis_copyright_in.size());
	EXPECT_EQ(sjis_copyright_out_utf16.size()-1, str.size());
	EXPECT_EQ(sjis_copyright_out_utf16.data(), str);
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
	static const array<char16_t, 19+1> utf16_out = {{
		'C',':','\\','W','i','n','d','o',
		'w','s','\\','S','y','s','t','e',
		'm','3','2',0
	}};

	// Test with implicit length.
	u16string str = cp1252_sjis_to_utf16(cp1252_in, -1);
	EXPECT_EQ(utf16_out.size()-1, str.size());
	EXPECT_EQ(utf16_out.data(), str);

	// Test with explicit length.
	str = cp1252_sjis_to_utf16(cp1252_in, ARRAY_SIZE_I(cp1252_in)-1);
	EXPECT_EQ(utf16_out.size()-1, str.size());
	EXPECT_EQ(utf16_out.data(), str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cp1252_sjis_to_utf16(cp1252_in, ARRAY_SIZE_I(cp1252_in));
	EXPECT_EQ(utf16_out.size()-1, str.size());
	EXPECT_EQ(utf16_out.data(), str);
}

/**
 * Test cp1252_sjis_to_utf16() with Japanese text.
 * This includes a wave dash character (8160).
 */
TEST_F(TextFuncsTest, cp1252_sjis_to_utf16_japanese)
{
	// Test with implicit length.
	u16string str = cp1252_sjis_to_utf16(C8(sjis_data), -1);
	EXPECT_EQ(sjis_utf16_data.size()-1, str.size());
	EXPECT_EQ(sjis_utf16_data.data(), str);

	// Test with explicit length.
	str = cp1252_sjis_to_utf16(C8(sjis_data), sjis_data.size()-1);
	EXPECT_EQ(sjis_utf16_data.size()-1, str.size());
	EXPECT_EQ(sjis_utf16_data.data(), str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cp1252_sjis_to_utf16(C8(sjis_data), sjis_data.size());
	EXPECT_EQ(sjis_utf16_data.size()-1, str.size());
	EXPECT_EQ(sjis_utf16_data.data(), str);
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
	str = utf8_to_utf16(C8(utf8_data), utf8_data.size()-1);
	EXPECT_EQ(C16_ARRAY_SIZE(utf16_data)-1, str.size());
	EXPECT_EQ(C16(utf16_data), str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = utf8_to_utf16(C8(utf8_data), utf8_data.size());
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
	EXPECT_EQ(utf8_data.size()-1, str.size());
	EXPECT_EQ(C8(utf8_data), str);

	// Test with explicit length.
	str = utf16le_to_utf8(C16(utf16le_data), C16_ARRAY_SIZE_I(utf16_data)-1);
	EXPECT_EQ(utf8_data.size()-1, str.size());
	EXPECT_EQ(C8(utf8_data), str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = utf16le_to_utf8(C16(utf16le_data), C16_ARRAY_SIZE_I(utf16_data));
	EXPECT_EQ(utf8_data.size()-1, str.size());
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
	EXPECT_EQ(utf8_data.size()-1, str.size());
	EXPECT_EQ(C8(utf8_data), str);

	// Test with explicit length.
	str = utf16be_to_utf8(C16(utf16be_data), C16_ARRAY_SIZE_I(utf16_data)-1);
	EXPECT_EQ(utf8_data.size()-1, str.size());
	EXPECT_EQ(C8(utf8_data), str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = utf16be_to_utf8(C16(utf16be_data), C16_ARRAY_SIZE_I(utf16_data));
	EXPECT_EQ(utf8_data.size()-1, str.size());
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
	EXPECT_EQ(utf8_data.size()-1, str.size());
	EXPECT_EQ(C8(utf8_data), str);

	// Test with explicit length.
	str = utf16_to_utf8(C16(utf16_data), C16_ARRAY_SIZE_I(utf16_data)-1);
	EXPECT_EQ(utf8_data.size()-1, str.size());
	EXPECT_EQ(C8(utf8_data), str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = utf16_to_utf8(C16(utf16_data), C16_ARRAY_SIZE_I(utf16_data));
	EXPECT_EQ(utf8_data.size()-1, str.size());
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
	EXPECT_EQ(latin1_utf8_data.size()-1, str.size());
	EXPECT_EQ(C8(latin1_utf8_data), str);

	// Test with explicit length.
	str = cpN_to_utf8(CP_LATIN1, C8(cp1252_data), cp1252_data.size()-1);
	EXPECT_EQ(latin1_utf8_data.size()-1, str.size());
	EXPECT_EQ(C8(latin1_utf8_data), str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cpN_to_utf8(CP_LATIN1, C8(cp1252_data), cp1252_data.size());
	EXPECT_EQ(latin1_utf8_data.size()-1, str.size());
	EXPECT_EQ(C8(latin1_utf8_data), str);
}

/**
 * Test latin1_to_utf16().
 */
TEST_F(TextFuncsTest, latin1_to_utf16)
{
	// Test with implicit length.
	u16string str = cpN_to_utf16(CP_LATIN1, C8(cp1252_data), -1);
	EXPECT_EQ(latin1_utf16_data.size()-1, str.size());
	EXPECT_EQ(latin1_utf16_data.data(), str);

	// Test with explicit length.
	str = cpN_to_utf16(CP_LATIN1, C8(cp1252_data), cp1252_data.size()-1);
	EXPECT_EQ(latin1_utf16_data.size()-1, str.size());
	EXPECT_EQ(latin1_utf16_data.data(), str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cpN_to_utf16(CP_LATIN1, C8(cp1252_data), cp1252_data.size());
	EXPECT_EQ(latin1_utf16_data.size()-1, str.size());
	EXPECT_EQ(latin1_utf16_data.data(), str);
}

/**
 * Test utf8_to_latin1().
 */
TEST_F(TextFuncsTest, utf8_to_latin1)
{
	// Test with implicit length.
	string str = utf8_to_latin1(C8(latin1_utf8_data), -1);
	EXPECT_EQ(cp1252_data.size()-1, str.size());
	EXPECT_EQ(C8(cp1252_data), str);

	// Test with explicit length.
	str = utf8_to_latin1(C8(latin1_utf8_data), latin1_utf8_data.size()-1);
	EXPECT_EQ(cp1252_data.size()-1, str.size());
	EXPECT_EQ(C8(cp1252_data), str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = utf8_to_latin1(C8(latin1_utf8_data), latin1_utf8_data.size());
	EXPECT_EQ(cp1252_data.size()-1, str.size());
	EXPECT_EQ(C8(cp1252_data), str);

	// Test with std::string source data.
	string src(C8(latin1_utf8_data));
	EXPECT_EQ(latin1_utf8_data.size()-1, src.size());
	str = utf8_to_latin1(src);
	EXPECT_EQ(cp1252_data.size()-1, str.size());
	EXPECT_EQ(C8(cp1252_data), str);

	// Test with std::string source data and an extra NULL.
	// The extra NULL should be trimmed.
	src.assign(C8(latin1_utf8_data), latin1_utf8_data.size());
	EXPECT_EQ(latin1_utf8_data.size(), src.size());
	str = utf8_to_latin1(src);
	EXPECT_EQ(cp1252_data.size()-1, str.size());
	EXPECT_EQ(C8(cp1252_data), str);
}

/**
 * Test utf16_to_latin1().
 */
TEST_F(TextFuncsTest, utf16_to_latin1)
{
	// Test with implicit length.
	string str = utf16_to_latin1(C16(latin1_utf16_data), -1);
	EXPECT_EQ(cp1252_data.size()-1, str.size());
	EXPECT_EQ(C8(cp1252_data), str);

	// Test with explicit length.
	str = utf16_to_latin1(C16(latin1_utf16_data), latin1_utf16_data.size()-1);
	EXPECT_EQ(cp1252_data.size()-1, str.size());
	EXPECT_EQ(C8(cp1252_data), str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = utf16_to_latin1(C16(latin1_utf16_data), latin1_utf16_data.size());
	EXPECT_EQ(cp1252_data.size()-1, str.size());
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
	static const array<char16_t, 26+1> u16_in = {{
		'a','b','c','d','e','f','g','h','i','j','k','l',
		'm','n','o','p','q','r','s','t','u','v','w','x',
		'y','z',0
	}};

	EXPECT_EQ(ARRAY_SIZE(ascii_in)-1, strlen(ascii_in));
	EXPECT_EQ(u16_in.size()-1, u16_strlen(u16_in.data()));
	EXPECT_EQ(strlen(ascii_in), u16_strlen(u16_in.data()));

	// Test u16_strlen() with SMP characters.
	// u16_strlen() will return the number of 16-bit characters,
	// NOT the number of code points.
	static const array<char16_t, 10+1> u16smp_in = {{
		0xD83C,0xDF4C,0xD83C,0xDF59,
		0xD83C,0xDF69,0xD83D,0xDCB5,
		0xD83D,0xDCBE,0x0000
	}};
	EXPECT_EQ(u16smp_in.size()-1, u16_strlen(u16smp_in.data()));
}

/**
 * Test u16_strdup().
 */
TEST_F(TextFuncsTest, u16_strdup)
{
	// NOTE: u16_strdup() is a wrapper for wcsdup() on Windows.
	// On all other systems, it's a simple implementation.

	// Test string.
	static const array<char16_t, 44+1> u16_str = {{
		'T','h','e',' ','q','u','i','c','k',' ','b','r',
		'o','w','n',' ','f','o','x',' ','j','u','m','p',
		's',' ','o','v','e','r',' ','t','h','e',' ','l',
		'a','z','y',' ','d','o','g','.',0
	}};

	char16_t *const u16_dup = u16_strdup(u16_str.data());
	ASSERT_TRUE(u16_dup != nullptr);

	// Verify the NULL terminator.
	EXPECT_EQ(0, u16_dup[u16_str.size()-1]);
	if (u16_dup[u16_str.size()-1] != 0) {
		// NULL terminator not found.
		// u16_strlen() and u16_strcmp() may crash,
		// so exit early.
		// NOTE: We can't use ASSERT_EQ() because we
		// have to free the u16_strdup()'d string.
		free(u16_dup);
		return;
	}

	// Verify the string length.
	EXPECT_EQ(u16_str.size()-1, u16_strlen(u16_dup));

	// Verify the string contents.
	// NOTE: EXPECT_STREQ() supports const wchar_t*,
	// but not const char16_t*.
	EXPECT_EQ(0, u16_strcmp(u16_str.data(), u16_dup));

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
	static const array<char16_t, 8> u16_str1 = {{'a','b','c','d','e','f','g',0}};
	static const array<char16_t, 8> u16_str2 = {{'a','b','d','e','f','g','h',0}};
	static const array<char16_t, 8> u16_str3 = {{'d','e','f','g','h','i','j',0}};

	// Compare strings to themselves.
	EXPECT_EQ(0, u16_strcmp(u16_str1.data(), u16_str1.data()));
	EXPECT_EQ(0, u16_strcmp(u16_str2.data(), u16_str2.data()));
	EXPECT_EQ(0, u16_strcmp(u16_str3.data(), u16_str3.data()));

	// Compare strings to each other.
	EXPECT_LT(u16_strcmp(u16_str1.data(), u16_str2.data()), 0);
	EXPECT_LT(u16_strcmp(u16_str1.data(), u16_str3.data()), 0);
	EXPECT_GT(u16_strcmp(u16_str2.data(), u16_str1.data()), 0);
	EXPECT_LT(u16_strcmp(u16_str2.data(), u16_str3.data()), 0);
	EXPECT_GT(u16_strcmp(u16_str3.data(), u16_str1.data()), 0);
	EXPECT_GT(u16_strcmp(u16_str3.data(), u16_str2.data()), 0);
}

/**
 * Test u16_strcasecmp().
 */
TEST_F(TextFuncsTest, u16_strcasecmp)
{
	// NOTE: u16_strcasecmp() is a wrapper for wcsicmp() on Windows.
	// On all other systems, it's a simple implementation.

	// Three test strings.
	static const array<char16_t, 8> u16_str1 = {{'A','b','C','d','E','f','G',0}};
	static const array<char16_t, 8> u16_str2 = {{'a','B','d','E','f','G','h',0}};
	static const array<char16_t, 8> u16_str3 = {{'D','e','F','g','H','i','J',0}};

	// Compare strings to themselves.
	EXPECT_EQ(0, u16_strcasecmp(u16_str1.data(), u16_str1.data()));
	EXPECT_EQ(0, u16_strcasecmp(u16_str2.data(), u16_str2.data()));
	EXPECT_EQ(0, u16_strcasecmp(u16_str3.data(), u16_str3.data()));

	// Compare strings to each other.
	EXPECT_LT(u16_strcasecmp(u16_str1.data(), u16_str2.data()), 0);
	EXPECT_LT(u16_strcasecmp(u16_str1.data(), u16_str3.data()), 0);
	EXPECT_GT(u16_strcasecmp(u16_str2.data(), u16_str1.data()), 0);
	EXPECT_LT(u16_strcasecmp(u16_str2.data(), u16_str3.data()), 0);
	EXPECT_GT(u16_strcasecmp(u16_str3.data(), u16_str1.data()), 0);
	EXPECT_GT(u16_strcasecmp(u16_str3.data(), u16_str2.data()), 0);
}

/** Specialized code page functions. **/

TEST_F(TextFuncsTest, atariST_to_utf8)
{
	// This tests all code points that can be converted from the
	// Atari ST character set to Unicode.
	// Reference: https://en.wikipedia.org/wiki/Atari_ST_character_set

	// Test with implicit length.
	string str = cpN_to_utf8(CP_RP_ATARIST, C8(atariST_data), -1);
	u16string u16str = utf8_to_utf16(str);
	EXPECT_EQ(atariST_utf16_data.size()-1, u16str.size());
	EXPECT_EQ(atariST_utf16_data.data(), u16str);

	// Test with explicit length.
	str = cpN_to_utf8(CP_RP_ATARIST, C8(atariST_data), atariST_data.size()-1);
	u16str = utf8_to_utf16(str);
	EXPECT_EQ(ARRAY_SIZE(atariST_utf16_data)-1, u16str.size());
	EXPECT_EQ(atariST_utf16_data.data(), u16str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cpN_to_utf8(CP_RP_ATARIST, C8(atariST_data), atariST_data.size());
	u16str = utf8_to_utf16(str);
	EXPECT_EQ(ARRAY_SIZE(atariST_utf16_data)-1, u16str.size());
	EXPECT_EQ(atariST_utf16_data.data(), u16str);
}

TEST_F(TextFuncsTest, atascii_to_utf8)
{
	// This tests all code points that can be converted from the
	// Atari ATASCII character set to Unicode.
	// Reference: https://en.wikipedia.org/wiki/ATASCII

	// Test with implicit length.
	// NOTE: We have to skip the first character, 0x00, because
	// implicit length mode would interpret that as an empty string.
	string str = cpN_to_utf8(CP_RP_ATASCII, &C8(atascii_data)[1], -1);
	u16string u16str = utf8_to_utf16(str);
	EXPECT_EQ(atascii_utf16_data.size()-2, u16str.size());
	EXPECT_EQ(&atascii_utf16_data[1], u16str);

	// Test with explicit length.
	str = cpN_to_utf8(CP_RP_ATASCII, C8(atascii_data), atascii_data.size()-1);
	u16str = utf8_to_utf16(str);
	EXPECT_EQ(atascii_utf16_data.size()-1, u16str.size());
	EXPECT_EQ(atascii_utf16_data.data(), u16str);

	// Test with explicit length and an extra NULL.
	// The extra NULL should be trimmed.
	str = cpN_to_utf8(CP_RP_ATASCII, C8(atascii_data), atascii_data.size());
	u16str = utf8_to_utf16(str);
	EXPECT_EQ(atascii_utf16_data.size()-1, u16str.size());
	EXPECT_EQ(atascii_utf16_data.data(), u16str);
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
	EXPECT_EQ(16U, strlen(utf8_2byte_text));
	EXPECT_EQ(8U, utf8_disp_strlen(utf8_2byte_text));

	// Test string with 3-byte UTF-8 code points. (U+0800 - U+FFFF)
	static const char utf8_3byte_text[] = "╔╗╚╝┼";
	EXPECT_EQ(15U, strlen(utf8_3byte_text));
	EXPECT_EQ(5U, utf8_disp_strlen(utf8_3byte_text));

#ifndef _WIN32
	// Test string with 4-byte UTF-8 code points. (U+10000 - U+10FFFF)
	// FIXME: Broken on Windows... (returns 7)
	static const char utf8_4byte_text[] = "😂🙄💾🖬";
	EXPECT_EQ(16U, strlen(utf8_4byte_text));
	EXPECT_EQ(4U, utf8_disp_strlen(utf8_4byte_text));
#endif /* !_WIN32 */
}

/**
 * Test formatFileSize().
 */
TEST_F(TextFuncsTest, formatFileSize)
{
	// NOTE: Due to LC_ALL="C", use standard binary sizes. (KiB, MiB, etc)
	// Testing 512, 768, 1024, 1536, 2048, 2560, and 3072 for each order of magnitude.

	// Special cases
	EXPECT_EQ("-1", formatFileSize(-1LL));	// negative: print as-is
	EXPECT_EQ("0 bytes", formatFileSize(0LL));
	EXPECT_EQ("1 byte", formatFileSize(1LL));
	EXPECT_EQ("2 bytes", formatFileSize(2LL));

	// Kilobyte
	EXPECT_EQ("512 bytes", formatFileSize(512LL));
	EXPECT_EQ("768 bytes", formatFileSize(768LL));
	EXPECT_EQ("1024 bytes", formatFileSize(1024LL));
	EXPECT_EQ("1536 bytes", formatFileSize(1536LL));
	EXPECT_EQ("2.00 KiB", formatFileSize(2048LL));
	EXPECT_EQ("2.50 KiB", formatFileSize(2560LL));
	EXPECT_EQ("3.00 KiB", formatFileSize(3072LL));

	// Megabyte
	EXPECT_EQ("512.0 KiB", formatFileSize(512LL*1024));
	EXPECT_EQ("768.0 KiB", formatFileSize(768LL*1024));
	EXPECT_EQ("1024.0 KiB", formatFileSize(1024LL*1024));
	EXPECT_EQ("1536.0 KiB", formatFileSize(1536LL*1024));
	EXPECT_EQ("2.00 MiB", formatFileSize(2048LL*1024));
	EXPECT_EQ("2.50 MiB", formatFileSize(2560LL*1024));
	EXPECT_EQ("3.00 MiB", formatFileSize(3072LL*1024));

	// Gigabyte
	EXPECT_EQ("512.0 MiB", formatFileSize(512LL*1024*1024));
	EXPECT_EQ("768.0 MiB", formatFileSize(768LL*1024*1024));
	EXPECT_EQ("1024.0 MiB", formatFileSize(1024LL*1024*1024));
	EXPECT_EQ("1536.0 MiB", formatFileSize(1536LL*1024*1024));
	EXPECT_EQ("2.00 GiB", formatFileSize(2048LL*1024*1024));
	EXPECT_EQ("2.50 GiB", formatFileSize(2560LL*1024*1024));
	EXPECT_EQ("3.00 GiB", formatFileSize(3072LL*1024*1024));

	// Terabyte
	EXPECT_EQ("512.0 GiB", formatFileSize(512LL*1024*1024*1024));
	EXPECT_EQ("768.0 GiB", formatFileSize(768LL*1024*1024*1024));
	EXPECT_EQ("1024.0 GiB", formatFileSize(1024LL*1024*1024*1024));
	EXPECT_EQ("1536.0 GiB", formatFileSize(1536LL*1024*1024*1024));
	EXPECT_EQ("2.00 TiB", formatFileSize(2048LL*1024*1024*1024));
	EXPECT_EQ("2.50 TiB", formatFileSize(2560LL*1024*1024*1024));
	EXPECT_EQ("3.00 TiB", formatFileSize(3072LL*1024*1024*1024));

	// Petabyte
	EXPECT_EQ("512.0 TiB", formatFileSize(512LL*1024*1024*1024*1024));
	EXPECT_EQ("768.0 TiB", formatFileSize(768LL*1024*1024*1024*1024));
	EXPECT_EQ("1024.0 TiB", formatFileSize(1024LL*1024*1024*1024*1024));
	EXPECT_EQ("1536.0 TiB", formatFileSize(1536LL*1024*1024*1024*1024));
	EXPECT_EQ("2.00 PiB", formatFileSize(2048LL*1024*1024*1024*1024));
	EXPECT_EQ("2.50 PiB", formatFileSize(2560LL*1024*1024*1024*1024));
	EXPECT_EQ("3.00 PiB", formatFileSize(3072LL*1024*1024*1024*1024));

	// Exabyte
	EXPECT_EQ("512.0 PiB", formatFileSize(512LL*1024*1024*1024*1024*1024));
	EXPECT_EQ("768.0 PiB", formatFileSize(768LL*1024*1024*1024*1024*1024));
	EXPECT_EQ("1024.0 PiB", formatFileSize(1024LL*1024*1024*1024*1024*1024));
	EXPECT_EQ("1536.0 PiB", formatFileSize(1536LL*1024*1024*1024*1024*1024));
	EXPECT_EQ("2.00 EiB", formatFileSize(2048LL*1024*1024*1024*1024*1024));
	EXPECT_EQ("2.50 EiB", formatFileSize(2560LL*1024*1024*1024*1024*1024));
	EXPECT_EQ("3.00 EiB", formatFileSize(3072LL*1024*1024*1024*1024*1024));

	// Largest value for a 64-bit signed integer
	EXPECT_EQ("7.99 EiB", formatFileSize(0x7FFFFFFFFFFFFFFFLL));
}

/**
 * Test formatFileSizeKiB().
 */
TEST_F(TextFuncsTest, formatFileSizeKiB)
{
	// NOTE: Due to LC_ALL="C", use standard binary sizes. (KiB, MiB, etc)
	// Testing 512, 768, 1024, 1536, 2048, 2560, and 3072 for each order of magnitude.

	// Special cases
	EXPECT_EQ("0 KiB", formatFileSizeKiB(0U));
	EXPECT_EQ("0 KiB", formatFileSizeKiB(1U));
	EXPECT_EQ("0 KiB", formatFileSizeKiB(2U));

	// Kilobyte
	EXPECT_EQ("0 KiB", formatFileSizeKiB(512U));
	EXPECT_EQ("0 KiB", formatFileSizeKiB(768U));
	EXPECT_EQ("1 KiB", formatFileSizeKiB(1024U));
	EXPECT_EQ("1 KiB", formatFileSizeKiB(1536U));
	EXPECT_EQ("2 KiB", formatFileSizeKiB(2048U));
	EXPECT_EQ("2 KiB", formatFileSizeKiB(2560U));
	EXPECT_EQ("3 KiB", formatFileSizeKiB(3072U));

	// Megabyte
	EXPECT_EQ("512 KiB", formatFileSizeKiB(512U*1024U));
	EXPECT_EQ("768 KiB", formatFileSizeKiB(768U*1024U));
	EXPECT_EQ("1024 KiB", formatFileSizeKiB(1024U*1024U));
	EXPECT_EQ("1536 KiB", formatFileSizeKiB(1536U*1024U));
	EXPECT_EQ("2048 KiB", formatFileSizeKiB(2048U*1024U));
	EXPECT_EQ("2560 KiB", formatFileSizeKiB(2560U*1024U));
	EXPECT_EQ("3072 KiB", formatFileSizeKiB(3072U*1024U));

	// Gigabyte
	EXPECT_EQ("524288 KiB", formatFileSizeKiB(512U*1024U*1024U));
	EXPECT_EQ("786432 KiB", formatFileSizeKiB(768U*1024U*1024U));
	EXPECT_EQ("1048576 KiB", formatFileSizeKiB(1024U*1024U*1024U));
	EXPECT_EQ("1572864 KiB", formatFileSizeKiB(1536U*1024U*1024U));
	EXPECT_EQ("2097152 KiB", formatFileSizeKiB(2048U*1024U*1024U));
	EXPECT_EQ("2621440 KiB", formatFileSizeKiB(2560U*1024U*1024U));
	EXPECT_EQ("3145728 KiB", formatFileSizeKiB(3072U*1024U*1024U));

	// Largest value for a 32-bit unsigned integer
	EXPECT_EQ("4194303 KiB", formatFileSizeKiB(0xFFFFFFFFU));
}

/**
 * Test formatFrequency().
 */
TEST_F(TextFuncsTest, formatFrequency)
{
	// Testing 512, 768, 1024, 1536, 2048, 2560, and 3072 for each order of magnitude.
	// NOTE: Frequencies aren't powers of two, so the resulting values will
	// have "weird" decimal points.

	// Special cases
	EXPECT_EQ("0 Hz", formatFrequency(0U));
	EXPECT_EQ("1 Hz", formatFrequency(1U));
	EXPECT_EQ("2 Hz", formatFrequency(2U));

	// Kilohertz
	EXPECT_EQ("512 Hz", formatFrequency(512U));
	EXPECT_EQ("768 Hz", formatFrequency(768U));
	EXPECT_EQ("1024 Hz", formatFrequency(1024U));
	EXPECT_EQ("1536 Hz", formatFrequency(1536U));
	EXPECT_EQ("2.048 kHz", formatFrequency(2048U));
	EXPECT_EQ("2.560 kHz", formatFrequency(2560U));
	EXPECT_EQ("3.072 kHz", formatFrequency(3072U));

	// Megahertz
	EXPECT_EQ("524.288 kHz", formatFrequency(512U*1024U));
	EXPECT_EQ("786.432 kHz", formatFrequency(768U*1024U));
	EXPECT_EQ("1048.576 kHz", formatFrequency(1024U*1024U));
	EXPECT_EQ("1572.864 kHz", formatFrequency(1536U*1024U));
	EXPECT_EQ("2.097 MHz", formatFrequency(2048U*1024U));
	EXPECT_EQ("2.621 MHz", formatFrequency(2560U*1024U));
	EXPECT_EQ("3.145 MHz", formatFrequency(3072U*1024U));

	// Gigabyte
	EXPECT_EQ("536.870 MHz", formatFrequency(512U*1024U*1024U));
	EXPECT_EQ("805.306 MHz", formatFrequency(768U*1024U*1024U));
	EXPECT_EQ("1073.741 MHz", formatFrequency(1024U*1024U*1024U));
	EXPECT_EQ("1610.612 MHz", formatFrequency(1536U*1024U*1024U));
	EXPECT_EQ("2.147 GHz", formatFrequency(2048U*1024U*1024U));
	EXPECT_EQ("2.684 GHz", formatFrequency(2560U*1024U*1024U));
	EXPECT_EQ("3.221 GHz", formatFrequency(3072U*1024U*1024U));

	// Largest value for a 32-bit unsigned integer
	EXPECT_EQ("4.294 GHz", formatFrequency(0xFFFFFFFFU));
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

	/** Same tests as above, but with an std::string source **/

	// Basic conversion. (no trailing newline sequence)
	lf_count = 0;
	EXPECT_EQ(expected_lf, dos2unix(string(test1), &lf_count));
	EXPECT_EQ(2, lf_count);

	// Trailing "\r\n"
	lf_count = 0;
	EXPECT_EQ(expected_lf2, dos2unix(string(test2), &lf_count));
	EXPECT_EQ(3, lf_count);

	// Trailing '\r' should be converted to '\n'.
	lf_count = 0;
	EXPECT_EQ(expected_lf2, dos2unix(string(test3), &lf_count));
	EXPECT_EQ(3, lf_count);

	// All standalone '\r' characters should be converted to '\n'.
	lf_count = 0;
	EXPECT_EQ(expected_lf2, dos2unix(string(test4), &lf_count));
	EXPECT_EQ(3, lf_count);

	// Existing standalone '\n' should be counted but not changed.
	lf_count = 0;
	EXPECT_EQ(expected_lf2, dos2unix(string(test5), &lf_count));
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
	// NOTE: Using (*13U/4U) instead of (*3.25) to avoid floating-point arithmetic.
	EXPECT_EQ("0:03.24", formatSampleAsTime(11025U*13U/4U, 11025U));
	EXPECT_EQ("0:03.25", formatSampleAsTime(16000U*13U/4U, 16000U));
	EXPECT_EQ("0:03.24", formatSampleAsTime(22050U*13U/4U, 22050U));
	EXPECT_EQ("0:03.25", formatSampleAsTime(24000U*13U/4U, 24000U));
	EXPECT_EQ("0:03.25", formatSampleAsTime(44100U*13U/4U, 44100U));
	EXPECT_EQ("0:03.25", formatSampleAsTime(48000U*13U/4U, 48000U));

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
	// NOTE: Using (*493U/4U) instead of (*123.25) to avoid floating-point arithmetic.
	EXPECT_EQ("2:03.24", formatSampleAsTime(11025U*493/4, 11025U));
	EXPECT_EQ("2:03.25", formatSampleAsTime(16000U*493/4, 16000U));
	EXPECT_EQ("2:03.24", formatSampleAsTime(22050U*493/4, 22050U));
	EXPECT_EQ("2:03.25", formatSampleAsTime(24000U*493/4, 24000U));
	EXPECT_EQ("2:03.25", formatSampleAsTime(44100U*493/4, 44100U));
	EXPECT_EQ("2:03.25", formatSampleAsTime(48000U*493/4, 48000U));
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
	// NOTE: Using (*13U/4U) instead of (*3.25) to avoid floating-point arithmetic.
	EXPECT_EQ(3249U, convSampleToMs(11025U*13U/4U, 11025U));
	EXPECT_EQ(3250U, convSampleToMs(16000U*13U/4U, 16000U));
	EXPECT_EQ(3249U, convSampleToMs(22050U*13U/4U, 22050U));
	EXPECT_EQ(3250U, convSampleToMs(24000U*13U/4U, 24000U));
	EXPECT_EQ(3250U, convSampleToMs(44100U*13U/4U, 44100U));
	EXPECT_EQ(3250U, convSampleToMs(48000U*13U/4U, 48000U));
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
