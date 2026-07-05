/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * paramsfo_structs.h: PlayStation PARAM.SFO / PSF file data structures    *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * Copyright (c) 2026 by Emma / InvoxiPlayGames.                           *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - https://www.psdevwiki.com/psp/PARAM.SFO
// - https://www.psdevwiki.com/ps3/PARAM.SFO
// - https://www.psdevwiki.com/vita/PARAM.SFO
// - https://www.psdevwiki.com/ps4/Param.sfo

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * PSF file header
 *
 * All fields are in little-endian.
 */
#define PSF_MAGIC	0x00505346 // '\0PSF'
#define PSF_VERSION	0x01010000 // v1.1
typedef struct _psf_header {
	uint32_t magic;		// [0x000] '\0PSF'
	uint32_t version;	// [0x004] 0x01010000
	uint32_t keyOffset;	// [0x008] Offset to the key name table
	uint32_t dataOffset;	// [0x00C] Offset to the data table
	int numKeys;		// [0x010] Number of keys (key table immediately follows this header)
} psf_header_t;
ASSERT_STRUCT(psf_header_t, 0x14);

/**
 * PSF key struct
 *
 * All fields are in little-endian.
 */
typedef struct _psf_key {
	uint16_t keyNameOffset;	// [0x000] Key name offset (in the key name table)
	uint16_t valueType;	// [0x002] Key type (see psf_type_e)
	int dataLength;		// [0x004] Used value length
	int dataMax;		// [0x008] Maximum value length
	uint32_t dataOffset;	// [0x00C] Data offset (in the data table)
} psf_key_t;
ASSERT_STRUCT(psf_key_t, 0x10);

/**
 * PSF value types
 */
typedef enum {
	kPSF_UTF8S = 0x0004,	// UTF-8 "Special Mode", *not* NULL-terminated
	kPSF_UTF8 = 0x0204,	// UTF-8, NULL-terminated
	kPSF_Int32 = 0x0404,	// int32 (or uint32?)
} psf_type_e;

#ifdef __cplusplus
}
#endif
