/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * hsfs_structs.h: High Sierra structs for old CD-ROM images.              *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * Copyright (c) 2020 by Egor.                                             *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: OpenSolaris source code /usr/src/uts/common/sys/fs/hsfs_spec.h

#pragma once

#include <stdint.h>

#include "common.h"
#include "librpcpu/byteorder.h"
#include "../iso_structs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * HSFS Primary Volume Descriptor date/time struct.
 * Note that the fields are all strings.
 *
 * For an unspecified time, all text fields contain '0' (ASCII zero)
 */
typedef union _HSFS_PVD_DateTime_t {
	char full[16];
	struct {
		char year[4];		// Year, from 1 to 9999.
		char month[2];		// Month, from 1 to 12.
		char day[2];		// Day, from 1 to 31.
		char hour[2];		// Hour, from 0 to 23.
		char minute[2];		// Minute, from 0 to 59.
		char second[2];		// Second, from 0 to 59.
		char csecond[2];	// Centiseconds, from 0 to 99.
	};
} HSFS_PVD_DateTime_t;
ASSERT_STRUCT(HSFS_PVD_DateTime_t, 16);

/**
 * HSFS Directory Entry date/time struct.
 */
// Some compilers pad this structure to a multiple of 4 bytes
#pragma pack(1)
typedef struct PACKED _HSFS_Dir_DateTime_t {
	uint8_t year;		// Number of years since 1900.
	uint8_t month;		// Month, from 1 to 12.
	uint8_t day;		// Day, from 1 to 31.
	uint8_t hour;		// Hour, from 0 to 23.
	uint8_t minute;		// Minute, from 0 to 59.
	uint8_t second;		// Second, from 0 to 59.
} HSFS_Dir_DateTime_t;
ASSERT_STRUCT(HSFS_Dir_DateTime_t, 6);
#pragma pack()

/**
 * Directory entry, excluding the variable-length file identifier.
 */
#pragma pack(1)
typedef struct PACKED _HSFS_DirEntry {
	uint8_t entry_length;			// [0x000] Length of Directory Record. (must be at least 33 + filename)
	uint8_t xattr_length;			// [0x001] Extended Attribute Record length.
	uint32_lsb_msb_t block;			// [0x002] Starting LBA of the file.
	uint32_lsb_msb_t size;			// Size of the file.
	HSFS_Dir_DateTime_t mtime;		// Recording date and time.
	uint8_t flags;				// File flags. (See ISO_File_Flags_t.)
	uint8_t reserved;
	uint8_t unit_size;			// File unit size if recorded in interleaved mode; otherwise 0.
	uint8_t interleave_gap;			// Interleave gap size if recorded in interleaved mode; otherwise 0.
	uint16_lsb_msb_t volume_seq_num;	// Volume sequence number. (disc this file is recorded on)
	uint8_t filename_length;		// Filename length. Terminated with ';' followed by the file ID number in ASCII ('1').
} HSFS_DirEntry;
ASSERT_STRUCT(HSFS_DirEntry, 33);
#pragma pack()

/**
 * Volume descriptor header.
 */
#pragma pack(1)
typedef struct PACKED _HSFS_Volume_Descriptor_Header {
	uint32_lsb_msb_t block;	// LBA of this volume descriptor.
	uint8_t type;		// Volume descriptor type code. (See ISO_Volume_Descriptor_Type.)
	char identifier[5];	// (strA) "CDROM"
	uint8_t version;	// Volume descriptor version. (0x01)
} HSFS_Volume_Descriptor_Header;
ASSERT_STRUCT(HSFS_Volume_Descriptor_Header, 15);
#pragma pack()

/**
 * Primary volume descriptor.
 *
 * NOTE: All fields are space-padded. (0x20, ' ')
 */
#pragma pack(1)
typedef struct PACKED _HSFS_Primary_Volume_Descriptor {
	HSFS_Volume_Descriptor_Header header;

	uint8_t reserved1;			// [0x00F] 0x00
	char sysID[32];				// [0x010] (strA) System identifier.
	char volID[32];				// [0x030] (strD) Volume identifier.
	uint8_t reserved2[8];			// [0x050] All zeroes.
	uint32_lsb_msb_t volume_space_size;	// [0x058] Size of volume, in blocks.
	uint8_t reserved3[32];			// [0x060] All zeroes.
	uint16_lsb_msb_t volume_set_size;	// [0x080] Size of the logical volume. (number of discs)
	uint16_lsb_msb_t volume_seq_number;	// [0x084] Disc number in the volume set.
	uint16_lsb_msb_t logical_block_size;	// [0x088] Logical block size. (usually 2048)
	uint32_lsb_msb_t path_table_size;	// [0x08C] Path table size, in bytes.
	uint32_t path_table_lba_L;		// [0x094] (LE32) Path table LBA. (contains LE values only)
	uint32_t path_table_optional_lba_L[3];	// [0x098] (LE32) Optional path tables LBA. (contain LE values only)
	uint32_t path_table_lba_M;		// [0x0A4] (BE32) Path table LBA. (contains BE values only)
	uint32_t path_table_optional_lba_M[3];	// [0x0A8] (BE32) Optional path tables LBA. (contain BE values only)
	HSFS_DirEntry dir_entry_root;		// [0x0B4] Root directory record
	char dir_entry_root_filename;		// [0x0D5] Root directory filename. (NULL byte)
	char volume_set_id[128];		// [0x0D6] (strD) Volume set identifier.

	// For the following fields:
	// - (???) "\x5F" "FILENAME.BIN" to refer to a file in the root directory.
	// - If empty, fill with all 0x20.
	char publisher[128];			// [0x156] (strA) Volume publisher.
	char data_preparer[128];		// [0x1D6] (strA) Data preparer.
	char application[128];			// [0x256] (strA) Application.

	// For the following fields:
	// - Filenames must be in the root directory.
	// - If empty, fill with all 0x20.
	char copyright_file[32];		// [0x2D6] (strD) Filename of the copyright file.
	char abstract_file[32];			// [0x2F6] (strD) Filename of the abstract file.

	// Timestamps.
	// Some compilers add padding here
	HSFS_PVD_DateTime_t btime;		// [0x316] Volume creation time.
	HSFS_PVD_DateTime_t mtime;		// [0x326] Volume modification time.
	HSFS_PVD_DateTime_t exptime;		// [0x336] Volume expiration time.
	HSFS_PVD_DateTime_t efftime;		// [0x346] Volume effective time.

	uint8_t file_structure_version;		// [0x356] Directory records and path table version. (0x01)
	uint8_t reserved4[1193];		// [0x357]
} HSFS_Primary_Volume_Descriptor;
ASSERT_STRUCT(HSFS_Primary_Volume_Descriptor, ISO_SECTOR_SIZE_MODE1_COOKED);
#pragma pack()

#define HSFS_VD_MAGIC "CDROM"
#define HSFS_VD_VERSION 0x01

#ifdef __cplusplus
}
#endif
