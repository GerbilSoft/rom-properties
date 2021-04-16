/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * pvr3_structs.h: PowerVR 3.0.0 texture format data structures.           *
 *                                                                         *
 * Copyright (c) 2019-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_PVR3_STRUCTS_H__
#define __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_PVR3_STRUCTS_H__

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * References:
 * - http://cdn.imgtec.com/sdk-documentation/PVR+File+Format.Specification.pdf
 */

/**
 * PowerVR 3.0.0: File header.
 * Reference: http://cdn.imgtec.com/sdk-documentation/PVR+File+Format.Specification.pdf
 *
 * Endianness depends on the value of the `version` field.
 * This field contains PVR3_VERSION_HOST if the proper endianness is used,
 * or PVR3_VERSION_SWAP if the endianness is swapped.
 *
 * Note that a little-endian file has the literal text "PVR\x03",
 * whereas a big-endian file has "\x03PVR".
 */
#define PVR3_VERSION_HOST 0x03525650	// "PVR\x03"
#define PVR3_VERSION_SWAP 0x50565203	// "PVR\x03"
typedef struct _PowerVR3_Header {
	uint32_t version;	// PVR3_VERSION_HOST
	uint32_t flags;		// See PVR3_Flags_t.

	// Pixel format is split into two 32-bit values.
	// If the Hi DWORD is 0, the Lo DWORD contains a
	// PVR3_Pixel_Format_t value.
	// Otherwise, the Lo DWORD contains the characters
	// 'r','g','b','a','\0' arranged in channel order,
	// and the Hi DWORD has corresponding channel bit counts.
	uint32_t pixel_format;	// See PVR3_Pixel_Format_t.
	uint32_t channel_depth;	// If non-zero, this is linear RGB.

	uint32_t color_space;	// See PVR3_Color_Space_t.
	uint32_t channel_type;	// See PVR3_Channel_Type_t.
	uint32_t height;
	uint32_t width;
	uint32_t depth;
	uint32_t num_surfaces;
	uint32_t num_faces;
	uint32_t mipmap_count;
	uint32_t metadata_size;
} PowerVR3_Header;
ASSERT_STRUCT(PowerVR3_Header, 52);

/**
 * PowerVR3 flags.
 */
typedef enum {
	PVR3_FLAG_COMPRESSED	= (1U << 0),	// File is compressed.
	PVR3_FLAG_PREMULTIPLIED	= (1U << 1),	// Pre-multiplied alpha.
} PowerVR3_Flags_t;

/**
 * PowerVR3 pixel formats.
 */
typedef enum {
	PVR3_PXF_PVRTC_2bpp_RGB		= 0,
	PVR3_PXF_PVRTC_2bpp_RGBA	= 1,
	PVR3_PXF_PVRTC_4bpp_RGB		= 2,
	PVR3_PXF_PVRTC_4bpp_RGBA	= 3,
	PVR3_PXF_PVRTCII_2bpp		= 4,
	PVR3_PXF_PVRTCII_4bpp		= 5,

	PVR3_PXF_ETC1			= 6,
	PVR3_PXF_DXT1			= 7,
	PVR3_PXF_DXT2			= 8,
	PVR3_PXF_DXT3			= 9,
	PVR3_PXF_DXT4			= 10,
	PVR3_PXF_DXT5			= 11,

	// BC1-BC3 are synonyms of the DXTn formats.
	PVR3_PXF_BC1			= PVR3_PXF_DXT1,
	PVR3_PXF_BC2			= PVR3_PXF_DXT3,
	PVR3_PXF_BC3			= PVR3_PXF_DXT5,

	PVR3_PXF_BC4			= 12,
	PVR3_PXF_BC5			= 13,
	PVR3_PXF_BC6			= 14,
	PVR3_PXF_BC7			= 15,

	PVR3_PXF_UYVY			= 16,
	PVR3_PXF_YUY2			= 17,
	PVR3_PXF_BW1bpp			= 18,
	PVR3_PXF_R9G9B9E5		= 19,
	PVR3_PXF_RGBG8888		= 20,
	PVR3_PXF_GRGB8888		= 21,

	PVR3_PXF_ETC2_RGB		= 22,
	PVR3_PXF_ETC2_RGBA		= 23,
	PVR3_PXF_ETC2_RGB_A1		= 24,
	PVR3_PXF_EAC_R11		= 25,
	PVR3_PXF_EAC_RG11		= 26,

	PVR3_PXF_ASTC_4x4		= 27,
	PVR3_PXF_ASTC_5x4		= 28,
	PVR3_PXF_ASTC_5x5		= 29,
	PVR3_PXF_ASTC_6x5		= 30,
	PVR3_PXF_ASTC_6x6		= 31,
	PVR3_PXF_ASTC_8x5		= 32,
	PVR3_PXF_ASTC_8x6		= 33,
	PVR3_PXF_ASTC_8x8		= 34,
	PVR3_PXF_ASTC_10x5		= 35,
	PVR3_PXF_ASTC_10x6		= 36,
	PVR3_PXF_ASTC_10x8		= 37,
	PVR3_PXF_ASTC_10x10		= 38,
	PVR3_PXF_ASTC_12x10		= 39,
	PVR3_PXF_ASTC_12x12		= 40,
	PVR3_PXF_ASTC_3x3x3		= 41,
	PVR3_PXF_ASTC_4x3x3		= 42,
	PVR3_PXF_ASTC_4x4x3		= 43,
	PVR3_PXF_ASTC_4x4x4		= 44,
	PVR3_PXF_ASTC_5x4x4		= 45,
	PVR3_PXF_ASTC_5x5x4		= 46,
	PVR3_PXF_ASTC_5x5x5		= 47,
	PVR3_PXF_ASTC_6x5x5		= 48,
	PVR3_PXF_ASTC_6x6x5		= 49,
	PVR3_PXF_ASTC_6x6x6		= 50,

	PVR3_PXF_MAX
} PowerVR3_Pixel_Format_t;

/**
 * PowerVR3 color space.
 */
typedef enum {
	PVR3_COLOR_SPACE_RGB	= 0,	// Linear RGB
	PVR3_COLOR_SPACE_sRGB	= 1,	// sRGB

	PVR3_COLOR_SPACE_MAX
} PowerVR3_Color_Space_t;

/**
 * PowerVR3 channel type.
 */
typedef enum {
	PVR3_CHTYPE_UBYTE_NORM	= 0,
	PVR3_CHTYPE_SBYTE_NORM	= 1,
	PVR3_CHTYPE_UBYTE	= 2,
	PVR3_CHTYPE_SBYTE	= 3,
	PVR3_CHTYPE_USHORT_NORM	= 4,
	PVR3_CHTYPE_SSHORT_NORM	= 5,
	PVR3_CHTYPE_USHORT	= 6,
	PVR3_CHTYPE_SHORT	= 7,
	PVR3_CHTYPE_UINT_NORM	= 8,
	PVR3_CHTYPE_SINT_NORM	= 9,
	PVR3_CHTYPE_UINT	= 10,
	PVR3_CHTYPE_SINT	= 11,
	PVR3_CHTYPE_FLOAT	= 12,

	PVR3_CHTYPE_MAX
} PowerVR3_Channel_Type_t;

/**
 * Metadata block header.
 */
typedef struct _PowerVR3_Metadata_Block_Header_t {
	uint32_t fourCC;
	uint32_t key;
	uint32_t size;
} PowerVR3_Metadata_Block_Header_t;
ASSERT_STRUCT(PowerVR3_Metadata_Block_Header_t, 3*sizeof(uint32_t));

/**
 * Metadata keys for PowerVR3 fourCC.
 */
typedef enum {
	PVR3_META_TEXTURE_ATLAS		= 0,
	PVR3_META_NORMAL_MAP		= 1,
	PVR3_META_CUBE_MAP		= 2,
	PVR3_META_ORIENTATION		= 3,
	PVR3_META_BORDER		= 4,
	PVR3_META_PADDING		= 5,
} PVR3_Metadata_Keys_t;

/**
 * PowerVR3 Metadata: Orientation struct.
 */
typedef struct _PowerVR3_Metadata_Orientation_t {
	uint8_t x;	// 0 == increases to the right; 1 == increases to the left
	uint8_t y;	// 0 == increases downwards; 1 == increases upwards
	uint8_t z;	// 0 == increases inwards; 1 == increases outwards
} PowerVR3_Metadata_Orientation;
ASSERT_STRUCT(PowerVR3_Metadata_Orientation, 3);

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_PVR3_STRUCTS_H__ */
