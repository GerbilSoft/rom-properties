/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * xdvdfs_structs.h: Xbox XDVDFS structs.                                  *
 *                                                                         *
 * Copyright (c) 2019-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - https://xboxdevwiki.net/Xbox_Game_Disc
// - https://github.com/XboxDev/extract-xiso/blob/master/extract-xiso.c
// - https://github.com/multimediamike/xbfuse/blob/master/src/xdvdfs.c
// - https://www.eurasia.nu/wiki/index.php/XboxFileSystemDetails

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

// XDVDFS offsets, in LBAs.
#define XDVDFS_BLOCK_SIZE	2048U	/* same as CD/DVD */
#define XDVDFS_LBA_OFFSET_XGD1	0x30600U
#define XDVDFS_LBA_OFFSET_XGD2	0x1FB20U
#define XDVDFS_LBA_OFFSET_XGD3	 0x4100U

/**
 * XDVDFS header.
 * Located at the start of the XDVDFS portion of the disc.
 *
 * All fields are in little-endian.
 */
#define XDVDFS_MAGIC "MICROSOFT*XBOX*MEDIA"
#define XDVDFS_HEADER_LBA_OFFSET	32U	/* relative to XDVDFS offset, in LBAs */
#pragma pack(4)
typedef struct PACKED _XDVDFS_Header {
	char magic[20];			// [0x000] "MICROSOFT*XBOX*MEDIA"
	uint32_t root_dir_sector;	// [0x014] Root directory sector
	uint32_t root_dir_size;		// [0x018] Root directory size
	uint64_t timestamp;		// [0x01C] Timestamp (Windows FILETIME format)
	uint8_t unused[0x7C8];		// [0x020]
	char magic_footer[20];		// [0x7EC] "MICROSOFT*XBOX*MEDIA" (footer magic)
} XDVDFS_Header;
ASSERT_STRUCT(XDVDFS_Header, XDVDFS_BLOCK_SIZE);
#pragma pack()

/**
 * Directory entry.
 * XDVDFS directories use a binary tree structure for fast searches.
 *
 * Binary search is case-insensitive.
 * NOTE: Not sure if strcasecmp() should be used - manual handling
 * of only 'a'-'z'? (this is what extract-xiso does)
 *
 * Filename is stored immediately after the directory entry.
 * If the filename does not end on a DWORD boundary, it is
 * padded using 0xFF.
 *
 * Subtree offsets are relative to the start of the directory.
 *
 * If left_offset or right_offset are 0, those subtrees don't exist.
 * If left_offset or right_offset are 0xFFFF, we've probably reached
 * the end of the directory.
 *
 * All fields are in little-endian.
 */
#pragma pack(2)
typedef struct PACKED _XDVDFS_DirEntry {
	uint16_t left_offset;		// [0x000] Offset to left subtree entry, in DWORDs. (0 for none)
	uint16_t right_offset;		// [0x002] Offset to right subtree entry, in DWORDs. (0 for none)
	uint32_t start_sector;		// [0x004] Starting sector
	uint32_t file_size;		// [0x008] File size, in bytes
	uint8_t attributes;		// [0x00C] Attributes bitfield (See XDVDFS_Attributes_e)
	uint8_t name_length;		// [0x00D] Filename length, in bytes
} XDVDFS_DirEntry;
ASSERT_STRUCT(XDVDFS_DirEntry, 14);
#pragma pack()

/**
 * File attributes. (bitfield)
 */
typedef enum {
	XDVDFS_ATTR_READONLY	= 0x01,
	XDVDFS_ATTR_HIDDEN	= 0x02,
	XDVDFS_ATTR_SYSTEM	= 0x04,
	XDVDFS_ATTR_DIRECTORY	= 0x10,
	XDVDFS_ATTR_ARCHIVE	= 0x20,
	XDVDFS_ATTR_NORMAL	= 0x80,
} XDVDFS_Attributes_e;

#ifdef __cplusplus
}
#endif
