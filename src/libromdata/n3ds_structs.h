/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * n3ds_structs.h: Nintendo 3DS data structures.                           *
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

// References:
// - https://www.3dbrew.org/wiki/SMDH
// - https://github.com/devkitPro/3dstools/blob/master/src/smdhtool.cpp
// - https://3dbrew.org/wiki/3DSX_Format
// - https://3dbrew.org/wiki/CIA

#ifndef __ROMPROPERTIES_LIBROMDATA_N3DS_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_N3DS_STRUCTS_H__

#include "common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Nintendo 3DS SMDH title struct.
 * All fields are UTF-16LE.
 * NOTE: Strings may not be NULL-terminated!
 */
#pragma pack(1)
typedef struct PACKED _N3DS_SMDH_Title_t {
	char16_t desc_short[64];
	char16_t desc_long[128];
	char16_t publisher[64];
} N3DS_SMDH_Title_t;
#pragma pack()
ASSERT_STRUCT(N3DS_SMDH_Title_t, 512);

/**
 * Nintendo 3DS SMDH settings struct.
 * All fields are little-endian.
 */
#pragma pack(1)
typedef struct PACKED _N3DS_SMDH_Settings_t {
	uint8_t ratings[16];	// Region-specific age ratings.
	uint32_t region_code;	// Region code. (bitfield)
	uint32_t match_maker_id;
	uint64_t match_maker_bit_id;
	uint32_t flags;
	uint16_t eula_version;
	uint8_t reserved[2];
	uint32_t animation_default_frame;
	uint32_t cec_id;	// StreetPass ID
} N3DS_SMDH_Settings_t;
#pragma pack()
ASSERT_STRUCT(N3DS_SMDH_Settings_t, 48);

/**
 * Age rating indexes.
 */
typedef enum {
	N3DS_RATING_JAPAN	= 0,	// CERO
	N3DS_RATING_USA		= 1,	// ESRB
	N3DS_RATING_GERMANY	= 3,	// USK
	N3DS_RATING_PEGI	= 4,	// PEGI
	N3DS_RATING_PORTUGAL	= 6,	// PEGI (Portugal)
	N3DS_RATING_BRITAIN	= 7,	// BBFC
	N3DS_RATING_AUSTRALIA	= 8,	// ACB
	N3DS_RATING_SOUTH_KOREA	= 9,	// GRB
	N3DS_RATING_TAIWAN	= 10,	// CGSRR
} N3DS_Age_Rating_Region;

/**
 * Region code bits.
 */
typedef enum {
	N3DS_REGION_JAPAN	= (1 << 0),
	N3DS_REGION_USA		= (1 << 1),
	N3DS_REGION_EUROPE	= (1 << 2),
	N3DS_REGION_AUSTRALIA	= (1 << 3),
	N3DS_REGION_CHINA	= (1 << 4),
	N3DS_REGION_SOUTH_KOREA	= (1 << 5),
	N3DS_REGION_TAIWAN	= (1 << 6),
} N3DS_Region_Code;

/**
 * Flag bits.
 */
typedef enum {
	N3DS_FLAG_VISIBLE		= (1 << 0),
	N3DS_FLAG_AUTOBOOT		= (1 << 1),
	N3DS_FLAG_USE_3D		= (1 << 2),
	N3DS_FLAG_REQUIRE_EULA		= (1 << 3),
	N3DS_FLAG_AUTOSAVE		= (1 << 4),
	N3DS_FLAG_EXT_BANNER		= (1 << 5),
	N3DS_FLAG_AGE_RATING_REQUIRED	= (1 << 6),
	N3DS_FLAG_HAS_SAVE_DATA		= (1 << 7),
	N3DS_FLAG_RECORD_USAGE		= (1 << 8),
	N3DS_FLAG_DISABLE_SD_BACKUP	= (1 << 10),
	N3DS_FLAG_NEW3DS_ONLY		= (1 << 12),
} N3DS_SMDH_Flags;

/**
 * Nintendo 3DS SMDH header.
 * SMDH files contain a description of the title as well
 * as large and small icons.
 * Reference: https://www.3dbrew.org/wiki/SMDH
 *
 * All fields are little-endian.
 * NOTE: Strings may not be NULL-terminated!
 */
#pragma pack(1)
#define N3DS_SMDH_HEADER_MAGIC "SMDH"
typedef struct PACKED _N3DS_SMDH_Header_t {
	char magic[4];		// "SMDH"
	uint16_t version;
	uint8_t reserved1[2];
	N3DS_SMDH_Title_t titles[16];
	N3DS_SMDH_Settings_t settings;
	uint8_t reserved2[8];
} N3DS_SMDH_Header_t;
#pragma pack()
ASSERT_STRUCT(N3DS_SMDH_Header_t, 8256);

/**
 * Language IDs.
 * These are indexes in N3DS_SMDH_Header_t.titles[].
 */
typedef enum {
	N3DS_LANG_JAPANESE	= 0,
	N3DS_LANG_ENGLISH	= 1,
	N3DS_LANG_FRENCH	= 2,
	N3DS_LANG_GERMAN	= 3,
	N3DS_LANG_ITALIAN	= 4,
	N3DS_LANG_SPANISH	= 5,
	N3DS_LANG_CHINESE_SIMP	= 6,
	N3DS_LANG_KOREAN	= 7,
	N3DS_LANG_DUTCH		= 8,
	N3DS_LANG_PORTUGUESE	= 9,
	N3DS_LANG_RUSSIAN	= 10,
	N3DS_LANG_CHINESE_TRAD	= 11,
} N3DS_Language_ID;

/**
 * Nintendo 3DS SMDH icon data.
 * NOTE: Assumes RGB565, though other formats
 * are supposedly usable. (No way to tell what
 * format is being used as of right now.)
 */
#define N3DS_SMDH_ICON_SMALL_W 24
#define N3DS_SMDH_ICON_SMALL_H 24
#define N3DS_SMDH_ICON_LARGE_W 48
#define N3DS_SMDH_ICON_LARGE_H 48
#pragma pack(1)
typedef struct PACKED _N3DS_SMDH_Icon_t {
	uint16_t small[N3DS_SMDH_ICON_SMALL_W * N3DS_SMDH_ICON_SMALL_H];
	uint16_t large[N3DS_SMDH_ICON_LARGE_W * N3DS_SMDH_ICON_LARGE_H];
} N3DS_SMDH_Icon_t;
#pragma pack()
ASSERT_STRUCT(N3DS_SMDH_Icon_t, 0x1680);

/**
 * Nintendo 3DS Homebrew Application header. (.3dsx)
 * Reference: https://3dbrew.org/wiki/3DSX_Format
 *
 * All fields are little-endian.
 */
#define N3DS_3DSX_HEADER_MAGIC "3DSX"
#define N3DS_3DSX_STANDARD_HEADER_SIZE 32
#define N3DS_3DSX_EXTENDED_HEADER_SIZE 44
#pragma pack(1)
typedef struct PACKED _N3DS_3DSX_Header_t {
	// Standard header.
	char magic[4];			// "3DSX"
	uint16_t header_size;		// Header size.
	uint16_t reloc_header_size;	// Relocation header size.
	uint32_t format_version;
	uint32_t flags;
	uint32_t code_segment_size;
	uint32_t rodata_segment_size;
	uint32_t data_segment_size;	// Includes BSS.
	uint32_t bss_segment_size;

	// Extended header. (only valid if header_size > 32)
	uint32_t smdh_offset;
	uint32_t smdh_size;
	uint32_t romfs_offset;
} N3DS_3DSX_Header_t;
#pragma pack()
ASSERT_STRUCT(N3DS_3DSX_Header_t, 44);

/**
 * Nintendo 3DS Installable Archive. (.cia)
 * Reference: https://www.3dbrew.org/wiki/CIA
 *
 * All fields are little-endian.
 */
#pragma pack(1)
typedef struct PACKED _N3DS_CIA_Header_t {
	uint32_t header_size;	// Usually 0x2020.
	uint16_t type;
	uint16_t version;
	uint32_t cert_chain_size;
	uint32_t ticket_size;
	uint32_t tmd_size;
	uint32_t meta_size;	// SMDH at the end of the file if non-zero.
	uint64_t content_size;
	uint8_t content_index[0x2000];	// TODO
} N3DS_CIA_Header_t;
#pragma pack()
ASSERT_STRUCT(N3DS_CIA_Header_t, 0x2020);

// Order of sections within CIA file:
// - CIA header
// - Certificate chain
// - Ticket
// - TMD
// - Content
// - Meta (optional)

/**
 * CIA: Meta section header.
 * All fields are little-endian.
 */
#pragma pack(1)
typedef struct PACKED _N3DS_CIA_Meta_Header_t {
	uint64_t tid_dep_list[48];	// Title ID dependency list.
	uint8_t reserved1[0x180];
	uint32_t core_version;
	uint8_t reserved2[0xFC];

	// Meta header is followed by an SMDH.
} N3DS_CIA_Meta_Header_t;
#pragma pack()
ASSERT_STRUCT(N3DS_CIA_Meta_Header_t, 0x400);

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_N3DS_STRUCTS_H__ */
