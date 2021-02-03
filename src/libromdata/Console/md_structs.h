/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * md_structs.h: Sega Mega Drive data structures.                          *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_MD_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_MD_STRUCTS_H__

#include "common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Motorola 68000 vector table.
 * All fields are big-endian.
 */
typedef struct _M68K_VectorTable {
	union {
		uint32_t vectors[64];
		struct {
			uint32_t initial_sp;
			uint32_t initial_pc;
			uint32_t bus_error;
			uint32_t address_error;
			uint32_t illegal_insn;
			uint32_t div_by_zero;
			uint32_t chk_exception;
			uint32_t trapv_exception;
			uint32_t priv_violation;
			uint32_t trace_exception;
			uint32_t lineA_emulator;
			uint32_t lineF_emulator;
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
ASSERT_STRUCT(M68K_VectorTable, 64*sizeof(uint32_t));

/**
 * Mega Drive ROM header.
 * This matches the MD ROM header format exactly.
 *
 * All fields are big-endian.
 * NOTE: Strings are NOT null-terminated!
 */
typedef struct _MD_RomHeader {
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
ASSERT_STRUCT(MD_RomHeader, 256);

/**
 * Mega Drive I/O support.
 * Maps to MD_RomHeader.io_support[] entries.
 */
typedef enum {
	MD_IO_JOYPAD_3		= 'J',
	MD_IO_JOYPAD_6		= '6',
	MD_IO_JOYPAD_SMS	= '0',
	MD_IO_TEAM_PLAYER	= '4',
	MD_IO_KEYBOARD		= 'K',
	MD_IO_SERIAL		= 'R',
	MD_IO_PRINTER		= 'P',
	MD_IO_TABLET		= 'T',
	MD_IO_TRACKBALL		= 'B',
	MD_IO_PADDLE		= 'V',
	MD_IO_FDD		= 'F',
	MD_IO_CDROM		= 'C',
	MD_IO_ACTIVATOR		= 'L',
	MD_IO_MEGA_MOUSE	= 'M',
} MD_IO_Support;

/**
 * Sega 32X security program user header.
 * Reference: http://gendev.spritesmind.net/forum/viewtopic.php?t=65
 *
 * All fields are in big-endian.
 * (Part of the MC68000 program, not the SH-2 subprograms.)
 */
#define __32X_SecurityProgram_UserHeader_ADDRESS 0x03C0
#define __32X_SecurityProgram_UserHeader_MODULE_NAME "MARS CHECK MODE "
typedef struct __32X_SecurityProgram_UserHeader {
	char module_name[16];		// [0x000] Module name (space-padded)
	uint32_t version;		// [0x010]
	uint32_t src_addr;		// [0x014]
	uint32_t dest_addr;		// [0x018]
	uint32_t size;			// [0x01C]
	uint32_t msh2_start_addr;	// [0x020] Master SH-2 start address
	uint32_t ssh2_start_addr;	// [0x024] Slave SH-2 start address
	uint32_t msh2_vbr;		// [0x028] Master SH-2 VBR
	uint32_t ssh2_vbr;		// [0x02C] Slave SH-2 VBR
} _32X_SecurityProgram_UserHeader;
ASSERT_STRUCT(_32X_SecurityProgram_UserHeader, 48);

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_MD_STRUCTS_H__ */
