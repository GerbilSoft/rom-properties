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
 * Representation of a value in a resource, supplying type
 * information.
 */
struct Res_value
{
	// Number of bytes in this structure.
	uint16_t size;

	// Always set to 0.
	uint8_t res0;

	// Type of the data value.
	enum : uint8_t {
		// The 'data' is either 0 or 1, specifying this resource is either
		// undefined or empty, respectively.
		TYPE_NULL = 0x00,
		// The 'data' holds a ResTable_ref, a reference to another resource
		// table entry.
		TYPE_REFERENCE = 0x01,
		// The 'data' holds an attribute resource identifier.
		TYPE_ATTRIBUTE = 0x02,
		// The 'data' holds an index into the containing resource table's
		// global value string pool.
		TYPE_STRING = 0x03,
		// The 'data' holds a single-precision floating point number.
		TYPE_FLOAT = 0x04,
		// The 'data' holds a complex number encoding a dimension value,
		// such as "100in".
		TYPE_DIMENSION = 0x05,
		// The 'data' holds a complex number encoding a fraction of a
		// container.
		TYPE_FRACTION = 0x06,
		// The 'data' holds a dynamic ResTable_ref, which needs to be
		// resolved before it can be used like a TYPE_REFERENCE.
		TYPE_DYNAMIC_REFERENCE = 0x07,
		// The 'data' holds an attribute resource identifier, which needs to be resolved
		// before it can be used like a TYPE_ATTRIBUTE.
		TYPE_DYNAMIC_ATTRIBUTE = 0x08,

		// Beginning of integer flavors...
		TYPE_FIRST_INT = 0x10,

		// The 'data' is a raw integer value of the form n..n.
		TYPE_INT_DEC = 0x10,
		// The 'data' is a raw integer value of the form 0xn..n.
		TYPE_INT_HEX = 0x11,
		// The 'data' is either 0 or 1, for input "false" or "true" respectively.
		TYPE_INT_BOOLEAN = 0x12,

		// Beginning of color integer flavors...
		TYPE_FIRST_COLOR_INT = 0x1c,

		// The 'data' is a raw integer value of the form #aarrggbb.
		TYPE_INT_COLOR_ARGB8 = 0x1c,
		// The 'data' is a raw integer value of the form #rrggbb.
		TYPE_INT_COLOR_RGB8 = 0x1d,
		// The 'data' is a raw integer value of the form #argb.
		TYPE_INT_COLOR_ARGB4 = 0x1e,
		// The 'data' is a raw integer value of the form #rgb.
		TYPE_INT_COLOR_RGB4 = 0x1f,

		// ...end of integer flavors.
		TYPE_LAST_COLOR_INT = 0x1f,

		// ...end of integer flavors.
		TYPE_LAST_INT = 0x1f
	};
	uint8_t dataType;

	// Structure of complex data values (TYPE_UNIT and TYPE_FRACTION)
	enum {
		// Where the unit type information is.  This gives us 16 possible
		// types, as defined below.
		COMPLEX_UNIT_SHIFT = 0,
		COMPLEX_UNIT_MASK = 0xf,

		// TYPE_DIMENSION: Value is raw pixels.
		COMPLEX_UNIT_PX = 0,
		// TYPE_DIMENSION: Value is Device Independent Pixels.
		COMPLEX_UNIT_DIP = 1,
		// TYPE_DIMENSION: Value is a Scaled device independent Pixels.
		COMPLEX_UNIT_SP = 2,
		// TYPE_DIMENSION: Value is in points.
		COMPLEX_UNIT_PT = 3,
		// TYPE_DIMENSION: Value is in inches.
		COMPLEX_UNIT_IN = 4,
		// TYPE_DIMENSION: Value is in millimeters.
		COMPLEX_UNIT_MM = 5,

		// TYPE_FRACTION: A basic fraction of the overall size.
		COMPLEX_UNIT_FRACTION = 0,
		// TYPE_FRACTION: A fraction of the parent size.
		COMPLEX_UNIT_FRACTION_PARENT = 1,

		// Where the radix information is, telling where the decimal place
		// appears in the mantissa.  This give us 4 possible fixed point
		// representations as defined below.
		COMPLEX_RADIX_SHIFT = 4,
		COMPLEX_RADIX_MASK = 0x3,

		// The mantissa is an integral number -- i.e., 0xnnnnnn.0
		COMPLEX_RADIX_23p0 = 0,
		// The mantissa magnitude is 16 bits -- i.e, 0xnnnn.nn
		COMPLEX_RADIX_16p7 = 1,
		// The mantissa magnitude is 8 bits -- i.e, 0xnn.nnnn
		COMPLEX_RADIX_8p15 = 2,
		// The mantissa magnitude is 0 bits -- i.e, 0x0.nnnnnn
		COMPLEX_RADIX_0p23 = 3,

		// Where the actual value is.  This gives us 23 bits of
		// precision.  The top bit is the sign.
		COMPLEX_MANTISSA_SHIFT = 8,
		COMPLEX_MANTISSA_MASK = 0xffffff
	};

	// Possible data values for TYPE_NULL.
	enum {
		// The value is not defined.
		DATA_NULL_UNDEFINED = 0,
		// The value is explicitly defined as empty.
		DATA_NULL_EMPTY = 1
	};

	// The data for this item, as interpreted according to dataType.
	typedef uint32_t data_type;
	data_type data;

	//void copyFrom_dtoh(const Res_value& src);
};

/**
 *  This is a reference to a unique entry (a ResTable_entry structure)
 *  in a resource table.  The value is structured as: 0xpptteeee,
 *  where pp is the package index, tt is the type index in that
 *  package, and eeee is the entry index in that type.  The package
 *  and type values start at 1 for the first item, to help catch cases
 *  where they have not been supplied.
 */
struct ResTable_ref
{
	uint32_t ident;
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
 * A collection of resource data types within a package.  Followed by
 * one or more ResTable_type and ResTable_typeSpec structures containing the
 * entry values for each resource type.
 */
struct ResTable_package
{
	struct ResChunk_header header;

	// If this is a base package, its ID.  Package IDs start
	// at 1 (corresponding to the value of the package bits in a
	// resource identifier).  0 means this is not a base package.
	uint32_t id;

	// Actual name of this package, \0-terminated.
	uint16_t name[128];

	// Offset to a ResStringPool_header defining the resource
	// type symbol table.  If zero, this package is inheriting from
	// another base package (overriding specific values in it).
	uint32_t typeStrings;

	// Last index into typeStrings that is for public use by others.
	uint32_t lastPublicType;

	// Offset to a ResStringPool_header defining the resource
	// key symbol table.  If zero, this package is inheriting from
	// another base package (overriding specific values in it).
	uint32_t keyStrings;

	// Last index into keyStrings that is for public use by others.
	uint32_t lastPublicKey;

	uint32_t typeIdOffset;
};

/**
 * A specification of the resources defined by a particular type.
 *
 * There should be one of these chunks for each resource type.
 *
 * This structure is followed by an array of integers providing the set of
 * configuration change flags (ResTable_config::CONFIG_*) that have multiple
 * resources for that configuration.  In addition, the high bit is set if that
 * resource has been made public.
 */
struct ResTable_typeSpec
{
	struct ResChunk_header header;

	// The type identifier this chunk is holding.  Type IDs start
	// at 1 (corresponding to the value of the type bits in a
	// resource identifier).  0 is invalid.
	uint8_t id;

	// Must be 0.
	uint8_t res0;
	// Must be 0.
	uint16_t res1;

	// Number of uint32_t entry configuration masks that follow.
	uint32_t entryCount;

	enum : uint32_t {
		// Additional flag indicating an entry is public.
		SPEC_PUBLIC = 0x40000000u,

		// Additional flag indicating the resource id for this resource may change in a future
		// build. If this flag is set, the SPEC_PUBLIC flag is also set since the resource must be
		// public to be exposed as an API to other applications.
		SPEC_STAGED_API = 0x20000000u,
	};
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
	//ResTable_config config;
	uint32_t config_size;
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
