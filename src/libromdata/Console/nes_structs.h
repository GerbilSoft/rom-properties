/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * nes_structs.h: Nintendo Entertainment System/Famicom data structures.   *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * Copyright (c) 2016-2017 by Egor.                                        *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

/**
 * References:
 * - https://wiki.nesdev.com/w/index.php/INES
 * - https://wiki.nesdev.com/w/index.php/NES_2.0
 * - https://wiki.nesdev.com/w/index.php/Family_Computer_Disk_System
 * - https://www.nesdev.org/wiki/Nintendo_header
 */

#pragma once

#include "common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Bank sizes for iNES.
#define INES_PRG_BANK_SIZE 16384
#define INES_CHR_BANK_SIZE 8192
#define INES_PRG_RAM_BANK_SIZE 8192

// Bank sizes for TNES.
#define TNES_PRG_BANK_SIZE 8192
#define TNES_CHR_BANK_SIZE 8192

/**
 * iNES ROM header.
 * This matches the ROM header format exactly.
 * References:
 * - https://wiki.nesdev.com/w/index.php/INES
 * - https://wiki.nesdev.com/w/index.php/NES_2.0
 *
 * All fields are in little-endian,
 * except for the magic number.
 *
 * NOTE: Strings are NOT null-terminated!
 */
#define INES_MAGIC		0x4E45531AU	// 'NES\x1A'
#define INES_MAGIC_WIIU_VC	0x4E455300U	// 'NES\x00'
typedef struct _INES_RomHeader {
	uint32_t magic;		// [0x000] 'NES\x1A' (big-endian)
	uint8_t prg_banks;	// [0x004] # of 16 KB PRG ROM banks.
	uint8_t chr_banks;	// [0x005] # of 8 KB CHR ROM banks.

	// Mapper values. Each byte has one
	// nybble, plus HW information.
	uint8_t mapper_lo;	// [0x006]
	uint8_t mapper_hi;	// [0x007]

	union {
		struct {
			uint8_t prg_ram_size;	// 8 KB units
			uint8_t tv_mode;
			// TODO: Byte 10?
		} ines;
		struct {
			uint8_t mapper_hi2;
			uint8_t banks_hi;	// [CCCC PPPP] High nybble of PRG/CHR bank size.
						// If PPPP == 0xF, an alternate method is used:
						// Low value is [EEEE EEMM] -> 2^E * (MM*2 + 1)
			uint8_t prg_ram_size;	// logarithmic
			uint8_t vram_size;	// logarithmic
			uint8_t tv_mode;	// 12
			uint8_t vs_hw;		// 13: Vs. System Type if (mapper_hi & 7) == 1
			                        //   Extd Console Type if (mapper_hi & 7) == 3
			uint8_t misc_roms;	// 14: Number of miscellaneous ROMs present.
			                        //     (Low two bits only.)
			uint8_t expansion;	// 15: Default expansion device. (& 0x3F)
			                        // See NES2_Expansion_e.
		} nes2;
	};
} INES_RomHeader;
ASSERT_STRUCT(INES_RomHeader, 16);

// mapper_lo flags.
typedef enum {
	// Mirroring.
	INES_F6_MIRROR_HORI = 0,
	INES_F6_MIRROR_VERT = (1U << 0),
	INES_F6_MIRROR_FOUR = (1U << 3),

	// Battery/trainer.
	INES_F6_BATTERY = (1U << 1),
	INES_F6_TRAINER = (1U << 2),

	// Mapper low nybble.
	INES_F6_MAPPER_MASK = 0xF0,
	INES_F6_MAPPER_SHIFT = 4,
} INES_Mapper_LO;

// mapper_hi flags.
typedef enum {
	// Hardware.
	INES_F7_SYSTEM_VS	= 1,
	INES_F7_SYSTEM_PC10	= 2,
	INES_F7_SYSTEM_EXTD	= 3,	// Extended Console Type (NES2)
	INES_F7_SYSTEM_MASK	= 3,

	// NES 2.0 identification.
	INES_F7_NES2_MASK = (1U << 3) | (1U << 2),
	INES_F7_NES2_INES_VAL = 0,
	INES_F7_NES2_NES2_VAL = (1U << 3),

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

// Byte 12 - CPU/PPU Timing (TV mode)
typedef enum {
	NES2_F12_NTSC		= 0,	// RP2C02
	NES2_F12_PAL		= 1,	// RP2C07
	NES2_F12_REGION_FREE	= 2,	// Multi-region
	NES2_F12_DENDY		= 3,	// UMC 6527P
} NES2_TV_Mode_e;

// Byte 13 - Vs. System Type (mapper_hi & 7 == 1)
// Low nybble: PPU type
typedef enum {
	VS_PPU_RP2C03B		= 0x0,
	VS_PPU_RP2C03G		= 0x1,
	VS_PPU_RP2C04_0001	= 0x2,
	VS_PPU_RP2C04_0002	= 0x3,
	VS_PPU_RP2C04_0003	= 0x4,
	VS_PPU_RP2C04_0004	= 0x5,
	VS_PPU_RC2C03B		= 0x6,
	VS_PPU_RC2C03C		= 0x7,
	VS_PPU_RC2C05_01	= 0x8,	// $2002 AND $?? == $1B
	VS_PPU_RC2C05_02	= 0x9,	// $2002 AND $3F == $3D
	VS_PPU_RC2C05_03	= 0xA,	// $2002 AND $1F == $1C
	VS_PPU_RC2C05_04	= 0xB,	// $2002 AND $1F == $1B
	VS_PPU_RC2C05_05	= 0xC,	// $2002 AND $1F == unknown
} NES2_VS_PPU_Type_e;

// Byte 13 - Vs. System Type (mapper_hi & 7 == 1)
// High nybble: Hardware type
typedef enum {
	VS_HW_UNISYSTEM				= 0x0,	// Normal
	VS_HW_UNISYSTEM_RBI_BASEBALL		= 0x1,
	VS_HW_UNISYSTEM_TKO_BOXING		= 0x2,
	VS_HW_UNISYSTEM_SUPER_XEVIOUS		= 0x3,
	VS_HW_UNISYSTEM_VS_ICE_CLIMBER_JPN	= 0x4,
	VS_HW_DUALSYSTEM			= 0x5,	// Normal
	VS_HW_DUALSYSTEM_RAID_ON_BUNGELING_BAY	= 0x6,
} NES2_VS_Hardware_Type_e;

// Byte 13 - Extended Console Type (mapper_hi & 7 == 3)
// Low nybble: Console type.
typedef enum {
	NES2_CT_NES		= 0x0,	// Not normally used.
	NES2_CT_VS_SYSTEM	= 0x1,	// Not normally used.
	NES2_CT_PLAYCHOICE_10	= 0x2,	// Not normally used.
	NES2_CT_FAMICLONE_BCD	= 0x3,
	NES2_CT_VT01_MONO	= 0x4,
	NES2_CT_VT01_RED_CYAN	= 0x5,
	NES2_CT_VT02		= 0x6,
	NES2_CT_VT03		= 0x7,
	NES2_CT_VT09		= 0x8,
	NES2_CT_VT32		= 0x9,
	NES2_CT_VT369		= 0xA,
	NES2_CT_UMC_UM6578	= 0xB,
} NES2_Console_Type_e;

// Byte 15 - Default Expansion Device (& 0x3F)
typedef enum {
	NES2_EXP_UNSPECIFIED			= 0x00,
	NES2_EXP_STANDARD			= 0x01,
	NES2_EXP_NES_4P				= 0x02,
	NES2_EXP_FC_4P				= 0x03,
	NES2_EXP_VS				= 0x04,
	NES2_EXP_VS_REVERSED			= 0x05,
	NES2_EXP_VS_PINBALL			= 0x06,
	NES2_EXP_VS_ZAPPER			= 0x07,
	NES2_EXP_ZAPPER				= 0x08,
	NES2_EXP_2X_ZAPPERS			= 0x09,
	NES2_EXP_BANDAI_HYPER_SHOT		= 0x0A,
	NES2_EXP_POWER_PAD_SIDE_A		= 0x0B,
	NES2_EXP_POWER_PAD_SIDE_B		= 0x0C,
	NES2_EXP_FAMILY_TRAINER_SIDE_A		= 0x0D,
	NES2_EXP_FAMILY_TRAINER_SIDE_B		= 0x0E,
	NES2_EXP_ARKANOID_NES			= 0x0F,
	NES2_EXP_ARKANOID_FC			= 0x10,
	NES2_EXP_ARKANOID_FC_RECORDER		= 0x11,
	NES2_EXP_KONAMI_HYPER_SHOT		= 0x12,
	NES2_EXP_COCONUTS_PACHINKO		= 0x13,
	NES2_EXP_EXCITING_BOXING_BAG		= 0x14,
	NES2_EXP_JISSEN_MAHJONG			= 0x15,
	NES2_EXP_PARTY_TAP			= 0x16,
	NES2_EXP_OEKA_KIDS_TABLET		= 0x17,
	NES2_EXP_SUNSOFT_BARCODE_BATTLER	= 0x18,
	NES2_EXP_MIRACLE_PIANO_KEYBOARD		= 0x19,
	NES2_EXP_POKKUN_MOGURAA			= 0x1A,
	NES2_EXP_TOP_RIDER			= 0x1B,
	NES2_EXP_DOUBLE_FISTED			= 0x1C,
	NES2_EXP_FAMICOM_3D_SYSTEM		= 0x1D,
	NES2_EXP_DOREMIKKO_KEYBOARD		= 0x1E,
	NES2_EXP_ROB_GYRO_SET			= 0x1F,
	NES2_EXP_FAMICOM_DATA_RECORDER_NO_KBD	= 0x20,
	NES2_EXP_ASCII_TURBO_FILE		= 0x21,
	NES2_EXP_IGS_STORAGE_BATTLE_BOX		= 0x22,
	NES2_EXP_FAMILY_BASIC_KEYBOARD_AND_REC	= 0x23,
	NES2_EXP_DONGDA_PEC_586_KEYBOARD	= 0x24,
	NES2_EXP_BIT_CORP_BIT_79_KEYBOARD	= 0x25,
	NES2_EXP_SUBOR_KEYBOARD			= 0x26,
	NES2_EXP_SUBOR_KEYBOARD_MOUSE_3x8	= 0x27,
	NES2_EXP_SUBOR_KEYBOARD_MOUSE_24	= 0x28,
	NES2_EXP_SNES_MOUSE			= 0x29,
	NES2_EXP_MULTICART			= 0x2A,
	NES2_EXP_SNES_CONTROLLERS		= 0x2B,
	NES2_EXP_RACERMATE_BICYCLE		= 0x2C,
	NES2_EXP_UFORCE				= 0x2D,
	NES2_EXP_ROB_STACKUP			= 0x2E,
	NES2_EXP_CITY_PATROLMAN_LIGHTGUN	= 0x2F,
} NES2_Expansion_e;

/**
 * Internal NES footer.
 * Located at the last 32 bytes of the last PRG bank in some ROMs.
 *
 * References:
 * - http://forums.no-intro.org/viewtopic.php?f=2&t=445
 * - https://github.com/GerbilSoft/rom-properties/issues/116
 * - https://www.nesdev.org/wiki/Nintendo_header
 *
 * TODO: Add enums?
 */
typedef union _NES_IntFooter {
	struct {
		char title[16];		// [0x000] Title. (right-aligned with 0xFF filler bytes)
		uint16_t prg_checksum;	// [0x010] PRG checksum
		uint16_t chr_checksum;	// [0x012] CHR checksum
		uint8_t rom_size;	// [0x014] ROM sizes: [PPPP TCCC]
					//         PPPP = PRG ROM
					//                (0=64KB, 1=16KB, 2=32KB, 3=128KB, 4=256KB, 5=512KB)
					//            T = CHR type: 0 = ROM, 1 = RAM
					//          CCC = CHR ROM
					//                (0=8KB, 1=16KB, 2=32KB, 3=64/128KB, 4=256KB)
		uint8_t board_info;	// [0x015] Board information.
					//         Bit 7: Mirroring (1=vertical, 0=horizontal)
					//         Bits 6-0: Mapper (see NES_IntFooter_Mapper_e)
		uint8_t title_encoding;	// [0x016] Title encoding: 0=None, 1=ASCII, 2=JIS X 0201 (Shift-JIS?)
		uint8_t title_length;	// [0x017] 0=None; 1-15 = 2-16 bytes (sometimes off by one)
		uint8_t publisher_code;	// [0x018] Old publisher code.
		uint8_t checksum;	// [0x019] Checksum: sum of [FFF2,FFF9] == 0
		uint16_t nmi_vector;	// [0x01A] NMI vector.
		uint16_t reset_vector;	// [0x01C] Reset vector.
		uint16_t irq_vector;	// [0x01E] IRQ vector.
	};
	uint8_t u8[32];
} NES_IntFooter;
ASSERT_STRUCT(NES_IntFooter, 32);

/**
 * NES internal footer: Mappers
 */
typedef enum {
	NES_INTFOOTER_MAPPER_NROM	= 0,
	NES_INTFOOTER_MAPPER_CNROM	= 1,
	NES_INTFOOTER_MAPPER_UNROM	= 2,
	NES_INTFOOTER_MAPPER_GNROM	= 3,
	NES_INTFOOTER_MAPPER_MMCx	= 4,
} NES_IntFooter_Mapper_e;

/**
 * NES internal footer: Encoding
 */
typedef enum {
	NES_INTFOOTER_ENCODING_NONE	= 0,
	NES_INTFOOTER_ENCODING_ASCII	= 1,
	NES_INTFOOTER_ENCODING_SJIS	= 2,
} NES_IntFooter_Encoding_e;

/**
 * TNES ROM header.
 * Used with Nintendo 3DS Virtual Console games.
 *
 * All fields are in little-endian,
 * except for the magic number.
 */
#define TNES_MAGIC 'TNES'
typedef struct _TNES_RomHeader {
	uint32_t magic;		// [0x000] 'TNES' (big-endian)
	uint8_t mapper;		// [0x004]
	uint8_t prg_banks;	// [0x005] # of 8 KB PRG ROM banks.
	uint8_t chr_banks;	// [0x006] # of 8 KB CHR ROM banks.
	uint8_t wram;		// [0x007] 00 == no; 01 == yes
	uint8_t mirroring;	// [0x008] 00 == none; 01 == horizontal; 02 == vertical
	uint8_t vram;		// [0x009] 00 == no; 01 == yes
	uint8_t reserved[6];	// [0x00A]
} TNES_RomHeader;
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
// Some compilers pad this structure to a multiple of 4 bytes
#pragma pack(1)
typedef struct RP_PACKED _FDS_BCD_DateStamp {
	uint8_t year;	// [0x000] Year, using Japanese eras:
	                // - >=58 (1983+): Shōwa era (1926-1989); add 1925
	                // - <=57: Heisei era (1989-2019); add 1988
	                // NOTE: Using 1983 as a lower bound for Shōwa instead
	                // of 1986 just in case.
	uint8_t mon;	// 1-12
	uint8_t mday;	// 1-31
} FDS_BCD_DateStamp;
ASSERT_STRUCT(FDS_BCD_DateStamp, 3);
#pragma pack()

/**
 * Famicom Disk System header.
 */
#pragma pack(1)
typedef struct RP_PACKED _FDS_DiskHeader {
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
ASSERT_STRUCT(FDS_DiskHeader, 58);
#pragma pack()

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
 *
 * All fields are in little-endian,
 * except for the magic number.
 */
#define fwNES_MAGIC 0x4644531AU	// 'FDS\x1A'
typedef struct _FDS_DiskHeader_fwNES {
	uint32_t magic;		// [0x000] 'FDS\x1A' (big-endian)
	uint8_t disk_sides;	// [0x004] Number of disk sides.
	uint8_t reserved[11];	// [0x005] Zero filled.
} FDS_DiskHeader_fwNES;
ASSERT_STRUCT(FDS_DiskHeader_fwNES, 16);

#ifdef __cplusplus
}
#endif
