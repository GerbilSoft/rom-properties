/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ciso_gcn.h: GameCube/Wii CISO structs.                                  *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/DiscIO/CISOBlob.cpp
// - https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/DiscIO/CISOBlob.h

#ifndef __ROMPROPERTIES_LIBROMDATA_DISC_CISO_GCN_H__
#define __ROMPROPERTIES_LIBROMDATA_DISC_CISO_GCN_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#pragma pack(1)

#define CISO_HEADER_SIZE 0x8000
#define CISO_MAP_SIZE (CISO_HEADER_SIZE - sizeof(uint32_t) - (sizeof(char) * 4))

// 32 KB minimum block size (GCN/Wii sector)
// 16 MB maximum block size
#define CISO_BLOCK_SIZE_MIN (32768)
#define CISO_BLOCK_SIZE_MAX (16*1024*1024)

/**
 * CISO (GameCube) header struct.
 *
 * All fields are in little-endian.
 */
#define CISO_MAGIC 'CISO'
typedef struct PACKED _CISOHeader {
	uint32_t magic;			// [0x000] 'CISO'
	uint32_t block_size;		// [0x004] LE32
	uint8_t map[CISO_MAP_SIZE];	// [0x008] 0 == unused; 1 == used; other == invalid
} CISOHeader;

#pragma pack()

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_CISO_GCN_H__ */
