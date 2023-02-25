/***************************************************************************
 * ROM Properties Page shell extension. (librptext)                        *
 * specialized.cpp: Text encoding functions (specialized conversions)      *
 *                                                                         *
 * Copyright (c) 2009-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "common.h"
#include "conversion.hpp"
#include "NULL-check.hpp"

// Code page tables
#include "RP_CP_tbls.hpp"

// C includes (C++ namespace)
#include <cassert>

// C++ STL classes
using std::string;

namespace LibRpText {

/**
 * Convert 8-bit text to UTF-8 using a lookup table.
 * Used by cpN_to_utf8 for RP-custom code pages.
 *
 * @param tbl	[in] 256-element decoding table. (Only supports Unicode BMP.)
 * @param str	[in] 8-bit text.
 * @param len	[in] Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
static string str8_to_utf8(const char16_t tbl[256], const char *str, int len)
{
	string s_utf8;

	// NOTE: We can't use check_NULL_terminator here because
	// 0x00 may be a valid character in some cases.
	if (len < 0) {
		// Implicit length. We'll use strlen().
		len = static_cast<int>(strlen(str));
	} else if (unlikely(len == 0)) {
		// Shouldn't happen...
		return s_utf8;
	} else {
		// Explicit length.
		// Search for NULLs starting at the end of the string.
		// This allows us to trim strings while preserving any
		// embedded NULLs that might be graphics characters.
		const char *end = str + len - 1;
		for (; end > str; end--) {
			if (*end != 0x00)
				break;
		}
		len = static_cast<int>(end - str + 1);
	}

	if (len <= 0) {
		// Nothing to do...
		return string();
	}

	s_utf8.reserve(len + 8);
	for (; len > 0; str++, len--) {
		// NOTE: The (uint8_t) cast is required.
		// Otherwise, *str is interpreted as a signed char,
		// which causes all sorts of shenanigans.
		const char16_t ch16 = tbl[(uint8_t)*str];
		// NOTE: Masks for the first byte might not be needed...
		if (ch16 < 0x0080) {
			s_utf8 += (char)(ch16 & 0x7F);
		} else if (ch16 < 0x0800) {
			s_utf8 += (char)(0xC0 | ((ch16 >>  6) & 0x1F));
			s_utf8 += (char)(0x80 | ((ch16 >>  0) & 0x3F));
		} else /*if (ch16 < 0x10000)*/ {
			s_utf8 += (char)(0xE0 | ((ch16 >> 12) & 0x0F));
			s_utf8 += (char)(0x80 | ((ch16 >>  6) & 0x3F));
			s_utf8 += (char)(0x80 | ((ch16 >>  0) & 0x3F));
		}
	}

	return s_utf8;
}

/**
 * Convert 8-bit text to UTF-8 using an RP-custom code page.
 * Code page number must be CP_RP_*.
 *
 * @param cp	[in] Code page number.
 * @param str	[in] 8-bit text.
 * @param len	[in] Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
std::string cpRP_to_utf8(unsigned int cp, const char *str, int len)
{
	assert(cp & CP_RP_BASE);
	if (!(cp & CP_RP_BASE))
		return string();

	cp &= ~CP_RP_BASE;
	assert(cp < ARRAY_SIZE(CodePageTables::lkup_tbls));
	if (cp >= ARRAY_SIZE(CodePageTables::lkup_tbls))
		return string();

	return str8_to_utf8(CodePageTables::lkup_tbls[cp], str, len);
}

}
