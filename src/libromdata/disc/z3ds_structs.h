/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * z3ds_structs.h: Nintendo 3DS Z3DS structs.                              *
 *                                                                         *
 * Copyright (c) 2025 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Z3DS header.
 *
 * All fields are in little-endian, except for
 * 'magic' and 'underlying_magic'.
 */
#define Z3DS_MAGIC 'Z3DS'
#define Z3DS_VERSION 1
typedef struct _Z3DS_Header {
	uint32_t magic;			// [0x000] Z3DS magic number ('Z3DS')
	uint32_t underlying_magic;	// [0x004] Magic number of the compressed data
	uint8_t version;		// [0x008] Z3DS version (currently 1)
	uint8_t reserved_0x09;		// [0x009] Reserved (should be 0)
	uint16_t header_size;		// [0x00A] Header size (should be 32)
	uint32_t metadata_size;		// [0x00C] Metadata size, immediately after the header
					//         (must be 16-byte aligned)
	uint64_t compressed_size;	// [0x010] Size of ROM data, compressed
	uint64_t uncompressed_size;	// [0x018] Size of ROM data, uncompressed
} Z3DS_Header;
ASSERT_STRUCT(Z3DS_Header, 32);

/**
 * Z3DS metadata: Item header.
 * NOTE: Metadata is *not* aligned in the file.
 *
 * All fields are in little-endian.
 */
#define Z3DS_METADATA_VERSION 0x01
#pragma pack(1)
typedef struct RP_PACKED _Z3DS_Metadata_Item_Header {
	uint8_t type;		// [0x000] Type (see Z3DS_Metadata_Item_Type_e)
	uint8_t key_len;	// [0x001] Key length (UTF-8, no NUL terminator, in bytes)
	uint16_t value_len;	// [0x002] Value length (in bytes)
} Z3DS_Metadata_Item_Header;
ASSERT_STRUCT(Z3DS_Metadata_Item_Header, 4);
#pragma pack()

/**
 * Z3DS metadata: Types
 */
typedef enum {
	Z3DS_METADATA_ITEM_TYPE_END	= 0x00,	// End of metadata items
	Z3DS_METADATA_ITEM_TYPE_BINARY	= 0x01,	// Generic binary data
} Z3DS_Metadata_Item_Type_e;

#ifdef __cplusplus
}
#endif /* __cplusplus */
