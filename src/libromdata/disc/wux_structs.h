/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * wux_structs.h: Wii U .wux format structs.                               *
 *                                                                         *
 * Copyright (c) 2018-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - https://gbatemp.net/threads/wii-u-image-wud-compression-tool.397901/

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

// Magic numbers (big-endian format)
#define WUX_MAGIC_0	'WUX0'
#define WUX_MAGIC_1	0x2ED09910

// 256 bytes minimum block size
// 128 MB maximum block size
// The original tool only uses 32,768 bytes.
#define WUX_BLOCK_SIZE_MIN (256)
#define WUX_BLOCK_SIZE_MAX (128*1024*1024)

/**
 * .wux disc header.
 *
 * All fields are in little-endian.
 */
typedef struct _wuxHeader_t {
	uint32_t magic[2];		// [0x000] 'WUX0', 0x2ED09910
	uint32_t sectorSize;		// [0x008] Range: [256, 256*1024*1024); must be pow2
	uint32_t reserved1;		// [0x00C]
	uint64_t uncompressedSize;	// [0x010] Total size of the uncompressed disc.
	uint32_t flags;			// [0x018] Currently unused.
	uint32_t reserved2;		// [0x01C]
} wuxHeader_t;
ASSERT_STRUCT(wuxHeader_t, 32);

#ifdef __cplusplus
}
#endif /* __cplusplus */
