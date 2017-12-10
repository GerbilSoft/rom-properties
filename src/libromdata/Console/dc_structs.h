/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * dc_structs.h: Sega Dreamcast data structures.                           *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_DC_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_DC_STRUCTS_H__

/**
 * References:
 * - http://mc.pp.se/dc/vms/fileheader.html
 * - http://mc.pp.se/dc/vms/vmi.html
 * - http://mc.pp.se/dc/vms/flashmem.html
 * - http://mc.pp.se/dc/ip0000.bin.html
 * - http://mc.pp.se/dc/ip.bin.html
 */

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

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
typedef struct PACKED _DC_VMS_ICONDATA_Header {
	char vms_description[16];	// Shift-JIS; space-padded.
	uint32_t mono_icon_addr;	// Address of monochrome icon.
	uint32_t color_icon_addr;	// Address of color icon.
} DC_VMS_ICONDATA_Header;
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
typedef struct PACKED _DC_VMI_Timestamp {
	uint16_t year;		// Year (exact value)
	uint8_t mon;		// Month (1-12)
	uint8_t mday;		// Day of month (1-31)
	uint8_t hour;		// Hour (0-23)
	uint8_t min;		// Minute (0-59)
	uint8_t sec;		// Second (0-59)
	uint8_t wday;		// Day of week (0=Sunday, 6=Saturday)
} DC_VMI_Timestamp;

/**
 * Dreamcast VMI header. (.vmi files)
 * Reference: http://mc.pp.se/dc/vms/fileheader.html
 *
 * All fields are in little-endian.
 *
 * NOTE: Strings are NOT null-terminated!
 */
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

/** Disc images **/

/**
 * IP0000.BIN
 * This is located in the boot sector of GD-ROM track 3.
 *
 * Reference: http://mc.pp.se/dc/ip0000.bin.html
 *
 * All fields are in little-endian.
 */
#define DC_IP0000_BIN_HW_ID	"SEGA SEGAKATANA "
#define DC_IP0000_BIN_MAKER_ID	"SEGA ENTERPRISES"
typedef struct PACKED _DC_IP0000_BIN_t {
	char hw_id[16];			// "SEGA SEGAKATANA "
	char maker_id[16];		// "SEGA ENTERPRISES"
	char device_info[16];		// "1234 GD-ROM1/1  "
	char area_symbols[8];		// "JUE     " (position-dependent)
	char peripherals[8];		// 7-digit hex string, use DC_IP0000_BIN_Peripherals to decode.
	char product_number[10];	// "HDR-nnnn"
	char product_version[6];	// "V1.000"
	char release_date[16];		// "YYYYMMDD        "
	char boot_filename[16];		// "1ST_READ.BIN    "
	char publisher[16];		// Name of the company that produced the disc.
	char title[128];		// Software title. (TODO: Encoding)
} DC_IP0000_BIN_t;
ASSERT_STRUCT(DC_IP0000_BIN_t, 256);

/**
 * Dreamcast peripherals bitfield.
 * For most items, indicates support for that feature.
 * For Windows CE, indicates the game uses it.
 *
 * For controller buttons, indicates a minimum requirement.
 * Example: If "Z" is listed, then the game must have a
 * controller with a "Z" button; otherwise, it won't work.
 *
 * Reference: http://mc.pp.se/dc/ip0000.bin.html
 */
typedef enum {
	DCP_WINDOWS_CE		= (1 << 0),	// Uses Windows CE
	DCP_VGA_BOX		= (1 << 4),	// Supports VGA Box

	// Supported expansion units.
	DCP_EXP_OTHER		= (1 << 8),	// Other expansions.
	DCP_PURU_PURU		= (1 << 9),	// Puru Puru pack (Jump Pack)
	DCP_MICROPHONE		= (1 << 10),	// Microphone
	DCP_MEMORY_CARD		= (1 << 11),	// Memory Card (VMU)

	// Controller requirements.
	// If any of these bits are set, the game *requires*
	// a controller with the specified functionality,
	DCP_CTRL_START_A_B_DPAD	= (1 << 12),	// Start, A, B, D-Pad
	DCP_CTRL_C		= (1 << 13),	// C button
	DCP_CTRL_D		= (1 << 14),	// D button
	DCP_CTRL_X		= (1 << 15),	// X button
	DCP_CTRL_Y		= (1 << 16),	// Y button
	DCP_CTRL_Z		= (1 << 17),	// Z button
	DCP_CTRL_DPAD_2		= (1 << 18),	// Second D-Pad
	DCP_CTRL_ANALOG_RT	= (1 << 19),	// Analog R trigger
	DCP_CTRL_ANALOG_LT	= (1 << 20),	// Analog L trigger
	DCP_CTRL_ANALOG_H1	= (1 << 21),	// Analog horizontal controller
	DCP_CTRL_ANALOG_V1	= (1 << 22),	// Analog vertical controller
	DCP_CTRL_ANALOG_H2	= (1 << 23),	// Analog horizontal controller #2
	DCP_CTRL_ANALOG_V2	= (1 << 24),	// Analog vertical controller #2

	// Optional expansion peripherals.
	DCP_CTRL_GUN		= (1 << 25),	// Light Gun
	DCP_CTRL_KEYBOARD	= (1 << 26),	// Keyboard
	DCP_CTRL_MOUSE		= (1 << 27),	// Mouse
} DC_IP0000_BIN_Peripherals;

/**
 * IP.BIN
 * This is located in the boot sector of GD-ROM track 3.
 *
 * Reference: http://mc.pp.se/dc/ip.bin.html
 *
 * All fields are in little-endian.
 */
typedef struct PACKED _DC_IP_BIN_t {
	DC_IP0000_BIN_t meta;			// Meta information. (IP0000.BIN)
	uint8_t toc[0x200];			// Table of contents.
	uint8_t license_screen_code[0x3400];	// License screen code.

	// Area symbols. (region lockout)
	// Contains longer strings indicating the valid areas.
	// Must match area_symbols in DC_IP0000_BIN_t.
	// NOTE: The first four bytes are a branch instruction.
	struct {
		uint16_t branch[2];
		char text[28];
	} area_syms[8];

	// Additional bootstrap code.
	uint8_t bootstrap1[0x2800];
	uint8_t bootstrap2[0x2000];
} DC_IP_BIN_t;
ASSERT_STRUCT(DC_IP_BIN_t, 0x8000);

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_DC_STRUCTS_H__ */
