/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * palmos_structs.h: Palm OS data structures.                              *
 *                                                                         *
 * Copyright (c) 2018-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - https://en.wikipedia.org/wiki/PRC_(Palm_OS)
// - https://web.mit.edu/pilot/pilot-docs/V1.0/cookbook.pdf
// - https://web.mit.edu/Tytso/www/pilot/prc-format.html
// - https://stuff.mit.edu/afs/sipb/user/yonah/docs/Palm%20OS%20Companion.pdf
// - https://stuff.mit.edu/afs/sipb/user/yonah/docs/Palm%20OS%20Reference.pdf
// - https://www.cs.trinity.edu/~jhowland/class.files.cs3194.html/palm-docs/Constructor%20for%20Palm%20OS.pdf

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Palm OS .prc (Palm Resource Code) header
 *
 * All fields are in big-endian, with 16-bit alignment.
 *
 * NOTE: PDB datetime is "number of seconds since 1904/01/01 00:00:00 UTC".
 */
#pragma pack(2)
typedef struct PACKED _PalmOS_PRC_Header_t {
	char name[32];			// [0x000] Internal name
	uint16_t flags;			// [0x020] Flags (see PalmOS_PRC_Flags_e)
	uint16_t version;		// [0x022] Header version
	uint32_t creation_time;		// [0x024] Creation time (PDB datetime)
	uint32_t modification_time;	// [0x028] Modification time (PDB datetime)
	uint32_t backup_time;		// [0x02C] Backup time (PDB datetime)
	uint32_t mod_num;		// [0x030]
	uint32_t app_info;		// [0x034]
	uint32_t sort_info;		// [0x038]
	uint32_t type;			// [0x03C] File type (see PalmOS_PRC_FileType_e)
	uint32_t creator_id;		// [0x040] Creator ID
	uint32_t unique_id_seed;	// [0x044]
	uint32_t next_record_list;	// [0x048]
	uint16_t num_records;		// [0x04C] Number of resource header records immediately following this header
} PalmOS_PRC_Header_t;
ASSERT_STRUCT(PalmOS_PRC_Header_t, 0x4E);
#pragma pack()

/**
 * Palm OS file types
 */
typedef enum {
	PalmOS_PRC_FileType_Application	= 'appl',
} PalmOS_PRC_FileType_e;

/**
 * Palm OS resource header record
 *
 * All fields are in big-endian, with 16-bit alignment.
 */
#pragma pack(2)
typedef struct PACKED _PalmOS_PRC_ResHeader_t {
	uint32_t type;		// [0x000] Resource type (see PalmOS_PRC_ResType_e)
	uint16_t id;		// [0x004] Resource ID
	uint32_t addr;		// [0x006] Address of the resource data (absolute)
} PalmOS_PRC_ResHeader_t;
ASSERT_STRUCT(PalmOS_PRC_ResHeader_t, 10);
#pragma pack()

/**
 * Palm OS resource types
 */
typedef enum {
	PalmOS_PRC_ResType_ApplicationIcon	= 'tAIB',
	PalmOS_PRC_ResType_ApplicationName	= 'tAIN',
	PalmOS_PRC_ResType_ApplicationVersion	= 'tver',
} PalmOS_PRC_ResType_e;

/**
 * Palm OS BitmapType struct
 *
 * All fields are in big-endian, with 16-bit alignment.
 *
 * NOTE: There's four versions of this struct.
 * For convenience, they'll all be merged into a
 * single struct with unions, and there will be
 * macros for the various struct sizes.
 */
#pragma pack(2)
typedef struct PACKED _PalmOS_BitmapType_t {
	int16_t width;		// [0x000] Width
	int16_t height;		// [0x002] Height
	uint16_t rowBytes;	// [0x004] Number of bytes per row
	uint16_t flags;		// [0x006] Flags (see PalmOS_BitmapType_Flags_e)

	// NOTE: pixelSize and version match up with reserved fields in v0.
	// These fields are always 0 in v0.

	uint8_t pixelSize;	// [0x008] Pixel size, i.e. bpp.
				//         v0: Always 0; assume 1 bpp.
				//         v1: 1, 2, 4
				//         v2: 1, 2, 4, 8, 16
				//         v3: 1, 2, 4, 8, 16?
	uint8_t version;	// [0x009] BitmapType version

	union {
		struct {
			uint16_t nextDepthOffset;	// [0x00A] For bitmap families, number of 32-bit DWORDs
							//         to the next bitmap in the family. (relative offset)
							//         Last bitmap in the family has 0 here.
			uint16_t reserved[2];		// [0x00C]
		} v1;
		struct {
			uint16_t nextDepthOffset;	// [0x00A] For bitmap families, number of 32-bit DWORDs
							//         to the next bitmap in the family. (relative offset)
							//         Last bitmap in the family has 0 here.
			uint8_t transparentIndex;	// [0x00C] If the hasTransparency flag is set, indicates
							//         the palette index to use as transparent.
			uint8_t compressionType;	// [0x00D] If the compressed flag is set, indicates
							//         compression type. (See PalmOS_BitmapType_CompressionType_e)
			uint16_t reserved;
		} v2;
		struct {
			uint8_t size;			// [0x00A] Size of the struct, in bytes (not including color table or bitmap data)
			uint8_t pixelFormat;		// [0x00B] Pixel format (See PalmOS_BitmapType_PixelFormat_e)
			uint8_t unused;			// [0x00C]
			uint8_t compressionType;	// [0x00D] If the compressed flag is set, indicates
							//         compression type. (See PalmOS_BitmapType_CompressionType_e)
			uint16_t density;		// [0x00E] Pixel density (See PalmOS_BitmapType_Density_e)
			uint32_t transparentValue;	// [0x010] For 8 bpp or less: Indicates transparent color index.
							//         For 16 bpp: Indicates 16-bit transparent color.
			uint32_t nextBitmapOffset;	// [0x014] For bitmap families, number of bytes to the
							//         next bitmap in the family. (relative offset)
							//         Last bitmap in the family has 0 here.
		} v3;
	};
} PalmOS_BitmapType_t;
ASSERT_STRUCT(PalmOS_BitmapType_t, 0x18);
#define PalmOS_BitmapType_v0_SIZE 16
#define PalmOS_BitmapType_v1_SIZE 16
#define PalmOS_BitmapType_v2_SIZE 16
#define PalmOS_BitmapType_v3_SIZE (sizeof(PalmOS_BitmapType_t))

/**
 * Palm OS BitmapType flags
 */
typedef enum {
	PalmOS_BitmapType_Flags_compressed		= (1U << 0),
	PalmOS_BitmapType_Flags_hasColorTable		= (1U << 1),
	PalmOS_BitmapType_Flags_hasTransparency		= (1U << 2),
	PalmOS_BitmapType_Flags_indirect		= (1U << 3),
	PalmOS_BitmapType_Flags_forScreen		= (1U << 4),
	PalmOS_BitmapType_Flags_directColor		= (1U << 5),	// Direct color (RGB) bitmap
	PalmOS_BitmapType_Flags_indirectColorTable	= (1U << 6),	// If set: Pointer to color table follows BitmapType structure
									// If clear: Color table follows BitmapType structure
	PalmOS_BitmapType_Flags_noDither		= (1U << 7),
} PalmOS_BitmapType_Flags_e;

/**
 * Palm OS BitmapType compression type
 * For v2/v3 only.
 */
typedef enum {
	PalmOS_BitmapType_CompressionType_ScanLine	= 0,
	PalmOS_BitmapType_CompressionType_RLE		= 1,
	PalmOS_BitmapType_CompressionType_PackBits	= 2,
	PalmOS_BitmapType_CompressionType_End		= 3,
	PalmOS_BitmapType_CompressionType_Best		= 0x64,
	PalmOS_BitmapType_CompressionType_None		= 0xFF,
} PalmOS_BitmapType_Compression_e;

/**
 * Palm OS BitmapType pixel format
 * For v3 only.
 */
typedef enum {
	PalmOS_BitmapType_PixelFormat_Indexed		= 0,	// Palette
	PalmOS_BitmapType_PixelFormat_RGB565_BE		= 1,	// RGB565 (big-endian)
	PalmOS_BitmapType_PixelFormat_RGB565_LE		= 2,	// RGB565 (little-endian)
	PalmOS_BitmapType_PixelFormat_Indexed_LE	= 3,	// Palette (little-endian?)
} PalmOS_BitmapType_PixelFormat_e;

/**
 * Palm OS BitmapType pixel density
 * For v3 only.
 */
typedef enum {
	PalmOS_BitmapType_Density_Low		= 72,	// 160x160 display
	PalmOS_BitmapType_Density_OneAndAHalf	= 108,	// 240x240 display (or 240x320 + soft Graffiti)
	PalmOS_BitmapType_Density_Double	= 144,	// 320x320 display
	PalmOS_BitmapType_Density_Triple	= 216,	// 480x480 display
	PalmOS_BitmapType_Density_Quadruple	= 288,	// 640x640 display
} PalmOS_BitmapType_Density_e;

#ifdef __cplusplus
}
#endif
