/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * gba_structs.h: Nintendo Game Boy Advance data structures.               *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_GBA_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_GBA_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Game Boy Advance ROM header.
 * This matches the GBA ROM header format exactly.
 * Reference: http://problemkaputt.de/gbatek.htm#gbacartridgeheader
 *
 * All fields are in little-endian.
 *
 * NOTE: Strings are NOT null-terminated!
 */
#pragma pack(1)
typedef struct PACKED _GBA_RomHeader {
	union {
		uint32_t entry_point;	// 32-bit ARM branch opcode.
		uint8_t entry_point_bytes[4];
	};
	uint8_t nintendo_logo[0x9C];	// Compressed logo.
	char title[12];
	union {
		char id6[6];	// Game code. (ID6)
		struct {
			char id4[4];		// Game code. (ID4)
			char company[2];	// Company code.
		};
	};
	uint8_t fixed_96h;	// Fixed value. (Must be 0x96)
	uint8_t unit_code;	// 0x00 for all GBA models.
	uint8_t device_type;	// 0x00. (bit 7 for debug?)
	uint8_t reserved1[7];
	uint8_t rom_version;
	uint8_t checksum;
	uint8_t reserved2[2];
} GBA_RomHeader;
#pragma pack()
ASSERT_STRUCT(GBA_RomHeader, 192);

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_GBA_STRUCTS_H__ */
