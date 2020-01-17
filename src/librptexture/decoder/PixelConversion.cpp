/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * PixelConversion.cpp: Pixel conversion inline functions.                 *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "PixelConversion.hpp"

namespace LibRpTexture { namespace PixelConversion {

// 2-bit alpha lookup table.
const uint32_t a2_lookup[4] = {
	0x00000000, 0x55000000,
	0xAA000000, 0xFF000000
};

// 3-bit alpha lookup table.
const uint32_t a3_lookup[8] = {
	0x00000000, 0x24000000,
	0x49000000, 0x6D000000,
	0x92000000, 0xB6000000,
	0xDB000000, 0xFF000000
};

// 2-bit color lookup table.
const uint8_t c2_lookup[4] = {
	0x00, 0x55, 0xAA, 0xFF
};

// 3-bit color lookup table.
const uint8_t c3_lookup[8] = {
	0x00, 0x24, 0x49, 0x6D,
	0x92, 0xB6, 0xDB, 0xFF
};

} }
