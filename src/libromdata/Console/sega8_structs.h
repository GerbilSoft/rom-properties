/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * sega8_structs.h: Sega 8-bit (SMS/GG) data structures.                   *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_SEGA8_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_SEGA8_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

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
typedef struct PACKED _Sega8_RomHeader {
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
 * Codemasters ROM header.
 * This matches the Codemasters ROM header format exactly.
 * Reference: http://www.smspower.org/Development/CodemastersHeader
 *
 * All fields are in little-endian.
 *
 * Located at $7FE0.
 */
typedef struct PACKED _Sega8_Codemasters_RomHeader {
	uint8_t checksum_banks;		// Number of 16KB banks over which to calculate the checksum.
	struct {
		// Build timestamp.
		// All fields are BCD.
		uint8_t day;
		uint8_t month;
		uint8_t year;
		uint8_t hour;		// 24-hour clock
		uint8_t minute;
	} timestamp;
	uint16_t checksum;
	uint16_t checksum_compl;	// 0x10000 - checksum
	uint8_t reserved[6];		// all zero
} Sega8_Codemasters_RomHeader;
ASSERT_STRUCT(Sega8_Codemasters_RomHeader, 16);

/**
 * SDSC ROM header.
 * This matches the SDSC ROM header format exactly.
 * Reference: http://www.smspower.org/Development/SDSCHeader
 *
 * All fields are in little-endian.
 *
 * Located at $7FE0.
 */
#define SDSC_MAGIC "SDSC"
typedef struct PACKED _Sega8_SDSC_RomHeader {
	char magic[4];		// "SDSC"
	uint8_t version[2];	// Program version, in BCD.
				// [0] = major version
				// [1] = minor version
	struct {
		// Build date, in BCD.
		uint8_t day;
		uint8_t month;
		uint8_t year;
		uint8_t century;
	} date;

	// The following are pointers to ASCII C-strings in the ROM image.
	// $FFFF indicates no string.
	uint16_t author_ptr;	// Author's name.
	uint16_t name_ptr;	// Program name.
	uint16_t desc_ptr;	// Program description.
} Sega8_SDSC_RomHeader;
ASSERT_STRUCT(Sega8_SDSC_RomHeader, 16);

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_SEGA8_STRUCTS_H__ */
