/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * bmp_structs.h: BMP struct definitions                                   *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 * Copyright (c) 2016-2017 by Egor.                                        *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_RPCLI_BMP_STRUCT_H__
#define __ROMPROPERTIES_RPCLI_BMP_STRUCT_H__

#ifdef _WIN32
// Windows: Use bitmap structs from Windows directly.
#include "RpWin32_sdk.h"
#else /* !_WIN32 */

// Non-Windows platform: Define bitmap structs.

/**
 * Bitmap header overview: https://msdn.microsoft.com/en-us/library/dd183376.aspx
 * Short version of the bitmap structure:
 * - BITMAPFILEHEADER
 * - BITMAPINFOHEADER
 * - palette (array of RGBQUAD)
 * - bitmap data
 * Some notes:
 * - Last two fields depend on things like biBitCount and biCompression
 * - RGBQUAD is basically uint32 with colors arranged in 0x00RRGGBB
 * - Everything is little-endian
 * - By default, scanlines go from bottom to top, unless the image height is negative
 */
#include "libromdata/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

/**
 * Bitmap file header.
 * Reference: https://msdn.microsoft.com/en-us/library/dd183374.aspx
 *
 * All fields are little-endian.
 */
#define BITMAPFILEHEADER_MAGIC 0x4D42
typedef struct PACKED _BITMAPFILEHEADER {
	uint16_t bfType;	// "BM" (0x4D42)
	uint32_t bfSize;
	uint16_t bfReserved1;
	uint16_t bfReserved2;
	uint32_t bfOffBits;
} BITMAPFILEHEADER;
ASSERT_STRUCT(BITMAPFILEHEADER, 14);

/**
 * Bitmap information header.
 * Reference: https://msdn.microsoft.com/en-us/library/dd183376.aspx
 *
 * All fields are little-endian.
 */
typedef struct PACKED _BITMAPINFOHEADER {
	uint32_t biSize;
	int32_t  biWidth;
	int32_t  biHeight;
	uint16_t biPlanes;
	uint16_t biBitCount;
	uint32_t biCompression;
	uint32_t biSizeImage;
	int32_t  biXPelsPerMeter;
	int32_t  biYPelsPerMeter;
	uint32_t biClrUsed;
	uint32_t biClrImportant;
} BITMAPINFOHEADER;
ASSERT_STRUCT(BITMAPINFOHEADER, 40);

/**
 * BITMAPINFOHEADER: biCompression
 */
typedef enum {
	BI_RGB = 0,
	BI_RLE8 = 1,
	BI_RLE4 = 2,
	BI_BITFIELDS = 3,
	BI_JPEG = 4,
	BI_PNG = 5,
} BI_COMPRESSION;

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* _WIN32 */

#endif /* __ROMPROPERTIES_RPCLI_BMP_STRUCT_H__ */
