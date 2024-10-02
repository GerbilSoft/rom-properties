/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * TextOut.hpp: Text output for RomData.                                   *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * Copyright (c) 2016-2017 by Egor.                                        *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// C includes (C++ namespace)
#include <cstdint>

// C++ includes
#include <string>
#include <ostream>

namespace LibRpBase {

class RomData;

enum OutputFlags {
	OF_SkipInternalImages		= (1U << 0),
	OF_SkipListDataMoreThan10	= (1U << 1),	// ROMOutput only
	OF_JSON_NoPrettyPrint		= (1U << 2),	// JSONROMOutput only
};

/**
 * Partially unescape a URL.
 * %20, %23, and %25 are left escaped.
 * @param url URL.
 * @return Partially unescaped URL.
 */
std::string urlPartialUnescape(const std::string &url);

class ROMOutput {
	const RomData *const romdata;
	uint32_t lc;
	unsigned int flags;
public:
	RP_LIBROMDATA_PUBLIC
	explicit ROMOutput(const RomData *romdata, uint32_t lc = 0, unsigned int flags = 0);

	RP_LIBROMDATA_PUBLIC
	friend std::ostream& operator<<(std::ostream& os, const ROMOutput& fo);
};

class JSONROMOutput {
	const RomData *const romdata;
	uint32_t lc;
	unsigned int flags;
	bool crlf_;
public:
	RP_LIBROMDATA_PUBLIC
	explicit JSONROMOutput(const RomData *romdata, uint32_t lc = 0, unsigned int flags = 0);

	RP_LIBROMDATA_PUBLIC
	friend std::ostream& operator<<(std::ostream& os, const JSONROMOutput& fo);

	inline bool crlf(void) const {
		return crlf_;
	}

	inline void setCrlf(bool val) {
		crlf_ = val;
	}
};

}
