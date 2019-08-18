/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * properties.hpp: Properties output.                                      *
 *                                                                         *
 * Copyright (c) 2016-2017 by Egor.                                        *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_RPCLI_PROPERTIES_HPP__
#define __ROMPROPERTIES_RPCLI_PROPERTIES_HPP__

#include <ostream>
namespace LibRpBase {
	class RomData;
}

class ROMOutput {
	const LibRpBase::RomData *const romdata;
public:
	explicit ROMOutput(const LibRpBase::RomData *romdata);
	friend std::ostream& operator<<(std::ostream& os, const ROMOutput& fo);
};

class JSONROMOutput {
	const LibRpBase::RomData *const romdata;
public:
	explicit JSONROMOutput(const LibRpBase::RomData *romdata);
	friend std::ostream& operator<<(std::ostream& os, const JSONROMOutput& fo);
};

#endif /* __ROMPROPERTIES_RPCLI_PROPERTIES_HPP__ */
