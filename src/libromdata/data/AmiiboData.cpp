/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * AmiiboData.cpp: Nintendo amiibo identification data.                    *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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

#include "AmiiboData.hpp"

// C includes.
#include <stdlib.h>

namespace LibRomData {

/**
 * References:
 * - https://www.3dbrew.org/wiki/Amiibo
 * - https://www.reddit.com/r/amiibo/comments/38hwbm/nfc_character_identification_my_findings_on_the/
 * - https://docs.google.com/spreadsheets/d/19E7pMhKN6x583uB6bWVBeaTMyBPtEAC-Bk59Y6cfgxA/
 */

/**
 * amiibo ID format
 * Two 4-byte pages starting at page 21 (raw offset 0x54).
 * Format: ssscvvtt-aaaaSS02
 * - sssc: Character series and ID.
 *         Series is bits 54-63.
 *         Character is bits 48-53.
 *         This allows up to 64 characters per series.
 *         Some series, e.g. Pokemon, has multiple series
 *         identifiers reserved.
 * - vv: Character variation.
 * - tt: Type. 00 = figure, 01 == card, 02 == plush (yarn)
 * - aaaa: amiibo ID. (Unique across all amiibo.)
 * - SS: amiibo series.
 * - 02: Always 02.
 */

class AmiiboDataPrivate {
	private:
		// Static class.
		AmiiboDataPrivate();
		~AmiiboDataPrivate();
		RP_DISABLE_COPY(AmiiboDataPrivate)

	public:
		/** Page 21 (raw offset 0x54): Character series **/
		static const char *const char_series_names[];

		// Character variants.
		// We can't use a standard character array because
		// the Skylanders variants use variant ID = 0xFF.
		struct char_variant_t {
			uint8_t variant_id;
			const char *const name;
		};

		// Character IDs.
		// Sparse array, since we're using the series + character value here.
		// Sorted by series + character value.
		struct char_id_t {
			uint16_t char_id;		// Character ID. (Includes series ID.) [high 16 bits of page 21]
			uint8_t variants_size;		// Number of elements in variants.
			const char *name;		// Character name. (same as variant 0)
			const char_variant_t *variants;	// Array of variants, if any.
		};

		// Character variants.
		static const char_variant_t smb_mario_variants[];
		static const char_variant_t smb_yoshi_variants[];
		static const char_variant_t smb_rosalina_variants[];
		static const char_variant_t smb_bowser_variants[];
		static const char_variant_t smb_donkey_kong_variants[];
		static const char_variant_t yoshi_poochy_variants[];
		static const char_variant_t tloz_link_variants[];
		static const char_variant_t tloz_zelda_variants[];
		static const char_variant_t tloz_ganondorf_variants[];
		static const char_variant_t metroid_samus_variants[];
		static const char_variant_t pikmin_olimar_variants[];
		static const char_variant_t mii_variants[];
		static const char_variant_t splatoon_inkling_variants[];
		static const char_variant_t fe_corrin_variants[];

		static const char_variant_t mss_mario_variants[];
		static const char_variant_t mss_luigi_variants[];
		static const char_variant_t mss_peach_variants[];
		static const char_variant_t mss_daisy_variants[];
		static const char_variant_t mss_yoshi_variants[];
		static const char_variant_t mss_wario_variants[];
		static const char_variant_t mss_waluigi_variants[];
		static const char_variant_t mss_donkey_kong_variants[];
		static const char_variant_t mss_diddy_kong_variants[];
		static const char_variant_t mss_bowser_variants[];
		static const char_variant_t mss_bowser_jr_variants[];
		static const char_variant_t mss_boo_variants[];
		static const char_variant_t mss_baby_mario_variants[];
		static const char_variant_t mss_baby_luigi_variants[];
		static const char_variant_t mss_birdo_variants[];
		static const char_variant_t mss_rosalina_variants[];
		static const char_variant_t mss_metal_mario_variants[];
		static const char_variant_t mss_pink_gold_peach_variants[];

		static const char_variant_t ac_isabelle_variants[];
		static const char_variant_t ac_kk_slider_variants[];
		static const char_variant_t ac_tom_nook_variants[];
		static const char_variant_t ac_timmy_variants[];
		static const char_variant_t ac_tommy_variants[];
		static const char_variant_t ac_digby_variants[];
		static const char_variant_t ac_resetti_variants[];
		static const char_variant_t ac_don_resetti_variants[];
		static const char_variant_t ac_redd_variants[];
		static const char_variant_t ac_dr_shrunk_variants[];
		static const char_variant_t ac_lottie_variants[];
		// Animal Crossing x Sanrio all have variant type 0x01.
		static const char_variant_t ac_marty_variants[];
		static const char_variant_t ac_chelsea_variants[];
		static const char_variant_t ac_chai_variants[];
		static const char_variant_t ac_rilla_variants[];
		static const char_variant_t ac_toby_variants[];
		static const char_variant_t ac_etoile_variants[];
		// Monster Hunter
		static const char_variant_t mh_rathalos_variants[];
		static const char_variant_t mh_rathian_cheval_variants[];
		static const char_variant_t mh_barioth_ayuria_variants[];
		static const char_variant_t mh_qurupeco_dan_variants[];

		// Cereal
		static const char_variant_t cereal_smb_variants[];

		// Character IDs.
		static const char_id_t char_ids[];

		/**
		 * char_id_t bsearch() comparison function.
		 * @param a
		 * @param b
		 * @return
		 */
		static int RP_C_API char_id_t_compar(const void *a, const void *b);

		/** Page 22 (raw offset 0x58): amiibo series **/

		// amiibo series names.
		// Array index = SS
		static const char *const amiibo_series_names[];

		// amiibo IDs.
		// Index is the amiibo ID. (aaaa)
		// NOTE: amiibo ID is unique across *all* amiibo,
		// so we can use a single array for all series.
		struct amiibo_id_t {
			uint16_t release_no;	// Release number. (0 for no ordering)
			uint8_t wave_no;	// Wave number.
			const char *name;	// Character name.
		};
		static const amiibo_id_t amiibo_ids[];
};

/** Page 21 (raw offset 0x54): Character series **/

/**
 * Character series.
 * Array index == sss, rshifted by 2.
 */
const char *const AmiiboDataPrivate::char_series_names[] = {
	"Super Mario Bros.",	// 0x000
	nullptr,		// 0x004
	"Yoshi",		// 0x008
	nullptr,		// 0x00C
	"The Legend of Zelda",	// 0x010
	"The Legend of Zelda",	// 0x014

	// Animal Crossing
	"Animal Crossing",	// 0x018
	"Animal Crossing",	// 0x01C
	"Animal Crossing",	// 0x020
	"Animal Crossing",	// 0x024
	"Animal Crossing",	// 0x028
	"Animal Crossing",	// 0x02C
	"Animal Crossing",	// 0x030
	"Animal Crossing",	// 0x034
	"Animal Crossing",	// 0x038
	"Animal Crossing",	// 0x03C
	"Animal Crossing",	// 0x040
	"Animal Crossing",	// 0x044
	"Animal Crossing",	// 0x048
	"Animal Crossing",	// 0x04C
	"Animal Crossing",	// 0x050

	nullptr,		// 0x054
	"Star Fox",		// 0x058
	"Metroid",		// 0x05C
	"F-Zero",		// 0x060
	"Pikmin",		// 0x064
	nullptr,		// 0x068
	"Punch-Out!!",		// 0x06C
	"Wii Fit",		// 0x070
	"Kid Icarus",		// 0x074
	"Classic Nintendo",	// 0x078
	"Mii",			// 0x07C
	"Splatoon",		// 0x080

	// 0x084 - 0x098
	nullptr, nullptr, nullptr,	// 0x084
	nullptr, nullptr, nullptr, 	// 0x090

	"Mario Sports Superstars",	// 0x09C

	// 0x0A0-0x18C
	nullptr, nullptr, nullptr, nullptr,	// 0x0A0
	nullptr, nullptr, nullptr, nullptr,	// 0x0B0
	nullptr, nullptr, nullptr, nullptr,	// 0x0C0
	nullptr, nullptr, nullptr, nullptr,	// 0x0D0
	nullptr, nullptr, nullptr, nullptr,	// 0x0E0
	nullptr, nullptr, nullptr, nullptr,	// 0x0F0
	nullptr, nullptr, nullptr, nullptr,	// 0x100
	nullptr, nullptr, nullptr, nullptr,	// 0x110
	nullptr, nullptr, nullptr, nullptr,	// 0x120
	nullptr, nullptr, nullptr, nullptr,	// 0x130
	nullptr, nullptr, nullptr, nullptr,	// 0x140
	nullptr, nullptr, nullptr, nullptr,	// 0x150
	nullptr, nullptr, nullptr, nullptr,	// 0x160
	nullptr, nullptr, nullptr, nullptr,	// 0x170
	nullptr, nullptr, nullptr, nullptr,	// 0x180

	// Pokémon (0x190 - 0x1BC)
	// NOTE: MSVC prior to 2015 doesn't support UTF-8 string constants.
	"Pok\xC3\xA9mon", "Pok\xC3\xA9mon", "Pok\xC3\xA9mon", "Pok\xC3\xA9mon",
	"Pok\xC3\xA9mon", "Pok\xC3\xA9mon", "Pok\xC3\xA9mon", "Pok\xC3\xA9mon",
	"Pok\xC3\xA9mon", "Pok\xC3\xA9mon", "Pok\xC3\xA9mon", "Pok\xC3\xA9mon",

	nullptr, nullptr, nullptr, nullptr,	// 0x1C0

	"Pokk\xC3\xA9n Tournament",		// 0x1D0
	nullptr, nullptr, nullptr,		// 0x1D4
	nullptr, nullptr, nullptr, nullptr,	// 0x1E0
	"Kirby",				// 0x1F0
	nullptr, nullptr, nullptr,		// 0x1F4
	nullptr, nullptr, nullptr, nullptr,	// 0x200
	"Fire Emblem",				// 0x210
	nullptr, nullptr, nullptr,		// 0x214
	nullptr,				// 0x220
	"Xenoblade",				// 0x224
	"Earthbound",				// 0x228
	"Chibi-Robo!",				// 0x22C

	// 0x230 - 0x31C
	nullptr, nullptr, nullptr, nullptr,	// 0x230
	nullptr, nullptr, nullptr, nullptr,	// 0x240
	nullptr, nullptr, nullptr, nullptr,	// 0x250
	nullptr, nullptr, nullptr, nullptr,	// 0x260
	nullptr, nullptr, nullptr, nullptr,	// 0x270
	nullptr, nullptr, nullptr, nullptr,	// 0x280
	nullptr, nullptr, nullptr, nullptr,	// 0x290
	nullptr, nullptr, nullptr, nullptr,	// 0x2A0
	nullptr, nullptr, nullptr, nullptr,	// 0x2B0
	nullptr, nullptr, nullptr, nullptr,	// 0x2C0
	nullptr, nullptr, nullptr, nullptr,	// 0x2D0
	nullptr, nullptr, nullptr, nullptr,	// 0x2E0
	nullptr, nullptr, nullptr, nullptr,	// 0x2F0
	nullptr, nullptr, nullptr, nullptr,	// 0x300
	nullptr, nullptr, nullptr, nullptr,	// 0x310

	"Sonic the Hedgehog",			// 0x320
	"Bayonetta",				// 0x324
	nullptr,				// 0x328
	nullptr,				// 0x32C
	nullptr,				// 0x330
	"Pac-Man",				// 0x334
	nullptr,				// 0x338
	nullptr,				// 0x33C
	nullptr,				// 0x340
	nullptr,				// 0x344
	"Mega Man",				// 0x348
	"Street Fighter",			// 0x34C
	"Monster Hunter",			// 0x350
	nullptr,				// 0x354
	nullptr,				// 0x358
	"Shovel Knight",			// 0x35C
	"Final Fantasy",			// 0x360
	nullptr,				// 0x364
	nullptr,				// 0x368
	nullptr,				// 0x36C
	nullptr,				// 0x370
	"Cereal",				// 0x374
};

// Character variants.
const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::smb_mario_variants[] = {
	{0x00, "Mario"},
	{0x01, "Dr. Mario"},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::smb_yoshi_variants[] = {
	{0x00, "Yoshi"},
	{0x01, "Yarn Yoshi"},	// Color variant is in Page 22, amiibo ID.
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::smb_rosalina_variants[] = {
	{0x00, "Rosalina"},
	{0x01, "Rosalina & Luma"},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::smb_bowser_variants[] = {
	{0x00, "Bowser"},

	// Skylanders
	// NOTE: Cannot distinguish between regular and dark
	// variants in amiibo mode.
	{0xFF, "Hammer Slam Bowser"},
	//{0xFF, "Dark Hammer Slam Bowser"},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::smb_donkey_kong_variants[] = {
	{0x00, "Donkey Kong"},

	// Skylanders
	// NOTE: Cannot distinguish between regular and dark
	// variants in amiibo mode.
	{0xFF, "Turbo Charge Donkey Kong"},
	//{0xFF, "Dark Turbo Charge Donkey Kong"},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::yoshi_poochy_variants[] = {
	{0x00, nullptr},	// TODO
	{0x01, "Yarn Poochy"},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::tloz_link_variants[] = {
	{0x00, "Link"},
	{0x01, "Toon Link"},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::tloz_zelda_variants[] = {
	{0x00, "Zelda"},
	{0x01, "Sheik"},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::tloz_ganondorf_variants[] = {
	{0x00, nullptr},	// TODO
	{0x01, "Ganondorf"},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::metroid_samus_variants[] = {
	{0x00, "Samus"},
	{0x01, "Zero Suit Samus"},
	{0x02, "Samus Aran"},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::pikmin_olimar_variants[] = {
	{0x00, nullptr},	// TODO
	{0x01, "Olimar"},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::mii_variants[] = {
	{0x00, "Mii Brawler"},
	{0x01, "Mii Swordfighter"},
	{0x02, "Mii Gunner"},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::splatoon_inkling_variants[] = {
	{0x00, "Inkling"},	// NOTE: Not actually assigned.
	{0x01, "Inkling Girl"},
	{0x02, "Inkling Boy"},
	{0x03, "Inkling Squid"},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::fe_corrin_variants[] = {
	{0x00, "Corrin"},
	{0x01, "Corrin (Player 2"},
};

// Mario Sports Superstars
// Each character has five variants (0x01-0x05).
// NOTE: Variant 0x00 is not actually assigned.
#define MSS_VARIANT_ARRAY_DEF(char_name, c_name) \
	const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::mss_##c_name##_variants[] = { \
		{0x00, char_name}, \
		{0x01, char_name " (Soccer)"}, \
		{0x02, char_name " (Baseball"}, \
		{0x03, char_name " (Tennis)"}, \
		{0x04, char_name " (Golf)"}, \
		{0x05, char_name " (Horse Racing"}, \
	}

MSS_VARIANT_ARRAY_DEF("Mario", mario);
MSS_VARIANT_ARRAY_DEF("Luigi", luigi);
MSS_VARIANT_ARRAY_DEF("Peach", peach);
MSS_VARIANT_ARRAY_DEF("Daisy", daisy);
MSS_VARIANT_ARRAY_DEF("Yoshi", yoshi);
MSS_VARIANT_ARRAY_DEF("Wario", wario);
MSS_VARIANT_ARRAY_DEF("Waluigi", waluigi);
MSS_VARIANT_ARRAY_DEF("Donkey Kong", donkey_kong);
MSS_VARIANT_ARRAY_DEF("Diddy Kong", diddy_kong);
MSS_VARIANT_ARRAY_DEF("Bowser", bowser);
MSS_VARIANT_ARRAY_DEF("Bowser Jr.", bowser_jr);
MSS_VARIANT_ARRAY_DEF("Boo", boo);
MSS_VARIANT_ARRAY_DEF("Baby Mario", baby_mario);
MSS_VARIANT_ARRAY_DEF("Baby Luigi", baby_luigi);
MSS_VARIANT_ARRAY_DEF("Birdo", birdo);
MSS_VARIANT_ARRAY_DEF("Rosalina", rosalina);
MSS_VARIANT_ARRAY_DEF("Metal Mario", metal_mario);
MSS_VARIANT_ARRAY_DEF("Pink Gold Peach", pink_gold_peach);

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_isabelle_variants[] = {
	{0x00, "Isabelle (Summer Outfit)"},
	{0x01, "Isabelle (Autumn Outfit)"},
	// TODO: How are these ones different?
	{0x03, "Isabelle (Series 4)"},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_kk_slider_variants[] = {
	{0x00, "K.K. Slider"},
	{0x01, "DJ K.K."},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_tom_nook_variants[] = {
	{0x00, "Tom Nook"},
	// TODO: Variant description.
	{0x01, "Tom Nook (Series 3)"},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_timmy_variants[] = {
	// TODO: Variant descriptions.
	{0x00, "Timmy"},
	{0x02, "Timmy (Series 3)"},
	{0x04, "Timmy (Series 4)"},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_tommy_variants[] = {
	// TODO: Variant descriptions.
	{0x01, "Tommy (Series 2)"},
	{0x03, "Tommy (Series 4)"},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_digby_variants[] = {
	{0x00, "Digby"},
	// TODO: Variant description.
	{0x01, "Digby (Series 3)"},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_resetti_variants[] = {
	{0x00, "Resetti"},
	// TODO: Variant description.
	{0x01, "Resetti (Series 4)"},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_don_resetti_variants[] = {
	// TODO: Variant descriptions.
	{0x00, "Don Resetti (Series 2)"},
	{0x01, "Don Resetti (Series 3)"},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_redd_variants[] = {
	{0x00, "Redd"},
	// TODO: Variant description.
	{0x01, "Redd (Series 4)"},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_dr_shrunk_variants[] = {
	{0x00, "Dr. Shrunk"},
	{0x01, "Shrunk"},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_lottie_variants[] = {
	{0x00, "Lottie"},
	// TODO: Variant description.
	{0x01, "Lottie (Series 4)"},
};

// Animal Crossing x Sanrio all have variant type 0x01.
const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_marty_variants[] = {
	{0x01, "Marty (Sanrio)"},
};
const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_chelsea_variants[] = {
	{0x01, "Chelsea (Sanrio)"},
};
const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_chai_variants[] = {
	{0x01, "Chai (Sanrio)"},
};
const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_rilla_variants[] = {
	{0x01, "Rilla (Sanrio)"},
};
const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_toby_variants[] = {
	{0x01, "Toby (Sanrio)"},
};
const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_etoile_variants[] = {
	{0x01, "\xC3\x89toile"},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::mh_rathalos_variants[] = {
	{0x00, "One-Eyed Rathalos and Rider"},	// NOTE: Not actually assigned.
	{0x01, "One-Eyed Rathalos and Rider (Male)"},
	{0x02, "One-Eyed Rathalos and Rider (Female)"},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::mh_rathian_cheval_variants[] = {
	{0x00, "Rathian and Cheval"},	// NOTE: Not actually assigned.
	{0x01, "Rathian and Cheval"},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::mh_barioth_ayuria_variants[] = {
	{0x00, "Barioth and Ayuria"},	// NOTE: Not actually assigned.
	{0x01, "Barioth and Ayuria"},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::mh_qurupeco_dan_variants[] = {
	{0x00, "Qurupeco and Dan"},	// NOTE: Not actually assigned.
	{0x01, "Qurupeco and Dan"},
};

// Cereal
const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::cereal_smb_variants[] = {
	{0x00, "Super Mario Cereal"},	// NOTE: Not actually assigned.
	{0x01, "Super Mario Cereal"},
};

#define AMIIBO_CHAR_ID_ONE(id, name) {id, 0, name, nullptr}
#define AMIIBO_CHAR_ID_VAR(id, name, vararray) {id, (uint8_t)ARRAY_SIZE(vararray), name, vararray}

// Character IDs.
const AmiiboDataPrivate::char_id_t AmiiboDataPrivate::char_ids[] = {
	// Super Mario Bros. (character series = 0x000)
	AMIIBO_CHAR_ID_VAR(0x0000, "Mario", smb_mario_variants),
	AMIIBO_CHAR_ID_ONE(0x0001, "Luigi"),
	AMIIBO_CHAR_ID_ONE(0x0002, "Peach"),
	AMIIBO_CHAR_ID_VAR(0x0003, "Yoshi", smb_yoshi_variants),
	AMIIBO_CHAR_ID_VAR(0x0004, "Rosalina", smb_rosalina_variants),
	AMIIBO_CHAR_ID_VAR(0x0005, "Bowser", smb_bowser_variants),
	AMIIBO_CHAR_ID_ONE(0x0006, "Bowser Jr."),
	AMIIBO_CHAR_ID_ONE(0x0007, "Wario"),
	AMIIBO_CHAR_ID_VAR(0x0008, "Donkey Kong", smb_donkey_kong_variants),
	AMIIBO_CHAR_ID_ONE(0x0009, "Diddy Kong"),
	AMIIBO_CHAR_ID_ONE(0x000A, "Toad"),
	AMIIBO_CHAR_ID_ONE(0x0013, "Daisy"),
	AMIIBO_CHAR_ID_ONE(0x0014, "Waluigi"),
	AMIIBO_CHAR_ID_ONE(0x0015, "Goomba"),
	AMIIBO_CHAR_ID_ONE(0x0017, "Boo"),
	AMIIBO_CHAR_ID_ONE(0x0023, "Koopa Troopa"),

	// Yoshi (character series = 0x008)
	AMIIBO_CHAR_ID_VAR(0x0080, "Poochy", yoshi_poochy_variants),

	// The Legend of Zelda (character series = 0x010)
	AMIIBO_CHAR_ID_VAR(0x0100, "Link", tloz_link_variants),
	AMIIBO_CHAR_ID_VAR(0x0101, "Zelda", tloz_zelda_variants),
	AMIIBO_CHAR_ID_VAR(0x0102, "Ganondorf", tloz_ganondorf_variants),
	AMIIBO_CHAR_ID_ONE(0x0103, "Midna & Wolf Link"),
	AMIIBO_CHAR_ID_ONE(0x0105, "Daruk"),
	AMIIBO_CHAR_ID_ONE(0x0106, "Urbosa"),
	AMIIBO_CHAR_ID_ONE(0x0107, "Mipha"),
	AMIIBO_CHAR_ID_ONE(0x0108, "Revali"),
	// The Legend of Zelda (character series = 0x014)
	AMIIBO_CHAR_ID_ONE(0x0141, "Bokoblin"),

	// Animal Crossing (character series = 0x018)
	AMIIBO_CHAR_ID_ONE(0x0180, "Villager"),
	AMIIBO_CHAR_ID_VAR(0x0181, "Isabelle", ac_isabelle_variants),
	AMIIBO_CHAR_ID_ONE(0x0182, "DJ KK"),
	AMIIBO_CHAR_ID_VAR(0x0182, "K.K. Slider", ac_kk_slider_variants),
	AMIIBO_CHAR_ID_VAR(0x0183, "Tom Nook", ac_tom_nook_variants),
	AMIIBO_CHAR_ID_ONE(0x0184, "Timmy & Tommy"),
	AMIIBO_CHAR_ID_VAR(0x0185, "Timmy", ac_timmy_variants),
	AMIIBO_CHAR_ID_VAR(0x0186, "Tommy", ac_tommy_variants),
	AMIIBO_CHAR_ID_ONE(0x0187, "Sable"),
	AMIIBO_CHAR_ID_ONE(0x0188, "Mabel"),
	AMIIBO_CHAR_ID_ONE(0x0189, "Labelle"),
	AMIIBO_CHAR_ID_ONE(0x018A, "Reese"),
	AMIIBO_CHAR_ID_ONE(0x018B, "Cyrus"),
	AMIIBO_CHAR_ID_VAR(0x018C, "Digby", ac_digby_variants),
	AMIIBO_CHAR_ID_ONE(0x018D, "Rover"),
	AMIIBO_CHAR_ID_VAR(0x018E, "Resetti", ac_resetti_variants),
	AMIIBO_CHAR_ID_VAR(0x018F, "Don Resetti", ac_don_resetti_variants),
	AMIIBO_CHAR_ID_ONE(0x0190, "Brewster"),
	AMIIBO_CHAR_ID_ONE(0x0191, "Harriet"),
	AMIIBO_CHAR_ID_ONE(0x0192, "Blathers"),
	AMIIBO_CHAR_ID_ONE(0x0193, "Celeste"),
	AMIIBO_CHAR_ID_ONE(0x0194, "Kicks"),
	AMIIBO_CHAR_ID_ONE(0x0195, "Porter"),
	AMIIBO_CHAR_ID_ONE(0x0196, "Kapp'n"),
	AMIIBO_CHAR_ID_ONE(0x0197, "Leilani"),
	AMIIBO_CHAR_ID_ONE(0x0198, "Lelia"),
	AMIIBO_CHAR_ID_ONE(0x0199, "Grams"),
	AMIIBO_CHAR_ID_ONE(0x019A, "Chip"),
	AMIIBO_CHAR_ID_ONE(0x019B, "Nat"),
	AMIIBO_CHAR_ID_ONE(0x019C, "Phineas"),
	AMIIBO_CHAR_ID_ONE(0x019D, "Copper"),
	AMIIBO_CHAR_ID_ONE(0x019E, "Booker"),
	AMIIBO_CHAR_ID_ONE(0x019F, "Pete"),
	AMIIBO_CHAR_ID_ONE(0x01A0, "Pelly"),
	AMIIBO_CHAR_ID_ONE(0x01A1, "Phyllis"),
	AMIIBO_CHAR_ID_ONE(0x01A2, "Gulliver"),
	AMIIBO_CHAR_ID_ONE(0x01A3, "Joan"),
	AMIIBO_CHAR_ID_ONE(0x01A4, "Pascal"),
	AMIIBO_CHAR_ID_ONE(0x01A5, "Katrina"),
	AMIIBO_CHAR_ID_ONE(0x01A6, "Sahara"),
	AMIIBO_CHAR_ID_ONE(0x01A7, "Wendell"),
	AMIIBO_CHAR_ID_VAR(0x01A8, "Redd", ac_redd_variants),
	AMIIBO_CHAR_ID_ONE(0x01A9, "Gracie"),
	AMIIBO_CHAR_ID_ONE(0x01AA, "Lyle"),
	AMIIBO_CHAR_ID_ONE(0x01AB, "Pave"),
	AMIIBO_CHAR_ID_ONE(0x01AC, "Zipper"),
	AMIIBO_CHAR_ID_ONE(0x01AD, "Jack"),
	AMIIBO_CHAR_ID_ONE(0x01AE, "Franklin"),
	AMIIBO_CHAR_ID_ONE(0x01AF, "Jingle"),
	AMIIBO_CHAR_ID_ONE(0x01B0, "Tortimer"),
	AMIIBO_CHAR_ID_VAR(0x01B1, "Dr. Shrunk", ac_dr_shrunk_variants),
	AMIIBO_CHAR_ID_ONE(0x01B3, "Blanca"),
	AMIIBO_CHAR_ID_ONE(0x01B4, "Leif"),
	AMIIBO_CHAR_ID_ONE(0x01B5, "Luna"),
	AMIIBO_CHAR_ID_ONE(0x01B5, "Luna"),
	AMIIBO_CHAR_ID_ONE(0x01B6, "Katie"),
	AMIIBO_CHAR_ID_VAR(0x01C1, "Lottie", ac_lottie_variants),
	AMIIBO_CHAR_ID_ONE(0x0200, "Cyrano"),
	AMIIBO_CHAR_ID_ONE(0x0201, "Antonio"),
	AMIIBO_CHAR_ID_ONE(0x0202, "Pango"),
	AMIIBO_CHAR_ID_ONE(0x0203, "Anabelle"),
	AMIIBO_CHAR_ID_ONE(0x0206, "Snooty"),
	AMIIBO_CHAR_ID_ONE(0x0208, "Annalisa"),
	AMIIBO_CHAR_ID_ONE(0x0209, "Olaf"),
	AMIIBO_CHAR_ID_ONE(0x0214, "Teddy"),
	AMIIBO_CHAR_ID_ONE(0x0215, "Pinky"),
	AMIIBO_CHAR_ID_ONE(0x0216, "Curt"),
	AMIIBO_CHAR_ID_ONE(0x0217, "Chow"),
	AMIIBO_CHAR_ID_ONE(0x0219, "Nate"),
	AMIIBO_CHAR_ID_ONE(0x021A, "Groucho"),
	AMIIBO_CHAR_ID_ONE(0x021B, "Tutu"),
	AMIIBO_CHAR_ID_ONE(0x021C, "Ursala"),
	AMIIBO_CHAR_ID_ONE(0x021D, "Grizzly"),
	AMIIBO_CHAR_ID_ONE(0x021E, "Paula"),
	AMIIBO_CHAR_ID_ONE(0x021F, "Ike"),
	AMIIBO_CHAR_ID_ONE(0x0220, "Charlise"),
	AMIIBO_CHAR_ID_ONE(0x0221, "Beardo"),
	AMIIBO_CHAR_ID_ONE(0x0222, "Klaus"),
	AMIIBO_CHAR_ID_ONE(0x022D, "Jay"),
	AMIIBO_CHAR_ID_ONE(0x022E, "Robin"),
	AMIIBO_CHAR_ID_ONE(0x022F, "Anchovy"),
	AMIIBO_CHAR_ID_ONE(0x0230, "Twiggy"),
	AMIIBO_CHAR_ID_ONE(0x0231, "Jitters"),
	AMIIBO_CHAR_ID_ONE(0x0232, "Piper"),
	AMIIBO_CHAR_ID_ONE(0x0233, "Admiral"),
	AMIIBO_CHAR_ID_ONE(0x0235, "Midge"),
	AMIIBO_CHAR_ID_ONE(0x0238, "Jacob"),
	AMIIBO_CHAR_ID_ONE(0x023C, "Lucha"),
	AMIIBO_CHAR_ID_ONE(0x023D, "Jacques"),
	AMIIBO_CHAR_ID_ONE(0x023E, "Peck"),
	AMIIBO_CHAR_ID_ONE(0x023F, "Sparro"),
	AMIIBO_CHAR_ID_ONE(0x024A, "Angus"),
	AMIIBO_CHAR_ID_ONE(0x024B, "Rodeo"),
	AMIIBO_CHAR_ID_ONE(0x024D, "Stu"),
	AMIIBO_CHAR_ID_ONE(0x024F, "T-Bone"),
	AMIIBO_CHAR_ID_ONE(0x024F, "T-Bone"),
	AMIIBO_CHAR_ID_ONE(0x0251, "Coach"),
	AMIIBO_CHAR_ID_ONE(0x0252, "Vic"),
	AMIIBO_CHAR_ID_ONE(0x025D, "Bob"),
	AMIIBO_CHAR_ID_ONE(0x025E, "Mitzi"),
	AMIIBO_CHAR_ID_ONE(0x025F, "Rosie"),	// amiibo Festival variant is in Page 22, amiibo series.
	AMIIBO_CHAR_ID_ONE(0x0260, "Olivia"),
	AMIIBO_CHAR_ID_ONE(0x0261, "Kiki"),
	AMIIBO_CHAR_ID_ONE(0x0262, "Tangy"),
	AMIIBO_CHAR_ID_ONE(0x0263, "Punchy"),
	AMIIBO_CHAR_ID_ONE(0x0264, "Purrl"),
	AMIIBO_CHAR_ID_ONE(0x0265, "Moe"),
	AMIIBO_CHAR_ID_ONE(0x0266, "Kabuki"),
	AMIIBO_CHAR_ID_ONE(0x0267, "Kid Cat"),
	AMIIBO_CHAR_ID_ONE(0x0268, "Monique"),
	AMIIBO_CHAR_ID_ONE(0x0269, "Tabby"),
	AMIIBO_CHAR_ID_ONE(0x026A, "Stinky"),
	AMIIBO_CHAR_ID_ONE(0x026B, "Kitty"),
	AMIIBO_CHAR_ID_ONE(0x026C, "Tom"),
	AMIIBO_CHAR_ID_ONE(0x026D, "Merry"),
	AMIIBO_CHAR_ID_ONE(0x026E, "Felicity"),
	AMIIBO_CHAR_ID_ONE(0x026F, "Lolly"),
	AMIIBO_CHAR_ID_ONE(0x0270, "Ankha"),
	AMIIBO_CHAR_ID_ONE(0x0271, "Rudy"),
	AMIIBO_CHAR_ID_ONE(0x0272, "Katt"),
	AMIIBO_CHAR_ID_ONE(0x027D, "Bluebear"),
	AMIIBO_CHAR_ID_ONE(0x027E, "Maple"),
	AMIIBO_CHAR_ID_ONE(0x027F, "Poncho"),
	AMIIBO_CHAR_ID_ONE(0x0280, "Pudge"),
	AMIIBO_CHAR_ID_ONE(0x0281, "Kody"),
	AMIIBO_CHAR_ID_ONE(0x0282, "Stitches"),	// amiibo Festival variant is in Page 22, amiibo series.
	AMIIBO_CHAR_ID_ONE(0x0283, "Vladimir"),
	AMIIBO_CHAR_ID_ONE(0x0284, "Murphy"),
	AMIIBO_CHAR_ID_ONE(0x0286, "Olive"),
	AMIIBO_CHAR_ID_ONE(0x0287, "Cheri"),
	AMIIBO_CHAR_ID_ONE(0x028A, "June"),
	AMIIBO_CHAR_ID_ONE(0x028B, "Pekoe"),
	AMIIBO_CHAR_ID_ONE(0x028C, "Chester"),
	AMIIBO_CHAR_ID_ONE(0x028D, "Barold"),
	AMIIBO_CHAR_ID_ONE(0x028E, "Tammy"),
	AMIIBO_CHAR_ID_VAR(0x028F, "Marty", ac_marty_variants),
	AMIIBO_CHAR_ID_ONE(0x0299, "Goose"),
	AMIIBO_CHAR_ID_ONE(0x029A, "Benedict"),
	AMIIBO_CHAR_ID_ONE(0x029B, "Egbert"),
	AMIIBO_CHAR_ID_ONE(0x029E, "Ava"),
	AMIIBO_CHAR_ID_ONE(0x02A2, "Becky"),
	AMIIBO_CHAR_ID_ONE(0x02A3, "Plucky"),
	AMIIBO_CHAR_ID_ONE(0x02A4, "Knox"),
	AMIIBO_CHAR_ID_ONE(0x02A5, "Broffina"),
	AMIIBO_CHAR_ID_ONE(0x02A6, "Ken"),
	AMIIBO_CHAR_ID_ONE(0x02B1, "Patty"),
	AMIIBO_CHAR_ID_ONE(0x02B2, "Tipper"),
	AMIIBO_CHAR_ID_ONE(0x02B7, "Norma"),
	AMIIBO_CHAR_ID_ONE(0x02B8, "Naomi"),
	AMIIBO_CHAR_ID_ONE(0x02C3, "Alfonso"),
	AMIIBO_CHAR_ID_ONE(0x02C4, "Alli"),
	AMIIBO_CHAR_ID_ONE(0x02C5, "Boots"),
	AMIIBO_CHAR_ID_ONE(0x02C7, "Del"),
	AMIIBO_CHAR_ID_ONE(0x02C9, "Sly"),
	AMIIBO_CHAR_ID_ONE(0x02CA, "Gayle"),
	AMIIBO_CHAR_ID_ONE(0x02CB, "Drago"),
	AMIIBO_CHAR_ID_ONE(0x02D6, "Fauna"),
	AMIIBO_CHAR_ID_ONE(0x02D7, "Bam"),
	AMIIBO_CHAR_ID_ONE(0x02D8, "Zell"),
	AMIIBO_CHAR_ID_ONE(0x02D9, "Bruce"),
	AMIIBO_CHAR_ID_ONE(0x02DA, "Deirdre"),
	AMIIBO_CHAR_ID_ONE(0x02DB, "Lopez"),
	AMIIBO_CHAR_ID_ONE(0x02DC, "Fuchsia"),
	AMIIBO_CHAR_ID_ONE(0x02DD, "Beau"),
	AMIIBO_CHAR_ID_ONE(0x02DE, "Diana"),
	AMIIBO_CHAR_ID_ONE(0x02DF, "Erik"),
	AMIIBO_CHAR_ID_VAR(0x02E0, "Chelsea", ac_chelsea_variants),
	AMIIBO_CHAR_ID_ONE(0x02EA, "Goldie"),	// amiibo Festival variant is in Page 22, amiibo series.
	AMIIBO_CHAR_ID_ONE(0x02EB, "Butch"),
	AMIIBO_CHAR_ID_ONE(0x02EC, "Lucky"),
	AMIIBO_CHAR_ID_ONE(0x02ED, "Biskit"),
	AMIIBO_CHAR_ID_ONE(0x02EE, "Bones"),
	AMIIBO_CHAR_ID_ONE(0x02EF, "Portia"),
	AMIIBO_CHAR_ID_ONE(0x02F0, "Joan"),
	AMIIBO_CHAR_ID_ONE(0x02F0, "Walker"),
	AMIIBO_CHAR_ID_ONE(0x02F1, "Daisy"),
	AMIIBO_CHAR_ID_ONE(0x02F2, "Cookie"),
	AMIIBO_CHAR_ID_ONE(0x02F3, "Maddie"),
	AMIIBO_CHAR_ID_ONE(0x02F4, "Bea"),
	AMIIBO_CHAR_ID_ONE(0x02F8, "Mac"),
	AMIIBO_CHAR_ID_ONE(0x02F9, "Marcel"),
	AMIIBO_CHAR_ID_ONE(0x02FA, "Benjamin"),
	AMIIBO_CHAR_ID_ONE(0x02FB, "Cherry"),
	AMIIBO_CHAR_ID_ONE(0x02FC, "Shep"),
	AMIIBO_CHAR_ID_ONE(0x0307, "Bill"),
	AMIIBO_CHAR_ID_ONE(0x0307, "Bill"),
	AMIIBO_CHAR_ID_ONE(0x0308, "Joey"),
	AMIIBO_CHAR_ID_ONE(0x0309, "Pate"),
	AMIIBO_CHAR_ID_ONE(0x030A, "Maelle"),
	AMIIBO_CHAR_ID_ONE(0x030B, "Deena"),
	AMIIBO_CHAR_ID_ONE(0x030C, "Pompom"),
	AMIIBO_CHAR_ID_ONE(0x030D, "Mallary"),
	AMIIBO_CHAR_ID_ONE(0x030E, "Freckles"),
	AMIIBO_CHAR_ID_ONE(0x030F, "Derwin"),
	AMIIBO_CHAR_ID_ONE(0x0310, "Drake"),
	AMIIBO_CHAR_ID_ONE(0x0311, "Scoot"),
	AMIIBO_CHAR_ID_ONE(0x0312, "Weber"),
	AMIIBO_CHAR_ID_ONE(0x0313, "Miranda"),
	AMIIBO_CHAR_ID_ONE(0x0314, "Ketchup"),
	AMIIBO_CHAR_ID_ONE(0x0316, "Gloria"),
	AMIIBO_CHAR_ID_ONE(0x0317, "Molly"),
	AMIIBO_CHAR_ID_ONE(0x0318, "Quillson"),
	AMIIBO_CHAR_ID_ONE(0x0323, "Opal"),
	AMIIBO_CHAR_ID_ONE(0x0324, "Dizzy"),
	AMIIBO_CHAR_ID_ONE(0x0325, "Big Top"),
	AMIIBO_CHAR_ID_ONE(0x0326, "Eloise"),
	AMIIBO_CHAR_ID_ONE(0x0327, "Margie"),
	AMIIBO_CHAR_ID_ONE(0x0328, "Paolo"),
	AMIIBO_CHAR_ID_ONE(0x0329, "Axel"),
	AMIIBO_CHAR_ID_ONE(0x032A, "Ellie"),
	AMIIBO_CHAR_ID_ONE(0x032C, "Tucker"),
	AMIIBO_CHAR_ID_ONE(0x032D, "Tia"),
	AMIIBO_CHAR_ID_VAR(0x032E, "Chai", ac_chai_variants),
	AMIIBO_CHAR_ID_ONE(0x0338, "Lily"),
	AMIIBO_CHAR_ID_ONE(0x0339, "Ribbot"),
	AMIIBO_CHAR_ID_ONE(0x033A, "Frobert"),
	AMIIBO_CHAR_ID_ONE(0x033B, "Camofrog"),
	AMIIBO_CHAR_ID_ONE(0x033C, "Drift"),
	AMIIBO_CHAR_ID_ONE(0x033D, "Wart Jr."),
	AMIIBO_CHAR_ID_ONE(0x033E, "Puddles"),
	AMIIBO_CHAR_ID_ONE(0x033F, "Jeremiah"),
	AMIIBO_CHAR_ID_ONE(0x0341, "Tad"),
	AMIIBO_CHAR_ID_ONE(0x0342, "Cousteau"),
	AMIIBO_CHAR_ID_ONE(0x0343, "Huck"),
	AMIIBO_CHAR_ID_ONE(0x0344, "Prince"),
	AMIIBO_CHAR_ID_ONE(0x0345, "Jambette"),
	AMIIBO_CHAR_ID_ONE(0x0347, "Raddle"),
	AMIIBO_CHAR_ID_ONE(0x0348, "Gigi"),
	AMIIBO_CHAR_ID_ONE(0x0349, "Croque"),
	AMIIBO_CHAR_ID_ONE(0x034A, "Diva"),
	AMIIBO_CHAR_ID_ONE(0x034B, "Henry"),
	AMIIBO_CHAR_ID_ONE(0x0356, "Chevre"),
	AMIIBO_CHAR_ID_ONE(0x0357, "Nan"),
	AMIIBO_CHAR_ID_ONE(0x0358, "Billy"),
	AMIIBO_CHAR_ID_ONE(0x035A, "Gruff"),
	AMIIBO_CHAR_ID_ONE(0x035C, "Velma"),
	AMIIBO_CHAR_ID_ONE(0x035D, "Kidd"),
	AMIIBO_CHAR_ID_ONE(0x035E, "Pashmina"),
	AMIIBO_CHAR_ID_ONE(0x0369, "Cesar"),
	AMIIBO_CHAR_ID_ONE(0x036A, "Peewee"),
	AMIIBO_CHAR_ID_ONE(0x036B, "Boone"),
	AMIIBO_CHAR_ID_ONE(0x036D, "Louie"),
	AMIIBO_CHAR_ID_ONE(0x036E, "Boyd"),
	AMIIBO_CHAR_ID_ONE(0x0370, "Violet"),
	AMIIBO_CHAR_ID_ONE(0x0371, "Al"),
	AMIIBO_CHAR_ID_ONE(0x0372, "Rocket"),
	AMIIBO_CHAR_ID_ONE(0x0373, "Hans"),
	AMIIBO_CHAR_ID_VAR(0x0374, "Rilla", ac_rilla_variants),
	AMIIBO_CHAR_ID_ONE(0x037E, "Hamlet"),
	AMIIBO_CHAR_ID_ONE(0x037F, "Apple"),
	AMIIBO_CHAR_ID_ONE(0x0380, "Graham"),
	AMIIBO_CHAR_ID_ONE(0x0381, "Rodney"),
	AMIIBO_CHAR_ID_ONE(0x0382, "Soleil"),
	AMIIBO_CHAR_ID_ONE(0x0383, "Clay"),
	AMIIBO_CHAR_ID_ONE(0x0384, "Flurry"),
	AMIIBO_CHAR_ID_ONE(0x0385, "Hamphrey"),
	AMIIBO_CHAR_ID_ONE(0x0390, "Rocco"),
	AMIIBO_CHAR_ID_ONE(0x0392, "Bubbles"),
	AMIIBO_CHAR_ID_ONE(0x0393, "Bertha"),
	AMIIBO_CHAR_ID_ONE(0x0394, "Biff"),
	AMIIBO_CHAR_ID_ONE(0x0395, "Bitty"),
	AMIIBO_CHAR_ID_ONE(0x0398, "Harry"),
	AMIIBO_CHAR_ID_ONE(0x0399, "Hippeux"),
	AMIIBO_CHAR_ID_ONE(0x03A4, "Buck"),
	AMIIBO_CHAR_ID_ONE(0x03A5, "Victoria"),
	AMIIBO_CHAR_ID_ONE(0x03A6, "Savannah"),
	AMIIBO_CHAR_ID_ONE(0x03A7, "Elmer"),
	AMIIBO_CHAR_ID_ONE(0x03A8, "Rosco"),
	AMIIBO_CHAR_ID_ONE(0x03A9, "Winnie"),
	AMIIBO_CHAR_ID_ONE(0x03AA, "Ed"),
	AMIIBO_CHAR_ID_ONE(0x03AB, "Cleo"),
	AMIIBO_CHAR_ID_ONE(0x03AC, "Peaches"),
	AMIIBO_CHAR_ID_ONE(0x03AD, "Annalise"),
	AMIIBO_CHAR_ID_ONE(0x03AE, "Clyde"),
	AMIIBO_CHAR_ID_ONE(0x03AF, "Colton"),
	AMIIBO_CHAR_ID_ONE(0x03B0, "Papi"),
	AMIIBO_CHAR_ID_ONE(0x03B1, "Julian"),
	AMIIBO_CHAR_ID_ONE(0x03BC, "Yuka"),
	AMIIBO_CHAR_ID_ONE(0x03BD, "Alice"),
	AMIIBO_CHAR_ID_ONE(0x03BE, "Melba"),
	AMIIBO_CHAR_ID_ONE(0x03BF, "Sydney"),
	AMIIBO_CHAR_ID_ONE(0x03C0, "Gonzo"),
	AMIIBO_CHAR_ID_ONE(0x03C1, "Ozzie"),
	AMIIBO_CHAR_ID_ONE(0x03C4, "Canberra"),
	AMIIBO_CHAR_ID_ONE(0x03C5, "Lyman"),
	AMIIBO_CHAR_ID_ONE(0x03C6, "Eugene"),
	AMIIBO_CHAR_ID_ONE(0x03D1, "Kitt"),
	AMIIBO_CHAR_ID_ONE(0x03D2, "Mathilda"),
	AMIIBO_CHAR_ID_ONE(0x03D3, "Carrie"),
	AMIIBO_CHAR_ID_ONE(0x03D6, "Astrid"),
	AMIIBO_CHAR_ID_ONE(0x03D7, "Sylvia"),
	AMIIBO_CHAR_ID_ONE(0x03D9, "Walt"),
	AMIIBO_CHAR_ID_ONE(0x03DA, "Rooney"),
	AMIIBO_CHAR_ID_ONE(0x03DB, "Marcie"),
	AMIIBO_CHAR_ID_ONE(0x03E6, "Bud"),
	AMIIBO_CHAR_ID_ONE(0x03E7, "Elvis"),
	AMIIBO_CHAR_ID_ONE(0x03E8, "Rex"),
	AMIIBO_CHAR_ID_ONE(0x03EA, "Leopold"),
	AMIIBO_CHAR_ID_ONE(0x03EC, "Mott"),
	AMIIBO_CHAR_ID_ONE(0x03ED, "Rory"),
	AMIIBO_CHAR_ID_ONE(0x03EE, "Lionel"),
	AMIIBO_CHAR_ID_ONE(0x03FA, "Nana"),
	AMIIBO_CHAR_ID_ONE(0x03FB, "Simon"),
	AMIIBO_CHAR_ID_ONE(0x03FC, "Tammi"),
	AMIIBO_CHAR_ID_ONE(0x03FD, "Monty"),
	AMIIBO_CHAR_ID_ONE(0x03FE, "Elise"),
	AMIIBO_CHAR_ID_ONE(0x03FF, "Flip"),
	AMIIBO_CHAR_ID_ONE(0x0400, "Shari"),
	AMIIBO_CHAR_ID_ONE(0x0401, "Deli"),
	AMIIBO_CHAR_ID_ONE(0x040C, "Dora"),
	AMIIBO_CHAR_ID_ONE(0x040D, "Limberg"),
	AMIIBO_CHAR_ID_ONE(0x040E, "Bella"),
	AMIIBO_CHAR_ID_ONE(0x040F, "Bree"),
	AMIIBO_CHAR_ID_ONE(0x0410, "Samson"),
	AMIIBO_CHAR_ID_ONE(0x0411, "Rod"),
	AMIIBO_CHAR_ID_ONE(0x0414, "Candi"),
	AMIIBO_CHAR_ID_ONE(0x0415, "Rizzo"),
	AMIIBO_CHAR_ID_ONE(0x0416, "Anicotti"),
	AMIIBO_CHAR_ID_ONE(0x0418, "Broccolo"),
	AMIIBO_CHAR_ID_ONE(0x041A, "Moose"),
	AMIIBO_CHAR_ID_ONE(0x041B, "Bettina"),
	AMIIBO_CHAR_ID_ONE(0x041C, "Greta"),
	AMIIBO_CHAR_ID_ONE(0x041D, "Penelope"),
	AMIIBO_CHAR_ID_ONE(0x041E, "Chadder"),
	AMIIBO_CHAR_ID_ONE(0x0429, "Octavian"),
	AMIIBO_CHAR_ID_ONE(0x042A, "Marina"),
	AMIIBO_CHAR_ID_ONE(0x042B, "Zucker"),
	AMIIBO_CHAR_ID_ONE(0x0436, "Queenie"),
	AMIIBO_CHAR_ID_ONE(0x0437, "Gladys"),
	AMIIBO_CHAR_ID_ONE(0x0438, "Sandy"),
	AMIIBO_CHAR_ID_ONE(0x0439, "Sprocket"),
	AMIIBO_CHAR_ID_ONE(0x043B, "Julia"),
	AMIIBO_CHAR_ID_ONE(0x043C, "Cranston"),
	AMIIBO_CHAR_ID_ONE(0x043D, "Phil"),
	AMIIBO_CHAR_ID_ONE(0x043E, "Blanche"),
	AMIIBO_CHAR_ID_ONE(0x043F, "Flora"),
	AMIIBO_CHAR_ID_ONE(0x0440, "Phoebe"),
	AMIIBO_CHAR_ID_ONE(0x044B, "Apollo"),
	AMIIBO_CHAR_ID_ONE(0x044C, "Amelia"),
	AMIIBO_CHAR_ID_ONE(0x044D, "Pierce"),
	AMIIBO_CHAR_ID_ONE(0x044E, "Buzz"),
	AMIIBO_CHAR_ID_ONE(0x0450, "Avery"),
	AMIIBO_CHAR_ID_ONE(0x0451, "Frank"),
	AMIIBO_CHAR_ID_ONE(0x0452, "Sterling"),
	AMIIBO_CHAR_ID_ONE(0x0453, "Keaton"),
	AMIIBO_CHAR_ID_ONE(0x0454, "Celia"),
	AMIIBO_CHAR_ID_ONE(0x045F, "Aurora"),
	AMIIBO_CHAR_ID_ONE(0x0460, "Joan"),
	AMIIBO_CHAR_ID_ONE(0x0460, "Roald"),
	AMIIBO_CHAR_ID_ONE(0x0461, "Cube"),
	AMIIBO_CHAR_ID_ONE(0x0462, "Hopper"),
	AMIIBO_CHAR_ID_ONE(0x0463, "Friga"),
	AMIIBO_CHAR_ID_ONE(0x0464, "Gwen"),
	AMIIBO_CHAR_ID_ONE(0x0465, "Puck"),
	AMIIBO_CHAR_ID_ONE(0x0468, "Wade"),
	AMIIBO_CHAR_ID_ONE(0x0469, "Boomer"),
	AMIIBO_CHAR_ID_ONE(0x046A, "Iggly"),
	AMIIBO_CHAR_ID_ONE(0x046B, "Tex"),
	AMIIBO_CHAR_ID_ONE(0x046C, "Flo"),
	AMIIBO_CHAR_ID_ONE(0x046D, "Sprinkle"),
	AMIIBO_CHAR_ID_ONE(0x0478, "Curly"),
	AMIIBO_CHAR_ID_ONE(0x0479, "Truffles"),
	AMIIBO_CHAR_ID_ONE(0x047A, "Rasher"),
	AMIIBO_CHAR_ID_ONE(0x047B, "Hugh"),
	AMIIBO_CHAR_ID_ONE(0x047C, "Lucy"),
	AMIIBO_CHAR_ID_ONE(0x047D, "Spork/Crackle"),
	AMIIBO_CHAR_ID_ONE(0x0480, "Cobb"),
	AMIIBO_CHAR_ID_ONE(0x0481, "Boris"),
	AMIIBO_CHAR_ID_ONE(0x0482, "Maggie"),
	AMIIBO_CHAR_ID_ONE(0x0483, "Peggy"),
	AMIIBO_CHAR_ID_ONE(0x0485, "Gala"),
	AMIIBO_CHAR_ID_ONE(0x0486, "Chops"),
	AMIIBO_CHAR_ID_ONE(0x0487, "Kevin"),
	AMIIBO_CHAR_ID_ONE(0x0488, "Pancetti"),
	AMIIBO_CHAR_ID_ONE(0x0489, "Agnes"),
	AMIIBO_CHAR_ID_ONE(0x0494, "Bunnie"),
	AMIIBO_CHAR_ID_ONE(0x0495, "Dotty"),
	AMIIBO_CHAR_ID_ONE(0x0496, "Coco"),
	AMIIBO_CHAR_ID_ONE(0x0497, "Snake"),
	AMIIBO_CHAR_ID_ONE(0x0498, "Gaston"),
	AMIIBO_CHAR_ID_ONE(0x0499, "Gabi"),
	AMIIBO_CHAR_ID_ONE(0x049A, "Pippy"),
	AMIIBO_CHAR_ID_ONE(0x049B, "Tiffany"),
	AMIIBO_CHAR_ID_ONE(0x049C, "Genji"),
	AMIIBO_CHAR_ID_ONE(0x049D, "Ruby"),
	AMIIBO_CHAR_ID_ONE(0x049E, "Doc"),
	AMIIBO_CHAR_ID_ONE(0x049F, "Claude"),
	AMIIBO_CHAR_ID_ONE(0x04A0, "Francine"),
	AMIIBO_CHAR_ID_ONE(0x04A1, "Chrissy"),
	AMIIBO_CHAR_ID_ONE(0x04A2, "Hopkins"),
	AMIIBO_CHAR_ID_ONE(0x04A3, "OHare"),
	AMIIBO_CHAR_ID_ONE(0x04A4, "Carmen"),
	AMIIBO_CHAR_ID_ONE(0x04A5, "Bonbon"),
	AMIIBO_CHAR_ID_ONE(0x04A6, "Cole"),
	AMIIBO_CHAR_ID_ONE(0x04A7, "Mira"),
	AMIIBO_CHAR_ID_VAR(0x04A8, "Toby", ac_toby_variants),
	AMIIBO_CHAR_ID_ONE(0x04B2, "Tank"),
	AMIIBO_CHAR_ID_ONE(0x04B3, "Rhonda"),
	AMIIBO_CHAR_ID_ONE(0x04B4, "Spike"),
	AMIIBO_CHAR_ID_ONE(0x04B6, "Hornsby"),
	AMIIBO_CHAR_ID_ONE(0x04B9, "Merengue"),
	// FIXME: MSVC 2010 interprets \xA9e as 2718 because
	// it's too dumb to realize \x takes *two* nybbles.
	AMIIBO_CHAR_ID_ONE(0x04BA, "Ren\303\251e"),
	AMIIBO_CHAR_ID_ONE(0x04C5, "Vesta"),
	AMIIBO_CHAR_ID_ONE(0x04C6, "Baabara"),
	AMIIBO_CHAR_ID_ONE(0x04C7, "Eunice"),
	AMIIBO_CHAR_ID_ONE(0x04C8, "Stella"),
	AMIIBO_CHAR_ID_ONE(0x04C9, "Cashmere"),
	AMIIBO_CHAR_ID_ONE(0x04CC, "Willow"),
	AMIIBO_CHAR_ID_ONE(0x04CD, "Curlos"),
	AMIIBO_CHAR_ID_ONE(0x04CE, "Wendy"),
	AMIIBO_CHAR_ID_ONE(0x04CF, "Timbra"),
	AMIIBO_CHAR_ID_ONE(0x04D0, "Frita"),
	AMIIBO_CHAR_ID_ONE(0x04D1, "Muffy"),
	AMIIBO_CHAR_ID_ONE(0x04D2, "Pietro"),
	AMIIBO_CHAR_ID_VAR(0x04D3, "\xC3\x89toile", ac_etoile_variants),
	AMIIBO_CHAR_ID_ONE(0x04DD, "Peanut"),
	AMIIBO_CHAR_ID_ONE(0x04DE, "Blaire"),
	AMIIBO_CHAR_ID_ONE(0x04DF, "Filbert"),
	AMIIBO_CHAR_ID_ONE(0x04E0, "Pecan"),
	AMIIBO_CHAR_ID_ONE(0x04E1, "Nibbles"),
	AMIIBO_CHAR_ID_ONE(0x04E2, "Agent S"),
	AMIIBO_CHAR_ID_ONE(0x04E3, "Caroline"),
	AMIIBO_CHAR_ID_ONE(0x04E4, "Sally"),
	AMIIBO_CHAR_ID_ONE(0x04E5, "Static"),
	AMIIBO_CHAR_ID_ONE(0x04E6, "Mint"),
	AMIIBO_CHAR_ID_ONE(0x04E7, "Ricky"),
	AMIIBO_CHAR_ID_ONE(0x04E8, "Cally"),
	AMIIBO_CHAR_ID_ONE(0x04EA, "Tasha"),
	AMIIBO_CHAR_ID_ONE(0x04EB, "Sylvana"),
	AMIIBO_CHAR_ID_ONE(0x04EC, "Poppy"),
	AMIIBO_CHAR_ID_ONE(0x04ED, "Sheldon"),
	AMIIBO_CHAR_ID_ONE(0x04EE, "Marshal"),
	AMIIBO_CHAR_ID_ONE(0x04EF, "Hazel"),
	AMIIBO_CHAR_ID_ONE(0x04FA, "Rolf"),
	AMIIBO_CHAR_ID_ONE(0x04FB, "Rowan"),
	AMIIBO_CHAR_ID_ONE(0x04FC, "Tybalt"),
	AMIIBO_CHAR_ID_ONE(0x04FD, "Bangle"),
	AMIIBO_CHAR_ID_ONE(0x04FE, "Leonardo"),
	AMIIBO_CHAR_ID_ONE(0x04FF, "Claudia"),
	AMIIBO_CHAR_ID_ONE(0x0500, "Bianca"),
	AMIIBO_CHAR_ID_ONE(0x050B, "Chief"),
	AMIIBO_CHAR_ID_ONE(0x050C, "Lobo"),
	AMIIBO_CHAR_ID_ONE(0x050D, "Wolfgang"),
	AMIIBO_CHAR_ID_ONE(0x050E, "Whitney"),
	AMIIBO_CHAR_ID_ONE(0x050F, "Dobie"),
	AMIIBO_CHAR_ID_ONE(0x0510, "Freya"),
	AMIIBO_CHAR_ID_ONE(0x0511, "Fang"),
	AMIIBO_CHAR_ID_ONE(0x0513, "Vivian"),
	AMIIBO_CHAR_ID_ONE(0x0514, "Skye"),
	AMIIBO_CHAR_ID_ONE(0x0515, "Kyle"),

	// Star Fox (character series = 0x058)
	AMIIBO_CHAR_ID_ONE(0x0580, "Fox"),
	AMIIBO_CHAR_ID_ONE(0x0581, "Falco"),

	// Metroid (character series = 0x05C)
	AMIIBO_CHAR_ID_VAR(0x05C0, "Samus", metroid_samus_variants),
	AMIIBO_CHAR_ID_ONE(0x05C1, "Metroid"),

	// F-Zero (character series = 0x060)
	AMIIBO_CHAR_ID_ONE(0x0600, "Captain Falcon"),

	// Pikmin (character series = 0x064)
	AMIIBO_CHAR_ID_VAR(0x0640, "Olimar", pikmin_olimar_variants),
	AMIIBO_CHAR_ID_ONE(0x0642, "Pikmin"),

	// Punch-Out!! (character series = 0x06C)
	AMIIBO_CHAR_ID_ONE(0x06C0, "Little Mac"),

	// Wii Fit (character series = 0x070)
	AMIIBO_CHAR_ID_ONE(0x0700, "Wii Fit Trainer"),

	// Kid Icarus (character series = 0x074)
	AMIIBO_CHAR_ID_ONE(0x0740, "Pit"),
	AMIIBO_CHAR_ID_ONE(0x0741, "Dark Pit"),
	AMIIBO_CHAR_ID_ONE(0x0742, "Palutena"),

	// Classic Nintendo (character series = 0x078)
	AMIIBO_CHAR_ID_ONE(0x0780, "Mr. Game & Watch"),
	AMIIBO_CHAR_ID_ONE(0x0781, "R.O.B."),	// NES/Famicom variant is in Page 22, amiibo series.
	AMIIBO_CHAR_ID_ONE(0x0782, "Duck Hunt"),

	// Mii (character series = 0x07C)
	AMIIBO_CHAR_ID_VAR(0x07C0, "Mii Brawler", mii_variants),

	// Splatoon (character series = 0x080)
	AMIIBO_CHAR_ID_VAR(0x0800, "Inkling", splatoon_inkling_variants),
	AMIIBO_CHAR_ID_ONE(0x0801, "Callie"),
	AMIIBO_CHAR_ID_ONE(0x0802, "Marie"),

	// Mario Sports Superstars (character series = 0x09C)
	AMIIBO_CHAR_ID_VAR(0x09C0, "Mario", mss_mario_variants),
	AMIIBO_CHAR_ID_VAR(0x09C1, "Luigi", mss_luigi_variants),
	AMIIBO_CHAR_ID_VAR(0x09C2, "Peach", mss_peach_variants),
	AMIIBO_CHAR_ID_VAR(0x09C3, "Daisy", mss_daisy_variants),
	AMIIBO_CHAR_ID_VAR(0x09C4, "Yoshi", mss_yoshi_variants),
	AMIIBO_CHAR_ID_VAR(0x09C5, "Wario", mss_wario_variants),
	AMIIBO_CHAR_ID_VAR(0x09C6, "Waluigi", mss_waluigi_variants),
	AMIIBO_CHAR_ID_VAR(0x09C7, "Donkey Kong", mss_donkey_kong_variants),
	AMIIBO_CHAR_ID_VAR(0x09C8, "Diddy Kong", mss_diddy_kong_variants),
	AMIIBO_CHAR_ID_VAR(0x09C9, "Bowser", mss_bowser_variants),
	AMIIBO_CHAR_ID_VAR(0x09CA, "Bowser Jr.", mss_bowser_jr_variants),
	AMIIBO_CHAR_ID_VAR(0x09CB, "Boo", mss_boo_variants),
	AMIIBO_CHAR_ID_VAR(0x09CC, "Baby Mario", mss_baby_mario_variants),
	AMIIBO_CHAR_ID_VAR(0x09CD, "Baby Luigi", mss_baby_luigi_variants),
	AMIIBO_CHAR_ID_VAR(0x09CE, "Birdo", mss_birdo_variants),
	AMIIBO_CHAR_ID_VAR(0x09CF, "Rosalina", mss_rosalina_variants),
	AMIIBO_CHAR_ID_VAR(0x09D0, "Metal Mario", mss_metal_mario_variants),
	AMIIBO_CHAR_ID_VAR(0x09D1, "Pink Gold Peach", mss_pink_gold_peach_variants),

	// Pokémon (character series = 0x190 - 0x1BC)
	AMIIBO_CHAR_ID_ONE(0x1900+  6, "Charizard"),
	AMIIBO_CHAR_ID_ONE(0x1900+ 25, "Pikachu"),
	AMIIBO_CHAR_ID_ONE(0x1900+ 39, "Jigglypuff"),
	AMIIBO_CHAR_ID_ONE(0x1900+150, "Mewtwo"),
	AMIIBO_CHAR_ID_ONE(0x1900+448, "Lucario"),
	AMIIBO_CHAR_ID_ONE(0x1900+658, "Greninja"),

	// Pokkén Tournament (character series = 0x1D00)
	AMIIBO_CHAR_ID_ONE(0x1D00, "Shadow Mewtwo"),

	// Kirby (character series = 0x1F0)
	AMIIBO_CHAR_ID_ONE(0x1F00, "Kirby"),
	AMIIBO_CHAR_ID_ONE(0x1F01, "Meta Knight"),
	AMIIBO_CHAR_ID_ONE(0x1F02, "King Dedede"),
	AMIIBO_CHAR_ID_ONE(0x1F03, "Waddle Dee"),

	// BoxBoy! (character series = 0x1F4)
	AMIIBO_CHAR_ID_ONE(0x1F40, "Qbby"),

	// Fire Emblem (character series = 0x210)
	AMIIBO_CHAR_ID_ONE(0x2100, "Marth"),
	AMIIBO_CHAR_ID_ONE(0x2101, "Ike"),
	AMIIBO_CHAR_ID_ONE(0x2102, "Lucina"),
	AMIIBO_CHAR_ID_ONE(0x2103, "Robin"),
	AMIIBO_CHAR_ID_ONE(0x2104, "Roy"),
	AMIIBO_CHAR_ID_VAR(0x2105, "Corrin", fe_corrin_variants),
	AMIIBO_CHAR_ID_ONE(0x2106, "Alm"),
	AMIIBO_CHAR_ID_ONE(0x2107, "Celica"),
	AMIIBO_CHAR_ID_ONE(0x2108, "Chrom"),
	AMIIBO_CHAR_ID_ONE(0x2109, "Tiki"),

	// Xenoblade (character series = 0x224)
	AMIIBO_CHAR_ID_ONE(0x2240, "Shulk"),

	// Earthbound (character series = 0x228)
	AMIIBO_CHAR_ID_ONE(0x2280, "Ness"),
	AMIIBO_CHAR_ID_ONE(0x2281, "Lucas"),

	// Chibi-Robo! (character series = 0x22C)
	AMIIBO_CHAR_ID_ONE(0x22C0, "Chibi Robo"),

	// Sonic the Hedgehog (character series = 0x320)
	AMIIBO_CHAR_ID_ONE(0x3200, "Sonic"),

	// Bayonetta (character series = 0x324)
	AMIIBO_CHAR_ID_ONE(0x3240, "Bayonetta"),

	// Pac-Man (character series = 0x334)
	AMIIBO_CHAR_ID_ONE(0x3340, "Pac-Man"),

	// Mega Man (character series = 0x348)
	AMIIBO_CHAR_ID_ONE(0x3480, "Mega Man"),

	// Street Fighter (character series = 0x34C)
	AMIIBO_CHAR_ID_ONE(0x34C0, "Ryu"),

	// Monster Hunter (character series = 0x350)
	AMIIBO_CHAR_ID_VAR(0x3500, "One-Eyed Rathalos and Rider", mh_rathalos_variants),
	AMIIBO_CHAR_ID_ONE(0x3501, "Nabiru"),
	AMIIBO_CHAR_ID_VAR(0x3502, "Rathian and Cheval", mh_rathian_cheval_variants),
	AMIIBO_CHAR_ID_VAR(0x3503, "Barioth and Ayuria", mh_barioth_ayuria_variants),
	AMIIBO_CHAR_ID_VAR(0x3504, "Qurupeco and Dan", mh_qurupeco_dan_variants),

	// Shovel Knight (character series = 0x35C)
	AMIIBO_CHAR_ID_ONE(0x35C0, "Shovel Knight"),

	// Final Fantasy (character series = 0x360)
	AMIIBO_CHAR_ID_ONE(0x3600, "Cloud"),

	// Cereal (character series = 0x374)
	AMIIBO_CHAR_ID_VAR(0x3740, "Super Mario Cereal", cereal_smb_variants),
};

/**
 * char_id_t bsearch() comparison function.
 * @param a
 * @param b
 * @return
 */
int RP_C_API AmiiboDataPrivate::char_id_t_compar(const void *a, const void *b)
{
	uint16_t id1 = static_cast<const char_id_t*>(a)->char_id;
	uint16_t id2 = static_cast<const char_id_t*>(b)->char_id;
	if (id1 < id2) return -1;
	if (id1 > id2) return 1;
	return 0;
}

/** Page 22 (byte 0x5C): amiibo series **/

// amiibo series names.
// Array index = SS
const char *const AmiiboDataPrivate::amiibo_series_names[] = {
	"Super Smash Bros.",			// 0x00
	"Super Mario Bros.",			// 0x01
	"Chibi Robo!",				// 0x02
	"Yarn Yoshi",				// 0x03
	"Splatoon",				// 0x04
	"Animal Crossing",			// 0x05
	"Super Mario Bros. 30th Anniversary",	// 0x06
	"Skylanders",				// 0x07
	nullptr,				// 0x08
	"The Legend of Zelda",			// 0x09
	"Shovel Knight",			// 0x0A
	nullptr,				// 0x0B
	"Kirby",				// 0x0C
	"Pokk\xC3\xA9n Tournament",		// 0x0D
	nullptr,				// 0x0E
	"Monster Hunter",			// 0x0F
	"BoxBoy!",				// 0x10
	"Pikmin",				// 0x11
	"Fire Emblem",				// 0x12
	"Metroid",				// 0x13
	"Cereal",				// 0x14
};

// amiibo IDs.
// Index is the amiibo ID. (aaaa)
// NOTE: amiibo ID is unique across *all* amiibo,
// so we can use a single array for all series.
const AmiiboDataPrivate::amiibo_id_t AmiiboDataPrivate::amiibo_ids[] = {
	// SSB: Wave 1 [0x0000-0x000B]
	{  1, 1, "Mario"},		// 0x0000
	{  2, 1, "Peach"},		// 0x0001
	{  3, 1, "Yoshi"},		// 0x0002
	{  4, 1, "Donkey Kong"},	// 0x0003
	{  5, 1, "Link"},		// 0x0004
	{  6, 1, "Fox"},		// 0x0005
	{  7, 1, "Samus"},		// 0x0006
	{  8, 1, "Wii Fit Trainer"},	// 0x0007
	{  9, 1, "Villager"},		// 0x0008
	{ 10, 1, "Pikachu"},		// 0x0009
	{ 11, 1, "Kirby"},		// 0x000A
	{ 12, 1, "Marth"},		// 0x000B

	// SSB: Wave 2 [0x000C-0x0012]
	{ 15, 2, "Luigi"},		// 0x000C
	{ 14, 2, "Diddy Kong"},		// 0x000D
	{ 13, 2, "Zelda"},		// 0x000E
	{ 16, 2, "Little Mac"},		// 0x000F
	{ 17, 2, "Pit"},		// 0x0010
	{ 21, 3, "Lucario"},		// 0x0011 (Wave 3, out of order)
	{ 18, 2, "Captain Falcon"},	// 0x0012

	// Waves 3+ [0x0013-0x0033]
	{ 19, 3, "Rosalina & Luma"},	// 0x0013
	{ 20, 3, "Bowser"},		// 0x0014
	{ 43, 6, "Bowser Jr."},		// 0x0015
	{ 22, 3, "Toon Link"},		// 0x0016
	{ 23, 3, "Sheik"},		// 0x0017
	{ 24, 3, "Ike"},		// 0x0018
	{ 42, 6, "Dr. Mario"},		// 0x0019
	{ 32, 4, "Wario"},		// 0x001A
	{ 41, 6, "Ganondorf"},		// 0x001B
	{ 52, 7, "Falco"},		// 0x001C
	{ 40, 6, "Zero Suit Samus"},	// 0x001D
	{ 44, 6, "Olimar"},		// 0x001E
	{ 38, 5, "Palutena"},		// 0x001F
	{ 39, 5, "Dark Pit"},		// 0x0020
	{ 48, 7, "Mii Brawler"},	// 0x0021
	{ 49, 7, "Mii Swordfighter"},	// 0x0022
	{ 50, 7, "Mii Gunner"},		// 0x0023
	{ 33, 4, "Charizard"},		// 0x0024
	{ 36, 4, "Greninja"},		// 0x0025
	{ 37, 4, "Jigglypuff"},		// 0x0026
	{ 29, 3, "Meta Knight"},	// 0x0027
	{ 28, 3, "King Dedede"},	// 0x0028
	{ 31, 4, "Lucina"},		// 0x0029
	{ 30, 4, "Robin"},		// 0x002A
	{ 25, 3, "Shulk"},		// 0x002B
	{ 34, 4, "Ness"},		// 0x002C
	{ 45, 6, "Mr. Game & Watch"},	// 0x002D
	{ 54, 9, "R.O.B. (Famicom)"},	// 0x002E (FIXME: Localized release numbers.)
	{ 47, 6, "Duck Hunt"},		// 0x002F
	{ 26, 3, "Sonic"},		// 0x0030
	{ 27, 3, "Mega Man"},		// 0x0031
	{ 35, 4, "Pac-Man"},		// 0x0032
	{ 46, 6, "R.O.B. (NES)"},	// 0x0033 (FIXME: Localized release numbers.)

	// SMB: Wave 1 [0x0034-0x0039]
	{  1, 1, "Mario"},		// 0x0034
	{  4, 1, "Luigi"},		// 0x0035
	{  2, 1, "Peach"},		// 0x0036
	{  5, 1, "Yoshi"},		// 0x0037
	{  3, 1, "Toad"},		// 0x0038
	{  6, 1, "Bowser"},		// 0x0039

	// Chibi-Robo!
	{  0, 0, "Chibi Robo"},		// 0x003A

	// Unused [0x003B]
	{  0, 0, nullptr},		// 0x003B

	// SMB: Wave 1: Special Editions [0x003C-0x003D]
	{  7, 1, "Mario (Gold Edition)"},	// 0x003C
	{  8, 1, "Mario (Silver Edition)"},	// 0x003D

	// Splatoon: Wave 1 [0x003E-0x0040]
	{  0, 1, "Inkling Girl"},	// 0x003E
	{  0, 1, "Inkling Boy"},	// 0x003F
	{  0, 1, "Inkling Squid"},	// 0x0040

	// Yarn Yoshi [0x0041-0x0043]
	{  1, 0, "Green Yarn Yoshi"},		// 0x0041
	{  2, 0, "Pink Yarn Yoshi"},		// 0x0042
	{  3, 0, "Light Blue Yarn Yoshi"},	// 0x0043

	// Animal Crossing Cards: Series 1 [0x0044-0x00A7]
	{  1, 1, "Isabelle"},		// 0x0044
	{  2, 1, "Tom Nook"},		// 0x0045
	{  3, 1, "DJ KK"},		// 0x0046
	{  4, 1, "Sable"},		// 0x0047
	{  5, 1, "Kapp'n"},		// 0x0048
	{  6, 1, "Resetti"},		// 0x0049
	{  7, 1, "Joan"},		// 0x004A
	{  8, 1, "Timmy"},		// 0x004B
	{  9, 1, "Digby"},		// 0x004C
	{ 10, 1, "Pascal"},		// 0x004D
	{ 11, 1, "Harriet"},		// 0x004E
	{ 12, 1, "Redd"},		// 0x004F
	{ 13, 1, "Sahara"},		// 0x0050
	{ 14, 1, "Luna"},		// 0x0051
	{ 15, 1, "Tortimer"},		// 0x0052
	{ 16, 1, "Lyle"},		// 0x0053
	{ 17, 1, "Lottie"},		// 0x0054
	{ 18, 1, "Bob"},		// 0x0055
	{ 19, 1, "Fauna"},		// 0x0056
	{ 20, 1, "Curt"},		// 0x0057
	{ 21, 1, "Portia"},		// 0x0058
	{ 22, 1, "Leonardo"},		// 0x0059
	{ 23, 1, "Cheri"},		// 0x005A
	{ 24, 1, "Kyle"},		// 0x005B
	{ 25, 1, "Al"},			// 0x005C
	// FIXME: MSVC 2010 interprets \xA9e as 2718 because
	// it's too dumb to realize \x takes *two* nybbles.
	{ 26, 1, "Ren\303\251e"},	// 0x005D
	{ 27, 1, "Lopez"},		// 0x005E
	{ 28, 1, "Jambette"},		// 0x005F
	{ 29, 1, "Rasher"},		// 0x0060
	{ 30, 1, "Tiffany"},		// 0x0061
	{ 31, 1, "Sheldon"},		// 0x0062
	{ 32, 1, "Bluebear"},		// 0x0063
	{ 33, 1, "Bill"},		// 0x0064
	{ 34, 1, "Kiki"},		// 0x0065
	{ 35, 1, "Deli"},		// 0x0066
	{ 36, 1, "Alli"},		// 0x0067
	{ 37, 1, "Kabuki"},		// 0x0068
	{ 38, 1, "Patty"},		// 0x0069
	{ 39, 1, "Jitters"},		// 0x006A
	{ 40, 1, "Gigi"},		// 0x006B
	{ 41, 1, "Quillson"},		// 0x006C
	{ 42, 1, "Marcie"},		// 0x006D
	{ 43, 1, "Puck"},		// 0x006E
	{ 44, 1, "Shari"},		// 0x006F
	{ 45, 1, "Octavian"},		// 0x0070
	{ 46, 1, "Winnie"},		// 0x0071
	{ 47, 1, "Knox"},		// 0x0072
	{ 48, 1, "Sterling"},		// 0x0073
	{ 49, 1, "Bonbon"},		// 0x0074
	{ 50, 1, "Punchy"},		// 0x0075
	{ 51, 1, "Opal"},		// 0x0076
	{ 52, 1, "Poppy"},		// 0x0077
	{ 53, 1, "Limberg"},		// 0x0078
	{ 54, 1, "Deena"},		// 0x0079
	{ 55, 1, "Snake"},		// 0x007A
	{ 56, 1, "Bangle"},		// 0x007B
	{ 57, 1, "Phil"},		// 0x007C
	{ 58, 1, "Monique"},		// 0x007D
	{ 59, 1, "Nate"},		// 0x007E
	{ 60, 1, "Samson"},		// 0x007F
	{ 61, 1, "Tutu"},		// 0x0080
	{ 62, 1, "T-Bone"},		// 0x0081
	{ 63, 1, "Mint"},		// 0x0082
	{ 64, 1, "Pudge"},		// 0x0083
	{ 65, 1, "Midge"},		// 0x0084
	{ 66, 1, "Gruff"},		// 0x0085
	{ 67, 1, "Flurry"},		// 0x0086
	{ 68, 1, "Clyde"},		// 0x0087
	{ 69, 1, "Bella"},		// 0x0088
	{ 70, 1, "Biff"},		// 0x0089
	{ 71, 1, "Yuka"},		// 0x008A
	{ 72, 1, "Lionel"},		// 0x008B
	{ 73, 1, "Flo"},		// 0x008C
	{ 74, 1, "Cobb"},		// 0x008D
	{ 75, 1, "Amelia"},		// 0x008E
	{ 76, 1, "Jeremiah"},		// 0x008F
	{ 77, 1, "Cherry"},		// 0x0090
	{ 78, 1, "Rosco"},		// 0x0091
	{ 79, 1, "Truffles"},		// 0x0092
	{ 80, 1, "Eugene"},		// 0x0093
	{ 81, 1, "Eunice"},		// 0x0094
	{ 82, 1, "Goose"},		// 0x0095
	{ 83, 1, "Annalisa"},		// 0x0096
	{ 84, 1, "Benjamin"},		// 0x0097
	{ 85, 1, "Pancetti"},		// 0x0098
	{ 86, 1, "Chief"},		// 0x0099
	{ 87, 1, "Bunnie"},		// 0x009A
	{ 88, 1, "Clay"},		// 0x009B
	{ 89, 1, "Diana"},		// 0x009C
	{ 90, 1, "Axel"},		// 0x009D
	{ 91, 1, "Muffy"},		// 0x009E
	{ 92, 1, "Henry"},		// 0x009F
	{ 93, 1, "Bertha"},		// 0x00A0
	{ 94, 1, "Cyrano"},		// 0x00A1
	{ 95, 1, "Peanut"},		// 0x00A2
	{ 96, 1, "Cole"},		// 0x00A3
	{ 97, 1, "Willow"},		// 0x00A4
	{ 98, 1, "Roald"},		// 0x00A5
	{ 99, 1, "Molly"},		// 0x00A6
	{100, 1, "Walker"},		// 0x00A7

	// Animal Crossing Cards: Series 2 [0x00A8-0x010B]
	{101, 2, "K.K. Slider"},	// 0x00A8
	{102, 2, "Reese"},		// 0x00A9
	{103, 2, "Kicks"},		// 0x00AA
	{104, 2, "Labelle"},		// 0x00AB
	{105, 2, "Copper"},		// 0x00AC
	{106, 2, "Booker"},		// 0x00AD
	{107, 2, "Katie"},		// 0x00AE
	{108, 2, "Tommy"},		// 0x00AF
	{109, 2, "Porter"},		// 0x00B0
	{110, 2, "Lelia"},		// 0x00B1
	{111, 2, "Dr. Shrunk"},		// 0x00B2
	{112, 2, "Don Resetti"},	// 0x00B3
	{113, 2, "Isabelle (Autumn Outfit)"},// 0x00B4
	{114, 2, "Blanca"},		// 0x00B5
	{115, 2, "Nat"},		// 0x00B6
	{116, 2, "Chip"},		// 0x00B7
	{117, 2, "Jack"},		// 0x00B8
	{118, 2, "Poncho"},		// 0x00B9
	{119, 2, "Felicity"},		// 0x00BA
	{120, 2, "Ozzie"},		// 0x00BB
	{121, 2, "Tia"},		// 0x00BC
	{122, 2, "Lucha"},		// 0x00BD
	{123, 2, "Fuchsia"},		// 0x00BE
	{124, 2, "Harry"},		// 0x00BF
	{125, 2, "Gwen"},		// 0x00C0
	{126, 2, "Coach"},		// 0x00C1
	{127, 2, "Kitt"},		// 0x00C2
	{128, 2, "Tom"},		// 0x00C3
	{129, 2, "Tipper"},		// 0x00C4
	{130, 2, "Prince"},		// 0x00C5
	{131, 2, "Pate"},		// 0x00C6
	{132, 2, "Vladimir"},		// 0x00C7
	{133, 2, "Savannah"},		// 0x00C8
	{134, 2, "Kidd"},		// 0x00C9
	{135, 2, "Phoebe"},		// 0x00CA
	{136, 2, "Egbert"},		// 0x00CB
	{137, 2, "Cookie"},		// 0x00CC
	{138, 2, "Sly"},		// 0x00CD
	{139, 2, "Blaire"},		// 0x00CE
	{140, 2, "Avery"},		// 0x00CF
	{141, 2, "Nana"},		// 0x00D0
	{142, 2, "Peck"},		// 0x00D1
	{143, 2, "Olivia"},		// 0x00D2
	{144, 2, "Cesar"},		// 0x00D3
	{145, 2, "Carmen"},		// 0x00D4
	{146, 2, "Rodney"},		// 0x00D5
	{147, 2, "Scoot"},		// 0x00D6
	{148, 2, "Whitney"},		// 0x00D7
	{149, 2, "Broccolo"},		// 0x00D8
	{150, 2, "Coco"},		// 0x00D9
	{151, 2, "Groucho"},		// 0x00DA
	{152, 2, "Wendy"},		// 0x00DB
	{153, 2, "Alfonso"},		// 0x00DC
	{154, 2, "Rhonda"},		// 0x00DD
	{155, 2, "Butch"},		// 0x00DE
	{156, 2, "Gabi"},		// 0x00DF
	{157, 2, "Moose"},		// 0x00E0
	{158, 2, "Timbra"},		// 0x00E1
	{159, 2, "Zell"},		// 0x00E2
	{160, 2, "Pekoe"},		// 0x00E3
	{161, 2, "Teddy"},		// 0x00E4
	{162, 2, "Mathilda"},		// 0x00E5
	{163, 2, "Ed"},			// 0x00E6
	{164, 2, "Bianca"},		// 0x00E7
	{165, 2, "Filbert"},		// 0x00E8
	{166, 2, "Kitty"},		// 0x00E9
	{167, 2, "Beau"},		// 0x00EA
	{168, 2, "Nan"},		// 0x00EB
	{169, 2, "Bud"},		// 0x00EC
	{170, 2, "Ruby"},		// 0x00ED
	{171, 2, "Benedict"},		// 0x00EE
	{172, 2, "Agnes"},		// 0x00EF
	{173, 2, "Julian"},		// 0x00F0
	{174, 2, "Bettina"},		// 0x00F1
	{175, 2, "Jay"},		// 0x00F2
	{176, 2, "Sprinkle"},		// 0x00F3
	{177, 2, "Flip"},		// 0x00F4
	{178, 2, "Hugh"},		// 0x00F5
	{179, 2, "Hopper"},		// 0x00F6
	{180, 2, "Pecan"},		// 0x00F7
	{181, 2, "Drake"},		// 0x00F8
	{182, 2, "Alice"},		// 0x00F9
	{183, 2, "Camofrog"},		// 0x00FA
	{184, 2, "Anicotti"},		// 0x00FB
	{185, 2, "Chops"},		// 0x00FC
	{186, 2, "Charlise"},		// 0x00FD
	{187, 2, "Vic"},		// 0x00FE
	{188, 2, "Ankha"},		// 0x00FF
	{189, 2, "Drift"},		// 0x0100
	{190, 2, "Vesta"},		// 0x0101
	{191, 2, "Marcel"},		// 0x0102
	{192, 2, "Pango"},		// 0x0103
	{193, 2, "Keaton"},		// 0x0104
	{194, 2, "Gladys"},		// 0x0105
	{195, 2, "Hamphrey"},		// 0x0106
	{196, 2, "Freya"},		// 0x0107
	{197, 2, "Kid Cat"},		// 0x0108
	{198, 2, "Agent S"},		// 0x0109
	{199, 2, "Big Top"},		// 0x010A
	{200, 2, "Rocket"},		// 0x010B

	// Animal Crossing Cards: Series 3 [0x010C-0x016F]
	{201, 3, "Rover"},		// 0x010C
	{202, 3, "Blathers"},		// 0x010D
	{203, 3, "Tom Nook"},		// 0x010E
	{204, 3, "Pelly"},		// 0x010F
	{205, 3, "Phyllis"},		// 0x0110
	{206, 3, "Pete"},		// 0x0111
	{207, 3, "Mabel"},		// 0x0112
	{208, 3, "Leif"},		// 0x0113
	{209, 3, "Wendell"},		// 0x0114
	{210, 3, "Cyrus"},		// 0x0115
	{211, 3, "Grams"},		// 0x0116
	{212, 3, "Timmy"},		// 0x0117
	{213, 3, "Digby"},		// 0x0118
	{214, 3, "Don Resetti"},	// 0x0119
	{215, 3, "Isabelle"},		// 0x011A
	{216, 3, "Franklin"},		// 0x011B
	{217, 3, "Jingle"},		// 0x011C
	{218, 3, "Lily"},		// 0x011D
	{219, 3, "Anchovy"},		// 0x011E
	{220, 3, "Tabby"},		// 0x011F
	{221, 3, "Kody"},		// 0x0120
	{222, 3, "Miranda"},		// 0x0121
	{223, 3, "Del"},		// 0x0122
	{224, 3, "Paula"},		// 0x0123
	{225, 3, "Ken"},		// 0x0124
	{226, 3, "Mitzi"},		// 0x0125
	{227, 3, "Rodeo"},		// 0x0126
	{228, 3, "Bubbles"},		// 0x0127
	{229, 3, "Cousteau"},		// 0x0128
	{230, 3, "Velma"},		// 0x0129
	{231, 3, "Elvis"},		// 0x012A
	{232, 3, "Canberra"},		// 0x012B
	{233, 3, "Colton"},		// 0x012C
	{234, 3, "Marina"},		// 0x012D
	{235, 3, "Spork/Crackle"},	// 0x012E
	{236, 3, "Freckles"},		// 0x012F
	{237, 3, "Bam"},		// 0x0130
	{238, 3, "Friga"},		// 0x0131
	{239, 3, "Ricky"},		// 0x0132
	{240, 3, "Deirdre"},		// 0x0133
	{241, 3, "Hans"},		// 0x0134
	{242, 3, "Chevre"},		// 0x0135
	{243, 3, "Drago"},		// 0x0136
	{244, 3, "Tangy"},		// 0x0137
	{245, 3, "Mac"},		// 0x0138
	{246, 3, "Eloise"},		// 0x0139
	{247, 3, "Wart Jr."},		// 0x013A
	{248, 3, "Hazel"},		// 0x013B
	{249, 3, "Beardo"},		// 0x013C
	{250, 3, "Ava"},		// 0x013D
	{251, 3, "Chester"},		// 0x013E
	{252, 3, "Merry"},		// 0x013F
	{253, 3, "Genji"},		// 0x0140
	{254, 3, "Greta"},		// 0x0141
	{255, 3, "Wolfgang"},		// 0x0142
	{256, 3, "Diva"},		// 0x0143
	{257, 3, "Klaus"},		// 0x0144
	{258, 3, "Daisy"},		// 0x0145
	{259, 3, "Stinky"},		// 0x0146
	{260, 3, "Tammi"},		// 0x0147
	{261, 3, "Tucker"},		// 0x0148
	{262, 3, "Blanche"},		// 0x0149
	{263, 3, "Gaston"},		// 0x014A
	{264, 3, "Marshal"},		// 0x014B
	{265, 3, "Gala"},		// 0x014C
	{266, 3, "Joey"},		// 0x014D
	{267, 3, "Pippy"},		// 0x014E
	{268, 3, "Buck"},		// 0x014F
	{269, 3, "Bree"},		// 0x0150
	{270, 3, "Rooney"},		// 0x0151
	{271, 3, "Curlos"},		// 0x0152
	{272, 3, "Skye"},		// 0x0153
	{273, 3, "Moe"},		// 0x0154
	{274, 3, "Flora"},		// 0x0155
	{275, 3, "Hamlet"},		// 0x0156
	{276, 3, "Astrid"},		// 0x0157
	{277, 3, "Monty"},		// 0x0158
	{278, 3, "Dora"},		// 0x0159
	{279, 3, "Biskit"},		// 0x015A
	{280, 3, "Victoria"},		// 0x015B
	{281, 3, "Lyman"},		// 0x015C
	{282, 3, "Violet"},		// 0x015D
	{283, 3, "Frank"},		// 0x015E
	{284, 3, "Chadder"},		// 0x015F
	{285, 3, "Merengue"},		// 0x0160
	{286, 3, "Cube"},		// 0x0161
	{287, 3, "Claudia"},		// 0x0162
	{288, 3, "Curly"},		// 0x0163
	{289, 3, "Boomer"},		// 0x0164
	{290, 3, "Caroline"},		// 0x0165
	{291, 3, "Sparro"},		// 0x0166
	{292, 3, "Baabara"},		// 0x0167
	{293, 3, "Rolf"},		// 0x0168
	{294, 3, "Maple"},		// 0x0169
	{295, 3, "Antonio"},		// 0x016A
	{296, 3, "Soleil"},		// 0x016B
	{297, 3, "Apollo"},		// 0x016C
	{298, 3, "Derwin"},		// 0x016D
	{299, 3, "Francine"},		// 0x016E
	{300, 3, "Chrissy"},		// 0x016F

	// Animal Crossing Cards: Series 4 [0x0170-0x01D3]
	{301, 4, "Isabelle"},		// 0x0170
	{302, 4, "Brewster"},		// 0x0171
	{303, 4, "Katrina"},		// 0x0172
	{304, 4, "Phineas"},		// 0x0173
	{305, 4, "Celeste"},		// 0x0174
	{306, 4, "Tommy"},		// 0x0175
	{307, 4, "Gracie"},		// 0x0176
	{308, 4, "Leilani"},		// 0x0177
	{309, 4, "Resetti"},		// 0x0178
	{310, 4, "Timmy"},		// 0x0179
	{311, 4, "Lottie"},		// 0x017A
	{312, 4, "Shrunk"},		// 0x017B
	{313, 4, "Pave"},		// 0x017C
	{314, 4, "Gulliver"},		// 0x017D
	{315, 4, "Redd"},		// 0x017E
	{316, 4, "Zipper"},		// 0x017F
	{317, 4, "Goldie"},		// 0x0180
	{318, 4, "Stitches"},		// 0x0181
	{319, 4, "Pinky"},		// 0x0182
	{320, 4, "Mott"},		// 0x0183
	{321, 4, "Mallary"},		// 0x0184
	{322, 4, "Rocco"},		// 0x0185
	{323, 4, "Katt"},		// 0x0186
	{324, 4, "Graham"},		// 0x0187
	{325, 4, "Peaches"},		// 0x0188
	{326, 4, "Dizzy"},		// 0x0189
	{327, 4, "Penelope"},		// 0x018A
	{328, 4, "Boone"},		// 0x018B
	{329, 4, "Broffina"},		// 0x018C
	{330, 4, "Croque"},		// 0x018D
	{331, 4, "Pashmina"},		// 0x018E
	{332, 4, "Shep"},		// 0x018F
	{333, 4, "Lolly"},		// 0x0190
	{334, 4, "Erik"},		// 0x0191
	{335, 4, "Dotty"},		// 0x0192
	{336, 4, "Pierce"},		// 0x0193
	{337, 4, "Queenie"},		// 0x0194
	{338, 4, "Fang"},		// 0x0195
	{339, 4, "Frita"},		// 0x0196
	{340, 4, "Tex"},		// 0x0197
	{341, 4, "Melba"},		// 0x0198
	{342, 4, "Bones"},		// 0x0199
	{343, 4, "Anabelle"},		// 0x019A
	{344, 4, "Rudy"},		// 0x019B
	{345, 4, "Naomi"},		// 0x019C
	{346, 4, "Peewee"},		// 0x019D
	{347, 4, "Tammy"},		// 0x019E
	{348, 4, "Olaf"},		// 0x019F
	{349, 4, "Lucy"},		// 0x01A0
	{350, 4, "Elmer"},		// 0x01A1
	{351, 4, "Puddles"},		// 0x01A2
	{352, 4, "Rory"},		// 0x01A3
	{353, 4, "Elise"},		// 0x01A4
	{354, 4, "Walt"},		// 0x01A5
	{355, 4, "Mira"},		// 0x01A6
	{356, 4, "Pietro"},		// 0x01A7
	{357, 4, "Aurora"},		// 0x01A8
	{358, 4, "Papi"},		// 0x01A9
	{359, 4, "Apple"},		// 0x01AA
	{360, 4, "Rod"},		// 0x01AB
	{361, 4, "Purrl"},		// 0x01AC
	{362, 4, "Static"},		// 0x01AD
	{363, 4, "Celia"},		// 0x01AE
	{364, 4, "Zucker"},		// 0x01AF
	{365, 4, "Peggy"},		// 0x01B0
	{366, 4, "Ribbot"},		// 0x01B1
	{367, 4, "Annalise"},		// 0x01B2
	{368, 4, "Chow"},		// 0x01B3
	{369, 4, "Sylvia"},		// 0x01B4
	{370, 4, "Jacques"},		// 0x01B5
	{371, 4, "Sally"},		// 0x01B6
	{372, 4, "Doc"},		// 0x01B7
	{373, 4, "Pompom"},		// 0x01B8
	{374, 4, "Tank"},		// 0x01B9
	{375, 4, "Becky"},		// 0x01BA
	{376, 4, "Rizzo"},		// 0x01BB
	{377, 4, "Sydney"},		// 0x01BC
	{378, 4, "Barold"},		// 0x01BD
	{379, 4, "Nibbles"},		// 0x01BE
	{380, 4, "Kevin"},		// 0x01BF
	{381, 4, "Gloria"},		// 0x01C0
	{382, 4, "Lobo"},		// 0x01C1
	{383, 4, "Hippeux"},		// 0x01C2
	{384, 4, "Margie"},		// 0x01C3
	{385, 4, "Lucky"},		// 0x01C4
	{386, 4, "Rosie"},		// 0x01C5
	{387, 4, "Rowan"},		// 0x01C6
	{388, 4, "Maelle"},		// 0x01C7
	{389, 4, "Bruce"},		// 0x01C8
	{390, 4, "OHare"},		// 0x01C9
	{391, 4, "Gayle"},		// 0x01CA
	{392, 4, "Cranston"},		// 0x01CB
	{393, 4, "Frobert"},		// 0x01CC
	{394, 4, "Grizzly"},		// 0x01CD
	{395, 4, "Cally"},		// 0x01CE
	{396, 4, "Simon"},		// 0x01CF
	{397, 4, "Iggly"},		// 0x01D0
	{398, 4, "Angus"},		// 0x01D1
	{399, 4, "Twiggy"},		// 0x01D2
	{400, 4, "Robin"},		// 0x01D3

	// Animal Crossing: Character Parfait, Amiibo Festival
	{401, 5, "Isabelle (Parfait)"},		// 0x01D4
	{402, 5, "Goldie (amiibo Festival)"},	// 0x01D5
	{403, 5, "Stitches (amiibo Festival)"},	// 0x01D6
	{404, 5, "Rosie (amiibo Festival)"},	// 0x01D7
	{405, 5, "K.K. Slider (Parfait)"},	// 0x01D8

	// Unused [0x01D9-0x01DF]
	{  0, 0, nullptr},			// 0x01D9
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x01DA,0x01DB
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x01DC,0x01DD
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x01DE,0x01DF

	// Unused [0x01E0-0x01EF]
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x01E0,0x01E1
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x01E2,0x01E3
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x01E4,0x01E5
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x01E6,0x01E7
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x01E8,0x01E9
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x01EA,0x01EB
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x01EC,0x01ED
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x01EE,0x01EF

	// Unused [0x01F0-0x01FF]
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x01F0,0x01E1
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x01F2,0x01F3
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x01F4,0x01F5
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x01F6,0x01F7
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x01F8,0x01F9
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x01FA,0x01FB
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x01FC,0x01FD
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x01FE,0x01FF

	// Unused [0x0200-0x020F]
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0200,0x0201
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0202,0x0203
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0204,0x0205
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0206,0x0207
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0208,0x0209
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x020A,0x020B
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x020C,0x020D
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x020E,0x020F

	// Unused [0x0210-0x021F]
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0210,0x0211
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0212,0x0213
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0214,0x0215
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0216,0x0217
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0218,0x0219
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x021A,0x021B
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x021C,0x021D
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x021E,0x021F

	// Unused [0x0220-0x022F]
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0220,0x0221
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0222,0x0223
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0224,0x0225
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0226,0x0227
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0228,0x0229
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x022A,0x022B
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x022C,0x022D
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x022E,0x022F

	// Unused [0x0230-0x0237]
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0230,0x0231
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0232,0x0233
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0234,0x0235
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0236,0x0237

	// SMB 30th Anniversary [0x0238-0x0239]
	{  1, 1, "8-bit Mario (Classic Color)"},// 0x0238
	{  2, 1, "8-bit Mario (Modern Color)"},	// 0x0239

	// Skylanders Series [0x023A-0x023B]
	{  1, 0, "Hammer Slam Bowser"},		// 0x023A
	{  2, 0, "Turbo Charge Donkey Kong"},	// 0x023B
#if 0
	// NOTE: Cannot distinguish between regular and dark
	// variants in amiibo mode.
	{  3, 0, "Dark Hammer Slam Bowser"},		// 0x023A
	{  4, 0, "Dark Turbo Charge Donkey Kong"},	// 0x023B
#endif

	// Unused [0x023C]
	{  0, 0, nullptr},		// 0x023C

	// SSB: Mewtwo (Wave 7) [0x023D]
	{ 51, 7, "Mewtwo"},		// 0x023D

	// Yarn Yoshi: Mega Yarn Yoshi [0x023E]
	{  4, 0, "Mega Yarn Yoshi"},	// 0x023E

	// Animal Crossing Figurines: Wave 1 [0x023F-0x0246]
	{  0, 1, "Isabelle"},		// 0x023F
	{  0, 1, "K.K. Slider"},	// 0x0240
	{  0, 1, "Mabel"},		// 0x0241
	{  0, 1, "Tom Nook"},		// 0x0242
	{  0, 1, "Digby"},		// 0x0243
	{  0, 1, "Lottie"},		// 0x0244
	{  0, 1, "Reese"},		// 0x0245
	{  0, 1, "Cyrus"},		// 0x0246

	// Animal Crossing Figurines: Wave 2 [0x0247-0x024A]
	{  0, 2, "Blathers"},		// 0x0247
	{  0, 2, "Celeste"},		// 0x0248
	{  0, 2, "Resetti"},		// 0x0249
	{  0, 2, "Kicks"},		// 0x024A

	// Animal Crossing Figurines: Wave 4 (out of order) [0x024B]
	{  0, 4, "Isabelle (Summer Outfit)"},// 0x024B

	// Animal Crossing Figurines: Wave 3 [0x024C-0x024E]
	{  0, 3, "Rover"},		// 0x024C
	{  0, 3, "Timmy & Tommy"},	// 0x024D
	{  0, 3, "Kapp'n"},		// 0x024E

	// The Legend of Zelda: Twilight Princess [0x024F]
	{  0, 1, "Midna & Wolf Link"},	// 0x024F

	// Shovel Knight [0x0250]
	{  0, 0, "Shovel Knight"},	// 0x0250

	// SSB: DLC characters (Waves 8+)
	{ 53, 8, "Lucas"},		// 0x0251
	{ 55, 9, "Roy"},		// 0x0252
	{ 56, 9, "Ryu"},		// 0x0253

	// Kirby [0x0254-0x0257]
	{  0, 0, "Kirby"},		// 0x0254
	{  0, 0, "Meta Knight"},	// 0x0255
	{  0, 0, "King Dedede"},	// 0x0256
	{  0, 0, "Waddle Dee"},		// 0x0257

	// SSB: Special amiibo [0x0258]
	{  0, 0, "Mega Man (Gold Edition)"},// 0x0258

	// SSB: Wave 10 [0x0259-0x025B]
	{ 57, 10, "Cloud"},		// 0x0259
	{ 58, 10, "Corrin"},		// 0x025A
	{ 59, 10, "Bayonetta"},		// 0x025B

	// Pokkén Tournament [0x025C]
	{  0, 0, "Shadow Mewtwo"},	// 0x025C

	// Splatoon: Wave 2 [0x025D-0x0261]
	{  0, 2, "Callie"},			// 0x025D
	{  0, 2, "Marie"},			// 0x025E
	{  0, 2, "Inkling Girl (Lime Green)"},	// 0x025F
	{  0, 2, "Inkling Boy (Purple)"},	// 0x0260
	{  0, 2, "Inkling Squid (Orange)"},	// 0x0261

	// SMB: Wave 2 [0x0262-0x0268]
	{ 12, 2, "Rosalina"},		// 0x0262
	{  9, 2, "Wario"},		// 0x0263
	{ 13, 2, "Donkey Kong"},	// 0x0264
	{ 14, 2, "Diddy Kong"},		// 0x0265
	{ 11, 2, "Daisy"},		// 0x0266
	{ 10, 2, "Waluigi"},		// 0x0267
	{ 15, 2, "Boo"},		// 0x0268

	// Mario Sports Superstars Cards [0x0269-0x02C2]
	{  1,  1, "Mario (Soccer)"},		// 0x0269
	{  2,  1, "Mario (Baseball)"},		// 0x026A
	{  3,  1, "Mario (Tennis)"},		// 0x026B
	{  4,  1, "Mario (Golf)"},		// 0x026C
	{  5,  1, "Mario (Horse Racing)"},	// 0x026D
	{  6,  1, "Luigi (Soccer)"},		// 0x026E
	{  7,  1, "Luigi (Baseball)"},		// 0x026F
	{  8,  1, "Luigi (Tennis)"},		// 0x0270
	{  9,  1, "Luigi (Golf)"},		// 0x0271
	{ 10,  1, "Luigi (Horse Racing)"},	// 0x0272
	{ 11,  1, "Peach (Soccer)"},		// 0x0273
	{ 12,  1, "Peach (Baseball)"},		// 0x0274
	{ 13,  1, "Peach (Tennis)"},		// 0x0275
	{ 14,  1, "Peach (Golf)"},		// 0x0276
	{ 15,  1, "Peach (Horse Racing)"},	// 0x0277
	{ 16,  1, "Daisy (Soccer)"},		// 0x0278
	{ 17,  1, "Daisy (Baseball)"},		// 0x0279
	{ 18,  1, "Daisy (Tennis)"},		// 0x027A
	{ 19,  1, "Daisy (Golf)"},		// 0x027B
	{ 20,  1, "Daisy (Horse Racing)"},	// 0x027C
	{ 21,  1, "Yoshi (Soccer)"},		// 0x027D
	{ 22,  1, "Yoshi (Baseball)"},		// 0x027E
	{ 23,  1, "Yoshi (Tennis)"},		// 0x027F
	{ 24,  1, "Yoshi (Golf)"},		// 0x0280
	{ 25,  1, "Yoshi (Horse Racing)"},	// 0x0281
	{ 26,  1, "Wario (Soccer)"},		// 0x0282
	{ 27,  1, "Wario (Baseball)"},		// 0x0283
	{ 28,  1, "Wario (Tennis)"},		// 0x0284
	{ 29,  1, "Wario (Golf)"},		// 0x0285
	{ 30,  1, "Wario (Horse Racing)"},	// 0x0286
	{ 31,  1, "Waluigi (Soccer)"},		// 0x0287
	{ 32,  1, "Waluigi (Baseball)"},	// 0x0288
	{ 33,  1, "Waluigi (Tennis)"},		// 0x0289
	{ 34,  1, "Waluigi (Golf)"},		// 0x028A
	{ 35,  1, "Waluigi (Horse Racing)"},	// 0x028B
	{ 36,  1, "Donkey Kong (Soccer)"},	// 0x028C
	{ 37,  1, "Donkey Kong (Baseball)"},	// 0x028D
	{ 38,  1, "Donkey Kong (Tennis)"},	// 0x028E
	{ 39,  1, "Donkey Kong (Golf)"},	// 0x028F
	{ 40,  1, "Donkey Kong (Horse Racing)"},// 0x0290
	{ 41,  1, "Diddy Kong (Soccer)"},	// 0x0291
	{ 42,  1, "Diddy Kong (Baseball)"},	// 0x0292
	{ 43,  1, "Diddy Kong (Tennis)"},	// 0x0293
	{ 44,  1, "Diddy Kong (Golf)"},		// 0x0294
	{ 45,  1, "Diddy Kong (Horse Racing)"},	// 0x0295
	{ 46,  1, "Bowser (Soccer)"},		// 0x0296
	{ 47,  1, "Bowser (Baseball)"},		// 0x0297
	{ 48,  1, "Bowser (Tennis)"},		// 0x0298
	{ 49,  1, "Bowser (Golf)"},		// 0x0299
	{ 50,  1, "Bowser (Horse Racing)"},	// 0x029A
	{ 51,  1, "Bowser Jr. (Soccer)"},	// 0x029B
	{ 52,  1, "Bowser Jr. (Baseball)"},	// 0x029C
	{ 53,  1, "Bowser Jr. (Tennis)"},	// 0x029D
	{ 54,  1, "Bowser Jr. (Golf)"},		// 0x029E
	{ 55,  1, "Bowser Jr. (Horse Racing)"},	// 0x029F
	{ 56,  1, "Boo (Soccer)"},		// 0x02A0
	{ 57,  1, "Boo (Baseball)"},		// 0x02A1
	{ 58,  1, "Boo (Tennis)"},		// 0x02A2
	{ 59,  1, "Boo (Golf)"},		// 0x02A3
	{ 60,  1, "Boo (Horse Racing)"},	// 0x02A4
	{ 61,  1, "Baby Mario (Soccer)"},	// 0x02A5
	{ 62,  1, "Baby Mario (Baseball)"},	// 0x02A6
	{ 63,  1, "Baby Mario (Tennis)"},	// 0x02A7
	{ 64,  1, "Baby Mario (Golf)"},		// 0x02A8
	{ 65,  1, "Baby Mario (Horse Racing)"},	// 0x02A9
	{ 66,  1, "Baby Luigi (Soccer)"},	// 0x02AA
	{ 67,  1, "Baby Luigi (Baseball)"},	// 0x02AB
	{ 68,  1, "Baby Luigi (Tennis)"},	// 0x02AC
	{ 69,  1, "Baby Luigi (Golf)"},		// 0x02AD
	{ 70,  1, "Baby Luigi (Horse Racing)"},	// 0x02AE
	{ 71,  1, "Birdo (Soccer)"},		// 0x02AF
	{ 72,  1, "Birdo (Baseball)"},		// 0x02B0
	{ 73,  1, "Birdo (Tennis)"},		// 0x02B1
	{ 74,  1, "Birdo (Golf)"},		// 0x02B2
	{ 75,  1, "Birdo (Horse Racing)"},	// 0x02B3
	{ 76,  1, "Rosalina (Soccer)"},		// 0x02B4
	{ 77,  1, "Rosalina (Baseball)"},	// 0x02B5
	{ 78,  1, "Rosalina (Tennis)"},		// 0x02B6
	{ 79,  1, "Rosalina (Golf)"},		// 0x02B7
	{ 80,  1, "Rosalina (Horse Racing)"},	// 0x02B8
	{ 81,  1, "Metal Mario (Soccer)"},	// 0x02B9
	{ 82,  1, "Metal Mario (Baseball)"},	// 0x02BA
	{ 83,  1, "Metal Mario (Tennis)"},	// 0x02BB
	{ 84,  1, "Metal Mario (Golf)"},	// 0x02BC
	{ 85,  1, "Metal Mario (Horse Racing)"},// 0x02BD
	{ 86,  1, "Pink Gold Peach (Soccer)"},	// 0x02BE
	{ 87,  1, "Pink Gold Peach (Baseball)"},// 0x02BF
	{ 88,  1, "Pink Gold Peach (Tennis)"},	// 0x02C0
	{ 89,  1, "Pink Gold Peach (Golf)"},	// 0x02C1
	{ 90,  1, "Pink Gold Peach (Horse Racing)"},	// 0x02C2

	// Unused [0x02C3-0x02CF]
	{  0, 0, nullptr},			// 0x02C3
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x02C4,0x02C5
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x02C6,0x02C7
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x02C8,0x02C9
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x02CA,0x02CB
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x02CC,0x02CD
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x02CE,0x02CF

	// Unused [0x02D0-0x02DF]
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x02D0,0x02D1
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x02D2,0x02D3
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x02D4,0x02D5
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x02D6,0x02D7
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x02D8,0x02D9
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x02DA,0x02DB
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x02DC,0x02DD
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x02DE,0x02DF

	// Unused [0x02E1]
	{  0, 0, nullptr},			// 0x02E1

	// Monster Hunter [0x02E1-0x02E6]
	{  2, 1, "One-Eyed Rathalos and Rider (Female)"},	// 0x02E1
	{  1, 1, "One-Eyed Rathalos and Rider (Male)"},		// 0x02E2
	{  3, 1, "Nabiru"},			// 0x02E3
	{  4, 2, "Rathian and Cheval"},		// 0x02E4
	{  5, 2, "Barioth and Ayuria"},		// 0x02E5
	{  6, 2, "Qurupeco and Dan"},		// 0x02E6

	// Animal Crossing: Welcome Amiibo Series [0x02E8-0x031E]
	{  1, 7, "Vivian"},		// 0x02E7
	{  2, 7, "Hopkins"},		// 0x02E8
	{  3, 7, "June"},		// 0x02E9
	{  4, 7, "Piper"},		// 0x02EA
	{  5, 7, "Paolo"},		// 0x02EB
	{  6, 7, "Hornsby"},		// 0x02EC
	{  7, 7, "Stella"},		// 0x02ED
	{  8, 7, "Tybalt"},		// 0x02EE
	{  9, 7, "Huck"},		// 0x02EF
	{ 10, 7, "Sylvana"},		// 0x02F0
	{ 11, 7, "Boris"},		// 0x02F1
	{ 12, 7, "Wade"},		// 0x02F2
	{ 13, 7, "Carrie"},		// 0x02F3
	{ 14, 7, "Ketchup"},		// 0x02F4
	{ 15, 7, "Rex"},		// 0x02F5
	{ 16, 7, "Stu"},		// 0x02F6
	{ 17, 7, "Ursala"},		// 0x02F7
	{ 18, 7, "Jacob"},		// 0x02F8
	{ 19, 7, "Maddie"},		// 0x02F9
	{ 20, 7, "Billy"},		// 0x02FA
	{ 21, 7, "Boyd"},		// 0x02FB
	{ 22, 7, "Bitty"},		// 0x02FC
	{ 23, 7, "Maggie"},		// 0x02FD
	{ 24, 7, "Murphy"},		// 0x02FE
	{ 25, 7, "Plucky"},		// 0x02FF
	{ 26, 7, "Sandy"},		// 0x0300
	{ 27, 7, "Claude"},		// 0x0301
	{ 28, 7, "Raddle"},		// 0x0302
	{ 29, 7, "Julia"},		// 0x0303
	{ 30, 7, "Louie"},		// 0x0304
	{ 31, 7, "Bea"},		// 0x0305
	{ 32, 7, "Admiral"},		// 0x0306
	{ 33, 7, "Ellie"},		// 0x0307
	{ 34, 7, "Boots"},		// 0x0308
	{ 35, 7, "Weber"},		// 0x0309
	{ 36, 7, "Candi"},		// 0x030A
	{ 37, 7, "Leopold"},		// 0x030B
	{ 38, 7, "Spike"},		// 0x030C
	{ 39, 7, "Cashmere"},		// 0x030D
	{ 40, 7, "Tad"},		// 0x030E
	{ 41, 7, "Norma"},		// 0x030F
	{ 42, 7, "Gonzo"},		// 0x0310
	{ 43, 7, "Sprocket"},		// 0x0311
	{ 44, 7, "Snooty"},		// 0x0312
	{ 45, 7, "Olive"},		// 0x0313
	{ 46, 7, "Dobie"},		// 0x0314
	{ 47, 7, "Buzz"},		// 0x0315
	{ 48, 7, "Cleo"},		// 0x0316
	{ 49, 7, "Ike"},		// 0x0317
	{ 50, 7, "Tasha"},		// 0x0318

	// Animal Crossing x Sanrio Series
	{  1, 6, "Rilla"},		// 0x0319
	{  2, 6, "Marty"},		// 0x031A
	{  3, 6, "\xC3\x89toile"},	// 0x031B
	{  4, 6, "Chai"},		// 0x031C
	{  5, 6, "Chelsea"},		// 0x031D
	{  6, 6, "Toby"},		// 0x031E

	// Unused [0x031F-0x32F]
	{  0, 0, nullptr},			// 0x031F
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0320,0x0321
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0322,0x0323
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0324,0x0325
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0326,0x0327
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0328,0x0329
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x032A,0x032B
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x032C,0x032D
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x032E,0x032F

	// Unused [0x0330-0x33F]
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0330,0x0331
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0332,0x0333
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0334,0x0335
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0336,0x0337
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0338,0x0339
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x033A,0x033B
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x033C,0x033D
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x033E,0x033F

	// Unused [0x0340-0x34A]
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0340,0x0341
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0342,0x0343
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0344,0x0345
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0346,0x0347
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x0348,0x0349
	{  0, 0, nullptr},			// 0x034A

	// The Legend of Zelda: 30th Anniversary Series
	{  0, 2, "Link (Ocarina of Time)"},	// 0x034B
	{  0, 4, "Link (Majora's Mask)"},	// 0x034C
	{  0, 4, "Link (Twilight Princess)"},	// 0x034D
	{  0, 4, "Link (Skyward Sword)"},	// 0x034E
	{  0, 2, "Link (8-bit)"},		// 0x034F
	{  0, 2, "Toon Link (The Wind Waker)"},	// 0x0350
	{  0, 0, nullptr},			// 0x0351
	{  0, 2, "Toon Zelda (The Wind Waker)"},// 0x0352

	// The Legend of Zelda: Breath of the Wild Series
	{  0, 3, "Link (Archer)"},		// 0x0353
	{  0, 3, "Link (Rider)"},		// 0x0354
	{  0, 3, "Guardian"},			// 0x0355
	{  0, 3, "Zelda"},			// 0x0356
	{  0, 0, nullptr},			// 0x0357 (???)
	// The Legend of Zelda: Breath of the Wild Series (Champions)
	{  0, 4, "Daruk"},			// 0x0358
	{  0, 4, "Urbosa"},			// 0x0359
	{  0, 4, "Mipha"},			// 0x035A
	{  0, 4, "Revali"},			// 0x035B
	// The Legend of Zelda: Breath of the Wild Series (Wave 3, continued)
	{  0, 3, "Bokoblin"},			// 0x035C

	// Yarn Yoshi: Poochy [0x035D]
	{  5, 0, "Poochy"},			// 0x035D

	// BoxBoy!: Qbby [0x035E]
	{  0, 0, "Qbby"},			// 0x035E

	// Unused [0x035F]
	{  0, 0, nullptr},			// 0x035F

	// Fire Emblem [0x0360-0x0361]
	{  1, 0, "Alm"},			// 0x0360
	{  2, 0, "Celica"},			// 0x0361

	// SSB: Wave 10 [0x0362-0x0364]
	{ 60, 10, "Cloud (Player 2)"},		// 0x0362
	{ 61, 10, "Corrin (Player 2)"},		// 0x0363
	{ 62, 10, "Bayonetta (Player 2)"},	// 0x0364

	// Metroid [0x365-0x366]
	{  1, 1, "Samus Aran"},			// 0x0365
	{  2, 1, "Metroid"},			// 0x0366

	// SMB: Wave 3 [0x0367-0x0368]
	{ 16, 3, "Goomba"},			// 0x0367
	{ 17, 3, "Koopa Troopa"},		// 0x0368

	// Splatoon: Wave 3 [0x0369-0x036B]
	{  0, 3, "Inkling Girl (Neon Pink)"},	// 0x0369
	{  0, 3, "Inkling Boy (Neon Green)"},	// 0x036A
	{  0, 3, "Inkling Squid (Neon Purple)"},// 0x036B

	// Unused [0x36C-0x0370]
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x036C-0x036D
	{  0, 0, nullptr},			// 0x036E

	// Fire Emblem [0x036F-0x0370]
	{  3, 0, "Chrom"},			// 0x036F
	{  4, 0, "Tiki"},			// 0x0370

	// SMB: Wave 4 (Super Mario Odyssey) [0x0371-0x373]
	{ 18, 4, "Mario - Wedding"},		// 0x0371
	{ 19, 4, "Peach - Wedding"},		// 0x0372
	{ 20, 4, "Bowser - Wedding"},		// 0x0373

	// Cereal [0x374]
	{  0, 0, "Super Mario Cereal"},		// 0x0374

#if 0
	// TODO: Not released yet.

	// The Legend of Zelda
	{  0,  0, "Link (Majora's Mask)"},	// 0x0xxx
	{  0,  0, "Link (Twilight Princess)"},	// 0x0xxx
	{  0,  0, "Link (Skyward Sword)"},	// 0x0xxx

	// Pikmin
	{  0, 0, "Pikmin"},			// 0x0xxx
#endif
};

/** AmiiboData **/

/**
 * Look up a character series name.
 * @param char_id Character ID. (Page 21) [must be host-endian]
 * @return Character series name, or nullptr if not found.
 */
const char *AmiiboData::lookup_char_series_name(uint32_t char_id)
{
	static_assert(ARRAY_SIZE(AmiiboDataPrivate::char_series_names) == (0x374/4)+1,
		"char_series_names[] is out of sync with the amiibo ID list.");

	const unsigned int series_id = (char_id >> 22) & 0x3FF;
	if (series_id >= ARRAY_SIZE(AmiiboDataPrivate::char_series_names))
		return nullptr;
	return AmiiboDataPrivate::char_series_names[series_id];
}

/**
 * Look up a character's name.
 * @param char_id Character ID. (Page 21) [must be host-endian]
 * @return Character name. (If variant, the variant name is used.)
 * If an invalid character ID or variant, nullptr is returned.
 */
const char *AmiiboData::lookup_char_name(uint32_t char_id)
{
	const uint16_t id = (char_id >> 16) & 0xFFFF;

	// Do a binary search.
	const AmiiboDataPrivate::char_id_t key = {id, 0, nullptr, nullptr};
	const AmiiboDataPrivate::char_id_t *res =
		static_cast<const AmiiboDataPrivate::char_id_t*>(bsearch(&key,
			AmiiboDataPrivate::char_ids,
			ARRAY_SIZE(AmiiboDataPrivate::char_ids),
			sizeof(AmiiboDataPrivate::char_id_t),
			AmiiboDataPrivate::char_id_t_compar));
	if (!res) {
		// Character ID not found.
		return nullptr;
	}

	// Check for variants.
	uint8_t variant_id = (char_id >> 8) & 0xFF;
	if (!res->variants || res->variants_size == 0) {
		if (variant_id == 0) {
			// No variants, and variant ID is 0.
			return res->name;
		}

		// No variants, but the variant ID is non-zero.
		// Invalid char_id.
		return nullptr;
	}

	// Do a linear search in the variant array.
	// (Linear instead of binary because the largest
	// variant array is 4 elements.)
	const AmiiboDataPrivate::char_variant_t *variant = res->variants;
	for (int i = res->variants_size; i > 0; i--, variant++) {
		if (variant->variant_id == variant_id) {
			// Found the variant.
			return variant->name;
		}
	}

	// Variant not found.
	return nullptr;
}

/**
 * Look up an amiibo series name.
 * @param amiibo_id	[in] amiibo ID. (Page 22) [must be host-endian]
 * @return amiibo series name, or nullptr if not found.
 */
const char *AmiiboData::lookup_amiibo_series_name(uint32_t amiibo_id)
{
	// FIXME: gcc-6.3.0 is trying to interpret 0x035E+1 as a
	// floating-point hex constant:
	// error: unable to find numeric literal operator ‘operator""+1’
	static_assert(ARRAY_SIZE(AmiiboDataPrivate::amiibo_ids) == ((0x0374)+1),
		"amiibo_ids[] is out of sync with the amiibo ID list.");

	const unsigned int series_id = (amiibo_id >> 8) & 0xFF;
	if (series_id >= ARRAY_SIZE(AmiiboDataPrivate::amiibo_series_names))
		return nullptr;
	return AmiiboDataPrivate::amiibo_series_names[series_id];
}

/**
 * Look up an amiibo's series identification.
 * @param amiibo_id	[in] amiibo ID. (Page 22) [must be host-endian]
 * @param pReleaseNo	[out,opt] Release number within series.
 * @param pWaveNo	[out,opt] Wave number within series.
 * @return amiibo series name, or nullptr if not found.
 */
const char *AmiiboData::lookup_amiibo_series_data(uint32_t amiibo_id, int *pReleaseNo, int *pWaveNo)
{
	const unsigned int id = (amiibo_id >> 16) & 0xFFFF;
	if (id >= ARRAY_SIZE(AmiiboDataPrivate::amiibo_ids)) {
		// ID is out of range.
		return nullptr;
	}

	const AmiiboDataPrivate::amiibo_id_t *const amiibo = &AmiiboDataPrivate::amiibo_ids[id];
	if (pReleaseNo) {
		*pReleaseNo = amiibo->release_no;
	}
	if (pWaveNo) {
		*pWaveNo = amiibo->wave_no;
	}

	return amiibo->name;
}

}
