/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * nes_structs.h: Nintendo Entertainment System/Famicom data structures.   *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 * Copyright (c) 2016-2017 by Egor.                                        *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_NES_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_NES_STRUCTS_H__

#include "common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * NES ROM header.
 * Reference: https://wiki.nesdev.com/w/index.php/INES
 */
#pragma pack(1)
struct PACKED NES_RomHeader {
	uint8_t magic[4]; // "NES\x1A"
	uint8_t prgrom;
	uint8_t chrrom;

	// Mapper values. Each byte has one
	// nybble, plus HW information.
	uint8_t mapper_lo;	// byte 6
	uint8_t mapper_hi;	// byte 7

	union {
		struct {
			uint8_t prg_ram_size;	// 8 KB units
			uint8_t tv_mode;
			// TODO: Byte 10?
		} ines;
		struct {
			uint8_t mapper_hi2;
			uint8_t rom_size_hi;
			uint8_t prg_ram_size;	// logarithmic
			uint8_t vram_size;	// logarithmic
			uint8_t tv_mode;	// 12
			uint8_t vs_ppu_variant;
		} nes2;
	};

	uint8_t reserved[2];
};
#pragma pack()
ASSERT_STRUCT(NES_RomHeader, 16);

// mapper_lo flags.
enum NES_Mapper_LO {
	// Mirroring.
	NES_F6_MIRROR_HORI = 0,
	NES_F6_MIRROR_VERT = (1 << 0),
	NES_F6_MIRROR_FOUR = (1 << 3),

	// Battery/trainer.
	NES_F6_BATTERY = (1 << 1),
	NES_F6_TRAINER = (1 << 2),

	// Mapper low nybble.
	NES_F6_MAPPER_MASK = 0xF0,
	NES_F6_MAPPER_SHIFT = 4,
};

// mapper_hi flags.
enum NES_Mapper_HI {
	// Hardware.
	NES_F7_VS	= (1 << 0),
	NES_F7_PC10	= (1 << 1),

	// NES 2.0 identification.
	NES_F7_NES2_MASK = (1 << 3) | (1 << 2),
	NES_F7_NES2_INES_VAL = 0,
	NES_F7_NES2_NES2_VAL = (1 << 3),

	// Mapper high nybble.
	NES_F7_MAPPER_MASK = 0xF0,
	NES_F7_MAPPER_SHIFT = 4,
};

// NES 2.0 stuff
// Not gonna make enums for those:
// Byte 8 - Mapper variant
//   top nibble = submapper, bottom nibble = mapper plane
// Byte 9 - Rom size upper bits
//   top = CROM, bottom = PROM
// Byte 10 - pram
//   top = battery pram, bottom = normal pram
// Byte 11 - cram
//   top = battery cram, bottom = normal cram
// Byte 13 - vs unisystem
//   top = vs mode, bottom = ppu version
enum NES2_TV_Mode {
	NES2_F12_NTSC = 0,
	NES2_F12_PAL = (1 << 0),
	NES2_F12_DUAL = (1 << 1),
	NES2_F12_REGION = (1 << 1) | (1 << 0),
};

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_NDS_STRUCTS_H__ */
