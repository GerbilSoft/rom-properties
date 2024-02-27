/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * dpf_structs.h: GameCube/Wii DPF/RPF structs.                            *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// DPF/RPF is a sparse format used by the official GameCube and Wii SDKs.
// These structs were identified using reverse-engineering of existing DPF and RPF files.

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * DPF/RPF header.
 *
 * All fields are in little-endian.
 */
#define DPF_MAGIC 0x23FC3E86
#define RPF_MAGIC 0xE0F92B6A
typedef struct _DpfHeader {
	uint32_t magic;			// [0x000] DPF/RPF magic number
	uint32_t version;		// [0x004] Version, usually 0
	uint32_t header_size;		// [0x008] Size of this header (usually 32, 0x20)
	uint32_t unknown_0C;		// [0x00C] Unknown, usually 0
	uint32_t entry_table_offset;	// [0x010] Offset to the entry table (usually 32, 0x20)
	uint32_t entry_count;		// [0x014] Number of sparse table entries
	uint32_t data_offset;		// [0x018] Offset to the beginning of the actual data
	uint32_t unknown_1C;		// [0x01C] Unknown (unless data_offset is actually 64-bit?)
} DpfHeader;
ASSERT_STRUCT(DpfHeader, 32);

/**
 * DPF entry.
 *
 * All fields are in little-endian.
 */
typedef struct _DpfEntry {
	uint32_t virt_offset;		// [0x000] Virtual offset inside the logical disc image
	uint32_t phys_offset;		// [0x004] Physical offset inside the sparse file
	uint32_t size;			// [0x008] Size of this block, in bytes
	uint32_t unknown_0C;		// [0x00C] Unknown (usually 0; may be 1 for zero-length blocks?)
} DpfEntry;
ASSERT_STRUCT(DpfEntry, 16);

/**
 * RPF entry.
 *
 * All fields are in little-endian.
 */
typedef struct _RpfEntry {
	uint64_t virt_offset;		// [0x000] Virtual offset inside the logical disc image
	uint64_t phys_offset;		// [0x008] Physical offset inside the sparse file
	uint32_t size;			// [0x010] Size of this block, in bytes
	uint32_t unknown_14;		// [0x014] Unknown (usually 0; may be 1 for zero-length blocks?)
} RpfEntry;
ASSERT_STRUCT(RpfEntry, 24);

#ifdef __cplusplus
}
#endif /* __cplusplus */
