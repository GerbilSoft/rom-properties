/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * dds_structs.h: DirectDraw Surface texture format data structures.       *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_DDS_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_DDS_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * References:
 * - https://msdn.microsoft.com/en-us/library/windows/desktop/bb943990(v=vs.85).aspx
 * - https://msdn.microsoft.com/en-us/library/windows/desktop/bb943992(v=vs.85).aspx
 * - https://msdn.microsoft.com/en-us/library/windows/desktop/bb943982(v=vs.85).aspx (DDS_HEADER)
 * - https://msdn.microsoft.com/en-us/library/windows/desktop/bb943983(v=vs.85).aspx (DDS_HEADER_DX10)
 * - https://msdn.microsoft.com/en-us/library/windows/desktop/bb943984(v=vs.85).aspx (DDS_PIXELFORMAT)
 */

// NOTE: This header may conflict with the official DirectX SDK.

/**
 * DirectDraw Surface: Pixel format.
 * Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/bb943984(v=vs.85).aspx
 *
 * All fields are in little-endian.
 */
#pragma pack(1)
typedef struct PACKED _DDS_PIXELFORMAT {
	uint32_t dwSize;
	uint32_t dwFlags;		// See DDS_PIXELFORMAT_FLAGS
	uint32_t dwFourCC;		// See DDS_PIXELFORMAT_FOURCC
	uint32_t dwRGBBitCount;
	uint32_t dwRBitMask;
	uint32_t dwGBitMask;
	uint32_t dwBBitMask;
	uint32_t dwABitMask;
} DDS_PIXELFORMAT;
#pragma pack()
ASSERT_STRUCT(DDS_PIXELFORMAT, 32);

// dwFlags
typedef enum {
	DDPF_ALPHAPIXELS	= 0x1,
	DDPF_ALPHA		= 0x2,
	DDPF_FOURCC		= 0x4,
	DDPF_RGB		= 0x40,
	DDPF_YUV		= 0x200,
	DDPF_LUMINANCE		= 0x20000,
} DDS_PIXELFORMAT_FLAGS;

// dwFourCC
typedef enum {
	DDPF_FOURCC_DXT1	= 0x31545844,	// "DXT1"
	DDPF_FOURCC_DXT2	= 0x32545844,	// "DXT2"
	DDPF_FOURCC_DXT3	= 0x33545844,	// "DXT3"
	DDPF_FOURCC_DXT4	= 0x34545844,	// "DXT4"
	DDPF_FOURCC_DXT5	= 0x35545844,	// "DXT5"
	DDPF_FOURCC_ATI1	= 0x31495441,	// "ATI1"
	DDPF_FOURCC_ATI2	= 0x32495441,	// "ATI2"
	DDPF_FOURCC_DX10	= 0x30315844,	// "DX10"
} DDS_PIXELFORMAT_FOURCC;

/**
 * DirectDraw Surface: File header.
 * This does NOT include the "DDS " magic.
 * Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/bb943982(v=vs.85).aspx
 *
 * All fields are in little-endian.
 */
#pragma pack(1)
#define DDS_MAGIC "DDS "
typedef struct PACKED _DDS_HEADER {
	uint32_t dwSize;
	uint32_t dwFlags;		// See DDS_HEADER_FLAGS
	uint32_t dwHeight;
	uint32_t dwWidth;
	uint32_t dwPitchOrLinearSize;
	uint32_t dwDepth;
	uint32_t dwMipMapCount;
	uint32_t dwReserved1[11];
	DDS_PIXELFORMAT ddspf;
	uint32_t dwCaps;
	uint32_t dwCaps2;
	uint32_t dwCaps3;
	uint32_t dwCaps4;
	uint32_t dwReserved2;
} DDS_HEADER;
#pragma pack()
ASSERT_STRUCT(DDS_HEADER, 124);

// dwFlags
typedef enum {
	DDSD_CAPS		= 0x1,
	DDSD_HEIGHT		= 0x2,
	DDSD_WIDTH		= 0x4,
	DDSD_PITCH		= 0x8,
	DDSD_PIXELFORMAT	= 0x1000,
	DDSD_MIPMAPCOUNT	= 0x20000,
	DDSD_LINEARSIZE		= 0x80000,
	DDSD_DEPTH		= 0x800000,
} DDS_HEADER_FLAGS;

// dwCaps
typedef enum {
	DDSCAPS_COMPLEX		= 0x8,
	DDSCAPS_MIPMAP		= 0x400000,
	DDSCAPS_TEXTURE		= 0x1000,
} DDS_HEADER_CAPS;

// dwCaps2
typedef enum {
	DDSCAPS2_CUBEMAP		= 0x200,
	DDSCAPS2_CUBEMAP_POSITIVEX	= 0x400,
	DDSCAPS2_CUBEMAP_NEGATIVEX	= 0x800,
	DDSCAPS2_CUBEMAP_POSITIVEY	= 0x1000,
	DDSCAPS2_CUBEMAP_NEGATIVEY	= 0x2000,
	DDSCAPS2_CUBEMAP_POSITIVEZ	= 0x4000,
	DDSCAPS2_CUBEMAP_NEGATIVEZ	= 0x8000,
	DDSCAPS2_VOLUME			= 0x200000,
} DDS_HEADER_CAPS2;

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_DDS_STRUCTS_H__ */
