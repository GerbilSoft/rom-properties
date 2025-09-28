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
// Reference: https://github.com/iBotPeaches/platform_frameworks_base/blob/main/libs/androidfw/include/androidfw/ResourceTypes.h

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

#ifdef __cplusplus
}
#endif
