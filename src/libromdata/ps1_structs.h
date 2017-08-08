/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ps1_structs.h: Sony PlayStation data structures.                        *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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

// References:
// - http://www.psdevwiki.com/ps3/Game_Saves#Game_Saves_PS1
// - http://problemkaputt.de/psx-spx.htm#memorycarddataformat

#ifndef __ROMPROPERTIES_LIBROMDATA_PS1_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_PS1_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

/**
 * 54-byte header used by some standalone saves
 */
typedef struct PACKED _PS1_54_Header {
	char filename[21];	// Filename from BlockEntry->filename
	char title[33];		// Title from SC->title converted to ASCII
} PS1_54_Header;
ASSERT_STRUCT(PS1_54_Header, 54);

typedef enum {
	PS1_ENTRY_ALLOC_FIRST         = 0x51, // First or only block of a file
	PS1_ENTRY_ALLOC_MID           = 0x52, // Middle blocks of a file (if 3 or more blocks)
	PS1_ENTRY_ALLOC_LAST          = 0x53, // Last block of a file (if 2 or more blocks)
	PS1_ENTRY_ALLOC_FREE          = 0xA0, // Freshly formatted
	PS1_ENTRY_ALLOC_DELETED_FIRST = 0xA1, // Deleted (first)
	PS1_ENTRY_ALLOC_DELETED_MID   = 0xA2, // Deleted (middle)
	PS1_ENTRY_ALLOC_DELETED_LAST  = 0xA3, // Deleted (last)
} PS1_Entry_Alloc_Flag;

/**
 * Block Entry. Stored in Block 0 of memorycard.
 * Also used as a header for some standalone saves.
 */
typedef struct PACKED _PS1_Block_Entry {
	uint32_t alloc_flag;	// Type
	uint32_t filesize;	// Filesize
	uint16_t next_block;	// Pointer to next block (0xFFFF = EOF)
	char filename[21];	// BxSxxS-xxxxxyyyyyyyy
	uint8_t padding[96];
	uint8_t checksum;
} PS1_Block_Entry;
ASSERT_STRUCT(PS1_Block_Entry, 128);

/**
 * "SC" icon display flag.
 */
typedef enum {
	PS1_SC_ICON_NONE	= 0x00,	// No icon.
	PS1_SC_ICON_STATIC	= 0x11,	// One frame.
	PS1_SC_ICON_ANIM_2	= 0x12,	// Two frames.
	PS1_SC_ICON_ANIM_3	= 0x13,	// Three frames.

	// Alternate values.
	PS1_SC_ICON_ALT_STATIC	= 0x16,	// One frame.
	PS1_SC_ICON_ALT_ANIM_2	= 0x17,	// Two frames.
	PS1_SC_ICON_ALT_ANIM_3	= 0x18,	// Three frames.
} PS1_SC_Icon_Flag;

/**
 * "SC" magic struct.
 * Found at 0x84 in PSV save files.
 *
 * All fields are little-endian.
 * NOTE: Strings are NOT null-terminated!
 */
#define PS1_SC_MAGIC "SC"
typedef struct PACKED _PS1_SC_Struct {
	char magic[2];		// Magic. ("SC")
	uint8_t icon_flag;	// Icon display flag.
	uint8_t blocks;		// Number of PS1 blocks per save file.
	char title[64];		// Save data title. (Shift-JIS)

	uint8_t reserved1[12];

	// PocketStation.
	uint16_t pocket_mcicon;	// Number of PokcetStation MCicon frames.
	char pocket_magic[4];	// PocketStation magic. ("MCX0", "MCX1", "CRD0")
	uint16_t pocket_apicon;	// Number of PocketStation APicon frames.

	uint8_t reserved2[8];

	// PlayStation icon.
	// NOTE: A palette entry of $0000 is transparent.
	uint16_t icon_pal[16];		// Icon palette. (RGB555)
	uint8_t icon_data[3][16*16/2];	// Icon data. (16x16, 4bpp; up to 3 frames)
} PS1_SC_Struct;
ASSERT_STRUCT(PS1_SC_Struct, 512);

/**
 * PSV save format. (PS1 on PS3)
 *
 * All fields are little-endian.
 * NOTE: Strings are NOT null-terminated!
 */
typedef struct PACKED _PS1_PSV_Header {
	char magic[8];		// Magic. ("\0VSP\0\0\0\0")
	uint8_t key_seed[20];	// Key seed.
	uint8_t sha1_hmac[20];	// SHA1 HMAC digest.

	// 0x30
	uint8_t reserved1[8];
	uint8_t reserved2[8];	// 14 00 00 00 01 00 00 00

	// 0x40
	uint32_t size;			// Size displayed on XMB.
	uint32_t data_block_offset;	// Offset of Data Block 1. (PS1_SC_Struct)
	uint32_t unknown1;		// 00 02 00 00

	// 0x5C
	uint8_t reserved3[16];
	uint32_t unknown2;		// 00 20 00 00
	uint32_t unknown3;		// 03 90 00 00

	char filename[20];	// Filename. (filename[6] == 'P' for PocketStation)

	uint8_t reserved4[12];
} PS1_PSV_Header;
ASSERT_STRUCT(PS1_PSV_Header, 0x84);

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_PS1_STRUCTS_H__ */
