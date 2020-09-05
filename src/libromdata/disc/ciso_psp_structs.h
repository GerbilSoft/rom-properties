/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ciso_psp_structs.h: PlayStation Portable CISO structs.                  *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - https://github.com/unknownbrackets/maxcso/blob/master/README_CSO.md

#ifndef __ROMPROPERTIES_LIBROMDATA_DISC_CISO_PSP_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_DISC_CISO_PSP_STRUCTS_H__

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * PlayStation Portable CISO header.
 *
 * All fields are in little-endian.
 */
#define CISO_MAGIC 'CISO'
typedef struct _CisoPspHeader {
	uint32_t magic;			// [0x000] 'CISO'
	uint32_t header_size;		// [0x004] Should be 0x18, but is not reliable in v1.
	uint64_t uncompressed_size;	// [0x008] Uncompressed data size.
	uint32_t block_size;		// [0x010] Block size, usually 2048.
	uint8_t version;		// [0x014] Version. (0, 1, or 2)
	uint8_t index_shift;		// [0x015] Left shift of index values.
	uint8_t unused[2];		// [0x016]
} CisoPspHeader;
ASSERT_STRUCT(CisoPspHeader, 0x18);

// 2 KB minimum block size (DVD sector)
// 16 MB maximum block size
#define CISO_PSP_BLOCK_SIZE_MIN (2048)
#define CISO_PSP_BLOCK_SIZE_MAX (16*1024*1024)

// For v0 and v1: High bit of index entry is set if the block is not compressed.
#define CISO_PSP_V0_NOT_COMPRESSED	(1U << 31)

// For v2: High bit of index entry is 1==LZ4, 0==deflate.
// Uncompressed is indicated by compressed size == block size.
#define CISO_PSP_V2_LZ4_COMPRESSED	(1U << 31)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_CISO_PSP_STRUCTS_H__ */
