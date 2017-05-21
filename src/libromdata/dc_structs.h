/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * dc_structs.h: Sega Dreamcast data structures.                           *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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

/**
 * References:
 * - http://mc.pp.se/dc/vms/fileheader.html
 * - http://mc.pp.se/dc/vms/vmi.html
 * - http://mc.pp.se/dc/vms/flashmem.html
 */

#ifndef __ROMPROPERTIES_LIBROMDATA_DC_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_DC_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// VMS blocks are 512 bytes.
#define DC_VMS_BLOCK_SIZE 512

/**
 * ICONDATA_VMS header.
 * Found at the top of .VMS files used as VMU icons.
 *
 * Reference: http://mc.pp.se/dc/vms/icondata.html
 *
 * All fields are in little-endian.
 */
#pragma pack(1)
typedef struct PACKED _DC_VMS_ICONDATA_Header {
	char vms_description[16];	// Shift-JIS; space-padded.
	uint32_t mono_icon_addr;	// Address of monochrome icon.
	uint32_t color_icon_addr;	// Address of color icon.
} DC_VMS_ICONDATA_Header;
#pragma pack()
ASSERT_STRUCT(DC_VMS_ICONDATA_Header, 24);

/** VMS header **/

/**
 * Dreamcast VMS header. (.vms files)
 * Reference: http://mc.pp.se/dc/vms/fileheader.html
 *
 * All fields are in little-endian.
 *
 * NOTE: Strings are NOT null-terminated!
 */
#pragma pack(1)
typedef union PACKED _DC_VMS_Header {
	struct {
		char vms_description[16];	// Shift-JIS; space-padded.
		char dc_description[32];	// Shift-JIS; space-padded.
		char application[16];		// Shift-JIS; NULL-padded.
		uint16_t icon_count;
		uint16_t icon_anim_speed;
		uint16_t eyecatch_type;
		uint16_t crc;
		uint32_t data_size;		// Ignored for game files.
		uint8_t reserved[20];
	};
	DC_VMS_ICONDATA_Header icondata_vms;	// ICONDATA_VMS header.

	// DCI is 32-bit byteswapped.
	// We need a 32-bit accessor in order to
	// avoid strict aliasing issues.
	uint32_t dci_dword[96/4];

	// Icon palette and icon bitmaps are located
	// immediately after the VMS header, followed
	// by the eyecatch (if present).
} DC_VMS_Header;
#pragma pack()
ASSERT_STRUCT(DC_VMS_Header, 96);

/**
 * Graphic eyecatch type.
 */
typedef enum {
	DC_VMS_EYECATCH_NONE		= 0,
	DC_VMS_EYECATCH_ARGB4444	= 1,
	DC_VMS_EYECATCH_CI8		= 2,
	DC_VMS_EYECATCH_CI4		= 3,
} DC_VMS_Eyecatch_Type;

// Icon and eyecatch sizes.
#define DC_VMS_ICON_W 32
#define DC_VMS_ICON_H 32
#define DC_VMS_EYECATCH_W 72
#define DC_VMS_EYECATCH_H 56

// Some monochrome ICONDATA_VMS files are only 160 bytes.
// TODO: Is there an equivalent for color icons?
#define DC_VMS_ICONDATA_MONO_MINSIZE 160
#define DC_VMS_ICONDATA_MONO_ICON_SIZE ((DC_VMS_ICON_W * DC_VMS_ICON_H) / 8)

// Icon and eyecatch data sizes.
#define DC_VMS_ICON_PALETTE_SIZE (16 * 2)
#define DC_VMS_ICON_DATA_SIZE ((DC_VMS_ICON_W * DC_VMS_ICON_H) / 2)
#define DC_VMS_EYECATCH_ARGB4444_DATA_SIZE ((DC_VMS_EYECATCH_W * DC_VMS_EYECATCH_H) * 2)
#define DC_VMS_EYECATCH_CI8_PALETTE_SIZE (256 * 2)
#define DC_VMS_EYECATCH_CI8_DATA_SIZE (DC_VMS_EYECATCH_W * DC_VMS_EYECATCH_H)
#define DC_VMS_EYECATCH_CI4_PALETTE_SIZE (16 * 2)
#define DC_VMS_EYECATCH_CI4_DATA_SIZE ((DC_VMS_EYECATCH_W * DC_VMS_EYECATCH_H) / 2)

// Filename length.
#define DC_VMS_FILENAME_LENGTH 12

/**
 * Dreamcast VMI timestamp.
 * Values are stored in binary format.
 *
 * Reference: http://mc.pp.se/dc/vms/fileheader.html
 *
 * All fields are in little-endian.
 */
#pragma pack(1)
typedef struct PACKED _DC_VMI_Timestamp {
	uint16_t year;		// Year (exact value)
	uint8_t mon;		// Month (1-12)
	uint8_t mday;		// Day of month (1-31)
	uint8_t hour;		// Hour (0-23)
	uint8_t min;		// Minute (0-59)
	uint8_t sec;		// Second (0-59)
	uint8_t wday;		// Day of week (0=Sunday, 6=Saturday)
} DC_VMI_Timestamp;
#pragma pack()

/**
 * Dreamcast VMI header. (.vmi files)
 * Reference: http://mc.pp.se/dc/vms/fileheader.html
 *
 * All fields are in little-endian.
 *
 * NOTE: Strings are NOT null-terminated!
 */
#pragma pack(1)
typedef struct PACKED _DC_VMI_Header {
	// Very primitive checksum.
	// First four bytes of vms_resource_name,
	// ANDed with 0x53454741 ("SEGA").
	uint8_t checksum[4];

	char description[32];		// Shift-JIS; NULL-padded.
	char copyright[32];		// Shift-JIS; NULL-padded.

	DC_VMI_Timestamp ctime;		// Creation time.
	uint16_t vmi_version;		// VMI version. (0)
	uint16_t file_number;		// File number. (1)
	char vms_resource_name[8];	// .VMS filename, without the ".VMS".
	char vms_filename[DC_VMS_FILENAME_LENGTH];	// Filename on the VMU.
	uint16_t mode;			// See DC_VMI_Mode.
	uint16_t reserved;		// Set to 0.
	uint32_t filesize;		// .VMS file size, in bytes.
} DC_VMI_Header;
#pragma pack()
ASSERT_STRUCT(DC_VMI_Header, 108);

/**
 * DC_VMI_Header.mode
 */
typedef enum {
	// Copy protection.
	DC_VMI_MODE_PROTECT_COPY_OK        = (0 << 0),
	DC_VMI_MODE_PROTECT_COPY_PROTECTED = (1 << 0),
	DC_VMI_MODE_PROTECT_MASK           = (1 << 0),

	// File type.
	DC_VMI_MODE_FTYPE_DATA = (0 << 1),
	DC_VMI_MODE_FTYPE_GAME = (1 << 1),
	DC_VMI_MODE_FTYPE_MASK = (1 << 1),
} DC_VMI_Mode;

/**
 * Dreamcast VMS BCD timestamp.
 * Values are stored in BCD format.
 *
 * Reference: http://mc.pp.se/dc/vms/flashmem.html
 */
#pragma pack(1)
typedef struct PACKED _DC_VMS_BCD_Timestamp {
	uint8_t century;	// Century.
	uint8_t year;		// Year.
	uint8_t mon;		// Month (1-12)
	uint8_t mday;		// Day of month (1-31)
	uint8_t hour;		// Hour (0-23)
	uint8_t min;		// Minute (0-59)
	uint8_t sec;		// Second (0-59)
	uint8_t wday;		// Day of week (0=Monday, 6=Sunday)
} DC_VMS_BCD_Timestamp;
#pragma pack()

/**
 * Dreamcast VMS directory entry.
 * Found at the top of DCI files and in the directory
 * table of raw VMU dumps.
 *
 * Reference: http://mc.pp.se/dc/vms/flashmem.html
 *
 * All fields are in little-endian.
 *
 * NOTE: Strings are NOT null-terminated!
 */
#pragma pack(1)
typedef struct PACKED _DC_VMS_DirEnt {
	uint8_t filetype;	// See DC_VMS_DirEnt_FType.
	uint8_t protect;	// See DC_VMS_DirEnt_Protect.
	uint16_t address;	// First block number.
	char filename[DC_VMS_FILENAME_LENGTH];

	DC_VMS_BCD_Timestamp ctime;	// Creation time. (BCD)
	uint16_t size;		// Size, in blocks.
	uint16_t header_addr;	// Offset of header (in blocks) from file start.
	uint8_t reserved[4];	// Reserved. (all zero)
} DC_VMS_DirEnt;
#pragma pack()
ASSERT_STRUCT(DC_VMS_DirEnt, 32);

/**
 * DC_VMS_DirEnt.filetype
 */
typedef enum {
	DC_VMS_DIRENT_FTYPE_NONE = 0x00,
	DC_VMS_DIRENT_FTYPE_DATA = 0x33,
	DC_VMS_DIRENT_FTYPE_GAME = 0xCC,
} DC_VMS_DirEnt_FType;

/**
 * DC_VMS_DirEnt.protect
 */
typedef enum {
	DC_VMS_DIRENT_PROTECT_COPY_OK        = 0x00,
	DC_VMS_DIRENT_PROTECT_COPY_PROTECTED = 0xFF,
} DC_VMS_DirEnt_Protect;

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_DC_STRUCTS_H__ */
