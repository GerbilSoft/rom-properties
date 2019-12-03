/****************************************************************************
 * ROM Properties Page shell extension. (librptexture)                      *
 * didj_tex_structs.h: Leapster Didj .tex format data structures.           *
 *                                                                          *
 * Copyright (c) 2019 by David Korth.                                       *
 * SPDX-License-Identifier: GPL-2.0-or-later                                *
 ****************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_DIDJ_TEX_STRUCTS_H__
#define __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_DIDJ_TEX_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

/**
 * Leapster Didj .tex: File header.
 * Reverse-engineered from Didj .tex files.
 *
 * NOTE: The "real" image dimensions are always a power of two.
 * The "used" size may be smaller.
 *
 * All fields are in little-endian.
 */
#define DIDJ_TEX_HEADER_MAGIC 3
typedef struct PACKED _Didj_Tex_Header {
	uint32_t magic;		// [0x000] Magic number? (always 3)
	uint32_t width;		// [0x004] Width [used size]
	uint32_t height;	// [0x008] Height [used size]
	uint32_t width_pow2;	// [0x00C] Width (pow2) [physical size]
	uint32_t height_pow2;	// [0x010] Height (pow2) [physical size]
	uint32_t uncompr_size;	// [0x014] Uncompressed data size, including palette
	uint32_t px_format;	// [0x018] Pixel format (see Didj_Pixel_Format_e)
	uint32_t num_images;	// [0x01C] Number of images? (always 1)
	uint32_t compr_size;	// [0x020] Compressed size (zlib)
} Didj_Tex_Header;
ASSERT_STRUCT(Didj_Tex_Header, 36);

/**
 * Pixel format.
 */
typedef enum {
	DIDJ_PIXEL_FORMAT_8BPP_RGB565	= 6,	// 8bpp; palette is RGB565 [TODO: Transparency?]
	DIDJ_PIXEL_FORMAT_4BPP_RGB565	= 9,	// 4bpp; palette is RGB565 [TODO: Transparency?]
} Didj_Pixel_Format_e;

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_DIDJ_TEX_STRUCTS_H__ */
