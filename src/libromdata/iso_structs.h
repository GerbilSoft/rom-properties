/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * iso9660.h: ISO-9660 structs for CD-ROM images.                          *
 *                                                                         *
 * Copyright (c) 2017 by David Korth.                                      *
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

// Reference: http://wiki.osdev.org/ISO_9660

#ifndef __ROMPROPERTIES_LIBROMDATA_ISO9660_H__
#define __ROMPROPERTIES_LIBROMDATA_ISO9660_H__

#include "librpbase/common.h"
#include "librpbase/byteorder.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

// ISO-9660 sector sizes.
#define ISO_SECTOR_SIZE_MODE1_RAW    2352
#define ISO_SECTOR_SIZE_MODE1_COOKED 2048

// strD: [A-Z0-9_]
// strA: strD plus: ! " % & ' ( ) * + , - . / : ; < = > ?

/**
 * ISO-9660 16-bit value, stored as both LE and BE.
 */
typedef union PACKED _uint16_lsb_msb_t {
	struct {
		uint16_t le;
		uint16_t be;
	};
	// Host-endian value.
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	struct {
		uint16_t he;
		uint16_t x;
	};
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
	struct {
		uint16_t x;
		uint16_t he;
	};
#endif
} uint16_lsb_msb_t;
ASSERT_STRUCT(uint16_lsb_msb_t, 4);

/**
 * ISO-9660 32-bit value, stored as both LE and BE.
 */
typedef union PACKED _uint32_lsb_msb_t {
	struct {
		uint32_t le;
		uint32_t be;
	};
	// Host-endian value.
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	struct {
		uint32_t he;
		uint32_t x;
	};
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
	struct {
		uint32_t x;
		uint32_t he;
	};
#endif
} uint32_lsb_msb_t;
ASSERT_STRUCT(uint32_lsb_msb_t, 8);

/**
 * ISO-9660 Primary Volume Descriptor date/time struct.
 * Note that the fields are all strings.
 *
 * For an unspecified time, all text fields contain '0' (ASCII zero)
 * and tz_offset is binary zero.
 */
typedef struct PACKED _ISO_PVD_DateTime_t {
	union {
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
	};

	// Timezone offset, in 15-minute intervals.
	//   0 == interval -48 (GMT-1200)
	// 100 == interval  52 (GMT+1300)
	uint8_t tz_offset;
} ISO_PVD_DateTime_t;
ASSERT_STRUCT(ISO_PVD_DateTime_t, 17);

/**
 * ISO-9660 Directory Entry date/time struct.
 */
typedef struct PACKED _ISO_Dir_DateTime_t {
	uint8_t year;		// Number of years since 1900.
	uint8_t month;		// Month, from 1 to 12.
	uint8_t day;		// Day, from 1 to 31.
	uint8_t hour;		// Hour, from 0 to 23.
	uint8_t minute;		// Minute, from 0 to 59.
	uint8_t second;		// Second, from 0 to 59.

	// Timezone offset, in 15-minute intervals.
	//   0 == interval -48 (GMT-1200)
	// 100 == interval  52 (GMT+1300)
	uint8_t tz_offset;
} ISO_Dir_DateTime_t;
ASSERT_STRUCT(ISO_Dir_DateTime_t, 7);

/**
 * Directory entry, excluding the variable-length file identifier.
 */
typedef struct PACKED _ISO_DirEntry {
	uint8_t entry_length;			// Length of Directory Record. (must be at least 33 + filename)
	uint8_t xattr_length;			// Extended Attribute Record length.
	uint32_lsb_msb_t block;			// Starting LBA of the file.
	uint32_lsb_msb_t size;			// Size of the file.
	ISO_Dir_DateTime_t mtime;		// Recording date and time.
	uint8_t flags;				// File flags. (See ISO_File_Flags_t.)
	uint8_t unit_size;			// File unit size if recorded in interleaved mode; otherwise 0.
	uint8_t interleave_gap;			// Interleave gap size if recorded in interleaved mode; otherwise 0.
	uint16_lsb_msb_t volume_seq_num;	// Volume sequence number. (disc this file is recorded on)
	uint8_t filename_length;		// Filename length. Terminated with ';' followed by the file ID number in ASCII ('1').
} ISO_DirEntry;
ASSERT_STRUCT(ISO_DirEntry, 33);

typedef enum {
	ISO_FLAG_HIDDEN		= (1 << 0),	// File is hidden.
	ISO_FLAG_DIRECTORY	= (1 << 1),	// File is a subdirectory.
	ISO_FLAG_ASSOCIATED	= (1 << 2),	// "Associated" file.
	ISO_FLAG_XATTR		= (1 << 3),	// xattr contaisn information about the format of this file.
	ISO_FLAG_UID_GID	= (1 << 4),	// xattr contains uid and gid.
	ISO_FLAG_NOT_FINAL	= (1 << 7),	// If set, this is not the final directory record for the file.
						// Could be used for files larger than 4 GB, but generally isn't.
} ISO_File_Flags_t;

/**
 * Volume descriptor. Located at 0x8000.
 */
#define ISO_MAGIC "CD001"
#define ISO_VD_VERSION 0x01
typedef struct PACKED _ISO_Volume_Descriptor {
	uint8_t type;		// Volume descriptor type code. (See ISO_Volume_Descriptor_Type.)
	char identifier[5];	// (strA) "CD001"
	uint8_t version;	// Volume descriptor version. (0x01)

	union {
		uint8_t data[2041];

		// Boot record.
		struct {
			char sysID[32];		// (strA) System identifier.
			char bootID[32];	// (strA) Boot identifier.
			union {
				uint32_t boot_catalog_addr;	// (LE32) Block address of the El Torito boot catalog.
				uint8_t boot_system_use[1977];
			};
		} boot;

		// Volume descriptor.
		struct {
			uint8_t reserved1;			// 0x00
			char sysID[32];				// (strA) System identifier.
			char volID[32];				// (strD) Volume identifier.
			uint8_t reserved2[8];			// All zeroes.
			uint32_lsb_msb_t volume_space_size;	// Size of volume, in blocks.
			uint8_t reserved3[32];			// All zeroes.
			uint16_lsb_msb_t volume_set_size;	// Size of the logical volume. (number of discs)
			uint16_lsb_msb_t volume_seq_number;	// Disc number in the volume set.
			uint16_lsb_msb_t logical_block_size;	// Logical block size. (usually 2048)
			uint32_lsb_msb_t path_table_size;	// Path table size, in bytes.
			uint32_t path_table_lba_L;		// (LE32) Path table LBA. (contains LE values only)
			uint32_t path_table_optional_lba_L;	// (LE32) Optional path table LBA. (contains LE values only)
			uint32_t path_table_lba_M;		// (BE32) Path table LBA. (contains BE values only)
			uint32_t path_table_optional_lba_M;	// (BE32) Optional path table LBA. (contains BE values only)
			ISO_DirEntry dir_entry_root;		// Root directory record.
			char dir_entry_root_filename;		// Root directory filename. (NULL byte)
			char volume_set_id[128];		// (strD) Volume set identifier.

			// For the following fields:
			// - "\x5F" "FILENAME.BIN" to refer to a file in the root directory.
			// - If empty, fill with all 0x20.
			char publisher[128];			// (strA) Volume publisher.
			char data_preparer[128];		// (strA) Data preparer.
			char application[128];			// (strA) Application.

			// For the following fields:
			// - Filenames must be in the root directory.
			// - If empty, fill with all 0x20.
			char copyright_file[38];		// (strD) Filename of the copyright file.
			char abstract_file[36];			// (strD) Filename of the abstract file.
			char bibliographic_file[37];		// (strD) Filename of the bibliographic file.

			// Timestamps.
			ISO_PVD_DateTime_t btime;		// Volume creation time.
			ISO_PVD_DateTime_t mtime;		// Volume modification time.
			ISO_PVD_DateTime_t exptime;		// Volume expiration time.
			ISO_PVD_DateTime_t efftime;		// Volume effective time.

			uint8_t file_structure_version;		// Directory records and path table version. (0x01)
			uint8_t reserved4;			// 0x00

			uint8_t application_data[512];		// Not defined by ISO-9660.
			uint8_t iso_reserved[653];		// Reserved by ISO.
		} pri;
	};
} ISO_Volume_Descriptor;
ASSERT_STRUCT(ISO_Volume_Descriptor, ISO_SECTOR_SIZE_MODE1_COOKED);

/**
 * Volume descriptor type.
 */
typedef enum {
	ISO_VDT_BOOT_RECORD = 0,
	ISO_VDT_PRIMARY = 1,
	ISO_VDT_SUPPLEMENTARY = 2,
	ISO_VDT_PARTITION = 3,
	ISO_VDT_TERMINATOR = 255,
} ISO_Volume_Descriptor_Type;

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_ISO9660_H__ */
