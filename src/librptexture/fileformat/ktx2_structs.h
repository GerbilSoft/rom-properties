/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ktx2_structs.h: Khronos KTX2 texture format data structures.            *
 *                                                                         *
 * Copyright (c) 2019-2023 by David Korth.                                 *
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
 * - https://github.khronos.org/KTX-Specification/
 * - https://github.com/KhronosGroup/KTX-Specification
 */

#include "vk_defs.h"

/**
 * Khronos KTX2: File header.
 * https://github.khronos.org/KTX-Specification/
 *
 * All fields are in little-endian.
 */
#define KTX2_IDENTIFIER "\xABKTX 20\xBB\r\n\x1A\n"
typedef struct _KTX2_Header {
	uint8_t identifier[12];			// [0x000] KTX2_IDENTIFIER
	uint32_t vkFormat;			// [0x00C] Vulkan texture format
	uint32_t typeSize;			// [0x010]
	uint32_t pixelWidth;			// [0x014] Width
	uint32_t pixelHeight;			// [0x018] Height
	uint32_t pixelDepth;			// [0x01C] Depth
	uint32_t layerCount;			// [0x020] Number of layers
	uint32_t faceCount;			// [0x024] Number of faces (cubemap)
	uint32_t levelCount;			// [0x028] Number of mipmap levels
	uint32_t supercompressionScheme;	// [0x02C] Supercompression scheme (See KTX2_Supercompression_e)

	// Indexes.
	// All offsets are absolute. (0 == beginning of file)
	uint32_t dfdByteOffset;			// [0x030] Data Format Descriptor
	uint32_t dfdByteLength;			// [0x034]
	uint32_t kvdByteOffset;			// [0x038] Key/Value Data
	uint32_t kvdByteLength;			// [0x03C]
	uint64_t sgdByteOffset;			// [0x040] Supercompression Global Data
	uint64_t sgdByteLength;			// [0x048]

	// Following KTX2_Header is an array of mipmap level indexes.
	// Array size is specified by levelCount.
} KTX2_Header;
ASSERT_STRUCT(KTX2_Header, 0x50);

/**
 * Khronos KTX2: Supercompression scheme.
 */
typedef enum {
	KTX2_SUPERZ_NONE	= 0,
	KTX2_SUPERZ_BASISU	= 1,
	KTX2_SUPERZ_ZLIB	= 2,
	KTX2_SUPERZ_ZSTD	= 3,
	KTX2_SUPERZ_LZMA	= 4,
} KTX2_Supercompression_e;

/**
 * Khronos KTX2: Mipmap level index.
 *
 * All fields are in little-endian.
 */
typedef struct _KTX2_Mipmap_Index {
	uint64_t byteOffset;			// [0x000] Mipmap offset. (absolute)
	uint64_t byteLength;			// [0x008] Length, in bytes.
	uint64_t uncompressedByteLength;	// [0x010] Total uncompressed size, including all z slices and faces.
} KTX2_Mipmap_Index;
ASSERT_STRUCT(KTX2_Mipmap_Index, 3*sizeof(uint64_t));

#ifdef __cplusplus
}
#endif
