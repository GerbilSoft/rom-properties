/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * cbmdos_structs.h: Commodore DOS floppy disk structs.                    *
 *                                                                         *
 * Copyright (c) 2019-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - http://unusedino.de/ec64/technical/formats/d64.html
// - http://unusedino.de/ec64/technical/formats/d71.html
// - http://unusedino.de/ec64/technical/formats/d80-d82.html
// - http://unusedino.de/ec64/technical/formats/d81.html
// - http://unusedino.de/ec64/technical/formats/g64.html
// - https://area51.dev/c64/cbmdos/autoboot/
// - http://unusedino.de/ec64/technical/formats/geos.html
// - https://sourceforge.net/p/vice-emu/patches/122/ (for .g71)

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
#define GEOS_ID_STRING "GEOS format V1.x"	// the 'x' may be 0 or 1; the '.' may be missing
typedef struct _cbmdos_C1541_BAM_t {
	cbmdos_TS_ptr_t next;	// $00: Location of the first directory sector
	                        //      NOTE: Ignore this; it should always be 18/1.
	uint8_t dos_version;	// $02: DOS version type ('A' for C1541)
	uint8_t double_sided;	// $03: [C1571] Double-sided flag (see CBMDOS_C1571_DoubleSided_e)
	uint8_t bam[35*4];	// $04: BAM entries for each track.
	                        //      4 bytes per track; 35 tracks.
	char disk_name[16];	// $90: Disk name (PETSCII, $A0-padded)
	uint8_t unused_A0[2];	// $A0: Filled with $A0
	char disk_id[2];	// $A2: Disk ID (PETSCII)
	uint8_t unused_A4;	// $A4: Unused (usually $A0)
	char dos_type[2];	// $A5: DOS type (usually "2A")
	uint8_t unused_A7[4];	// $A7: Filled with $A0

	// C1541: $AB-$FF is unused by CBM DOS, but may be
	// used by third-party enhancements.
	// C1571: $DD-$FF is the free sector count for tracks 36-70.
	union {
		uint8_t unused_CBMDOS_AB[0x55];		// $AB
		struct {
			uint8_t unused_other_AB;	// $AB
			uint8_t dolphin_dos_BAM[5*4];	// $AC: Dolphin DOS track 36-40 BAM entries
			uint8_t speed_dos_BAM[5*4];	// $C0: Speed DOS track 36-40 BAM entries
		};
		struct {
			uint8_t unused_C1571_AB[0x32];	// $AB
			uint8_t free_sector_count[35];	// $DD: [C1571] Free sector count for tracks 36-70
		};
		struct {
			cbmdos_TS_ptr_t border_sector;	// $AB: Border sector location
			char geos_id_string[16];	// $AC: GEOS ID string, in ASCII
			                                // (check first 4 char for "GEOS" in ASCII)
			uint8_t unused_other_BD[16];	// $BD
			uint8_t free_sector_count[35];	// $DD: [C1571] Free sector count for tracks 36-70
		} geos;
	};
} cbmdos_C1541_BAM_t;
ASSERT_STRUCT(cbmdos_C1541_BAM_t, CBMDOS_SECTOR_SIZE);

/**
 * CBMDOS: C1571 double-sided flag
 */
typedef enum {
	CBMDOS_C1571_SingleSided = (0U << 7),
	CBMDOS_C1571_DoubleSided = (1U << 7),

	CBMDOS_C1571_DoubleSided_mask = (1U << 7),
} CBMDOS_C1571_DoubleSided_e;

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
	union {
		struct {
			cbmdos_TS_ptr_t rel_side_sector;	// $15: Location of first side-sector block (REL files only)
			uint8_t rel_record_len;	// $17: REL file record length (max 254)
			uint8_t unused_18[6];	// $18: Unused (should be $00)
		};
		struct {
			cbmdos_TS_ptr_t info_addr;	// $15: Location of GEOS info sector
			uint8_t file_structure;		// $17: GEOS file structure (see GEOS_File_Structure_e)
			uint8_t file_type;		// $18: GEOS file type (see GEOS_File_Type_e)
			struct {
				uint8_t year;		// $19: Year (1900 + value)
				uint8_t month;		// $1A: Month (01-12)
				uint8_t day;		// $1B: Day (01-31)
				uint8_t hour;		// $1C: Hour (00-23)
				uint8_t minute;		// $1D: Minute (00-59)
			} timestamp;
		} geos;
	};
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

/**
 * CBMDOS: GEOS file structure
 */
typedef enum {
	GEOS_FILE_STRUCTURE_SEQ		= 0,
	GEOS_FILE_STRUCTURE_VLIR	= 1,
} GEOS_File_Structure_e;

/**
 * CBMDOS: GEOS file type
 */
typedef enum {
	GEOS_FILE_TYPE_NON_GEOS		= 0x00,
	GEOS_FILE_TYPE_BASIC		= 0x01,
	GEOS_FILE_TYPE_ASSEMBLER	= 0x02,
	GEOS_FILE_TYPE_DATA_FILE	= 0x03,
	GEOS_FILE_TYPE_SYSTEM_FILE	= 0x04,
	GEOS_FILE_TYPE_DESK_ACCESSORY	= 0x05,
	GEOS_FILE_TYPE_APPLICATION	= 0x06,
	GEOS_FILE_TYPE_APPLICATION_DATA	= 0x07,
	GEOS_FILE_TYPE_FONT_FILE	= 0x08,
	GEOS_FILE_TYPE_PRINTER_DRIVER	= 0x09,
	GEOS_FILE_TYPE_INPUT_DRIVER	= 0x0A,
	GEOS_FILE_TYPE_DISK_DRIVER	= 0x0B,
	GEOS_FILE_TYPE_SYSTEM_BOOT_FILE	= 0x0C,
	GEOS_FILE_TYPE_TEMPORARY	= 0x0D,
	GEOS_FILE_TYPE_AUTO_EXEC_FILE	= 0x0E,
} GEOS_File_Type_e;

/**
 * CBMDOS: C8050/C8250 header sector (39/0)
 */
typedef struct _cbmdos_C8050_header_t {
	cbmdos_TS_ptr_t bam0;	// $00: Location of the first BAM sector (38/0)
	uint8_t dos_version;	// $02: DOS version type ('C' for C8050/C8250)
	uint8_t unused_03;	// $03: Reserved
	uint8_t unused_04[2];	// $04: Unused
	char disk_name[17];	// $06: Disk name (PETSCII, $A0-padded)
	uint8_t unused_17;	// $17: $A0
	char disk_id[2];	// $18: Disk ID (PETSCII)
	uint8_t unused_1A;	// $1A: $A0
	char dos_type[2];	// $1B: DOS type (usually "2C")
	uint8_t unused_1D[4];	// $1D: $A0
	uint8_t unused_21[223];	// $21: Unused
} cbmdos_C8050_header_t;
ASSERT_STRUCT(cbmdos_C8050_header_t, CBMDOS_SECTOR_SIZE);

/**
 * CBMDOS: C1581 header sector (40/0)
 */
typedef struct _cbmdos_C1581_header_t {
	cbmdos_TS_ptr_t dir0;	// $00: Location of the first directory sector (40/3)
	uint8_t dos_version;	// $02: DOS version type ('D' for C1581)
	uint8_t unused_03;	// $03: $00
	char disk_name[16];	// $04: Disk name (PETSCII, $A0-padded)
	uint8_t unused_14[2];	// $14: $A0
	char disk_id[2];	// $16: Disk ID (PETSCII)
	uint8_t unused_18;	// $18: $A0
	char dos_type[2];	// $19: DOS type (usually "2D")
	uint8_t unused_1B[2];	// $1B: $A0
	uint8_t unused_1D[227];	// $1D: Unused
} cbmdos_C1581_header_t;
ASSERT_STRUCT(cbmdos_C1581_header_t, CBMDOS_SECTOR_SIZE);

/**
 * CBMDOS: C128 autoboot sector (1/0)
 */
#define CBMDOS_C128_AUTOBOOT_SIGNATURE "CBM"
typedef struct _cbmdos_C128_autoboot_sector_t {
	char signature[3];		// $00: "CBM"
	cbmdos_TS_ptr_t addl_sectors;	// $03: Start address of additional sectors to load. (0/0 for none)
	uint8_t bank;			// $05: Bank for additional sectors. (default $00)
	uint8_t load_count;		// $06: Number of sectors to load. (default $00)
	char messages[249];		// $07: Contains two NULL-terminated strings, then a bootloader:
	                                //      - String 1: Boot message. (if empty, uses "BOOTING...")
	                                //      - String 2: Filename of program to load. (can be empty)
} cbmdos_C128_autoboot_sector_t;
ASSERT_STRUCT(cbmdos_C128_autoboot_sector_t, CBMDOS_SECTOR_SIZE);

/**
 * CBMDOS: GCR-1541 header (for .g64 disk images)
 * Also used for GCR-1571 (for .g71 disk images)
 *
 * All fields are in little-endian.
 */
#define CBMDOS_G64_MAGIC "GCR-1541"
#define CBMDOS_G71_MAGIC "GCR-1571"
typedef struct _cbmdos_G64_header_t {
	char magic[8];			// $00: "GCR-1541" or "GCR-1571"
	uint8_t version;		// $08: G64 version (usually 0)
	uint8_t track_count;		// $09: Number of tracks
	                                //      For G64: Usually  84 (42 full + half tracks)
	                                //      For G71: Usually 168 (84 full + half tracks)
	uint16_t track_size;		// $0A: Size of each track, in bytes (usually 7928)
	uint32_t track_offsets[168];	// $0B: Track offsets (absolute)
} cbmdos_G64_header_t;
ASSERT_STRUCT(cbmdos_G64_header_t, 684);

/**
 * CBMDOS: GCR data block (decoded)
 */
typedef union _cbmdos_GCR_data_block_t {
	struct {
		uint8_t id;		// $000: Data block ID ($07)
		uint8_t data[CBMDOS_SECTOR_SIZE];	// $001: Data
		uint8_t checksum;	// $101: Checksum (XOR of all data bytes)
		uint8_t reserved_00[2];	// $102: 00 bytes to make the sector size a multiple of 5
	};
	uint8_t raw[260];
} cbmdos_GCR_data_block_t;
ASSERT_STRUCT(cbmdos_GCR_data_block_t, 260);

/**
 * CBMDOS: GEOS INFO block
 *
 * All fields are in little-endian.
 * All strings are in ASCII, NULL-terminated.
 */
#pragma pack(1)
typedef struct PACKED _cbmdos_GEOS_info_block_t {
	cbmdos_TS_ptr_t next;		// $00: Next sector (usually 0/255 because it's only one block)
	uint8_t id[3];			// $01: ID bytes (03 15 BF). 03 = icon width, 15 = icon height?
	uint8_t icon[63];		// $02: Icon bitmap (C64 high-res sprite format)
	uint8_t c64_file_type;		// $44: C64 file type
	uint8_t geos_file_type;		// $45: GEOS file type (see GEOS_File_Type_e)
	uint8_t geos_file_structure;	// $46: GEOS file structure (see GEOS_File_Structure_e)
	uint16_t prg_load_addr;		// $47: Program load address
	uint16_t prg_end_addr;		// $49: Program end address (only with accessories)
	uint16_t prg_start_addr;	// $4B: Program start address
	char class_text[20];		// $4D: Class text
	char author[20];		// $61: Author
	char creator[20];		// $75: For documents, the application that created this file
	uint8_t for_appl[23];		// $89: Available for applications
	char description[96];		// $A0: Description
} cbmdos_GEOS_info_block_t;
#pragma pack()
ASSERT_STRUCT(cbmdos_GEOS_info_block_t, CBMDOS_SECTOR_SIZE);

#ifdef __cplusplus
}
#endif
