/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ktx_structs.h: Khronos KTX texture format data structures.              *
 *                                                                         *
 * Copyright (c) 2017-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_KTX_STRUCTS_H__
#define __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_KTX_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

/**
 * References:
 * - https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/
 */

/**
 * Khronos KTX: File header.
 * Reference: https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/
 *
 * Endianness depends on the value of the `endianness` field.
 * This field contains KTX_ENDIAN_MAGIC if the proper endianness is used.
 */
#define KTX_IDENTIFIER "\xABKTX 11\xBB\r\n\x1A\n"
#define KTX_ENDIAN_MAGIC 0x04030201
typedef struct PACKED _KTX_Header {
	uint8_t identifier[12];		// KTX_IDENTIFIER
	uint32_t endianness;		// KTX_ENDIAN_MAGIC
	uint32_t glType;
	uint32_t glTypeSize;
	uint32_t glFormat;
	uint32_t glInternalFormat;
	uint32_t glBaseInternalFormat;
	uint32_t pixelWidth;
	uint32_t pixelHeight;
	uint32_t pixelDepth;
	uint32_t numberOfArrayElements;
	uint32_t numberOfFaces;
	uint32_t numberOfMipmapLevels;
	uint32_t bytesOfKeyValueData;
} KTX_Header;
ASSERT_STRUCT(KTX_Header, 64);

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_KTX_STRUCTS_H__ */
