/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * cv_structs.h: ColecoVision ROM image data structures.                   *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ColecoVision ROM image file header
 * Reference: https://forums.atariage.com/topic/168314-coleco-cartridge-header-from-official-documentation/
 *
 * All fields are in little-endian.
 */
typedef struct _ColecoVision_ROMHeader {
	uint16_t magic;			// [0x000] Magic (see ColecoVision_Magic_e)
	uint16_t local_sprite_table;	// [0x002] Local sprite table address
	uint16_t sprite_order;		// [0x004] Sprite order table address
	uint16_t work_buffer;		// [0x006] Work buffer address
	uint8_t controller_map[2];	// [0x008] Controller map
	uint16_t entry_point;		// [0x00A] Entry point
	uint8_t rst_vectors[6][3];	// [0x00C] RST 8h - RST 30h RAM vectors
	uint8_t irq_int_vect[3];	// [0x01E] IRQ interrupt vector (JSR)
	uint8_t nmi_int_vect[3];	// [0x021] NMI interrupt vector (JSR)
	char game_name[96];		// [0x024] Game name (not fixed-length; assuming max of 96 chars...)
} ColecoVision_ROMHeader;
ASSERT_STRUCT(ColecoVision_ROMHeader, 0x24 + 96);

/**
 * ColecoVision magic number
 */
typedef enum {
	COLECOVISION_MAGIC_SHOW_LOGO	= 0x55AA,	// literal 0xAA 0x55 == show ColecoVision logo
	COLECOVISION_MAGIC_SKIP_LOGO	= 0xAA55,	// literal 0x55 0xAA == skip ColecoVision logo

	// BIOS and Monitor Test have different magic numbers.
	COLECOVISION_MAGIC_BIOS		= 0xB931,
	COLECOVISION_MAGIC_MONITOR_TEST	= 0xEDF3,
} ColecoVision_Magic_e;

#ifdef __cplusplus
}
#endif
