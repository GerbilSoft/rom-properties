/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * stex_structs.h: Godot STEX texture format data structures.              *
 *                                                                         *
 * Copyright (c) 2017-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_STEX_STRUCTS_H__
#define __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_STEX_STRUCTS_H__

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Godot STEX 3.0: File header.
 * NOTE: The format is changing in Godot master branch. (4.0)
 *
 * All fields are in little-endian.
 */
#define STEX_30_MAGIC 'GDST'
typedef struct _STEX_30_Header {
	uint32_t magic;			// [0x000] 'GDST'
	uint16_t width;			// [0x004] Width
	uint16_t width_custom;		// [0x006] If set, this is width, and `width` is flags?
	uint16_t height;		// [0x008] Height
	uint16_t height_custom;		// [0x008] If set, this is height, and `height` is flags?
	uint32_t flags;			// [0x00C] Texture flags (see STEX_Flags_e)
	uint32_t format;		// [0x010] Texture format (see STEX_Format_e)
} STEX_30_Header;
ASSERT_STRUCT(STEX_30_Header, 5*sizeof(uint32_t));

/**
 * Godot STEX: Texture flags
 */
typedef enum {
	STEX_FLAG_MIPMAPS		= (1U <<  0),	// Enable automatic mipmap generation
	STEX_FLAG_REPEAT		= (1U <<  1),	// Repeate texture (Tiling); otherwise Clamping
	STEX_FLAG_FILTER		= (1U <<  2),	// Create texture with linear (or available) filter
	STEX_FLAG_ANISOTROPIC_FILTER	= (1U <<  3),
	STEX_FLAG_CONVERT_TO_LINEAR	= (1U <<  4),
	STEX_FLAG_MIRRORED_REPEAT	= (1U <<  5),	// Repeat texture, with alternate sections mirrored
	STEX_FLAG_CUBEMAP		= (1U << 11),
	STEX_FLAG_USED_FOR_STREAMING	= (1U << 12),

	STEX_FLAGS_DEFAULT		= STEX_FLAG_REPEAT | STEX_FLAG_MIPMAPS | STEX_FLAG_FILTER
} STEX_30_Flags_e;

/**
 * Godot STEX: Texture format
 */
typedef enum {
	// 0x00
	STEX_FORMAT_L8,		// luminance
	STEX_FORMAT_LA8,	// luminance-alpha
	STEX_FORMAT_R8,
	STEX_FORMAT_RG8,
	STEX_FORMAT_RGB8,
	STEX_FORMAT_RGBA8,
	STEX_FORMAT_RGBA4444,
	STEX_FORMAT_RGB565,

	// 0x08
	STEX_FORMAT_RF,		// float
	STEX_FORMAT_RGF,
	STEX_FORMAT_RGBF,
	STEX_FORMAT_RGBAF,
	STEX_FORMAT_RH,		// half float
	STEX_FORMAT_RGH,
	STEX_FORMAT_RGBH,
	STEX_FORMAT_RGBAH,

	// 0x10
	STEX_FORMAT_RGBE9995,
	STEX_FORMAT_DXT1,	// s3tc bc1
	STEX_FORMAT_DXT3,	// bc2
	STEX_FORMAT_DXT5,	// bc3
	STEX_FORMAT_RGTC_R,
	STEX_FORMAT_RGTC_RG,
	STEX_FORMAT_BPTC_RGBA,	// btpc bc7
	STEX_FORMAT_BPTC_RGBF,	// float bc6h

	// 0x18
	STEX_FORMAT_BPTC_RGBFU,	// unsigned float bc6hu
	STEX_FORMAT_PVRTC1_2,	// pvrtc1
	STEX_FORMAT_PVRTC1_2A,
	STEX_FORMAT_PVRTC1_4,
	STEX_FORMAT_PVRTC1_4A,
	STEX_FORMAT_ETC,	// etc1
	STEX_FORMAT_ETC2_R11,	// etc2
	STEX_FORMAT_ETC2_R11S,	// signed, NOT srgb.

	// 0x20
	STEX_FORMAT_ETC2_RG11,
	STEX_FORMAT_ETC2_RG11S,
	STEX_FORMAT_ETC2_RGB8,
	STEX_FORMAT_ETC2_RGBA8,
	STEX_FORMAT_ETC2_RGB8A1,

	// Proprietary formats used in Sonic Colors Ultimate.
	STEX_FORMAT_SCU_ASTC_8x8 = 0x25,

	// NOTE: The following formats were added in Godot 4.0.
	//STEX_FORMAT_ETC2_RA_AS_RG = 0x25,	//used to make basis universal happy
	//STEX_FORMAT_DXT5_RA_AS_RG = 0x26,	//used to make basis universal happy

	STEX_FORMAT_MAX,

	// Format flags
	STEX_FORMAT_MASK		= (1U << 20) - 1,
	STEX_FORMAT_FLAG_LOSSLESS	= (1U << 21),
	STEX_FORMAT_FLAG_LOSSY		= (1U << 22),
	STEX_FORMAT_FLAG_HAS_MIPMAPS	= (1U << 23),
	STEX_FORMAT_FLAG_DETECT_3D	= (1U << 24),
	STEX_FORMAT_FLAG_DETECT_SRGB	= (1U << 25),
	STEX_FORMAT_FLAG_DETECT_NORMAL	= (1U << 26),
} STEX_Format_e;

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_STEX_STRUCTS_H__ */
