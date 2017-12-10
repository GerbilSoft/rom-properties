/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * dmg_structs.h: Virtual Boy (DMG/CGB/SGB) data structures.               *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
 * Copyright (c) 2016 by Egor.                                             *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_VB_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_VB_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

/**
 * Virtual Boy ROM header.
 * This matches the ROM header format exactly.
 * References:
 * - http://www.goliathindustries.com/vb/download/vbprog.pdf page 9
 * 
 * NOTE: Strings are NOT null-terminated!
 */
typedef struct PACKED _VB_RomHeader {
	char title[21];
	uint8_t reserved[4];
	char publisher[2];
	char gameid[4];
	uint8_t version;
} VB_RomHeader;
ASSERT_STRUCT(VB_RomHeader, 32);

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_VB_STRUCTS_H__ */
