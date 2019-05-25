/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * xbox360_xdbf_structs.h: Microsoft Xbox 360 game resource structures.    *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_CONSOLE_XBOX360_XDBF_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_CONSOLE_XBOX360_XDBF_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

// TODO: In addition to the main game info, handle achievements?

// NOTE: Entries begin after all headers:
// - XDBF_Header
// - XDBF_Entry * entry_table_length
// - XDBG_Free_Space_Entry * free_space_table_length

/**
 * Microsoft Xbox 360 XDBF header.
 * References:
 * - https://github.com/xenia-project/xenia/blob/HEAD/src/xenia/kernel/util/xdbf_utils.h
 * - https://github.com/xenia-project/xenia/blob/HEAD/src/xenia/kernel/util/xdbf_utils.cc
 * - https://free60project.github.io/wiki/XDBF.html
 * - https://free60project.github.io/wiki/GPD.html
 * - https://free60project.github.io/wiki/SPA.html
 *
 * All fields are in big-endian.
 */
#define XDBF_MAGIC 'XDBF'
#define XDBF_VERSION 0x10000
typedef struct PACKED _XDBF_Header {
	uint32_t magic;				// [0x000] 'XDBF'
	uint32_t version;			// [0x004] Version (0x10000)
	uint32_t entry_table_length;		// [0x008] Entry table length, in number of entries
	uint32_t entry_count;			// [0x00C] Entry count (# of used entries)
	uint32_t free_space_table_length;	// [0x010] Free space table length, in number of entries
	uint32_t free_space_table_count;	// [0x014] Free space table entry count (# of used entries)
} XDBF_Header;
ASSERT_STRUCT(XDBF_Header, 0x18);

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
typedef struct PACKED _XDBF_Entry {
	uint16_t namespace_id;		// [0x000] See XDBF_Namespace_e
	uint64_t resource_id;		// [0x002] ID
	uint32_t offset;		// [0x00A] Offset specifier
	uint32_t length;		// [0x00E] Length
} XDBF_Entry;

/**
 * XDBG free space table entry
 * All fields are in big-endian.
 */
typedef struct PACKED _XDBF_Free_Space_Entry {
	uint32_t offset;		// [0x000] Offset specifier
	uint32_t length;		// [0x004] Length
} XDBF_Free_Space_Entry;

/**
 * XDBF: Namespace IDs
 */
typedef enum {
	/** SPA (XEX XDBF) **/
	XDBF_SPA_NAMESPACE_METADATA		= 1,	// Metadata
	XDBF_SPA_NAMESPACE_IMAGE		= 2,	// Image (usually PNG format)
	XDBF_SPA_NAMESPACE_STRING_TABLE		= 3,	// String table (ID == XDBF_Language_e)
} XDBF_Namespace_e;

/**
 * XSTC: Default language block.
 * Namespace ID: XDBF_NAMESPACE_METADATA
 * ID: XDBF_XSTC_MAGIC
 * All fields are in big-endian.
 */
#define XDBF_XSTC_MAGIC 'XSTC'
#define XDBF_XSTC_VERSION 1
typedef struct PACKED _XDBF_Default_Language {
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
	XDBF_LANGUAGE_UNKNOWN	= 0,
	XDBF_LANGUAGE_ENGLISH	= 1,
	XDBF_LANGUAGE_JAPANESE	= 2,
	XDBF_LANGUAGE_GERMAN	= 3,
	XDBF_LANGUAGE_FRENCH	= 4,
	XDBF_LANGUAGE_SPANISH	= 5,
	XDBF_LANGUAGE_ITALIAN	= 6,
	XDBF_LANGUAGE_KOREAN	= 7,
	XDBF_LANGUAGE_CHINESE	= 8,

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
typedef struct PACKED _XDBF_XSTR_Header {
	uint32_t magic;		// [0x000] 'XSTR'
	uint32_t version;	// [0x004] Version (1)
	uint32_t size;		// [0x008] Size
	uint16_t string_count;	// [0x00C] String count
} XDBF_XSTR_Header;
ASSERT_STRUCT(XDBF_XSTR_Header, 14);

/**
 * XDBF: String table entry header
 * All fields are in big-endian.
 */
typedef struct PACKED _XDBF_XSTR_Entry_Header {
	uint16_t string_id;	// [0x000] ID
	uint16_t length;	// [0x002] String length (NOT NULL-terminated)
} XDBF_XSTR_Entry_Header;
ASSERT_STRUCT(XDBF_XSTR_Entry_Header, sizeof(uint32_t));

/**
 * XDBF: Title ID
 * Contains two characters and a 16-bit number.
 * NOTE: Struct positioning only works with the original BE32 value.
 * TODO: Combine with XEX2 version.
 */
typedef union PACKED _XDBF_Title_ID {
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
typedef struct PACKED _XDBF_XACH_Header {
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

/**
 * XDBF: XACH - Achievements table entry
 * All fields are in big-endian.
 */
typedef struct PACKED _XDBF_XACH_Entry {
	uint16_t achievement_id;	// [0x000] Achievement ID
	uint16_t name_id;		// [0x002] Name ID (string table)
	uint16_t unlocked_desc_id;	// [0x004] Unlocked description ID (string table)
	uint16_t locked_desc_id;	// [0x006] Locked description ID (string table)
	uint32_t image_id;		// [0x008] Image ID
	uint16_t gamerscore;		// [0x00C] Gamerscore
	uint16_t unknown1;		// [0x00E]
	uint32_t flags;			// [0x010] Flags (??)
	uint32_t unknown2[4];		// [0x014]
} XDBF_XACH_Entry;
ASSERT_STRUCT(XDBF_XACH_Entry, 0x24);

/**
 * XDBF: XTHD - contains title information
 * All fields are in big-endian.
 */
#define XDBF_XTHD_MAGIC 'XTHD'
#define XDBF_XTHD_VERSION 1
typedef struct PACKED _XDBF_XTHD {
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
	uint32_t unknown[4];	// [0x018]
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
typedef struct PACKED _XDBF_XGAA_Header {
	uint32_t magic;		// [0x000] 'XGAA'
	uint32_t version;	// [0x004] Version (1)
	uint32_t size;		// [0x008] Size (must be at least 14)
	uint16_t xgaa_count;	// [0x00C] Number of avatar awards
	// Following XDBF_XGAA_Header are xgaa_count instances
	// of XDBF_XGAA_Entry.
} XDBF_XGAA_Header;
ASSERT_STRUCT(XDBF_XGAA_Header, 14);

/**
 * XDBF: XGAA - Avatar award entry
 * All fields are in big-endian.
 */
typedef struct PACKED _XDBF_XGAA_Entry {
	uint32_t unk_0x000;		// [0x000] ???
	uint16_t avatar_award_id;	// [0x004] Avatar award ID
	uint16_t unk_0x006;		// [0x006] ???
	uint8_t unk_0x008[8];		// [0x008] ???
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
typedef struct PACKED _XDBF_XSRC_Header {
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
typedef struct PACKED _XDBF_XSRC_Header2 {
	uint32_t uncompressed_size;	// [0x000] Uncompressed data size
	uint32_t compressed_size;	// [0x004] Compressed data size
} XDBF_XSRC_Header2;
ASSERT_STRUCT(XDBF_XSRC_Header2, 2*sizeof(uint32_t));

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_CONSOLE_XBOX360_XDBF_STRUCTS_H__ */
