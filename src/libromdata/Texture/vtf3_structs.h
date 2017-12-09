/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * vtf3_structs.h: Valve VTF3 (PS3) texture format data structures.        *
 *                                                                         *
 * Copyright (c) 2017 by David Korth.                                      *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_VTF3_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_VTF3_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

/**
 * Valve VTF3: File header.
 * Reverse-engineered from Portal (PS3) textures.
 *
 * All fields are in big-endian.
 */
#define VTF3_SIGNATURE 0x56544633	// "VTF3"
#define VTF_VERSION_MAJOR 7
#define VTF_VERSION_MINOR 2
typedef struct PACKED _VTF3HEADER {
	// TODO: Figure out these fields:
	// - Image format.
	// - Number of mipmaps.

	uint32_t signature;		// [0x000] VTF3_SIGNATURE
	uint8_t unknown1[12];		// [0x004]

	uint32_t flags;			// [0x010] See VTF3_FLAGS.
	uint16_t width;			// [0x014] Width of largest mipmap. (must be a power of 2)
	uint16_t height;		// [0x016] Height of largest mipmap. (must be a power of 2)
	uint8_t unknown2[8];		// [0x018]
} VTF3HEADER;
// FIXME: Not sure if 32 is correct.
ASSERT_STRUCT(VTF3HEADER, 32);

/**
 * Flags
 */
typedef enum {
	VTF3_FLAG_0x0080	= 0x0080,	// TODO: What does this do?
	VTF3_FLAG_ALPHA		= 0x2000,	// If set, has alpha (DXT5).
						// Otherwise, no alpha. (DXT1)
} VTF3_IMAGE_FORMAT;

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_VTF3_STRUCTS_H__ */
