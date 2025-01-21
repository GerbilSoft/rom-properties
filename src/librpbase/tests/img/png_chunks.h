/***************************************************************************
 * ROM Properties Page shell extension. (librpbase/tests)                  *
 * png_chunks.h: PNG chunk definitions.                                    *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"
#include <stdint.h>

#include "librpbase/config.librpbase.h"

#ifndef HAVE_PNG
// libpng isn't available, so we have to define
// various libpng constants here.
// (Copied from libpng-1.6.21's png.h.)

/* These describe the color_type field in png_info. */
/* color type masks */
#define PNG_COLOR_MASK_PALETTE    1
#define PNG_COLOR_MASK_COLOR      2
#define PNG_COLOR_MASK_ALPHA      4

/* color types.  Note that not all combinations are legal */
#define PNG_COLOR_TYPE_GRAY 0
#define PNG_COLOR_TYPE_PALETTE  (PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_PALETTE)
#define PNG_COLOR_TYPE_RGB        (PNG_COLOR_MASK_COLOR)
#define PNG_COLOR_TYPE_RGB_ALPHA  (PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_ALPHA)
#define PNG_COLOR_TYPE_GRAY_ALPHA (PNG_COLOR_MASK_ALPHA)
/* aliases */
#define PNG_COLOR_TYPE_RGBA  PNG_COLOR_TYPE_RGB_ALPHA
#define PNG_COLOR_TYPE_GA  PNG_COLOR_TYPE_GRAY_ALPHA

#endif /* HAVE_PNG */

// PNG magic.
static const uint8_t PNG_magic[8] = {0x89,'P','N','G','\r','\n',0x1A,'\n'};

// PNG IHDR struct.
static const char PNG_IHDR_name[4] = {'I','H','D','R'};
#pragma pack(1)
typedef struct RP_PACKED _PNG_IHDR_t {
	uint32_t width;		// BE32
	uint32_t height;	// BE32
	uint8_t bit_depth;
	uint8_t color_type;
	uint8_t compression_method;
	uint8_t filter_method;
	uint8_t interlace_method;
} PNG_IHDR_t;
ASSERT_STRUCT(PNG_IHDR_t, 13);
#pragma pack()

// PNG IHDR struct, with length, name, and CRC32.
#pragma pack(1)
typedef struct RP_PACKED _PNG_IHDR_full_t {
	uint32_t chunk_size;	// BE32
	char chunk_name[4];	// "IHDR"
	PNG_IHDR_t data;
	uint32_t crc32;
} PNG_IHDR_full_t;
ASSERT_STRUCT(PNG_IHDR_full_t, 25);
#pragma pack()
