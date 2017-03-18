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
 * 68000 vector table.
 * All fields are big-endian.
 */
#pragma pack(1)
typedef struct PACKED _M68K_VectorTable {
	union {
		uint32_t vectors[64];
		struct {
			uint32_t initial_sp;
			uint32_t initial_pc;
			uint32_t bus_error;
			uint32_t address_error;
			uint32_t illegal_insn;
			uint32_t div_by_zero;
			uint32_t chk_insn;
			uint32_t trapv_insn;
			uint32_t priv_violation;
			uint32_t trace;
			uint32_t unimpl_insn1;
			uint32_t unimpl_insn2;
			uint32_t reserved1[3];
			uint32_t uninit_interrupt;
			uint32_t reserved2[8];
			uint32_t interrupts[8];		// 0 == spurious
			uint32_t trap_insns[16];	// TRAP #x
			uint32_t reserved3[16];

			// User interrupt vectors #64-255 are not included,
			// since they overlap the MD ROM header.
		};
	};
} M68K_VectorTable;
ASSERT_STRUCT(M68K_VectorTable, 256);

/**
 * Mega Drive ROM header.
 * This matches the MD ROM header format exactly.
 *
 * All fields are big-endian.
 * NOTE: Strings are NOT null-terminated!
 */
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
	union {
		char notes[40];
		struct {
			char notes24[24];
			uint32_t info;
			uint8_t data[12];
		} extrom;
	};
	char region_codes[16];
} MD_RomHeader;
#pragma pack()
ASSERT_STRUCT(MD_RomHeader, 256);

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_MD_STRUCTS_H__ */
