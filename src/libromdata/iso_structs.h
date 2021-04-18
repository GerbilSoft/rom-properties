/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * iso9660.h: ISO-9660 structs for CD-ROM images.                          *
 *                                                                         *
 * Copyright (c) 2017-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - http://wiki.osdev.org/ISO_9660
// - https://github.com/roysmeding/cditools/blob/master/cdi.py

#ifndef __ROMPROPERTIES_LIBROMDATA_ISO9660_H__
#define __ROMPROPERTIES_LIBROMDATA_ISO9660_H__

#include <stdint.h>

#include "common.h"
#include "librpcpu/byteswap_rp.h"

#ifdef __cplusplus
extern "C" {
#endif

// ISO-9660 sector sizes.
#define ISO_SECTOR_SIZE_MODE1_COOKED		2048
#define ISO_SECTOR_SIZE_MODE1_RAW		2352
#define ISO_SECTOR_SIZE_MODE1_RAW_SUBCHAN	2448

// Data offsets.
#define ISO_DATA_OFFSET_MODE1_RAW    16
#define ISO_DATA_OFFSET_MODE1_COOKED 0
#define ISO_DATA_OFFSET_MODE2_XA     (16+8)

// strD: [A-Z0-9_]
// strA: strD plus: ! " % & ' ( ) * + , - . / : ; < = > ?

/**
 * ISO-9660 16-bit value, stored as both LE and BE.
 */
typedef union _uint16_lsb_msb_t {
	struct {
		uint16_t le;	// Little-endian
		uint16_t be;	// Big-endian
	};
	// Host-endian value.
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	struct {
		uint16_t he;	// Host-endian
		uint16_t se;	// Swap-endian
	};
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
	struct {
		uint16_t se;	// Swap-endian
		uint16_t he;	// Host-endian
	};
#endif
} uint16_lsb_msb_t;
ASSERT_STRUCT(uint16_lsb_msb_t, 4);

/**
 * ISO-9660 32-bit value, stored as both LE and BE.
 */
typedef union _uint32_lsb_msb_t {
	struct {
		uint32_t le;	// Little-endian
		uint32_t be;	// Big-endian
	};
	// Host-endian value.
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	struct {
		uint32_t he;	// Host-endian
		uint32_t se;	// Swap-endian
	};
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
	struct {
		uint32_t se;	// Swap-endian
		uint32_t he;	// Host-endian
	};
#endif
} uint32_lsb_msb_t;
ASSERT_STRUCT(uint32_lsb_msb_t, 8);

/**
 * Get the host-endian 16-bit value if it's non-zero.
 * Otherwise, get the swap-endian 16-bit value.
 */
static inline uint16_t host16ifNZ(const uint16_lsb_msb_t *lm16)
{
	uint16_t val = lm16->he;
	if (val == 0) {
		val = __swab16(lm16->se);
	}
	return val;
}

/**
 * Get the host-endian 32-bit value if it's non-zero.
 * Otherwise, get the swap-endian 32-bit value.
 */
static inline uint32_t host32ifNZ(const uint32_lsb_msb_t *lm32)
{
	uint32_t val = lm32->he;
	if (val == 0) {
		val = __swab32(lm32->se);
	}
	return val;
}

/**
 * ISO-9660 Primary Volume Descriptor date/time struct.
 * Note that the fields are all strings.
 *
 * For an unspecified time, all text fields contain '0' (ASCII zero)
 * and tz_offset is binary zero.
 */
#pragma pack(1)
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
	// Range: [-48 (GMT-1200), +52 (GMT+1300)]
	int8_t tz_offset;
} ISO_PVD_DateTime_t;
#pragma pack()
ASSERT_STRUCT(ISO_PVD_DateTime_t, 17);

/**
 * ISO-9660 Directory Entry date/time struct.
 */
#pragma pack(1)
typedef struct PACKED _ISO_Dir_DateTime_t {
	uint8_t year;		// Number of years since 1900.
	uint8_t month;		// Month, from 1 to 12.
	uint8_t day;		// Day, from 1 to 31.
	uint8_t hour;		// Hour, from 0 to 23.
	uint8_t minute;		// Minute, from 0 to 59.
	uint8_t second;		// Second, from 0 to 59.

	// Timezone offset, in 15-minute intervals.
	// Range: [-48 (GMT-1200), +52 (GMT+1300)]
	int8_t tz_offset;
} ISO_Dir_DateTime_t;
#pragma pack()
ASSERT_STRUCT(ISO_Dir_DateTime_t, 7);

/**
 * Directory entry, excluding the variable-length file identifier.
 */
#pragma pack(1)
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
#pragma pack()

typedef enum {
	ISO_FLAG_HIDDEN		= (1U << 0),	// File is hidden.
	ISO_FLAG_DIRECTORY	= (1U << 1),	// File is a subdirectory.
	ISO_FLAG_ASSOCIATED	= (1U << 2),	// "Associated" file.
	ISO_FLAG_XATTR		= (1U << 3),	// xattr contaisn information about the format of this file.
	ISO_FLAG_UID_GID	= (1U << 4),	// xattr contains uid and gid.
	ISO_FLAG_NOT_FINAL	= (1U << 7),	// If set, this is not the final directory record for the file.
						// Could be used for files larger than 4 GB, but generally isn't.
} ISO_File_Flags_t;

/**
 * Volume descriptor header.
 */
#pragma pack(1)
typedef struct PACKED _ISO_Volume_Descriptor_Header {
	uint8_t type;		// Volume descriptor type code. (See ISO_Volume_Descriptor_Type.)
	char identifier[5];	// (strA) "CD001"
	uint8_t version;	// Volume descriptor version. (0x01)
} ISO_Volume_Descriptor_Header;
#pragma pack()
ASSERT_STRUCT(ISO_Volume_Descriptor_Header, 7);

/**
 * Boot volume descriptor.
 */
#pragma pack(1)
typedef struct PACKED _ISO_Boot_Volume_Descriptor {
	ISO_Volume_Descriptor_Header header;
	char sysID[32];		// (strA) System identifier.
	char bootID[32];	// (strA) Boot identifier.
	union {
		uint32_t boot_catalog_addr;	// (LE32) Block address of the El Torito boot catalog.
		uint8_t boot_system_use[1977];
	};
} ISO_Boot_Volume_Descriptor;
#pragma pack()
ASSERT_STRUCT(ISO_Boot_Volume_Descriptor, ISO_SECTOR_SIZE_MODE1_COOKED);

/**
 * Primary volume descriptor.
 *
 * NOTE: All fields are space-padded. (0x20, ' ')
 */
#pragma pack(1)
typedef struct PACKED _ISO_Primary_Volume_Descriptor {
	ISO_Volume_Descriptor_Header header;

	uint8_t reserved1;			// [0x007] 0x00
	char sysID[32];				// [0x008] (strA) System identifier.
	char volID[32];				// [0x028] (strD) Volume identifier.
	uint8_t reserved2[8];			// [0x048] All zeroes.
	uint32_lsb_msb_t volume_space_size;	// [0x050] Size of volume, in blocks.
	uint8_t reserved3[32];			// [0x058] All zeroes.
	uint16_lsb_msb_t volume_set_size;	// [0x078] Size of the logical volume. (number of discs)
	uint16_lsb_msb_t volume_seq_number;	// [0x07C] Disc number in the volume set.
	uint16_lsb_msb_t logical_block_size;	// [0x080] Logical block size. (usually 2048)
	uint32_lsb_msb_t path_table_size;	// [0x084] Path table size, in bytes.
	uint32_t path_table_lba_L;		// [0x08C] (LE32) Path table LBA. (contains LE values only)
	uint32_t path_table_optional_lba_L;	// [0x090] (LE32) Optional path table LBA. (contains LE values only)
	uint32_t path_table_lba_M;		// [0x094] (BE32) Path table LBA. (contains BE values only)
	uint32_t path_table_optional_lba_M;	// [0x098] (BE32) Optional path table LBA. (contains BE values only)
	ISO_DirEntry dir_entry_root;		// [0x09C] Root directory record.
	char dir_entry_root_filename;		// [0x0BD] Root directory filename. (NULL byte)
	char volume_set_id[128];		// [0x0BE] (strD) Volume set identifier.

	// For the following fields:
	// - "\x5F" "FILENAME.BIN" to refer to a file in the root directory.
	// - If empty, fill with all 0x20.
	char publisher[128];			// [0x13E] (strA) Volume publisher.
	char data_preparer[128];		// [0x1BE] (strA) Data preparer.
	char application[128];			// [0x23E] (strA) Application.

	// For the following fields:
	// - Filenames must be in the root directory.
	// - If empty, fill with all 0x20.
	char copyright_file[37];		// [0x2BE] (strD) Filename of the copyright file.
	char abstract_file[37];			// [0x2E3] (strD) Filename of the abstract file.
	char bibliographic_file[37];		// [0x308] (strD) Filename of the bibliographic file.

	// Timestamps.
	ISO_PVD_DateTime_t btime;		// [0x32D] Volume creation time.
	ISO_PVD_DateTime_t mtime;		// [0x33E] Volume modification time.
	ISO_PVD_DateTime_t exptime;		// [0x34F] Volume expiration time.
	ISO_PVD_DateTime_t efftime;		// [0x360] Volume effective time.

	uint8_t file_structure_version;		// [0x371] Directory records and path table version. (0x01)
	uint8_t reserved4;			// [0x372] 0x00

	uint8_t application_data[512];		// [0x373] Not defined by ISO-9660.
	uint8_t iso_reserved[653];		// [0x573] Reserved by ISO.
} ISO_Primary_Volume_Descriptor;
ASSERT_STRUCT(ISO_Primary_Volume_Descriptor, ISO_SECTOR_SIZE_MODE1_COOKED);
#pragma pack()

/**
 * Volume descriptor.
 *
 * Primary volume descriptor is located at sector 0x10. (0x8000)
 */
#define ISO_VD_MAGIC "CD001"
#define ISO_VD_VERSION 0x01
#define ISO_PVD_LBA 0x10
#define ISO_PVD_ADDRESS_2048	(ISO_PVD_LBA * ISO_SECTOR_SIZE_MODE1_COOKED)
#define ISO_PVD_ADDRESS_2352	(ISO_PVD_LBA * ISO_SECTOR_SIZE_MODE1_RAW)
#define ISO_PVD_ADDRESS_2448	(ISO_PVD_LBA * ISO_SECTOR_SIZE_MODE1_RAW_SUBCHAN)
#pragma pack(1)
typedef union PACKED _ISO_Volume_Descriptor {
	ISO_Volume_Descriptor_Header header;

	struct {
		uint8_t header8[7];
		uint8_t data[2041];
	};

	ISO_Boot_Volume_Descriptor boot;
	ISO_Primary_Volume_Descriptor pri;
} ISO_Volume_Descriptor;
ASSERT_STRUCT(ISO_Volume_Descriptor, ISO_SECTOR_SIZE_MODE1_COOKED);
#pragma pack()

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

/**
 * UDF volume descriptors.
 *
 * Reference: https://wiki.osdev.org/UDF
 */
#define UDF_VD_BEA01 "BEA01"
#define UDF_VD_NSR01 "NSR01"	/* UDF 1.00 */
#define UDF_VD_NSR02 "NSR02"	/* UDF 1.50 */
#define UDF_VD_NSR03 "NSR03"	/* UDF 2.00 */
#define UDF_VD_BOOT2 "BOOT2"
#define UDF_VD_TEA01 "TEA01"

/**
 * CD-i information.
 * Reference: https://github.com/roysmeding/cditools/blob/master/cdi.py
 */
#define CDi_VD_MAGIC "CD-I "
#define CDi_VD_VERSION 0x01

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_ISO9660_H__ */
