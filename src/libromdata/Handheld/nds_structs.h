/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * nds_structs.h: Nintendo DS(i) data structures.                          *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>
#include "common.h"
#include "nintendo_system_id.h"

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
typedef struct _NDS_RomHeader {
	char title[12];
	// Some compilers pad this structure to a multiple of 4 bytes
#pragma pack(1)
	union RP_PACKED {
		char id6[6];	// Game code. (ID6)
		struct RP_PACKED {
			char id4[4];		// Game code. (ID4)
			char company[2];	// Company code.
		};
	};
#pragma pack()

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
	struct {
		// 0x180 [memory settings]
		uint32_t global_mbk[5];		// Global MBK1..MBK5 settings.
		uint32_t arm9_mbk[3];		// Local ARM9 MBK6..MBK8 settings.
		uint32_t arm7_mbk[3];		// Local ARM7 MBK6..MBK8 settings.
		uint8_t arm9_mbk9_master[3];	// Global MBK9 setting, WRAM slot master.
		uint8_t unknown;		// Usually 0x03, but System Menu has 0xFC, System Settings has 0x00.

		// 0x1B0
		uint32_t region_code;		// DSi region code. (See DSi_Region.)
		uint32_t access_control;	// ???
		uint32_t arm7_scfg_mask;
		uint8_t reserved1[3];		// Unknown flags. (always 0)
		uint8_t flags;			// See DSi_Flags.

		// 0x1C0
		struct {
			uint32_t rom_offset;	// Usually 0xXX03000h, where XX is the 1MB boundary after the NDS area.
			uint32_t reserved;	// Zero-filled.
			uint32_t load_address;
			uint32_t size;
		} arm9i;
		struct {
			uint32_t rom_offset;
			uint32_t param_addr;	// Pointer to base address where structures are passed to the title.
			uint32_t load_address;
			uint32_t size;
		} arm7i;

		// 0x1E0 [digest offsets]
		struct {
			uint32_t ntr_region_offset;	// Usually the same as ARM9 rom_offset, 0x0004000
			uint32_t ntr_region_length;
			uint32_t twl_region_offset;	// Usually the same as ARM9i rom_offset, 0xXX03000
			uint32_t twl_region_length;
			uint32_t sector_hashtable_offset;	// SHA1 HMACs on all sectors
			uint32_t sector_hashtable_length;	// in the above NTR+TWL regions.
			uint32_t block_hashtable_offset;	// SHA1 HMACs on each N entries
			uint32_t block_hashtable_length;	// in the above Sector Hashtable.
			uint32_t sector_size;			// e.g. 0x400 bytes per sector
			uint32_t block_sector_count;		// e.g. 0x20 sectors per block
		} digest;

		// 0x208
		uint32_t icon_title_size;	// Size of icon/title. (usually 0x23C0)
		uint32_t reserved2;		// 00 00 01 00
		uint32_t total_used_rom_size;	// *INCLUDING* DSi area
		uint32_t reserved3[3];		// 00 00 00 00; 84 D0 04 00; 2C 05 00 00

		// 0x220
		uint32_t modcrypt1_offset;	// Usually the same as ARM9i rom_offset, 0xXX03000
		uint32_t modcrypt1_size;	// Usually min(0x4000, ARM9i ((size + 0x0F) & ~0x0F))
		uint32_t modcrypt2_offset;	// 0 for none
		uint32_t modcrypt2_size;	// 0 for none

		// 0x230
		Nintendo_TitleID_LE_t title_id;	// [0x230] Title ID

		// 0x238
		uint32_t sd_public_sav_size;
		uint32_t sd_private_sav_size;

		// 0x240
		uint8_t reserved6[176];		// Zero-filled

		// 0x2F0
		uint8_t age_ratings[0x10];	// Age ratings.

		// 0x300
		uint8_t sha1_hmac_arm9[20];	// SHA1 HMAC of ARM9 (with encrypted secure area)
		uint8_t sha1_hmac_arm7[20];	// SHA1 HMAC of ARM7
		uint8_t sha1_hmac_digest_master[20];
		uint8_t sha1_hmac_icon_title[20];
		uint8_t sha1_hmac_arm9i[20];	// decrypted
		uint8_t sha1_hmac_arm7i[20];	// decrypted
		uint8_t reserved7[40];
		uint8_t sha1_hmac_arm9_nosecure[20];	// SHA1 HMAC of ARM9 without 16 KB secure area
		uint8_t reserved8[2636];
		uint8_t debug_args[0x180];	// Zero and unchecked on retail; used for arguments on debug.
		uint8_t rsa_sha1[0x80];		// RSA SHA1 signature on 0x000...0xDFF.
	} dsi;
} NDS_RomHeader;
ASSERT_STRUCT(NDS_RomHeader, 4096);

/**
 * Nintendo DSi region code.
 */
typedef enum {
	DSi_REGION_JAPAN	= (1U << 0),
	DSi_REGION_USA		= (1U << 1),
	DSi_REGION_EUROPE	= (1U << 2),
	DSi_REGION_AUSTRALIA	= (1U << 3),
	DSi_REGION_CHINA	= (1U << 4),
	DSi_REGION_SKOREA	= (1U << 5),
} DSi_Region;

/**
 * Nintendo DSi access control. (0x1B4)
 */
typedef enum {
	DSi_ACCESS_COMMON_KEY			= (1U <<  0),
	DSi_ACCESS_AES_SLOT_B			= (1U <<  1),
	DSi_ACCESS_AES_SLOT_C			= (1U <<  2),
	DSi_ACCESS_SD_CARD			= (1U <<  3),
	DSi_ACCESS_eMMC_ACCESS			= (1U <<  4),
	DSi_ACCESS_GAME_CARD_POWER_ON		= (1U <<  5),
	DSi_ACCESS_SHARED2_FILE			= (1U <<  6),
	DSi_ACCESS_SIGN_JPEG_FOR_LAUNCHER	= (1U <<  7),
	DSi_ACCESS_GAME_CARD_NTR_MODE		= (1U <<  8),
	DSi_ACCESS_SSL_CLIENT_CERT		= (1U <<  9),
	DSi_ACCESS_SIGN_JPEG_FOR_USER		= (1U << 10),
	DSi_ACCESS_PHOTO_READ_ACCESS		= (1U << 11),
	DSi_ACCESS_PHOTO_WRITE_ACCESS		= (1U << 12),
	DSi_ACCESS_SD_CARD_READ_ACCESS		= (1U << 13),
	DSi_ACCESS_SD_CARD_WRITE_ACCESS		= (1U << 14),
	DSi_ACCESS_GAME_CARD_SAVE_READ_ACCESS	= (1U << 15),
	DSi_ACCESS_GAME_CARD_SAVE_WRITE_ACCESS	= (1U << 16),

	DSi_ACCESS_DEBUG_KEY			= (1U << 31),
} DSi_Access;

/**
 * Nintendo DSi Flags. (0x1BF)
 */
typedef enum {
	DSi_FLAGS_TOUCHSCREEN_MODE	= (1U << 0),	// 0 == NDS; 1 == DSi
	DSi_FLAGS_REQUIRE_EULA		= (1U << 1),
	DSi_FLAGS_CUSTOM_ICON		= (1U << 2),	// 0 == normal; 1 == banner.sav
	DSi_FLAGS_NINTENDO_WFC		= (1U << 3),	// Show Nintendo WFC icon in launcher
	DSi_FLAGS_DS_WIRELESS		= (1U << 4),	// Show DS Wireless icon in launcher
	DSi_FLAGS_NDS_ICON_SHA1		= (1U << 5),	// NDS cart with icon SHA-1 (DSi FW v1.4+)
	DSi_FLAGS_NDS_HEADER_RSA	= (1U << 6),	// NDS cart with header RSA (DSi FW v1.0+)
	DSi_FLAGS_DEVELOPER		= (1U << 7),	// Developer application
} DSi_Flags;

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
} DSi_FileType;

/**
 * Nintendo DSi: Country indexes for age_ratings[].
 */
typedef enum {
	DSi_RATING_JAPAN = 0,		// CERO
	DSi_RATING_USA = 1,		// ESRB
	DSi_RATING_GERMANY = 3,		// USK
	DSi_RATING_PEGI = 4,		// PEGI
	DSi_RATING_FINLAND = 5,		// MEKU?
	DSi_RATING_PORTUGAL = 6,	// Modified PEGI
	DSi_RATING_BRITAIN = 7,		// BBFC
	DSi_RATING_AUSTRALIA = 8,	// AGCB
	DSi_RATING_SOUTH_KOREA = 9,	// GRB
} DSi_RatingCountry;

// NDS_IconTitleData version.
typedef enum {
	NDS_ICON_VERSION_ORIGINAL	= 0x0001,	// Original
	NDS_ICON_VERSION_HANS		= 0x0002,	// +HANS
	NDS_ICON_VERSION_HANS_KO	= 0x0003,	// +KO
	NDS_ICON_VERSION_DSi		= 0x0103,	// +DSi
} NDS_IconTitleData_Version;

// NDS_IconTitleData sizes.
typedef enum {
	NDS_ICON_SIZE_ORIGINAL		= 0x0840,	// Original
	NDS_ICON_SIZE_HANS		= 0x0940,	// +HANS
	NDS_ICON_SIZE_HANS_KO		= 0x0A40,	// +KO
	NDS_ICON_SIZE_DSi		= 0x23C0,	// +DSi
} NDS_IconTitleData_Size;

// Icon/title languages.
typedef enum {
	NDS_LANG_JAPANESE	= 0,
	NDS_LANG_ENGLISH	= 1,
	NDS_LANG_FRENCH		= 2,
	NDS_LANG_GERMAN		= 3,
	NDS_LANG_ITALIAN	= 4,
	NDS_LANG_SPANISH	= 5,
	NDS_LANG_CHINESE_SIMP	= 6,	// Simplified Chinese
	NDS_LANG_KOREAN		= 7,

	NDS_LANG_MAX
} NDS_Language_ID;

/**
 * Nintendo DS icon and title struct.
 * Reference: http://problemkaputt.de/gbatek.htm#dscartridgeicontitle
 *
 * All fields are little-endian.
 */
typedef struct _NDS_IconTitleData{
	uint16_t version;		// known values: 0x0001, 0x0002, 0x0003, 0x0103
	uint16_t crc16[4];		// CRC16s for the four known versions.
	uint8_t reserved1[0x16];

	uint8_t icon_data[0x200];	// Icon data. (32x32, 4x4 tiles, 4-bit color)
	uint16_t icon_pal[0x10];	// Icon palette. (16-bit color; color 0 is transparent)

	// [0x240] Titles. (128 characters each; UTF-16LE)
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
ASSERT_STRUCT(NDS_IconTitleData, 9152);

#ifdef __cplusplus
}
#endif
