/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * adx_structs.h: CRI ADX audio data structures.                           *
 *                                                                         *
 * Copyright (c) 2018 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_AUDIO_ADX_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_AUDIO_ADX_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

/**
 * ADX loop data.
 * This is the same for both types 03 and 04,
 * but type 04 has an extra 12 bytes before this data.
 *
 * All fields are in big-endian.
 */
typedef struct PACKED _ADX_LoopData {
	uint32_t unknown;	// [0x000] Unknown.
	uint32_t loop_flag;	// [0x004] Loop flag.
	uint32_t start_sample;	// [0x008] Starting sample.
	uint32_t start_byte;	// [0x00C] Starting byte.
	uint32_t end_sample;	// [0x010] Ending sample.
	uint32_t end_byte;	// [0x014] Ending byte.
} ADX_LoopData;
ASSERT_STRUCT(ADX_LoopData, 24);

/**
 * ADX header.
 * This matches the ADX header format exactly.
 * References:
 * - https://en.wikipedia.org/wiki/ADX_(file_format)
 * - https://wiki.multimedia.cx/index.php/CRI_ADX_file
 *
 * Types:
 * - 03: Uses loop_03.
 * - 04: Uses loop_04.
 * - 05: No looping data.
 *
 * All fields are in big-endian.
 */
#define ADX_MAGIC_NUM 0x8000
#define ADX_MAGIC_STR "(c)CRI"
#define ADX_FORMAT 3
typedef struct PACKED _ADX_Header {
	uint16_t magic;			// [0x000] Magic number. (0x8000)
					// NOTE: Too short to use by itself;
					// check for "(c)CRI" at the data offset.
	uint16_t data_offset;		// [0x002] Data offset.
					// Copyright string starts at data_offset-2.
	uint8_t format;			// [0x004] Format. (See ADX_Format_e.)
	uint8_t block_size;		// [0x005] Block size. (typically 18)
	uint8_t bits_per_sample;	// [0x006] Bits per sample. (4)
	uint8_t channel_count;		// [0x007] Channel count.
	uint32_t sample_rate;		// [0x008] Sample rate.
	uint32_t sample_count;		// [0x00C] Sample count.
	uint16_t high_pass_cutoff;	// [0x010] High-pass cutoff.
	uint8_t loop_data_style;	// [0x012] Loop data style. ("type": 03, 04, 05)
	uint8_t flags;			// [0x013] Flags. (See ADX_Flags_e.)

	union {
		struct {
			ADX_LoopData data;	// [0x014] Loop data.
		} loop_03;
		struct {
			uint32_t unknown[3];	// [0x014] Unknown.
			ADX_LoopData data;	// [0x020] Loop data.
		} loop_04;
	};
} ADX_Header;
ASSERT_STRUCT(ADX_Header, 56);

/**
 * ADX format. [0x005]
 */
typedef enum {
	ADX_FORMAT_FIXED_COEFF_ADPCM	= 2,
	ADX_FORMAT_ADX			= 3,
	ADX_FORMAT_ADX_EXP_SCALE	= 4,
	ADX_FORMAT_AHX_DC		= 0x10,
	ADX_FORMAT_AHX			= 0x11,
} ADX_Format_e;

/**
 * ADX flags. [0x018]
 */
typedef enum {
	ADX_FLAG_ENCRYPTED	= 0x08,	// Encrypted
} ADX_Flags_e;

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_AUDIO_ADX_STRUCTS_H__ */
