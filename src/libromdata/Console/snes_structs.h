/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * snes_structs.h: Super Nintendo data structures.                         *
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
 * References:
 * - http://problemkaputt.de/fullsnes.htm
 * - http://satellaview.wikia.com/wiki/Satellaview_ROM_header
 */

/**
 * ROM mapping. (SNES_RomHeader.rom_mapping)
 */
typedef enum {
	SNES_ROMMAPPING_MASK = 0x37,
	SNES_ROMMAPPING_FLAG_ALWAYS = 0x20,	// Always set.

	// ROM type flags.
	SNES_ROMMAPPING_FLAG_LoROM   = 0x00,
	SNES_ROMMAPPING_FLAG_HiROM   = 0x01,
	SNES_ROMMAPPING_FLAG_ExLoROM = 0x02,
	SNES_ROMMAPPING_FLAG_SA_1    = 0x03,
	SNES_ROMMAPPING_FLAG_ExHiROM = 0x05,
	SNES_ROMMAPPING_FLAG_HiROM_SPC7110 = 0x0A,

	// ROM speed flags.
	SNES_ROMMAPPING_FLAG_SlowROM = 0x00,
	SNES_ROMMAPPING_FLAG_FastROM = 0x10,

	// Combine them.
	SNES_ROMMAPPING_LoROM           = 0x20,
	SNES_ROMMAPPING_HiROM           = 0x21,
	SNES_ROMMAPPING_LoROM_S_DD1     = 0x22,
	SNES_ROMMAPPING_LoROM_SA_1      = 0x23,
	SNES_ROMMAPPING_ExHiROM         = 0x25,
	SNES_ROMMAPPING_LoROM_FastROM   = 0x30,
	SNES_ROMMAPPING_HiROM_FastROM   = 0x31,
	SNES_ROMMAPPING_ExLoROM_FastROM = 0x32,
	SNES_ROMMAPPING_ExHiROM_FastROM = 0x35,
	SNES_ROMMAPPING_HiROM_FastROM_SPC7110 = 0x3A,
} SNES_ROM_Mapping;

/**
 * ROM type. (SNES_RomHeader.rom_type)
 */
typedef enum {
	// ROM type is split into two nybbles.
	// ROM = standard ROM cartridge
	// RAM = extra RAM
	// BATT = battery backup
	// ENH = enhancement chip
	// TODO: Verify with an emulator.

	// Low nybble.
	SNES_ROMTYPE_ROM			= 0x00,
	SNES_ROMTYPE_ROM_RAM			= 0x01,
	SNES_ROMTYPE_ROM_RAM_BATT		= 0x02,
	SNES_ROMTYPE_ROM_ENH			= 0x03,
	SNES_ROMTYPE_ROM_RAM_ENH		= 0x04,
	SNES_ROMTYPE_ROM_RAM_BATT_ENH		= 0x05,
	SNES_ROMTYPE_ROM_BATT_ENH		= 0x06,
	SNES_ROMTYPE_ROM_BATT_RTC_4513_ENH	= 0x09,
	SNES_ROMTYPE_ROM_BATT_RTC_GSU1_ENH	= 0x0A,
	SNES_ROMTYPE_ROM_MASK			= 0x0F,

	// High nybble.
	SNES_ROMTYPE_ENH_DSP1		= 0x00,
	SNES_ROMTYPE_ENH_SUPERFX	= 0x10,	// GSU-1 (Star Fox, Stunt Race FX, etc.)
	SNES_ROMTYPE_ENH_OBC1		= 0x20,	// Metal Combat: Falcon's Revenge
	SNES_ROMTYPE_ENH_SA_1		= 0x30,
	SNES_ROMTYPE_ENH_S_DD1		= 0x40,	// Star Ocean, Street Fighter Alpha 2
	SNES_ROMTYPE_ENH_S_RTC		= 0x50,
	SNES_ROMTYPE_ENH_OTHER		= 0xE0,
	SNES_ROMTYPE_ENH_CUSTOM		= 0xF0,
	SNES_ROMTYPE_ENH_MASK		= 0xF0,
} SNES_ROM_Type;

/**
 * Chipset subtype for custom cart HW. (0xFx) [0x7FBF]
 */
typedef enum {
	SNES_CHIPSUBTYPE_SPC7110	= 0x00,
	SNES_CHIPSUBTYPE_STO10_ST011	= 0x01,
	SNES_CHIPSUBTYPE_STO18		= 0x02,
	SNES_CHIPSUBTYPE_CX4		= 0x10,
} SNES_Chipset_Subtype;

/**
 * Super Nintendo ROM header.
 * This matches the ROM header format exactly.
 * Located at 0x7FB0 (LoROM) or 0xFFB0 (HiROM).
 *
 * References:
 * - http://www.smwiki.net/wiki/Internal_ROM_Header
 * - https://en.wikibooks.org/wiki/Super_NES_Programming/SNES_memory_map#The_SNES_header
 *
 * All fields are in little-endian.
 *
 * NOTE: Strings are NOT null-terminated!
 */
typedef struct _SNES_RomHeader {
	union {
		// SNES header format.
		struct {
			/** Extended header is only present if old_publisher_code == 0x33. **/
			struct {
				union RP_PACKED {
					char c[2];
					uint16_t u16;
				} new_publisher_code;		// [0x7FB0]
#pragma pack(1)
				union RP_PACKED {
					char c[4];
					uint32_t u32;
				} id4;				// [0x7FB2]
#pragma pack()
				uint8_t reserved[6];		// [0x7FB6] Always 0x00.
				uint8_t exp_flash_size;		// [0x7FBC] Expansion FLASH size.
				uint8_t exp_ram_size;		// [0x7FBD] Expansion RAM size.
				uint8_t special_version;	// [0x7FBE]
				uint8_t chipset_subtype;	// [0x7FBF] For custom cart HW. (0xFx)
			} ext;

			/** Standard SNES header. **/
			char title[21];			// [0x7FC0] NOTE: May be Shift-JIS?
			uint8_t rom_mapping;		// [0x7FD5] LoROM, HiROM
			uint8_t rom_type;		// [0x7FD6] ROM type (enhancements)
			uint8_t rom_size;		// [0x7FD7] ROM size (1024 << rom_size)
			uint8_t sram_size;		// [0x7FD8] SRAM size (1024 << sram_size);
			uint8_t destination_code;	// [0x7FD9] Destination code (see SNES_Destination_Code_e)
			uint8_t old_publisher_code;	// [0x7FDA]
			uint8_t version;		// [0x7FDB]
			uint16_t checksum_complement;	// [0x7FDC]
			uint16_t checksum;		// [0x7FDE]
		} snes;

		// BS-X header format.
		// Similar to SNES, but several fields are repurposed.
		// References:
		// - http://problemkaputt.de/fullsnes.htm#snescartsatellaviewflashfileheader
		// - http://satellaview.wikia.com/wiki/Satellaview_ROM_header
		struct {
			/** Extended header **/
			// NOTE: Invalid if 0x7FDB == 0x01
#pragma pack(1)
			struct RP_PACKED {
				char new_publisher_code[2];	// [0x7FB0]
				uint32_t program_type;		// [0x7FB2] See SNES_BSX_Program_Type_e.
				uint8_t reserved[10];		// [0x7FB6] Usually 0x00.
			} ext;
#pragma pack()

			/** Standard SNES header. **/
			char title[16];			// [0x7FC0] Shift-JIS
			uint32_t block_alloc;		// [0x7FD0] Block allocation flags.
			uint16_t limited_starts;	// [0x7FD4] Limited starts
			// Some compilers pad this structure to a multiple of 4 bytes
#pragma pack(1)
			struct RP_PACKED {
				uint8_t month;		// [0x7FD6] Month
				uint8_t day;		// [0x7FD7] Day
			} date;
#pragma pack()
			uint8_t rom_mapping;		// [0x7FD8] LoROM, HiROM
			uint8_t file_type;		// [0x7FD9] File/Execution type
			uint8_t old_publisher_code;	// [0x7FDA] 0x33 if valid; 0x00 if deleted.
			uint8_t x7FDB;			// [0x7FDB] If 0x01, ext is invalid.
			uint16_t checksum_complement;	// [0x7FDC]
			uint16_t checksum;		// [0x7FDE]
		} bsx;
	};

	/** Vectors. **/
	struct {
		struct {
			uint8_t reserved[4];	// [0x7FE0]
			uint16_t cop;		// [0x7FE4]
			uint16_t brk;		// [0x7FE6]
			uint16_t abort;		// [0x7FE8]
			uint16_t nmi;		// [0x7FEA]
			uint16_t reset;		// [0x7FEC]
			uint16_t irq;		// [0x7FEE]
		} native;
		struct {
			uint8_t reserved1[4];	// [0x7FF0]
			uint16_t cop;		// [0x7FF4]
			uint8_t reserved2[2];	// [0x7FF6]
			uint16_t abort;		// [0x7FF8]
			uint16_t nmi;		// [0x7FFA]
			uint16_t res;		// [0x7FFC]
			// Some compilers pad this structure to a multiple of 4 bytes
#pragma pack(1)
			union RP_PACKED {
				// IRQ/BRK share the same vector
				// in 6502 emulation mode.
				uint16_t irq;	// [0x7FFE]
				uint16_t brk;
			};
#pragma pack()
		} emulation;
	} vectors;
} SNES_RomHeader;
ASSERT_STRUCT(SNES_RomHeader, 80);

/**
 * SNES destination codes.
 */
typedef enum {
	SNES_DEST_JAPAN		= 0x00,
	SNES_DEST_NORTH_AMERICA	= 0x01,
	SNES_DEST_EUROPE	= 0x02,
	SNES_DEST_SCANDINAVIA	= 0x03,
	SNES_DEST_FRANCE	= 0x06,
	SNES_DEST_NETHERLANDS	= 0x07,
	SNES_DEST_SPAIN		= 0x08,
	SNES_DEST_GERMANY	= 0x09,
	SNES_DEST_ITALY		= 0x0A,
	SNES_DEST_CHINA		= 0x0B,
	SNES_DEST_SOUTH_KOREA	= 0x0D,
	SNES_DEST_ALL		= 0x0E,
	SNES_DEST_CANADA	= 0x0F,
	SNES_DEST_BRAZIL	= 0x10,
	SNES_DEST_AUSTRALIA	= 0x11,
	SNES_DEST_OTHER_X	= 0x12,
	SNES_DEST_OTHER_Y	= 0x13,
	SNES_DEST_OTHER_Z	= 0x14,
} SNES_Destination_Code_e;

/**
 * BS-X program type.
 */
typedef enum {
	SNES_BSX_PRG_65c816	= 0x00000000,	// 65c816 program
	SNES_BSX_PRG_SCRIPT	= 0x00000100,	// BS-X script
	SNES_BSX_PRG_SA_1	= 0x00000200,	// SA-1 program
} SNES_BSX_Program_Type_e;

/**
 * Nintendo Power directory entry
 * Reference: https://problemkaputt.de/fullsnes.htm#snescartnintendopowerdirectory
 *
 * All fields are in little-endian.
 */
#define SNES_NP_DIRECTORY_ADDRESS 0x60000U
#define SNES_NP_FILE0_FOOTER "MULTICASSETTE 32"
#pragma pack(1)
typedef struct RP_PACKED _SNES_NP_DirEntry {
	uint8_t directory_index;	// [0x0000] Directory index: 0-7 (or 0xFF for unused)
	uint8_t first_flash_block;	// [0x0001] First 512K FLASH block (0-7 for blocks 0-7)
	uint8_t first_sram_block;	// [0x0002] First 2K SRAM block (0-15 for blocks 0-15)
	uint16_t num_flash_blocks;	// [0x0003] Number of 512K FLASH blocks (x4)
	uint16_t num_sram_blocks;	// [0x0005] Number of 2K SRAM blocks (x16)
	char game_code[12];		// [0x0007] Game code, e.g. "SHVC-AxxJ-  ")
	char title_sjis[44];		// [0x0013] Title in Shift-JIS, NULL-padded (not used by the menu program)
	uint8_t title_bmp[384];		// [0x003F] Title in bitmap format (192x12)
	char date[10];			// [0x01BF] Date ("MM/DD/YYYY" on LAW carts; "YYYY/MM/DD" on NIN carts)
	char time[8];			// [0x01C9] Time ("HH:MM:SS")
	char kiosk_id[8];		// [0x01D1] Kiosk ID:
					//          - "LAWnnnnn" for Lawson Convenience Store kiosks
					//          - "NINnnnnn" for titles pre-installed by Nintendo
	uint8_t unused[7703];		// [0x01D9] Unused (0xFF-filled)
	char multicassette[16];		// [0x1FF0] File0 contains "MULTICASSETTE 32"; others have 0xFF.
} SNES_NP_DirEntry;
ASSERT_STRUCT(SNES_NP_DirEntry, 0x2000);
#pragma pack()

#ifdef __cplusplus
}
#endif
