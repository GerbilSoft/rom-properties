/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ps1_structs.h: Sony PlayStation data structures.                        *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * Copyright (c) 2017 by Egor.                                             *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - https://www.psdevwiki.com/ps3/PS1_Savedata
// - http://problemkaputt.de/psx-spx.htm#memorycarddataformat

#pragma once

#include "common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 54-byte header used by some standalone saves
 */
// Some compilers pad this structure to a multiple of 4 bytes
#pragma pack(1)
typedef struct RP_PACKED _PS1_54_Header {
	char filename[21];	// Filename from BlockEntry->filename
	char title[33];		// Title from SC->title converted to ASCII
} PS1_54_Header;
ASSERT_STRUCT(PS1_54_Header, 54);
#pragma pack()

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
typedef struct _PS1_Block_Entry {
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
#define PS1_SC_MAGIC 		'SC'
#define PS1_POCKETSTATION_MCX0	'MCX0'
#define PS1_POCKETSTATION_MCX1	'MCX1'
#define PS1_POCKETSTATION_CRD0	'CRD0'
#pragma pack(1)
typedef struct RP_PACKED _PS1_SC_Struct {
	uint16_t magic;		// [0x000] Magic. ("SC")
	uint8_t icon_flag;	// [0x002] Icon display flag.
	uint8_t blocks;		// [0x003] Number of PS1 blocks per save file.
	char title[64];		// [0x004] Save data title. (Shift-JIS)

	uint8_t reserved1[12];	// [0x044]

	// PocketStation.
	uint16_t pocket_mcicon;	// [0x050] Number of PokcetStation MCicon frames.
	uint32_t pocket_magic;	// [0x052] PocketStation magic. ("MCX0", "MCX1", "CRD0")
	uint16_t pocket_apicon;	// [0x056] Number of PocketStation APicon frames.

	uint8_t reserved2[8];	// [0x058]

	// PlayStation icon.
	// NOTE: A palette entry of $0000 is transparent.
	uint16_t icon_pal[16];		// [0x060] Icon palette. (RGB555)
	uint8_t icon_data[3][16*16/2];	// [0x080] Icon data. (16x16, 4bpp; up to 3 frames)
} PS1_SC_Struct;
ASSERT_STRUCT(PS1_SC_Struct, 512);
#pragma pack()

/**
 * PSV save format. (PS1 on PS3)
 *
 * All fields are little-endian.
 * NOTE: Strings are NOT null-terminated!
 */
#define PS1_PSV_MAGIC 0x0056535000000000ULL	// "\0VSP\0\0\0\0"
#pragma pack(1)
typedef struct RP_PACKED _PS1_PSV_Header {
	uint64_t magic;		// [0x000] Magic. ("\0VSP\0\0\0\0")
	uint8_t key_seed[20];	// [0x008] Key seed.
	uint8_t sha1_hmac[20];	// [0x01C] SHA1 HMAC digest.

	// 0x30
	uint8_t reserved1[8];	// [0x030]
	uint8_t reserved2[8];	// [0x038] 14 00 00 00 01 00 00 00

	// 0x40
	uint32_t size;			// [0x040] Size displayed on XMB.
	uint32_t data_block_offset;	// [0x044] Offset of Data Block 1. (PS1_SC_Struct)
	uint32_t unknown1;		// [0x048] 00 02 00 00

	// 0x4C
	uint8_t reserved3[16];	// [0x04C]
	uint32_t unknown2;	// [0x05C] 00 20 00 00
	uint32_t unknown3;	// [0x060] 03 90 00 00

	char filename[20];	// [0x064] Filename. (filename[6] == 'P' for PocketStation)

	uint8_t reserved4[12];	// [0x078]
} PS1_PSV_Header;
ASSERT_STRUCT(PS1_PSV_Header, 0x84);
#pragma pack()

#ifdef __cplusplus
}
#endif
