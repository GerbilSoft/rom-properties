/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * xbox360_stfs_structs.h: Microsoft Xbox 360 STFS data structures.        *
 *                                                                         *
 * Copyright (c) 2019-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "xbox360_common_structs.h"

#ifdef __cplusplus
extern "C" {
#endif

// STFS uses 4 KB blocks.
#define STFS_BLOCK_SIZE 4096

/**
 * Microsoft Xbox 360 content package header.
 * References:
 * - https://free60project.github.io/wiki/STFS.html
 * - https://github.com/Free60Project/wiki/blob/master/STFS.md
 * 
 * All fields are in big-endian.
 */
#define STFS_MAGIC_CON  'CON '	// Package signed by a console.
#define STFS_MAGIC_PIRS 'PIRS'	// Package signed by Microsoft from a non-Xbox Live source, e.g. System Update.
#define STFS_MAGIC_LIVE 'LIVE'	// Package signed by Microsoft from an Xbox Live source, e.g. a title update.
typedef struct _STFS_Package_Header {
	uint32_t magic;			// [0x000] 'CON ', 'PIRS', or 'LIVE'

	union {
		struct {
			// Console-signed package.
			uint16_t pubkey_cert_size;	// [0x004] Public key certificate size
			uint8_t console_id[5];		// [0x006] Certificate owner Console ID
			char part_number[20];		// [0x00B] Certificate owner Part Number
			uint8_t console_type;		// [0x01F] Certificate owner Console Type (see STFS_Console_Type_e)
			char datestamp[8];		// [0x020] Certificate date of generation
			uint32_t pub_exponent;		// [0x028] Public exponent
			uint8_t pub_modulus[0x80];	// [0x02C] Public modulus
			uint8_t cert_signature[0x100];	// [0x0AC] Certificate signature
			uint8_t signature[0x80];	// [0x1AC] Signature
		} console;
		struct {
			// Microsoft-signed package.
			uint8_t signature[0x100];	// [0x004] RSA-2048 signature
			uint8_t padding[0x128];		// [0x104] Padding
		} ms;
	};
} STFS_Package_Header;
ASSERT_STRUCT(STFS_Package_Header, 0x22C);

/**
 * Console type.
 */
typedef enum {
	STFS_CONSOLE_TYPE_DEBUG		= 1,
	STFS_CONSOLE_TYPE_RETAIL	= 2,
} STFS_Console_Type_e;

/**
 * STFS: License entry.
 * All fields are in big-endian.
 */
typedef struct _STFS_License_Entry {
	uint64_t license_id;	// [0x000] License ID (XUID / PUID / console ID)
	uint32_t license_bits;	// [0x008] License bits (TODO)
	uint32_t license_flags;	// [0x00C] License flags (TODO)
} STFS_License_Entry;
ASSERT_STRUCT(STFS_License_Entry, 16);

/**
 * STFS: Volume descriptor
 * All fields are in big-endian.
 */
#pragma pack(1)
typedef struct RP_PACKED _STFS_Volume_Descriptor {
	uint8_t size;				// [0x000] Size (0x24)
	uint8_t reserved;			// [0x001]
	uint8_t block_separation;		// [0x002]
	int16_t file_table_block_count;		// [0x003] File table block count. (BE16)
	uint8_t file_table_block_number[3];	// [0x005] File table block number. (BE24)
	uint8_t top_hash_table_hash[0x14];	// [0x008]
	uint32_t total_alloc_block_count;	// [0x01C]
	uint32_t total_unalloc_block_count;	// [0x020]
} STFS_Volume_Descriptor;
ASSERT_STRUCT(STFS_Volume_Descriptor, 0x24);
#pragma pack()

/**
 * SVOD: Volume descriptor
 * All fields are in big-endian.
 */
typedef struct _SVOD_Volume_Descriptor {
	uint8_t size;				// [0x000] Size (0x24)
	uint8_t block_cache_element_count;	// [0x001]
	uint8_t worker_thread_processor;	// [0x002]
	uint8_t worker_thread_priority;		// [0x003]
	uint8_t hash[0x14];			// [0x004]
	uint8_t device_features;		// [0x018]
	uint8_t data_block_count[3];		// [0x019] (BE24)
	uint8_t data_block_offset[3];		// [0x01C] (BE24)
	uint8_t reserved[5];			// [0x01F]
} SVOD_Volume_Descriptor;
ASSERT_STRUCT(SVOD_Volume_Descriptor, 0x24);

/**
 * Package metadata.
 * Stored immediately after the package header.
 *
 * NOTE: Offsets are relative to the beginning of the file.
 *
 * All fields are in big-endian.
 */
#define STFS_METADATA_ADDRESS 0x22C
#pragma pack(1)
typedef struct RP_PACKED _STFS_Package_Metadata {
	STFS_License_Entry license_entries[16];	// [0x22C] License entries
	uint8_t header_sha1[0x14];		// [0x32C] Header SHA1 (from 0x344 to first hash table)
	uint32_t header_size;			// [0x340] Size of this header
	uint32_t content_type;			// [0x344] Content type (See STFS_Content_Type_e)
	uint32_t metadata_version;		// [0x348] Metadata version
	uint64_t content_size;			// [0x34C] Content size
	uint32_t media_id;			// [0x354] Media ID (low 32 bits)
	Xbox360_Version_t version;		// [0x358] Version (system/title updates)
	Xbox360_Version_t base_version;		// [0x35C] Base version (system/title updates)
	Xbox360_Title_ID title_id;		// [0x360] Title ID
	uint8_t platform;			// [0x364] Platform (360=2, Win=4) [TODO: Enum]
	uint8_t executable_type;		// [0x365]
	uint8_t disc_number;			// [0x366]
	uint8_t disc_in_set;			// [0x367]
	uint32_t savegame_id;			// [0x368] Savegame ID
	uint8_t console_id[5];			// [0x36C] Console ID
	uint64_t profile_id;			// [0x371] Profile ID
	union {
		STFS_Volume_Descriptor stfs_desc;	// [0x379] STFS volume descriptor
		SVOD_Volume_Descriptor svod_desc;	// [0x379] SVOD volume descriptor
	};
	uint32_t data_file_count;		// [0x39D] Data file count
	uint64_t data_file_combined_size;	// [0x3A1] Data file combined size
	uint32_t descriptor_type;		// [0x3A9] Descriptor type (STFS=0, SVOD=1) [TODO: Enum]
	uint32_t reserved;			// [0x3AD]

	union {
		uint8_t mdv0_padding[0x4C];		// [0x3B1] Padding (Metadata v0)
		struct {
			// Metadata v2
			uint8_t series_id[0x10];	// [0x3B1] Series ID
			uint8_t season_id[0x10];	// [0x3C1] Season ID
			uint16_t season_number;		// [0x3D1] Season number
			uint16_t episode_number;	// [0x3D3] Episode number
			uint8_t padding[0x28];		// [0x3D5]
		} mdv2_video;
	};

	uint8_t device_id[0x14];			// [0x3FD] Device ID
	char16_t display_name[18][0x40];		// [0x411] Display name (up to 18 languages)
	char16_t display_description[18][0x40];		// [0xD11] Display description (up to 18 languages)
	char16_t publisher_name[0x40];			// [0x1611] Publisher name
	char16_t title_name[0x40];			// [0x1691] Title name
	uint8_t transfer_flags;				// [0x1711] Transfer flags (See STFS_Transfer_Flags_e)
} STFS_Package_Metadata;
ASSERT_STRUCT(STFS_Package_Metadata, 0x1712-0x22C);
#pragma pack()

/**
 * STFS: Thumbnail data.
 * Also contains additional display names for metadata v2.
 *
 * NOTE: Offsets are relative to the beginning of the file.
 *
 * All fields are in big-endian.
 */
#define STFS_THUMBNAILS_ADDRESS 0x1712
typedef struct _STFS_Package_Thumbnails {
	// Thumbnail sizes are 0x4000 for v0, 0x3D00 for v2.
	uint32_t thumbnail_image_size;			// [0x1712] Thumbnail image size
	uint32_t title_thumbnail_image_size;		// [0x1716] Title thumbnail image size

	// Thumbnail image format is PNG.
	union {
		struct {
			// Metadata v0
			uint8_t thumbnail_image[0x4000];		// [0x171A] Thumbnail image
			uint8_t title_thumbnail_image[0x4000];		// [0x571A] Title thumbnail image
		} mdv0;
		struct {
			// Metadata v2
			uint8_t thumbnail_image[0x3D00];		// [0x171A] Thumbnail image
			char16_t display_name_extra[6][0x40];		// [0x541A] Additional display names
			uint8_t title_thumbnail_image[0x3D00];		// [0x571A] Title thumbnail image
			char16_t display_description_extra[6][0x40];	// [0x941A] Additional display descriptions
		} mdv2;
	};
} STFS_Package_Thumbnails;
ASSERT_STRUCT(STFS_Package_Thumbnails, 0x971A-0x1712);

/**
 * STFS: Content type
 */
typedef enum {
	STFS_CONTENT_TYPE_SAVED_GAME		=       0x1,
	STFS_CONTENT_TYPE_MARKETPLACE_CONTENT	=       0x2,
	STFS_CONTENT_TYPE_PUBLISHER		=       0x3,
	STFS_CONTENT_TYPE_XBOX_360_TITLE	=    0x1000,
	STFS_CONTENT_TYPE_IPTV_PAUSE_BUFFER	=    0x2000,
	STFS_CONTENT_TYPE_INSTALLED_GAME	=    0x4000,
	STFS_CONTENT_TYPE_XBOX_ORIGINAL_GAME	=    0x5000,
	STFS_CONTENT_TYPE_XBOX_TITLE		=    0x5000,	// FIXME: Free60 lists the same value as Xbox Original Game.
	STFS_CONTENT_TYPE_AVATAR_ITEM		=    0x9000,
	STFS_CONTENT_TYPE_PROFILE		=   0x10000,
	STFS_CONTENT_TYPE_GAMER_PICTURE		=   0x20000,
	STFS_CONTENT_TYPE_THEME			=   0x30000,
	STFS_CONTENT_TYPE_CACHE_FILE		=   0x40000,
	STFS_CONTENT_TYPE_STORAGE_DOWNLOAD	=   0x50000,
	STFS_CONTENT_TYPE_XBOX_SAVED_GAME	=   0x60000,
	STFS_CONTENT_TYPE_XBOX_DOWNLOAD		=   0x70000,
	STFS_CONTENT_TYPE_GAME_DEMO		=   0x80000,
	STFS_CONTENT_TYPE_VIDEO			=   0x90000,
	STFS_CONTENT_TYPE_GAME_TITLE		=   0xA0000,
	STFS_CONTENT_TYPE_INSTALLER		=   0xB0000,
	STFS_CONTENT_TYPE_GAME_TRAILER		=   0xC0000,
	STFS_CONTENT_TYPE_ARCADE_TITLE		=   0xD0000,
	STFS_CONTENT_TYPE_XNA			=   0xE0000,
	STFS_CONTENT_TYPE_LICENSE_STORE		=   0xF0000,
	STFS_CONTENT_TYPE_MOVIE			=  0x100000,
	STFS_CONTENT_TYPE_TV			=  0x200000,
	STFS_CONTENT_TYPE_MUSIC_VIDEO           =  0x300000,
	STFS_CONTENT_TYPE_GAME_VIDEO		=  0x400000,
	STFS_CONTENT_TYPE_PODCAST_VIDEO		=  0x500000,
	STFS_CONTENT_TYPE_VIRAL_VIDEO		=  0x600000,
	STFS_CONTENT_TYPE_COMMUNITY_GAME	= 0x2000000,
} STFS_Content_Type_e;

/**
 * STFS: Transfer flags
 */
typedef enum {
	STFS_TRANSFER_FLAG_DEVICEID_AND_CONTENTID	= 0x00,
	STFS_TRANSFER_FLAG_MOVE_ONLY			= 0x20,
	STFS_TRANSFER_FLAG_DEVICEID			= 0x40,
	STFS_TRANSFER_FLAG_PROFILEID			= 0x80,
	STFS_TRANSFER_FLAG_NONE				= 0xC0,

	// Bitfield values.
	// FIXME: This doesn't seem right...
	STFS_TRANSFER_FLAG_BIT_DEEP_LINK_SUPPORTED	= (1U << 2),
	STFS_TRANSFER_FLAG_BIT_DISABLE_NETWORK_STORAGE	= (1U << 3),
	STFS_TRANSFER_FLAG_BIT_KINECT_ENABLED		= (1U << 4),
	STFS_TRANSFER_FLAG_BIT_MOVE_ONLY_TRANSFER	= (1U << 5),
	STFS_TRANSFER_FLAG_BIT_DEVICE_TRANSFER		= (1U << 6),
	STFS_TRANSFER_FLAG_BIT_PROFILE_TRANSFER		= (1U << 7),
} STFS_Transfer_Flags_e;

/**
 * STFS: Directory entry.
 */
typedef struct _STFS_DirEntry_t {
	char filename[0x28];		// [0x000] Filename, NULL-terminated.
	uint8_t flags_len;		// [0x028] Flags, plus filename length. (mask with 0x3F)
	uint8_t blocks[3];		// [0x029] Blocks. (LE24)
	uint8_t blocks2[3];		// [0x02B] Copy of blocks. (LE24)
	uint8_t block_number[3];	// [0x02F] Starting block number. (LE24)
	int16_t path;			// [0x032] Path indicator. (BE16)
	uint32_t filesize;		// [0x034] Filesize. (BE32)
	int32_t update_time;		// [0x038] Update time. (BE32; FAT format)
	int32_t access_time;		// [0x03C] ACcess time. (BE32; FAT format)
} STFS_DirEntry_t;
ASSERT_STRUCT(STFS_DirEntry_t, 0x40);

#ifdef __cplusplus
}
#endif
