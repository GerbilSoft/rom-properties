/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * TextOut.hpp: Text output for RomData.                                   *
 *                                                                         *
 * Copyright (c) 2016-2017 by Egor.                                        *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_TEXTOUT_HPP__
#define __ROMPROPERTIES_LIBRPBASE_TEXTOUT_HPP__

// C includes.
#include <stdint.h>

// C++ includes.
#include <string>
#include <ostream>

namespace LibRpBase {

class RomData;

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
	bool crlf_;
public:
	explicit ROMOutput(const RomData *romdata, uint32_t lc = 0);
	friend std::ostream& operator<<(std::ostream& os, const ROMOutput& fo);

	inline bool crlf(void) const {
		return crlf_;
	}

	inline void setCrlf(bool val) {
		crlf_ = val;
	}
};

class JSONROMOutput {
	const RomData *const romdata;
	uint32_t lc;
	bool crlf_;
public:
	explicit JSONROMOutput(const RomData *romdata, uint32_t lc = 0);
	friend std::ostream& operator<<(std::ostream& os, const JSONROMOutput& fo);

	inline bool crlf(void) const {
		return crlf_;
	}

	inline void setCrlf(bool val) {
		crlf_ = val;
	}
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_TEXTOUT_HPP__ */
