/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * pkmnmini_structs.h: Pokémon Mini data structures.                       *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_HANDHELD_PKMNMINI_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_HANDHELD_PKMNMINI_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

/**
 * Pokémon Mini ROM header.
 * This matches the Pokémon Mini ROM header format exactly.
 * References:
 * - https://www.pokemon-mini.net/documentation/cartridge/
 * - https://wiki.sublab.net/index.php/PM_Cartridge
 *
 * All fields are in ???-endian.
 *
 * NOTE: Cartridge header starts at 0x2100.
 * NOTE: IRQs are load+long-jump instructions.
 * NOTE: Strings are NOT null-terminated!
 */
#define POKEMONMINI_HEADER_ADDRESS 0x2100
#define POKEMONMINI_PM_MAGIC 'PM'	// Documents say it has 'PM',
#define POKEMONMINI_MN_MAGIC 'MN'	// but my test images have 'MN'...
#define POKEMONMINI_2P_MAGIC '2P'
typedef struct PACKED _PokemonMini_RomHeader {
	uint16_t pm_magic;	// [0x000] 'PM' or 'MN'
	uint8_t irqs[27][6];	// [0x002] IRQs. (See PokemonMini_IRQ_e for descriptions.)
	char nintendo[8];	// [0x0A4] "NINTENDO"
	char game_id[4];	// [0x0AC] Game ID
	char title[12];		// [0x0B0] Game title (NULL-padded)
	uint16_t unk_2p;	// [0x0BC] '2P' (unknown purpose)
	uint8_t reserved[18];	// [0x0BE] Reserved (zero)
} PokemonMini_RomHeader;
ASSERT_STRUCT(PokemonMini_RomHeader, 208);

/**
 * IRQ descriptions.
 */
typedef enum {
	PokemonMini_IRQ_Reset			= 0,
	PokemonMini_IRQ_PRC_Frame_Copy		= 1,
	PokemonMini_IRQ_PRC_Render		= 2,
	PokemonMini_IRQ_Timer2_Underflow_Hi	= 3,
	PokemonMini_IRQ_Timer2_Underflow_Lo	= 4,
	PokemonMini_IRQ_Timer1_Underflow_Hi	= 5,
	PokemonMini_IRQ_Timer1_Underflow_Lo	= 6,
	PokemonMini_IRQ_Timer3_Underflow_Hi	= 7,
	PokemonMini_IRQ_Timer3_Comparator	= 8,
	PokemonMini_IRQ_Timer_32Hz		= 9,
	PokemonMini_IRQ_Timer_8Hz		= 10,
	PokemonMini_IRQ_Timer_2Hz		= 11,
	PokemonMini_IRQ_Timer_1Hz		= 12,
	PokemonMini_IRQ_IR_Receiver		= 13,
	PokemonMini_IRQ_Shake_Sensor		= 14,
	PokemonMini_IRQ_KBD_Power		= 15,
	PokemonMini_IRQ_KBD_Right		= 16,
	PokemonMini_IRQ_KBD_Left		= 17,
	PokemonMini_IRQ_KBD_Down		= 18,
	PokemonMini_IRQ_KBD_Up			= 19,
	PokemonMini_IRQ_KBD_C			= 20,
	PokemonMini_IRQ_KBD_B			= 21,
	PokemonMini_IRQ_KBD_A			= 22,
	PokemonMini_IRQ_Cartridge		= 26,

	PokemonMini_IRQ_MAX			= 27
} PokemonMini_IRQ_e;

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_HANDHELD_PKMNMINI_STRUCTS_H__ */
