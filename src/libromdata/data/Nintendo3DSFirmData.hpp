/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Nintendo3DSFirmData.hpp: Nintendo 3DS firmware data.                    *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>
#include "compiler-attrs.h"

namespace LibRomData { namespace Nintendo3DSFirmData {

// Flags
enum ATTR_FLAG_ENUM Flags : uint8_t {
	FLAG_New3DS	= (1U << 0),	// for New 3DS
	FLAG_Devel	= (1U << 1),	// devkit version
	FLAG_SafeMode	= (1U << 2),	// safe mode only
	FLAG_TWL_FIRM	= (1U << 3),	// TWL_FIRM
	FLAG_AGB_FIRM	= (1U << 4),	// AGB_FIRM
};

struct FirmBin_t {
	uint32_t crc;		// FIRM CRC32
	struct {		// Kernel version
		uint8_t major;
		uint8_t minor;
		uint8_t revision;
	} kernel;
	struct {		// System version
		uint8_t major;
		uint8_t minor;
	} sys;
	uint8_t flags;		// See Flags
};

/**
 * Look up a Nintendo 3DS firmware binary.
 * @param crc Firmware binary CRC32.
 * @return Firmware binary data, or nullptr if not found.
 */
const FirmBin_t *lookup_firmBin(uint32_t crc);

} } // namespace LibRomData::Nintendo3DSFirmData
