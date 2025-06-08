/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ico_structs.h: Windows icon and cursor format data structures.          *
 *                                                                         *
 * Copyright (c) 2019-2025 by David Korth.                                 *
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
 * - http://justsolve.archiveteam.org/wiki/Windows_1.0_Icon
 * - http://justsolve.archiveteam.org/wiki/Windows_1.0_Cursor
 * - http://justsolve.archiveteam.org/wiki/ICO
 * - http://justsolve.archiveteam.org/wiki/CUR
 * - https://devblogs.microsoft.com/oldnewthing/20101018-00/?p=12513
 */

/**
 * Windows 1.0: Icon (and cursor)
 *
 * All fields are in little-endian.
 */
typedef struct _ICO_Win1_Header {
	uint16_t format;	// [0x000] See ICO_Win1_Format_e
	uint16_t hotX;		// [0x002] Cursor hotspot X (cursors only)
	uint16_t hotY;		// [0x004] Cursor hotspot Y (cursors only)
	uint16_t width;		// [0x006] Width, in pixels
	uint16_t height;	// [0x008] Height, in pixels
	uint16_t stride;	// [0x00A] Row stride, in bytes
	uint16_t color;		// [0x00C] Cursor color
} ICO_Win1_Header;
ASSERT_STRUCT(ICO_Win1_Header, 14);

/**
 * Windows 1.0: Icon format
 */
typedef enum {
	ICO_WIN1_FORMAT_MAYBE_WIN3	= 0x0000U,	// may be a Win3 icon/cursor

	ICO_WIN1_FORMAT_ICON_DIB	= 0x0001U,
	ICO_WIN1_FORMAT_ICON_DDB	= 0x0101U,
	ICO_WIN1_FORMAT_ICON_BOTH	= 0x0201U,

	ICO_WIN1_FORMAT_CURSOR_DIB	= 0x0003U,
	ICO_WIN1_FORMAT_CURSOR_DDB	= 0x0103U,
	ICO_WIN1_FORMAT_CURSOR_BOTH	= 0x0203U,
} ICO_Win1_Format_e;

/**
 * Windows 3.x: Icon header
 * Layout-compatible with the Windows 1.0 header.
 *
 * All fields are in little-endian.
 */
typedef struct _ICONDIR {
	uint16_t idReserved;	// [0x000] Zero for Win3.x icons
	uint16_t idType;	// [0x002] See ICO_Win3_idType_e
	uint16_t idCount;	// [0x004] Number of images
} ICONDIR, ICONHEADER;
ASSERT_STRUCT(ICONDIR, 6);

/**
 * Windows 3.x: Icon types
 */
typedef enum {
	ICO_WIN3_TYPE_ICON	= 0x0001U,
	ICO_WIN3_TYPE_CURSOR	= 0x0002U,
} ICO_Win3_IdType_e;

/**
 * Windows 3.x: Icon directory entry
 */
typedef struct _ICONDIRENTRY {
	uint8_t  bWidth;	// [0x000] Width (0 is actually 256)
	uint8_t  bHeight;	// [0x001] Height (0 is actually 256)
	uint8_t  bColorCount;	// [0x002] Color count
	uint8_t  bReserved;	// [0x003]
	uint16_t wPlanes;	// [0x004] Bitplanes (if >1, multiply by wBitCount)
	uint16_t wBitCount;	// [0x006] Bitcount
	uint32_t dwBytesInRes;	// [0x008] Size
	uint32_t dwImageOffset;	// [0x00C] Offset
} ICONDIRENTRY;
ASSERT_STRUCT(ICONDIRENTRY, 16);

// Windows 3.x icons can either have BITMAPCOREHEADER, BITMAPINFOHEADER,
// or a raw PNG image (supported by Windows Vista and later).

// TODO: Combine with librpbase/tests/bmp.h?

/**
 * BITMAPCOREHEADER
 * All fields are little-endian.
 * Reference: https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapcoreheader
 */
#define BITMAPCOREHEADER_SIZE 12U
typedef struct tagBITMAPCOREHEADER {
	uint32_t bcSize;
	uint16_t bcWidth;
	uint16_t bcHeight;
	uint16_t bcPlanes;
	uint16_t bcBitCount;
} BITMAPCOREHEADER;
ASSERT_STRUCT(BITMAPCOREHEADER, BITMAPCOREHEADER_SIZE);

/**
 * BITMAPINFOHEADER
 * All fields are little-endian.
 * Reference: https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfoheader
 */
#define BITMAPINFOHEADER_SIZE	40U
#define BITMAPV2INFOHEADER_SIZE	52U
#define BITMAPV3INFOHEADER_SIZE	56U
#define BITMAPV4HEADER_SIZE	108U
#define BITMAPV5HEADER_SIZE	124U
typedef struct tagBITMAPINFOHEADER {
	uint32_t biSize;		// [0x000] sizeof(BITMAPINFOHEADER)
	int32_t  biWidth;		// [0x004] Width, in pixels
	int32_t  biHeight;		// [0x008] Height, in pixels
	uint16_t biPlanes;		// [0x00C] Bitplanes
	uint16_t biBitCount;		// [0x00E] Bitcount
	uint32_t biCompression;		// [0x010] Compression (see BMP_Compression_e)
	uint32_t biSizeImage;
	int32_t  biXPelsPerMeter;
	int32_t  biYPelsPerMeter;
	uint32_t biClrUsed;
	uint32_t biClrImportant;
} BITMAPINFOHEADER;
ASSERT_STRUCT(BITMAPINFOHEADER, BITMAPINFOHEADER_SIZE);

/**
 * Bitmap compression
 *
 * NOTE: For Windows icons, only BI_RGB and BI_BITFIELDS are valid.
 * For PNG, use a raw PNG without BITMAPINFOHEADER.
 */
typedef enum {
	BI_RGB		= 0U,
	BI_RLE8		= 1U,
	BI_RLE4		= 2U,
	BI_BITFIELDS	= 3U,
	BI_JPEG		= 4U,
	BI_PNG		= 5U,
} BMP_Compression_e;

/** PNG chunks (TODO: Combine with librpbase/tests/bmp.h?) **/

/* These describe the color_type field in png_info. */
/* color type masks */
#define PNG_COLOR_MASK_PALETTE    1
#define PNG_COLOR_MASK_COLOR      2
#define PNG_COLOR_MASK_ALPHA      4

/* color types.  Note that not all combinations are legal */
#define PNG_COLOR_TYPE_GRAY		0
#define PNG_COLOR_TYPE_PALETTE		(PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_PALETTE)
#define PNG_COLOR_TYPE_RGB		(PNG_COLOR_MASK_COLOR)
#define PNG_COLOR_TYPE_RGB_ALPHA	(PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_ALPHA)
#define PNG_COLOR_TYPE_GRAY_ALPHA	(PNG_COLOR_MASK_ALPHA)
/* aliases */
#define PNG_COLOR_TYPE_RGBA		PNG_COLOR_TYPE_RGB_ALPHA
#define PNG_COLOR_TYPE_GA		PNG_COLOR_TYPE_GRAY_ALPHA

/**
 * PNG IHDR chunk
 */
#pragma pack(1)
typedef struct RP_PACKED _PNG_IHDR_t {
	uint32_t width;		// BE32
	uint32_t height;	// BE32
	uint8_t bit_depth;
	uint8_t color_type;
	uint8_t compression_method;
	uint8_t filter_method;
	uint8_t interlace_method;
} PNG_IHDR_t;
ASSERT_STRUCT(PNG_IHDR_t, 13);
#pragma pack()

/**
 * PNG IHDR struct, with length, name, and CRC32.
 */
#pragma pack(1)
typedef struct RP_PACKED _PNG_IHDR_full_t {
	uint32_t chunk_size;	// BE32
	char chunk_name[4];	// "IHDR"
	PNG_IHDR_t data;
	uint32_t crc32;
} PNG_IHDR_full_t;
ASSERT_STRUCT(PNG_IHDR_full_t, 25);
#pragma pack()

#ifdef __cplusplus
}
#endif
