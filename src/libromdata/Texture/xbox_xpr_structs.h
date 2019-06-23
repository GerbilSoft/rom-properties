/****************************************************************************
 * ROM Properties Page shell extension. (libromdata)                        *
 * xbox_xpr0_structs.h: Microsoft Xbox XPR0 texture format data structures. *
 *                                                                          *
 * Copyright (c) 2019 by David Korth.                                       *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ****************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_TEXTURE_XBOX_XPR0_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_TEXTURE_XBOX_XPR0_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

/**
 * Microsoft Xbox XPR0: File header.
 * Reverse-engineered from Xbox save files.
 *
 * All fields are in little-endian.
 */
#define XBOX_XPR0_MAGIC 'XPR0'
#define XBOX_XPR1_MAGIC 'XPR1'
#define XBOX_XPR2_MAGIC 'XPR2'
typedef struct PACKED _Xbox_XPR0_Header {
	uint32_t magic;		// [0x000] 'XPR0'
	uint32_t filesize;	// [0x004] Size of the entire file
	uint32_t data_offset;	// [0x008] Offset to image data
	uint32_t flags;		// [0x00C] Unknown flags
	uint8_t reserved1[8];	// [0x010]
	uint8_t unknown;	// [0x018]
	uint8_t pixel_format;	// [0x019] Pixel format (See XPR0_Pixel_Format_e)
	uint8_t width_pow2;	// [0x01A] Width (high nybble) as a power of 2
	uint8_t height_pow2;	// [0x01B] Height (low nybble) as a power of 2
	uint32_t reserved2;	// [0x01C]

	// 0x020-0x03F are garbage data, usually 0xFFFFFFFF
	// followed by all 0xADADADAD.
} Xbox_XPR0_Header;
ASSERT_STRUCT(Xbox_XPR0_Header, 32);

/**
 * Pixel format.
 * Reference: https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/c709f9e3054ad8e1dae62816f25bef06248415c4/src/core/hle/D3D8/XbConvert.cpp#L871
 */
typedef enum {
	XPR0_PIXEL_FORMAT_L8		= 0x00,
	XPR0_PIXEL_FORMAT_AL8		= 0x01,
	XPR0_PIXEL_FORMAT_ARGB1555	= 0x02,
	XPR0_PIXEL_FORMAT_RGB555	= 0x03,
	XPR0_PIXEL_FORMAT_ARGB4444	= 0x04,
	XPR0_PIXEL_FORMAT_RGB565	= 0x05,
	XPR0_PIXEL_FORMAT_ARGB8888	= 0x06,
	XPR0_PIXEL_FORMAT_xRGB8888	= 0x07,
	// 0x08, 0x09, 0x0A undefined
	XPR0_PIXEL_FORMAT_P8		= 0x0B,
	XPR0_PIXEL_FORMAT_DXT1		= 0x0C,
	// 0x0D undefined
	XPR0_PIXEL_FORMAT_DXT2		= 0x0E,
	XPR0_PIXEL_FORMAT_DXT4		= 0x0F,

	XPR0_PIXEL_FORMAT_LIN_ARGB1555	= 0x10,
	XPR0_PIXEL_FORMAT_LIN_RGB565	= 0x11,
	XPR0_PIXEL_FORMAT_LIN_ARGB8888	= 0x12,
	XPR0_PIXEL_FORMAT_LIN_L8	= 0x13,
	// 0x14, 0x15 undefined
	XPR0_PIXEL_FORMAT_LIN_R8B8	= 0x16,
	XPR0_PIXEL_FORMAT_LIN_G8B8	= 0x17,
	// 0x18 undefined
	XPR0_PIXEL_FORMAT_A8		= 0x19,
	XPR0_PIXEL_FORMAT_A8L8		= 0x1A,
	XPR0_PIXEL_FORMAT_LIN_AL8	= 0x1B,
	XPR0_PIXEL_FORMAT_LIN_RGB555	= 0x1C,
	XPR0_PIXEL_FORMAT_LIN_ARGB4444	= 0x1D,
	XPR0_PIXEL_FORMAT_LIN_xRGB8888	= 0x1E,
	XPR0_PIXEL_FORMAT_LIN_A8	= 0x1F,

	XPR0_PIXEL_FORMAT_LIN_A8L8	= 0x20,
	// 0x21, 0x22, 0x23 undefined
	XPR0_PIXEL_FORMAT_YUY2		= 0x24,
	XPR0_PIXEL_FORMAT_UYVY		= 0x25,
	// 0x26 undefined
	XPR0_PIXEL_FORMAT_L6V5U5	= 0x27,
	XPR0_PIXEL_FORMAT_V8U8		= 0x28,
	XPR0_PIXEL_FORMAT_R8B8		= 0x29,
	XPR0_PIXEL_FORMAT_D24S8		= 0x2A,
	XPR0_PIXEL_FORMAT_F24S8		= 0x2B,
	XPR0_PIXEL_FORMAT_D16		= 0x2C,
	XPR0_PIXEL_FORMAT_F16		= 0x2D,
	XPR0_PIXEL_FORMAT_LIN_D24S8	= 0x2E,
	XPR0_PIXEL_FORMAT_LIN_F24S8	= 0x2F,

	XPR0_PIXEL_FORMAT_LIN_D16	= 0x30,
	XPR0_PIXEL_FORMAT_LIN_F16	= 0x31,
	XPR0_PIXEL_FORMAT_L16		= 0x32,
	XPR0_PIXEL_FORMAT_V16U16	= 0x33,
	// 0x34 undefined
	XPR0_PIXEL_FORMAT_LIN_L16	= 0x35,
	XPR0_PIXEL_FORMAT_LIN_V16U16	= 0x36,
	XPR0_PIXEL_FORMAT_LIN_L6V5U5	= 0x37,
	XPR0_PIXEL_FORMAT_RGBA5551	= 0x38,
	XPR0_PIXEL_FORMAT_RGBA4444	= 0x39,
	XPR0_PIXEL_FORMAT_QWVU8888	= 0x3A,
	XPR0_PIXEL_FORMAT_BGRA8888	= 0x3B,
	XPR0_PIXEL_FORMAT_RGBA8888	= 0x3C,
	XPR0_PIXEL_FORMAT_LIN_RGBA5551	= 0x3D,
	XPR0_PIXEL_FORMAT_LIN_RGBA4444	= 0x3E,
	XPR0_PIXEL_FORMAT_LIN_ABGR8888	= 0x3F,

	XPR0_PIXEL_FORMAT_LIN_BGRA8888	= 0x40,
	XPR0_PIXEL_FORMAT_LIN_RGBA8888	= 0x41,
	// 0x42 to 0x63 undefined

	XPR0_PIXEL_FORMAT_VERTEXDATA	= 0x64,
} XPR0_Pixel_Format_e;

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_TEXTURE_XBOX_XPR0_STRUCTS_H__ */
