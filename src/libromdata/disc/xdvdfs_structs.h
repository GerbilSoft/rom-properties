/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * xdvdfs_structs.h: Xbox XDVDFS structs.                                  *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

// References:
// - https://xboxdevwiki.net/Xbox_Game_Disc
// - https://github.com/XboxDev/extract-xiso/blob/master/extract-xiso.c
// - https://github.com/multimediamike/xbfuse/blob/master/src/xdvdfs.c
// - https://www.eurasia.nu/wiki/index.php/XboxFileSystemDetails

#ifndef __ROMPROPERTIES_LIBROMDATA_DISC_XDVDFS_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_DISC_XDVDFS_STRUCTS_H__

#include "librpbase/common.h"
#include "librpbase/byteorder.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

// XDVDFS offsets, in LBAs.
// TODO: Multiple XGD1 offsets? (This matches Futurama.)
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
typedef struct PACKED _XDVDFS_Header {
	char magic[20];			// [0x000] "MICROSOFT*XBOX*MEDIA"
	uint32_t root_dir_sector;	// [0x014] Root directory sector
	uint32_t root_dir_size;		// [0x018] Root directory size
	uint64_t timestamp;		// [0x01C] Timestamp (Windows FILETIME format)
	uint8_t unused[0x7C8];		// [0x020]
	char magic_footer[20];		// [0x7EC] "MICROSOFT*XBOX*MEDIA" (footer magic)
} XDVDFS_Header;
ASSERT_STRUCT(XDVDFS_Header, XDVDFS_BLOCK_SIZE);

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
typedef struct PACKED _XDVDFS_DirEntry {
	uint16_t left_offset;		// [0x000] Offset to left subtree entry, in DWORDs. (0 for none)
	uint16_t right_offset;		// [0x002] Offset to right subtree entry, in DWORDs. (0 for none)
	uint32_t start_sector;		// [0x004] Starting sector
	uint32_t file_size;		// [0x008] File size, in bytes
	uint8_t attributes;		// [0x00C] Attributes bitfield (See XDVDFS_Attributes_e)
	uint8_t name_length;		// [0x00D] Filename length, in bytes
} XDVDFS_DirEntry;
ASSERT_STRUCT(XDVDFS_DirEntry, 14);

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

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_XDVDFS_STRUCTS_H__ */
