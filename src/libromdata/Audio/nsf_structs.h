/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * nsf_structs.h: NSF audio data structures.                               *
 *                                                                         *
 * Copyright (c) 2018-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - http://vgmrips.net/wiki/NSF_File_Format

#pragma once

#include "common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Nintendo Sound Format. (NES/Famicom)
 * All fields are little-endian.
 */
#define NSF_MAGIC "NESM\x1A\x01"
typedef struct _NSF_Header {
	char magic[6];			// [0x000] "NESM\x1A\x01"
	uint8_t track_count;		// [0x006] Number of tracks
	uint8_t default_track;		// [0x007] Default track number, plus one.
	uint16_t load_address;		// [0x008] Load address. (must be $8000-$FFFF)
	uint16_t init_address;		// [0x00A] Init address. (must be $8000-$FFFF)
	uint16_t play_address;		// [0x00C] Play address.
	char title[32];			// [0x00E] Title. (ASCII, NULL-terminated)
	char composer[32];		// [0x02E] Composer. (ASCII, NULL-terminated)
	char copyright[32];		// [0x04E] Copyright. (ASCII, NULL-terminated)
	uint16_t ntsc_framerate;	// [0x06E] NTSC framerate, in microseconds.
					//         (not always in use)
	uint8_t bankswitching[8];	// [0x070] If non-zero, initial bank setting for
					//         $8xxx, $9xxx, etc.
	uint16_t pal_framerate;		// [0x078] PAL framerate, in microseconds.
					//         (not always in use)
	uint8_t tv_system;		// [0x07A] TV system. (See NSF_TV_System_e.)
	uint8_t expansion_audio;	// [0x07B] Expansion audio. (See NSF_Expansion_e.)
	uint8_t reserved[4];		// [0x07C] Reserved. (must be 0)
} NSF_Header;
ASSERT_STRUCT(NSF_Header, 128);

/**
 * NSF: TV system.
 */
typedef enum {
	NSF_TV_NTSC	= 0,
	NSF_TV_PAL	= 1,
	NSF_TV_BOTH	= 2,

	NSF_TV_MAX
} NSF_TV_System_e;

/**
 * NSF: Expansion audio.
 * NOTE: This is a bitfield.
 */
typedef enum {
	NSF_EXP_VRC6		= (1U << 0),	// Konami VRC6
	NSF_EXP_VRC7		= (1U << 1),	// Konami VRC7
	NSF_EXP_2C33		= (1U << 2),	// 2C33 (Famicom Disk System)
	NSF_EXP_MMC5		= (1U << 3),	// MMC5
	NSF_EXP_N163		= (1U << 4),	// Namco N163
	NSF_EXP_SUNSOFT_5B	= (1U << 5),	// Sunsoft 5B
} NSF_Expansion_e;

#ifdef __cplusplus
}
#endif
