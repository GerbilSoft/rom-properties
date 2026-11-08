/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * nsf_structs.h: NSF audio data structures.                               *
 *                                                                         *
 * Copyright (c) 2018-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - https://vgmrips.net/wiki/NSF_File_Format
// - https://www.nesdev.org/wiki/NSF
// - https://www.nesdev.org/wiki/NSF2
// - https://www.nesdev.org/wiki/NSFe
// - https://github.com/bbbradsmith/nes-audio-tests

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
#define NSF_MAGIC "NESM\x1A"
typedef struct _NSF_Header {
	char magic[5];			// [0x000] "NESM\x1A"
	uint8_t version;		// [0x005] Version (1 or 2)
	uint8_t track_count;		// [0x006] Number of tracks
	uint8_t default_track;		// [0x007] Default track number, plus one.
	uint16_t load_address;		// [0x008] Load address (must be $8000-$FFFF)
	uint16_t init_address;		// [0x00A] Init address (must be $8000-$FFFF)
	uint16_t play_address;		// [0x00C] Play address (must be $8000-$FFFF)
	char title[32];			// [0x00E] Title (ASCII, NULL-terminated)
	char composer[32];		// [0x02E] Composer (ASCII, NULL-terminated)
	char copyright[32];		// [0x04E] Copyright (ASCII, NULL-terminated)
	uint16_t rate_ntsc;		// [0x06E] NTSC playback rate, in microseconds.
					//         (not always in use)
	uint8_t banks_init[8];		// [0x070] If non-zero, initial bank setting for
					//         $8xxx, $9xxx, etc.
	uint16_t rate_pal;		// [0x078] PAL playback rate, in microseconds.
					//         (not always in use)
	uint8_t tv_system;		// [0x07A] TV system (See NSF_TV_System_e)
	uint8_t expansion_audio;	// [0x07B] Expansion audio (See NSF_Expansion_e)

	// NSF2 fields
	uint8_t nsf2_feature_flags;	// [0x07C] NSF2 feature flags (See NSF2_Feature_Flags_e)
	uint8_t nsf2_prg_size[3];	// [0x07D] 24-bit program length; NSFe metadata follows the program if non-zero.
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
typedef enum ATTR_FLAG_ENUM {
	NSF_EXP_VRC6		= (1U << 0),	// Konami VRC6
	NSF_EXP_VRC7		= (1U << 1),	// Konami VRC7
	NSF_EXP_2C33		= (1U << 2),	// 2C33 (Famicom Disk System)
	NSF_EXP_MMC5		= (1U << 3),	// MMC5
	NSF_EXP_N163		= (1U << 4),	// Namco N163
	NSF_EXP_SUNSOFT_5B	= (1U << 5),	// Sunsoft 5B
} NSF_Expansion_e;

/**
 * NSF2: Feature flags.
 * NOTE: This is a bitfield.
 */
typedef enum ATTR_FLAG_ENUM {
	// NOTE: Bits 0-3 are currently reserved and must be 0.
	NSF2_FEATURE_FLAG_IRQ_SUPPORT		= (1U << 4),	// If set: may use IRQ support features
	NSF2_FEATURE_FLAG_NON_RETURNING_INIT	= (1U << 5),	// If set: non-returning INIT feature is used
	NSF2_FEATURE_FLAG_PLAY_SUB_NOT_USED	= (1U << 6),	// If set: PLAY subroutine is *not* used
	NSF2_FEATURE_FLAG_NSFe_MANDATORY_CHUNK	= (1U << 7),	// If set: NSFe metadata may contain a mandatory
								//         chunk required for playback.
} NSF2_Feature_Flags_e;

#ifdef __cplusplus
}
#endif
