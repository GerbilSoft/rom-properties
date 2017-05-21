/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * snes_structs.h: Super Nintendo data structures.                         *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_SNES_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_SNES_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ROM mapping. (SNES_RomHeader.rom_mapping)
 */
typedef enum {
	SNES_ROMMAPPING_MASK = 0x37,

	// ROM type flags.
	SNES_ROMMAPPING_FLAG_ALWAYS = 0x20,	// Always set.
	SNES_ROMMAPPING_FLAG_LoROM = 0x00,
	SNES_ROMMAPPING_FLAG_HiROM = 0x01,
	SNES_ROMMAPPING_FLAG_SlowROM = 0x00,
	SNES_ROMMAPPING_FLAG_FastROM = 0x10,
	SNES_ROMMAPPING_FLAG_ExLoROM = 0x02,
	SNES_ROMMAPPING_FLAG_ExHiROM = 0x04,

	// Standard ROM types.
	SNES_ROMMAPPING_LoROM = 0x20,
	SNES_ROMMAPPING_HiROM = 0x21,
	SNES_ROMMAPPING_LoROM_FastROM = 0x30,
	SNES_ROMMAPPING_HiROM_FastROM = 0x31,
	SNES_ROMMAPPING_ExLoROM = 0x32,
	SNES_ROMMAPPING_ExHiROM = 0x35,
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
	SNES_ROMTYPE_ROM		= 0x00,
	SNES_ROMTYPE_ROM_RAM		= 0x01,
	SNES_ROMTYPE_ROM_RAM_BATT	= 0x02,
	SNES_ROMTYPE_ROM_ENH		= 0x03,
	SNES_ROMTYPE_ROM_RAM_ENH	= 0x04,
	SNES_ROMTYPE_ROM_RAM_BATT_ENH	= 0x05,
	SNES_ROMTYPE_ROM_BATT_ENH	= 0x06,
	SNES_ROMTYPE_ROM_MASK		= 0x0F,

	// High nybble.
	SNES_ROMTYPE_ENH_DSP1		= 0x00,
	SNES_ROMTYPE_ENH_SUPERFX	= 0x10,
	SNES_ROMTYPE_ENH_OBC1		= 0x20,	// Metal Combat: Falcon's Revenge
	SNES_ROMTYPE_ENH_SA1		= 0x30,
	SNES_ROMTYPE_ENH_SDD1		= 0x40,	// Star Ocean, Street Fighter Alpha 2
	SNES_ROMTYPE_ENH_OTHER		= 0xE0,
	SNES_ROMTYPE_ENH_CUSTOM		= 0xF0,
	SNES_ROMTYPE_ENH_MASK		= 0xF0,
} SNES_ROM_Type;

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
#pragma pack(1)
typedef struct PACKED _SNES_RomHeader {
	/** Extended header is only present if old_publisher_code == 0x33. **/
	struct {
		char new_publisher_code[2];	// [0x7FB0]
		char id4[4];			// [0x7FB2]
		uint8_t reserved[7];		// [0x7FB6] Always 0x00.
		uint8_t exp_ram_size;		// [0x7FBD] Expansion RAM size.
		uint8_t special_version;	// [0x7FBE]
		uint8_t cart_type;		// [0x7FBF]
	} ext;

	/** Standard SNES header. **/
	char title[21];			// [0x7FC0]
	uint8_t rom_mapping;		// [0x7FD5] LoROM, HiROM
	uint8_t rom_type;		// [0x7FD6]
	uint8_t rom_size;		// [0x7FD7] ROM size. (1024 << rom_size)
	uint8_t sram_size;		// [0x7FD8] SRAM size. (1024 << sram_size);
	uint8_t destination_code;	// [0x7FD9] Destination code. (TODO: Enum?)
	uint8_t old_publisher_code;	// [0x7FDA]
	uint8_t version;		// [0x7FDB]
	uint16_t checksum_complement;	// [0x7FDC]
	uint16_t checksum;		// [0x7FDE]

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
			union {
				// IRQ/BRK share the same vector
				// in 6502 emulation mode.
				uint16_t irq;	// [0x7FFE]
				uint16_t brk;
			};
		} emulation;
	} vectors;
} SNES_RomHeader;
#pragma pack()
ASSERT_STRUCT(SNES_RomHeader, 80);

/**
 * SNES destination codes.
 */
enum SNES_Destination_Code {
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
};

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_SNES_STRUCTS_H__ */
