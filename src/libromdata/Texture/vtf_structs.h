/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * vtf_structs.h: Valve VTF texture format data structures.                *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_VTF_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_VTF_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

/**
 * Valve VTF: File header.
 * Reference: https://developer.valvesoftware.com/wiki/Valve_Texture_Format
 *
 * All fields are in little-endian.
 */
#define VTF_SIGNATURE 0x00465456	// "VTF\0"
#define VTF_VERSION_MAJOR 7
#define VTF_VERSION_MINOR 2
typedef struct PACKED _VTFHEADER {
	uint32_t signature;		// VTF_SIGNATURE
	uint32_t version[2];		// Version number. (current version is 7.2)
	uint32_t headerSize;		// Header size (16-byte aligned)
					// For 7.3, includes size of resources dictionary.
	uint16_t width;			// Width of largest mipmap. (must be a power of 2)
	uint16_t height;		// Height of largest mipmap. (must be a power of 2)
	uint32_t flags;

	uint16_t frames;		// Number of frames, if animated. (1 for no animation.
	uint16_t firstFrame;		// First frame in animation. (0-based)

	uint8_t padding0[4];		// reflectivity padding (16-byte alignment)
	float reflectivity[3];		// reflectivity vector
	uint8_t padding1[4];		// reflectivity padding (8-byte packing)

	float bumpmapScale;			// Bumpmap scale.
	unsigned int highResImageFormat;	// High resolution image format.
	uint8_t mipmapCount;			// Number of mipmaps.
	unsigned int lowResImageFormat;		// Low resolution image format. (always DXT1)
	uint8_t lowResImageWidth;		// Low resolution image width.
	uint8_t lowResImageHeight;		// Low resolution image height.

	// 7.2+
	uint16_t depth;			// Depth of largest mipmap. Must be a power of 2.
					// Can be 0 or 1 for a 2D texture.

	// 7.3+
	uint8_t padding2[3];		// depth padding (4-byte alignment)
	uint32_t numResources;		// Number of resources this VTF has.
} VTFHEADER;
// FIXME: Not sure if 72 is correct.
ASSERT_STRUCT(VTFHEADER, 72);

/**
 * Image format.
 */
typedef enum {
	VTF_IMAGE_FORMAT_NONE		= -1,
	VTF_IMAGE_FORMAT_RGBA8888	= 0,
	VTF_IMAGE_FORMAT_ABGR8888,
	VTF_IMAGE_FORMAT_RGB888,
	VTF_IMAGE_FORMAT_BGR888,
	VTF_IMAGE_FORMAT_RGB565,
	VTF_IMAGE_FORMAT_I8,
	VTF_IMAGE_FORMAT_IA88,
	VTF_IMAGE_FORMAT_P8,
	VTF_IMAGE_FORMAT_A8,
	VTF_IMAGE_FORMAT_RGB888_BLUESCREEN,
	VTF_IMAGE_FORMAT_BGR888_BLUESCREEN,
	VTF_IMAGE_FORMAT_ARGB8888,
	VTF_IMAGE_FORMAT_BGRA8888,
	VTF_IMAGE_FORMAT_DXT1,
	VTF_IMAGE_FORMAT_DXT3,
	VTF_IMAGE_FORMAT_DXT5,
	VTF_IMAGE_FORMAT_BGRX8888,
	VTF_IMAGE_FORMAT_BGR565,
	VTF_IMAGE_FORMAT_BGRX5551,
	VTF_IMAGE_FORMAT_BGRA4444,
	VTF_IMAGE_FORMAT_DXT1_ONEBITALPHA,
	VTF_IMAGE_FORMAT_BGRA5551,
	VTF_IMAGE_FORMAT_UV88,
	VTF_IMAGE_FORMAT_UVWQ8888,
	VTF_IMAGE_FORMAT_RGBA16161616F,
	VTF_IMAGE_FORMAT_RGBA16161616,
	VTF_IMAGE_FORMAT_UVLX8888,

	VTF_IMAGE_FORMAT_MAX
} VTF_IMAGE_FORMAT;

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_VTF_STRUCTS_H__ */
