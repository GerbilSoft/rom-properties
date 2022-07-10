/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CBMData.hpp: Commodore cartridge data.                                  *
 *                                                                         *
 * Copyright (c) 2022 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_DATA_CBMDATA_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DATA_CBMDATA_HPP__

#include "common.h"

// C includes.
#include <stdint.h>

namespace LibRomData { namespace CBMData {

/**
 * Look up a C64 cartridge type.
 * @param type Cartridge type
 * @return Cartridge type name, or nullptr if not found.
 */
const char8_t *lookup_C64_cart_type(uint16_t type);

/**
 * Look up a VIC-20 cartridge type.
 * @param type Cartridge type
 * @return Cartridge type name, or nullptr if not found.
 */
const char8_t *lookup_VIC20_cart_type(uint16_t type);

} }

#endif /* __ROMPROPERTIES_LIBROMDATA_DATA_CBMDATA_HPP__ */
