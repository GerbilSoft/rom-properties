/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CBMData.hpp: Commodore cartridge data.                                  *
 *                                                                         *
 * Copyright (c) 2022-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "CBMData.hpp"

// C++ STL classes
using std::array;

namespace LibRomData { namespace CBMData {

// Cartridge types are synchronized with VICE 3.8.
// Reference: https://vice-emu.sourceforge.io/vice_16.html#SEC432
#include "CBM_C64_Cartridge_Type_data.h"

// VIC-20 cartridge types
static const array<const char*, 9> crt_types_vic20 = {{
	"generic cartridge",
	"Mega-Cart",
	"Behr Bonz",
	"Vic Flash Plugin",
	"UltiMem",
	"Final Expansion",
	"VIC-Rabbit",
	"Super Expander",
	"Mikro Assembler",
}};

// Plus/4 cartridge types
static const array<const char*, 4> crt_types_plus4 = {{
	"generic cartridge",
	"c264 magic cart",
	"Plus4 multi cart",
	"1MB Cartridge",
}};

/** Public functions **/

/**
 * Look up a C64 cartridge type.
 * @param type Cartridge type
 * @return Cartridge type name, or nullptr if not found.
 */
const char *lookup_C64_cart_type(uint16_t type)
{
	if (unlikely(type >= ARRAY_SIZE(CBM_C64_cart_type_offtbl))) {
		return nullptr;
	}

	const unsigned int offset = CBM_C64_cart_type_offtbl[type];
	return (likely(offset != 0) ? &CBM_C64_cart_type_strtbl[offset] : nullptr);
}

/**
 * Look up a VIC-20 cartridge type.
 * @param type Cartridge type
 * @return Cartridge type name, or nullptr if not found.
 */
const char *lookup_VIC20_cart_type(uint16_t type)
{
	if (unlikely(type >= crt_types_vic20.size())) {
		return nullptr;
	}
	return crt_types_vic20[type];
}

/**
 * Look up a Plus/4 cartridge type.
 * @param type Cartridge type
 * @return Cartridge type name, or nullptr if not found.
 */
const char *lookup_Plus4_cart_type(uint16_t type)
{
	if (unlikely(type >= crt_types_plus4.size())) {
		return nullptr;
	}
	return crt_types_plus4[type];
}

} }
