/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * badge_structs.h: Nintendo Badge Arcade data structures.                 *
 *                                                                         *
 * Copyright (c) 2017 by David Korth.                                      *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_BADGE_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_BADGE_STRUCTS_H__

/**
 * References:
 * - https://github.com/GerbilSoft/rom-properties/issues/92
 * - https://github.com/CaitSith2/BadgeArcadeTool
 * - https://github.com/TheMachinumps/Advanced-badge-editor
 */

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

// Badge dimensions.
#define BADGE_SIZE_SMALL_W 32
#define BADGE_SIZE_SMALL_H 32
#define BADGE_SIZE_LARGE_W 64
#define BADGE_SIZE_LARGE_H 64

/**
 * PRBS: Badge file.
 *
 * Contains an individual badge, or multiple badges
 * as part of a "mega badge".
 *
 * If width * height == 1: Image data starts at 0x1100.
 * Otherwise, image data starts at 0x4300.
 *
 * All fields are in little-endian.
 */
#define BADGE_PRBS_MAGIC "PRBS"
typedef struct PACKED _Badge_PRBS_Header {
	char magic[4];		// [0x000] "PRBS"
	uint8_t reserved1[56];	// [0x004] Unknown
	uint32_t badge_id;	// [0x03C] Badge ID
	uint8_t reserved2[4];	// [0x040] Unknown
	char filename[48];	// [0x044] Image filename. (Latin-1?)
	char setname[48];	// [0x074] Set name. (Latin-1?)
	union {
		// [0x0A4] Title ID for program launch.
		// If no program is assigned, this is all 0xFF.
		uint64_t id;		// [0x0A4]
		struct {
			uint32_t lo;	// [0x0A4]
			uint32_t hi;	// [0x0A8]
		};
	} title_id;
	uint8_t reserved3[12];	// [0x0AC] Unknown
	uint32_t mb_width;	// [0x0B8] Mega-badge width.
	uint32_t mb_height;	// [0x0BC] Mega-badge height.
	uint8_t reserved4[32];	// [0x0C0] Unknown
	char16_t name[16][128];	// [0x0E0] Badge names. (UTF-16LE)
} Badge_PRBS_Header;
ASSERT_STRUCT(Badge_PRBS_Header, 0x10E0);

/**
 * CABS: Badge set file.
 *
 * Contains an icon representing a set of badges.
 *
 * Image data starts at 0x2080;
 *
 * All fields are in little-endian.
 */
#define BADGE_CABS_MAGIC "CABS"
typedef struct PACKED _Badge_CABS_Header {
	char magic[4];		// [0x000] "CABS"
	uint8_t reserved1[32];	// [0x004] Unknown
	uint32_t set_id;	// [0x024] Set ID.
	uint8_t reserved2[4];	// [0x028] Unknown
	char setname[48];	// [0x02C] Set name. (Latin-1?)
	uint8_t reserved3[12];	// [0x05C] Unknown
	char16_t name[16][128];	// [0x068] Set names. (UTF-16LE)
} Badge_CABS_Header;
ASSERT_STRUCT(Badge_CABS_Header, 0x1068);

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_PVR_STRUCTS_H__ */
