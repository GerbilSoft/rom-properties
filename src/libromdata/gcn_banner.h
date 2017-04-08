/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * gcn_banner.h: Nintendo GameCube and Wii banner structures.              *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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

/**
 * Reference:
 * - http://hitmen.c02.at/files/yagcd/yagcd/chap14.html
 */

#ifndef __ROMPROPERTIES_LIBROMDATA_GCN_BANNER_H__
#define __ROMPROPERTIES_LIBROMDATA_GCN_BANNER_H__

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

// Magic numbers.
#define BANNER_MAGIC_BNR1 0x424E5231	/* 'BNR1' */
#define BANNER_MAGIC_BNR2 0x424E5232	/* 'BNR2' */

// Banner size.
#define BANNER_IMAGE_W 96
#define BANNER_IMAGE_H 32

// NOTE: Strings are encoded in either cp1252 or Shift-JIS,
// depending on the game region.

// Banner comment.
#pragma pack(1)
#define GCN_BANNER_COMMENT_SIZE 0x140
#pragma pack(1)
typedef struct PACKED _banner_comment_t
{
	char gamename[0x20];
	char company[0x20];
	char gamename_full[0x40];
	char company_full[0x40];
	char gamedesc[0x80];
} banner_comment_t;
#pragma pack()
ASSERT_STRUCT(banner_comment_t, GCN_BANNER_COMMENT_SIZE);

// BNR1
#pragma pack(1)
#define GCN_BANNER_BNR1_SIZE (0x1820 + GCN_BANNER_COMMENT_SIZE)
typedef struct PACKED _banner_bnr1_t
{
	uint32_t magic;			// BANNER_MAGIC_BNR1
	uint8_t reserved[0x1C];
	uint16_t banner[0x1800>>1];	// Banner image. (96x32, RGB5A3)
	banner_comment_t comment;
} banner_bnr1_t;
#pragma pack()
ASSERT_STRUCT(banner_bnr1_t, GCN_BANNER_BNR1_SIZE);

// BNR2
#pragma pack(1)
#define GCN_BANNER_BNR2_SIZE (0x1820 + (GCN_BANNER_COMMENT_SIZE * 6))
typedef struct PACKED _banner_bnr2_t
{
	uint32_t magic;			// BANNER_MAGIC_BNR2
	uint8_t reserved[0x1C];
	uint16_t banner[0x1800>>1];	// Banner image. (96x32, RGB5A3)
	banner_comment_t comments[6];
} banner_bnr2_t;
#pragma pack()
ASSERT_STRUCT(banner_bnr2_t, GCN_BANNER_BNR2_SIZE);

// BNR2 languages. (Maps to GameCube language setting.)
typedef enum {
	GCN_PAL_LANG_ENGLISH	= 0,
	GCN_PAL_LANG_GERMAN	= 1,
	GCN_PAL_LANG_FRENCH	= 2,
	GCN_PAL_LANG_SPANISH	= 3,
	GCN_PAL_LANG_ITALIAN	= 4,
	GCN_PAL_LANG_DUTCH	= 5,
} GCN_PAL_Language;

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
#define BANNER_WIBN_IMAGE_SIZE 24576
#define BANNER_WIBN_ICON_SIZE 0x1200
#define BANNER_WIBN_STRUCT_SIZE 24736
#define BANNER_WIBN_STRUCT_SIZE_ICONS(icons) \
	(BANNER_WIBN_STRUCT_SIZE + ((icons)*BANNER_WIBN_ICON_SIZE))

#pragma pack(1)
typedef struct PACKED _wii_savegame_header_t {
	uint32_t magic;			// BANNER_MAGIC_WIBN
	uint32_t flags;
	uint16_t iconDelay;		// Similar to GCN.
	uint8_t reserved[22];
	uint16_t gameTitle[32];		// Game title. (UTF-16 BE)
	uint16_t gameSubTitle[32];	// Game subtitle. (UTF-16 BE)
} wii_savegame_header_t;
#pragma pack()
ASSERT_STRUCT(wii_savegame_header_t, 160);

#pragma pack(1)
typedef struct PACKED _wii_savegame_banner_t {
	uint16_t banner[BANNER_WIBN_IMAGE_W*BANNER_WIBN_IMAGE_H];	// Banner image. (192x64, RGB5A3)
	uint16_t icon[BANNER_WIBN_ICON_W*BANNER_WIBN_ICON_H];		// Main icon. (48x48, RGB5A3)
} wii_savegame_banner_t;
#pragma pack()
ASSERT_STRUCT(wii_savegame_banner_t, BANNER_WIBN_IMAGE_SIZE+BANNER_WIBN_ICON_SIZE);

#pragma pack(1)
typedef struct PACKED _wii_savegame_icon_t {
	uint16_t icon[BANNER_WIBN_ICON_W*BANNER_WIBN_ICON_H];	// Additional icon. (48x48, RGB5A3) [optional]
} wii_savegame_icon_t;
#pragma pack()
ASSERT_STRUCT(_wii_savegame_icon_t, BANNER_WIBN_ICON_SIZE);

// IMET magic number.
#define WII_IMET_MAGIC 0x494D4554	/* 'IMET' */

/**
 * IMET (Wii opening.bnr header)
 * This contains the game title.
 * Reference: http://wiibrew.org/wiki/Opening.bnr#banner.bin_and_icon.bin
 *
 * All fields are big-endian.
 */
#pragma pack(1)
typedef struct _wii_imet_t {
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
} wii_imet_t;
#pragma pack()
ASSERT_STRUCT(wii_imet_t, 1536);

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
} Wii_Language;

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_GCN_BANNER_H__ */
