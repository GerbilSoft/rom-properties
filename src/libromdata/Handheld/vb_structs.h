/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * dmg_structs.h: Virtual Boy (DMG/CGB/SGB) data structures.               *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * Copyright (c) 2016 by Egor.                                             *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_VB_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_VB_STRUCTS_H__

#include <stdint.h>
#include "common.h"

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
