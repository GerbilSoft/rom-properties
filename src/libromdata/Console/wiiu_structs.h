/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * wiiu_structs.h: Nintendo Wii U data structures.                         *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Nintendo Wii U disc header. (Retail games only!)
 * Reference: https://github.com/maki-chan/wudecrypt/blob/master/main.c
 * 
 * All fields are big-endian.
 * NOTE: Strings are NOT null-terminated!
 */
#pragma pack(1)	// NOTE: Some compilers pad this structure to a multiple of 4 bytes
#define WIIU_MAGIC 'WUP-'
typedef struct _WiiU_DiscHeader {
	union PACKED {
		uint32_t magic;		// 'WUP-'
		char id[10];		// "WUP-P-xxxx"
		struct PACKED {
			char wup[3];	// "WUP"
			char hyphen1;	// '-'
			char p;		// 'P'
			char hyphen2;	// '-'
			char id4[4];	// "xxxx"
		};
	};
	char hyphen3;
	char version[2];	// Version number, in ASCII. (e.g. "00")
	char hyphen4;
	char os_version[3];	// Required OS version, in ASCII. (e.g. "551")
	char region[3];		// Region code, in ASCII. ("USA", "EUR") (TODO: Is this the enforced region?)
	char hyphen5;
	char disc_number;	// Disc number, in ASCII. (TODO: Verify?)
} WiiU_DiscHeader;
ASSERT_STRUCT(WiiU_DiscHeader, 22);
#pragma pack()

// Secondary Wii U disc magic at 0x10000.
#define WIIU_SECONDARY_MAGIC 0xCC549EB9

/**
 * Wii U CMD group entry (for v1 TMD)
 *
 * All fields are big-endian.
 */
typedef struct _WUP_CMD_GroupEntry {
	uint16_t offset;		// [0x000] Offset of the CMD group
	uint16_t nbr_cont;		// [0x002] Number of CMDs in the group
	uint8_t sha256_hash[32];	// [0x004] SHA-256 hash of the CMDs in the group
} WUP_CMD_GroupEntry;
ASSERT_STRUCT(WUP_CMD_GroupEntry, 36);

/**
 * Wii U CMD group header (for v1 TMD)
 *
 * All fields are big-endian.
 */
#pragma pack(1)
typedef struct _WUP_CMD_GroupHeader {
	uint8_t sha256_hash[32];	// [0x000] SHA-256 hash of CMD groups
	WUP_CMD_GroupEntry entries[64];	// [0x020] Up to 64 CMD group entries
} WUP_CMD_GroupHeader;
ASSERT_STRUCT(WUP_CMD_GroupHeader, 2336);
#pragma pack()

/**
 * Wii U content entry (Stored after the TMD) (v1)
 * Reference: https://wiibrew.org/wiki/Title_metadata
 *
 * All fields are big-endian.
 */
typedef struct _WUP_Content_Entry {
	uint32_t content_id;		// [0x000] Content ID
	uint16_t index;			// [0x004] Index
	uint16_t type;			// [0x006] Type (see RVL_Content_Type_e)
	uint64_t size;			// [0x008] Size
	uint8_t sha1_hash[20];		// [0x010] SHA-1 hash of the content (installed) or H3 table (disc).
	uint8_t unused[12];		// [0x024] Unused. (Maybe it was going to be used for SHA-256?)
} WUP_Content_Entry;
ASSERT_STRUCT(WUP_Content_Entry, 48);

#ifdef __cplusplus
}
#endif
