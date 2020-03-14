/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * saturn_structs.h: Sega Saturn data structures.                          *
 *                                                                         *
 * Copyright (c) 2017-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_SATURN_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_SATURN_STRUCTS_H__

/**
 * References:
 * - https://antime.kapsi.fi/sega/docs.html
 * - https://antime.kapsi.fi/sega/files/ST-040-R4-051795.pdf
 * - https://www.gamefaqs.com/saturn/916393-sega-saturn/faqs/26021
 */

#include "common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

/**
 * IP0000.BIN
 * This is located in the boot sector of CD-ROM track 1.
 *
 * All fields are in little-endian.
 */
#define SATURN_IP0000_BIN_HW_ID		"SEGA SEGASATURN "
#define SATURN_IP0000_BIN_MAKER_ID	"SEGA ENTERPRISES"
typedef struct PACKED _Saturn_IP0000_BIN_t {
	char hw_id[16];			// "SEGA SEGASATURN "
	char maker_id[16];		// Sega:  "SEGA ENTERPRISES"
					// Other: "SEGA TP T-999   "
	char product_number[10];	// Sega:  "GS-xxxx   "
					// Other: "T-xxxxxx  "
	char product_version[6];	// "V1.000"
	char release_date[8];		// "YYYYMMDD"
	char device_info[8];		// "CD-1/1  "
	char area_symbols[10];		// "JTUE      " (position-dependent)
	uint8_t reserved1[6];
	char peripherals[16];		// MD-style peripherals listing.
	char title[112];		// Software title. (TODO: Encoding)
	uint8_t reserved2[16];
	uint32_t ip_size;
	uint32_t reserved3;
	uint32_t stack_m;
	uint32_t stack_s;
	uint32_t boot_addr;		// 1st Read address (LBA)
	uint32_t boot_size;		// 1st Read size (bytes)
	uint32_t reserved4[2];
} Saturn_IP0000_BIN_t;
ASSERT_STRUCT(Saturn_IP0000_BIN_t, 256);

/**
 * Sega Saturn peripherals.
 * Maps to Saturn_IP0000_BIN_t.peripherals[] entries.
 */
typedef enum {
	SATURN_IO_CONTROL_PAD		= 'J',
	SATURN_IO_ANALOG_CONTROLLER	= 'A',
	SATURN_IO_MOUSE			= 'M',
	SATURN_IO_KEYBOARD		= 'K',
	SATURN_IO_STEERING		= 'S',
	SATURN_IO_MULTITAP		= 'T',

	// from https://www.gamefaqs.com/saturn/916393-sega-saturn/faqs/26021
	SATURN_IO_LIGHT_GUN		= 'G',
	SATURN_IO_RAM_CARTRIDGE		= 'W',
	SATURN_IO_3D_CONTROLLER		= 'E',
	SATURN_IO_LINK_CABLE_JPN	= 'C',
	SATURN_IO_LINK_CABLE_USA	= 'D',
	SATURN_IO_NETLINK		= 'X',
	SATURN_IO_PACHINKO		= 'Q',
	SATURN_IO_FDD			= 'F',
	SATURN_IO_ROM_CARTRIDGE		= 'R',
	SATURN_IO_MPEG_CARD		= 'P',
} Saturn_Peripherals;

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_SATURN_STRUCTS_H__ */
