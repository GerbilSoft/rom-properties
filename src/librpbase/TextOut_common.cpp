/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * TextOut.hpp: Text output for RomData. (common functions)                *
 *                                                                         *
 * Copyright (c) 2016-2018 by Egor.                                        *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "TextOut.hpp"

// C includes. (C++ namespace)
#include "ctypex.h"

// C++ includes.
#include <string>
using std::string;

namespace LibRpBase {

/**
 * Partially unescape a URL.
 * %20, %23, and %25 are left escaped.
 * @param url URL.
 * @return Partially unescaped URL.
 */
string urlPartialUnescape(const string &url)
{
	// Unescape everything except %20, %23, and %25.
	string unesc_url;
	const size_t src_size = url.size();
	unesc_url.reserve(src_size);

	for (size_t i = 0; i < src_size; i++) {
		if (url[i] == '%' && (i + 2) < src_size) {
			// Check for two hex digits.
			const char chr0 = url[i+1];
			const char chr1 = url[i+2];
			if (isxdigit_ascii(chr0) && isxdigit_ascii(chr1)) {
				// Unescape it.
				uint8_t val = (chr0 & 0x0F) << 4;
				if (chr0 >= 'A') {
					val += 0x90;
				}
				val |= (chr1 & 0x0F);
				if (chr1 >= 'A') {
					val += 0x09;
				}

				if (val != 0x20 && val != 0x23 && val != 0x25) {
					unesc_url += static_cast<char>(val);
					i += 2;
					continue;
				}
			}
		}

		// Print the character as-is.
		unesc_url += url[i];
	}

	return unesc_url;
}

}
