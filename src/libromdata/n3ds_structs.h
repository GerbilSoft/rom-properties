/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * n3ds_structs.h: Nintendo 3DS data structures.                           *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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
// - https://3dbrew.org/wiki/SMDH
// - https://github.com/devkitPro/3dstools/blob/master/src/smdhtool.cpp
// - https://3dbrew.org/wiki/3DSX_Format
// - https://3dbrew.org/wiki/CIA
// - https://3dbrew.org/wiki/NCSD
// - https://3dbrew.org/wiki/ExeFS
// - https://3dbrew.org/wiki/TMD
// - https://3dbrew.org/wiki/NCCH/Extended_Header
// - https://3dbrew.org/wiki/Ticket

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
 * Reference: https://3dbrew.org/wiki/SMDH
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
 * Title ID struct/union. (little-endian version)
 * TODO: Verify operation on big-endian systems.
 */
#pragma pack(1)
typedef union PACKED _N3DS_TitleID_LE_t {
	uint64_t id;
	struct {
		uint32_t lo;
		uint32_t hi;
	};
} N3DS_TitleID_LE_t;
#pragma pack()
ASSERT_STRUCT(N3DS_TitleID_LE_t, 8);

/**
 * Title ID struct/union. (big-endian version)
 * TODO: Verify operation on big-endian systems.
 */
#pragma pack(1)
typedef union PACKED _N3DS_TitleID_BE_t {
	uint64_t id;
	struct {
		uint32_t hi;
		uint32_t lo;
	};
} N3DS_TitleID_BE_t;
#pragma pack()
ASSERT_STRUCT(N3DS_TitleID_BE_t, 8);

/**
 * Nintendo 3DS cartridge and eMMC header. (NCSD)
 * This version does not have the 256-byte RSA-2048 signature.
 * Reference: https://3dbrew.org/wiki/NCSD
 *
 * All fields are little-endian.
 */
#define N3DS_NCSD_HEADER_MAGIC "NCSD"
#define N3DS_NCSD_NOSIG_HEADER_ADDRESS 0x100
#pragma pack(1)
typedef struct PACKED _N3DS_NCSD_Header_NoSig_t {
	// [0x100]
	char magic[4];			// [0x100] "NCSD"
	uint32_t image_size;		// [0x104] Image size, in media units. (1 media unit = 512 bytes)
	N3DS_TitleID_LE_t media_id;	// [0x108] Media ID.

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
} N3DS_NCSD_Header_NoSig_t;
#pragma pack()
ASSERT_STRUCT(N3DS_NCSD_Header_NoSig_t, 256);

/**
 * Nintendo 3DS cartridge and eMMC header. (NCSD)
 * This version has the 256-byte RSA-2048 signature.
 * Reference: https://3dbrew.org/wiki/NCSD
 *
 * All fields are little-endian.
 */
#pragma pack(1)
typedef struct PACKED _N3DS_NCSD_Header_t {
	uint8_t signature[0x100];		// [0x000] RSA-2048 SHA-256 signature
	N3DS_NCSD_Header_NoSig_t hdr;		// [0x100] NCSD header
} N3DS_NCSD_Header_t;
#pragma pack()
ASSERT_STRUCT(N3DS_NCSD_Header_t, 512);

/**
 * NCSD partition index.
 */
typedef enum {
	N3DS_NCSD_PARTITION_GAME	= 0,
	N3DS_NCSD_PARTITION_MANUAL	= 1,
	N3DS_NCSD_PARTITION_DLP		= 2,
	N3DS_NCSD_PARTITION_N3DS_UPDATE	= 6,
	N3DS_NCSD_PARTITION_O3DS_UPDATE	= 7,
} N3DS_NCSD_Partition_Index;

/**
 * NCSD partition flags. (byte array indexes)
 */
typedef enum {
	N3DS_NCSD_PARTITION_FLAG_BACKUP_WRITE_WAIT_TIME	= 0,
	N3DS_NCSD_PARTITION_FLAG_MEDIA_CARD_DEVICE_SDK3	= 3,
	N3DS_NCSD_PARTITION_FLAG_MEDIA_PLATFORM_INDEX	= 4,
	N3DS_NCSD_PARTITION_FLAG_MEDIA_TYPE_INDEX	= 5,
	N3DS_NCSD_PARTITION_FLAG_MEDIA_UNIT_SIZE	= 6,
	N3DS_NCSD_PARTITION_FLAG_MEDIA_CARD_DEVICE_SDK2	= 7,
} N3DS_NCSD_Partition_Flags;

/**
 * NCSD partition flags: Card Device.
 */
typedef enum {
	N3DS_NCSD_CARD_DEVICE_NOR_FLASH		= 1,
	N3DS_NCSD_CARD_DEVICE_NONE		= 2,
	N3DS_NCSD_CARD_DEVICE_BLUETOOTH		= 3,

	N3DS_NCSD_CARD_DEVICE_MIN		= N3DS_NCSD_CARD_DEVICE_NOR_FLASH,
	N3DS_NCSD_CARD_DEVICE_MAX		= N3DS_NCSD_CARD_DEVICE_BLUETOOTH,
} N3DS_NCSD_Card_Device;

/**
 * NCSD partition flags: Media Type.
 */
typedef enum {
	N3DS_NCSD_MEDIA_TYPE_INNER_DEVICE	= 0,
	N3DS_NCSD_MEDIA_TYPE_CARD1		= 1,
	N3DS_NCSD_MEDIA_TYPE_CARD2		= 2,
	N3DS_NCSD_MEDIA_TYPE_EXTENDED_DEVICE	= 3,
} N3DS_NCSD_Media_Type;

/**
 * NCSD: Card Info Header.
 * Reference: https://3dbrew.org/wiki/NCSD
 *
 * All fields are little-endian.
 */
#pragma pack(1)
#define N3DS_NCSD_CARD_INFO_HEADER_ADDRESS 0x200
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
	union {
		uint64_t partition_id;		// [0x108] Partition ID.
		struct {
			uint8_t reserved[6];	// [0x108]
			uint16_t sysversion;	// [0x10E] System Update version for update partitions.
		};
	};
	char maker_code[2];			// [0x110] Maker code.
	uint16_t version;			// [0x112] Version.
	uint32_t fw96lock;			// [0x114] Used by FIRM 9.6.0-X to verify the content lock seed.
	N3DS_TitleID_LE_t program_id;		// [0x118] Program ID.
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
 * Nintendo 3DS NCCH header.
 * This version has the 256-byte RSA-2048 signature.
 * Reference: https://3dbrew.org/wiki/NCSD
 *
 * All fields are little-endian.
 */
#pragma pack(1)
typedef struct PACKED _N3DS_NCCH_Header_t {
	uint8_t signature[0x100];		// [0x000] RSA-2048 SHA-256 signature
	N3DS_NCCH_Header_NoSig_t hdr;		// [0x100] NCCH header
} N3DS_NCCH_Header_t;
#pragma pack()
ASSERT_STRUCT(N3DS_NCCH_Header_t, 512);

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
	N3DS_NCCH_CONTENT_TYPE_Data		= 0x01,
	N3DS_NCCH_CONTENT_TYPE_Executable	= 0x02,
	N3DS_NCCH_CONTENT_TYPE_SystemUpdate	= 0x04,
	N3DS_NCCH_CONTENT_TYPE_Manual		= 0x08,
	N3DS_NCCH_CONTENT_TYPE_Child		= (0x04|0x08),
	N3DS_NCCH_CONTENT_TYPE_Trial		= 0x10,
} N3DS_NCCH_Content_Type;

/**
 * NCCH bit masks.
 */
typedef enum {
	N3DS_NCCH_BIT_MASK_FixedCryptoKey	= 0x01,
	N3DS_NCCH_BIT_MASK_NoMountRomFS		= 0x02,
	N3DS_NCCH_BIT_MASK_NoCrypto		= 0x04,
	N3DS_NCCH_BIT_MASK_Fw96KeyY		= 0x20,	// SEED encryption
} N3DS_NCCH_Bit_Masks;

/**
 * NCCH section numbers.
 * Used as part of the counter initialization.
 * Reference: https://github.com/profi200/Project_CTR/blob/master/makerom/ncch.h
 */
typedef enum {
	N3DS_NCCH_SECTION_EXHEADER	= 1,
	N3DS_NCCH_SECTION_EXEFS		= 2,
	N3DS_NCCH_SECTION_ROMFS		= 3,
} N3DS_NCCH_Sections;

/**
 * 3DS keyset.
 * Reference: https://github.com/profi200/Project_CTR/blob/master/makerom/keyset.h
 */
typedef enum {
	N3DS_PKI_TEST,
	//N3DS_PKI_BETA,
	N3DS_PKI_DEVELOPMENT,
	N3DS_PKI_PRODUCTION,
	//N3DS_PKI_CUSTOM,
} N3DS_KeySet;

/**
 * Nintendo 3DS: ExeFS file header.
 * Reference: https://3dbrew.org/wiki/ExeFS
 *
 * All fields are little-endian.
 */
#pragma pack(1)
typedef struct PACKED _N3DS_ExeFS_File_Header_t {
	char name[8];
	uint32_t offset;
	uint32_t size;
} N3DS_ExeFS_File_Header_t;
#pragma pack()
ASSERT_STRUCT(N3DS_ExeFS_File_Header_t, 16);

/**
 * Nintendo 3DS: ExeFS header.
 * Reference: https://3dbrew.org/wiki/ExeFS
 *
 * All fields are little-endian.
 */
#pragma pack(1)
typedef struct PACKED _N3DS_ExeFS_Header_t {
	N3DS_ExeFS_File_Header_t files[10];
	uint8_t reserved[0x20];
	uint8_t hashes[10][32];	// SHA-256 hashes of each file.
} N3DS_ExeFS_Header_t;
#pragma pack()
ASSERT_STRUCT(N3DS_ExeFS_Header_t, 512);

/**
 * Nintendo 3DS: Ticket and Title Metadata signature type.
 * TMD header location depends on the signature type.
 * Reference: https://3dbrew.org/wiki/TMD#Signature_Data
 */
typedef enum {
	// NOTE: The first three are not generally used on 3DS.
	N3DS_SIGTYPE_RSA_4096_SHA1		= 0x00010000,	// len = 0x200, pad = 0x3C
	N3DS_SIGTYPE_RSA_2048_SHA1		= 0x00010001,	// len = 0x100, pad = 0x3C
	N3DS_SIGTYPE_EC_SHA1			= 0x00010002,	// len =  0x3C, pad = 0x40

	// These are used on 3DS.
	N3DS_SIGTYPE_RSA_4096_SHA256		= 0x00010003,	// len = 0x200, pad = 0x3C
	N3DS_SIGTYPE_RSA_2048_SHA256		= 0x00010004,	// len = 0x100, pad = 0x3C
	N3DS_SIGTYPE_ECDSA_SHA256		= 0x00010005,	// len =  0x3C, pad = 0x40
} N3DS_Signature_Type;

/**
 * Nintendo 3DS: Title Metadata header.
 * Reference: https://3dbrew.org/wiki/TMD#Header
 *
 * The signature is NOT included, since it's variable-length.
 *
 * All fields are BIG-endian due to its
 * roots in the Wii TMD format.
 */
#pragma pack(1)
typedef struct PACKED _N3DS_TMD_Header_t {
	char signature_issuer[0x40];	// [0x00] Signature issuer.
	uint8_t tmd_version;		// [0x40]
	uint8_t ca_crl_version;		// [0x41]
	uint8_t signer_crl_version;	// [0x42]
	uint8_t reserved1;		// [0x43]
	uint64_t system_version;	// [0x44] Required system version.
	N3DS_TitleID_BE_t title_id;	// [0x4C] Title ID.
	uint32_t title_type;		// [0x54] Title type.
	uint16_t group_id;		// [0x58] Group ID.
	uint32_t save_data_size;	// [0x4A] Save data size. (SRL: Public save data size)
	uint32_t srl_private_save_data_size;	// [0x5E] SRL: Private save data size.
	uint32_t reserved2;		// [0x62]
	uint8_t srl_flag;		// [0x66] SRL flag.
	uint8_t reserved3[0x31];	// [0x67]
	uint32_t access_rights;		// [0x98] Access rights.
	uint16_t title_version;		// [0x9C] Title version.
	uint16_t content_count;		// [0x9E] Content count.
	uint16_t boot_content;		// [0xA0] Boot content.
	uint8_t padding[2];		// [0xA2]
	uint8_t content_info_sha256[0x20];	// [0xA4] SHA-256 hash of content info records.
} N3DS_TMD_Header_t;
#pragma pack()
ASSERT_STRUCT(N3DS_TMD_Header_t, 0xC4);

/**
 * Nintendo 3DS: Content Info Record.
 * Reference: https://3dbrew.org/wiki/TMD#Content_Info_Records
 *
 * All fields are BIG-endian due to its
 * roots in the Wii TMD format.
 */
#pragma pack(1)
typedef struct PACKED _N3DS_Content_Info_Record_t {
	uint16_t content_index_offset;
	uint16_t content_command_count;	// [k]
	uint8_t sha256_next[0x20];	// SHA-256 hash of the next [k] content records.
} N3DS_Content_Info_Record_t;
#pragma pack()
ASSERT_STRUCT(N3DS_Content_Info_Record_t, 0x24);

/**
 * Nintendo 3DS: Content Chunk Record.
 * Reference: https://3dbrew.org/wiki/TMD#Content_chunk_records
 *
 * All fields are BIG-endian due to its
 * roots in the Wii TMD format.
 */
#pragma pack(1)
typedef struct PACKED _N3DS_Content_Chunk_Record_t {
	uint32_t id;		// [0x00]
	uint16_t index;		// [0x04] Content index. (Used for CIA title key encryption.)
	uint16_t type;		// [0x06] See N3DS_Content_Chunk_Type_Flags.
	uint64_t size;		// [0x08]
	uint8_t sha256[0x20];	// [0x10]
} N3DS_Content_Chunk_Record_t;
#pragma pack()
ASSERT_STRUCT(N3DS_Content_Chunk_Record_t, 0x30);

/**
 * Nintendo 3DS: Content Chunk type flags.
 * Reference: https://3dbrew.org/wiki/TMD#Content_Type_flags
 */
typedef enum {
	N3DS_CONTENT_CHUNK_ENCRYPTED	= 1,
	N3DS_CONTENT_CHUNK_DISC		= 2,
	N3DS_CONTENT_CHUNK_CFM		= 4,
	N3DS_CONTENT_CHUNK_OPTIONAL	= 0x4000,
	N3DS_CONTENT_CHUNK_SHARED	= 0x8000,
} N3DS_Content_Chunk_Type_Flags;

/**
 * Nintendo 3DS: Title Metadata.
 * Reference: https://3dbrew.org/wiki/TMD
 *
 * The signature is NOT included, since it's variable-length.
 *
 * All fields are BIG-endian due to its
 * roots in the Wii TMD format.
 */
#pragma pack(1)
typedef struct PACKED _N3DS_TMD_t {
	N3DS_TMD_Header_t header;			// [0x00] TMD header.
	N3DS_Content_Info_Record_t cinfo_records[64];	// [0xA4] Content info records.
} N3DS_TMD_t;
#pragma pack()
ASSERT_STRUCT(N3DS_TMD_t, 0xC4+(0x24*64));

/**
 * Nintendo 3DS: NCCH Extended Header: Code Set Info.
 * Reference: https://3dbrew.org/wiki/NCCH/Extended_Header#Code_Set_Info
 *
 * All fields are little-endian.
 */
#pragma pack(1)
typedef struct PACKED _N3DS_NCCH_ExHeader_CodeSet_t {
	uint32_t address;
	uint32_t physical_region_size;	// in page multiples
	uint32_t size;			// in bytes
} N3DS_NCCH_ExHeader_CodeSet_t;
#pragma pack()
ASSERT_STRUCT(N3DS_NCCH_ExHeader_CodeSet_t, 12);

/**
 * Nintendo 3DS: NCCH Extended Header: System Control Info.
 * Reference: https://3dbrew.org/wiki/NCCH/Extended_Header#System_Control_Info
 *
 * All fields are little-endian.
 */
#pragma pack(1)
typedef struct PACKED _N3DS_NCCH_ExHeader_SCI_t {
	char title[8];		// Application title. (default is "CtrApp")
	uint8_t reserved1[5];
	uint8_t flags;		// See N3DS_NCCH_ExHeader_SCI_Flags.
	uint16_t remaster_version;
	N3DS_NCCH_ExHeader_CodeSet_t text_code_set_info;
	uint32_t stack_size;
	N3DS_NCCH_ExHeader_CodeSet_t ro_code_set_info;
	uint8_t reserved2[4];
	N3DS_NCCH_ExHeader_CodeSet_t data_code_set_info;
	uint32_t bss_size;
	char dep_list[48][8];	// Dependency module list.

	// Reference: https://3dbrew.org/wiki/NCCH/Extended_Header#System_Info
	struct {
		uint64_t savedata_size;
		uint64_t jump_id;
		uint8_t reserved[0x30];
	} system_info;
} N3DS_NCCH_ExHeader_SCI_t;
#pragma pack()
ASSERT_STRUCT(N3DS_NCCH_ExHeader_SCI_t, 0x200);

/**
 * NCCH Extended Header: SCI flags.
 */
typedef enum {
	N3DS_NCCH_EXHEADER_SCI_CompressExefsCode	= (1 << 0),
	N3DS_NCCH_EXHEADER_SCI_SDApplication		= (1 << 1),
} N3DS_NCCH_ExHeader_SCI_Flags;

/**
 * Nintendo 3DS: NCCH Extended Header: Access Control Info.
 * Reference: https://3dbrew.org/wiki/NCCH/Extended_Header#Access_Control_Info
 *
 * All fields are little-endian.
 */
#pragma pack(1)
typedef struct PACKED _N3DS_NCCH_ExHeader_ACI_t {
	// [0x000]
	// Reference: https://3dbrew.org/wiki/NCCH/Extended_Header#ARM11_Local_System_Capabilities
	struct {
		N3DS_TitleID_LE_t program_id;
		uint32_t core_version;		// Title ID low of required FIRM.

		// Flags:
		// - [0] == New3DS CPU speed and L2 cache. (See N3DS_NCCH_ExHeader_ACI_Flag_New3DS_CPUMode)
		// - [1] == New3DS system mode. (See N3DS_NCCH_ExHeader_ACI_Flag_New3DS_SysMode)
		// - [2] == System mode. (See N3DS_NCCH_ExHeader_ACI_Flag_SysMode)
		uint8_t flags[3];

		uint8_t priority;
		uint16_t res_limit_descriptors[16];

		// Reference: https://3dbrew.org/wiki/NCCH/Extended_Header#Storage_Info
		struct {
			uint64_t extdata_id;
			uint8_t sys_savedata_ids[8];
			uint8_t storage_accessible_unique_ids[8];
			// NOTE: The high byte here is technically separate,
			// but I'm using a single uint64_t for convenience.
			union {
				uint64_t fs_access;	// See N3DS_NCCH_ExHeader_ACI_FsAccess.
				struct {
					uint8_t reserved[7];
					uint8_t other_attr;	// See N3DS_NCCH_ExHeader_ACI_OtherAttr.
				};
			};
		} storage;

		char services[32][8];		// [0x050] Services.
		char ext_services[2][8];	// [0x150] Extended services.
		uint8_t reserved[15];		// [0x160]
		uint8_t res_limit_category;	// [0x16F] See N3DS_NCCH_ExHeader_ACI_ResLimit_Category.
	} arm11_local;

	// [0x170]
	// Reference: https://3dbrew.org/wiki/NCCH/Extended_Header#ARM11_Kernel_Capabilities
	struct {
		uint32_t descriptors[28];
		uint8_t reserved[16];
	} arm11_kernel;

	// [0x1F0]
	// Reference: https://3dbrew.org/wiki/NCCH/Extended_Header#ARM9_Access_Control
	struct {
		// FIXME: Determine the format of the descriptors.
		// See N3DS_NCCH_ExHeader_IoAccessControl.
		uint8_t descriptors[15];
		uint8_t descriptor_version;
	} arm9;
} N3DS_NCCH_ExHeader_ACI_t;
#pragma pack()
ASSERT_STRUCT(N3DS_NCCH_ExHeader_ACI_t, 0x200);

/**
 * NCCH Extended Header: ACI New3DS CPU mode. (flags[0])
 */
typedef enum {
	N3DS_NCCH_EXHEADER_ACI_FLAG0_EnableL2Cache	= (1 << 0),	// Ignored.
	N3DS_NCCH_EXHEADER_ACI_FLAG0_cpuspeed_804MHz	= (1 << 1),
} N3DS_NCCH_ExHeader_ACI_Flag_New3DS_CPUMode;

/**
 * NCCH Extended Header: ACI New3DS System Mode. (flags[1])
 */
typedef enum {
	// New3DS system modes.
	N3DS_NCCH_EXHEADER_ACI_FLAG1_New3DS_SysMode_Legacy	= (0 << 0),	// 64 MB
	N3DS_NCCH_EXHEADER_ACI_FLAG1_New3DS_SysMode_Prod	= (1 << 0),	// 124 MB
	N3DS_NCCH_EXHEADER_ACI_FLAG1_New3DS_SysMode_Dev1	= (2 << 0),	// 178 MB
	N3DS_NCCH_EXHEADER_ACI_FLAG1_New3DS_SysMode_Dev2	= (3 << 0),	// 124 MB
	N3DS_NCCH_EXHEADER_ACI_FLAG1_New3DS_SysMode_Mask	= (15 << 0),
} N3DS_NCCH_ExHeader_ACI_Flag_New3DS_SysMode;

/**
 * NCCH Extended Header: ACI System Mode. (flags[2])
 */
typedef enum {
	N3DS_NCCH_EXHEADER_ACI_FLAG2_IdealCPU_Mask		= (2 << 0),
	N3DS_NCCH_EXHEADER_ACI_FLAG2_Affinity_Mask		= (2 << 2),

	// Old3DS system modes.
	N3DS_NCCH_EXHEADER_ACI_FLAG2_Old3DS_SysMode_Prod	= (0 << 4),	// 64 MB
	N3DS_NCCH_EXHEADER_ACI_FLAG2_Old3DS_SysMode_Dev1	= (2 << 4),	// 96 MB
	N3DS_NCCH_EXHEADER_ACI_FLAG2_Old3DS_SysMode_Dev2	= (3 << 4),	// 80 MB
	N3DS_NCCH_EXHEADER_ACI_FLAG2_Old3DS_SysMode_Dev3	= (4 << 4),	// 72 MB
	N3DS_NCCH_EXHEADER_ACI_FLAG2_Old3DS_SysMode_Dev4	= (5 << 4),	// 32 MB
	N3DS_NCCH_EXHEADER_ACI_FLAG2_Old3DS_SysMode_Mask	= (15 << 4),
} N3DS_NCCH_ExHeader_ACI_Flag_SysMode;

/**
 * NCCH Extended Header: ACI resource limit category.
 */
typedef enum {
	N3DS_NCCH_EXHEADER_ACI_ResLimit_Categry_APPLICATION	= 0,
	N3DS_NCCH_EXHEADER_ACI_ResLimit_Categry_SYS_APPLET	= 1,
	N3DS_NCCH_EXHEADER_ACI_ResLimit_Categry_LIB_APPLET	= 2,
	N3DS_NCCH_EXHEADER_ACI_ResLimit_Categry_OTHER		= 3,	// sysmodules running in BASE
} N3DS_NCCH_ExHeader_ACI_ResLimit_Category;

/**
 * NCCH Extended Header: ACI filesystem access info.
 */
typedef enum {
	N3DS_NCCH_EXHEADER_ACI_FsAccess_CategorySystemApplication	= (1 <<  0),
	N3DS_NCCH_EXHEADER_ACI_FsAccess_CategoryHardwareCheck		= (1 <<  1),
	N3DS_NCCH_EXHEADER_ACI_FsAccess_CategoryFilesystemTool		= (1 <<  2),
	N3DS_NCCH_EXHEADER_ACI_FsAccess_Debug				= (1 <<  3),
	N3DS_NCCH_EXHEADER_ACI_FsAccess_TwlCardBackup			= (1 <<  4),
	N3DS_NCCH_EXHEADER_ACI_FsAccess_TwlNandData			= (1 <<  5),
	N3DS_NCCH_EXHEADER_ACI_FsAccess_Boss				= (1 <<  6),
	N3DS_NCCH_EXHEADER_ACI_FsAccess_DirectSdmc			= (1 <<  7),
	N3DS_NCCH_EXHEADER_ACI_FsAccess_Core				= (1 <<  8),
	N3DS_NCCH_EXHEADER_ACI_FsAccess_CtrNandRo			= (1 <<  9),
	N3DS_NCCH_EXHEADER_ACI_FsAccess_CtrNandRw			= (1 << 10),
	N3DS_NCCH_EXHEADER_ACI_FsAccess_CtrNandRoWrite			= (1 << 11),
	N3DS_NCCH_EXHEADER_ACI_FsAccess_CategorySystemSettings		= (1 << 12),
	N3DS_NCCH_EXHEADER_ACI_FsAccess_Cardboard			= (1 << 13),
	N3DS_NCCH_EXHEADER_ACI_FsAccess_ExportImportIvs			= (1 << 14),
	N3DS_NCCH_EXHEADER_ACI_FsAccess_DirectSdmcWrite			= (1 << 15),
	N3DS_NCCH_EXHEADER_ACI_FsAccess_SwitchCleanup			= (1 << 16),
	N3DS_NCCH_EXHEADER_ACI_FsAccess_SaveDataMove			= (1 << 17),
	N3DS_NCCH_EXHEADER_ACI_FsAccess_Shop				= (1 << 18),
	N3DS_NCCH_EXHEADER_ACI_FsAccess_Shell				= (1 << 19),
	N3DS_NCCH_EXHEADER_ACI_FsAccess_CategoryHomeMenu		= (1 << 20),
	N3DS_NCCH_EXHEADER_ACI_FsAccess_SeedDB				= (1 << 21),
} N3DS_NCCH_ExHeader_ACI_FsAccess;

/**
 * NCCH Extended Header: ACI other attributes.
 */
typedef enum {
	N3DS_NCCH_EXHEADER_ACI_OtherAttr_NoRomFS			= (1 << 0),
	N3DS_NCCH_EXHEADER_ACI_OtherAttr_ExtendedSavedataAccess		= (1 << 1),
} N3DS_NCCH_ExHeader_ACI_OtherAttr;

/**
 * NCCH Extended Header: I/O access control. (ARM9)
 */
typedef enum {
	N3DS_NCCH_EXHEADER_ACI_IoAccess_FsMountNand		= (1 << 0),
	N3DS_NCCH_EXHEADER_ACI_IoAccess_FsMountNandRoWrite	= (1 << 1),
	N3DS_NCCH_EXHEADER_ACI_IoAccess_FsMountTwln		= (1 << 2),
	N3DS_NCCH_EXHEADER_ACI_IoAccess_FsMountWnand		= (1 << 3),
	N3DS_NCCH_EXHEADER_ACI_IoAccess_FsMountCardSpi		= (1 << 4),
	N3DS_NCCH_EXHEADER_ACI_IoAccess_UseSdif3		= (1 << 5),
	N3DS_NCCH_EXHEADER_ACI_IoAccess_CreateSeed		= (1 << 6),
	N3DS_NCCH_EXHEADER_ACI_IoAccess_UseCardSpi		= (1 << 7),
	N3DS_NCCH_EXHEADER_ACI_IoAccess_SDApplication		= (1 << 8),
	N3DS_NCCH_EXHEADER_ACI_IoAccess_FsMountSdmcWrite	= (1 << 9),
} N3DS_NCCH_ExHeader_IoAccessControl;

/**
 * Nintendo 3DS: NCCH Extended Header.
 * Reference: https://3dbrew.org/wiki/NCCH/Extended_Header
 *
 * All fields are little-endian.
 */
// NOTE: FBI.3ds only has SCI and ACI.
// It doesn't have any signatures or the second ACI.
#define N3DS_NCCH_EXHEADER_MIN_SIZE (sizeof(N3DS_NCCH_ExHeader_SCI_t) + sizeof(N3DS_NCCH_ExHeader_ACI_t))
#pragma pack(1)
typedef struct PACKED _N3DS_NCCH_ExHeader_t {
	N3DS_NCCH_ExHeader_SCI_t sci;
	N3DS_NCCH_ExHeader_ACI_t aci;
	uint8_t signature_accessdesc[0x100];	// RSA-2048 SHA256
	uint8_t ncch_pubkey[0x100];		// RSA-2048
	N3DS_NCCH_ExHeader_ACI_t aci2;		// TODO: How is this different from aci?
} N3DS_NCCH_ExHeader_t;
#pragma pack()
ASSERT_STRUCT(N3DS_NCCH_ExHeader_t, 0x800);

// Nintendo 3DS: Ticket issuers.
#define N3DS_TICKET_ISSUER_RETAIL "Root-CA00000003-XS0000000c"
#define N3DS_TICKET_ISSUER_DEBUG  "Root-CA00000004-XS00000009"

/**
 * Nintendo 3DS: Ticket.
 * Reference: https://www.3dbrew.org/wiki/Ticket
 *
 * The signature is NOT included, since it's variable-length.
 *
 * All fields are BIG-endian due to its
 * roots in the Wii ticket format.
 */
#pragma pack(1)
typedef struct PACKED _N3DS_Ticket_t {
	char issuer[0x40];		// [0x000] Ticket issuer. Can be used to distinguish debug vs. retail.
	uint8_t ecc_public_key[0x3C];	// [0x040] ECC public key.
	uint8_t version;		// [0x07C] Version. (Wii == 0; 3DS == 1)
	uint8_t ca_crl_version;		// [0x07D]
	uint8_t signer_crl_version;	// [0x07E]
	uint8_t title_key[0x10];	// [0x07F] Title key. (encrypted)
	uint8_t reserved1;		// [0x08F]
	uint64_t ticket_id;		// [0x090]
	uint32_t console_id;		// [0x098]
	N3DS_TitleID_BE_t title_id;	// [0x09C] Title ID.
	uint8_t reserved2[2];		// [0x0A4]
	uint16_t title_version;		// [0x0A6] ticket title version.
	uint8_t reserved3[8];		// [0x0A8]
	uint8_t license_type;		// [0x0B0]
	uint8_t keyY_index;		// [0x0B1] Common KeyY index. (keyslot 0x3D)
	uint8_t reserved4[0x2A];	// [0x0B2]
	uint32_t eshop_account_id;	// [0x0DC] eShop Account ID?
	uint8_t reserved5;		// [0x0E0]
	uint8_t audit;			// [0x0E1]
	uint8_t reserved6[0x42];	// [0x0E2]
	uint32_t limits[0x10];		// [0x124] Demo use limits.
	uint8_t content_index[0xAC];	// [0x164] Content index.
} N3DS_Ticket_t;
#pragma pack()
ASSERT_STRUCT(N3DS_Ticket_t, 528);

/**
 * Ticket common key index for title key encryption.
 */
typedef enum {
	// Bits 0-1: Issuer.
	// - 0: Unknown, or no encryption.
	// - 1: Retail
	// - 2: Debug
	N3DS_TICKET_TITLEKEY_ISSUER_UNKNOWN	= (0 << 0),
	N3DS_TICKET_TITLEKEY_ISSUER_RETAIL	= (1 << 0),
	N3DS_TICKET_TITLEKEY_ISSUER_DEBUG	= (2 << 0),
	N3DS_TICKET_TITLEKEY_ISSUER_MASK	= (3 << 0),

	// Bits 2-4: KeyY index.
	// - 0: eShop titles
	// - 1: System titles
	// - 2-5: Unknown
	N3DS_TICKET_TITLEKEY_KEYY_INDEX_MASK	= (7 << 2),
} N3DS_Ticket_TitleKey_KeyY;

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_N3DS_STRUCTS_H__ */
