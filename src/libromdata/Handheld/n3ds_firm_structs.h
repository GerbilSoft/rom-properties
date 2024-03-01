/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * n3ds_firm_structs.h: Nintendo 3DS firmware data structures.             *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - https://3dbrew.org/wiki/FIRM

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Nintendo 3DS firmware section header struct.
 * All fields are little-endian.
 */
typedef struct _N3DS_FIRM_Section_Header_t {
	uint32_t offset;		// [0x000] Byte offset
	uint32_t load_addr;		// [0x004] Physical address where the section is loaded to
	uint32_t size;			// [0x008] Byte size (If 0, section does not exist)
	uint32_t copy_method;		// [0x00C] 0 = NDMA, 1 = XDMA, 2 = CPU memcpy()
	uint8_t sha256[32];		// [0x010] SHA-256 of the previous fields
} N3DS_FIRM_Section_Header_t;
ASSERT_STRUCT(_N3DS_FIRM_Section_Header_t, 48);

/**
 * Nintendo 3DS firmware binary header struct.
 *
 * All fields are little-endian,
 * except for the magic number.
 */
#define N3DS_FIRM_MAGIC 'FIRM'
typedef struct _N3DS_FIRM_Header_t {
	uint32_t magic;				// [0x000] 'FIRM' (big-endian)
	uint32_t boot_priority;			// [0x004] Normally 0 (highest value = max prio)
	uint32_t arm11_entrypoint;		// [0x008] Non-zero for FIRM; zero for Boot9Strap payloads
	uint32_t arm9_entrypoint;		// [0x00C]
	uint8_t reserved[0x30];			// [0x010]
	N3DS_FIRM_Section_Header_t sections[4];	// [0x040] Firmware section headers
	union {
		uint8_t signature[0x100];	// [0x100] RSA-2048 signature (uint8_t version)
		uint32_t signature32[0x100/4];	// [0x100] RSA-2048 signature (uint32_t version)
	};
} N3DS_FIRM_Header_t;
ASSERT_STRUCT(_N3DS_FIRM_Header_t, 512);

#ifdef __cplusplus
}
#endif
