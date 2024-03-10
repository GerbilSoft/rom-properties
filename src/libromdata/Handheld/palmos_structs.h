/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * palmos_structs.h: Palm OS data structures.                              *
 *                                                                         *
 * Copyright (c) 2018-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - https://en.wikipedia.org/wiki/PRC_(Palm_OS)
// - https://web.mit.edu/pilot/pilot-docs/V1.0/cookbook.pdf
// - https://web.mit.edu/Tytso/www/pilot/prc-format.html
// - https://stuff.mit.edu/afs/sipb/user/yonah/docs/Palm%20OS%20Companion.pdf
// - https://stuff.mit.edu/afs/sipb/user/yonah/docs/Palm%20OS%20Reference.pdf
// - https://www.cs.trinity.edu/~jhowland/class.files.cs3194.html/palm-docs/Constructor%20for%20Palm%20OS.pdf
// - https://www.cs.uml.edu/~fredm/courses/91.308-spr05/files/palmdocs/uiguidelines.pdf

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Palm OS .prc (Palm Resource Code) header
 *
 * All fields are in big-endian, with 16-bit alignment.
 *
 * NOTE: PDB datetime is "number of seconds since 1904/01/01 00:00:00 UTC".
 */
#pragma pack(2)
typedef struct PACKED _PalmOS_PRC_Header_t {
	char name[32];			// [0x000] Internal name
	uint16_t flags;			// [0x020] Flags (see PalmOS_PRC_Flags_e)
	uint16_t version;		// [0x022] Header version
	uint32_t creation_time;		// [0x024] Creation time (PDB datetime)
	uint32_t modification_time;	// [0x028] Modification time (PDB datetime)
	uint32_t backup_time;		// [0x02C] Backup time (PDB datetime)
	uint32_t mod_num;		// [0x030]
	uint32_t app_info;		// [0x034]
	uint32_t sort_info;		// [0x038]
	uint32_t type;			// [0x03C] File type (see PalmOS_PRC_FileType_e)
	uint32_t creator_id;		// [0x040] Creator ID
	uint32_t unique_id_seed;	// [0x044]
	uint32_t next_record_list;	// [0x048]
	uint16_t num_records;		// [0x04C] Number of resource header records immediately following this header
} PalmOS_PRC_Header_t;
ASSERT_STRUCT(PalmOS_PRC_Header_t, 0x4E);
#pragma pack()

/**
 * Palm OS file types
 */
typedef enum {
	PalmOS_PRC_FileType_Application	= 'appl',
} PalmOS_PRC_FileType_e;

/**
 * Palm OS resource header record
 *
 * All fields are in big-endian, with 16-bit alignment.
 */
#pragma pack(2)
typedef struct PACKED _PalmOS_PRC_ResHeader_t {
	uint32_t type;		// [0x000] Resource type (see PalmOS_PRC_ResType_e)
	uint16_t id;		// [0x004] Resource ID
	uint32_t addr;		// [0x006] Address of the resource data (absolute)
} PalmOS_PRC_ResHeader_t;
ASSERT_STRUCT(PalmOS_PRC_ResHeader_t, 10);
#pragma pack()

/**
 * Palm OS resource types
 */
typedef enum {
	PalmOS_PRC_ResType_ApplicationIcon	= 'tAIB',
	PalmOS_PRC_ResType_ApplicationName	= 'tAIN',
	PalmOS_PRC_ResType_ApplicationVersion	= 'tver',
	PalmOS_PRC_ResType_ApplicationCategory	= 'taic',
} PalmOS_PRC_ResType_e;

#ifdef __cplusplus
}
#endif
