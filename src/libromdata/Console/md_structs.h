/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * md_structs.h: Sega Mega Drive data structures.                          *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

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
			uint32_t initial_sp;		// [0x000]
			uint32_t initial_pc;		// [0x004]
			uint32_t bus_error;		// [0x008]
			uint32_t address_error;		// [0x00C]
			uint32_t illegal_insn;		// [0x010]
			uint32_t div_by_zero;		// [0x014]
			uint32_t chk_exception;		// [0x018]
			uint32_t trapv_exception;	// [0x01C]
			uint32_t priv_violation;	// [0x020]
			uint32_t trace_exception;	// [0x024]
			uint32_t lineA_emulator;	// [0x028]
			uint32_t lineF_emulator;	// [0x02C]
			uint32_t reserved1[3];		// [0x030]
			uint32_t uninit_interrupt;	// [0x03C]
			uint32_t reserved2[8];		// [0x040]
			uint32_t interrupts[8];		// [0x060] 0 == spurious
			uint32_t trap_insns[16];	// [0x080] TRAP #x
			uint32_t reserved3[16];		// [0x0C0]

			// User interrupt vectors #64-255 are not included,
			// since they overlap the MD ROM header.
		};
	};
} M68K_VectorTable;
ASSERT_STRUCT(M68K_VectorTable, 64*sizeof(uint32_t));

/**
 * ROM/RAM address information sub-struct.
 */
typedef struct _MD_RomRamInfo {
	uint32_t rom_start;	// [0x1A0]
	uint32_t rom_end;	// [0x1A4]
	uint32_t ram_start;	// [0x1A8]
	uint32_t ram_end;	// [0x1AC]
} MD_RomRamInfo;
ASSERT_STRUCT(MD_RomRamInfo, 4*sizeof(uint32_t));

/**
 * Mega Drive ROM header.
 * This matches the MD ROM header format exactly.
 *
 * All fields are big-endian.
 * NOTE: Strings are NOT null-terminated!
 */
#define MD_ROMHEADER_ADDRESS 0x100
typedef struct _MD_RomHeader {
	// Titles may be encoded in either
	// Shift-JIS (cp932) or cp1252.
	// NOTE: Offsets are based on the absolute ROM address,
	// since the header is located at 0x100.
	union {
		struct {
			// Standard ROM header
			char system[16];	// [0x100] System ID
			char copyright[16];	// [0x110] Copyright
			char title_domestic[48];// [0x120] Japanese ROM name
			char title_export[48];	// [0x150] US/European ROM name
			char serial_number[14];	// [0x180] Serial number
			uint16_t checksum;	// [0x18E] Checksum (excluding vector table and header)
			char io_support[16];	// [0x190] Supported I/O devices
			MD_RomRamInfo rom_ram;	// [0x1A0] ROM/RAM address information
		};
		struct {
			// Some early ROMs have 32-byte title fields.
			char system[16];	// [0x100] System ID
			char copyright[16];	// [0x110] Copyright
			char title_domestic[32];// [0x120] Japanese ROM name
			char title_export[32];	// [0x140] US/European ROM name
			char serial_number[14];	// [0x160] Serial number
			uint16_t checksum;	// [0x16E] Checksum (excluding vector table and header)
			char io_support[16];	// [0x170] Supported I/O devices
			MD_RomRamInfo rom_ram;	// [0x180] ROM/RAM address information
			// TODO: Does the early format have SRAM information
			// at 0x190 or 0x1B0?
			uint8_t reserved[0x20];	// [0x190]
		} early;
		struct {
			// "Juusou Kihei Leynos (Japan) (Virtual Console).gen" has an
			// Off-by-one error in the header: System is 1 byte too small.
			char system[15];	// [0x100] System ID
			char copyright[16];	// [0x10F] Copyright
			char title_domestic[49];// [0x11F] Japanese ROM name
			char title_export[48];	// [0x150] US/European ROM name
			char serial_number[14];	// [0x180] Serial number
			uint16_t checksum;	// [0x18E] Checksum (excluding vector table and header)
			char io_support[16];	// [0x190] Supported I/O devices
			MD_RomRamInfo rom_ram;	// [0x1A0] ROM/RAM address information
		} target_earth;
	};

	// Save RAM information.
	// Info format: 'R', 'A', %1x1yz000, 0x20
	// x == 1 for backup (SRAM), 0 for not backup
	// yz == 10 for even addresses, 11 for odd addresses
	uint32_t sram_info;		// [0x1B0]
	uint32_t sram_start;		// [0x1B4]
	uint32_t sram_end;		// [0x1B8]

	// Miscellaneous.
	char modem_info[12];		// [0x1BC]
	union {
		char notes[40];		// [0x1C8]
		struct {
			char notes24[24];	// [0x1C8]
			uint32_t info;		// [0x1E0]
			uint8_t data[12];	// [0x1E4]
		} extrom;
	};
	char region_codes[16];		// [0x1F0]
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
	MD_IO_ANALOG		= 'A',
	MD_IO_TEAM_PLAYER	= '4',
	MD_IO_LIGHT_GUN		= 'G',
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
 * Sega Mega CD: System ID area
 * Located at 0x0000 in the first sector.
 * (This is where the M68K vector table would be in Mega Drive ROMs.)
 *
 * Reference: https://forums.sonicretro.org/index.php?threads/how-do-mega-cd-games-start-up.30588/#post-727202
 *
 * All fields are in big-endian.
 * String fields are space-padded.
 */
#define MCD_SYSTEMID_SIGNATURE "SEGADISCSYSTEM  "
typedef struct _MCD_SystemID {
	char sega_disc_system[16];	// [0x000] "SEGADISCSYSTEM  "
	char volume_name[11];		// [0x010] Volume name
	char zero0;			// [0x01B] 0
	uint16_t volume_system;		// [0x01C] Volume system
	uint16_t volume_type;		// [0x01E] Volume type (usually 0x0001?)
	char system_name[11];		// [0x020] System name
	char zero1;			// [0x02B] 0
	uint16_t system_version;	// [0x02C] System version (usually 0x0001?)
	uint16_t zero2;			// [0x02E] 0

	uint32_t ip_address;		// [0x030] Main68K Initial Program CD offset
	uint32_t ip_size;		// [0x034] Main68K Initial Program CD size
	uint32_t ip_entry;		// [0x038] Main68K Initial Program CD entry offset
	uint32_t ip_wram_size;		// [0x03C] Main68K Initial Program Work RAM size

	uint32_t sp_address;		// [0x040] Sub68K Initial Program CD offset
	uint32_t sp_size;		// [0x044] Sub68K Initial Program CD size
	uint32_t sp_entry;		// [0x048] Sub68K Initial Program CD entry offset
	uint32_t sp_wram_size;		// [0x04C] Sub68K Initial Program Work RAM size

	// The remainder of the System ID is "reserved", but there's usually
	// a build date at the start, in "MMDDYYYY" format.
	char build_date[8];		// [0x050] Build date, in "MMDDYYYY" format.

	uint8_t reserved[168];
} MCD_SystemID;
ASSERT_STRUCT(MCD_SystemID, 256);

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
