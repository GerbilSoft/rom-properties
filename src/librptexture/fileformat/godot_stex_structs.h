/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * stex_structs.h: Godot STEX texture format data structures.              *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Godot STEX 3: File header
 *
 * All fields are in little-endian.
 */
#define STEX3_MAGIC 'GDST'
typedef struct _STEX3_Header {
	uint32_t magic;			// [0x000] 'GDST'
	uint16_t width;			// [0x004] Width
	uint16_t width_rescale;		// [0x006] If set, viewer should rescale image to this width.
	uint16_t height;		// [0x008] Height
	uint16_t height_rescale;	// [0x008] If set, viewer should rescale image to this height.
	uint32_t flags;			// [0x00C] Texture flags (see STEX_Flags_e)
	uint32_t format;		// [0x010] Texture format (see STEX_Format_e)
} STEX3_Header;
ASSERT_STRUCT(STEX3_Header, 5*sizeof(uint32_t));

/**
 * Godot STEX 4: File header
 *
 * All fields are in little-endian.
 */
#define STEX4_MAGIC 'GST2'
#define STEX4_FORMAT_VERSION 1
typedef struct _STEX4_Header {
	// GST2 header
	uint32_t magic;			// [0x000] 'GST2' (2D texture)
	uint32_t version;		// [0x004] Format version (1)
	uint32_t width;			// [0x008]
	uint32_t height;		// [0x00C]
	uint32_t format_flags;		// [0x010] Format flags (see STEX_Format_e) [FLAGS ONLY!]
	uint32_t mipmap_limit;		// [0x014] TODO: What is this used for?
	uint32_t reserved[3];		// [0x018]

	// Image header
	// NOTE: This is the physical image size. If it's different
	// from the above image size (e.g. in ETC2), then rescaling
	// is needed when displaying the image.
	uint32_t data_format;		// [0x024] Data format (see STEX4_DataFormat_e)
	uint16_t img_width;		// [0x028] Image width
	uint16_t img_height;		// [0x02A] Image height
	uint32_t mipmap_count;		// [0x02C] Mipmap count
	uint32_t pixel_format;		// [0x030] Pixel format (see STEX_Format_e) [NO FLAGS!]
} STEX4_Header;
ASSERT_STRUCT(STEX4_Header, 13*sizeof(uint32_t));

/**
 * Godot STEX: Embedded file header for lossless/lossy format.
 * This is immediately followed by a PNG and/or WebP image.
 *
 * Size is little-endian; FourCC is big-endian.
 */
#define STEX_FourCC_PNG		'PNG '
#define STEX_FourCC_WEBP	'WEBP'
typedef struct _STEX_Embed_Header {
	uint32_t size;		// [0x000] Embedded file size
	uint32_t fourCC;	// [0x004] FourCC
} STEX_Embed_Header;
ASSERT_STRUCT(STEX_Embed_Header, 2*sizeof(uint32_t));

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
} STEX3_Flags_e;

/**
 * Godot STEX: Texture format
 * NOTE: Format flags are only part of the texture format in Godot 3.
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

	// NOTE: Godot 4 removed the PVRTC formats.
	// Godot commit 40be15920f849100dbf5bf94a1d09e81bf05c6e4,
	// 2021/12/29 02:06:12 +0100
	// Remove support for PVRTC texture encoding and decoding
	// Pixel format numbering diverges at this point.
	STEX3_FORMAT_PVRTC1_2	= 0x19,	// pvrtc1
	STEX3_FORMAT_PVRTC1_2A,
	STEX3_FORMAT_PVRTC1_4,
	STEX3_FORMAT_PVRTC1_4A,
	STEX3_FORMAT_ETC,	// etc1
	STEX3_FORMAT_ETC2_R11,	// etc2
	STEX3_FORMAT_ETC2_R11S,	// signed, NOT srgb.
	STEX3_FORMAT_ETC2_RG11,
	STEX3_FORMAT_ETC2_RG11S,
	STEX3_FORMAT_ETC2_RGB8,
	STEX3_FORMAT_ETC2_RGBA8,
	STEX3_FORMAT_ETC2_RGB8A1,

	// Proprietary formats used in Sonic Colors Ultimate.
	STEX3_FORMAT_SCU_ASTC_8x8 = 0x25,

	STEX3_FORMAT_MAX,

	// Godot 4 pixel formats, starting at ETC.
	STEX4_FORMAT_ETC	= 0x19,	// etc1
	STEX4_FORMAT_ETC2_R11,		// etc2
	STEX4_FORMAT_ETC2_R11S,		// signed, NOT srgb.
	STEX4_FORMAT_ETC2_RG11,
	STEX4_FORMAT_ETC2_RG11S,
	STEX4_FORMAT_ETC2_RGB8,
	STEX4_FORMAT_ETC2_RGBA8,
	STEX4_FORMAT_ETC2_RGB8A1,

	// NOTE: The following formats were added in Godot 4.0.
	STEX4_FORMAT_ETC2_RA_AS_RG,	//used to make basis universal happy
	STEX4_FORMAT_DXT5_RA_AS_RG,	//used to make basis universal happy
	STEX4_FORMAT_ASTC_4x4,
	STEX4_FORMAT_ASTC_4x4_HDR,
	STEX4_FORMAT_ASTC_8x8,
	STEX4_FORMAT_ASTC_8x8_HDR,

	STEX4_FORMAT_MAX,

	// Format flags
	// NOTE: Godot 4 doesn't use lossless, lossy, or detect sRGB.
	STEX_FORMAT_MASK			= (1U << 20) - 1,
	STEX_FORMAT_FLAG_LOSSLESS		= (1U << 20),
	STEX_FORMAT_FLAG_LOSSY			= (1U << 21),
	STEX_FORMAT_FLAG_STREAM			= (1U << 22),
	STEX_FORMAT_FLAG_HAS_MIPMAPS		= (1U << 23),
	STEX_FORMAT_FLAG_DETECT_3D		= (1U << 24),
	STEX_FORMAT_FLAG_DETECT_SRGB		= (1U << 25),
	STEX_FORMAT_FLAG_DETECT_NORMAL		= (1U << 26),
	// Added in Godot 4
	STEX_FORMAT_FLAG_DETECT_ROUGHNESS	= (1U << 27),
} STEX_Format_e;

/**
 * Godot STEX 4: Data format
 */
typedef enum {
	STEX4_DATA_FORMAT_IMAGE			= 0,
	STEX4_DATA_FORMAT_PNG			= 1,
	STEX4_DATA_FORMAT_WEBP			= 2,
	STEX4_DATA_FORMAT_BASIS_UNIVERSAL	= 3,
} STEX4_DataFormat_e;

#ifdef __cplusplus
}
#endif
