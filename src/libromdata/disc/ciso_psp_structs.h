/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ciso_psp_structs.h: PlayStation Portable CISO structs.                  *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - https://github.com/unknownbrackets/maxcso/blob/master/README_CSO.md
// - https://github.com/unknownbrackets/maxcso/blob/master/src/dax.h
// - https://github.com/unknownbrackets/maxcso/blob/master/src/input.cpp

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
#define CISO_MAGIC 'CISO'	// CISO
#define ZISO_MAGIC 'ZISO'	// Same as CISO v0/v1, but uses LZ4.
typedef struct _CisoPspHeader {
	uint32_t magic;			// [0x000] 'CISO' or 'ZISO'
	uint32_t header_size;		// [0x004] Should be 0x18, but is not reliable in v1.
	uint64_t uncompressed_size;	// [0x008] Uncompressed data size.
	uint32_t block_size;		// [0x010] Block size, usually 2048.
	uint8_t version;		// [0x014] Version. (CISO: 0, 1, or 2; ZISO: 1)
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

/**
 * PlayStation Portable DAX header.
 *
 * All fields are in little-endian.
 */
#define DAX_MAGIC 0x44415800
typedef struct _DaxHeader {
	uint32_t magic;			// [0x000] 'DAX\0'
	uint32_t uncompressed_size;	// [0x004] Uncompressed data size.
	uint32_t version;		// [0x010] Version. (0 or 1)
	uint32_t nc_areas;		// [0x014] Number of non-compressed areas.
	uint32_t unused[4];		// [0x018]
} DaxHeader;
ASSERT_STRUCT(DaxHeader, 8*sizeof(uint32_t));

/**
 * DAX: Non-compressed area.
 */
typedef struct _DaxNCArea {
	uint32_t start;		// [0x000]
	uint32_t count;		// [0x004]
} DaxNCArea;
ASSERT_STRUCT(DaxNCArea, 2*sizeof(uint32_t));

// DAX has a fixed block size.
#define DAX_BLOCK_SIZE 0x2000

/**
 * PlayStation Portable JISO header.
 * NOTE: Based on reverse-engineered samples,
 * so this may be incomplete.
 *
 * - An extra index entry is included to determine the size of the
 *   last compressed block, similar to CISO.
 * - Index entries do NOT use the high bit to indicate uncompressed.
 * - TODO: There might be an NC area.
 * - If a block is uncompressed, the difference between index entries
 *   equals the block size. (Same as CISOv2.)
 *
 * - If block headers are enabled, a 4-byte header is prepended to
 *   each block. Not sure what it's useful for...
 *
 * All fields are in little-endian.
 */
#define JISO_MAGIC 'JISO'	// JISO
typedef struct _JisoHeader {
	uint32_t magic;			// [0x000] 'JISO'
	uint8_t unk_x001;		// [0x004] 0x03?
	uint8_t unk_x002;		// [0x005] 0x01?
	uint16_t block_size;		// [0x006] Block size, usually 2048.
	// TODO: Are block_headers and method 8-bit or 16-bit?
	uint8_t block_headers;		// [0x008] Block headers. (1 if present; 0 if not.)
	uint8_t unk_x009;		// [0x009]
	uint8_t method;			// [0x00A] Method. (See JisoAlgorithm_e.)
	uint8_t unk_x00b;		// [0x00B]
	uint32_t uncompressed_size;	// [0x00C] Uncompressed data size.
	uint8_t md5sum[16];		// [0x010] MD5 hash of the original image.
	uint32_t header_size;		// [0x020] Header size? (0x30)
	uint8_t unknown[12];		// [0x024]
} JisoHeader;
ASSERT_STRUCT(JisoHeader, 0x30);

// 2 KB minimum block size (DVD sector)
// 64 KB maximum block size
#define JISO_BLOCK_SIZE_MIN (2048)
#define JISO_BLOCK_SIZE_MAX (64*1024)

/**
 * JISO: Compression method.
 */
typedef enum {
	JISO_METHOD_LZO		= 0,
	JISO_METHOD_ZLIB	= 1,
} JisoMethod_e;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_CISO_PSP_STRUCTS_H__ */
