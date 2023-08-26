/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * spc_structs.h: SPC audio data structures.                               *
 *                                                                         *
 * Copyright (c) 2018-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - http://vspcplay.raphnet.net/spc_file_format.txt
// - https://ocremix.org/info/SPC_Format_Specification

#pragma once

#include "common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ID666 tag format.
 * All fields are little-endian.
 * Text is assumed to be ASCII.
 *
 * NOTE: There is no obvious way to distinguish between
 * binary and text formats. Heuristics can be used for
 * the Release Date field.
 *
 * NOTE: The ID666 tag is always located at 0x02E.
 * Both the relative and absolute addresses are listed.
 *
 * uint32_t fields are little-endian.
 */
// Some compilers pad this structure to a multiple of 4 bytes
#pragma pack(1)
typedef struct PACKED _SPC_ID666 {
	char song_title[32];	// [0x000, 0x02E] Song title.
	char game_title[32];	// [0x020, 0x04E] Game title.
	char dumper_name[16];	// [0x040, 0x06E] Name of dumper.
	char comments[32];	// [0x050, 0x07E] Comments.

	union PACKED {
		struct {
			char dump_date[11];		// [0x070, 0x09E] Date SPC was dumped. (MM/DD/YYYY)
			char seconds_before_fade[3];	// [0x07B, 0x0A9] Seconds to play before fading out (24-bit!)
			char fadeout_length_ms[5];	// [0x07E, 0x0AC] Length of fade-out, in milliseconds
			char artist[32];		// [0x083, 0x0B1] Artist
			uint8_t channel_disables;	// [0x0A3, 0x0D1] Default channel disables (0 = enable, 1 = disable)
			uint8_t emulator_used;		// [0x0A4, 0x0D2] Emulator used to dump the SPC (See SPC_Emulator_e.)
			uint8_t reserved[45];		// [0x0A5, 0x0D3]
		} text;
		struct {
			uint8_t dump_date[4];		// [0x070, 0x09E] Date SPC was dumped. (BCD: YY YY MM DD)
			uint8_t unused[7];		// [0x074, 0x0A2]
			uint8_t seconds_before_fade[3];	// [0x07B, 0x0A9] Seconds to play before fading out (24-bit!)
			uint32_t fadeout_length_ms;	// [0x07E, 0x0AC] Length of fade-out, in milliseconds
			char artist[32];		// [0x082, 0x0B0] Artist
			uint8_t channel_disables;	// [0x0A2, 0x0D0] Default channel disables (0 = enable, 1 = disable)
			uint8_t emulator_used;		// [0x0A3, 0x0D1] Emulator used to dump the SPC (See SPC_Emulator_e.)
			uint8_t reserved[46];		// [0x0A4, 0x0D2]
		} bin;
		struct {
			uint8_t skip[11];		// [0x070, 0x09E]
			uint8_t length_fields[7];	// [0x07B, 0x0A9] Common portion of the length fields.
		} test;
	};
} SPC_ID666;
ASSERT_STRUCT(SPC_ID666, 210);
#pragma pack()

/**
 * Emulator used to dump the SPC file.
 */
typedef enum {
	SPC_EMULATOR_UNKNOWN	= 0,
	SPC_EMULATOR_ZSNES	= 1,
	SPC_EMULATOR_SNES9X	= 2,
} SPC_Emulator_e;

/**
 * SPC File Format v0.30
 * All fields are little-endian.
 */
#define SPC_MAGIC "SNES-SPC700 Sound File Data v0.30"
#pragma pack(1)
typedef struct PACKED _SPC_Header {
	char magic[sizeof(SPC_MAGIC)-1];// [0x000] SPC_MAGIC
	uint8_t d26[2];			// [0x021] 26, 26 (TODO: Include as part of SPC_MAGIC?)
	uint8_t has_id666;		// [0x023] 26 = has ID666; 27 = no ID666
	uint8_t version;		// [0x024] Minor version number, i.e. 30.

	// Initial registers.
	struct {
		uint16_t PC;		// [0x025] PC
		uint8_t A;		// [0x027] A
		uint8_t X;		// [0x028] X
		uint8_t Y;		// [0x029] Y
		uint8_t PSW;		// [0x02A] PSW
		uint8_t SPl;		// [0x02B] SP (lower byte)
		uint8_t reserved[2];	// [0x02C]
	} regs;

	// ID666 tag.
	SPC_ID666 id666;		// [0x02E]
} SPC_Header;
ASSERT_STRUCT(SPC_Header, 256);
#pragma pack()

/**
 * Extended ID666: Item IDs.
 */
typedef enum {
	// ID666 fields.
	// NOTE: These are only present in Extended ID666
	// if the full value couldn't be represented properly
	// in regular ID666.
	SPC_xID6_ITEM_SONG_NAME		= 0x01,	// string: 4-256 chars
	SPC_xID6_ITEM_GAME_NAME		= 0x02,	// string: 4-256 chars
	SPC_xID6_ITEM_ARTIST_NAME	= 0x03,	// string: 4-256 chars
	SPC_xID6_ITEM_DUMPER_NAME	= 0x04,	// string: 4-256 chars
	SPC_xID6_ITEM_DUMP_DATE		= 0x05,	// integer: uint32_t, BCD: YY YY MM DD
	SPC_xID6_ITEM_EMULATOR_USED	= 0x06,	// length: uint8_t
	SPC_xID6_ITEM_COMMENTS		= 0x07,	// string: 4-256 chars

	// New fields.
	SPC_xID6_ITEM_OST_TITLE		= 0x10,	// string: 4-256 chars
	SPC_xID6_ITEM_OST_DISC		= 0x11,	// length: uint8_t
	SPC_xID6_ITEM_OST_TRACK		= 0x12,	// length: uint16_t: hi=0-99, lo=optional ascii char
	SPC_xID6_ITEM_PUBLISHER		= 0x13,	// string: 4-256 chars
	SPC_xID6_ITEM_COPYRIGHT_YEAR	= 0x14,	// length: uint16_t

	// Song length values are stored in ticks. (uint32_t)
	// One tick = 1/64000 of a second.
	// Maximum length is 383,999,999 ticks.
	// The "End" length can contain a negative value.
	SPC_xID6_ITEM_INTRO_LENGTH	= 0x30,	// integer: ticks
	SPC_xID6_ITEM_LOOP_LENGTH	= 0x31,	// integer: ticks
	SPC_xID6_ITEM_END_LENGTH	= 0x32,	// integer: ticks
	SPC_xID6_ITEM_FADE_LENGTH	= 0x33,	// integer: ticks

	SPC_xID6_ITEM_MUTED_CHANNELS	= 0x34,	// length: uint8_t: one bit is set for each channel that's muted
	SPC_xID6_ITEM_LOOP_COUNT	= 0x35,	// length: uint8_t: number of times to loop the looped section
	SPC_xID6_ITEM_AMP_VALUE		= 0x36,	// integer: uint32_t: Amplification value (Normal SNES == 65536)
} SPC_xID6_Item_e;

/**
 * Extended ID666: Header.
 * Located at 0x10200.
 *
 * All fields are in little-endian.
 */
#define SPC_xID6_MAGIC 'xid6'
#define SPC_xID6_ADDRESS 0x10200
typedef struct _SPC_xID6_Header {
	uint32_t magic;		// [0x000] 'xid6'
	uint32_t size;		// [0x004] Size, not including the header.
} SPC_xID6_Header;
ASSERT_STRUCT(SPC_xID6_Header, 8);

#ifdef __cplusplus
}
#endif
