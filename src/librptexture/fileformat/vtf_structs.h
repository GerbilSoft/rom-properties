/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * vtf_structs.h: Valve VTF texture format data structures.                *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Valve VTF: File header.
 * Reference: https://developer.valvesoftware.com/wiki/Valve_Texture_Format
 *
 * All fields are little-endian,
 * except for the magic number.
 */
#define VTF_SIGNATURE 'VTF\0'
#define VTF_VERSION_MAJOR 7
#define VTF_VERSION_MINOR 2
#pragma pack(1)
typedef struct RP_PACKED _VTFHEADER {
	uint32_t signature;		// 'VTF\0' (big-endian)
	uint32_t version[2];		// Version number. (current version is 7.2)
	uint32_t headerSize;		// Header size (16-byte aligned)
					// For 7.3, includes size of resources dictionary.
	uint16_t width;			// [0x010] Width of largest mipmap. (must be a power of 2)
	uint16_t height;		// [0x012] Height of largest mipmap. (must be a power of 2)
	uint32_t flags;			// [0x014]

	uint16_t frames;		// [0x018] Number of frames, if animated. (1 for no animation.
	uint16_t firstFrame;		// [0x01A] First frame in animation. (0-based)

	uint8_t padding0[4];		// [0x01C] reflectivity padding (16-byte alignment)
	float reflectivity[3];		// [0x020] reflectivity vector
	uint8_t padding1[4];		// [0x02C] reflectivity padding (8-byte packing)

	float bumpmapScale;		// [0x030] Bumpmap scale.
	int highResImageFormat;		// [0x034] High resolution image format.
	uint8_t mipmapCount;		// [0x038] Number of mipmaps.
	int lowResImageFormat;		// [0x039] Low resolution image format. (usually DXT1; -1 for none)
	uint8_t lowResImageWidth;	// [0x03D] Low resolution image width.
	uint8_t lowResImageHeight;	// [0x03E] Low resolution image height.

	// 7.2+
	uint16_t depth;			// [0x03F] Depth of largest mipmap. Must be a power of 2.
					// Can be 0 or 1 for a 2D texture.

	// 7.3+
	uint8_t padding2[3];		// [0x041] depth padding (4-byte alignment)
	uint32_t numResources;		// [0x044] Number of resources this VTF has.
} VTFHEADER;
// FIXME: Not sure if 72 is correct.
ASSERT_STRUCT(VTFHEADER, 72);
#pragma pack()

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
	VTF_IMAGE_FORMAT_BGRx8888,
	VTF_IMAGE_FORMAT_BGR565,
	VTF_IMAGE_FORMAT_BGRx5551,
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

#ifdef __cplusplus
}
#endif
