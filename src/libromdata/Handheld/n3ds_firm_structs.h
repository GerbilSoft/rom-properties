/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * n3ds_firm_structs.h: Nintendo 3DS firmware data structures.             *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

// References:
// - https://3dbrew.org/wiki/FIRM

#ifndef __ROMPROPERTIES_LIBROMDATA_N3DS_FIRM_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_N3DS_FIRM_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

/**
 * Nintendo 3DS firmware section header struct.
 * All fields are little-endian.
 */
typedef struct PACKED _N3DS_FIRM_Section_Header_t {
	uint32_t offset;		// [0x000] Byte offset.
	uint32_t load_addr;		// [0x004] Physical address where the section is loaded to.
	uint32_t size;			// [0x008] Byte size. (If 0, section does not exist.)
	uint32_t copy_method;		// [0x00C] 0 = NDMA, 1 = XDMA, 2 = CPU memcpy()
	uint8_t sha256[32];		// [0x010] SHA-256 of the previous fields.
} N3DS_FIRM_Section_Header_t;
ASSERT_STRUCT(_N3DS_FIRM_Section_Header_t, 48);

/**
 * Nintendo 3DS firmware binary header struct.
 * All fields are little-endian.
 */
#define N3DS_FIRM_MAGIC "FIRM"
typedef struct PACKED _N3DS_FIRM_Header_t {
	uint8_t magic[4];			// [0x000] "FIRM"
	uint32_t boot_priority;			// [0x004] Normally 0. (highest value = max prio)
	uint32_t arm11_entrypoint;		// [0x008] Non-zero for FIRM; zero for Boot9Strap payloads.
	uint32_t arm9_entrypoint;		// [0x00C]
	uint8_t reserved[0x30];			// [0x010]
	N3DS_FIRM_Section_Header_t sections[4];	// [0x040] Firmware section headers.
	uint8_t signature[0x100];		// [0x100] RSA-2048 signature.
} N3DS_FIRM_Header_t;
ASSERT_STRUCT(_N3DS_FIRM_Header_t, 512);

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_N3DS_FIRM_STRUCTS_H__ */
