/***************************************************************************
 * ROM Properties Page shell extension. (librpbase/tests)                  *
 * bmp.h: BMP image format definitions.                                    *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_TESTS_BMP_H__
#define __ROMPROPERTIES_LIBRPBASE_TESTS_BMP_H__

#include "common.h"
#include <stdint.h>

// BMP magic.
static const uint16_t BMP_magic = 0x424D; // "BM"

#define BITMAPFILEHEADER_SIZE	14U

// Reference: https://en.wikipedia.org/wiki/BMP_file_format#DIB_header_.28bitmap_information_header.29
#define BITMAPCOREHEADER_SIZE		12U
#define OS21XBITMAPHEADER_SIZE		12U
#define OS22XBITMAPHEADER_SIZE		64U
#define OS22XBITMAPHEADER_SHORT_SIZE	16U
#define BITMAPINFOHEADER_SIZE	 	40U
#define BITMAPV2INFOHEADER_SIZE		52U
#define BITMAPV3INFOHEADER_SIZE		56U
#define BITMAPV4HEADER_SIZE		108U
#define BITMAPV5HEADER_SIZE		124U

#ifdef _WIN32

// Windows: Get the bitmap structs from the Windows SDK.
#include <windows.h>

#else /* !_WIN32 */

// Other pltaforms: Define the bitmap structures here.

#ifdef __cplusplus
extern "C" {
#endif

/**
 * BITMAPFILEHEADER
 * All fields are little-endian, except for bfType.
 * Copied from MinGW-w64.
 */
#pragma pack(2)
typedef struct tagBITMAPFILEHEADER {
	uint16_t bfType;
	uint32_t bfSize;
	uint16_t bfReserved1;
	uint16_t bfReserved2;
	uint32_t bfOffBits;
} BITMAPFILEHEADER;
#pragma pack()

/**
 * BITMAPINFOHEADER
 * All fields are little-endian.
 * Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/dd183375(v=vs.85).aspx
 */
typedef struct PACKED tagBITMAPINFOHEADER {
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

// biCompression
#define BI_RGB		0
#define BI_RLE8		1
#define BI_RLE4		2
#define BI_BITFIELDS	3
#define BI_JPEG		4
#define BI_PNG		5

#ifdef __cplusplus
}
#endif

#endif /* _WIN32 */

#endif /* __ROMPROPERTIES_LIBRPBASE_TESTS_BMP_H__ */
