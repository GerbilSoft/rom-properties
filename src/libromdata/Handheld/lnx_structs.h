/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * lnx_structs.h: Atari Lynx data structures.                              *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
 * Copyright (c) 2017 by Egor.                                             *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_LNX_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_LNX_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

/**
 * Atari Lynx ROM header.
 * This matches the ROM header format exactly.
 * Reference:
 * - http://handy.cvs.sourceforge.net/viewvc/handy/win32src/public/handybug/dvreadme.txt
 *
 * All fields are little-endian.
 * NOTE: Strings are NOT null-terminated!
 */
typedef struct PACKED _Lynx_RomHeader {
	char magic[4]; // "LYNX"
	uint16_t page_size_bank0;
	uint16_t page_size_bank1;
	uint16_t version;
	char cartname[32];
	char manufname[16];
	uint8_t rotation; // 0 - none, 1 - left, 2 - right
	uint8_t spare[5]; // padding
} Lynx_RomHeader;
ASSERT_STRUCT(Lynx_RomHeader, 64);

// Rotation values.
typedef enum {
	LYNX_ROTATE_NONE	= 0,
	LYNX_ROTATE_LEFT	= 1,
	LYNX_ROTATE_RIGHT	= 2,
} Lynx_Rotation;

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_LNX_STRUCTS_H__ */
