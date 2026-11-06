/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * kss_structs.h: KSS audio data structures.                               *
 *                                                                         *
 * Copyright (c) 2018-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - https://www.vgmpf.com/Wiki/index.php?title=KSS
// - https://ocremix.org/info/KSS_Format_Specification

#pragma once

#include "common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Konami Sound System sound format.
 * All fields are in little-endian.
 */
#define KSS_MAGIC 'KSCC'
#define KSSX_MAGIC 'KSSX'
typedef struct _KSS_Header {
	uint32_t magic;			// [0x000] 'KSCC' or 'KSSX'
	uint16_t load_address;		// [0x004] Load address, in Z80 space
	uint16_t init_data_length;	// [0x006] Initial data length, in bytes
	uint16_t init_address;		// [0x008] Init address, in Z80 space
	uint16_t play_address;		// [0x00A] Play address, in Z80 space
	uint8_t start_no_of_bank;	// [0x00C] Start (offset) number of bank
	uint8_t banked_data_blocks;	// [0x00D] Number of banked data blocks
					//         Bit 7: If set, uses 8 KB mapper; otherwise, 16 KB mapper
	uint8_t reserved;		// [0x00E] Reserved for expansion (must be 0)
	uint8_t extra_sound_chip;	// [0x00F] Extra sound chips (see KSS_Extra_Sound_Chip_e)
} KSS_Header;
ASSERT_STRUCT(KSS_Header, 16);

/**
 * KSS: Extra sound chips
 */
typedef enum ATTR_FLAG_ENUM {
	KSS_SOUND_CHIP_USES_SN76489		= (1U << 1),	// If set, uses SN76489

	// Bits 0, 2, and 3 have different meanings depending on if bit 1 is set or not.

	// Bit 1 is set: SN76489 systems
	KSS_SOUND_CHIP_SN7_USES_FMUNIT		= (1U << 0),	// If set, uses FM UNIT
	KSS_SOUND_CHIP_SN7_USES_GG_STEREO	= (1U << 2),	// If set, uses Game Gear Stereo
	KSS_SOUND_CHIP_SN7_USES_RAM		= (1U << 3),	// If set, uses RAM

	// Bit 0 is set: No SN76489
	KSS_SOUND_CHIP_NOSN7_USES_FMPAC		= (1U << 0),	// If set, uses FM PAC
	KSS_SOUND_CHIP_NOSN7_USES_RAM		= (1U << 2),	// If set, uses RAM
	KSS_SOUND_CHIP_NOSN7_USES_MSX_AUDIO	= (1U << 3),	// If set, uses MSX-AUDIO
} KSS_Extra_Sound_Chip_e;

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
