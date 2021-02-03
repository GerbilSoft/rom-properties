/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * n64_structs.h: Nintendo 64 data structures.                             *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_N64_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_N64_STRUCTS_H__

#include "common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

/**
 * Nintendo 64 ROM header.
 * This matches the ROM header format exactly.
 * Reference: http://www.romhacking.net/forum/index.php/topic,20415.msg286889.html?PHPSESSID=8bc8li2rrckkt4arqv7kdmufu1#msg286889
 * 
 * All fields are in big-endian.
 * NOTE: Strings are NOT null-terminated!
 */
#define N64_Z64_MAGIC   0x803712400000000FULL
#define N64_V64_MAGIC   0x3780401200000F00ULL
#define N64_SWAP2_MAGIC 0x12408037000F0000ULL
#define N64_LE32_MAGIC  0x401237800F000000ULL
typedef union _N64_RomHeader {
	struct {
		union {
			// [0x000]
			// NOTE: Technically, the first two DWORDs
			// are initialization settings, but in practice,
			// they're usually identical for all N64 ROMs.
			uint8_t magic[8];
			struct {
				uint32_t init_pi;
				uint32_t clockrate;
			};
			uint64_t magic64;
		};

		uint32_t entrypoint;	// [0x008]
		uint8_t os_version[4];	// [0x00C] OS version. (Previously called "release")
					//         Format: 00 00 AA BB
					//         AA is decimal; BB is ASCII.
					//         OoT is 00 00 14 49 == OS 20I
		uint32_t crc[2];	// [0x010] Two CRCs.
		uint8_t reserved1[8];	// [0x018]
		char title[0x14];	// [0x020] Title. (cp932)
		uint8_t reserved[7];	// [0x034]
		char id4[4];		// [0x03B] Game ID.
		uint8_t revision;	// [0x03F] Revision.
	};

	// Direct access for byteswapping.
	uint8_t u8[64];
	uint16_t u16[64/2];
	uint32_t u32[64/4];
} N64_RomHeader;
ASSERT_STRUCT(N64_RomHeader, 64);

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_N64_STRUCTS_H__ */
