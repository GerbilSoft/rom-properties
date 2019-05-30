/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * dmg_structs.h: Game Boy (DMG/CGB/SGB) data structures.                  *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
 * Copyright (c) 2016 by Egor.                                             *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_DMG_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_DMG_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

/**
 * Game Boy ROM header.
 * This matches the ROM header format exactly.
 * References:
 * - http://problemkaputt.de/pandocs.htm#thecartridgeheader
 * - http://gbdev.gg8.se/wiki/articles/The_Cartridge_Header
 * 
 * NOTE: Strings are NOT null-terminated!
 */
typedef struct PACKED _DMG_RomHeader {
	uint8_t entry[4];
	uint8_t nintendo[0x30];

	/**
	 * There are 3 variations on the next 16 bytes:
	 * 1) title(16)
	 * 2) title(15) cgbflag(1)
	 * 3) title(11) gamecode(4) cgbflag(1)
	 *
	 * In all three cases, title is NULL-padded.
	 */
	union {
		char title16[16];
		struct {
			union {
				char title15[15];
				struct {
					char title11[11];
					char id4[4];
				};
			};
			uint8_t cgbflag;
		};
	};

	char new_publisher_code[2];
	uint8_t sgbflag;
	uint8_t cart_type;
	uint8_t rom_size;
	uint8_t ram_size;
	uint8_t region;
	uint8_t old_publisher_code;
	uint8_t version;

	uint8_t header_checksum; // checked by bootrom
	uint16_t rom_checksum;   // checked by no one
} DMG_RomHeader;
ASSERT_STRUCT(DMG_RomHeader, 80);

/**
 * GBX footer.
 * All fields are in big-endian.
 *
 * References:
 * - http://hhug.me/gbx/1.0
 * - https://github.com/GerbilSoft/rom-properties/issues/125
 */
#define GBX_MAGIC 'GBX!'
typedef struct _GBX_Footer {
	/** Cartridge information. **/
	union {
		char mapper[4];		// [0x000] Mapper identifier. (ASCII; NULL-padded)
		uint32_t mapper_id;	// [0x000] Mapper identifier. (See GBX_Mapper_e.)
	};
	uint8_t battery_flag;		// [0x004] 1 if battery is present; 0 if not.
	uint8_t rumble_flag;		// [0x005] 1 if rumble is present; 0 if not.
	uint8_t timer_flag;		// [0x006] 1 if timer is present; 0 if not.
	uint8_t reserved1;		// [0x007]
	uint32_t rom_size;		// [0x008] ROM size, in bytes.
	uint32_t ram_size;		// [0x00C] RAM size, in bytes.
	uint32_t mapper_vars[8];	// [0x010] Mapper-specific variables.

	/** GBX metadata. **/
	uint32_t footer_size;		// [0x030] Footer size, in bytes. (Should be 64.)
	struct {
		uint32_t major;		// [0x034] Major version number.
		uint32_t minor;		// [0x038] Minor version number.
	} version;
	uint32_t magic;			// [0x03C] "GBX!"
} GBX_Footer;
ASSERT_STRUCT(GBX_Footer, 64);

/**
 * GBX: Mapper FourCCs.
 */
typedef enum {
	// Nintendo
	GBX_MAPPER_ROM_ONLY		= 'ROM\0',
	GBX_MAPPER_MBC1			= 'MBC1',
	GBX_MAPPER_MBC2			= 'MBC2',
	GBX_MAPPER_MBC3			= 'MBC3',
	GBX_MAPPER_MBC5			= 'MBC5',
	GBX_MAPPER_MBC7			= 'MBC7',
	GBX_MAPPER_MBC1_MULTICART	= 'MB1M',
	GBX_MAPPER_MMM01		= 'MMM1',
	GBX_MAPPER_POCKET_CAMERA	= 'CAMR',

	// Licensed third-party
	GBX_MAPPER_HuC1			= 'HUC1',
	GBX_MAPPER_HuC3			= 'HUC3',
	GBX_MAPPER_TAMA5		= 'TAM5',

	// Unlicensed
	GBX_MAPPER_BBD			= 'BBD\0',
	GBX_MAPPER_HITEK		= 'HITK',
	GBX_MAPPER_SINTAX		= 'SNTX',
	GBX_MAPPER_NT_OLDER_TYPE_1	= 'NTO1',
	GBX_MAPPER_NT_OLDER_TYPE_2	= 'NTO2',
	GBX_MAPPER_NT_NEWER		= 'NTN\0',
	GBX_MAPPER_LI_CHENG		= 'LICH',
	GBX_MAPPER_LAST_BIBLE		= 'LBMC',
	GBX_MAPPER_LIEBAO		= 'LIBA',
} GBX_Mapper_e;

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_DMG_STRUCTS_H__ */
