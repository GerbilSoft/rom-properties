/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * gcom_structs.h: Tiger game.com data structures.                         *
 *                                                                         *
 * Copyright (c) 2018-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

// Icon information.
// NOTE: Icons are 2bpp.
#define GCOM_ICON_BANK_W 256
#define GCOM_ICON_BANK_H 256
#define GCOM_ICON_BANK_SIZE ((GCOM_ICON_BANK_W * GCOM_ICON_BANK_H) / 4)
#define GCOM_ICON_W 64
#define GCOM_ICON_H 64

// RLE-compressed icons have a different bank size.
#define GCOM_ICON_BANK_SIZE_RLE 0x2000
#define GCOM_ICON_RLE_BANK_LOAD_OFFSET 0x6000

// NOTE: The official game.com emulator requires the header to be at 0x40000.
// Some ROMs have the header at 0, though.
#define GCOM_HEADER_ADDRESS 0x40000
#define GCOM_HEADER_ADDRESS_ALT 0

/**
 * Tiger game.com ROM header.
 *
 * All fields are in little-endian.
 * NOTE: Icon is rotated.
 *
 * NOTE: Strings are NOT null-terminated!
 */
#define GCOM_SYS_ID "TigerDMGC"
typedef struct _Gcom_RomHeader {
	uint8_t rom_size;		// [0x000] ROM size?
	uint8_t entry_point_bank;	// [0x001] Entry point: Bank number.
	uint16_t entry_point;		// [0x002] Entry point
	uint8_t flags;			// [0x004] Flags (See Gcom_Flags_e)
	char sys_id[9];			// [0x005] System identifier

	// Some compilers pad this structure to a multiple of 4 bytes
#pragma pack(1)
	struct RP_PACKED {
		/**
		 * game.com ROM images are divided into 16 KB banks,
		 * each of which makes up a 2bpp 256x256 bitmap.
		 * The game's icon is specified by selecting a bank
		 * number and the icon's (x,y) coordinates.
		 *
		 * NOTE: Bitmaps are rotated 270 degrees and vertically flipped.
		 *
		 * NOTE 2: If RLE compression is enabled, the start address of
		 * the RLE-compressed data is calculated differently:
		 * (bank * 0x2000) | ((x << 8) | y)
		 */

		uint8_t bank;	// [0x00E] Bank number. (16 KB; 256x256)
		uint8_t x;	// [0x00F] X coordinate within the bank.
		uint8_t y;	// [0x010] Y coordinate within the bank.
	} icon;
#pragma pack()

	char title[9];			// [0x011] Game title.
	uint16_t game_id;		// [0x01A] Game ID.
	uint8_t security_code;		// [0x01C] Security code.
	uint8_t padding[3];		// [0x01D] Padding.
} Gcom_RomHeader;
ASSERT_STRUCT(Gcom_RomHeader, 32);

/**
 * game.com: Flags
 */
typedef enum {
	GCOM_FLAG_HAS_ICON	= (1U << 1),	// Icon is present
	GCOM_FLAG_ICON_RLE	= (1U << 3),	// Icon is RLE-compressed
} Gcom_Flags_e;

#ifdef __cplusplus
}
#endif
