/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * properties.hpp: Properties output.                                      *
 *                                                                         *
 * Copyright (c) 2016-2017 by Egor.                                        *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_RPCLI_PROPERTIES_HPP__
#define __ROMPROPERTIES_RPCLI_PROPERTIES_HPP__

// C includes.
#include <stdint.h>

// C++ includes.
#include <string>
#include <ostream>

namespace LibRpBase {
	class RomData;
}

/**
 * Partially unescape a URL.
 * %20, %23, and %25 are left escaped.
 * @param url URL.
 * @return Partially unescaped URL.
 */
std::string urlPartialUnescape(const std::string &url);

class ROMOutput {
	const LibRpBase::RomData *const romdata;
	uint32_t lc;
public:
	explicit ROMOutput(const LibRpBase::RomData *romdata, uint32_t lc = 0);
	friend std::ostream& operator<<(std::ostream& os, const ROMOutput& fo);
};

class JSONROMOutput {
	const LibRpBase::RomData *const romdata;
	uint32_t lc;
public:
	explicit JSONROMOutput(const LibRpBase::RomData *romdata, uint32_t lc = 0);
	friend std::ostream& operator<<(std::ostream& os, const JSONROMOutput& fo);
};

#endif /* __ROMPROPERTIES_RPCLI_PROPERTIES_HPP__ */
