/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * xbox360_xdbf_structs.h: Microsoft Xbox 360 game resource structures.    *
 *                                                                         *
 * Copyright (c) 2019-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

// NOTE: Entries begin after all headers:
// - XDBF_Header
// - XDBF_Entry * entry_table_length
// - XDBG_Free_Space_Entry * free_space_table_length

/**
 * Microsoft Xbox 360 XDBF header.
 * References:
 * - https://github.com/xenia-project/xenia/blob/HEAD/src/xenia/kernel/util/xdbf_utils.h
 * - https://github.com/xenia-project/xenia/blob/HEAD/src/xenia/kernel/util/xdbf_utils.cc
 * - https://github.com/Free60Project/wiki/blob/master/docs/XDBF.md
 * - https://github.com/Free60Project/wiki/blob/master/docs/GPD.md
 * - https://github.com/Free60Project/wiki/blob/master/docs/SPA.md
 *
 * All fields are in big-endian.
 */
#define XDBF_MAGIC 'XDBF'
#define XDBF_VERSION 0x10000
typedef struct _XDBF_Header {
	uint32_t magic;				// [0x000] 'XDBF'
	uint32_t version;			// [0x004] Version (0x10000)
	uint32_t entry_table_length;		// [0x008] Entry table length, in number of entries
	uint32_t entry_count;			// [0x00C] Entry count (# of used entries)
	uint32_t free_space_table_length;	// [0x010] Free space table length, in number of entries
	uint32_t free_space_table_count;	// [0x014] Free space table entry count (# of used entries)
} XDBF_Header;
ASSERT_STRUCT(XDBF_Header, 6*sizeof(uint32_t));

// Title resource ID.
// This resource ID contains the game title in each language-specific string table.
// (Namespace XDBF_SPA_NAMESPACE_STRING, ID from XDBF_Language_e)
// It's also used for the dashboard icon. (Namespace XDBF_SPA_IMAGE)
// For the game's default language, see the 'XSTC' block.
#define XDBF_ID_TITLE 0x8000

/**
 * XDBF entry
 * All fields are in big-endian.
 */
#pragma pack(1)
typedef struct RP_PACKED _XDBF_Entry {
	uint16_t namespace_id;		// [0x000] See XDBF_Namespace_e
	uint64_t resource_id;		// [0x002] ID
	uint32_t offset;		// [0x00A] Offset specifier
	uint32_t length;		// [0x00E] Length
} XDBF_Entry;
ASSERT_STRUCT(XDBF_Entry, 18);
#pragma pack()

/**
 * XDBG free space table entry
 * All fields are in big-endian.
 */
typedef struct _XDBF_Free_Space_Entry {
	uint32_t offset;		// [0x000] Offset specifier
	uint32_t length;		// [0x004] Length
} XDBF_Free_Space_Entry;
ASSERT_STRUCT(XDBF_Free_Space_Entry, 2*sizeof(uint32_t));

/**
 * XDBF: Namespace IDs
 */
typedef enum {
	/** SPA (XEX XDBF) **/
	XDBF_SPA_NAMESPACE_METADATA		= 1,	// Metadata
	XDBF_SPA_NAMESPACE_IMAGE		= 2,	// Image (usually PNG format)
	XDBF_SPA_NAMESPACE_STRING_TABLE		= 3,	// String table (ID == XDBF_Language_e)

	/** GPD **/
	XDBF_GPD_NAMESPACE_ACHIEVEMENT		= 1,	// Achievement
	XDBF_GPD_NAMESPACE_IMAGE		= 2,	// Image (same as SPA)
	XDBF_GPD_NAMESPACE_SETTING		= 3,
	XDBF_GPD_NAMESPACE_TITLE		= 4,
	XDBF_GPD_NAMESPACE_STRING		= 5,
	XDBF_GPD_NAMESPACE_ACHIEVEMENT_SECURITY_GFWL	= 6,
	XDBF_GPD_NAMESPACE_AVATAR_AWARD_360	= 6,
} XDBF_Namespace_e;

// Special entry IDs for Sync List and Sync Data. (GPD)
#define XDBF_GPD_SYNC_LIST_ENTRY	0x0000000100000000U
#define XDBF_GPD_SYNC_DATA_ENTRY	0x0000000200000000U

/**
 * XSTC: Default language block.
 * Namespace ID: XDBF_NAMESPACE_METADATA
 * ID: XDBF_XSTC_MAGIC
 * All fields are in big-endian.
 */
#define XDBF_XSTC_MAGIC 'XSTC'
#define XDBF_XSTC_VERSION 1
typedef struct _XDBF_XSTC {
	uint32_t magic;			// [0x000] 'XSTC'
	uint32_t version;		// [0x004] Version (1)
	uint32_t size;			// [0x008] sizeof(XDBF_XSTC) - sizeof(uint32_t)
	uint32_t default_language;	// [0x00C] See XDBF_Language_e
} XDBF_XSTC;
ASSERT_STRUCT(XDBF_XSTC, 4*sizeof(uint32_t));

/**
 * XDBF: Language IDs
 */
typedef enum {
	XDBF_LANGUAGE_UNKNOWN		= 0,
	XDBF_LANGUAGE_ENGLISH		= 1,
	XDBF_LANGUAGE_JAPANESE		= 2,
	XDBF_LANGUAGE_GERMAN		= 3,
	XDBF_LANGUAGE_FRENCH		= 4,
	XDBF_LANGUAGE_SPANISH		= 5,
	XDBF_LANGUAGE_ITALIAN		= 6,
	XDBF_LANGUAGE_KOREAN		= 7,
	XDBF_LANGUAGE_CHINESE_TRAD	= 8,	// Traditional Chinese
	XDBF_LANGUAGE_PORTUGUESE	= 9,
	XDBF_LANGUAGE_CHINESE_SIMP	= 10,	// Simplified Chinese
	XDBF_LANGUAGE_POLISH		= 11,
	XDBF_LANGUAGE_RUSSIAN		= 12,

	XDBF_LANGUAGE_MAX
} XDBF_Language_e;

/** String tables **/
/** NOTE: String tables are encoded using UTF-8. **/

/**
 * XDBF: String table header
 * Namespace ID: XDBF_NAMESPACE_STRING_TABLE
 * ID: See XDBF_Language_e
 * All fields are in big-endian.
 */
#define XDBF_XSTR_MAGIC 'XSTR'
#define XDBF_XSTR_VERSION 1
#pragma pack(1)
typedef struct RP_PACKED _XDBF_XSTR_Header {
	uint32_t magic;		// [0x000] 'XSTR'
	uint32_t version;	// [0x004] Version (1)
	uint32_t size;		// [0x008] Size
	uint16_t string_count;	// [0x00C] String count
} XDBF_XSTR_Header;
ASSERT_STRUCT(XDBF_XSTR_Header, 14);
#pragma pack()

/**
 * XDBF: String table entry header
 * All fields are in big-endian.
 */
typedef struct _XDBF_XSTR_Entry_Header {
	uint16_t string_id;	// [0x000] ID
	uint16_t length;	// [0x002] String length (NOT NULL-terminated)
} XDBF_XSTR_Entry_Header;
ASSERT_STRUCT(XDBF_XSTR_Entry_Header, 2*sizeof(uint16_t));

/**
 * XDBF: Title ID
 * Contains two characters and a 16-bit number.
 * NOTE: Struct positioning only works with the original BE32 value.
 * TODO: Combine with XEX2 version.
 */
typedef union _XDBF_Title_ID {
	struct {
		char c[2];
		uint16_t u16;
	};
	uint32_t u32;
} XDBF_Title_ID;
ASSERT_STRUCT(XDBF_Title_ID, sizeof(uint32_t));

/**
 * XDBF: XACH - Achievements table
 * All fields are in big-endian.
 */
#define XDBF_XACH_MAGIC 'XACH'
#define XDBF_XACH_VERSION 1
#pragma pack(1)
typedef struct RP_PACKED _XDBF_XACH_Header {
	uint32_t magic;		// [0x000] 'XACH'
	uint32_t version;	// [0x004] Version (1)
	uint32_t size;		// [0x008] Structure size, minus magic
	uint16_t xach_count;	// [0x00C] Achivement count.
				// NOTE: Should be compared to structure size
				// and XDBF table entry.
	// Following XDBF_XACH_Header are xach_count instances
	// of XDBF_XACH_Entry.
} XDBF_XACH_Header;
ASSERT_STRUCT(XDBF_XACH_Header, 14);
#pragma pack()

/**
 * XDBF: XACH - Achievements table entry (SPA)
 * All fields are in big-endian.
 */
typedef struct _XDBF_XACH_Entry_SPA {
	uint16_t achievement_id;	// [0x000] Achievement ID
	uint16_t name_id;		// [0x002] Name ID (string table)
	uint16_t unlocked_desc_id;	// [0x004] Unlocked description ID (string table)
	uint16_t locked_desc_id;	// [0x006] Locked description ID (string table)
	uint32_t image_id;		// [0x008] Image ID
	uint16_t gamerscore;		// [0x00C] Gamerscore
	uint16_t unknown1;		// [0x00E]
	uint32_t flags;			// [0x010] Flags (See XDBF_XACH_Flags_e)
	uint32_t unknown2[4];		// [0x014]
} XDBF_XACH_Entry_SPA;
ASSERT_STRUCT(XDBF_XACH_Entry_SPA, 0x24);

/**
 * XDBF: XACH - Achievements table entry header (GPD)
 * All fields are in big-endian.
 */
#pragma pack(1)
typedef struct RP_PACKED _XDBF_XACH_Entry_Header_GPD {
	uint32_t size;			// [0x000] Struct size (0x1C)
	uint32_t achievement_id;	// [0x004] Achievement ID
	uint32_t image_id;		// [0x008] Image ID
	uint32_t gamerscore;		// [0x00C] Gamerscore
	uint32_t flags;			// [0x010] Flags (See XDBF_XACH_Flags_e)
	uint64_t unlock_time;		// [0x014] Unlock time

	// Following the struct are three UTF-16BE NULL-terminated strings,
	// in the following order:
	// - Name
	// - Unlocked description
	// - Locked description
} XDBF_XACH_Entry_Header_GPD;
ASSERT_STRUCT(XDBF_XACH_Entry_Header_GPD, 0x1C);
#pragma pack()

/**
 * XDBF: XACH - Achievements flags.
 */
typedef enum {
	// Achievement type
	XDBF_XACH_TYPE_COMPLETION	= 1U,
	XDBF_XACH_TYPE_LEVELING		= 2U,
	XDBF_XACH_TYPE_UNLOCK		= 3U,
	XDBF_XACH_TYPE_EVENT		= 4U,
	XDBF_XACH_TYPE_TOURNAMENT	= 5U,
	XDBF_XACH_TYPE_CHECKPOINT	= 6U,
	XDBF_XACH_TYPE_OTHER		= 7U,
	XDBF_XACH_TYPE_MASK		= 7U,

	// Status
	XDBF_XACH_STATUS_UNACHIEVED	= (1U << 4),	// Set if *not* achieved.
	XDBF_XACH_STATUS_EARNED_ONLINE	= (1U << 16),
	XDBF_XACH_STATUS_EARNED		= (1U << 17),
	XDBF_XACH_STATUS_EDITED		= (1U << 20),	// ??
} XDBF_XACH_Flags_e;

/**
 * XDBF: XTHD - contains title information
 * All fields are in big-endian.
 */
#define XDBF_XTHD_MAGIC 'XTHD'
#define XDBF_XTHD_VERSION 1
typedef struct _XDBF_XTHD {
	uint32_t magic;		// [0x000] 'XTHD'
	uint32_t version;	// [0x004] Version (1)
	uint32_t size;		// [0x008] Size (might be 0?)
	XDBF_Title_ID title_id;	// [0x00C] Title ID
	uint32_t title_type;	// [0x010] Type (See XDBF_Title_Type_e)
	struct {
		uint16_t major;
		uint16_t minor;
		uint16_t build;
		uint16_t revision;
	} title_version;	// [0x014] Title version
	uint32_t unknown[4];	// [0x01C]
} XDBF_XTHD;
ASSERT_STRUCT(XDBF_XTHD, 0x2C);

/**
 * XDBF: Title type
 */
typedef enum {
	XDBF_TITLE_TYPE_SYSTEM		= 0,	// System title
	XDBF_TITLE_TYPE_FULL		= 1,	// Full retail game
	XDBF_TITLE_TYPE_DEMO		= 2,	// Demo
	XDBF_TITLE_TYPE_DOWNLOAD	= 3,	// Download game (XBLA, etc)
} XDBF_Title_Type_e;

/**
 * XDBF: XGAA - Avatar awards
 * All fields are in big-endian.
 */
#define XDBF_XGAA_MAGIC 'XGAA'
#define XDBF_XGAA_VERSION 1
#pragma pack(1)
typedef struct RP_PACKED _XDBF_XGAA_Header {
	uint32_t magic;		// [0x000] 'XGAA'
	uint32_t version;	// [0x004] Version (1)
	uint32_t size;		// [0x008] Size (must be at least 14)
	uint16_t xgaa_count;	// [0x00C] Number of avatar awards
	// Following XDBF_XGAA_Header are xgaa_count instances
	// of XDBF_XGAA_Entry.
} XDBF_XGAA_Header;
ASSERT_STRUCT(XDBF_XGAA_Header, 14);
#pragma pack()

/**
 * XDBF: XGAA - Avatar award entry
 * All fields are in big-endian.
 */
typedef struct _XDBF_XGAA_Entry {
	uint32_t unk_0x000;		// [0x000] ???
	uint16_t avatar_award_id;	// [0x004] Avatar award ID
	uint16_t unk_0x006;		// [0x006] ???
	uint8_t unk_0x008[4];		// [0x008] ???
	XDBF_Title_ID title_id;		// [0x00C] Title ID
	uint16_t name_id;		// [0x010] Name ID (string table)
	uint16_t unlocked_desc_id;	// [0x012] Unlocked description ID (string table)
	uint16_t locked_desc_id;	// [0x014] Locked description ID (string table)
	uint16_t unk_0x016;		// [0x016] ???
	uint32_t image_id;		// [0x018] Image ID
	uint8_t unk_0x01C[8];		// [0x01C] ???
} XDBF_XGAA_Entry;
ASSERT_STRUCT(XDBF_XGAA_Entry, 36);

/**
 * XDBF: XSRC - xlast XML data
 *
 * Contains a gzipped UTF-16LE translation file, which can be
 * used to get things like developer, publisher, genre, and
 * description.
 *
 * All fields are in big-endian.
 */
#define XDBF_XSRC_MAGIC 'XSRC'
#define XDBF_XSRC_VERSION 1
typedef struct _XDBF_XSRC_Header {
	uint32_t magic;		// [0x000] 'XSRC'
	uint32_t version;	// [0x004] Version (1)
	uint32_t size;		// [0x008] Size of entire struct, including gzipped data.
	uint32_t filename_len;	// [0x00C] Length of the original filename.

	// Following this header is the original filename,
	// then XDBF_XSRC_Header2.
} XDBF_XSRC_Header;
ASSERT_STRUCT(XDBF_XSRC_Header, 4*sizeof(uint32_t));

/**
 * XDBF: XSRC - second header, stored after the filename.
 * All fields are in big-endian.
 */
typedef struct _XDBF_XSRC_Header2 {
	uint32_t uncompressed_size;	// [0x000] Uncompressed data size
	uint32_t compressed_size;	// [0x004] Compressed data size
} XDBF_XSRC_Header2;
ASSERT_STRUCT(XDBF_XSRC_Header2, 2*sizeof(uint32_t));

#ifdef __cplusplus
}
#endif
