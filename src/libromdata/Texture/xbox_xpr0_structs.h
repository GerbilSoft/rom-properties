/****************************************************************************
 * ROM Properties Page shell extension. (libromdata)                        *
 * xbox_xpr0_structs.h: Microsoft Xbox XPR0 texture format data structures. *
 *                                                                          *
 * Copyright (c) 2019 by David Korth.                                       *
 *                                                                          *
 * This program is free software; you can redistribute it and/or modify it  *
 * under the terms of the GNU General Public License as published by the    *
 * Free Software Foundation; either version 2 of the License, or (at your   *
 * option) any later version.                                               *
 *                                                                          *
 * This program is distributed in the hope that it will be useful, but      *
 * WITHOUT ANY WARRANTY; without even the implied warranty of               *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 * GNU General Public License for more details.                             *
 *                                                                          *
 * You should have received a copy of the GNU General Public License along  *
 * with this program; if not, write to the Free Software Foundation, Inc.,  *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.            *
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
 * TODO: Is an image format specified somewhere?
 * Assuming DXT1 for now.
 *
 * All fields are in little-endian.
 */
#define XBOX_XPR0_MAGIC 'XPR0'
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
 * Reverse-engineered from xprextract2.exe.
 */
typedef enum {
	XPR0_PIXEL_FORMAT_ARGB1555	= 0x02,
	XPR0_PIXEL_FORMAT_ARGB4444	= 0x04,
	XPR0_PIXEL_FORMAT_RGB565	= 0x05,
	XPR0_PIXEL_FORMAT_ARGB8888	= 0x06,
	XPR0_PIXEL_FORMAT_xRGB8888	= 0x07,
	XPR0_PIXEL_FORMAT_DXT1		= 0x0C,
	XPR0_PIXEL_FORMAT_DXT2		= 0x0E,
	XPR0_PIXEL_FORMAT_DXT4		= 0x0F,
	XPR0_PIXEL_FORMAT_LIN_ARGB1555	= 0x10,
	XPR0_PIXEL_FORMAT_LIN_RGB565	= 0x11,
	XPR0_PIXEL_FORMAT_LIN_ARGB8888	= 0x12,
	XPR0_PIXEL_FORMAT_LIN_ARGB4444	= 0x1D,
	XPR0_PIXEL_FORMAT_LIN_xRGB8888	= 0x1E,
} XPR0_Pixel_Format_e;

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_TEXTURE_XBOX_XPR0_STRUCTS_H__ */
