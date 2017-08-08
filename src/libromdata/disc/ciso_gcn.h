/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ciso_gcn.h: GameCube/Wii CISO structs.                                  *
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

// References:
// - https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/DiscIO/CISOBlob.cpp
// - https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/DiscIO/CISOBlob.h

#ifndef __ROMPROPERTIES_LIBROMDATA_DISC_CISO_GCN_H__
#define __ROMPROPERTIES_LIBROMDATA_DISC_CISO_GCN_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#pragma pack(1)

#define CISO_HEADER_SIZE 0x8000
#define CISO_MAP_SIZE (CISO_HEADER_SIZE - sizeof(uint32_t) - (sizeof(char) * 4))

// 32 KB minimum block size (GCN/Wii sector)
// 16 MB maximum block size
#define CISO_BLOCK_SIZE_MIN (32768)
#define CISO_BLOCK_SIZE_MAX (16*1024*1024)

typedef struct PACKED _CISOHeader {
	char magic[4];			// "CISO"
	uint32_t block_size;		// LE32
	uint8_t map[CISO_MAP_SIZE];	// 0 == unused; 1 == used; other == invalid
} CISOHeader;

#pragma pack()

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_CISO_GCN_H__ */
