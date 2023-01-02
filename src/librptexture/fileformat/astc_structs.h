/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * astc_structs.h: ASTC texture format data structures.                    *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ASTC: File header.
 * Reference: https://arm-software.github.io/vulkan-sdk/_a_s_t_c.html
 *
 * All fields are in little-endian.
 */
#define ASTC_MAGIC 0x5CA1AB13
typedef struct _ASTC_Header {
	uint32_t magic;			// [0x000] 0x5CA1AB13 (little-endian)
	uint8_t blockdimX;		// [0x004] X block dimension
	uint8_t blockdimY;		// [0x005] Y block dimension
	uint8_t blockdimZ;		// [0x006] Z block dimension (1 for 2D ASTC)
	uint8_t width[3];		// [0x007] Width (24-bit LE)
	uint8_t height[3];		// [0x00A] Height (24-bit LE)
	uint8_t depth[3];		// [0x00D] Depth (24-bit LE) (1 for 2D ASTC)
} ASTC_Header;
ASSERT_STRUCT(ASTC_Header, 16);

#ifdef __cplusplus
}
#endif
