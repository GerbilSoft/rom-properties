/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * DMGSpecialCases.cpp: Game Boy special cases for RPDB images.            *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "DMGSpecialCases.hpp"
#include "Handheld/dmg_structs.h"

namespace LibRomData { namespace DMGSpecialCases {

// Special cases for ROM images with identical titles.
// Flags value indicates "supports CGB" and "is JP".
// Array *must* be sorted by title, then publisher, then flags value.
// NOTE: An empty publisher value indicates the publisher isn't relevant.

#define FLAG_NONE	0
#define FLAG_JP		(1U << 0)
#define FLAG_CGB	(1U << 1)

struct DmgSpecialCase_t {
	char title[17];
	char publisher[3];
	uint8_t flags;
};

// NOTE: Linux is case-sensitive; Windows is not.
// TODO: Sachen "TETRIS" ROMs have the same global checksum.
static const DmgSpecialCase_t dmgSpecialCases[] = {
	{"BIONIC-COMMANDO", "", FLAG_NONE},
	{"BOKEMOB BLUE", "", FLAG_NONE},
	{"BUGS BUNNY", "", FLAG_CGB},
	{"CAESARS PALACE", "61", FLAG_NONE},
	{"COKEMON BLUE", "", FLAG_NONE},
	{"COOL HAND", "", FLAG_CGB},
	{"COOL SPOT", "", FLAG_NONE},
	{"DENNIS", "67", FLAG_NONE},
	{"DIG DUG", "", FLAG_NONE},
	{"DIG DUG+  ASG", "", FLAG_NONE},		// Other hacks
	{"DIGIMON 5", "MK", FLAG_JP|FLAG_CGB},
	{"DONKEYKONGLAND 3", "", FLAG_NONE},
	{"DUCK TALES", "", FLAG_NONE},
	{"DUCK TALES+ ASG", "", FLAG_NONE},	// Other hacks
	{"GALAGA&GALAXIAN", "01", FLAG_NONE},	// TM vs. (R); different CGB colorization
	{"GAME", "", FLAG_JP},			// Sachen
	{"GB SMART CARD", "", FLAG_CGB},	// Unl
	{"GBDAYTEST", "", FLAG_JP|FLAG_CGB},	// Unl
	{"GBWARST", "", FLAG_JP},
	{"HARVEST-MOON GB", "", FLAG_CGB},
	{"HARVEST-MOON GB", "", FLAG_JP|FLAG_CGB},
	{"LOST WORLD", "78", FLAG_NONE},
	{"MENU", "00", FLAG_JP},			// Unl
	{"METAL SLUG 2", "01", FLAG_JP|FLAG_CGB},
	{"MOTOCROSS+  ASG", "", FLAG_NONE},	// Other hacks
	{"MOTOCROSSMANIACS", "", FLAG_NONE},
	{"MYSTIC QUEST", "", FLAG_NONE},
	{"NFL QUARTERBACK", "56", FLAG_NONE},
	{"OBELIX", "", FLAG_NONE},
	{"PAC-MAN", "AF", FLAG_NONE},
	{"PKMN Generations", "", FLAG_NONE},
	{"POCKET MONSTERS", "", FLAG_JP},
	{"POCKETMON", "", FLAG_JP},
	{"POKEMON AQUA", "", FLAG_NONE},
	{"POKEMON BLUE", "", FLAG_NONE},
	{"POKEMON RED", "", FLAG_NONE},
	{"Pokemon Blue", "", FLAG_NONE},
	{"Pokemon Red", "", FLAG_NONE},
	{"SAGA", "C3", FLAG_JP},
	{"SGBPACK", "01", FLAG_NONE},		// Unl
	{"SHADOWGATE CLAS", "", FLAG_CGB},
	{"SHANGHAI POCKET", "", FLAG_CGB},
	{"SNOW BROS.JR", "", FLAG_NONE},
	{"SOLOMON'S CLUB", "", FLAG_NONE},
	{"SPY VS SPY", "7F", FLAG_NONE},
	{"SUPER HUNCHBACK", "67", FLAG_NONE},
	{"SYLVESTER", "", FLAG_CGB},
	{"TAZMANIA", "78", FLAG_NONE},
	{"TESSERAE", "54", FLAG_NONE},
	{"TEST", "00", FLAG_JP},			// Unl
	{"THE LION KING", "", FLAG_NONE},
	{"THE SWORD OFHOPE", "7F", FLAG_NONE},
	{"TOM AND JERRY", "", FLAG_NONE},
	{"TOM AND JERRY", "", FLAG_JP},
	{"TRACK MEET", "", FLAG_NONE},
	{"ZELDA", "", FLAG_CGB},
	{"ZELDA PL", "", FLAG_CGB},
	{"Zelda Colour", "", FLAG_NONE},	// Other hacks

	{"", "", FLAG_NONE}
};

// Special cases for CGB ROM images with identical ID6s.
static const char cgbSpecialCases[][8] = {
	// Loppi Puzzle Magazine
	"B52J8N", "B53J8N", "B5IJ8N",
	"B62J8N", "B63J8N", "B6IJ8N",

	// Antz Racing (E) - different non-CGB error screens
	"BAZP69",

	// Gift (E) - different non-CGB error screens
	"BGFP5T",

	// Tomb Raider (UE) - different non-CGB error screens
	"AT9E78",

	// F-1 Racing Championship (E) - slightly different copyright text on CGB
	"AEQP41",

	// Pokémon Crystal (U) - "Pokémon 2004" hack has the same game ID.
	"BYTE01",
};

/**
 * Get the publisher code as a string.
 * @param pbcode	[out] Output buffer (Must be at least 3 bytes)
 * @param romHeader	[in] ROM header
 * @return 0 on success; non-zero on error.
 */
int get_publisher_code(char pbcode[3], const DMG_RomHeader *romHeader)
{
	// TODO: Consolidate with DMG's code.
	if (romHeader->old_publisher_code == 0x33) {
		// New publisher code.
		if (unlikely(romHeader->new_publisher_code[0] == '\0' &&
		             romHeader->new_publisher_code[1] == '\0'))
		{
			// NULL publisher code. Use 00.
			// NOTE: memcpy() here is optimized by gcc, but *not* MSVC. (due to len=3)
			pbcode[0] = '0';
			pbcode[1] = '0';
			pbcode[2] = '\0';
		} else {
			pbcode[0] = romHeader->new_publisher_code[0];
			pbcode[1] = romHeader->new_publisher_code[1];
			pbcode[2] = '\0';
		}
	} else {
		// Old publisher code.
		snprintf(pbcode, 3, "%02X", romHeader->old_publisher_code);
	}

	return 0;
}

/**
 * Get the lookup flags for a given DMG ROM header.
 * @param romHeader DMG ROM header
 * @return Lookup flags
 */
static constexpr uint8_t get_lookup_flags(const DMG_RomHeader *romHeader)
{
	return ((romHeader->region == 0) ? FLAG_JP : FLAG_NONE) |
	       ((romHeader->cgbflag & 0x80) ? FLAG_CGB : FLAG_NONE);
}

/**
 * Check if a DMG ROM image needs a checksum appended to its
 * filename for proper RPDB image lookup.
 *
 * Title-based version. This is used for games that don't have
 * a valid Game ID.
 *
 * @param romHeader DMG_RomHeader
 * @return True if a checksum is needed; false if not.
 */
bool is_rpdb_checksum_needed_TitleBased(const DMG_RomHeader *romHeader)
{
	// ROM title is either 15 or 16 characters, depending on
	// if the CGB mode byte is present.
	// If an ID6 is present, caller must use the ID6 function.
	// TODO: Do the ID6 check here instead?
	const char *const title = romHeader->title16;
	const size_t title_len = (romHeader->cgbflag & 0x80) ? 15 : 16;

	char pbcode[3];
	get_publisher_code(pbcode, romHeader);
	const uint8_t flags = get_lookup_flags(romHeader);

	// TODO: Change to a binary search.
	for (const DmgSpecialCase_t *p = dmgSpecialCases; p->title[0] != '\0'; p++) {
		if (strncmp(p->title, title, title_len) != 0) {
			// Title does not match.
			continue;
		}

		if (p->publisher[0] != '\0' && strcmp(pbcode, p->publisher) != 0) {
			// Publisher is being checked and does not match.
			continue;
		}

		if (flags == p->flags) {
			// Lookup flags match. We found it!
			return true;
		}
	}

	// Not found.
	return false;
}

/**
 * Check if a DMG ROM image needs a checksum appended to its
 * filename for proper RPDB image lookup.
 *
 * Game ID version. This is used for CGB games with a valid ID6.
 *
 * @param id6 ID6
 * @return True if a checksum is needed; false if not.
 */
bool is_rpdb_checksum_needed_ID6(const char *id6)
{
	// TODO: Binary search?
	for (const char *p = &cgbSpecialCases[0][0];
	     p[0] != '\0'; p += sizeof(cgbSpecialCases[0]))
	{
		if (!strncmp(p, id6, 6)) {
			// Game ID matches.
			return true;
		}
	}

	// No match.
	return false;
}

} }
