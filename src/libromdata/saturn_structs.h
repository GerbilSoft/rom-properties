/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * dc_structs.h: Sega Dreamcast data structures.                           *
 *                                                                         *
 * Copyright (c) 2017 by David Korth.                                      *
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

/**
 * References:
 * - https://antime.kapsi.fi/sega/docs.html
 * - https://antime.kapsi.fi/sega/files/ST-040-R4-051795.pdf
 * - https://www.gamefaqs.com/saturn/916393-sega-saturn/faqs/26021
 */

#include "librpbase/common.h"
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

/**
 * Dreamcast peripherals bitfield.
 * For most items, indicates support for that feature.
 * For Windows CE, indicates the game uses it.
 *
 * For controller buttons, indicates a minimum requirement.
 * Example: If "Z" is listed, then the game must have a
 * controller with a "Z" button; otherwise, it won't work.
 *
 * Reference: http://mc.pp.se/dc/ip0000.bin.html
 */
typedef enum {
	DCP_WINDOWS_CE		= (1 << 0),	// Uses Windows CE
	DCP_VGA_BOX		= (1 << 4),	// Supports VGA Box

	// Supported expansion units.
	DCP_EXP_OTHER		= (1 << 8),	// Other expansions.
	DCP_PURU_PURU		= (1 << 9),	// Puru Puru pack (Jump Pack)
	DCP_MICROPHONE		= (1 << 10),	// Microphone
	DCP_MEMORY_CARD		= (1 << 11),	// Memory Card (VMU)

	// Controller requirements.
	// If any of these bits are set, the game *requires*
	// a controller with the specified functionality,
	DCP_CTRL_START_A_B_DPAD	= (1 << 12),	// Start, A, B, D-Pad
	DCP_CTRL_C		= (1 << 13),	// C button
	DCP_CTRL_D		= (1 << 14),	// D button
	DCP_CTRL_X		= (1 << 15),	// X button
	DCP_CTRL_Y		= (1 << 16),	// Y button
	DCP_CTRL_Z		= (1 << 17),	// Z button
	DCP_CTRL_DPAD_2		= (1 << 18),	// Second D-Pad
	DCP_CTRL_ANALOG_RT	= (1 << 19),	// Analog R trigger
	DCP_CTRL_ANALOG_LT	= (1 << 20),	// Analog L trigger
	DCP_CTRL_ANALOG_H1	= (1 << 21),	// Analog horizontal controller
	DCP_CTRL_ANALOG_V1	= (1 << 22),	// Analog vertical controller
	DCP_CTRL_ANALOG_H2	= (1 << 23),	// Analog horizontal controller #2
	DCP_CTRL_ANALOG_V2	= (1 << 24),	// Analog vertical controller #2

	// Optional expansion peripherals.
	DCP_CTRL_GUN		= (1 << 25),	// Light Gun
	DCP_CTRL_KEYBOARD	= (1 << 26),	// Keyboard
	DCP_CTRL_MOUSE		= (1 << 27),	// Mouse
} DC_IP0000_BIN_Peripherals;

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_DC_STRUCTS_H__ */
