/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * sega8_structs.h: Sega 8-bit (SMS/GG) data structures.                   *
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
 * Sega Master System ROM header.
 * This matches the SMS ROM header format exactly.
 * Reference: http://www.smspower.org/Development/ROMHeader
 *
 * All fields are in little-endian.
 *
 * Located at $7FF0, $3FF0, or $1FF0.
 * Note that $7FF0 is the only one used in any released titles.
 */
#define SEGA8_MAGIC "TMR SEGA"
typedef struct _Sega8_RomHeader {
	char magic[8];			// "TMR SEGA"
	uint8_t reserved[2];		// $00 $00, $FF $FF, $20 $20
	uint16_t checksum;		// ROM checksum. (may not be correct)
	uint8_t product_code[3];	// 5-digit BCD product code.
					// [0] = last two digits
					// [1] = first two digits
					// High 4 bits of [2], if non-zero,
					// is an extra leading digit, which
					// *may* be >9, in which case it's
					// two digits.
					// Low 4 bits of [2] is the version.
	uint8_t region_and_size;	// High 4 bits: Sega8_Region_Code
					// Low 4 bits: Sega8_ROM_Size
} Sega8_RomHeader;
ASSERT_STRUCT(Sega8_RomHeader, 16);

// Region code and system ID.
typedef enum {
	Sega8_SMS_Japan		= 0x3,
	Sega8_SMS_Export	= 0x4,
	Sega8_GG_Japan		= 0x5,
	Sega8_GG_Export		= 0x6,
	Sega8_GG_International	= 0x7,
} Sega8_Region_Code;

// ROM size.
typedef enum {
	Sega8_ROM_8KB		= 0xA,	// unused
	Sega8_ROM_16KB		= 0xB,	// unused
	Sega8_ROM_32KB		= 0xC,
	Sega8_ROM_48KB		= 0xD,	// unused, buggy
	Sega8_ROM_64KB		= 0xE,	// rarely used
	Sega8_ROM_128KB		= 0xF,
	Sega8_ROM_256KB		= 0x0,
	Sega8_ROM_512KB		= 0x1,	// rarely used
	Sega8_ROM_1MB		= 0x2,	// buggy
} Sega8_ROM_Size;

/**
 * Codemasters timestamp struct.
 * NOTE: Fields are in BCD.
 * Reference: http://www.smspower.org/Development/CodemastersHeader
 */
// Some compilers pad this structure to a multiple of 4 bytes
#pragma pack(1)
typedef struct RP_PACKED _Sega8_Codemasters_Timestamp {
	uint8_t day;
	uint8_t month;
	uint8_t year;
	uint8_t hour;
	uint8_t minute;
} Sega8_Codemasters_Timestamp;
ASSERT_STRUCT(Sega8_Codemasters_Timestamp, 5);
#pragma pack()

/**
 * Codemasters ROM header.
 * This matches the Codemasters ROM header format exactly.
 * Reference: http://www.smspower.org/Development/CodemastersHeader
 *
 * All fields are in little-endian.
 *
 * Located at $7FE0.
 */
typedef struct _Sega8_Codemasters_RomHeader {
	uint8_t checksum_banks;		// [0x000] Number of 16KB banks over which to calculate the checksum.
	Sega8_Codemasters_Timestamp timestamp;	// [0x001] Timestamp.
	uint16_t checksum;		// [0x006]
	uint16_t checksum_compl;	// [0x008] 0x10000 - checksum
	uint8_t reserved[6];		// [0x00A] all zero
} Sega8_Codemasters_RomHeader;
ASSERT_STRUCT(Sega8_Codemasters_RomHeader, 16);

/**
 * SDSC date struct.
 * NOTE: Fields are in BCD.
 * Reference: http://www.smspower.org/Development/SDSCHeader
 */
typedef struct _Sega8_SDSC_Date {
	uint8_t day;
	uint8_t month;
	uint8_t year;
	uint8_t century;
} Sega8_SDSC_Date;
ASSERT_STRUCT(Sega8_SDSC_Date, 4);

/**
 * SDSC ROM header.
 * This matches the SDSC ROM header format exactly.
 * Reference: http://www.smspower.org/Development/SDSCHeader
 *
 * All fields are in little-endian.
 *
 * Located at $7FE0.
 */
#define SDSC_MAGIC 'SDSC'
#pragma pack(1)
typedef struct RP_PACKED _Sega8_SDSC_RomHeader {
	uint32_t magic;		// [0x000] 'SDSC'
	uint8_t version[2];	// [0x004] Program version, in BCD.
				//         [0] = major version
				//         [1] = minor version
	// Some compilers add padding here
	Sega8_SDSC_Date date;	// [0x006] Build date.

	// The following are pointers to ASCII C-strings in the ROM image.
	// $FFFF indicates no string.
	uint16_t author_ptr;	// [0x00A] Author's name.
	uint16_t name_ptr;	// [0x00C] Program name.
	uint16_t desc_ptr;	// [0x00E] Program description.
} Sega8_SDSC_RomHeader;
ASSERT_STRUCT(Sega8_SDSC_RomHeader, 16);
#pragma pack()

#ifdef __cplusplus
}
#endif
