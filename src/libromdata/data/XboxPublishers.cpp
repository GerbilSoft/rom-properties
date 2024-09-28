/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * XboxPublishers.hpp: Xbox third-party publishers list.                   *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "XboxPublishers.hpp"

namespace LibRomData { namespace XboxPublishers {

#include "XboxPublishers_data.h"

/** Public functions **/

/**
 * Look up a company code.
 * @param code Company code.
 * @return Publisher, or nullptr if not found.
 */
const char *lookup(uint16_t code)
{
	// NOTE: Homebrew titles might have code == 0.
	if (code == 0) {
		return nullptr;
	}

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
const char *lookup(const char *code)
{
	// NOTE: Homebrew titles might have code == "\0\0".
	assert(code);
	if (!code || !code[0]) {
		return nullptr;
	}

	// Code must be 2 characters, plus NULL.
	assert(code[1] && !code[2]);
	if (!code[1] || code[2]) {
		return nullptr;
	}

	// Lookup table uses Base 26. [A-Z]
	unsigned int idx;
	if (ISUPPER(code[0])) {
		idx = (code[0] - 'A') * 26;
	} else {
		// Invalid code.
		return nullptr;
	}
	if (ISUPPER(code[1])) {
		idx += (code[1] - 'A');
	} else {
		// Invalid code.
		return nullptr;
	}

	if (idx >= ARRAY_SIZE(XboxPublishers_offtbl)) {
		return nullptr;
	}

	const unsigned int offset = XboxPublishers_offtbl[idx];
	return (likely(offset != 0) ? &XboxPublishers_strtbl[offset] : nullptr);
}

} }
