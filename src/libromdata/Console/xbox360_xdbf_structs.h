/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * xbox360_xdbf_structs.h: Microsoft Xbox 360 game resource structures.    *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
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

// Title ID.
// This ID contains the game title in each language-specific string table.
// (Namespace XDBF_NAMESPACE_STRING, ID from XDBF_Language_e)
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
	// NOTE: These namespace IDs are valid for XEX resources only!
	XDBF_NAMESPACE_METADATA			= 1,	// Metadata
	XDBF_NAMESPACE_IMAGE			= 2,	// Image (usually PNG format)
	XDBF_NAMESPACE_STRING_TABLE		= 3,	// String table (ID == XDBF_Language_e)
} XDBF_Namespace_e;

/**
 * XSTC: Default language block.
 * Namespace ID: XDBF_NAMESPACE_METADATA
 * ID: XDBF_XSTC_MAGIC
 * All fields are in big-endian.
 */
#define XDBF_XSTC_MAGIC 'XSTC'
#define XDBF_XSTC_VERSION 0x00000001
typedef struct PACKED _XDBF_Default_Language {
	uint32_t magic;			// [0x000] 'XSTC'
	uint32_t version;		// [0x004] Version (0x00000001)
	uint32_t size;			// [0x008] sizeof(XDBF_Default_Language) - sizeof(uint32_t)
	uint32_t default_language;	// [0x00C] See XDBF_Language_e
} XDBF_Default_Language;
ASSERT_STRUCT(XDBF_Default_Language, 4*sizeof(uint32_t));

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
#define XDBF_XSTR_VERSION 0x00000001
typedef struct PACKED _XDBF_String_Table_Header {
	uint32_t magic;		// [0x000] 'XSTR'
	uint32_t version;	// [0x004] Version (0x00000001)
	uint32_t size;		// [0x008] Size
	uint16_t string_count;	// [0x00C] String count
} XDBF_String_Table_Header;
ASSERT_STRUCT(XDBF_String_Table_Header, 14);

/**
 * XDBF: String table entry header
 * All fields are in big-endian.
 */
typedef struct PACKED _XDBF_String_Table_Entry_Header {
	uint16_t string_id;	// [0x000] ID
	uint16_t length;	// [0x002] String length (NOT NULL-terminated)
} XDBF_String_Table_Entry_Header;
ASSERT_STRUCT(XDBF_String_Table_Entry_Header, sizeof(uint32_t));

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_CONSOLE_XBOX360_XDBF_STRUCTS_H__ */
