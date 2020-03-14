/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * lnx_structs.h: Atari Lynx data structures.                              *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * Copyright (c) 2017 by Egor.                                             *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_LNX_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_LNX_STRUCTS_H__

#include <stdint.h>
#include "common.h"

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
 * All fields are little-endian,
 * except for the magic number.
 *
 * NOTE: Strings are NOT null-terminated!
 */
#define LYNX_MAGIC 'LYNX'
typedef struct PACKED _Lynx_RomHeader {
	uint32_t magic;			// [0x000] 'LYNX' (big-endian)
	uint16_t page_size_bank0;	// [0x004]
	uint16_t page_size_bank1;	// [0x006]
	uint16_t version;		// [0x008]
	char cartname[32];		// [0x00A]
	char manufname[16];		// [0x02A]
	uint8_t rotation;		// [0x03A] // 0 - none, 1 - left, 2 - right
	uint8_t spare[5];		// [0x03B] // padding
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
