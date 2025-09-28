/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * android_apk_structs.h: Android APK data structures.                     *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: Apache-2.0                                     *
 ***************************************************************************/

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

// from ResourceTypes.h
// References:
// - https://github.com/iBotPeaches/platform_frameworks_base/blob/main/libs/androidfw/include/androidfw/ResourceTypes.h
// - https://apktool.org/wiki/advanced/resources-arsc/

/**
 * Header that appears at the front of every data chunk in a resource.
 */
struct ResChunk_header
{
	// Type identifier for this chunk.  The meaning of this value depends
	// on the containing chunk.
	uint16_t type;

	// Size of the chunk header (in bytes).  Adding this value to
	// the address of the chunk allows you to find its associated data
	// (if any).
	uint16_t headerSize;

	// Total size of this chunk (in bytes).  This is the chunkSize plus
	// the size of any data associated with the chunk.  Adding this value
	// to the chunk allows you to completely skip its contents (including
	// any child chunks).  If this value is the same as chunkSize, there is
	// no data associated with the chunk.
	uint32_t size;
};

enum {
	RES_NULL_TYPE                     = 0x0000,
	RES_STRING_POOL_TYPE              = 0x0001,
	RES_TABLE_TYPE                    = 0x0002,
	RES_XML_TYPE                      = 0x0003,

	// Chunk types in RES_XML_TYPE
	RES_XML_FIRST_CHUNK_TYPE          = 0x0100,
	RES_XML_START_NAMESPACE_TYPE      = 0x0100,
	RES_XML_END_NAMESPACE_TYPE        = 0x0101,
	RES_XML_START_ELEMENT_TYPE        = 0x0102,
	RES_XML_END_ELEMENT_TYPE          = 0x0103,
	RES_XML_CDATA_TYPE                = 0x0104,
	RES_XML_LAST_CHUNK_TYPE           = 0x017f,
	// This contains a uint32_t array mapping strings in the string
	// pool back to resource identifiers.  It is optional.
	RES_XML_RESOURCE_MAP_TYPE         = 0x0180,

	// Chunk types in RES_TABLE_TYPE
	RES_TABLE_PACKAGE_TYPE            = 0x0200,
	RES_TABLE_TYPE_TYPE               = 0x0201,
	RES_TABLE_TYPE_SPEC_TYPE          = 0x0202,
	RES_TABLE_LIBRARY_TYPE            = 0x0203,
	RES_TABLE_OVERLAYABLE_TYPE        = 0x0204,
	RES_TABLE_OVERLAYABLE_POLICY_TYPE = 0x0205,
	RES_TABLE_STAGED_ALIAS_TYPE       = 0x0206,
};

/**
 * Macros for building/splitting resource identifiers.
 */
#define Res_VALIDID(resid) (resid != 0)
#define Res_CHECKID(resid) ((resid&0xFFFF0000) != 0)
#define Res_MAKEID(package, type, entry) \
    (((package+1)<<24) | (((type+1)&0xFF)<<16) | (entry&0xFFFF))
#define Res_GETPACKAGE(id) ((id>>24)-1)
#define Res_GETTYPE(id) (((id>>16)&0xFF)-1)
#define Res_GETENTRY(id) (id&0xFFFF)

#define Res_INTERNALID(resid) ((resid&0xFFFF0000) != 0 && (resid&0xFF0000) == 0)
#define Res_MAKEINTERNAL(entry) (0x01000000 | (entry&0xFFFF))
#define Res_MAKEARRAY(entry) (0x02000000 | (entry&0xFFFF))

static const size_t Res_MAXPACKAGE = 255;
static const size_t Res_MAXTYPE = 255;

/**
 * Header for a resource table.  Its data contains a series of
 * additional chunks:
 *   * A ResStringPool_header containing all table values.  This string pool
 *     contains all of the string values in the entire resource table (not
 *     the names of entries or type identifiers however).
 *   * One or more ResTable_package chunks.
 *
 * Specific entries within a resource table can be uniquely identified
 * with a single integer as defined by the ResTable_ref structure.
 */
struct ResTable_header
{
	struct ResChunk_header header;

	// The number of ResTable_package structures.
	uint32_t packageCount;
};

/**
 * Reference to a string in a string pool.
 */
struct ResStringPool_ref
{
	// Index into the string pool table (uint32_t-offset from the indices
	// immediately after ResStringPool_header) at which to find the location
	// of the string data in the pool.
	uint32_t index;
};

/**
 * Definition for a pool of strings.  The data of this chunk is an
 * array of uint32_t providing indices into the pool, relative to
 * stringsStart.  At stringsStart are all of the UTF-16 strings
 * concatenated together; each starts with a uint16_t of the string's
 * length and each ends with a 0x0000 terminator.  If a string is >
 * 32767 characters, the high bit of the length is set meaning to take
 * those 15 bits as a high word and it will be followed by another
 * uint16_t containing the low word.
 *
 * If styleCount is not zero, then immediately following the array of
 * uint32_t indices into the string table is another array of indices
 * into a style table starting at stylesStart.  Each entry in the
 * style table is an array of ResStringPool_span structures.
 */
struct ResStringPool_header
{
	struct ResChunk_header header;

	// Number of strings in this pool (number of uint32_t indices that follow
	// in the data).
	uint32_t stringCount;

	// Number of style span arrays in the pool (number of uint32_t indices
	// follow the string indices).
	uint32_t styleCount;

	// Flags.
	enum {
		// If set, the string index is sorted by the string values (based
		// on strcmp16()).
		SORTED_FLAG = 1<<0,

		// String pool is encoded in UTF-8
		UTF8_FLAG = 1<<8
	};
	uint32_t flags;

	// Index from header of the string data.
	uint32_t stringsStart;

	// Index from header of the style data.
	uint32_t stylesStart;
};

/**
 * A collection of resource entries for a particular resource data
 * type.
 *
 * If the flag FLAG_SPARSE is not set in `flags`, then this struct is
 * followed by an array of uint32_t defining the resource
 * values, corresponding to the array of type strings in the
 * ResTable_package::typeStrings string block. Each of these hold an
 * index from entriesStart; a value of NO_ENTRY means that entry is
 * not defined.
 *
 * If the flag FLAG_SPARSE is set in `flags`, then this struct is followed
 * by an array of ResTable_sparseTypeEntry defining only the entries that
 * have values for this type. Each entry is sorted by their entry ID such
 * that a binary search can be performed over the entries. The ID and offset
 * are encoded in a uint32_t. See ResTabe_sparseTypeEntry.
 *
 * There may be multiple of these chunks for a particular resource type,
 * supply different configuration variations for the resource values of
 * that type.
 *
 * It would be nice to have an additional ordered index of entries, so
 * we can do a binary search if trying to find a resource by string name.
 */
struct ResTable_type
{
	struct ResChunk_header header;

	enum {
		NO_ENTRY = 0xFFFFFFFF
	};

	// The type identifier this chunk is holding.  Type IDs start
	// at 1 (corresponding to the value of the type bits in a
	// resource identifier).  0 is invalid.
	uint8_t id;

	enum {
		// If set, the entry is sparse, and encodes both the entry ID and offset into each entry,
		// and a binary search is used to find the key. Only available on platforms >= O.
		// Mark any types that use this with a v26 qualifier to prevent runtime issues on older
		// platforms.
		FLAG_SPARSE = 0x01,
	};
	uint8_t flags;

	// Must be 0.
	uint16_t reserved;

	// Number of uint32_t entry indices that follow.
	uint32_t entryCount;

	// Offset from header where ResTable_entry data starts.
	uint32_t entriesStart;

	// Configuration this collection of entries is designed for. This must always be last.
	// NOTE: Disabled for now...
	//ResTable_config config;
	uint32_t config_size;
};

/**
 * An entry in a ResTable_type with the flag `FLAG_SPARSE` set.
 */
union ResTable_sparseTypeEntry {
	// Holds the raw uint32_t encoded value. Do not read this.
	uint32_t entry;
	struct {
		// The index of the entry.
		uint16_t idx;

		// The offset from ResTable_type::entriesStart, divided by 4.
		uint16_t offset;
	};
};

struct ResTable_entry
{
	// Number of bytes in this structure.
	uint16_t size;

	enum {
		// If set, this is a complex entry, holding a set of name/value
		// mappings.  It is followed by an array of ResTable_map structures.
		FLAG_COMPLEX = 0x0001,
		// If set, this resource has been declared public, so libraries
		// are allowed to reference it.
		FLAG_PUBLIC = 0x0002,
		// If set, this is a weak resource and may be overriden by strong
		// resources of the same name/type. This is only useful during
		// linking with other resource tables.
		FLAG_WEAK = 0x0004,
	};
	uint16_t flags;

	// Reference into ResTable_package::keyStrings identifying this entry.
	struct ResStringPool_ref key;
};

#ifdef __cplusplus
}
#endif
