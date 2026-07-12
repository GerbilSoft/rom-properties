/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * exe_ne_structs.h: DOS/Windows executable structures. (LE)               *
 *                                                                         *
 * Copyright (c) 2017-2026 by David Korth.                                 *
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

/**
 * Linear Executable (LE/LX) header.
 * Endianness is determined by the byte_order and word_order fields.
 */
typedef struct _LE_Header {
	uint16_t sig;				// [0x000] 'LE' (0x4C45), or 'LX' (0x4C58) for OS/2 Warp
	uint8_t byte_order;			// [0x002] 0 == little-endian; other == big-endian
	uint8_t word_order;			// [0x003] 0 == little-endian; other == big-endian
	uint32_t format_level;			// [0x004] Executable format level
	uint16_t cpu_type;			// [0x008] See LE_CPU_Type
	uint16_t targOS;			// [0x00A] See NE_Target_OS
	uint32_t module_version;		// [0x00C]
	uint32_t module_type_flags;		// [0x010] See LE_Module_Type_Flags
	uint32_t module_page_count;		// [0x014] Number of memory pages
	uint32_t initial_cs_number;		// [0x018] LE: Initial object CS number; LX: Initial EIP object number
	uint32_t initial_eip;			// [0x01C] Initial EIP
	uint32_t initial_ss_number;		// [0x020] LE: Initial object SS number; LX: Initial ESP object number
	uint32_t initial_esp;			// [0x024] Initial ESP
	uint32_t page_size;			// [0x028]
	union {
		uint32_t bytes_on_last_page;	// [0x02C] LE
		uint32_t page_offset_shift;	// [0x02C] LX
	};
	uint32_t fixup_section_size;		// [0x030]
	uint32_t fixup_section_checksum;	// [0x034]
	uint32_t loader_section_size;		// [0x038]
	uint32_t loader_section_checksum;	// [0x03C]
	uint32_t object_table_offset;		// [0x040]
	uint32_t object_table_count;		// [0x044] Number of entries in the object table
	uint32_t object_page_map_offset;	// [0x048]
	uint32_t object_iterate_data_map_offset;// [0x04C]
	uint32_t resource_table_offset;		// [0x050] Resource table offset (similar to NE format?)
	uint32_t resource_table_count;		// [0x054] Number of entries in the resource table
	uint32_t resident_names_table_offset;	// [0x058]
	uint32_t entry_table_offset;		// [0x05C]
	uint32_t module_directives_offset;	// [0x060]
	uint32_t module_directives_count;	// [0x064]
	uint32_t fixup_page_table_offset;	// [0x068]
	uint32_t fixup_record_table_offset;	// [0x06C]
	uint32_t import_module_table_offset;	// [0x070]
	uint32_t import_module_table_count;	// [0x074]
	uint32_t import_proc_table_offset;	// [0x078]
	uint32_t per_page_checksum_offset;	// [0x07C]
	uint32_t data_page_offset;		// [0x080]
	uint32_t preload_pages_count;		// [0x084]
	uint32_t nonres_name_table_offset;	// [0x088]
	uint32_t nonres_name_table_length;	// [0x08C]
	uint32_t nonres_name_table_checksum;	// [0x090]
	uint32_t auto_da_object_number;		// [0x094]
	uint32_t debug_info_offset;		// [0x098]
	uint32_t debug_info_length;		// [0x09C]
	uint32_t instance_preload_count;	// [0x0A0] # of instance data pages in the preload section
	uint32_t instance_demand_count;		// [0x0A4] # of instance data pages in the demand section
	uint32_t heap_size;			// [0x0A8]
	uint32_t stack_size;			// [0x0AC]
} LE_Header;
ASSERT_STRUCT(LE_Header, 0xB0);

/**
 * CPU type
 */
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

/**
 * Module type flags
 */
typedef enum ATTR_FLAG_ENUM {
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

/**
 * Object table entry
 *
 * Endianness is determined by the byte_order and word_order fields
 * in the main LE/LX executable header.
 */
typedef struct _LE_Object_Table_Entry {
	uint32_t virtual_segment_size;		// [0x000]
	uint32_t relocation_base_address;	// [0x004]
	uint32_t object_flags;			// [0x008] See LE_Object_Flags_e
	uint32_t page_table_index;		// [0x00C]
	uint32_t page_table_count;		// [0x010]
	uint32_t reserved;			// [0x014]
} LE_Object_Table_Entry;
ASSERT_STRUCT(LE_Object_Table_Entry, 0x18);

/**
 * Object flags
 */
typedef enum ATTR_FLAG_ENUM {
	LE_OBJECT_FLAG_READABLE			= (1U <<  0),
	LE_OBJECT_FLAG_WRITABLE			= (1U <<  1),
	LE_OBJECT_FLAG_EXECUTABLE		= (1U <<  2),
	LE_OBJECT_FLAG_RESOURCE			= (1U <<  3),
	LE_OBJECT_FLAG_DISCARDABLE		= (1U <<  4),
	LE_OBJECT_FLAG_SHARED			= (1U <<  5),
	LE_OBJECT_FLAG_HAS_PRELOAD_PAGES	= (1U <<  6),
	LE_OBJECT_FLAG_HAS_INVALID_PAGES	= (1U <<  7),
	LE_OBJECT_FLAG_IS_RESIDENT		= (1U <<  8),	// valid for VDDs and PDDs only
	LE_OBJECT_FLAG_RESERVED_0200h		= (1U <<  9),
	LE_OBJECT_FLAG_RESIDENT_CONTIGUOUS	= 0x0300U,	// valid for VDDs and PDDs only
	LE_OBJECT_FLAG_RESIDENT_LONG_LOCKABLE	= (1U << 10),	// valid for VDDs and PDDs only
	LE_OBJECT_FLAG_IBM_MICROCHANNEL_EXT	= (1U << 11),
	LE_OBJECT_FLAG_16_16_ALIAS_REQUIRED	= (1U << 12),	// 80x86 specific
	LE_OBJECT_FLAG_BIG_DEFAULT_SETTING	= (1U << 13),	// 80x86 specific
	LE_OBJECT_FLAG_CONFORMING_FOR_CODE	= (1U << 14),	// 80x86 specific
	LE_OBJECT_FLAG_IO_PRIVILEGE_LEVEL	= (1U << 15),	// 80x86 specific; only used for 16:16 Alias Objects
} LE_Object_Flags_e;

/**
 * Object page table entry
 * TODO: Verify the layout.
 *
 * Endianness is determined by the byte_order and word_order fields
 * in the main LE/LX executable header.
 */
typedef struct _LE_Object_Page_Table_Entry {
	uint32_t page_data_offset;	// [0x000]
	uint16_t data_size;		// [0x004] Shift left by page_offset_shift
	uint16_t flags;			// [0x006] See LE_Object_Page_Table_Flags_e
} LE_Object_Page_Table_Entry;
ASSERT_STRUCT(LE_Object_Page_Table_Entry, 8);

/**
 * Object page table flags
 */
typedef enum {
	LE_OBJECT_PAGE_TABLE_FLAG_LEGAL_PHYSICAL_PAGE	= 0,	// offset from Preload Pages section
	LE_OBJECT_PAGE_TABLE_FLAG_ITERATED_DATA_PAGE	= 1,	// offset from Iterated Data Pages section
	LE_OBJECT_PAGE_TABLE_FLAG_INVALID_PAGE		= 2,
	LE_OBJECT_PAGE_TABLE_FLAG_ZERO_FILLED_PAGE	= 3,
	LE_OBJECT_PAGE_TABLE_FLAG_UNUSED		= 4,
	LE_OBJECT_PAGE_TABLE_FLAG_COMPRESSED_PAGE	= 5,	// offset from Preload Pages section
} LE_Object_Page_Table_Flags_e;

#ifdef __cplusplus
}
#endif
