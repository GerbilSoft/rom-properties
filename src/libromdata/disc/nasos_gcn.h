/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * nasos_gcn.h: GameCube/Wii NASOS (.iso.dec) disc image reader.           *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// NOTE: This is reverse-engineered, and most fields are unknown.

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * .iso.dec header.
 *
 * All fields are in little-endian, except for the
 * magic number, which is considered "big-endian".
 */
#define NASOS_MAGIC_GCML 'GCML'
#define NASOS_MAGIC_GCMM 'GCMM'	// TODO: Figure out what this is used for.
#define NASOS_MAGIC_WII5 'WII5'
#define NASOS_MAGIC_WII9 'WII9'
typedef struct _NASOSHeader {
	uint32_t magic;		// [0x000] Magic number. ('GCML', 'WII5', 'WII9')
	char id4[4];		// [0x004] ID4 of the disc image.
} NASOSHeader;
ASSERT_STRUCT(NASOSHeader, 8);

/**
 * .iso.dec header. (with 'GCML' fields)
 *
 * Block size is 2,048 bytes. (0x800)
 * Block count is 0xAE0B0. (712,880)
 *
 * All fields are in little-endian, except for the
 * magic number, which is considered "big-endian".
 */
#define NASOS_GCML_BlockCount 712880
typedef struct _NASOSHeader_GCML {
	NASOSHeader header;	// [0x000] Main NASOS header.
	uint8_t md5_orig[16];	// [0x008] MD5 of the original disc image.
} NASOSHeader_GCML;
ASSERT_STRUCT(NASOSHeader_GCML, 24);

/**
 * .iso.dec header. (with 'WII5' or 'WII9' fields)
 *
 * Block size is 1,024 bytes. (0x400)
 *
 * All fields are in little-endian, except for the
 * magic number, which is considered "big-endian".
 */
#define NASOS_WII5_BlockCount 0x460900 /* 4589824 */
#define NASOS_WII9_BlockCount 0x7ED380 /* 8311680 */
typedef struct _NASOSHeader_WIIx {
	NASOSHeader header;	// [0x000] Main NASOS header.
	uint8_t md5_orig[16];	// [0x008] MD5 of the original disc image.
	uint8_t unknown1[48];	// [0x018]
	uint32_t block_count;	// [0x048] Block count. (divide by 256)
	uint8_t unknown2[16];	// [0x04C]
} NASOSHeader_WIIx;
ASSERT_STRUCT(NASOSHeader_WIIx, 0x5C);

#ifdef __cplusplus
}
#endif /* __cplusplus */
