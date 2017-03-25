/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * n3ds_structs.h: Nintendo 3DS data structures.                           *
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

// References:
// - https://www.3dbrew.org/wiki/SMDH
// - https://github.com/devkitPro/3dstools/blob/master/src/smdhtool.cpp
// - https://3dbrew.org/wiki/3DSX_Format
// - https://3dbrew.org/wiki/CIA

#ifndef __ROMPROPERTIES_LIBROMDATA_N3DS_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_N3DS_STRUCTS_H__

#include "common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Nintendo 3DS SMDH title struct.
 * All fields are UTF-16LE.
 * NOTE: Strings may not be NULL-terminated!
 */
#pragma pack(1)
typedef struct PACKED _N3DS_SMDH_Title_t {
	char16_t desc_short[64];
	char16_t desc_long[128];
	char16_t publisher[64];
} N3DS_SMDH_Title_t;
#pragma pack()
ASSERT_STRUCT(N3DS_SMDH_Title_t, 512);

/**
 * Nintendo 3DS SMDH settings struct.
 * All fields are little-endian.
 */
#pragma pack(1)
typedef struct PACKED _N3DS_SMDH_Settings_t {
	uint8_t ratings[16];	// Region-specific age ratings.
	uint32_t region_code;	// Region code. (bitfield)
	uint32_t match_maker_id;
	uint64_t match_maker_bit_id;
	uint32_t flags;
	uint16_t eula_version;
	uint8_t reserved[2];
	uint32_t animation_default_frame;
	uint32_t cec_id;	// StreetPass ID
} N3DS_SMDH_Settings_t;
#pragma pack()
ASSERT_STRUCT(N3DS_SMDH_Settings_t, 48);

/**
 * Age rating indexes.
 */
typedef enum {
	N3DS_RATING_JAPAN	= 0,	// CERO
	N3DS_RATING_USA		= 1,	// ESRB
	N3DS_RATING_GERMANY	= 3,	// USK
	N3DS_RATING_PEGI	= 4,	// PEGI
	N3DS_RATING_PORTUGAL	= 6,	// PEGI (Portugal)
	N3DS_RATING_BRITAIN	= 7,	// BBFC
	N3DS_RATING_AUSTRALIA	= 8,	// ACB
	N3DS_RATING_SOUTH_KOREA	= 9,	// GRB
	N3DS_RATING_TAIWAN	= 10,	// CGSRR
} N3DS_Age_Rating_Region;

/**
 * Region code bits.
 */
typedef enum {
	N3DS_REGION_JAPAN	= (1 << 0),
	N3DS_REGION_USA		= (1 << 1),
	N3DS_REGION_EUROPE	= (1 << 2),
	N3DS_REGION_AUSTRALIA	= (1 << 3),
	N3DS_REGION_CHINA	= (1 << 4),
	N3DS_REGION_SOUTH_KOREA	= (1 << 5),
	N3DS_REGION_TAIWAN	= (1 << 6),
} N3DS_Region_Code;

/**
 * Flag bits.
 */
typedef enum {
	N3DS_FLAG_VISIBLE		= (1 << 0),
	N3DS_FLAG_AUTOBOOT		= (1 << 1),
	N3DS_FLAG_USE_3D		= (1 << 2),
	N3DS_FLAG_REQUIRE_EULA		= (1 << 3),
	N3DS_FLAG_AUTOSAVE		= (1 << 4),
	N3DS_FLAG_EXT_BANNER		= (1 << 5),
	N3DS_FLAG_AGE_RATING_REQUIRED	= (1 << 6),
	N3DS_FLAG_HAS_SAVE_DATA		= (1 << 7),
	N3DS_FLAG_RECORD_USAGE		= (1 << 8),
	N3DS_FLAG_DISABLE_SD_BACKUP	= (1 << 10),
	N3DS_FLAG_NEW3DS_ONLY		= (1 << 12),
} N3DS_SMDH_Flags;

/**
 * Nintendo 3DS SMDH header.
 * SMDH files contain a description of the title as well
 * as large and small icons.
 * Reference: https://www.3dbrew.org/wiki/SMDH
 *
 * All fields are little-endian.
 * NOTE: Strings may not be NULL-terminated!
 */
#pragma pack(1)
#define N3DS_SMDH_HEADER_MAGIC "SMDH"
typedef struct PACKED _N3DS_SMDH_Header_t {
	char magic[4];		// "SMDH"
	uint16_t version;
	uint8_t reserved1[2];
	N3DS_SMDH_Title_t titles[16];
	N3DS_SMDH_Settings_t settings;
	uint8_t reserved2[8];
} N3DS_SMDH_Header_t;
#pragma pack()
ASSERT_STRUCT(N3DS_SMDH_Header_t, 8256);

/**
 * Language IDs.
 * These are indexes in N3DS_SMDH_Header_t.titles[].
 */
typedef enum {
	N3DS_LANG_JAPANESE	= 0,
	N3DS_LANG_ENGLISH	= 1,
	N3DS_LANG_FRENCH	= 2,
	N3DS_LANG_GERMAN	= 3,
	N3DS_LANG_ITALIAN	= 4,
	N3DS_LANG_SPANISH	= 5,
	N3DS_LANG_CHINESE_SIMP	= 6,
	N3DS_LANG_KOREAN	= 7,
	N3DS_LANG_DUTCH		= 8,
	N3DS_LANG_PORTUGUESE	= 9,
	N3DS_LANG_RUSSIAN	= 10,
	N3DS_LANG_CHINESE_TRAD	= 11,
} N3DS_Language_ID;

/**
 * Nintendo 3DS SMDH icon data.
 * NOTE: Assumes RGB565, though other formats
 * are supposedly usable. (No way to tell what
 * format is being used as of right now.)
 */
#define N3DS_SMDH_ICON_SMALL_W 24
#define N3DS_SMDH_ICON_SMALL_H 24
#define N3DS_SMDH_ICON_LARGE_W 48
#define N3DS_SMDH_ICON_LARGE_H 48
#pragma pack(1)
typedef struct PACKED _N3DS_SMDH_Icon_t {
	uint16_t small[N3DS_SMDH_ICON_SMALL_W * N3DS_SMDH_ICON_SMALL_H];
	uint16_t large[N3DS_SMDH_ICON_LARGE_W * N3DS_SMDH_ICON_LARGE_H];
} N3DS_SMDH_Icon_t;
#pragma pack()
ASSERT_STRUCT(N3DS_SMDH_Icon_t, 0x1680);

/**
 * Nintendo 3DS Homebrew Application header. (.3dsx)
 * Reference: https://3dbrew.org/wiki/3DSX_Format
 *
 * All fields are little-endian.
 */
#define N3DS_3DSX_HEADER_MAGIC "3DSX"
#define N3DS_3DSX_STANDARD_HEADER_SIZE 32
#define N3DS_3DSX_EXTENDED_HEADER_SIZE 44
#pragma pack(1)
typedef struct PACKED _N3DS_3DSX_Header_t {
	// Standard header.
	char magic[4];			// "3DSX"
	uint16_t header_size;		// Header size.
	uint16_t reloc_header_size;	// Relocation header size.
	uint32_t format_version;
	uint32_t flags;
	uint32_t code_segment_size;
	uint32_t rodata_segment_size;
	uint32_t data_segment_size;	// Includes BSS.
	uint32_t bss_segment_size;

	// Extended header. (only valid if header_size > 32)
	uint32_t smdh_offset;
	uint32_t smdh_size;
	uint32_t romfs_offset;
} N3DS_3DSX_Header_t;
#pragma pack()
ASSERT_STRUCT(N3DS_3DSX_Header_t, 44);

/**
 * Nintendo 3DS Installable Archive. (.cia)
 * Reference: https://www.3dbrew.org/wiki/CIA
 *
 * All fields are little-endian.
 */
#pragma pack(1)
typedef struct PACKED _N3DS_CIA_Header_t {
	uint32_t header_size;	// Usually 0x2020.
	uint16_t type;
	uint16_t version;
	uint32_t cert_chain_size;
	uint32_t ticket_size;
	uint32_t tmd_size;
	uint32_t meta_size;	// SMDH at the end of the file if non-zero.
	uint64_t content_size;
	uint8_t content_index[0x2000];	// TODO
} N3DS_CIA_Header_t;
#pragma pack()
ASSERT_STRUCT(N3DS_CIA_Header_t, 0x2020);

// Order of sections within CIA file:
// - CIA header
// - Certificate chain
// - Ticket
// - TMD
// - Content
// - Meta (optional)

/**
 * CIA: Meta section header.
 * All fields are little-endian.
 */
#pragma pack(1)
typedef struct PACKED _N3DS_CIA_Meta_Header_t {
	uint64_t tid_dep_list[48];	// Title ID dependency list.
	uint8_t reserved1[0x180];
	uint32_t core_version;
	uint8_t reserved2[0xFC];

	// Meta header is followed by an SMDH.
} N3DS_CIA_Meta_Header_t;
#pragma pack()
ASSERT_STRUCT(N3DS_CIA_Meta_Header_t, 0x400);

/**
 * Nintendo 3DS cartridge and eMMC header. (NCSD)
 * Reference: https://3dbrew.org/wiki/NCSD
 *
 * All fields are little-endian.
 */
#define N3DS_NCSD_HEADER_MAGIC "NCSD"
#pragma pack(1)
typedef struct PACKED _N3DS_NCSD_Header_t {
	uint8_t signature[0x100];	// RSA-2048 SHA-256 signature

	// [0x100]
	char magic[4];			// "NCSD"
	uint32_t image_size;		// Image size, in media units. (1 media unit = 512 bytes)
	uint64_t media_id;		// Media ID.

	// [0x110] eMMC-specific partition table.
	struct {
		uint8_t fs_type[8];	// [0x110] Partition FS type. (eMMC only)
		uint8_t crypt_type[8];	// [0x118] Partition crypt type. (eMMC only)
	} emmc_part_tbl;

	// [0x120] Partition table.
	struct {
		uint32_t offset;	// Partition offset, in media units.
		uint32_t length;	// Partition length, in media units.
	} partitions[8];

	// [0x160]
	union {
		// CCI-specific. (not present in eMMC)
		struct {
			uint8_t exheader_sha256[32];	// [0x160] Exheader SHA-256 hash
			uint32_t addl_header_size;	// [0x180] Additional header size.
			uint32_t sector_zero_offset;	// [0x184] Sector zero offset.
			uint8_t partition_flags[8];	// [0x188] Partition flags. (see N3DS_NCSD_Partition_Flags)
			uint64_t partition_tid[8];	// [0x190] Partition title IDs.
			uint8_t reserved[0x30];		// [0x1D0]
		} cci;
		// eMMC-specific. (not present in CCI)
		struct {
			uint8_t reserved[0x5E];		// [0x160]
			uint8_t mbr[0x42];		// [0x1BE] Encrypted MBR partition table for TWL partitions.
		} emmc;
	};
} N3DS_NCSD_Header_t;
#pragma pack()
ASSERT_STRUCT(N3DS_NCSD_Header_t, 0x200);

/**
 * NCSD partition index.
 */
typedef enum {
	NCSD_PARTITION_GAME		= 0,
	NCSD_PARTITION_MANUAL		= 1,
	NCSD_PARTITION_DLP		= 2,
	NCSD_PARTITION_N3DS_UPDATE	= 6,
	NCSD_PARTITION_O3DS_UPDATE	= 7,
} N3DS_NCSD_Partition_Index;

/**
 * NCSD partition flags. (byte array indexes)
 */
typedef enum {
	NCSD_PARTITION_FLAG_BACKUP_WRITE_WAIT_TIME	= 0,
	NCSD_PARTITION_FLAG_MEDIA_CARD_DEVICE_SDK3	= 3,
	NCSD_PARTITION_FLAG_MEDIA_PLATFORM_INDEX	= 4,
	NCSD_PARTITION_FLAG_MEDIA_TYPE_INDEX		= 5,
	NCSD_PARTITION_FLAG_MEDIA_UNIT_SIZE		= 6,
	NCSD_PARTITION_FLAG_MEDIA_CARD_DEVICE_SDK2	= 7,
} N3DS_NCSD_Partition_Flags;

/**
 * NCSD: Card Info Header.
 * Reference: https://3dbrew.org/wiki/NCSD
 *
 * All fields are little-endian.
 */
#pragma pack(1)
typedef struct PACKED _N3DS_NCSD_Card_Info_Header_t {
	uint32_t card2_writable_address;	// CARD2: Writable address, in media units. (CARD1: Always 0xFFFFFFFF)
	uint32_t card_info_bitmask;
	uint8_t reserved1[0x108];
	uint16_t title_version;
	uint16_t card_revision;			// FIXME: May be uint8_t.
	uint8_t reserved2[0xCEC];		// FIXME: 3dbrew says 0xCEE, but that goes over by 2.
	uint8_t card_seed_keyY[0x10];		// First u64 is the media ID. (same as first NCCH partition ID)
	uint8_t enc_card_seed[0x10];		// Encrypted card seed. (AES-CCM, keyslot 0x3B for retail cards)
	uint8_t card_seed_aes_mac[0x10];
	uint8_t card_seed_nonce[0x0C];
	uint8_t reserved3[0xC4];
	// Card Info Header is followed by a copy of the
	// first partition's NCCH header.
} N3DS_NCSD_Card_Info_Header_t;
#pragma pack()
ASSERT_STRUCT(N3DS_NCSD_Card_Info_Header_t, 0xF00);

/**
 * Nintendo 3DS NCCH header.
 * This version does not have the 256-byte RSA-2048 signature.
 * Reference: https://3dbrew.org/wiki/NCSD
 *
 * All fields are little-endian.
 */
#define N3DS_NCCH_HEADER_MAGIC "NCCH"
#pragma pack(1)
typedef struct PACKED _N3DS_NCCH_Header_NoSig_t {
	// NOTE: Addresses are relative to the version *with* a signature.
	char magic[4];				// [0x100] "NCCH"
	uint32_t content_size;			// [0x104] Content size, in media units. (1 media unit = 512 bytes)
	uint64_t partition_id;			// [0x108] Partition ID.
	char maker_code[2];			// [0x110] Maker code.
	uint16_t version;			// [0x112] Version.
	uint32_t fw96lock;			// [0x114] Used by FIRM 9.6.0-X to verify the content lock seed.
	uint64_t program_id;			// [0x118] Program ID.
	uint8_t reserved1[0x10];		// [0x120]
	uint8_t logo_region_hash[0x20];		// [0x130] Logo region SHA-256 hash. (SDK 5+)
	char product_code[0x10];		// [0x150] ASCII product code, e.g. "CTR-P-CTAP"
	uint8_t exheader_hash[0x20];		// [0x160] Extended header SHA-256 hash.
	uint32_t exheader_size;			// [0x180] Extended header size, in bytes.
	uint8_t reserved2[4];			// [0x184]
	uint8_t flags[8];			// [0x188] Flags. (see N3DS_NCCH_Flags)
	uint32_t plain_region_offset;		// [0x190] Plain region offset, in media units.
	uint32_t plain_region_size;		// [0x194] Plain region size, in media units.
	uint32_t logo_region_offset;		// [0x198] Logo region offset, in media units. (SDK 5+)
	uint32_t logo_region_size;		// [0x19C] Logo region size, in media units. (SDK 5+)
	uint32_t exefs_offset;			// [0x1A0] ExeFS offset, in media units.
	uint32_t exefs_size;			// [0x1A4] ExeFS size, in media units.
	uint32_t exefs_hash_region_size;	// [0x1A8] ExeFS hash region size, in media units.
	uint32_t reserved3;			// [0x1AC]
	uint32_t romfs_offset;			// [0x1B0] RomFS offset, in media units.
	uint32_t romfs_size;			// [0x1B4] RomFS size, in media units.
	uint32_t romfs_hash_region_size;	// [0x1B8] RomFS hash region size, in media units.
	uint32_t reserved4;			// [0x1BC]
	uint8_t exefs_uperblock_hash[0x20];	// [0x1C0] ExeFS superblock SHA-256 hash
	uint8_t romfs_uperblock_hash[0x20];	// [0x1E0] RomFS superblock SHA-256 hash
} N3DS_NCCH_Header_NoSig_t;
#pragma pack()
ASSERT_STRUCT(N3DS_NCCH_Header_NoSig_t, 256);

/**
 * NCCH flags. (byte array indexes)
 */
typedef enum {
	N3DS_NCCH_FLAG_CRYPTO_METHOD		= 3,	// If non-zero, an NCCH crypto method is used.
	N3DS_NCCH_FLAG_PLATFORM			= 4,	// See N3DS_NCCH_Platform.
	N3DS_NCCH_FLAG_CONTENT_TYPE		= 5,	// See N3DS_NCCH_Content_Type.
	N3DS_NCCH_FLAG_CONTENT_UNIT_SIZE	= 6,
	N3DS_NCCH_FLAG_BIT_MASKS		= 7,	// See N3DS_NCCH_Bit_Masks.
} N3DS_NCCH_Flags;

/**
 * NCCH platform type.
 */
typedef enum {
	N3DS_NCCH_PLATFORM_CTR		= 1,	// Old3DS
	N3DS_NCCH_PLATFORM_SNAKE	= 2,	// New3DS
} N3DS_NCCH_Platform;

/**
 * NCCH content type.
 */
typedef enum {
	N3DS_NCCH_CONTENT_TYPE_DATA	= 0x01,
	N3DS_NCCH_CONTENT_TYPE_EXE	= 0x02,
	N3DS_NCCH_CONTENT_TYPE_UPDATE	= 0x04,
	N3DS_NCCH_CONTENT_TYPE_MANUAL	= 0x08,
	N3DS_NCCH_CONTENT_TYPE_CHILD	= (0x04|0x08),
	N3DS_NCCH_CONTENT_TYPE_TRIAL	= 0x10,
} N3DS_NCCH_Content_Type;

/**
 * NCCH bit masks.
 */
typedef enum {
	N3DS_NCCH_BIT_MASK_FIXEDCRYPTOKEY	= 0x01,
	N3DS_NCCH_BIT_MASK_NOMOUNTROMFS		= 0x02,
	N3DS_NCCH_BIT_MASK_NOCRYPTO		= 0x04,
	N3DS_NCCH_BIT_MASK_FW96KEYY		= 0x20,
} N3DS_NCCH_Bit_Masks;

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_N3DS_STRUCTS_H__ */
