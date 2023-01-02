/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * st_structs.h: Sufami Turbo data structures.                             *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Sufami Turbo ROM header.
 * This matches the ROM header format exactly.
 * Located at 0x0000.
 *
 * References:
 * - https://problemkaputt.de/fullsnes.htm#snescartsufamiturboromramheaders
 *
 * All fields are in little-endian.
 *
 * NOTE: Strings are NOT null-terminated!
 */
#define ST_HEADER_ADDRESS 0x0000
#define ST_MAGIC	"BANDAI SFC-ADX"
#define ST_BIOS_TITLE	"SFC-ADX BACKUP"
typedef struct _ST_RomHeader {
	char magic[14];		// [0x000] "BANDAI SFC-ADX"
	char padding1[2];	// [0x00E] Zero-filled
	char title[14];		// [0x010] Title: Can be ASCII and/or 8-bit JIS
	char padding2[2];	// [0x01E] Zero-filled

	// All vectors use bank 0x20.
	uint16_t entry_point;	// [0x020] Entry point (Slot A only)
	uint16_t nmi_vector;	// [0x022] NMI vector (if BIOS NMI handler is disabled)
	uint16_t irq_vector;	// [0x024] IRQ vector
	uint16_t cop_vector;	// [0x026] COP vector
	uint16_t brk_vector;	// [0x028] BRK vector
	uint16_t abt_vector;	// [0x02A] ABT vector
	uint8_t padding3[4];	// [0x02C] Zero-filled

	uint8_t game_id[3];	// [0x030] Unique 24-bit ID (usually 0X 00 0Y)
	uint8_t series_index;	// [0x033] If non-zero, index within a series, e.g. SD Gundam.
	uint8_t rom_speed;	// [0x034] ROM speed. (See ST_RomSpeed_e.)
	uint8_t features;	// [0x035] Features. (See ST_Features_e.)
	uint8_t rom_size;	// [0x036] ROM size, in 128 KB units
	uint8_t sram_size;	// [0x037] SRAM size, in 2 KB units
	uint8_t padding4[8];	// [0x038] Zero-filled
} ST_RomHeader;
ASSERT_STRUCT(ST_RomHeader, 0x40);

/**
 * Sufami Turbo: ROM speed
 */
typedef enum {
	ST_RomSpeed_SlowROM	= 0,
	ST_RomSpeed_FastROM	= 1,
} ST_RomSpeed_e;

/**
 * Sufami Turbo: Features
 */
typedef enum {
	ST_Feature_Simple	= 0,
	ST_Feature_SRAM		= 1,
	ST_Feature_Special	= 3,
} ST_Features_e;

#pragma pack()

#ifdef __cplusplus
}
#endif
