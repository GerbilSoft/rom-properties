/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * nds_structs.h: Nintendo DS(i) data structures.                          *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_NDS_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_NDS_STRUCTS_H__

#include "common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Nintendo DS ROM header.
 * This matches the ROM header format exactly.
 * Reference: http://problemkaputt.de/gbatek.htm#dscartridgeheader
 * 
 * All fields are little-endian.
 * NOTE: Strings are NOT null-terminated!
 */
#pragma pack(1)
#define NDS_RomHeader_SIZE 4096
typedef struct PACKED _NDS_RomHeader {
	char title[12];
	union {
		char id6[6];	// Game code. (ID6)
		struct {
			char id4[4];		// Game code. (ID4)
			char company[2];	// Company code.
		};
	};

	// 0x12
	uint8_t unitcode;	// 00h == NDS, 02h == NDS+DSi, 03h == DSi only
	uint8_t enc_seed_select;
	uint8_t device_capacity;
	uint8_t reserved1[7];
	uint8_t reserved2_dsi;
	uint8_t nds_region;	// 0x00 == normal, 0x80 == China, 0x40 == Korea
	uint8_t rom_version;
	uint8_t autostart;

	// 0x20
	struct {
		uint32_t rom_offset;
		uint32_t entry_address;
		uint32_t ram_address;
		uint32_t size;
	} arm9;
	struct {
		uint32_t rom_offset;
		uint32_t entry_address;
		uint32_t ram_address;
		uint32_t size;
	} arm7;

	// 0x40
	uint32_t fnt_offset;	// File Name Table offset
	uint32_t fnt_size;	// File Name Table size
	uint32_t fat_offset;
	uint32_t fat_size;

	// 0x50
	uint32_t arm9_overlay_offset;
	uint32_t arm9_overlay_size;
	uint32_t arm7_overlay_offset;
	uint32_t arm7_overlay_size;

	// 0x60
	uint32_t cardControl13;	// Port 0x40001A4 setting for normal commands (usually 0x00586000)
	uint32_t cardControlBF;	// Port 0x40001A4 setting for KEY1 commands (usually 0x001808F8)

	// 0x68
	uint32_t icon_offset;
	uint16_t secure_area_checksum;		// CRC32 of 0x0020...0x7FFF
	uint16_t secure_area_delay;		// Delay, in 131 kHz units (0x051E=10ms, 0x0D7E=26ms)

	uint32_t arm9_auto_load_list_ram_address;
	uint32_t arm7_auto_load_list_ram_address;

	uint64_t secure_area_disable;

	// 0x80
	uint32_t total_used_rom_size;		// Excluding DSi area
	uint32_t rom_header_size;		// Usually 0x4000
	uint8_t reserved3[0x38];
	uint8_t nintendo_logo[0x9C];		// GBA-style Nintendo logo
	uint16_t nintendo_logo_checksum;	// CRC16 of nintendo_logo[] (always 0xCF56)
	uint16_t header_checksum;		// CRC16 of 0x0000...0x015D

	// 0x160
	struct {
		uint32_t rom_offset;
		uint32_t size;
		uint32_t ram_address;
	} debug;

	// 0x16C
	uint8_t reserved4[4];
	uint8_t reserved5[0x10];

	/** DSi-specific **/

	// 0x180 [memory settings] [TODO]
	uint32_t dsi_mbk[12];

	// 0x1B0
	uint32_t dsi_region;	// DSi region flags.

	// 0x1B4 [TODO]
	uint8_t dsi_reserved1[124];

	// 0x230
	uint32_t dsi_title_id;
	uint8_t dsi_filetype;		// See DSi_FILETYPE.
	uint8_t dsi_reserved2[3];	// 0x00, 0x03, 0x00

	// 0x238
	uint32_t dsi_sd_public_sav_size;
	uint32_t dsi_sd_private_sav_size;

	// 0x240
	uint8_t dsi_reserved3[176];

	// 0x2F0
	uint8_t age_ratings[0x10];	// Age ratings. [TODO]

	// 0x300
	// TODO: More DSi header entries.
	// Reference: http://problemkaputt.de/gbatek.htm#dsicartridgeheader
	uint8_t dsi_reserved_end[3328];
} NDS_RomHeader;
#pragma pack()

/**
 * Nintendo DSi file type.
 */
typedef enum {
	DSi_FTYPE_CARTRIDGE		= 0x00,
	DSi_FTYPE_DSiWARE		= 0x04,
	DSi_FTYPE_SYSTEM_FUN_TOOL	= 0x05,
	DSi_FTYPE_NONEXEC_DATA		= 0x0F,
	DSi_FTYPE_SYSTEM_BASE_TOOL	= 0x15,
	DSi_FTYPE_SYSTEM_MENU		= 0x17,
} DSi_FILETYPE;

// NDS_IconTitleData version.
typedef enum {
	// Original icon version.
	NDS_ICON_VERSION_ORIGINAL	= 0x0001,

	// Added Chinese title.
	NDS_ICON_VERSION_ZH		= 0x0002,

	// Added Korean title.
	NDS_ICON_VERSION_ZH_KO		= 0x0003,

	// Added DSi animated icon.
	NDS_ICON_VERSION_DSi		= 0x0103,
} NDS_IconTitleData_Version;

/**
 * Nintendo DS icon and title struct.
 * Reference: http://problemkaputt.de/gbatek.htm#dscartridgeicontitle
 *
 * All fields are little-endian.
 */
#define NDS_IconTitleData_SIZE 9152
#pragma pack(1)
typedef struct PACKED _NDS_IconTitleData{
	uint16_t version;		// known values: 0x0001, 0x0002, 0x0003, 0x0103
	uint16_t crc16[4];		// CRC16s for the four known versions.
	uint8_t reserved1[0x16];

	uint8_t icon_data[0x200];	// Icon data. (32x32, 4x4 tiles, 4-bit color)
	uint16_t icon_pal[0x10];	// Icon palette. (16-bit color; color 0 is transparent)

	// [0x240] Titles. (128 characters each; UTF-16)
	// Order: JP, EN, FR, DE, IT, ES, ZH (v0002), KR (v0003)
	char16_t title[8][128];

	// [0xA40] Reserved space, possibly for other titles.
	char reserved2[0x800];

	// [0x1240] DSi animated icons (v0103h)
	// Icons use the same format as DS icons.
	uint8_t dsi_icon_data[8][0x200];	// Icon data. (Up to 8 frames)
	uint16_t dsi_icon_pal[8][0x10];		// Icon palettes.
	uint16_t dsi_icon_seq[0x40];		// Icon animation sequence.
} NDS_IconTitleData;
#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_NDS_STRUCTS_H__ */
