/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ngpc_structs.h: Neo Geo Pocket (Color) data structures.                 *
 *                                                                         *
 * Copyright (c) 2019-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_HANDHELD_NGPC_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_HANDHELD_NGPC_STRUCTS_H__

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

/**
 * Neo geo Pocket (Color) ROM header.
 * This matches the ROM header format exactly.
 * Reference: http://devrs.com/ngp/files/DoNotLink/ngpcspec.txt
 *
 * All fields are in little-endian.
 * NOTE: Strings are NOT necessarily null-terminated!
 */
#define NGPC_COPYRIGHT_STR "COPYRIGHT BY SNK CORPORATION"
#define NGPC_LICENSED_STR  " LICENSED BY SNK CORPORATION"
typedef struct PACKED _NGPC_RomHeader {
	char copyright[28];		// [0x000] Copyright/Licensed by SNK Corporation
	uint32_t entry_point;		// [0x01C] Entry point. (If high byte == 0xFF, debug is enabled)
	uint8_t id_code[2];		// [0x020] Little-endian BCD software ID code.
	uint8_t version;		// [0x022] Version number.
	uint8_t machine_type;		// [0x023] Machine type. (See NGPC_MachineType_e.)
	char title[12];			// [0x024] Title, in ASCII.
	uint8_t reserved[16];		// [0x030] All zero.
} NGPC_RomHeader;
ASSERT_STRUCT(NGPC_RomHeader, 64);

/**
 * Machine type.
 */
typedef enum {
	NGPC_MACHINETYPE_MONOCHROME	= 0x00,
	NGPC_MACHINETYPE_COLOR		= 0x10,
} NGPC_MachineType_e;

/**
 * Debug mode. (high byte of the entry point)
 */
typedef enum {
	NGPC_DEBUG_MODE_OFF	= 0x00,
	NGPC_DEBUG_MODE_ON	= 0x10,
} NGPC_DebugMode_t;

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_HANDHELD_NGPC_STRUCTS_H__ */
