/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Nintendo3DSFirmData.hpp: Nintendo 3DS firmware data.                    *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>

namespace LibRomData { namespace Nintendo3DSFirmData {

struct FirmBin_t {
	uint32_t crc;		// FIRM CRC32.
	struct {		// Kernel version.
		uint8_t major;
		uint8_t minor;
		uint8_t revision;
	} kernel;
	struct {		// System version.
		uint8_t major;
		uint8_t minor;
	} sys;
	bool isNew3DS;		// Is this New3DS?
};

/**
 * Look up a Nintendo 3DS firmware binary.
 * @param crc Firmware binary CRC32.
 * @return Firmware binary data, or nullptr if not found.
 */
const FirmBin_t *lookup_firmBin(uint32_t crc);

} }
