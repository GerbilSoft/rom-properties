/***************************************************************************
 * ROM Properties Page shell extension. (amiibo-data)                      *
 * amiibo_bin_structs.h: Nintendo amiibo binary data structs.              *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * amiibo binary database file header.
 * This file format is specific to rom-properties.
 *
 * All offsets are absolute offsets. (relative to the start of the file)
 *
 * All fields are little-endian.
 */
#define AMIIBO_BIN_MAGIC "RPNFPB10"
typedef struct _AmiiboBinHeader {
	char magic[8];          // [0x000] "RPNFPB10"
	uint32_t strtbl_offset; // [0x008] String table
	uint32_t strtbl_len;    // [0x00C]

	// Page 21 (characters)
	uint32_t cseries_offset; // [0x010] Series table
	uint32_t cseries_len;    // [0x014]
	uint32_t char_offset;    // [0x018] Character table
	uint32_t char_len;       // [0x01C]
	uint32_t cvar_offset;    // [0x020] Character variant table
	uint32_t cvar_len;       // [0x024]

	// Page 22 (amiibos)
	uint32_t aseries_offset; // [0x028] amiibo series table
	uint32_t aseries_len;    // [0x02C]
	uint32_t amiibo_offset;  // [0x030] amiibo ID table
	uint32_t amiibo_len;     // [0x034]

	// Reserved
	uint32_t reserved[18]; // [0x038]
} AmiiboBinHeader;
ASSERT_STRUCT(AmiiboBinHeader, 0x080);

/**
 * Character table entry. (p.21)
 *
 * If bit 31 of char_id is set, character variants are present.
 * The character variant table should be checked.
 *
 * All fields are little-endian.
 */
#define CHARTABLE_VARIANT_FLAG (1U << 31)
typedef struct _CharTableEntry {
	uint32_t char_id; // Character ID (low 16 bits are significant)
	uint32_t name;    // Character name (string table offset)
} CharTableEntry;
ASSERT_STRUCT(CharTableEntry, 2 * sizeof(uint32_t));

/**
 * Character table entry. (p.21)
 *
 * All fields are little-endian.
 */
typedef struct _CharVariantTableEntry {
	uint16_t char_id; // Character ID
	uint8_t var_id;   // Variant ID
	uint8_t reserved;
	uint32_t name; // Character variant name (string table)
} CharVariantTableEntry;
ASSERT_STRUCT(CharVariantTableEntry, 2 * sizeof(uint32_t));

/**
 * amiibo ID table entry. (p.22)
 *
 * All fields are little-endian.
 */
typedef struct _AmiiboIDTableEntry {
	uint16_t release_no; // Release number
	uint8_t wave_no;     // Wave number
	uint8_t reserved;
	uint32_t name; // amiibo name (string table)
} AmiiboIDTableEntry;
ASSERT_STRUCT(AmiiboIDTableEntry, 2 * sizeof(uint32_t));

#ifdef __cplusplus
}
#endif
