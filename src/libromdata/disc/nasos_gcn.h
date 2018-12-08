/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * nasos_gcn.h: GameCube/Wii NASOS (.iso.dec) disc image reader.           *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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

// NOTE: This is reverse-engineered, and most fields are unknown.

#ifndef __ROMPROPERTIES_LIBROMDATA_DISC_NASOS_GCN_H__
#define __ROMPROPERTIES_LIBROMDATA_DISC_NASOS_GCN_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#pragma pack(1)

/**
 * .iso.dec header.
 *
 * All fields are in little-endian, except for the
 * magic number, which is considered "big-endian".
 */
#define NASOS_MAGIC_GCML 'GCML'
#define NASOS_MAGIC_WII5 'WII5'
typedef struct PACKED _NASOSHeader {
	uint32_t magic;		// [0x000] Magic number. ('GCML', 'WII5')
	char id4[4];		// [0x004] ID4 of the disc image.
} NASOSHeader;
ASSERT_STRUCT(NASOSHeader, 8);

/**
 * .iso.dec header. (with 'GCML' fields)
 *
 * Block size is 2,048 bytes. (0x800)
 * Block count is 0xAE0B0. (712,880)
 *
 * All fields are in little-endian, except for the
 * magic number, which is considered "big-endian".
 */
#define NASOS_GCML_BlockCount 712880
typedef struct _NASOSHeader_GCML {
	NASOSHeader header;	// [0x000] Main NASOS header.
	uint8_t unknown1[16];	// [0x008]
} NASOSHeader_GCML;
ASSERT_STRUCT(NASOSHeader_GCML, 24);

/**
 * .iso.dec header. (with 'WII5' fields)
 *
 * Block size is 1,024 bytes. (0x400)
 *
 * All fields are in little-endian, except for the
 * magic number, which is considered "big-endian".
 */

typedef struct _NASOSHeader_WII5 {
	NASOSHeader header;	// [0x000] Main NASOS header.
	uint8_t unknown1[64];	// [0x008]
	uint32_t block_count;	// [0x048] Block count. (divide by 256)
	uint8_t unknown2[16];	// [0x04C]
} NASOSHeader_WII5;
ASSERT_STRUCT(NASOSHeader_WII5, 0x5C);

#pragma pack()

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_NASOS_GCN_H__ */
