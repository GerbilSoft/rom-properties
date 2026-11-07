/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * nsfe_structs.h: NSFe audio data structures.                             *
 *                                                                         *
 * Copyright (c) 2018-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference:
// - https://www.nesdev.org/wiki/NSFe

#pragma once

#include "common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Extended Nintendo Sound Format. (NES/Famicom)
 *
 * NOTE: Chunks may appear in different orders. The only requirement
 * is that 'INFO' appears before 'DATA', and the file ends with 'NEND'.
 *
 * All fields are in little-endian, except for the
 * magic number, which is considered "big-endian".
 */
#define NSFe_MAGIC 'NSFE'
typedef struct _NSFe_Header {
	uint32_t magic;		// [0x000] 'NSFe'
} NSFe_Header;
ASSERT_STRUCT(NSFe_Header, sizeof(uint32_t));

/**
 * NSFe: Chunk header.
 *
 * All fields are in little-endian, except for the
 * fourCC, which is considered "big-endian".
 */
typedef struct _NSFe_Chunk_Header {
	uint32_t size;		// [0x000] Size of chunk, not including this header.
	uint32_t fourCC;	// [0x004] FourCC (see NSFe_Chunk_FourCC_e)
} NSFe_Chunk_Header;
ASSERT_STRUCT(NSFe_Chunk_Header, 2 * sizeof(uint32_t));

// Known chunk FourCCs
typedef enum {
	// Mandatory chunks
	NSFe_Chunk_INFO		= 'INFO',
	NSFe_Chunk_DATA		= 'DATA',
	NSFe_Chunk_NEND		= 'NEND',

	// Other chunks
	NSFe_Chunk_BANK		= 'BANK',
	NSFe_Chunk_RATE		= 'RATE',
	NSFe_Chunk_NSF2		= 'NSF2',
	NSFe_Chunk_VRC7		= 'VRC7',

	// Optional chunks
	NSFe_Chunk_plst		= 'plst',
	NSFe_Chunk_psfx		= 'psfx',
	NSFe_Chunk_time		= 'time',
	NSFe_Chunk_fade		= 'fade',
	NSFe_Chunk_tlbl		= 'tlbl',
	NSFe_Chunk_taut		= 'taut',
	NSFe_Chunk_auth		= 'auth',
	NSFe_Chunk_text		= 'text',
	NSFe_Chunk_mixe		= 'mixe',
	NSFe_Chunk_regn		= 'regn',
} NSFe_Chunk_FourCC_e;

// NOTE: The following chunks do NOT include NSFe_Chunk_Header.

/**
 * NSFe: INFO chunk
 * All fields are in little-endian.
 */
typedef struct _NSFe_INFO_t {
	uint16_t load_address;		// [0x000] Load address (must be $8000-$FFFF)
	uint16_t init_address;		// [0x002] Init address (must be $8000-$FFFF)
	uint16_t play_address;		// [0x004] Play address (must be $8000-$FFFF)
	uint8_t pal_ntsc_bits;		// [0x006] PAL/NTSC bits (see NSFe_INFO_PAL_NTSC_e)
	uint8_t extra_sound_chips;	// [0x007] Extra sound chips (see NSFe_INFO_Extra_Sound_Chips_e)
	uint8_t song_count;		// [0x008] Song count
	uint8_t starting_song;		// [0x009] Starting song (0-based)
} NSFe_INFO_t;
ASSERT_STRUCT(NSFe_INFO_t, 10);

/**
 * NSFe: INFO: PAL/NTSC bits
 */
typedef enum ATTR_FLAG_ENUM {
	NSFe_INFO_IS_NTSC	= (0U << 0),	// Bit 0: if clear, NTSC
	NSFe_INFO_IS_PAL	= (1U << 0),	// Bit 0: if set, PAL
	NSFe_INFO_TV_TYPE_MASK	= (1U << 0),	// Bit 0: TV type mask

	NSFe_INFO_DUAL_NTSC_PAL	= (1U << 1),	// Bit 1: if set, dual NTSC/PAL

	// Bits 2-7 are unused and must be 0.
} NSFe_INFO_PAL_NTSC_e;

/**
 * NSFe: INFO: Extra sound chips
 */
typedef enum ATTR_FLAG_ENUM {
	NSFe_INFO_EXTRA_SOUND_CHIP_VRC6		= (1U << 0),
	NSFe_INFO_EXTRA_SOUND_CHIP_VRC7		= (1U << 1),
	NSFe_INFO_EXTRA_SOUND_CHIP_FDS		= (1U << 2),
	NSFe_INFO_EXTRA_SOUND_CHIP_MMC5		= (1U << 3),
	NSFe_INFO_EXTRA_SOUND_CHIP_NAMCO_163	= (1U << 4),
	NSFe_INFO_EXTRA_SOUND_CHIP_SUNSOFT_5B	= (1U << 5),
	NSFe_INFO_EXTRA_SOUND_CHIP_VT02PLUS	= (1U << 6),
	NSFe_INFO_EXTRA_SOUND_CHIP_reserved	= (1U << 7),
} NSFe_INFO_Extra_Sound_Chips_e;

/**
 * NSFe: BANK chunk
 *
 * If present, indicates the NSFe should use bankswitching and
 * contains 8 banks specifying which 8 banks to initialize with.
 */
typedef struct _NSFe_BANK_t {
	uint8_t banks[8];		// [0x008] Bankswitch initialization
} NSFe_BANK_t;
ASSERT_STRUCT(NSFe_BANK_t, 8);

/**
 * NSFe: RATE chunk
 * If present, indicates the playback rate, in microseconds.
 *
 * All fields are in little-endian.
 */
typedef union _NSFe_RATE_t {
	struct {
		uint16_t ntsc;	// [0x000] Playback rate, for NTSC
		uint16_t pal;	// [0x002] Playback rate, for PAL
		uint16_t dendy;	// [0x004] Playback rate, for Dendy
	} rate;
	uint16_t rates[3];	// [0x000] Playback rates
} NSFe_RATE_t;
ASSERT_STRUCT(NSFe_RATE_t, 6);

/**
 * NSFe: NSF2 chunk
 *
 * If present, contains NSF2 feature flags.
 * See nsf_structs.h, NSF2_Feature_Flags_e.
 */
typedef struct _NSFe_NSF2_t {
	uint8_t nsf2_feature_flags;	// [0x000] NSF2 feature flags (See NSF2_Feature_Flags_e)
} NSFe_NSF2_t;
ASSERT_STRUCT(NSFe_NSF2_t, 1);

/**
 * NSFe: VRC7 chunk
 *
 * If present, a VRC7 audio chip is required for this file.
 *
 * NOTE: Only the first byte is required.
 * 128 or 152 additional bytes may be provided as a replacement
 * patch set for the VRC7.
 */
typedef struct _NSFe_VRC7_t {
	uint8_t vrc7_variant;		// [0x000] VRC7 variant (see NSFe_VRC7_Variant_e)
} NSFe_VRC7_t;
ASSERT_STRUCT(NSFe_VRC7_t, 1);

/**
 * NSFe: VRC7: Variants
 */
typedef enum {
	NSFe_VRC7_VARIANT_VRC7		= 0,	// VRC7
	NSFe_VRC7_VARIANT_YM2413	= 1,	// YM2413
} NSFe_VRC7_Variant_e;

/**
 * NSFe: regn chunk
 *
 * Indicates the supported regions.
 */
typedef struct _NSFe_regn_t {
	uint8_t supported_regions;	// [0x000] Supported regions (see NSFe_regn_Regions_BF_e)
	uint8_t preferred_region;	// [0x001] Preferred region (see NSFe_regn_Regions_e)
} NSFe_regn_t;
ASSERT_STRUCT(NSFe_regn_t, 2);

/**
 * NSFe: regn: Supported regions bitfield
 */
typedef enum ATTR_FLAG_ENUM {
	NSFe_regn_REGION_BF_NTSC	= (1U << 0),	// NTSC
	NSFe_regn_REGION_BF_PAL		= (1U << 1),	// PAL
	NSFe_regn_REGION_BF_Dendy	= (1U << 2),	// Dendy
} NSFe_regn_Regions_BF_e;

/**
 * NSFe: regn: Preferred region
 */
typedef enum ATTR_FLAG_ENUM {
	NSFe_regn_REGION_NTSC	= 0,	// NTSC
	NSFe_regn_REGION_PAL	= 1,	// PAL
	NSFe_regn_REGION_Dendy	= 2,	// Dendy
} NSFe_regn_Regions_e;

#ifdef __cplusplus
}
#endif
