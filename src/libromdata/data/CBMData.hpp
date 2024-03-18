/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CBMData.hpp: Commodore cartridge data.                                  *
 *                                                                         *
 * Copyright (c) 2022-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>

namespace LibRomData { namespace CBMData {

/**
 * Look up a C64 cartridge type.
 * @param type Cartridge type
 * @return Cartridge type name, or nullptr if not found.
 */
const char *lookup_C64_cart_type(uint16_t type);

/**
 * Look up a VIC-20 cartridge type.
 * @param type Cartridge type
 * @return Cartridge type name, or nullptr if not found.
 */
const char *lookup_VIC20_cart_type(uint16_t type);

/**
 * Look up a Plus/4 cartridge type.
 * @param type Cartridge type
 * @return Cartridge type name, or nullptr if not found.
 */
const char *lookup_Plus4_cart_type(uint16_t type);

} }
