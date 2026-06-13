/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * PSFTagParser.cpp: PSF-style tag parser.                                 *
 *                                                                         *
 * Copyright (c) 2018-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "PSFTagParser.hpp"

#include "psf_structs.h"
#include "s98_structs.h"

// Other rom-properties libraries
#include "librptext/conversion.hpp"
using namespace LibRpText;

// C includes (C++ namespace)
#include <cstring>

// C++ STL classes
#include <algorithm>
using std::string;
using std::unordered_map;

namespace LibRomData { namespace PSFTagParser {

/**
 * Parse a PSF tag section.
 * @param pData Tag section
 * @param size Size of tag section
 * @param style Style of PSF tags
 */
unordered_map<string, string> parseTags(const char *pData, size_t size, PSFTagStyle style)
{
	const char *p = pData;
	const char *const pEnd = pData + size;

	// Verify the magic number first.
	const char *magicNumber;
	size_t magicSize;
	switch (style) {
		default:
		case PSFTagStyle::PSF:
			magicNumber = PSF_TAG_MAGIC;
			magicSize = sizeof(PSF_TAG_MAGIC) - 1;
			break;
		case PSFTagStyle::S98:
			magicNumber = S98_TAG_MAGIC;
			magicSize = sizeof(S98_TAG_MAGIC) - 1;
			break;
	}
	if (p + magicSize >= pEnd) {
		// Out of bounds?
		return {};
	}
	if (memcmp(p, magicNumber, magicSize) != 0) {
		// Invalid magic number.
		return {};
	}
	p += magicSize;

	// Older files may have tags encoded in cp1252 or Shift-JIS.
	bool isUtf8 = false;

	if (style == PSFTagStyle::S98) {
		// S98: Check for a UTF-8 BOM.
		if (p + 3 >= pEnd) {
			// Not enough data...
			return {};
		}

		const uint8_t *const up = reinterpret_cast<const uint8_t*>(p);
		if (up[0] == 0xEFU && up[1] == 0xBBU && up[2] == 0xBFU) {
			// Found a UTF-8 BOM.
			isUtf8 = true;
			p += 3;
		}
	}

	unordered_map<string, string> kv;
#ifdef HAVE_UNORDERED_MAP_RESERVE
	kv.reserve(12);
#endif /* HAVE_UNORDERED_MAP_RESERVE */

	for (; p < pEnd; p++) {
		if (*p == '\0') {
			// NULL byte. End of data for S98.
			// Not usually seen for PSF, but we'll handle it there too.
			break;
		}

		// Find the next newline.
		const char *nl = static_cast<const char*>(memchr(p, '\n', pEnd - p));
		if (!nl) {
			// No newline. Assume this is the end of the tag section,
			// and read up to the end.
			nl = pEnd;
		}
		if (p == nl) {
			// Empty line.
			continue;
		}

		// Find the equals sign.
		const char *eq = static_cast<const char*>(memchr(p, '=', nl - p));
		if (eq) {
			// Found the equals sign.
			const int k_len = static_cast<int>(eq - p);
			const int v_len = static_cast<int>(nl - eq - 1);
			if (k_len > 0 && v_len > 0) {
				// Key and value are valid.
				// NOTE: Key is case-insensitive, so convert to lowercase.
				// NOTE: Key *must* be ASCII.
				string s_key(p, k_len);
				std::transform(s_key.begin(), s_key.end(), s_key.begin(),
					[](unsigned char c) noexcept -> char { return std::tolower(c); });

				if (style == PSFTagStyle::PSF) {
					// Check for UTF-8.
					// NOTE: The v_len check is redundant...
					if (s_key == "utf8" && v_len > 0) {
						// "utf8" key with non-empty value.
						// This is UTF-8.
						isUtf8 = true;
					}
				}

				string s_value(eq+1, v_len);
				kv.emplace(std::move(s_key), std::move(s_value));
			}
		}

		// Next line.
		p = nl;
	}

	// If we're not using UTF-8, convert the values.
	if (!isUtf8) {
		for (auto &p : kv) {
			p.second = cp1252_sjis_to_utf8(p.second);
		}
	}

	return kv;
}

} }
