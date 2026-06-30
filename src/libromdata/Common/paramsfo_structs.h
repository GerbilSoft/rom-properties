/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * paramsfo_structs.h: PlayStation PARAM.SFO / PSF file data structures    *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * Copyright (c) 2026 by Emma / InvoxiPlayGames.                           *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _psf_header {
	uint32_t magic;
	uint32_t version;
	uint32_t keyOffset;
	uint32_t dataOffset;
	int numKeys;
} psf_header_t;
ASSERT_STRUCT(psf_header_t, 0x14);
#define PSF_MAGIC	0x00505346 // '\0PSF'
#define PSF_VERSION 0x01010000 // v1.1

typedef enum _psf_type {
	kPSF_UTF8 = 0x0204,
	kPSF_Int32 = 0x0404,
} psf_type_e;

typedef struct _psf_key {
	uint16_t keyNameOffset;
	uint16_t valueType;
	int dataLength;
	int dataMax;
	uint32_t dataOffset;
} psf_key_t;
ASSERT_STRUCT(psf_key_t, 0x10);

#ifdef __cplusplus
}
#endif
