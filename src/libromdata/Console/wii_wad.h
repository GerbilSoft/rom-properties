/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * wii_wad.h: Nintendo Wii WAD file data structures.                       *
 *                                                                         *
 * Copyright (c) 2018 by David Korth.                                      *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_WII_WAD_H__
#define __ROMPROPERTIES_LIBROMDATA_WII_WAD_H__

// References:
// - https://wiibrew.org/wiki/WAD_files
// - https://wiibrew.org/wiki/Content.bin

#include "librpbase/common.h"
#include <stdint.h>

// Wii data structures.
#include "wii_structs.h"
#include "wii_banner.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

/**
 * Nintendo Wii WAD file header.
 * Reference: https://wiibrew.org/wiki/WAD_files
 * 
 * All fields are big-endian.
 */
#define WII_WAD_TYPE_Is	0x49730000	// 'Is\0\0'
#define WII_WAD_TYPE_ib	0x69620000	// 'ib\0\0'
#define WII_WAD_TYPE_Bk	0x426B0000	// 'Bk\0\0'
typedef struct PACKED _Wii_WAD_Header {
	uint32_t header_size;		// [0x000] Header size. (0x0020)
	uint32_t type;			// [0x004] Type. ('Is\0\0', 'ib\0\0', 'Bk\0\0')
	uint32_t cert_chain_size;	// [0x008] Certificate chain size.
	uint32_t reserved;		// [0x00C] Reserved. (0)
	uint32_t ticket_size;		// [0x010] Ticket size. (0x2A4)
	uint32_t tmd_size;		// [0x014] TMD size.
	uint32_t data_size;		// [0x018] Data size.
	uint32_t footer_size;		// [0x01C] Footer size.
} Wii_WAD_Header;
ASSERT_STRUCT(Wii_WAD_Header, 32);

/**
 * Nintendo Wii WAD file header.
 * EARLY VERSION; used with some early devkit software.
 *
 * To identify the early version, check for an invalid WAD type,
 * then the ticket size.
 *
 * NOTE: Sections are NOT 64-byte aligned in early WADs!
 *
 * All fields are big-endian.
 */
typedef struct PACKED _Wii_WAD_Header_EARLY {
	uint32_t header_size;		// [0x000] Header size. (0x0020)
	uint32_t data_offset;		// [0x004] Data offset. (usually 0x1140)
	uint32_t cert_chain_size;	// [0x008] Certificate chain size.
	uint32_t ticket_size;		// [0x00C] Ticket size. (0x2A4)
	uint32_t tmd_size;		// [0x010] TMD size.
	uint32_t name_size;		// [0x014] Name size.
	uint32_t unknown;		// [0x018] Unknown.
	uint32_t reserved;		// [0x01C] Reserved.
} Wii_WAD_Header_EARLY;
ASSERT_STRUCT(Wii_WAD_Header_EARLY, 32);

/**
 * content.bin header.
 *
 * This is the encrypted program data in a WAD file and/or
 * copied to an SD card.
 *
 * This is followed by one of the following:
 * - Wii_IMET_t: IMET header. (most WADs)
 * - Wii_WIBN_Header_t: WIBN header. (DLC WADs)
 *
 * Reference: https://wiibrew.org/wiki/Content.bin
 *
 * All fields are big-endian.
 */
typedef struct PACKED _Wii_Content_Bin_Header {
	RVL_TitleID_t title_id;		// [0x000] Title ID. (tid-hi is usually 0x00010001)
	uint32_t partB_size;		// [0x008] Size of part B.
	uint8_t md5_header[16];		// [0x00C] MD5 hash of the header.
	uint8_t md5_icon[16];		// [0x01C] MD5 hash of the DECRYPTED icon.
	uint8_t unknown_2[4];		// [0x02C]
	RVL_TitleID_t tid_ref[2];	// [0x030] Title dependencies?
} Wii_Content_Bin_Header;
ASSERT_STRUCT(Wii_Content_Bin_Header, 64);

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_WII_WAD_H__ */
