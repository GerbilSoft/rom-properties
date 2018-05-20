/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * wux_structs.h: Wii U .wux format structs.                               *
 *                                                                         *
 * Copyright (c) 2018 by David Korth.                                      *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

// References:
// - https://gbatemp.net/threads/wii-u-image-wud-compression-tool.397901/

#ifndef __ROMPROPERTIES_LIBROMDATA_DISC_WUX_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_DISC_WUX_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#pragma pack(1)

// Magic numbers are stored in little-endian format.
#define WUX_MAGIC_0	'0XUW'
#define WUX_MAGIC_1	0x1099D02E

// 256 bytes minimum block size
// 128 MB maximum block size
// The original tool only uses 32,768 bytes.
#define WUX_BLOCK_SIZE_MIN (256)
#define WUX_BLOCK_SIZE_MAX (128*1024*1024)

/**
 * .wux disc header.
 *
 * All fields are in little-endian.
 */
typedef struct PACKED _wuxHeader_t {
	uint32_t magic[2];		// [0x000] 'WUX0', 0x2ED09910
	uint32_t sectorSize;		// [0x008] Range: [256, 256*1024*1024); must be pow2
	uint32_t reserved1;		// [0x00C]
	uint64_t uncompressedSize;	// [0x014] Total size of the uncompressed disc.
	uint32_t flags;			// [0x018] Currently unused.
	uint32_t reserved2;		// [0x01C]
} wuxHeader_t;
ASSERT_STRUCT(wuxHeader_t, 32);

#pragma pack()

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_WUX_STRUCTS_H__ */
