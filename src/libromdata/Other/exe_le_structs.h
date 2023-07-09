/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * exe_ne_structs.h: DOS/Windows executable structures. (LE)               *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Linear Executable structs. **/
// NOTE: The header format is the same for LE (Win16 drivers)
// and LX (32-bit OS/2 executables).
// References:
// - http://fileformats.archiveteam.org/wiki/Linear_Executable
// - http://faydoc.tripod.com/formats/exe-LE.htm
// - http://www.textfiles.com/programming/FORMATS/lxexe.txt

typedef struct _LE_Header {
	// 0x00
	uint16_t sig;		// 'LE' (0x4C45)
	uint8_t byte_order;	// 0 == little-endian; other == big-endian
	uint8_t word_order;	// 0 == little-endian; other == big-endian
	uint32_t format_level;	// Executable format level.
	uint16_t cpu_type;	// See LE_CPU_Type.
	uint16_t targOS;	// See NE_Target_OS.
	uint32_t module_version;
	// 0x10
	uint32_t module_type_flags;	// See LE_Module_Type_Flags.
	uint32_t module_page_count;	// Number of memory pages.
	uint32_t initial_cs_number;	// Initial object CS number.
	uint32_t initial_eip;		// Initial EIP.
	// 0x20
	uint32_t initial_ss_number;	// Initial object SS number.
	uint32_t initial_esp;		// Initial ESP.
	uint32_t page_size;
	uint32_t bytes_on_last_page;	// or page offset shift?
	// 0x30
	uint32_t fixup_section_size;
	uint32_t fixup_section_checksum;
	uint32_t loader_section_size;
	uint32_t loader_section_checksum;
	// 0x40
	uint32_t object_table_offset;
	uint32_t object_table_count;	// Number of entries in the object table.
	uint32_t object_page_map_offset;
	uint32_t object_iterate_data_map_offset;
	// 0x50
	uint32_t resource_table_offset;
	uint32_t resource_table_count;	// Number of entries in the resource table.
	uint32_t resident_names_table_offset;
	uint32_t entry_table_offset;
	// 0x60
	// TODO more
	uint8_t filler[0xA8-0x60];
} LE_Header;
ASSERT_STRUCT(LE_Header, 0xA8);

// CPU type.
typedef enum {
	// TODO add to 'file'
	LE_CPU_UNKNOWN		= 0x00,
	LE_CPU_80286		= 0x01,
	LE_CPU_80386		= 0x02,
	LE_CPU_80486		= 0x03,
	LE_CPU_80586		= 0x04,
	LE_CPU_i860_N10		= 0x20,	// i860 XR
	LE_CPU_i860_N11		= 0x21,	// i860 XP
	LE_CPU_MIPS_I		= 0x40,	// MIPS Mark I (R2000, R3000)
	LE_CPU_MIPS_II		= 0x41,	// MIPS Mark II (R6000)
	LE_CPU_MIPS_III		= 0x42,	// MIPS Mark III (R4000)
} LE_CPU_Type;

// Module type flags.
typedef enum {
	LE_DLL_INIT_GLOBAL		= (0U << 2),
	LE_DLL_INIT_PER_PROCESS		= (1U << 2),
	LE_DLL_INIT_MASK		= (1U << 2),

	LE_EXE_NO_INTERNAL_FIXUP	= (1U << 4),
	LE_EXE_NO_EXTERNAL_FIXUP	= (1U << 5),

	// Same as NE_AppType.
	LE_WINDOW_TYPE_UNKNOWN		= (0U << 8),
	LE_WINDOW_TYPE_INCOMPATIBLE	= (1U << 8),
	LE_WINDOW_TYPE_COMPATIBLE	= (2U << 8),
	LE_WINDOW_TYPE_USES		= (3U << 8),
	LE_WINDOW_TYPE_MASK		= (3U << 8),

	LE_MODULE_NOT_LOADABLE		= (1U << 13),
	LE_MODULE_IS_DLL		= (1U << 15),
} LE_Module_Type_Flags;

#ifdef __cplusplus
}
#endif
