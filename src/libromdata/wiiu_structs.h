/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * wiiu_structs.h: Nintendo Wii U data structures.                         *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_WIIU_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_WIIU_STRUCTS_H__

#include "common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Nintendo Wii U disc header.
 * Reference: https://github.com/maki-chan/wudecrypt/blob/master/main.c
 * 
 * All fields are big-endian.
 * NOTE: Strings are NOT null-terminated!
 */
#pragma pack(1)
typedef struct PACKED _WiiU_DiscHeader {
	union {
		char id[10];		// WUP-P-xxxx
		struct {
			char wup[3];	// WUP
			char hyphen1;	// -
			char p;		// P
			char hyphen2;	// -
			char id4[4];	// xxxx
		};
	};
	char hyphen3;
	char version[2];	// Version number, in ASCII. (e.g. "00")
	char hyphen4;
	char os_version[3];	// Required OS version, in ASCII. (e.g. "551")
	char region[3];		// Region code, in ASCII. ("USA", "EUR") (TODO: Is this the enforced region?)
	char hyphen5;
	char disc_number;	// Disc number, in ASCII. (TODO: Verify?)
} WiiU_DiscHeader;
#pragma pack()
ASSERT_STRUCT(WiiU_DiscHeader, 22);

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_WIIU_STRUCTS_H__ */
