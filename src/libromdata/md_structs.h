/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * md_structs.h: Sega Mega Drive data structures.                          *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_MD_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_MD_STRUCTS_H__

#include "common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Mega Drive ROM header.
 * This matches the MD ROM header format exactly.
 *
 * All fields are big-endian.
 * NOTE: Strings are NOT null-terminated!
 */
#define MD_RomHeader_SIZE 256
#pragma pack(1)
typedef struct PACKED _MD_RomHeader {
	// Titles may be encoded in either
	// Shift-JIS (cp932) or cp1252.
	char system[16];
	char copyright[16];
	char title_domestic[48];	// Japanese ROM name.
	char title_export[48];		// US/Europe ROM name.
	char serial[14];
	uint16_t checksum;
	char io_support[16];

	// ROM/RAM address information.
	uint32_t rom_start;
	uint32_t rom_end;
	uint32_t ram_start;
	uint32_t ram_end;

	// Save RAM information.
	// Info format: 'R', 'A', %1x1yz000, 0x20
	// x == 1 for backup (SRAM), 0 for not backup
	// yz == 10 for even addresses, 11 for odd addresses
	uint32_t sram_info;
	uint32_t sram_start;
	uint32_t sram_end;

	// Miscellaneous.
	char modem_info[12];
	char notes[40];
	char region_codes[16];
} MD_RomHeader;
#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_MD_STRUCTS_H__ */
