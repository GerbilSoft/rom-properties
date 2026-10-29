/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * flipnote_structs.h: Flipnote Studio data structures.                    *
 *                                                                         *
 * Copyright (c) 2026 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

// References:
// - https://www.dsibrew.org/wiki/Flipnote_Files/PPM

/**
 * Flipnote Studio (DSi) .ppm header.
 *
 * All fields are in little-endian. (except for magic)
 */
#define FLIPNOTE_PPM_MAGIC 'PARA'
#define FLIPNOTE_PPM_WIDTH 64
#define FLIPNOTE_PPM_HEIGHT 48
#pragma pack(2)
typedef struct RP_PACKED _FlipnoteStudio_PPM_Header_t {
	uint32_t magic;				// [0x000] Magic ('PARA')
	uint32_t anim_data_size;		// [0x004] Animation data size
	uint32_t audio_data_size;		// [0x008] Audio data size
	uint16_t num_frames;			// [0x00C] Number of frames
	uint16_t unknown_00E;			// [0x00E] Unknown; usually 0x0024
	uint16_t locked;			// [0x010] 0 = open; 1 = locked
	uint16_t preview_frame_num;		// [0x012] Preview frame number
	char16_t orig_author_name[11];		// [0x014] Original author name (UCS-2)
	char16_t last_edited_author_name[11];	// [0x02A] Last Edited By author name (UCS-2)
	char16_t user_name[11];			// [0x040] User name (UCS-2)
	uint64_t orig_author_id;		// [0x056] Original author ID
	uint64_t last_edited_author_id;		// [0x05E] Last Edited By author ID
	uint8_t orig_file_name[18];		// [0x066] Original file name (has a weird format)
	uint8_t file_name[18];			// [0x078] File name (has a weird format)
	uint64_t prev_editing_author_id;	// [0x08A] Previous Editing author ID
	uint8_t partial_file_name[8];		// [0x092] Partial file name? (has a weird format)
	uint32_t timestamp;			// [0x09A] Timestamp (GameCube format)
	uint16_t unknown_09E;			// [0x09E] Filler (usually 0x0000)
	uint8_t preview_bitmap[1536];		// [0x0A0] Preview bitmap (NDS tiled 4bpp, 64x48)
} FlipnoteStudio_PPM_Header_t;
ASSERT_STRUCT(FlipnoteStudio_PPM_Header_t, 1696);
#pragma pack()

#ifdef __cplusplus
}
#endif
