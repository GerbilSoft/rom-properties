/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * n64_structs.h: Nintendo 64 data structures.                             *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_N64_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_N64_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

/**
 * Nintendo 64 ROM header.
 * This matches the ROM header format exactly.
 * Reference: http://www.romhacking.net/forum/index.php/topic,20415.msg286889.html?PHPSESSID=8bc8li2rrckkt4arqv7kdmufu1#msg286889
 * 
 * All fields are big-endian.
 * NOTE: Strings are NOT null-terminated!
 */
typedef union PACKED _N64_RomHeader {
	struct {
		union {
			// NOTE: Technically, the first two DWORDs
			// are initialization settings, but in practice,
			// they're usually identical for all N64 ROMs.
			uint8_t magic[8];
			struct {
				uint32_t init_pi;
				uint32_t clockrate;
			};
		};

		uint32_t entrypoint;
		uint32_t release;	// ???
		uint64_t checksum;
		uint8_t reserved1[8];
		char title[0x14];	// Title. (cp932)
		uint8_t reserved[7];
		char id4[4];		// Game ID.
		uint8_t revision;	// Revision.
	};

	// Direct access for byteswapping.
	uint8_t u8[64];
	uint16_t u16[64/2];
	uint32_t u32[64/4];
} N64_RomHeader;
ASSERT_STRUCT(N64_RomHeader, 64);

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_N64_STRUCTS_H__ */
