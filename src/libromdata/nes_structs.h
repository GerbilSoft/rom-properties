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
 * References:
 * - https://wiki.nesdev.com/w/index.php/INES
 * - https://wiki.nesdev.com/w/index.php/NES_2.0
 * - https://wiki.nesdev.com/w/index.php/Family_Computer_Disk_System
 */

// Bank sizes for iNES.
#define INES_PRG_BANK_SIZE 16384
#define INES_CHR_BANK_SIZE 8192

// Bank sizes for TNES.
#define TNES_PRG_BANK_SIZE 8192
#define TNES_CHR_BANK_SIZE 8192

#pragma pack(1)
typedef struct PACKED _INES_RomHeader {
	uint8_t magic[4];	// "NES\x1A"
	uint8_t prg_banks;	// # of 16 KB PRG ROM banks.
	uint8_t chr_banks;	// # of 8 KB CHR ROM banks.

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
			uint8_t prg_banks_hi;
			uint8_t prg_ram_size;	// logarithmic
			uint8_t vram_size;	// logarithmic
			uint8_t tv_mode;	// 12
			uint8_t vs_hw;
		} nes2;
	};

	uint8_t reserved[2];
} INES_RomHeader;
#pragma pack()
ASSERT_STRUCT(INES_RomHeader, 16);

// mapper_lo flags.
typedef enum {
	// Mirroring.
	INES_F6_MIRROR_HORI = 0,
	INES_F6_MIRROR_VERT = (1 << 0),
	INES_F6_MIRROR_FOUR = (1 << 3),

	// Battery/trainer.
	INES_F6_BATTERY = (1 << 1),
	INES_F6_TRAINER = (1 << 2),

	// Mapper low nybble.
	INES_F6_MAPPER_MASK = 0xF0,
	INES_F6_MAPPER_SHIFT = 4,
} INES_Mapper_LO;

// mapper_hi flags.
typedef enum {
	// Hardware.
	INES_F7_SYSTEM_VS	= (1 << 0),
	INES_F7_SYSTEM_PC10	= (1 << 1),
	INES_F7_SYSTEM_MASK	= (INES_F7_SYSTEM_VS | INES_F7_SYSTEM_PC10),

	// NES 2.0 identification.
	INES_F7_NES2_MASK = (1 << 3) | (1 << 2),
	INES_F7_NES2_INES_VAL = 0,
	INES_F7_NES2_NES2_VAL = (1 << 3),

	// Mapper high nybble.
	INES_F7_MAPPER_MASK = 0xF0,
	INES_F7_MAPPER_SHIFT = 4,
} INES_Mapper_HI;

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
typedef enum {
	NES2_F12_NTSC = 0,
	NES2_F12_PAL = (1 << 0),
	NES2_F12_DUAL = (1 << 1),
	NES2_F12_REGION = (1 << 1) | (1 << 0),
} NES2_TV_Mode;

/**
 * TNES ROM header.
 * Used with Nintendo 3DS Virtual Console games.
 */
#pragma pack(1)
typedef struct PACKED _TNES_RomHeader {
	uint8_t magic[4];	// "TNES"
	uint8_t mapper;
	uint8_t prg_banks;	// # of 8 KB PRG ROM banks.
	uint8_t chr_banks;	// # of 8 KB CHR ROM banks.
	uint8_t wram;		// 00 == no; 01 == yes
	uint8_t mirroring;	// 00 == none; 01 == horizontal; 02 == vertical
	uint8_t vram;		// 00 == no; 01 == yes
	uint8_t reserved[6];
} TNES_RomHeader;
#pragma pack()
ASSERT_STRUCT(TNES_RomHeader, 16);

/**
 * TNES mappers.
 */
typedef enum {
	TNES_MAPPER_NROM	= 0,
	TNES_MAPPER_SxROM	= 1,
	TNES_MAPPER_PxROM	= 2,
	TNES_MAPPER_TxROM	= 3,
	TNES_MAPPER_FxROM	= 4,
	TNES_MAPPER_ExROM	= 5,
	TNES_MAPPER_UxROM	= 6,
	TNES_MAPPER_CNROM	= 7,
	TNES_MAPPER_AxROM	= 9,

	TNES_MAPPER_FDS		= 100,
} TNES_Mapper;

/**
 * TNES mirroring.
 */
typedef enum {
	TNES_MIRRORING_PROGRAMMABLE	= 0,	// Programmable
	TNES_MIRRORING_HORIZONTAL	= 1,	// Horizontal
	TNES_MIRRORING_VERTICAL		= 2,	// Vertical
} TNES_Mirroring;

/**
 * 3-byte BCD date stamp.
 */
#pragma pack(1)
typedef struct PACKED _FDS_BCD_DateStamp {
	uint8_t year;	// Add 1925 to this.
	uint8_t mon;	// 1-12
	uint8_t mday;	// 1-31
} FDS_BCD_DateStamp;
#pragma pack()
ASSERT_STRUCT(FDS_BCD_DateStamp, 3);

/**
 * Famicom Disk System header.
 */
#pragma pack(1)
typedef struct PACKED _FDS_DiskHeader {
	uint8_t block_code;	// 0x01
	uint8_t magic[14];	// "*NINTENDO-HVC*"
	uint8_t publisher_code;	// Old publisher code format
	char game_id[3];	// 3-character game ID.
	char game_type;		// Game type. (See FDS_Game_Type.)
	uint8_t revision;	// Revision.
	uint8_t side_number;	// Side number.
	uint8_t disk_number;	// Disk number.
	uint8_t disk_type;	// Disk type. (See FDS_Disk_Type.)
	uint8_t unknown1;
	uint8_t boot_read_file_code;	// File number to read on startup.
	uint8_t unknown2[5];		// 0xFF 0xFF 0xFF 0xFF 0xFF
	FDS_BCD_DateStamp mfr_date;	// Manufacturing date.
	uint8_t country_code;		// Country code. (0x49 == Japan)
	uint8_t unknown3[9];
	FDS_BCD_DateStamp rw_date;	// "Rewritten disk" date.
	uint8_t unknown4[2];
	uint16_t disk_writer_serial;	// Disk Writer serial number.
	uint8_t unknown5;
	uint8_t disk_rewrite_count;	// Stored in BCD format. $00 = original
	uint8_t actual_disk_side;
	uint8_t unknown6;
	uint8_t price;
	uint16_t crc;
} FDS_DiskHeader;
#pragma pack()
ASSERT_STRUCT(FDS_DiskHeader, 58);

typedef enum {
	FDS_GTYPE_NORMAL	= ' ',
	FDS_GTYPE_EVENT		= 'E',
	FDS_GTYPE_REDUCTION	= 'R',	// Sale!!!
} FDS_Game_Type;

typedef enum {
	FDS_DTYPE_FMC	= 0,	// FMC ("normal card")
	FDS_DTYPE_FSC	= 1,	// FSC ("card with shutter")
} FDS_Disk_type;

/**
 * fwNES FDS header.
 * If present, it's placed before the regular FDS header.
 */
#pragma pack(1)
typedef struct PACKED _FDS_DiskHeader_fwNES {
	uint8_t magic[4];	// "FDS\x1A"
	uint8_t disk_sides;	// Number of disk sides.
	uint8_t reserved[11];	// Zero filled.
} FDS_DiskHeader_fwNES;
#pragma pack()
ASSERT_STRUCT(FDS_DiskHeader_fwNES, 16);

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_NES_STRUCTS_H__ */
