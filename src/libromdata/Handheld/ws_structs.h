/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ws_structs.h: Bandai WonderSwan (Color) data structures.                *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * WonderSwan ROM footer.
 * This matches the WonderSwan ROM footer format exactly.
 * Reference: http://daifukkat.su/docs/wsman/#cart_meta
 *
 * All fields are in little-endian.
 */
#pragma pack(1)
typedef struct RP_PACKED _WS_RomFooter {
	uint8_t zero;		// [-0x001] Must be zero
	uint8_t publisher;	//  [0x000] Publisher ID
	uint8_t system_id;	//  [0x001] System ID (see WS_System_ID_e)
	uint8_t game_id;	//  [0x002] Game ID
	uint8_t revision;	//  [0x003] Revision
	uint8_t rom_size;	//  [0x004] ROM size
	uint8_t save_type;	//  [0x005] Save size and type
	uint8_t flags;		//  [0x006] Flags (see WS_Flags_e)
	uint8_t rtc_present;	//  [0x007] RTC present? 0 == No, 1 == Yes
	uint16_t checksum;	//  [0x008] 16-bit sum of entire ROM except this word.
				//          This is set to zero for WonderWitch.
} WS_RomFooter;
#pragma pack()
ASSERT_STRUCT(WS_RomFooter, 11);

/**
 * WonderSwan system ID.
 */
typedef enum {
	WS_SYSTEM_ID_ORIGINAL	= 0,
	WS_SYSTEM_ID_COLOR	= 1,
} WS_System_ID_e;

/**
 * WonderSwan flags. (bitfield)
 */
typedef enum {
	WS_FLAG_DISPLAY_HORIZONTAL		= (0U << 0),
	WS_FLAG_DISPLAY_VERTICAL		= (1U << 0),
	WS_FLAG_DISPLAY_MASK			= (1U << 0),

	WS_FLAG_ROM_BUS_WIDTH_16_BIT		= (0U << 1),
	WS_FLAG_ROM_BUS_WIDTH_8_BIT		= (1U << 1),
	WS_FLAG_ROM_BUS_WIDTH_MASK		= (1U << 1),

	WS_FLAG_ROM_ACCESS_SPEED_3_CYCLE	= (0U << 2),
	WS_FLAG_ROM_ACCESS_SPEED_1_CYCLE	= (1U << 2),
	WS_FLAG_ROM_ACCESS_SPEED_MASK		= (1U << 2),
} WS_Flags_e;

#ifdef __cplusplus
}
#endif
