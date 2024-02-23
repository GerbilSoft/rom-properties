/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * cbmdos_structs.h: Commodore DOS floppy disk structs.                    *
 *                                                                         *
 * Copyright (c) 2019-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: http://unusedino.de/ec64/technical/formats/d64.html

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

// CBM DOS almost always uses 256-byte sectors.
// The one exception is C1581, which uses 512-byte
// physical sectors and 256-byte logical sectors.
#define CBMDOS_SECTOR_SIZE 256

/**
 * Track/sector pointer
 */
typedef struct _cbmdos_TS_ptr_t {
	uint8_t track;	// Next track (starts at 1)
	uint8_t sector;	// Next sector (starts at 1)
} cbmdos_TS_ptr_t;
ASSERT_STRUCT(cbmdos_TS_ptr_t, 2);

/**
 * CBMDOS: C1541 Block Allocation Map (18/0)
 */
typedef struct _cbmdos_C1541_BAM_t {
	cbmdos_TS_ptr_t next;	// $00: Location of the first directory sector
	                        //      NOTE: Ignore this; it should always be 18/1.
	uint8_t dos_version;	// $02: DOS version type. ("A" for C1541)
	uint8_t unused_03;	// $03
	uint8_t bam[35*4];	// $04: BAM entries for each track.
	                        //      4 bytes per track; 35 tracks.
	char disk_name[16];	// $90: Disk name (PETSCII, $A0-padded)
	uint8_t unused_A0[2];	// $A0: Filled with $A0
	char disk_id[2];	// $A2: Disk ID (PETSCII)
	uint8_t unused_A4;	// $A4: Unused (usually $A0)
	char dos_type[2];	// $A5: DOS type (usually "2A")
	uint8_t unused_A7[4];	// $A7: Filled with $A0

	// $AB-$FF is unused by CBM DOS, but may be
	// used by third-party enhancements.
	union {
		uint8_t unused_CBMDOS_AB[0x55];		// $AB
		struct {
			uint8_t unused_other_AB;	// $AB
			uint8_t dolphin_dos_BAM[5*4];	// $AC: Dolphin DOS track 36-40 BAM entries
			uint8_t speed_dos_BAM[5*4];	// $C0: Speed DOS track 36-40 BAM entries
		};
	};
} cbmdos_C1541_BAM_t;
ASSERT_STRUCT(cbmdos_C1541_BAM_t, CBMDOS_SECTOR_SIZE);

/**
 * CBMDOS: Directory entry
 * NOTE: For C1541; may be the same on others, but needs verification.
 *
 * All fields are in little-endian.
 */
typedef struct _cbmdos_dir_entry_t {
	cbmdos_TS_ptr_t next;	// $00: Location of next directory sector.
	                        //      Only valid on the first directory entry
	                        //      in a given sector. (Others should have 0/0.)
	                        //      Final directory sector has T=$00.
	uint8_t file_type;	// $02: File type. (see CBMDOS_FileType_e)
	cbmdos_TS_ptr_t start;	// $03: Location of first sector of the file.
	char filename[16];	// $05: Filename (PETSCII, $A0-padded)
	cbmdos_TS_ptr_t rel_side_sector;	// $15: Location of first side-sector block (REL files only)
	uint8_t rel_record_len;	// $17: REL file record length (max 254)
	uint8_t unused_18[6];	// $18: Unused (except by GEOS...)
	uint16_t sector_count;	// $1E: File size, in sectors.
} cbmdos_dir_entry_t;
ASSERT_STRUCT(cbmdos_dir_entry_t, 32);

/**
 * CBMDOS: One sector worth of directory entries.
 */
typedef struct _cbmdos_dir_sector_t {
	cbmdos_dir_entry_t entry[CBMDOS_SECTOR_SIZE / sizeof(cbmdos_dir_entry_t)];
} cbmdos_dir_sector_t;
ASSERT_STRUCT(cbmdos_dir_sector_t, CBMDOS_SECTOR_SIZE);

/**
 * CBMDOS: File type (bitfield)
 */
typedef enum {
	// Bits 0-3: The actual filetype
	CBMDOS_FileType_DEL	= 0,
	CBMDOS_FileType_SEQ	= 1,
	CBMDOS_FileType_PRG	= 2,
	CBMDOS_FileType_USR	= 3,
	CBMDOS_FileType_REL	= 4,
	CBMDOS_FileType_Mask	= 0x0F,

	// Bit 4: Not used

	// Bit 5: Used during SAVE-@ replacement
	CBMDOS_FileType_SaveReplace	= (1U << 5),

	// Bit 6: Locked flag (">")
	CBMDOS_FileType_Locked		= (1U << 6),

	// Bit 7: Closed flag (if unset, and Bits 0-3 are non-zero, results in a "*" (splat) file)
	CBMDOS_FileType_Closed		= (1U << 7),
} CBMDOS_FileType_e;

#ifdef __cplusplus
}
#endif
