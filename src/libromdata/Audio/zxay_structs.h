/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * zxay_structs.h: ZX Spectrum AY audio data structures.                   *
 *                                                                         *
 * Copyright (c) 2018-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference:
// - https://web.archive.org/web/20160304140808/http://projectay.dl.spilsby.net/AY_Spec.txt

#pragma once

#include "common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// NOTE: The ZX Spectrum uses a Z80 CPU, which is little-endian.
// The ZXAY format, on the other hand, was designed for the Amiga,
// which uses an MC68000 CPU, which is big-endian.

/**
 * ZX Spectrum AY sound format.
 *
 * All fields are in big-endian.
 * All pointers are relative to the current offset.
 * All strings are C strings. (NULL-terminated)
 *
 * Following the header is a list of song structures.
 * See ZXAY_Song_Struct.
 */
#define ZXAY_MAGIC 'ZXAY'
#define ZXAY_TYPE_EMUL 'EMUL'
#define ZXAY_TYPE_AMAD 'AMAD'
#pragma pack(2)
typedef struct RP_PACKED _ZXAY_Header {
	uint32_t magic;			// [0x000] 'ZXAY'
	uint32_t type;			// [0x004] 'EMUL' or 'AMAD'
	uint8_t file_version;		// [0x008] File version
	uint8_t player_version;		// [0x009] Required player version (see ZXAY_Player_Version_e)
	uint16_t p_special_player;	// [0x00A] Pointer to MC68000 code for playback
	uint16_t p_author;		// [0x00C] Pointer to the author's name
	uint16_t p_misc;		// [0x00E] Pointer to description string
	uint8_t song_count;		// [0x010] Song count, minus 1
	uint8_t first_song;		// [0x011] First song (0-index)
	uint16_t p_first_song_struct;	// [0x012] Pointer to first song struct
} ZXAY_Header;
ASSERT_STRUCT(ZXAY_Header, 20);
#pragma pack()

/**
 * ZXAY: Player versions
 */
typedef enum {
	ZXAY_PLAYER_VERSION_0	= 0,	// Any version works
	ZXAY_PLAYER_VERSION_1	= 1,	// Initial player version
	ZXAY_PLAYER_VERSION_2	= 2,	// First 256 bytes of Z80 memory is
					// initialized with 0xC9 (RET).
	ZXAY_PLAYER_VERSION_3	= 3,	// Last version, with more complex
					// initialization.
} ZXAY_Player_Version_e;

/**
 * ZXAY: Song struct.
 *
 * All fields are in big-endian.
 * All pointers are relative to the current offset.
 * All strings are C strings. (NULL-terminated)
 */
typedef struct RP_PACKED _ZXAY_Song_Struct {
	uint16_t p_song_name;		// [0x000] Pointer to song name
	uint16_t p_song_data;		// [0x002] Pointer to song data
} ZXAY_Song_Struct;
ASSERT_STRUCT(ZXAY_Song_Struct, 4);

#ifdef __cplusplus
}
#endif
