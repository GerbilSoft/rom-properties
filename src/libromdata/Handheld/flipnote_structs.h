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
// - https://github.com/jaames/flipnote.js/blob/master/src/parsers/KwzParser.ts

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

/**
 * Flipnote Studio 3D (3DS) chunk header.
 * All chunks start with this header.
 *
 * All fields are in little-endian. (except for magic)
 * NOTE: Low byte of magic is not actually part of the magic number...
 */
typedef struct _FlipnoteStudio3D_Chunk_Header_t {
	uint32_t magic;		// [0x000] Magic (BE32) (high 3 bytes; low byte is something else...)
	uint32_t size;		// [0x004] Size of the chunk, minus 8 bytes.
	uint32_t crc32;		// [0x008] CRC32 of the chunk.
} FlipnoteStudio3D_Chunk_Header_t;
ASSERT_STRUCT(FlipnoteStudio3D_Chunk_Header_t, 3*sizeof(uint32_t));

/**
 * Flipnote Studio 3D (3DS) .kwz header.
 *
 * All fields are in little-endian. (except for magic)
 */
#define FLIPNOTE3D_KFH_MAGIC		0x4B464800U	// 'KFH\0'
#define FLIPNOTE3D_KFH_MAGIC_MASK	0xFFFFFF00U
typedef struct _FlipnoteStudio3D_KFH_t {
	FlipnoteStudio3D_Chunk_Header_t header;	// [0x000] Chunk header (magic: 'KFHx')
	uint32_t creation_time;			// [0x00C] Creation time (GameCube format)
	uint32_t modified_time;			// [0x010] Modified time (GameCube format)
	uint32_t app_version;			// [0x014]
	uint8_t root_author_id[10];		// [0x018]
	uint8_t parent_author_id[10];		// [0x022]
	uint8_t current_author_id[10];		// [0x02C]
	char16_t root_author_name[11];		// [0x036] Root author name (UCS-2)
	char16_t parent_author_name[11];	// [0x04C] Parent author name (UCS-2)
	char16_t current_author_name[11];	// [0x062] Current author name (UCS-2)
	char root_filename[28];			// [0x078] Root filename (uses an unusual encoding?)
	char parent_filename[28];		// [0x094] Root filename (uses an unusual encoding?)
	char current_filename[28];		// [0x0B0] Root filename (uses an unusual encoding?)
	uint16_t frame_count;			// [0x0CC] Frame count
	uint16_t thumb_index;			// [0x0CE] Thumbnail frame index
	uint16_t flags;				// [0x0D0] Flags (TODO)
	uint8_t frame_speed;			// [0x0D2] Frame speed (TODO)
	uint8_t layer_flags;			// [0x0D3] Layer flags (TODO)
} FlipnoteStudio3D_KFH_t;
ASSERT_STRUCT(FlipnoteStudio3D_KFH_t, 212);

/**
 * Flipnote Studio 3D (3DS): Thumbnail preview chunk. (KTN)
 * NOTE: The chunk header is immediately followed by a JPEG image.
 *
 * All fields are in little-endian. (except for magic)
 */
#define FLIPNOTE3D_KTN_MAGIC		0x4B544E00U	// 'KTN\0'
#define FLIPNOTE3D_KTN_MAGIC_MASK	0xFFFFFF00U
#define FLIPNOTE3D_KTN_WIDTH 80
#define FLIPNOTE3D_KTN_HEIGHT 64

#ifdef __cplusplus
}
#endif
