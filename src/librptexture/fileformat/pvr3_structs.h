/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * pvr3_structs.h: PowerVR 3.0.0 texture format data structures.           *
 *                                                                         *
 * Copyright (c) 2019-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * References:
 * - http://cdn.imgtec.com/sdk-documentation/PVR+File+Format.Specification.pdf
 * - https://github.com/powervr-graphics/Native_SDK/blob/master/framework/PVRCore/textureio/FileDefinesPVR.h
 */

/**
 * PowerVR 3.0.0: File header
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
	uint32_t pixel_format;	// See PowerVR3_Pixel_Format_e.
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
 * PowerVR3 flags
 */
typedef enum {
	PVR3_FLAG_COMPRESSED	= (1U << 0),	// File is compressed.
	PVR3_FLAG_PREMULTIPLIED	= (1U << 1),	// Pre-multiplied alpha.
} PowerVR3_Flags_t;

/**
 * PowerVR3 pixel formats
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
} PowerVR3_Pixel_Format_e;

/**
 * PowerVR3 color space
 */
typedef enum {
	PVR3_COLOR_SPACE_RGB	= 0,	// Linear RGB
	PVR3_COLOR_SPACE_sRGB	= 1,	// sRGB

	PVR3_COLOR_SPACE_MAX
} PowerVR3_Color_Space_t;

/**
 * PowerVR3 channel type
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
 * Metadata block header
 */
typedef struct _PowerVR3_Metadata_Block_Header_t {
	uint32_t fourCC;
	uint32_t key;
	uint32_t size;
} PowerVR3_Metadata_Block_Header_t;
ASSERT_STRUCT(PowerVR3_Metadata_Block_Header_t, 3*sizeof(uint32_t));

/**
 * Metadata keys for PowerVR3 fourCC
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
 * PowerVR3 Metadata: Orientation struct
 */
// Some compilers pad this structure to a multiple of 4 bytes
#pragma pack(1)
typedef struct PACKED _PowerVR3_Metadata_Orientation_t {
	uint8_t x;	// 0 == increases to the right; 1 == increases to the left
	uint8_t y;	// 0 == increases downwards; 1 == increases upwards
	uint8_t z;	// 0 == increases inwards; 1 == increases outwards
} PowerVR3_Metadata_Orientation;
ASSERT_STRUCT(PowerVR3_Metadata_Orientation, 3);
#pragma pack()

/**
 * PowerVR legacy header (v1 and v2) [only v2 has a magic number]
 * Reference: https://github.com/powervr-graphics/Native_SDK/blob/master/framework/PVRCore/textureio/FileDefinesPVR.h
 */
#define PVR1_HEADER_SIZE 0x2CU
#define PVR2_HEADER_SIZE 0x34U
#define PVR2_MAGIC_HOST 0x21525650U	// "PVR!"
#define PVR2_MAGIC_SWAP 0x50565221U	// "PVR!"
typedef struct _PowerVR_Legacy_Header {
	uint32_t header_size;			// [0x000]
	uint32_t height;			// [0x004]
	uint32_t width;				// [0x008]
	uint32_t mipmap_count;			// [0x00C]
	uint32_t pixel_format_and_flags;	// [0x010] Pixel format and flags (see PowerVR_Legacy_Pixel_Format_e and PowerVR_Legacy_Flags_e)
	uint32_t data_size;			// [0x014]
	uint32_t bit_count;			// [0x018] Bits per pixel
	uint32_t red_bit_mask;			// [0x01C]
	uint32_t green_bit_mask;		// [0x020]
	uint32_t blue_bit_mask;			// [0x024]
	uint32_t alpha_bit_mask;		// [0x028]

	// v2 fields
	uint32_t magic;				// [0x02C] "PVR!"
	uint32_t num_surfaces;			// [0x030]
} PowerVR_Legacy_Header;
ASSERT_STRUCT(PowerVR_Legacy_Header, PVR2_HEADER_SIZE);

/**
 * PowerVR legacy header (v1 and v2): Flags
 */
typedef enum {
	PVR_LEGACY_FLAG_MIPMAP		= (1U <<  8),
	PVR_LEGACY_FLAG_BUMPMAP		= (1U << 10),
	PVR_LEGACY_FLAG_CUBEMAP		= (1U << 12),
	PVR_LEGACY_FLAG_VOLUME_TEXTURE	= (1U << 14),
	PVR_LEGACY_FLAG_HAS_ALPHA	= (1U << 15),
	PVR_LEGACY_FLAG_VERTICAL_FLIP	= (1U << 16),
} PowerVR_Legacy_Flags_e;

/**
 * PowerVR legacy pixel formats
 */
#define PVR_LEGACY_PIXEL_FORMAT_MASK 0xFFU
typedef enum {
	// MGL Formats
	MGL_ARGB_4444 = 0x00,
	MGL_ARGB_1555,
	MGL_RGB_565,
	MGL_RGB_555,
	MGL_RGB_888,
	MGL_ARGB_8888,
	MGL_ARGB_8332,
	MGL_I_8,
	MGL_AI_88,
	MGL_1_BPP,
	MGL_VY1UY0,
	MGL_Y1VY0U,
	MGL_PVRTC2,
	MGL_PVRTC4,

	// openGL Formats
	GL_RGBA_4444 = 0x10,
	GL_RGBA_5551,
	GL_RGBA_8888,
	GL_RGB_565,
	GL_RGB_555,
	GL_RGB_888,
	GL_I_8,
	GL_AI_88,
	GL_PVRTC2,
	GL_PVRTC4,
	GL_BGRA_8888,
	GL_A_8,
	GL_PVRTCII4,
	GL_PVRTCII2,

	// DirectX 9 and Earlier Formats
	D3D_DXT1 = 0x20,
	D3D_DXT2,
	D3D_DXT3,
	D3D_DXT4,
	D3D_DXT5,
	D3D_RGB_332,
	D3D_AL_44,
	D3D_LVU_655,
	D3D_XLVU_8888,
	D3D_QWVU_8888,
	D3D_ABGR_2101010,
	D3D_ARGB_2101010,
	D3D_AWVU_2101010,
	D3D_GR_1616,
	D3D_VU_1616,
	D3D_ABGR_16161616,
	D3D_R16F,
	D3D_GR_1616F,
	D3D_ABGR_16161616F,
	D3D_R32F,
	D3D_GR_3232F,
	D3D_ABGR_32323232F,

	// Ericsson Texture Compression formats
	e_etc_RGB_4BPP,

	// More DirectX 9 Formats
	D3D_A8 = 0x40,
	D3D_V8U8,
	D3D_L16,
	D3D_L8,
	D3D_AL_88,
	D3D_UYVY,
	D3D_YUY2,

	// DirectX 10+ Formats
	DXGI_R32G32B32A32_FLOAT = 0x50,
	DXGI_R32G32B32A32_UINT,
	DXGI_R32G32B32A32_SINT,
	DXGI_R32G32B32_FLOAT,
	DXGI_R32G32B32_UINT,
	DXGI_R32G32B32_SINT,
	DXGI_R16G16B16A16_FLOAT,
	DXGI_R16G16B16A16_UNORM,
	DXGI_R16G16B16A16_UINT,
	DXGI_R16G16B16A16_SNORM,
	DXGI_R16G16B16A16_SINT,
	DXGI_R32G32_FLOAT,
	DXGI_R32G32_UINT,
	DXGI_R32G32_SINT,
	DXGI_R10G10B10A2_UNORM,
	DXGI_R10G10B10A2_UINT,
	DXGI_R11G11B10_FLOAT,
	DXGI_R8G8B8A8_UNORM,
	DXGI_R8G8B8A8_UNORM_SRGB,
	DXGI_R8G8B8A8_UINT,
	DXGI_R8G8B8A8_SNORM,
	DXGI_R8G8B8A8_SINT,
	DXGI_R16G16_FLOAT,
	DXGI_R16G16_UNORM,
	DXGI_R16G16_UINT,
	DXGI_R16G16_SNORM,
	DXGI_R16G16_SINT,
	DXGI_R32_FLOAT,
	DXGI_R32_UINT,
	DXGI_R32_SINT,
	DXGI_R8G8_UNORM,
	DXGI_R8G8_UINT,
	DXGI_R8G8_SNORM,
	DXGI_R8G8_SINT,
	DXGI_R16_FLOAT,
	DXGI_R16_UNORM,
	DXGI_R16_UINT,
	DXGI_R16_SNORM,
	DXGI_R16_SINT,
	DXGI_R8_UNORM,
	DXGI_R8_UINT,
	DXGI_R8_SNORM,
	DXGI_R8_SINT,
	DXGI_A8_UNORM,
	DXGI_R1_UNORM,
	DXGI_R9G9B9E5_SHAREDEXP,
	DXGI_R8G8_B8G8_UNORM,
	DXGI_G8R8_G8B8_UNORM,
	DXGI_BC1_UNORM,
	DXGI_BC1_UNORM_SRGB,
	DXGI_BC2_UNORM,
	DXGI_BC2_UNORM_SRGB,
	DXGI_BC3_UNORM,
	DXGI_BC3_UNORM_SRGB,
	DXGI_BC4_UNORM, // unimplemented
	DXGI_BC4_SNORM, // unimplemented
	DXGI_BC5_UNORM, // unimplemented
	DXGI_BC5_SNORM, // unimplemented

	// openVG
	VG_sRGBX_8888 = 0x90,
	VG_sRGBA_8888,
	VG_sRGBA_8888_PRE,
	VG_sRGB_565,
	VG_sRGBA_5551,
	VG_sRGBA_4444,
	VG_sL_8,
	VG_lRGBX_8888,
	VG_lRGBA_8888,
	VG_lRGBA_8888_PRE,
	VG_lL_8,
	VG_A_8,
	VG_BW_1,
	VG_sXRGB_8888,
	VG_sARGB_8888,
	VG_sARGB_8888_PRE,
	VG_sARGB_1555,
	VG_sARGB_4444,
	VG_lXRGB_8888,
	VG_lARGB_8888,
	VG_lARGB_8888_PRE,
	VG_sBGRX_8888,
	VG_sBGRA_8888,
	VG_sBGRA_8888_PRE,
	VG_sBGR_565,
	VG_sBGRA_5551,
	VG_sBGRA_4444,
	VG_lBGRX_8888,
	VG_lBGRA_8888,
	VG_lBGRA_8888_PRE,
	VG_sXBGR_8888,
	VG_sABGR_8888,
	VG_sABGR_8888_PRE,
	VG_sABGR_1555,
	VG_sABGR_4444,
	VG_lXBGR_8888,
	VG_lABGR_8888,
	VG_lABGR_8888_PRE,

	// Number of pixel types, no point iterating beyond this.
	NumPixelTypes,

	// Error type.
	InvalidType = 0xffffffff
} PowerVR_Legacy_Pixel_Format_e;

#ifdef __cplusplus
}
#endif
