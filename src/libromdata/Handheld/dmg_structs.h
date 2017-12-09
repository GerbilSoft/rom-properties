/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * dmg_structs.h: Game Boy (DMG/CGB/SGB) data structures.                  *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
 * Copyright (c) 2016 by Egor.                                             *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_DMG_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_DMG_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

/**
 * Game Boy ROM header.
 * This matches the ROM header format exactly.
 * References:
 * - http://problemkaputt.de/pandocs.htm#thecartridgeheader
 * - http://gbdev.gg8.se/wiki/articles/The_Cartridge_Header
 * 
 * NOTE: Strings are NOT null-terminated!
 */
typedef struct PACKED _DMG_RomHeader {
	uint8_t entry[4];
	uint8_t nintendo[0x30];

	/**
	 * There are 3 variations on the next 16 bytes:
	 * 1) title(16)
	 * 2) title(15) cgbflag(1)
	 * 3) title(11) gamecode(4) cgbflag(1)
	 *
	 * In all three cases, title is NULL-padded.
	 */
	union {
		char title16[16];
		struct {
			union {
				char title15[15];
				struct {
					char title11[11];
					char id4[4];
				};
			};
			uint8_t cgbflag;
		};
	};

	char new_publisher_code[2];
	uint8_t sgbflag;
	uint8_t cart_type;
	uint8_t rom_size;
	uint8_t ram_size;
	uint8_t region;
	uint8_t old_publisher_code;
	uint8_t version;

	uint8_t header_checksum; // checked by bootrom
	uint16_t rom_checksum;   // checked by no one
} DMG_RomHeader;
ASSERT_STRUCT(DMG_RomHeader, 80);

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_DMG_STRUCTS_H__ */
