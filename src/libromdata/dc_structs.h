/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * dc_structs.h: Sega Dreamcast data structures.                           *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_DC_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_DC_STRUCTS_H__

#include "common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Dreamcast VMS header. (.vms files)
 * Reference: http://mc.pp.se/dc/vms/fileheader.html
 *
 * All fields are in little-endian.
 *
 * NOTE: Strings are NOT null-terminated!
 */
#define DC_VMS_Header_SIZE 96
#pragma pack(1)
typedef struct PACKED _DC_VMS_Header {
	char vms_description[16];	// Shift-JIS; space-padded.
	char dc_description[32];	// Shift-JIS; space-padded.
	char application[16];		// Shift-JIS; NULL-padded.
	uint16_t icon_count;
	uint16_t icon_anim_speed;
	uint16_t eyecatch_type;
	uint16_t crc;
	uint32_t data_size;		// Ignored for game files.
	uint8_t reserved[20];

	// Icon palette and icon bitmaps are located
	// immediately after the VMS header, followed
	// by the eyecatch (if present).
} DC_VMS_Header;
#pragma pack()

/**
 * Graphic eyecatch type.
 */
enum DC_VMS_Eyecatch_Type {
	DC_VMS_EYECATCH_NONE		= 0,
	DC_VMS_EYECATCH_ARGB4444	= 1,
	DC_VMS_EYECATCH_CI8		= 2,
	DC_VMS_EYECATCH_CI4		= 3,
};

// Icon and eyecatch sizes.
#define DC_VMS_ICON_W 32
#define DC_VMS_ICON_H 32
#define DC_VMS_EYECATCH_W 72
#define DC_VMS_EYECATCH_H 56

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_DC_STRUCTS_H__ */
