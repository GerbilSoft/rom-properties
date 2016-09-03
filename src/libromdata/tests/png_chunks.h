/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * png_chunks.h: PNG chunk definitions.                                    *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_TESTS_PNG_CHUNKS_H__
#define __ROMPROPERTIES_LIBROMDATA_TESTS_PNG_CHUNKS_H__

#include "common.h"
#include <png.h>
#include <stdint.h>

// PNG magic.
static const uint8_t PNG_magic[8] = {0x89,'P','N','G','\r','\n',0x1A,'\n'};

// PNG IHDR struct.
#define PNG_IHDR_t_SIZE 13
static const char PNG_IHDR_name[4] = {'I','H','D','R'};
#pragma pack(1)
typedef struct PACKED _PNG_IHDR_t {
	uint32_t width;		// BE32
	uint32_t height;	// BE32
	uint8_t bit_depth;
	uint8_t color_type;
	uint8_t compression_method;
	uint8_t filter_method;
	uint8_t interlace_method;
} PNG_IHDR_t;
#pragma pack()

// PNG IHDR struct, with length, name, and CRC32.
#define PNG_IHDR_full_t_SIZE (PNG_IHDR_t_SIZE+12)
#pragma pack(1)
typedef struct PACKED _PNG_IHDR_full_t {
	uint32_t chunk_size;	// BE32
	char chunk_name[4];	// "IHDR"
	PNG_IHDR_t data;
	uint32_t crc32;
} PNG_IHDR_full_t;
#pragma pack(0)

#endif /* __ROMPROPERTIES_LIBROMDATA_TESTS_PNG_CHUNKS_H__ */
