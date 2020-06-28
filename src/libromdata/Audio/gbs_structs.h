/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * nsf_structs.h: GBS audio data structures.                               *
 *                                                                         *
 * Copyright (c) 2018-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - http://ocremix.org/info/GBS_Format_Specification

#ifndef __ROMPROPERTIES_LIBROMDATA_AUDIO_GBS_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_AUDIO_GBS_STRUCTS_H__

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
#define GBS_MAGIC 'GBS\x01'
typedef struct _GBS_Header {
	uint32_t magic;			// [0x000] 'GBS\x01' (big-endian)
					//         NOTE: \x01 is technically a version number.
	uint8_t track_count;		// [0x004] Number of tracks
	uint8_t default_track;		// [0x005] Default track number, plus one. (usually 1)
	uint16_t load_address;		// [0x006] Load address. (must be $0400-$7FFF)
	uint16_t init_address;		// [0x008] Init address. (must be $0400-$7FFF)
	uint16_t play_address;		// [0x00A] Play address. (must be $0400-$7FFF)
	uint16_t stack_pointer;		// [0x00C] Stack pointer.
	uint8_t timer_modulo;		// [0x00E] Timer modulo.
	uint8_t timer_control;		// [0x00F] Timer control.

	char title[32];			// [0x010] Title. (ASCII, NULL-terminated)
	char composer[32];		// [0x030] Composer. (ASCII, NULL-terminated)
	char copyright[32];		// [0x050] Copyright. (ASCII, NULL-terminated)
} GBS_Header;
ASSERT_STRUCT(GBS_Header, 112);

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_AUDIO_GBS_STRUCTS_H__ */
