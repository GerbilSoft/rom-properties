/****************************************************************************
 * ROM Properties Page shell extension. (librptexture)                      *
 * ps1_tim_structs.h: Sony PlayStation .tim format data structures.         *
 *                                                                          *
 * Copyright (c) 2026 by David Korth.                                       *
 * SPDX-License-Identifier: GPL-2.0-or-later                                *
 ****************************************************************************/

// References:
// - https://qhimm-modding.fandom.com/wiki/PSX/TIM_file
// - http://justsolve.archiveteam.org/wiki/TIM_(PlayStation_graphics)

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Sony PlayStation .tim: File header.
 *
 * NOTE: .tim doesn't have a unique magic number.
 * `file` checks the following:
 * - 0	ulelong			0x00000010
 * - >4	ulelong&0xffFFffF0	=0
 * All fields are in little-endian.
 */
#define PS1_TIM_TAG 0x10
typedef struct _PS1_TIM_Header_t {
	union {
		struct {
			uint8_t tag;		// [0x000] Tag (0x10)
			uint8_t version;	// [0x001] Version (usually 0)
			uint16_t unused_0;	// [0x002]
		};
		uint32_t nq_magic;		// [0x000] Not quite a magic number...
	};
	uint32_t flags;				// [0x004] Flags (see PS1_TIM_Flags_e)

	// If the CLP (Color Lookup table Present) flag is set,
	// this header is immediately followed by PS1_TIM_CLUT_Header_t
	// and the color lookup table.

	// Following the CLUT (or this header if no CLUT is present),
	// there will be PS1_TIM_Image_Header_t, followed by image data.
} PS1_TIM_Header_t;
ASSERT_STRUCT(PS1_TIM_Header_t, 2*sizeof(uint32_t));

/**
 * TIM flags
 */
typedef enum {
	// Color depth (bpp) [2-bit bitfield]
	// NOTE: 4-bpp and 8-bpp may or may not have CLP set.
	// 15-bpp and 24-bpp can *not* have CLP set.
	PS1_TIM_FLAG_BPP_4BPP	= 0U,
	PS1_TIM_FLAG_BPP_8BPP	= 1U,
	PS1_TIM_FLAG_BPP_15BPP	= 2U,
	PS1_TIM_FLAG_BPP_24BPP	= 3U,
	PS1_TIM_FLAG_BPP_MASK	= 0x03,

	// If set, this image has a Color Lookup Table (CLUT).
	PS1_TIM_FLAG_CLP	= 0x08,

	// All valid bits.
	// NOTE: Bit 2 isn't used...
	PS1_TIM_FLAG_VALID_BITS	= PS1_TIM_FLAG_BPP_MASK | PS1_TIM_FLAG_CLP,
} PS1_TIM_Flags_e;

/**
 * Sony PlayStation .tim: Framebuffer positioning struct.
 *
 * All fields are in little-endian.
 */
typedef struct _PS1_TIM_FBpos_t {
	uint16_t x;		// [0x000] X coordinate
	uint16_t y;		// [0x002] Y coordinate
	uint16_t width;		// [0x004] Width
	uint16_t height;	// [0x006] Height
} PS1_TIM_FBpos_t;
ASSERT_STRUCT(PS1_TIM_FBpos_t, 2*sizeof(uint32_t));

/**
 * Sony PlayStation .tim: CLUT header.
 *
 * All fields are in little-endian.
 */
typedef struct _PS1_TIM_CLUT_Header_t {
	uint32_t size;		// [0x000] Size of CLUT, including this header.
				//         (usually 0x2C for 4-bpp)
	PS1_TIM_FBpos_t fb;	// [0x004] Framebuffer positioning
				//         (not important for basic texture decoding)

	// Following the header are 16 or 256 15-bit color entries in
	// BGR555_PS1 format. The STP bit indicates the pixel is transparent,
	// but only if transparent mode is enabled on the console. Hence, it's
	// ignored here.
	//
	// Special case: All-zero pixels are considered transparent, *unless*
	// the STP bit is set.
} PS1_TIM_CLUT_Header_t;
ASSERT_STRUCT(PS1_TIM_CLUT_Header_t, 3*sizeof(uint32_t));

/**
 * Sony PlayStation .tim: Image header.
 *
 * All fields are in little-endian.
 */
typedef struct _PS1_TIM_Image_Header_t {
	uint32_t size;		// [0x000] Size of image, including this header.
	PS1_TIM_FBpos_t fb;	// [0x004] Framebuffer positioning
				// (only width and height are important)

	// Following the header is image data.
	// NOTE: fb.width is in units of 16-bit data blocks.
	// For 15-bpp images, this matches the number of pixels.
	// For all others, adjustments will be necessary, and there will
	// be padding bytes.
} PS1_TIM_Image_Header_t;

#ifdef __cplusplus
}
#endif
