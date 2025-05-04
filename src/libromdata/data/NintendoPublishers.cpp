/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NintendoPublishers.cpp: Nintendo third-party publishers list.           *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "NintendoPublishers.hpp"

// C++ STL classes
using std::array;

namespace LibRomData { namespace NintendoPublishers {

// TODO: Combine the two string tables?
#include "NintendoPublishers_data.h"
#include "NintendoPublishers_FDS_data.h"

/** Public functions **/

/**
 * Look up a company code.
 * @param code Company code.
 * @return Publisher, or nullptr if not found.
 */
const char *lookup(uint16_t code)
{
	const array<char, 3> s_code = {{
		static_cast<char>(code >> 8),
		static_cast<char>(code & 0xFF),
		'\0'
	}};
	return lookup(s_code.data());
}

/**
 * Look up a company code.
 * @param code Company code.
 * @return Publisher, or nullptr if not found.
 */
const char *lookup(const char *code)
{
	// Code must be 2 characters.
	// NOTE: Some callers, e.g. NintendoDS, might not have
	// a NULL byte after code[1], and some unlicensed ROMs
	// might not have a valid publisher at all.
	if (!code || !code[0] || !code[1]) {
		return nullptr;
	}

	// Lookup table uses Base 36. [0-9A-Z]
	unsigned int idx;
	if (isdigit_ascii(code[0])) {
		idx = (code[0] - '0') * 36;
	} else if (isupper_ascii(code[0])) {
		idx = (code[0] - 'A' + 10) * 36;
	} else {
		// Invalid code.
		return nullptr;
	}
	if (isdigit_ascii(code[1])) {
		idx += (code[1] - '0');
	} else if (isupper_ascii(code[1])) {
		idx += (code[1] - 'A' + 10);
	} else {
		// Invalid code.
		return nullptr;
	}

	if (idx >= ARRAY_SIZE(NintendoPublishers_offtbl)) {
		return nullptr;
	}

	const unsigned int offset = NintendoPublishers_offtbl[idx];
	return (likely(offset != 0) ? &NintendoPublishers_strtbl[offset] : nullptr);
}

/**
 * Look up a company code.
 * This uses the *old* company code, present in
 * older Game Boy titles.
 * @param code Company code.
 * @return Publisher, or nullptr if not found.
 */
const char *lookup_old(uint8_t code)
{
	static constexpr array<char, 16> hex_lookup = {{
		'0','1','2','3','4','5','6','7',
		'8','9','A','B','C','D','E','F'
	}};
	const array<char, 3> s_code = {{
		hex_lookup[code >> 4],
		hex_lookup[code & 0x0F],
		'\0'
	}};
	return lookup(s_code.data());
}

/**
 * Look up a company code for FDS titles.
 * This uses the *old* company code format.
 * @param code Company code.
 * @return Publisher, or nullptr if not found.
 */
const char *lookup_fds(uint8_t code)
{
	if (code >= ARRAY_SIZE(NintendoPublishers_FDS_offtbl)) {
		return nullptr;
	}

	// TODO: NameJP?
	const unsigned int offset = NintendoPublishers_FDS_offtbl[code].nameUS_idx;
	return (likely(offset != 0) ? &NintendoPublishers_FDS_strtbl[offset] : nullptr);
}

} }
