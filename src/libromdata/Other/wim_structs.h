/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * wim_structs.h: Microsoft WIM structs                                    *
 *                                                                         *
 * Copyright (c) 2023 by ecumber.                                          *
 * Copyright (c) 2019-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

// References:
// 7-Zip Source Code (CPP/7zip/Archive/Wim/WimIn.h)
// https://github.com/libyal/assorted/blob/main/documentation/Windows%20Imaging%20(WIM)%20file%20format.asciidoc

// Version struct, read like MAJOR.MINOR.

#define FLAG_HEADER_RESERVED            0x00000001
#define FLAG_HEADER_COMPRESSION         0x00000002
#define FLAG_HEADER_READONLY            0x00000004
#define FLAG_HEADER_SPANNED             0x00000008
#define FLAG_HEADER_RESOURCE_ONLY       0x00000010
#define FLAG_HEADER_METADATA_ONLY       0x00000020
#define FLAG_HEADER_WRITE_IN_PROGRESS   0x00000040
#define FLAG_HEADER_RP_FIX              0x00000080 // reparse point fixup
#define FLAG_HEADER_COMPRESS_RESERVED   0x00010000
#define FLAG_HEADER_COMPRESS_XPRESS     0x00020000
#define FLAG_HEADER_COMPRESS_LZX        0x00040000


typedef struct {
	char unknown;
	uint8_t minor_version;
	uint8_t major_version;
	char unknown2;
} WIM_Version;
ASSERT_STRUCT(WIM_Version, 0x4);

typedef enum {
	Wim_Unknown = -1,

	Wim113_014 = 0,
	Wim109_112 = 1,
	Wim107_108 = 2,

	Wim_Max
} WIM_Version_Type;

typedef enum {
	header_reserved		= (1U << 0),
	has_compression		= (1U << 1),
	read_only		= (1U << 2),
	spanned			= (1U << 3),
	resource_only		= (1U << 4),
	metadata_only		= (1U << 5),
	write_in_progress	= (1U << 6),
	rp_fix			= (1U << 7),
} WIM_Flags;

typedef enum {
	compress_reserved	= (1U << 16),
	compress_xpress		= (1U << 17),
	compress_lzx		= (1U << 18),
	compress_lzms		= (1U << 19),
	compress_xpress2	= (1U << 21),
} WIM_Compression_Flags;

typedef enum {
	Wim_Arch_x86 = 0,
	Wim_Arch_ARM32 = 5,
	Wim_Arch_IA64 = 6,
	Wim_Arch_AMD64 = 9,
	Wim_Arch_ARM64 = 12,
} WimWindowsArchitecture;

typedef struct _WIM_File_Resource {
	//this is 7 bytes but there isn't a good way of
	//representing that 
	uint64_t size;
	uint64_t offset_of_xml;
	uint64_t not_important;
} WIM_File_Resource;
ASSERT_STRUCT(WIM_File_Resource, 0x18);

#define MSWIM_MAGIC "MSWIM\0\0\0"
#define WLPWM_MAGIC "WLPWM\0\0\0"
#define MSWIMOLD_MAGIC "\x7E\0\0\0"
typedef struct _WIM_Header {
	char magic[8];					// [0x000] "MSWIM\0\0\0" (also has version info in some cases)
	uint32_t header_size;				// [0x008]
	WIM_Version version;				// [0x00C]
	uint32_t flags;					// [0x010]
	uint32_t chunk_size;				// [0x014]
	char guid[0x10];				// [0x018]
	uint16_t part_number;				// [0x028]
	uint16_t total_parts;				// [0x02A]
	uint32_t number_of_images;			// [0x02C]
	WIM_File_Resource offset_table;			// [0x030]
	WIM_File_Resource xml_resource;			// [0x048]
	WIM_File_Resource boot_metadata_resource;	// [0x060]
	uint32_t bootable_index;			// [0x078]
	uint32_t unused1;				// [0x07C]
	WIM_File_Resource integrity_resource;		// [0x080]
	char unused2[0x38];				// [0x098]
} WIM_Header;
ASSERT_STRUCT(WIM_Header, 0xD0);

#ifdef __cplusplus
}
#endif
