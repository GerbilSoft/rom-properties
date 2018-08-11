/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * nsf_structs.h: GBS audio data structures.                               *
 *                                                                         *
 * Copyright (c) 2018 by David Korth.                                      *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

// References:
// - http://ocremix.org/info/GBS_Format_Specification

#ifndef __ROMPROPERTIES_LIBROMDATA_AUDIO_GBS_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_AUDIO_GBS_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

/**
 * Game Boy Sound System.
 * All fields are little-endian.
 */
#define GBS_MAGIC "GBS\x01"
typedef struct PACKED _GBS_Header {
	char magic[4];			// [0x000] "GBS\x01"
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

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_AUDIO_GBS_STRUCTS_H__ */
