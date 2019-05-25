/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * xbox_xbe_structs.h: Microsoft Xbox executable data structures.          *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_CONSOLE_XBOX_XBE_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_CONSOLE_XBOX_XBE_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

/**
 * Microsoft Xbox executable header.
 * References:
 * - http://xboxdevwiki.net/Xbe
 * - http://www.caustik.com/cxbx/download/xbe.htm
 *
 * Entry point is XOR'd with a key depending on if it's debug or retail.
 * To determine the type, XOR it with debug, then check if the entry point
 * is >= base address and < 128 MB. If not, try again with retail and check
 * for >= base address and < 64 MB.
 *
 * Addresses are generally relative to the executable when loaded in memory.
 * Note that Xbox loads the executable directly into the base address with
 * no special section management, so we can simply subtract the base address
 * from the memory address to get the file offset.
 *
 * All fields are in little-endian.
 */
#define XBE_MAGIC 'XBEH'
#define XBE_ENTRY_POINT_KEY_RETAIL	0xA8FC57AB
#define XBE_ENTRY_POINT_KEY_DEBUG	0x94859D4B
#define XBE_KERNEL_THUNK_KEY_RETAIL	0x5B6D40B6
#define XBE_KERNEL_THUNK_KEY_DEBUG	0xEFB1F152
typedef struct PACKED _XBE_Header {
	uint32_t magic;				// [0x000] 'XBEH'
	uint8_t signature[256];			// [0x004] RSA-2048 digital signature
	uint32_t base_address;			// [0x104] Base address (usually 0x00010000)
	uint32_t total_header_size;		// [0x108] Size of all headers
	uint32_t image_size;			// [0x10C] Image size
	uint32_t image_header_size;		// [0x110] Size of the image header
	uint32_t timestamp;			// [0x114] UNIX timestamp
	uint32_t cert_address;			// [0x118] Certificate address (in memory)
	uint32_t section_count;			// [0x11C] Number of sections
	uint32_t section_headers_address;	// [0x120] Address of SectionHeader structs (in memory)
	uint32_t init_flags;			// [0x124] Initialization flags (See XBE_InitFlags_e.)
	uint32_t entry_point;			// [0x128] Entry point (XOR'd with Retail or Debug key)
	uint32_t tls_address;			// [0x12C] TLS address

	// The following fields are taken directly from
	// the original PE executable.
	uint32_t pe_stack_commit;		// [0x130]
	uint32_t pe_heap_reserve;		// [0x134]
	uint32_t pe_heap_commit;		// [0x138]
	uint32_t pe_base_address;		// [0x13C]
	uint32_t pe_size_of_image;		// [0x140]
	uint32_t pe_checksum;			// [0x144]
	uint32_t pe_timestamp;			// [0x148]

	uint32_t debug_pathname_address;	// [0x14C] Address to debug pathname
	uint32_t debug_filename_address;	// [0x150] Address to debug filename
						//         (usually points to the filename
						//          portion of the debug pathname)
	uint32_t debug_filenameW_address;	// [0x154] Address to Unicode debug filename
	uint32_t kernel_thunk_address;		// [0x158] Kernel image thunk address
						//         (XOR'd with Retail or Debug key)

	uint32_t nonkernel_import_dir_address;		// [0x15C]
	uint32_t library_version_count;			// [0x160]
	uint32_t library_version_address;		// [0x164]
	uint32_t kernel_library_version_address;	// [0x168]
	uint32_t xapi_library_version_address;		// [0x16C]

	// Logo (usually a Microsoft logo)
	// Encoded using RLE.
	uint32_t logo_bitmap_address;		// [0x170]
	uint32_t logo_bitmap_size;		// [0x174]
} XBE_Header;
ASSERT_STRUCT(XBE_Header, 0x178);

/**
 * Initialization flags
 */
typedef enum {
	XBE_INIT_FLAG_MountUtilityDrive		= 0x00000001,
	XBE_INIT_FLAG_FormatUtilityDrive	= 0x00000002,
	XBE_INIT_FLAG_Limit64Megabytes		= 0x00000004,
	XBE_INIT_FLAG_DontSetupHarddisk		= 0x00000008,
} XBE_InitFlags_e;

/**
 * XBE: Title ID
 * Contains two characters and a 16-bit number.
 * NOTE: Struct positioning only works with the original LE32 value.
 */
typedef union PACKED _XBE_Title_ID {
	struct {
		uint16_t u16;
		char b;
		char a;
	};
	uint32_t u32;
} XBE_Title_ID;
ASSERT_STRUCT(XBE_Title_ID, sizeof(uint32_t));

/**
 * XBE certificate.
 * Reference: http://www.caustik.com/cxbx/download/xbe.htm
 *
 * All fields are in little-endian.
 */
typedef struct PACKED _XBE_Certificate {
	uint32_t size;				// [0x000] Size of certificate
	uint32_t timestamp;			// [0x004] UNIX timestamp
	XBE_Title_ID title_id;			// [0x008] Title ID
	char16_t title_name[40];		// [0x00C] Title name (UTF-16LE)
	uint32_t alt_title_ids[16];		// [0x05C] Alternate title IDs
	uint32_t allowed_media_types;		// [0x09C] Allowed media (bitfield) (see XBE_Media_e)
	uint32_t region_code;			// [0x0A0] Region code (see XBE_Region_Code_e)
	uint32_t ratings;			// [0x0A4] Age ratings (TODO)
	uint32_t disc_number;			// [0x0A8] Disc number
	uint32_t cert_version;			// [0x0AC] Certificate version
	uint8_t lan_key[16];			// [0x0B0] LAN key
	uint8_t signature_key[16];		// [0x0C0] Signature key
	uint8_t alt_signature_keys[16][16];	// [0x0D0] Alternate signature keys
} XBE_Certificate;
ASSERT_STRUCT(XBE_Certificate, 0x1D0);

/**
 * Allowed media (bitfield)
 */
typedef enum {
	XBE_MEDIA_TYPE_HARD_DISK		= 0x00000001,
	XBE_MEDIA_TYPE_XGD1			= 0x00000002,
	XBE_MEDIA_TYPE_DVD_CD			= 0x00000004,
	XBE_MEDIA_TYPE_CD			= 0x00000008,
	XBE_MEDIA_TYPE_DVD_5_RO			= 0x00000010,
	XBE_MEDIA_TYPE_DVD_9_RO			= 0x00000020,
	XBE_MEDIA_TYPE_DVD_5_RW			= 0x00000040,
	XBE_MEDIA_TYPE_DVD_9_RW			= 0x00000080,
	XBE_MEDIA_TYPE_DONGLE			= 0x00000100,
	XBE_MEDIA_TYPE_MEDIA_BOARD		= 0x00000200,
	XBE_MEDIA_TYPE_NONSECURE_HARD_DISK	= 0x40000000,
	XBE_MEDIA_TYPE_NONSECURE_MODE		= 0x80000000,

	XBE_MEDIA_TYPE_MEDIA_MASK		= 0x00FFFFFF,
} XBE_Media_e;

/**
 * Region code (bitfield)
 */
typedef enum {
	XBE_REGION_CODE_NORTH_AMERICA	= 0x00000001,
	XBE_REGION_CODE_JAPAN		= 0x00000002,
	XBE_REGION_CODE_RESTOFWORLD	= 0x00000004,
	XBE_REGION_CODE_MANUFACTURING	= 0x80000000,
} XBE_Region_Code_e;

/**
 * XBE section header.
 * Reference: http://www.caustik.com/cxbx/download/xbe.htm
 *
 * All fields are in little-endian.
 */
typedef struct PACKED _XBE_Section_Header {
	uint32_t flags;			// [0x000] Section flags (See XBE_Section_Flags_e)
	uint32_t vaddr;			// [0x004] Virtual load address for this section
	uint32_t vsize;			// [0x008] Size of this section
	uint32_t paddr;			// [0x00C] Physical address in the XBE file
	uint32_t psize;			// [0x010] Physical size of this section
	uint32_t section_name_address;	// [0x014] Address of the section name (in memory)
	uint32_t section_name_refcount;	// [0x018]
	uint32_t head_shared_page_recount_address;	// [0x01C]
	uint32_t tail_shared_page_recount_address;	// [0x020]
	uint8_t sha1_digest[20];	// [0x024]
} XBE_Section_Header;
ASSERT_STRUCT(XBE_Section_Header, 0x38);

/**
 * Section flags
 */
typedef enum {
	XBE_SECTION_FLAG_Writable		= 0x00000001,
	XBE_SECTION_FLAG_Preload		= 0x00000002,
	XBE_SECTION_FLAG_Executable		= 0x00000004,
	XBE_SECTION_FLAG_Inserted_File		= 0x00000008,
	XBE_SECTION_FLAG_Head_Page_Read_Only	= 0x00000010,
	XBE_SECTION_FLAG_Tail_Page_Read_Only	= 0x00000020,
} XBE_Section_Flags_e;

/**
 * Library version.
 * Reference: http://www.caustik.com/cxbx/download/xbe.htm
 *
 * All fields are in little-endian.
 */
typedef struct PACKED _XBE_Library_Version {
	char name[8];			// [0x000] Library name
	uint16_t version_major;		// [0x008] Major version number
	uint16_t version_minor;		// [0x00A] Minor version number
	uint16_t version_build;		// [0x00C] Build number
	uint16_t flags;			// [0x00E] Flags (see XBE_Library_Version_Flags_e)
} XBE_Library_Version;
ASSERT_STRUCT(XBE_Library_Version, 16);

/**
 * Library version flags
 */
typedef enum {
	XBE_LIB_FLAG_QFEVersion		= 0x1FFF,	// 13-bit mask
	XBE_LIB_FLAG_Approved		= 0x6000,	// 2-bit mask
	XBE_LIB_FLAG_DebugBuild		= 0x8000,	// 1-bit mask
} XBE_Library_Version_Flags_e;

/**
 * TLS struct.
 * Reference: http://www.caustik.com/cxbx/download/xbe.htm
 *
 * All fields are in little-endian.
 */
typedef struct PACKED _XBE_TLS {
	uint32_t data_start_address;	// [0x000]
	uint32_t data_end_address;	// [0x004]
	uint32_t tls_index_address;	// [0x008]
	uint32_t tls_callback_address;	// [0x00C]
	uint32_t size_zero_fill;	// [0x010]
	uint32_t characteristics;	// [0x014]
} XBE_TLS;
ASSERT_STRUCT(XBE_TLS, 0x18);

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_CONSOLE_XBOX_XBE_STRUCTS_H__ */
