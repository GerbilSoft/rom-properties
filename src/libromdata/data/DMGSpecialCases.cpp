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
};

// NOTE: Linux is case-sensitive; Windows is not.
// TODO: Sachen "TETRIS" ROMs have the same global checksum.

// DMG, Non-JP
static const DmgSpecialCase_t dmgSpecialCases_DMG_NoJP[] = {
	{"BIONIC-COMMANDO", ""},
	{"BOKEMOB BLUE", ""},
	{"CAESARS PALACE", "61"},
	{"COKEMON BLUE", ""},
	{"COOL SPOT", ""},
	{"DENNIS", "67"},
	{"DIG DUG", ""},
	{"DIG DUG+  ASG", ""},		// Other hacks
	{"DONKEYKONGLAND 3", ""},
	{"DUCK TALES", ""},
	{"DUCK TALES+ ASG", ""},	// Other hacks
	{"GALAGA&GALAXIAN", "01"},	// TM vs. (R); different CGB colorization
	{"LOST WORLD", "78"},
	{"MOTOCROSS+  ASG", ""},	// Other hacks
	{"MOTOCROSSMANIACS", ""},
	{"MYSTIC QUEST", ""},
	{"NFL QUARTERBACK", "56"},
	{"OBELIX", ""},
	{"PAC-MAN", "AF"},
	{"PKMN Generations", ""},
	{"POKEMON AQUA", ""},
	{"POKEMON BLUE", ""},
	{"POKEMON RED", ""},
	{"Pokemon Blue", ""},
	{"Pokemon Red", ""},
	{"SGBPACK", "01"},		// Unl
	{"SNOW BROS.JR", ""},
	{"SOLOMON'S CLUB", ""},
	{"SPY VS SPY", "7F"},
	{"SUPER HUNCHBACK", "67"},
	{"TAZMANIA", "78"},
	{"TESSERAE", "54"},
	{"THE LION KING", ""},
	{"THE SWORD OFHOPE", "7F"},
	{"TOM AND JERRY", ""},
	{"TRACK MEET", ""},
	{"Zelda Colour", ""},	// Other hacks
};

// DMG, JP
static const DmgSpecialCase_t dmgSpecialCases_DMG_JP[] = {
	{"GAME", ""},			// Sachen
	{"GBWARST", ""},
	{"MENU", "00"},			// Unl
	{"POCKET MONSTERS", ""},
	{"POCKETMON", ""},
	{"SAGA", "C3"},
	{"TEST", "00"},			// Unl
	{"TOM AND JERRY", ""},
};

// CGB, Non-JP
static const DmgSpecialCase_t dmgSpecialCases_CGB_NoJP[] = {
	{"BUGS BUNNY", ""},
	{"COOL HAND", ""},
	{"GB SMART CARD", ""},	// Unl
	{"HARVEST-MOON GB", ""},
	{"SHADOWGATE CLAS", ""},
	{"SHANGHAI POCKET", ""},
	{"SYLVESTER", ""},
	{"ZELDA", ""},
	{"ZELDA PL", ""},
};

// CGB, JP
static const DmgSpecialCase_t dmgSpecialCases_CGB_JP[] = {
	{"DIGIMON 5", "MK"},
	{"GBDAYTEST", ""},	// Unl
	{"HARVEST-MOON GB", ""},
	{"METAL SLUG 2", "01"},
};

// Dispatch array for DMG special cases
struct DmgSpecialCase_Dispatch_tbl_t {
	const DmgSpecialCase_t *ptr;
	size_t size;
};
static const DmgSpecialCase_Dispatch_tbl_t dmgSpecialCases_dispatch_tbl[] = {
	{dmgSpecialCases_DMG_NoJP, ARRAY_SIZE(dmgSpecialCases_DMG_NoJP)},
	{dmgSpecialCases_DMG_JP,   ARRAY_SIZE(dmgSpecialCases_DMG_JP)},
	{dmgSpecialCases_CGB_NoJP, ARRAY_SIZE(dmgSpecialCases_CGB_NoJP)},
	{dmgSpecialCases_CGB_JP,   ARRAY_SIZE(dmgSpecialCases_CGB_JP)},
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
	char pbcode[3];
	get_publisher_code(pbcode, romHeader);
	const uint8_t flags = get_lookup_flags(romHeader);
	assert(flags < ARRAY_SIZE(dmgSpecialCases_dispatch_tbl));
	if (flags >= ARRAY_SIZE(dmgSpecialCases_dispatch_tbl))
		return false;

	const char *const title = romHeader->title16;
	size_t title_len = (romHeader->cgbflag & 0x80) ? 15 : 16;
	// Trim title_len for spaces and NULLs.
	for (; title_len > 0; title_len--) {
		const char chr = title[title_len-1];
		if (chr == '\0' || chr == ' ') {
			// Space. Keep going.
		} else {
			// Found a non-space character. We're done.
			break;
		}
	}
	if (title_len == 0) {
		// No title...
		return false;
	}

	// TODO: Change to a binary search.
	size_t count = dmgSpecialCases_dispatch_tbl[flags].size;
	for (const DmgSpecialCase_t *p = dmgSpecialCases_dispatch_tbl[flags].ptr; count > 0; p++, count--) {
		if (p->title[title_len] != '\0') {
			// Title length is incorrect. Not a match.
			continue;
		} else if (strncmp(p->title, title, title_len) != 0) {
			// Title does not match.
			continue;
		}

		if (p->publisher[0] != '\0' && strcmp(pbcode, p->publisher) != 0) {
			// Publisher is being checked and does not match.
			continue;
		}

		// Title and publisher match. We found it!
		return true;
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
