/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * intv_structs.h: Intellivision ROM image data structures.                *
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
 * Intellivision: BIDECLE type
 * This type stores a low byte and high byte from a 16-bit word
 * in two consecutive 10-bit words.
 *
 * All fields are in 16-bit big-endian, but the byte ordering
 * within the fields is (low, high).
 */
typedef union _Intv_BIDECLE {
	struct {
		uint8_t unused_lo;
		uint8_t lo;
		uint8_t unused_hi;
		uint8_t hi;
	};
	uint16_t u16[2];

#ifdef __cplusplus
	uint16_t get_real_value(void) const {
		return (hi << 8) | lo;
	}
#endif /* __cplusplus */
} Intv_BIDECLE;
ASSERT_STRUCT(Intv_BIDECLE, 2*sizeof(uint16_t));

/**
 * Intellivision ROM image file header.
 * Reference: https://wiki.intellivision.us/index.php/Hello_World_Tutorial
 *
 * All fields are in 16-bit big-endian.
 *
 * NOTE: Intellivision used 10-bit ROMs. ROM images use
 * 16-bit words for convenience, plus homebrew games
 * sometimes use 16-bit ROMs.
 */
typedef union _Intellivision_ROMHeader {
	struct {
		// Pointers (NOTE: Addresses are in 16-bit word units.)
		Intv_BIDECLE mob_picture_base;		// [0x000] MOB picture base
		Intv_BIDECLE process_table;		// [0x004] Process table
		Intv_BIDECLE program_start_address;	// [0x008] Entry point (only used if EXEC is in use)
		Intv_BIDECLE bkgnd_picture_base;	// [0x00C] Background picture base
		Intv_BIDECLE gram_pictures;		// [0x010] GRAM pictures
		Intv_BIDECLE title_date;		// [0x014] Title and date (date is year minus 1900)

		uint16_t flags;			// [0x018] Flags (see Intellivision_Flags_e)
		uint16_t screen_border_ctrl;	// [0x01A] Screen border control
		uint16_t color_stack_mode;	// [0x01C] Color stack and framebuffer mode

		uint16_t color_stack[4];	// [0x01E] Initial color stack
		uint16_t border_color;		// [0x026] Initial border color
	};
	uint16_t u16[256];			// Direct access for e.g. title/date
} Intellivision_ROMHeader;
ASSERT_STRUCT(Intellivision_ROMHeader, 512);

/**
 * Intelliviison flags
 */
typedef enum {
	INTV_SKIP_ECS			= (1U << 9) | (1U << 8),	// Skip ECS title screen (both bits must be set)
	INTV_RUN_CODE_AFTER_TITLE	= (1U << 7),			// Run code that appears after the title string
	INTV_SUPPORT_INTV2		= (1U << 6),			// Must be set to allow use on Intellivision 2
	INTV_KEYCLICK_MASK		= 0x001F,			// Keyclick mask (requires EXEC)
} Intellivision_Flags_e;

#ifdef __cplusplus
}
#endif
