/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * nsf_structs.h: GBS audio data structures.                               *
 *                                                                         *
 * Copyright (c) 2018-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - http://ocremix.org/info/GBS_Format_Specification

#pragma once

#include "common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Game Boy Sound System.
 *
 * All fields are little-endian,
 * except for the magic number.
 */
#define GBS_MAGIC 0x47425301U	// 'GBS\x01'
typedef struct _GBS_Header {
	uint32_t magic;			// [0x000] 'GBS\x01' (big-endian)
					//         NOTE: \x01 is technically a version number.
	uint8_t track_count;		// [0x004] Number of tracks
	uint8_t default_track;		// [0x005] Default track number, plus one (usually 1)
	uint16_t load_address;		// [0x006] Load address (must be $0400-$7FFF)

	uint16_t init_address;		// [0x008] Init address (must be $0400-$7FFF)
	uint16_t play_address;		// [0x00A] Play address (must be $0400-$7FFF)
	uint16_t stack_pointer;		// [0x00C] Stack pointer
	uint8_t timer_modulo;		// [0x00E] Timer modulo (TMA)
	uint8_t timer_control;		// [0x00F] Timer control (TMC)

	char title[32];			// [0x010] Title (ASCII, NULL-terminated)
	char composer[32];		// [0x030] Composer (ASCII, NULL-terminated)
	char copyright[32];		// [0x050] Copyright (ASCII, NULL-terminated)
} GBS_Header;
ASSERT_STRUCT(GBS_Header, 112);

/**
 * Game Boy Ripped.
 * Predecessor to GBS format.
 * Reference: http://nezplug.sourceforge.net/
 *
 * All fields are little-endian,
 * except for the magic number.
 */
#define GBR_MAGIC 0x47425246U	// 'GBRF'
typedef struct _GBR_Header {
	uint32_t magic;			// [0x000] 'GBRF' (big-endian)

	uint8_t bankromnum;		// [0x004]
	uint8_t bankromfirst_0;		// [0x005]
	uint8_t bankromfirst_1;		// [0x006]
	uint8_t timer_flag;		// [0x007] Timer interrupt flags (part of TMC in GBS)

	uint16_t init_address;		// [0x008] Init address (must be $0400-$7FFF)
	uint16_t vsync_address;		// [0x00A] VSync address
	uint16_t timer_address;		// [0x00C] Timer address
	uint8_t timer_modulo;		// [0x00E] Timer modulo (TMA)
	uint8_t timer_control;		// [0x00F] Timer control (TMC)
} GBR_Header;
ASSERT_STRUCT(GBR_Header, 16);

#ifdef __cplusplus
}
#endif
