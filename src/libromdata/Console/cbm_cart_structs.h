/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * cbm_cart_structs.h: Commodore ROM cartridge data structures.            *
 *                                                                         *
 * Copyright (c) 2016-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_CBM_CART_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_CBM_CART_STRUCTS_H__

#include "common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Commodore .CRT cartridge file header.
 * Reference: https://vice-emu.sourceforge.io/vice_17.html#SEC391
 *
 * All fields are in big-endian.
 */
#define CBM_C64_CRT_MAGIC	"C64 CARTRIDGE   "
#define CBM_C128_CRT_MAGIC	"C128 CARTRIDGE  "
#define CBM_CBM2_CRT_MAGIC	"CBM2 CARTRIDGE  "
#define CBM_VIC20_CRT_MAGIC	"VIC20 CARTRIDGE "
#define CBM_PLUS4_CRT_MAGIC	"PLUS4 CARTRIDGE "
typedef struct _CBM_CRTHeader {
	char magic[16];		// [0x000] Magic (identifies computer type)
	uint32_t hdr_len;	// [0x010] Header length (always 0x40)
	uint16_t version;	// [0x014] Version: $0100 (v1.0), $0101 (v1.1), $0200 (v2.0)
				// v1.1 adds subtypes; v2.0 adds non-C64 machines
	uint16_t type;		// [0x016] Cartridge type
	uint8_t c64_exrom;	// [0x018] EXROM line status (C64, type 0 only)
	uint8_t c64_game;	// [0x019] GAME line status (C64, type 0 only)
	uint8_t subtype;	// [0x01A] Subtype (v1.1+)
	uint8_t reserved[5];	// [0x01B]
	char title[32];		// [0x020] Cartridge title, NULL-padded.
} CBM_CRTHeader;
ASSERT_STRUCT(CBM_CRTHeader, 0x40);

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_CBM_CART_STRUCTS_H__ */
