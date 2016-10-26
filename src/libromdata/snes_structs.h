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

#include "common.h"
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
	// SRAM = save RAM
	// ENH = enhancement chip
	// TODO: Verify with an emulator.

	// Low nybble.
	SNES_ROMTYPE_ROM		= 0x00,
	SNES_ROMTYPE_ROM_RAM		= 0x01,
	SNES_ROMTYPE_ROM_RAM_SRAM	= 0x02,
	SNES_ROMTYPE_ROM_ENH		= 0x03,
	SNES_ROMTYPE_ROM_RAM_ENH	= 0x04,
	SNES_ROMTYPE_ROM_RAM_SRAM_ENH	= 0x05,
	SNES_ROMTYPE_ROM_SRAM_ENH	= 0x06,
	SNES_ROMTYPE_ROM_MASK		= 0x0F,

	// High nybble.
	SNES_ROMTYPE_ENH_NONE		= 0x00,
	SNES_ROMTYPE_ENH_DSP1		= 0x10,
	SNES_ROMTYPE_ENH_SUPERFX	= 0x20,
	SNES_ROMTYPE_ENH_OBC1		= 0x30,
	SNES_ROMTYPE_ENH_SA1		= 0xE0,
	SNES_ROMTYPE_ENH_OTHER		= 0xF0,
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
#define SNES_RomHeader_SIZE 80
#pragma pack(1)
typedef struct PACKED _SNES_RomHeader {
	/** Extended header is only present if old_publisher_code == 0x33. **/
	struct {
		char new_publisher_code[2];
		char id4[4];
		uint8_t reserved[7];	// Always 0x00.
		uint8_t exp_ram_size;	// Expansion RAM size.
		uint8_t special_version;
		uint8_t cart_type;
	} ext;

	/** Standard SNES header. **/
	char title[21];
	uint8_t rom_mapping;		// LoROM, HiROM
	uint8_t rom_type;
	uint8_t rom_size;		// ROM size. (1024 << rom_size)
	uint8_t sram_size;		// SRAM size. (1024 << sram_size);
	uint8_t destination_code;	// Destination code. (TODO: Enum?)
	uint8_t old_publisher_code;
	uint8_t version;
	uint16_t checksum_complement;
	uint16_t checksum;

	/** Vectors. **/
	struct {
		struct {
			uint8_t reserved[4];
			uint16_t cop;
			uint16_t brk;
			uint16_t abort;
			uint16_t nmi;
			uint16_t reset;
			uint16_t irq;
		} native;
		struct {
			uint8_t reserved1[4];
			uint16_t cop;
			uint8_t reserved2[2];
			uint16_t abort;
			uint16_t nmi;
			uint16_t res;
			union {
				// IRQ/BRK share the same vector
				// in 6502 emulation mode.
				uint16_t irq;
				uint16_t brk;
			};
		} emulation;
	} vectors;
} SNES_RomHeader;
#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_SNES_STRUCTS_H__ */
