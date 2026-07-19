/***************************************************************************
 * ROM Properties Page shell extension. (librptext)                        *
 * utf8_funcs.cpp: UTF-8 functions                                         *
 *                                                                         *
 * Copyright (c) 2022-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.librptext.h"
#include "utf8_funcs.hpp"
#include "common.h"

// C includes (C++ namespace)
#include <cassert>
#include <cstdint>
#ifdef HAVE_WCWIDTH
#  include <cwchar>
#else /* !HAVE_WCWIDTH */
#  include "uniwidth.h"
#  define wcwidth(c) uc_width(c)
#endif /* HAVE_WCWIDTH */

// C++ STL classes
using std::string;

namespace LibRpText {

/**
 * Determine the display length of a UTF-8 string.
 * This is used for monospaced console/text output only.
 * NOTE: Assuming the string is valid UTF-8.
 * @param str UTF-8 string
 * @param max_len Maximum length to check
 * @return Display length
 */
size_t utf8_disp_strlen(const char *str, size_t max_len)
{
	size_t len = 0;
	uint32_t uchr = 0;
	for (const uint8_t *u8str = reinterpret_cast<const uint8_t*>(str);
	     *u8str != 0 && max_len > 0; u8str++, max_len--) {
		if (!(u8str[0] & 0x80)) {
			// 1-byte UTF-8 sequence
			uchr = u8str[0];
		} else if ((u8str[0] & 0xE0) == 0xC0) {
			// 2-byte UTF-8 sequence
			if ((u8str[1] & 0xC0) == 0x80) {
				// Valid sequence
				uchr = ((u8str[0] & 0x1F) << 6) |
				        (u8str[1] & 0x3F);
				u8str++;
			} else if (u8str[1] == 0) {
				assert(!"Invalid 2-byte UTF-8 sequence");
				break;
			} else {
				assert(!"Invalid 2-byte UTF-8 sequence");
				continue;
			}
		} else if ((u8str[0] & 0xF0) == 0xE0) {
			// 3-byte UTF-8 sequence
			if (((u8str[1] & 0xC0) == 0x80) &&
			    ((u8str[2] & 0xC0) == 0x80))
			{
				// Valid sequence
				uchr = ((u8str[0] & 0x0F) << 12) |
				       ((u8str[1] & 0x3F) <<  6) |
				        (u8str[2] & 0x3F);
				u8str += 2;
			} else if (u8str[1] == 0 || u8str[2] == 0) {
				assert(!"Invalid 3-byte UTF-8 sequence");
				break;
			} else {
				assert(!"Invalid 3-byte UTF-8 sequence");
				continue;
			}
		} else if ((u8str[0] & 0xF1) == 0xF0) {
			// 4-byte UTF-8 sequence
			if (((u8str[1] & 0xC0) == 0x80) &&
			    ((u8str[2] & 0xC0) == 0x80) &&
			    ((u8str[3] & 0xC0) == 0x80))
			{
				// Valid sequence
				uchr = ((u8str[0] & 0x07) << 18) |
				       ((u8str[1] & 0x3F) << 12) |
				       ((u8str[2] & 0x3F) <<  6) |
				        (u8str[3] & 0x3F);
				u8str += 3;
			} else if (u8str[1] == 0 || u8str[2] == 0 || u8str[3] == 0) {
				assert(!"Invalid 4-byte UTF-8 sequence");
				continue;
			} else {
				assert(!"Invalid 4-byte UTF-8 sequence");
				continue;
			}
		} else {
			assert(!"Invalid UTF-8 sequence");
			continue;
		}

		// Check the character width.
		const int w = wcwidth(uchr);
		if (likely(w >= 0)) {
			len += w;
		} else {
			// Probably a control character. (U+0000-U+001F)
			// FIXME: Convert to U+2400-U+241F before-hand to get the correct width.
			len++;
		}
	}

	return len;
}

/**
 * Encode a Unicode code point as UTF-8.
 * @param chr Code point
 * @return UTF-8 encoded code point
 */
string utf8_encode_code_point(char32_t chr)
{
	char buf[4];
	size_t size;

	if (chr <= 0x7F) {
		buf[0] = static_cast<char>(chr);
		size = 1;
	} else if (chr <= 0x7FF) {
		buf[0] = static_cast<char>(0xC0 |  (chr >>  6));
		buf[1] = static_cast<char>(0x80 |  (chr & 0x3F));
		size = 2;
	} else if (chr <= 0xFFFF) {
		buf[0] = static_cast<char>(0xE0 |  (chr >> 12));
		buf[1] = static_cast<char>(0x80 | ((chr >>  6) & 0x3F));
		buf[2] = static_cast<char>(0x80 |  (chr & 0x3F));
		size = 3;
	} else if (chr <= 0x10FFFF) {
		buf[0] = static_cast<char>(0xF0 |  (chr >> 18));
		buf[1] = static_cast<char>(0x80 | ((chr >> 12) & 0x3F));
		buf[2] = static_cast<char>(0x80 | ((chr >>  6) & 0x3F));
		buf[3] = static_cast<char>(0x80 |  (chr & 0x3F));
		size = 4;
	} else {
		// Invalid UTF-8 character...
		// Use the Unicode replacment character. (U+FFFD)
		return "\xEF\xBF\xBD";
	}

	return string(buf, size);
}

} // namespace LibRpText
