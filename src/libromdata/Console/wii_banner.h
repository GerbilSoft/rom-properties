/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * wii_banner.h: Nintendo Wii banner structures.                           *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_WII_BANNER_H__
#define __ROMPROPERTIES_LIBROMDATA_WII_BANNER_H__

#include <stdint.h>
#include "librpbase/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

/**
 * WIBN (Wii Banner)
 * Reference: http://wiibrew.org/wiki/Savegame_Files
 * NOTE: This may be located at one of two places:
 * - 0x0000: banner.bin extracted via SaveGame Manager GX
 * - 0x0020: Savegame extracted via Wii System Menu
 */

// Magic numbers.
#define BANNER_WIBN_MAGIC		0x5749424E	/* 'WIBN' */
#define BANNER_WIBN_ADDRESS_RAW		0x0000		/* banner.bin from SaveGame Manager GX */
#define BANNER_WIBN_ADDRESS_ENCRYPTED	0x0020		/* extracted from Wii System Menu */

// Flags.
#define BANNER_WIBN_FLAGS_NOCOPY	0x01
#define BANNER_WIBN_FLAGS_ICON_BOUNCE	0x10

// Banner size.
#define BANNER_WIBN_IMAGE_W 192
#define BANNER_WIBN_IMAGE_H 64

// Icon size.
#define BANNER_WIBN_ICON_W 48
#define BANNER_WIBN_ICON_H 48

// Struct size.
#define BANNER_WIBN_IMAGE_SIZE (BANNER_WIBN_IMAGE_W * BANNER_WIBN_IMAGE_H * 2)
#define BANNER_WIBN_ICON_SIZE (BANNER_WIBN_ICON_W * BANNER_WIBN_ICON_H * 2)
#define BANNER_WIBN_STRUCT_SIZE (sizeof(Wii_WIBN_Header_t) + BANNER_WIBN_IMAGE_SIZE)
#define BANNER_WIBN_STRUCT_SIZE_ICONS(icons) \
	(BANNER_WIBN_STRUCT_SIZE + ((icons)*BANNER_WIBN_ICON_SIZE))

/**
 * Wii save game banner header.
 * Reference: http://wiibrew.org/wiki/Savegame_Files#Banner
 *
 * All fields are in big-endian.
 */
#define WII_WIBN_MAGIC 'WIBN'
typedef struct PACKED _Wii_WIBN_Header_t {
	uint32_t magic;			// [0x000] 'WIBN'
	uint32_t flags;			// [0x004] Flags. (See Wii_WIBN_Flags_e.)
	uint16_t iconspeed;		// [0x008] Similar to GCN.
	uint8_t reserved[22];		// [0x00A]
	char16_t gameTitle[32];		// [0x020] Game title. (UTF-16 BE)
	char16_t gameSubTitle[32];	// [0x060] Game subtitle. (UTF-16 BE)
} Wii_WIBN_Header_t;
ASSERT_STRUCT(Wii_WIBN_Header_t, 160);

/**
 * Wii save game flags.
 */
typedef enum {
	WII_WIBN_FLAG_NO_COPY		= (1 << 0),	// Cannot copy from NAND normally.
	WII_WIBN_FLAG_ICON_BOUNCE	= (1 << 4),	// Icon animation "bounces" instead of looping.
} Wii_WIBN_Flags_e;

// IMET magic number.
#define WII_IMET_MAGIC 0x494D4554	/* 'IMET' */

/**
 * IMET (Wii opening.bnr header)
 * This contains the game title.
 * Reference: http://wiibrew.org/wiki/Opening.bnr#banner.bin_and_icon.bin
 *
 * All fields are in big-endian.
 */
typedef struct _Wii_IMET_t {
	uint8_t zeroes1[64];
	uint32_t magic;		// "IMET"
	uint32_t hashsize;	// Hash length
	uint32_t unknown;
	uint32_t sizes[3];	// icon.bin, banner.bin, sound.bin
	uint32_t flag1;

	// Titles. (UTF-16BE)
	// - Index 0: Language: JP,EN,DE,FR,ES,IT,NL,xx,xx,KO
	// - Index 1: Line
	// - Index 2: Character
	char16_t names[10][2][21];

	uint8_t zeroes2[588];
	uint8_t md5[16];	// MD5 of 0 to 'hashsize' in the header.
				// This field is all 0 when calculating.
} Wii_IMET_t;
ASSERT_STRUCT(Wii_IMET_t, 1536);

// Wii languages. (Maps to IMET indexes.)
typedef enum {
	WII_LANG_JAPANESE	= 0,
	WII_LANG_ENGLISH	= 1,
	WII_LANG_GERMAN		= 2,
	WII_LANG_FRENCH		= 3,
	WII_LANG_SPANISH	= 4,
	WII_LANG_ITALIAN	= 5,
	WII_LANG_DUTCH		= 6,
	// 7 and 8 are unknown. (Chinese?)
	WII_LANG_KOREAN		= 9,
} Wii_Language_ID;

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_WII_BANNER_H__ */
