/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ciso_gcn.h: GameCube/Wii CISO structs.                                  *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/DiscIO/CompressedBlob.cpp
// - https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/DiscIO/CompressedBlob.h

#ifndef __ROMPROPERTIES_LIBROMDATA_DISC_GCZ_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_DISC_GCZ_STRUCTS_H__

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Dolphin Compressed Blob (GCZ) header.
 *
 * All fields are in little-endian.
 */
#define GCZ_MAGIC 0xB10BC001
typedef struct _GczHeader {
	uint32_t magic;			// [0x000] 0xB10BC001
	uint32_t sub_type;		// [0x004] Subtype. (See GCZ_SubType_e.)
	uint64_t z_data_size;		// [0x008] Compressed data size.
	uint64_t data_size;		// [0x010] Uncompressed data size.
	uint32_t block_size;		// [0x014] Block size.
	uint32_t num_blocks;		// [0x018] Number of blocks.
} GczHeader;
ASSERT_STRUCT(GczHeader, 8*sizeof(uint32_t));

/**
 * GCZ: Subtype.
 * NOTE: NKit uses this field to force a Redump CRC32 match.
 * Don't rely on it being accurate.
 * Decompress the GCN/Wii disc header and verify the magic
 * numbers there instead.
 */
typedef enum {
	GCZ_SubType_GameCube	= 0,
	GCZ_SubType_Wii		= 1,
} GCZ_SubType_e;

// 32 KB minimum block size (GCN/Wii sector) [FIXME: NKit uses 16 KB]
// 16 MB maximum block size
//#define GCZ_BLOCK_SIZE_MIN (32768)
#define GCZ_BLOCK_SIZE_MIN (16384)
#define GCZ_BLOCK_SIZE_MAX (16*1024*1024)

// Bit 63 of the block pointer is set if the block is not compressed.
#define GCZ_FLAG_BLOCK_NOT_COMPRESSED	(1ULL << 63)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_GCZ_STRUCTS_H__ */
