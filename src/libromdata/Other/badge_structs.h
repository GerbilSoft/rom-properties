/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * badge_structs.h: Nintendo Badge Arcade data structures.                 *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

/**
 * References:
 * - https://github.com/GerbilSoft/rom-properties/issues/92
 * - https://github.com/CaitSith2/BadgeArcadeTool
 * - https://github.com/TheMachinumps/Advanced-badge-editor
 */

#include <stdint.h>
#include "common.h"
#include "nintendo_system_id.h"

#ifdef __cplusplus
extern "C" {
#endif

// Badge dimensions.
#define BADGE_SIZE_SMALL_W 32
#define BADGE_SIZE_SMALL_H 32
#define BADGE_SIZE_LARGE_W 64
#define BADGE_SIZE_LARGE_H 64

typedef char16_t badge_names_t[16][128];
ASSERT_STRUCT(badge_names_t, 4096);

/**
 * PRBS: Badge file.
 *
 * Contains an individual badge, or multiple badges
 * as part of a "mega badge".
 *
 * If width * height == 1: Image data starts at 0x1100.
 * Otherwise, image data starts at 0x4300.
 *
 * All fields are in little-endian,
 * except for the magic number.
 */
#define BADGE_PRBS_MAGIC 'PRBS'
typedef struct _Badge_PRBS_Header {
	uint32_t magic;		// [0x000] 'PRBS' (big-endian)
	uint8_t reserved1[56];	// [0x004]
	uint32_t badge_id;	// [0x03C] Badge ID
	uint8_t reserved2[4];	// [0x040]
	char filename[48];	// [0x044] Image filename (Latin-1?)
	char setname[48];	// [0x074] Set name (Latin-1?)
#pragma pack(1)
	struct PACKED {
		Nintendo_TitleID_LE_t title_id;	// [0x0A4] Title ID for program launch.
						// If no program is assigned, this is all 0xFF.
	};
#pragma pack()
	uint8_t reserved3[12];	// [0x0AC]
	uint32_t mb_width;	// [0x0B8] Mega-badge width
	uint32_t mb_height;	// [0x0BC] Mega-badge height
	uint8_t reserved4[32];	// [0x0C0]
	badge_names_t names;	// [0x0E0] Badge names (UTF-16LE)
} Badge_PRBS_Header;
ASSERT_STRUCT(Badge_PRBS_Header, 0x10E0);

/**
 * CABS: Badge set file.
 *
 * Contains an icon representing a set of badges.
 *
 * Image data starts at 0x2080;
 *
 * All fields are in little-endian,
 * except for the magic number.
 */
#define BADGE_CABS_MAGIC 'CABS'
typedef struct _Badge_CABS_Header {
	uint32_t magic;		// [0x000] 'CABS'
	uint8_t reserved1[32];	// [0x004]
	uint32_t set_id;	// [0x024] Set ID
	uint8_t reserved2[4];	// [0x028]
	char setname[48];	// [0x02C] Set name (Latin-1?)
	uint8_t reserved3[12];	// [0x05C]
	badge_names_t names;	// [0x068] Set names (UTF-16LE)
} Badge_CABS_Header;
ASSERT_STRUCT(Badge_CABS_Header, 0x1068);

#ifdef __cplusplus
}
#endif
