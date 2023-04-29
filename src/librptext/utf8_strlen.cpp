/***************************************************************************
 * ROM Properties Page shell extension. (librptext)                        *
 * utf8_strlen.cpp: UTF-8 strlen() functions                               *
 *                                                                         *
 * Copyright (c) 2022-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.librptext.h"
#include "utf8_strlen.hpp"
#include "common.h"

// C includes
#include <stdint.h>
#ifdef HAVE_WCWIDTH
#  include <wchar.h>
#else /* !HAVE_WCWIDTH */
#  include "uniwidth.h"
#  define wcwidth(c) uc_width(c)
#endif /* HAVE_WCWIDTH */

// C includes (C++ namespace)
#include <cassert>

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

}
