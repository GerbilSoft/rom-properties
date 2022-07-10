/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NintendoPublishers.cpp: Nintendo third-party publishers list.           *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "NintendoPublishers.hpp"

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
const char8_t *lookup(uint16_t code)
{
	const char s_code[3] = {
		(char)(code >> 8),
		(char)(code & 0xFF),
		'\0'
	};
	return lookup(s_code);
}

/**
 * Look up a company code.
 * @param code Company code.
 * @return Publisher, or nullptr if not found.
 */
const char8_t *lookup(const char *code)
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
	if (ISDIGIT(code[0])) {
		idx = (code[0] - '0') * 36;
	} else if (ISUPPER(code[0])) {
		idx = (code[0] - 'A' + 10) * 36;
	} else {
		// Invalid code.
		return nullptr;
	}
	if (ISDIGIT(code[1])) {
		idx += (code[1] - '0');
	} else if (ISUPPER(code[1])) {
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
const char8_t *lookup_old(uint8_t code)
{
	static const char hex_lookup[16] = {
		'0','1','2','3','4','5','6','7',
		'8','9','A','B','C','D','E','F'
	};
	const char s_code[3] = {
		hex_lookup[code >> 4],
		hex_lookup[code & 0x0F],
		'\0'
	};
	return lookup(s_code);
}

/**
 * Look up a company code for FDS titles.
 * This uses the *old* company code format.
 * @param code Company code.
 * @return Publisher, or nullptr if not found.
 */
const char8_t *lookup_fds(uint8_t code)
{
	if (code >= ARRAY_SIZE(NintendoPublishers_FDS_offtbl)) {
		return nullptr;
	}

	// TODO: NameJP?
	const unsigned int offset = NintendoPublishers_FDS_offtbl[code].nameUS_idx;
	return (likely(offset != 0) ? &NintendoPublishers_FDS_strtbl[offset] : nullptr);
}

} }
