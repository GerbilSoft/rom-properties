/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * xbox360_xex_structs.h: Microsoft Xbox 360 executable data structures.   *
 *                                                                         *
 * Copyright (c) 2019-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "xbox360_common_structs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Microsoft Xbox 360 executable header.
 * References:
 * - https://free60project.github.io/wiki/XEX.html
 * - http://www.noxa.org/blog/2011/08/13/building-an-xbox-360-emulator-part-5-xex-files/
 * - http://xorloser.com/blog/?p=395
 * - https://github.com/xenia-project/xenia/blob/HEAD/src/xenia/kernel/util/xex2_info.h
 * 
 * All fields are in big-endian.
 */
#define XEX1_MAGIC 'XEX1'
#define XEX2_MAGIC 'XEX2'
typedef struct _XEX2_Header {
	uint32_t magic;			// [0x000] 'XEX2' or 'XEX1'
	uint32_t module_flags;		// [0x004] See XEX2_Flags_e
	uint32_t pe_offset;		// [0x008] PE data offset
	uint32_t reserved;		// [0x00C]
	uint32_t sec_info_offset;	// [0x010] Security info offset (See XEX2_Security_Info)
	uint32_t opt_header_count;	// [0x014] Optional header count
} XEX2_Header;
ASSERT_STRUCT(XEX2_Header, 6*sizeof(uint32_t));

/**
 * XEX2: Module flags
 */
typedef enum {
	XEX2_MODULE_FLAG_TITLE			= (1U << 0),
	XEX2_MODULE_FLAG_EXPORTS_TO_TITLE	= (1U << 1),
	XEX2_MODULE_FLAG_SYSTEM_DEBUGGER	= (1U << 2),
	XEX2_MODULE_FLAG_DLL_MODULE		= (1U << 3),
	XEX2_MODULE_FLAG_MODULE_PATCH		= (1U << 4),
	XEX2_MODULE_FLAG_PATCH_FULL		= (1U << 5),
	XEX2_MODULE_FLAG_PATCH_DELTA		= (1U << 6),
	XEX2_MODULE_FLAG_USER_MODE		= (1U << 7),
} XEX2_Module_Flags_e;

/**
 * XEX1: Security info.
 * NOTE: XEX1 is only used on early preproduction XDKs.
 *
 * All fields are in big-endian.
 */
typedef struct _XEX1_Security_Info {
	uint32_t header_size;		// [0x000] Header size [should be at least sizeof(XEX1_Security_Info)]
	uint32_t image_size;		// [0x004] Image size (slightly larger than the .xex file)
	uint8_t rsa_signature[0x100];	// [0x008] RSA-2048 signature
	uint8_t image_sha1[0x14];	// [0x108] SHA-1 of the entire image?
	uint8_t import_table_sha1[0x14]; // [0x11C] Import table SHA-1
	uint32_t load_address;		// [0x130] Load address
	uint8_t title_key[0x10];	// [0x134] AES-128 title key (encrypted)
	uint8_t xgd2_media_id[0x10];	// [0x144] XGD2 media ID (TODO: Verify?)
	uint32_t region_code;		// [0x154] Region code (See XEX2_Region_Code_e)
	uint32_t image_flags;		// [0x158] Image flags (See XEX2_Image_Flags_e)
	uint32_t export_table;		// [0x15C] Export table offset (0 if none)
	uint32_t allowed_media_types;	// [0x160] Allowed media types (See XEX2_Media_Types_e)
	uint32_t page_descriptor_count;	// [0x164] Page descriptor count (these follow XEX2_Security_Info)
} XEX1_Security_Info;
ASSERT_STRUCT(XEX1_Security_Info, 0x168);

/**
 * XEX2: Security info
 * All fields are in big-endian.
 */
typedef struct _XEX2_Security_Info {
	uint32_t header_size;		// [0x000] Header size [should be at least sizeof(XEX2_Security_Info)]
	uint32_t image_size;		// [0x004] Image size (slightly larger than the .xex file)
	uint8_t rsa_signature[0x100];	// [0x008] RSA-2048 signature
	uint32_t unk_0x108;		// [0x108]
	uint32_t image_flags;		// [0x10C] Image flags (See XEX2_Image_Flags_e)
	uint32_t load_address;		// [0x110] Load address
	uint8_t section_sha1[0x14];	// [0x114] SHA-1 of something
	uint32_t import_table_count;	// [0x128] Import table count
	uint8_t import_table_sha1[0x14]; // [0x12C] Import table SHA-1
	uint8_t xgd2_media_id[0x10];	// [0x140] XGD2 media ID
	uint8_t title_key[0x10];	// [0x150] AES-128 title key (encrypted)
	uint32_t export_table;		// [0x160] Export table offset (0 if none)
	uint8_t header_sha1[0x14];	// [0x164] Header SHA-1
	uint32_t region_code;		// [0x178] Region code (See XEX2_Region_Code_e)
	uint32_t allowed_media_types;	// [0x17C] Allowed media types (See XEX2_Media_Types_e)
	uint32_t page_descriptor_count;	// [0x180] Page descriptor count (these follow XEX2_Security_Info)
} XEX2_Security_Info;
ASSERT_STRUCT(XEX2_Security_Info, 0x184);

/**
 * XEX2: Image flags
 */
typedef enum {
	XEX2_IMAGE_FLAG_MANUFACTURING_UTILITY		= (1U <<  1),
	XEX2_IMAGE_FLAG_MANUFACTURING_SUPPORT_TOOLS	= (1U <<  2),
	XEX2_IMAGE_FLAG_XGD2_MEDIA_ONLY			= (1U <<  3),	// Must be on a retail disc.
	XEX2_IMAGE_FLAG_CARDEA_KEY			= (1U <<  8),
	XEX2_IMAGE_FLAG_XEIKA_KEY			= (1U <<  9),
	XEX2_IMAGE_FLAG_USERMODE_TITLE			= (1U << 10),
	XEX2_IMAGE_FLAG_USERMODE_SYSTEM			= (1U << 11),
	XEX2_IMAGE_FLAG_ORANGE0				= (1U << 12),
	XEX2_IMAGE_FLAG_ORANGE1				= (1U << 13),
	XEX2_IMAGE_FLAG_ORANGE2				= (1U << 14),
	XEX2_IMAGE_FLAG_IPTV_SIGNUP_APPLICATION		= (1U << 16),
	XEX2_IMAGE_FLAG_IPTV_TITLE_APPLICATION		= (1U << 17),
	XEX2_IMAGE_FLAG_KEYVAULT_PRIVILEGES_REQUIRED	= (1U << 26),
	XEX2_IMAGE_FLAG_ONLINE_ACTIVATION_REQUIRED	= (1U << 27),
	XEX2_IMAGE_FLAG_PAGE_SIZE_4KB			= (1U << 28),	// Default is 64 KB.
	XEX2_IMAGE_FLAG_REGION_FREE			= (1U << 29),
	XEX2_IMAGE_FLAG_REVOCATION_CHECK_OPTIONAL	= (1U << 30),
	XEX2_IMAGE_FLAG_REVOCATION_CHECK_REQUIRED	= (1U << 31),
} XEX2_Image_Flags_e;

/**
 * XEX2: Media types
 * NOTE: Might be ignored if XEX2_IMAGE_FLAG_XGD2_MEDIA_ONLY is set.
 */
typedef enum {
	XEX2_MEDIA_TYPE_HARDDISK		= (1U <<  0),
	XEX2_MEDIA_TYPE_XGD1			= (1U <<  1),
	XEX2_MEDIA_TYPE_DVD_CD			= (1U <<  2),
	XEX2_MEDIA_TYPE_DVD_5			= (1U <<  3),
	XEX2_MEDIA_TYPE_DVD_9			= (1U <<  4),
	XEX2_MEDIA_TYPE_SYSTEM_FLASH		= (1U <<  5),
	XEX2_MEDIA_TYPE_MEMORY_UNIT		= (1U <<  7),
	XEX2_MEDIA_TYPE_USB_MASS_STORAGE_DEVICE	= (1U <<  8),
	XEX2_MEDIA_TYPE_NETWORK			= (1U <<  9),
	XEX2_MEDIA_TYPE_DIRECT_FROM_MEMORY	= (1U << 10),
	XEX2_MEDIA_TYPE_RAM_DRIVE		= (1U << 11),
	XEX2_MEDIA_TYPE_SVOD			= (1U << 12),
	XEX2_MEDIA_TYPE_INSECURE_PACKAGE	= (1U << 24),
	XEX2_MEDIA_TYPE_SAVEGAME_PACKAGE	= (1U << 25),
	XEX2_MEDIA_TYPE_LOCALLY_SIGNED_PACKAGE	= (1U << 26),
	XEX2_MEDIA_TYPE_LIVE_SIGNED_PACKAGE	= (1U << 27),
	XEX2_MEDIA_TYPE_XBOX_PACKAGE		= (1U << 28),
} XEX2_Media_Types_e;

/**
 * XEX2: Region code
 * Note that certain bits are country-specific.
 */
typedef enum {
	XEX2_REGION_CODE_NTSC_U		= 0x000000FF,
	XEX2_REGION_CODE_NTSC_J		= 0x0000FF00,
	XEX2_REGION_CODE_NTSC_J_OTHER	= 0x0000FC00,
	XEX2_REGION_CODE_NTSC_J_JAPAN	= 0x00000100,
	XEX2_REGION_CODE_NTSC_J_CHINA	= 0x00000200,
	XEX2_REGION_CODE_PAL		= 0x00FF0000,
	XEX2_REGION_CODE_PAL_OTHER	= 0x00FE0000,
	XEX2_REGION_CODE_PAL_AU_NZ	= 0x00010000,
	XEX2_REGION_CODE_OTHER		= 0xFF000000,
	XEX2_REGION_CODE_ALL		= 0xFFFFFFFF,
} XEX2_Region_Code_e;

/**
 * Microsoft Xbox 360 optional header table.
 * An array of this struct is located after the XEX2 header.
 * Count is determined by the `opt_header_count` field.
 *
 * All offsets are absolute addresses, relative to the beginning
 * of the file.
 *
 * All fields are in big-endian.
 */
typedef struct _XEX2_Optional_Header_Tbl {
	uint32_t header_id;	// [0x000] Header ID. (See XEX2_Optional_Header_e.)
	uint32_t offset;	// [0x004] Data/offset, depending on the low byte of Header ID:
				// - 0x00: Field contains a 32-bit value.
				// - 0x01-0xFE: Field contains an address pointing
				//   to a struct, and that struct is 0x01-0xFE
				//   DWORDs in size.
				// - 0xFF: Field contains an address pointing to
				//   a struct, and the first DWORD of the struct
				//   contains its size, in bytes.
} XEX2_Optional_Header_Tbl;
ASSERT_STRUCT(XEX2_Optional_Header_Tbl, 2*sizeof(uint32_t));

/**
 * XEX2 optional header IDs
 */
typedef enum {
	XEX2_OPTHDR_RESOURCE_INFO		=    0x2FF,
	XEX2_OPTHDR_FILE_FORMAT_INFO		=    0x3FF,	// See XEX2_File_Format_Info
	XEX2_OPTHDR_BASE_REFERENCE		=    0x405,
	XEX2_OPTHDR_DELTA_PATCH_DESCRIPTOR	=    0x5FF,
	XEX2_OPTHDR_DISC_PROFILE_ID		=   0x4304,	// AP25 disc profile ID
	XEX2_OPTHDR_BOUNDING_PATH		=   0x80FF,
	XEX2_OPTHDR_DEVICE_ID			=   0x8105,
	XEX2_OPTHDR_ORIGINAL_BASE_ADDRESS	=  0x10001,
	XEX2_OPTHDR_ENTRY_POINT			=  0x10100,
	XEX2_OPTHDR_IMAGE_BASE_ADDRESS		=  0x10201,
	XEX2_OPTHDR_IMPORT_LIBRARIES		=  0x103FF,
	XEX2_OPTHDR_CHECKSUM_TIMESTAMP		=  0x18002,	// See XEX2_Checksum_Timestamp
	XEX2_OPTHDR_ENABLED_FOR_CALLCAP		=  0x18102,
	XEX2_OPTHDR_ENABLED_FOR_FASTCAP		=  0x18200,
	XEX2_OPTHDR_ORIGINAL_PE_NAME		=  0x183FF,	// DWORD with length, followed by filename.
	XEX2_OPTHDR_STATIC_LIBRARIES		=  0x200FF,
	XEX2_OPTHDR_TLS_INFO			=  0x20104,	// See XEX2_TLS_Info.
	XEX2_OPTHDR_DEFAULT_STACK_SIZE		=  0x20200,
	XEX2_OPTHDR_DEFAULT_FS_CACHE_SIZE	=  0x20301,
	XEX2_OPTHDR_DEFAULT_HEAP_SIZE		=  0x20401,
	XEX2_OPTHDR_PAGE_HEAP_SIZE_AND_FLAGS	=  0x28002,
	XEX2_OPTHDR_SYSTEM_FLAGS		=  0x30000,	// See XEX2_System_Flags_e
	XEX2_OPTHDR_EXECUTION_ID		=  0x40006,	// See XEX2_Execution_ID
	XEX2_OPTHDR_SERVICE_ID_LIST		=  0x401FF,
	XEX2_OPTHDR_TITLE_WORKSPACE_SIZE	=  0x40201,
	XEX2_OPTHDR_GAME_RATINGS		=  0x40310,	// See XEX2_Game_Ratings
	XEX2_OPTHDR_LAN_KEY			=  0x40404,	// See XEX2_LAN_Key
	XEX2_OPTHDR_XBOX_360_LOGO		=  0x405FF,
	XEX2_OPTHDR_MULTIDISC_MEDIA_IDS		=  0x406FF,
	XEX2_OPTHDR_ALTERNATE_TITLE_IDS		=  0x407FF,
	XEX2_OPTHDR_ADDITIONAL_TITLE_MEMORY	=  0x40801,
	XEX2_EXPORTS_BY_NAME			= 0xE10402,
} XEX2_Optional_Header_e;

/**
 * XEX2: Resource info (0x2FF)
 *
 * NOTE: This field only has an individual resource.
 * The actual resource info header has a 32-bit size,
 * and may contain multiple resources.
 *
 * All fields are in big-endian.
 */
typedef struct _XEX2_Resource_Info {
	char resource_id[8];	// [0x000] Resource ID. This is usually the
				//         title ID as a hex string.
	uint32_t vaddr;		// [0x00C] Virtual address.
				//         Subtract the image base address to
				//         get the location of the XDBF section
				//         in the decrypted PE executable.
	uint32_t size;		// [0x010] Size of resource, in bytes.
} XEX2_Resource_Info;
ASSERT_STRUCT(XEX2_Resource_Info, 4*sizeof(uint32_t));

/**
 * XEX2: File format info (0x3FF)
 * All fields are in big-endian.
 */
typedef struct _XEX2_File_Format_Info {
	uint32_t size;			// [0x000] Structure size
	uint16_t encryption_type;	// [0x004] Encryption type (See XEX2_Encryption_Type_e)
	uint16_t compression_type;	// [0x006] Compression type (See XEX2_Compression_Type_e)
	// Compression information follows.
	// Not relevant if we don't want to decompress the PE executable.
} XEX2_File_Format_Info;
ASSERT_STRUCT(XEX2_File_Format_Info, 2*sizeof(uint32_t));

/**
 * XEX2: Encryption type
 */
typedef enum {
	XEX2_ENCRYPTION_TYPE_NONE	= 0,
	XEX2_ENCRYPTION_TYPE_NORMAL	= 1,
} XEX2_Encryption_Type_e;

/**
 * XEX2: Compression type
 */
typedef enum {
	XEX2_COMPRESSION_TYPE_NONE	= 0,
	XEX2_COMPRESSION_TYPE_BASIC	= 1,
	XEX2_COMPRESSION_TYPE_NORMAL	= 2,
	XEX2_COMPRESSION_TYPE_DELTA	= 3,
} XEX2_Compression_Type_e;

/**
 * XEX2: Basic compression block.
 * Used with XEX2_COMPRESSION_TYPE_BASIC.
 *
 * Indicates how many bytes of data are in the block,
 * and then how many bytes after the block are zeroes.
 *
 * Located immediately after XEX2_File_Format_Info.
 * XEX2_File_Format_Info's size field can be used to
 * determine the total number of basic compression blocks.
 *
 * All fields are in big-endian.
 */
typedef struct _XEX2_Compression_Basic_Info {
	uint32_t data_size;	// [0x000] Number of valid data bytes.
	uint32_t zero_size;	// [0x004] Number of zero bytes to be inserted after the data bytes.
} XEX2_Compression_Basic_Info;
ASSERT_STRUCT(XEX2_Compression_Basic_Info, 2*sizeof(uint32_t));

/**
 * XEX2: Normal compression block. (LZX)
 * Used with XEX2_COMPRESSION_TYPE_NORMAL.
 *
 * Each section contains the block size and an SHA-1 hash
 * of the decompressed data.
 *
 * The first block is located immediately after XEX2_Compression_Normal_Header.
 * Subsequent block information is located in the compressed data.
 *
 * The uncompressed block size is the first 16-bit BE value
 * of the compressed data block.
 *
 * All fields are in big-endian.
 */
typedef struct _XEX2_Compression_Normal_Info {
	uint32_t block_size;	// [0x000] Compressed block size.
	uint8_t sha1_hash[20];	// [0x004] SHA-1 hash.
} XEX2_Compression_Normal_Info;
ASSERT_STRUCT(XEX2_Compression_Normal_Info, 24);

/**
 * XEX2: Normal compression header. (LZX)
 * Used with XEX2_COMPRESSION_TYPE_NORMAL.
 *
 * Located immediately after XEX2_File_Format_Info.
 * XEX2_File_Format_Info's size field can be used to
 * determine the total number of normal compression blocks
 * located after this header.
 *
 * All fields are in big-endian.
 */
typedef struct _XEX2_Compression_Normal_Header {
	uint32_t window_size;				// [0x000] LZX compression window size.
	XEX2_Compression_Normal_Info first_block;	// [0x004] First block information.
} XEX2_Compression_Normal_Header;
ASSERT_STRUCT(XEX2_Compression_Normal_Header, sizeof(uint32_t) + 24);

/**
 * XEX2: Import libraries (0x103FF)
 * All fields are in big-endian.
 */
typedef struct _XEX2_Import_Libraries_Header {
	uint32_t size;		// [0x000] Size of the library header
	uint32_t str_tbl_size;	// [0x004] String table size, in bytes
	uint32_t str_tbl_count;	// [0x008] Number of string table entries

	// The string table is located immediately after
	// this header. Located immediately after the
	// string table is the list of import libraries.
} XEX2_Import_Libraries_Header;
ASSERT_STRUCT(XEX2_Import_Libraries_Header, 3*sizeof(uint32_t));

/**
 * XEX2: Import library entry.
 * Located immediately after the import library string table.
 *
 * All fields are in big-endian.
 */
typedef struct _XEX2_Import_Library_Entry {
	uint32_t size;			// [0x000] Size of entry
	uint8_t next_import_digest[20];	// [0x004] SHA1 of the *next* entry?
	uint32_t id;			// [0x018] Library ID
	Xbox360_Version_t version;	// [0x01C] Library version
	Xbox360_Version_t version_min;	// [0x020] Minimum library version
	uint16_t name_index;		// [0x024] Library name (index in string table)
	uint16_t count;			// [0x026] Number of imports

	// The import listing for this library is located immediately
	// after this struct. Each import is a single 32-bit value.
} XEX2_Import_Library_Entry;
ASSERT_STRUCT(XEX2_Import_Library_Entry, 0x28);

/**
 * XEX2: Checksum and timestamp (0x18002)
 * All fields are in big-endian.
 */
typedef struct _XEX2_Checksum_Timestamp {
	uint32_t checksum;	// [0x000] Checksum (???)
	uint32_t filetime;	// [0x004] Timestamp (UNIX time)
} XEX2_Checksum_Timestamp;
ASSERT_STRUCT(XEX2_Checksum_Timestamp, 2*sizeof(uint32_t));

/**
 * XEX2: TLS info (0x20104)
 * All fields are in big-endian.
 */
typedef struct _XEX2_TLS_Info {
	uint32_t slot_count;		// [0x000]
	uint32_t raw_data_address;	// [0x004]
	uint32_t data_size;		// [0x008]
	uint32_t raw_data_size;		// [0x00C]
} XEX2_TLS_Info;
ASSERT_STRUCT(XEX2_TLS_Info, 4*sizeof(uint32_t));

/**
 * XEX2: System flags (0x30000)
 */
typedef enum {
	XEX2_SYSTEM_FLAG_NO_FORCED_REBOOT			= (1U <<  0),
	XEX2_SYSTEM_FLAG_FOREGROUND_TASKS			= (1U <<  1),
	XEX2_SYSTEM_FLAG_NO_ODD_MAPPING				= (1U <<  2),
	XEX2_SYSTEM_FLAG_HANDLE_MCE_INPUT			= (1U <<  3),
	XEX2_SYSTEM_FLAG_RESTRICTED_HUD_FEATURES		= (1U <<  4),
	XEX2_SYSTEM_FLAG_HANDLE_GAMEPAD_DISCONNECT		= (1U <<  5),
	XEX2_SYSTEM_FLAG_INSECURE_SOCKETS			= (1U <<  6),
	XEX2_SYSTEM_FLAG_XBOX1_INTEROPERABILITY			= (1U <<  7),
	XEX2_SYSTEM_FLAG_DASH_CONTEXT				= (1U <<  8),
	XEX2_SYSTEM_FLAG_USES_GAME_VOICE_CHANNEL		= (1U <<  9),
	XEX2_SYSTEM_FLAG_PAL50_INCOMPATIBLE			= (1U << 10),
	XEX2_SYSTEM_FLAG_INSECURE_UTILITY_DRIVE			= (1U << 11),
	XEX2_SYSTEM_FLAG_XAM_HOOKS				= (1U << 12),
	XEX2_SYSTEM_FLAG_ACCESS_PII				= (1U << 13),
	XEX2_SYSTEM_FLAG_CROSS_PLATFORM_SYSTEM_LINK		= (1U << 14),
	XEX2_SYSTEM_FLAG_MULTIDISC_SWAP				= (1U << 15),
	XEX2_SYSTEM_FLAG_MULTIDISC_INSECURE_MEDIA		= (1U << 16),
	XEX2_SYSTEM_FLAG_AP25_MEDIA				= (1U << 17),
	XEX2_SYSTEM_FLAG_NO_CONFIRM_EXIT			= (1U << 18),
	XEX2_SYSTEM_FLAG_ALLOW_BACKGROUND_DOWNLOAD		= (1U << 19),
	XEX2_SYSTEM_FLAG_CREATE_PERSISTABLE_RAMDRIVE		= (1U << 20),
	XEX2_SYSTEM_FLAG_INHERIT_PERSISTENT_RAMDRIVE		= (1U << 21),
	XEX2_SYSTEM_FLAG_ALLOW_HUD_VIBRATION			= (1U << 22),
	XEX2_SYSTEM_FLAG_ACCESS_UTILITY_PARTITIONS		= (1U << 23),
	XEX2_SYSTEM_FLAG_IPTV_INPUT_SUPPORTED			= (1U << 24),
	XEX2_SYSTEM_FLAG_PREFER_BIG_BUTTON_INPUT		= (1U << 25),
	XEX2_SYSTEM_FLAG_ALLOW_EXTENDED_SYSTEM_RESERVATION	= (1U << 26),
	XEX2_SYSTEM_FLAG_MULTIDISC_CROSS_TITLE			= (1U << 27),
	XEX2_SYSTEM_FLAG_INSTALL_INCOMPATIBLE			= (1U << 28),
	XEX2_SYSTEM_FLAG_ALLOW_AVATAR_GET_METADATA_BY_XUID	= (1U << 29),
	XEX2_SYSTEM_FLAG_ALLOW_CONTROLLER_SWAPPING		= (1U << 30),
	XEX2_SYSTEM_FLAG_DASH_EXTENSIBILITY_MODULE		= (1U << 31),
} XEX2_System_Flags_e;

/**
 * XEX2: Execution ID (0x40006)
 * All fields are in big-endian.
 */
typedef struct _XEX2_Execution_ID {
	uint32_t media_id;		// [0x000] Media ID
	Xbox360_Version_t version;	// [0x004] Version
	Xbox360_Version_t base_version;	// [0x008] Base version
	Xbox360_Title_ID title_id;	// [0x00C] Title ID (two characters, and uint16_t)
	uint8_t platform;		// [0x010] Platform
	uint8_t exec_type;		// [0x011] Executable type
	uint8_t disc_number;		// [0x012] Disc number
	uint8_t disc_count;		// [0x013] Number of discs
	uint32_t savegame_id;		// [0x014] Savegame ID.
} XEX2_Execution_ID;
ASSERT_STRUCT(XEX2_Execution_ID, 24);

/**
 * XEX2: Game ratings. (0x40310)
 *
 * NOTE: This field is supposed to be 10 DWORDs,
 * but only 14 rating regions have been assigned.
 */
// Some compilers pad this structure to a multiple of 4 bytes
#pragma pack(1)
typedef union RP_PACKED _XEX2_Game_Ratings {
	uint8_t ratings[14];
	struct RP_PACKED {
		uint8_t esrb;		// [0x000] See XEX2_Game_Ratings_ESRB_e
		uint8_t pegi;		// [0x001] See XEX2_Game_Ratings_PEGI_e
		uint8_t pegi_fi;	// [0x002] See XEX2_Game_Ratings_PEGI_FI_e
		uint8_t pegi_pt;	// [0x003] See XEX2_Game_Ratings_PEGI_PT_e
		uint8_t bbfc;		// [0x004] See XEX2_Game_Ratings_BBFC
		uint8_t cero;		// [0x005] See XEX2_Game_Ratings_CERO_e
		uint8_t usk;		// [0x006] See XEX2_Game_Ratings_USK_e
		uint8_t oflc_au;	// [0x007] See XEX2_Game_Ratings_OFLC_AU_e
		uint8_t oflc_nz;	// [0x008] See XEX2_Game_Ratings_OFLC_NZ_e
		uint8_t kmrb;		// [0x009] See XEX2_Game_Ratings_KMRB_e
		uint8_t brazil;		// [0x00A] See XEX2_Game_Ratings_Brazil_e
		uint8_t fpb;		// [0x00B] See XEX2_Game_Ratings_FPB_e
		uint8_t taiwan;		// [0x00C]
		uint8_t singapore;	// [0x00D]
	};
} XEX2_Game_Ratings;
ASSERT_STRUCT(XEX2_Game_Ratings, 14);
#pragma pack()

/**
 * XEX2: ESRB ratings value.
 */
typedef enum {
	XEX2_GAME_RATINGS_ESRB_eC	= 0,
	XEX2_GAME_RATINGS_ESRB_E	= 2,
	XEX2_GAME_RATINGS_ESRB_E10	= 4,
	XEX2_GAME_RATINGS_ESRB_T	= 6,
	XEX2_GAME_RATINGS_ESRB_M	= 8,
	XEX2_GAME_RATINGS_ESRB_AO	= 14,
	XEX2_GAME_RATINGS_ESRB_UNRATED	= 0xFF,
} XEX2_Game_Ratings_ESRB_e;

/**
 * XEX2: PEGI ratings value.
 */
typedef enum {
	XEX2_GAME_RATINGS_PEGI_3_PLUS	= 0,
	XEX2_GAME_RATINGS_PEGI_7_PLUS	= 4,
	XEX2_GAME_RATINGS_PEGI_12_PLUS	= 9,
	XEX2_GAME_RATINGS_PEGI_16_PLUS	= 13,
	XEX2_GAME_RATINGS_PEGI_18_PLUS	= 14,
	XEX2_GAME_RATINGS_PEGI_UNRATED	= 0xFF,
} XEX2_Game_Ratings_PEGI_e;

/**
 * XEX2: PEGI (Finland) ratings value.
 */
typedef enum {
	XEX2_GAME_RATINGS_PEGI_FI_3_PLUS	= 0,
	XEX2_GAME_RATINGS_PEGI_FI_7_PLUS	= 4,
	XEX2_GAME_RATINGS_PEGI_FI_11_PLUS	= 8,
	XEX2_GAME_RATINGS_PEGI_FI_15_PLUS	= 12,
	XEX2_GAME_RATINGS_PEGI_FI_18_PLUS	= 14,
	XEX2_GAME_RATINGS_PEGI_FI_UNRATED	= 0xFF,
} XEX2_Game_Ratings_PEGI_FI_e;

/**
 * XEX2: PEGI (Portugal) ratings value.
 */
typedef enum {
	XEX2_GAME_RATINGS_PEGI_PT_4_PLUS	= 1,
	XEX2_GAME_RATINGS_PEGI_PT_6_PLUS	= 3,
	XEX2_GAME_RATINGS_PEGI_PT_12_PLUS	= 9,
	XEX2_GAME_RATINGS_PEGI_PT_16_PLUS	= 13,
	XEX2_GAME_RATINGS_PEGI_PT_18_PLUS	= 14,
	XEX2_GAME_RATINGS_PEGI_PT_UNRATED	= 0xFF,
} XEX2_Game_Ratings_PEGI_PT_e;

/**
 * XEX2: BBFC ratings value.
 */
typedef enum {
	XEX2_GAME_RATINGS_BBFC_UNIVERSAL	= 1,
	XEX2_GAME_RATINGS_BBFC_PG		= 5,
	XEX2_GAME_RATINGS_BBFC_3_PLUS		= 0,
	XEX2_GAME_RATINGS_BBFC_7_PLUS		= 4,
	XEX2_GAME_RATINGS_BBFC_12_PLUS		= 9,
	XEX2_GAME_RATINGS_BBFC_15_PLUS		= 12,
	XEX2_GAME_RATINGS_BBFC_16_PLUS		= 13,
	XEX2_GAME_RATINGS_BBFC_18_PLUS		= 14,
	XEX2_GAME_RATINGS_BBFC_UNRATED		= 0xFF,
} XEX2_Game_Ratings_BBFC_e;

/**
 * XEX2: CERO ratings value.
 */
typedef enum {
	XEX2_GAME_RATINGS_CERO_A	= 0,
	XEX2_GAME_RATINGS_CERO_B	= 2,
	XEX2_GAME_RATINGS_CERO_C	= 4,
	XEX2_GAME_RATINGS_CERO_D	= 6,
	XEX2_GAME_RATINGS_CERO_Z	= 8,
	XEX2_GAME_RATINGS_CERO_UNRATED	= 0xFF,
} XEX2_Game_Ratings_CERO_e;

/**
 * XEX2: USK ratings value.
 */
typedef enum {
	XEX2_GAME_RATINGS_USK_ALL	= 0,
	XEX2_GAME_RATINGS_USK_6_PLUS	= 2,
	XEX2_GAME_RATINGS_USK_12_PLUS	= 4,
	XEX2_GAME_RATINGS_USK_16_PLUS	= 6,
	XEX2_GAME_RATINGS_USK_18_PLUS	= 8,
	XEX2_GAME_RATINGS_USK_UNRATED	= 0xFF,
} XEX2_Game_Ratings_USK_e;

/**
 * XEX2: OFLC (AU) ratings value.
 */
typedef enum {
	XEX2_GAME_RATINGS_OFLC_AU_G		= 0,
	XEX2_GAME_RATINGS_OFLC_AU_PG		= 2,
	XEX2_GAME_RATINGS_OFLC_AU_M		= 4,
	XEX2_GAME_RATINGS_OFLC_AU_MA15_PLUS	= 6,
	XEX2_GAME_RATINGS_OFLC_AU_UNRATED	= 0xFF,
} XEX2_Game_Ratings_OFLC_AU_e;

/**
 * XEX2: OFLC (NZ) ratings value.
 */
typedef enum {
	XEX2_GAME_RATINGS_OFLC_NZ_G		= 0,
	XEX2_GAME_RATINGS_OFLC_NZ_PG		= 2,
	XEX2_GAME_RATINGS_OFLC_NZ_M		= 4,
	XEX2_GAME_RATINGS_OFLC_NZ_MA15_PLUS	= 6,
	XEX2_GAME_RATINGS_OFLC_NZ_UNRATED	= 0xFF,
} XEX2_Game_Ratings_OFLC_NZ_e;

/**
 * XEX2: KMRB ratings value.
 * NOTE: This is now the GRB.
 */
typedef enum {
	XEX2_GAME_RATINGS_KMRB_ALL	= 0,
	XEX2_GAME_RATINGS_KMRB_12_PLUS	= 2,
	XEX2_GAME_RATINGS_KMRB_15_PLUS	= 4,
	XEX2_GAME_RATINGS_KMRB_18_PLUS	= 6,
	XEX2_GAME_RATINGS_KMRB_UNRATED	= 0xFF,
} XEX2_Game_Ratings_KMRB_e;

/**
 * XEX2: Brazil ratings value.
 */
typedef enum {
	XEX2_GAME_RATINGS_BRAZIL_ALL		= 0,
	XEX2_GAME_RATINGS_BRAZIL_12_PLUS	= 2,
	XEX2_GAME_RATINGS_BRAZIL_14_PLUS	= 4,
	XEX2_GAME_RATINGS_BRAZIL_16_PLUS	= 5,
	XEX2_GAME_RATINGS_BRAZIL_18_PLUS	= 8,
	XEX2_GAME_RATINGS_BRAZIL_UNRATED	= 0xFF,
} XEX2_Game_Ratings_Brazil_e;

/**
 * XEX2: FPB ratings value.
 */
typedef enum {
	XEX2_GAME_RATINGS_FPB_ALL	= 0,
	XEX2_GAME_RATINGS_FPB_PG	= 6,
	XEX2_GAME_RATINGS_FPB_10_PLUS	= 7,
	XEX2_GAME_RATINGS_FPB_13_PLUS	= 10,
	XEX2_GAME_RATINGS_FPB_16_PLUS	= 13,
	XEX2_GAME_RATINGS_FPB_18_PLUS	= 14,
	XEX2_GAME_RATINGS_FPB_UNRATED	= 0xFF,
} XEX2_Game_Ratings_FPB_e;

/**
 * XEX2: LAN key. (0x40404)
 */
typedef struct _XEX2_LAN_Key {
	uint8_t key[16];
} XEX2_LAN_Key;
ASSERT_STRUCT(XEX2_LAN_Key, 16);

#ifdef __cplusplus
}
#endif
