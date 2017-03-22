/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * AmiiboData.cpp: Nintendo amiibo identification data.                    *
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

#include "AmiiboData.hpp"

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
		static const rp_char *const char_series_names[];

		// Character variants.
		// We can't use a standard character array because
		// the Skylanders variants use variant ID = 0xFF.
		struct char_variant_t {
			uint8_t variant_id;
			const rp_char *const name;
		};

		// Character IDs.
		// Sparse array, since we're using the series + character value here.
		// Sorted by series + character value.
		struct char_id_t {
			uint16_t char_id;		// Character ID. (Includes series ID.) [high 16 bits of page 21]
			const rp_char *name;		// Character name. (same as variant 0)
			const char_variant_t *variants;	// Array of variants, if any.
			int variants_size;		// Number of elements in variants.
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

		// Character IDs.
		static const char_id_t char_ids[];

		/**
		 * char_id_t bsearch() comparison function.
		 * @param a
		 * @param b
		 * @return
		 */
		static int char_id_t_compar(const void *a, const void *b);

		/** Page 22 (raw offset 0x58): amiibo series **/

		// amiibo series names.
		// Array index = SS
		static const rp_char *const amiibo_series_names[];

		// amiibo IDs.
		// Index is the amiibo ID. (aaaa)
		// NOTE: amiibo ID is unique across *all* amiibo,
		// so we can use a single array for all series.
		struct amiibo_id_t {
			uint16_t release_no;	// Release number. (0 for no ordering)
			uint8_t wave_no;	// Wave number.
			const rp_char *name;	// Character name.
		};
		static const amiibo_id_t amiibo_ids[];
};

/** Page 21 (raw offset 0x54): Character series **/

/**
 * Character series.
 * Array index == sss, rshifted by 2.
 */
const rp_char *const AmiiboDataPrivate::char_series_names[] = {
	_RP("Super Mario Bros."),	// 0x000
	nullptr,			// 0x004
	_RP("Yoshi"),			// 0x008
	nullptr,			// 0x00C
	_RP("The Legend of Zelda"),	// 0x010
	_RP("The Legend of Zelda"),	// 0x014

	// Animal Crossing
	_RP("Animal Crossing"),		// 0x018
	_RP("Animal Crossing"),		// 0x01C
	_RP("Animal Crossing"),		// 0x020
	_RP("Animal Crossing"),		// 0x024
	_RP("Animal Crossing"),		// 0x028
	_RP("Animal Crossing"),		// 0x02C
	_RP("Animal Crossing"),		// 0x030
	_RP("Animal Crossing"),		// 0x034
	_RP("Animal Crossing"),		// 0x038
	_RP("Animal Crossing"),		// 0x03C
	_RP("Animal Crossing"),		// 0x040
	_RP("Animal Crossing"),		// 0x044
	_RP("Animal Crossing"),		// 0x048
	_RP("Animal Crossing"),		// 0x04C
	_RP("Animal Crossing"),		// 0x050

	nullptr,			// 0x054
	_RP("Star Fox"),		// 0x058
	_RP("Metroid"),			// 0x05C
	_RP("F-Zero"),			// 0x060
	_RP("Pikmin"),			// 0x064
	nullptr,			// 0x068
	_RP("Punch-Out!!"),		// 0x06C
	_RP("Wii Fit"),			// 0x070
	_RP("Kid Icarus"),		// 0x074
	_RP("Classic Nintendo"),	// 0x078
	_RP("Mii"),			// 0x07C
	_RP("Splatoon"),		// 0x080

	// 0x084 - 0x098
	nullptr, nullptr, nullptr,	// 0x084
	nullptr, nullptr, nullptr, 	// 0x090

	_RP("Mario Sports Superstars"),	// 0x09C

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
#ifdef RP_UTF16
	_RP("Pok\u00E9mon"), _RP("Pok\u00E9mon"), _RP("Pok\u00E9mon"), _RP("Pok\u00E9mon"),
	_RP("Pok\u00E9mon"), _RP("Pok\u00E9mon"), _RP("Pok\u00E9mon"), _RP("Pok\u00E9mon"),
	_RP("Pok\u00E9mon"), _RP("Pok\u00E9mon"), _RP("Pok\u00E9mon"), _RP("Pok\u00E9mon"),
#else /* RP_UTF8 */
	_RP("Pok\xC3\xA9mon"), _RP("Pok\xC3\xA9mon"), _RP("Pok\xC3\xA9mon"), _RP("Pok\xC3\xA9mon"),
	_RP("Pok\xC3\xA9mon"), _RP("Pok\xC3\xA9mon"), _RP("Pok\xC3\xA9mon"), _RP("Pok\xC3\xA9mon"),
	_RP("Pok\xC3\xA9mon"), _RP("Pok\xC3\xA9mon"), _RP("Pok\xC3\xA9mon"), _RP("Pok\xC3\xA9mon"),
#endif

	nullptr, nullptr, nullptr, nullptr,	// 0x1C0

#ifdef RP_UTF16
	_RP("Pokk\u00E9n Tournament"),		// 0x1D0
#else /* RP_UTF8 */
	_RP("Pokk\xC3\xA9n Tournament"),	// 0x1D0
#endif
	nullptr, nullptr, nullptr,		// 0x1D4
	nullptr, nullptr, nullptr, nullptr,	// 0x1E0
	_RP("Kirby"),				// 0x1F0
	nullptr, nullptr, nullptr,		// 0x1F4
	nullptr, nullptr, nullptr, nullptr,	// 0x200
	_RP("Fire Emblem"),			// 0x210
	nullptr, nullptr, nullptr,		// 0x214
	nullptr,				// 0x220
	_RP("Xenoblade"),			// 0x224
	_RP("Earthbound"),			// 0x228
	_RP("Chibi-Robo!"),			// 0x22C

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

	_RP("Sonic the Hedgehog"),		// 0x320
	nullptr, nullptr, nullptr,		// 0x324
	nullptr,				// 0x330
	_RP("Pac-Man"),				// 0x334
	nullptr,				// 0x338
	nullptr,				// 0x33C
	nullptr,				// 0x340
	nullptr,				// 0x344
	_RP("Mega Man"),			// 0x348
	_RP("Street Fighter"),			// 0x34C
	_RP("Monster Hunter"),			// 0x350
	nullptr,				// 0x354
	nullptr,				// 0x358
	_RP("Shovel Knight"),			// 0x35C

	// TODO
#if 0
	_RP("Bayonetta"),
	_RP("Final Fantasy"),
#endif
};

// Character variants.
const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::smb_mario_variants[] = {
	{0x00, _RP("Mario")},
	{0x01, _RP("Dr. Mario")},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::smb_yoshi_variants[] = {
	{0x00, _RP("Yoshi")},
	{0x01, _RP("Yarn Yoshi")},	// Color variant is in Page 22, amiibo ID.
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::smb_rosalina_variants[] = {
	{0x00, _RP("Rosalina")},
	{0x01, _RP("Rosalina & Luma")},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::smb_bowser_variants[] = {
	{0x00, _RP("Bowser")},

	// Skylanders
	// NOTE: Cannot distinguish between regular and dark
	// variants in amiibo mode.
	{0xFF, _RP("Hammer Slam Bowser")},
	//{0xFF, _RP("Dark Hammer Slam Bowser")},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::smb_donkey_kong_variants[] = {
	{0x00, _RP("Donkey Kong")},

	// Skylanders
	// NOTE: Cannot distinguish between regular and dark
	// variants in amiibo mode.
	{0xFF, _RP("Turbo Charge Donkey Kong")},
	//{0xFF, _RP("Dark Turbo Charge Donkey Kong")},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::yoshi_poochy_variants[] = {
	{0x00, nullptr},	// TODO
	{0x01, _RP("Yarn Poochy")},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::tloz_link_variants[] = {
	{0x00, _RP("Link")},
	{0x01, _RP("Toon Link")},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::tloz_zelda_variants[] = {
	{0x00, _RP("Zelda")},
	{0x01, _RP("Sheik")},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::tloz_ganondorf_variants[] = {
	{0x00, nullptr},	// TODO
	{0x01, _RP("Ganondorf")},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::metroid_samus_variants[] = {
	{0x00, _RP("Samus")},
	{0x01, _RP("Zero Suit Samus")},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::pikmin_olimar_variants[] = {
	{0x00, nullptr},	// TODO
	{0x01, _RP("Olimar")},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::mii_variants[] = {
	{0x00, _RP("Mii Brawler")},
	{0x01, _RP("Mii Swordfighter")},
	{0x02, _RP("Mii Gunner")},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::splatoon_inkling_variants[] = {
	{0x00, _RP("Inkling")},	// NOTE: Not actually assigned.
	{0x01, _RP("Inkling Girl")},
	{0x02, _RP("Inkling Boy")},
	{0x03, _RP("Inkling Squid")},
};

// Mario Sports Superstars
// Each character has five variants (0x01-0x05).
// NOTE: Variant 0x00 is not actually assigned.
#define MSS_VARIANT_ARRAY_DEF(char_name, c_name) \
	const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::mss_##c_name##_variants[] = { \
		{0x00, _RP(char_name)}, \
		{0x01, _RP2(char_name, " (Soccer)")}, \
		{0x02, _RP2(char_name, " (Baseball")}, \
		{0x03, _RP2(char_name, " (Tennis)")}, \
		{0x04, _RP2(char_name, " (Golf)")}, \
		{0x05, _RP2(char_name, " (Horse Racing")}, \
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
	{0x00, _RP("Isabelle (Summer Outfit)")},
	{0x01, _RP("Isabelle (Autumn Outfit)")},
	// TODO: How are these ones different?
	{0x03, _RP("Isabelle (Series 4)")},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_kk_slider_variants[] = {
	{0x00, _RP("K.K. Slider")},
	{0x01, _RP("DJ K.K.")},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_tom_nook_variants[] = {
	{0x00, _RP("Tom Nook")},
	// TODO: Variant description.
	{0x01, _RP("Tom Nook (Series 3)")},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_timmy_variants[] = {
	// TODO: Variant descriptions.
	{0x00, _RP("Timmy")},
	{0x02, _RP("Timmy (Series 3)")},
	{0x04, _RP("Timmy (Series 4)")},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_tommy_variants[] = {
	// TODO: Variant descriptions.
	{0x01, _RP("Tommy (Series 2)")},
	{0x03, _RP("Tommy (Series 4)")},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_digby_variants[] = {
	{0x00, _RP("Digby")},
	// TODO: Variant description.
	{0x01, _RP("Digby (Series 3)")},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_resetti_variants[] = {
	{0x00, _RP("Resetti")},
	// TODO: Variant description.
	{0x01, _RP("Resetti (Series 4)")},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_don_resetti_variants[] = {
	// TODO: Variant descriptions.
	{0x00, _RP("Don Resetti (Series 2)")},
	{0x01, _RP("Don Resetti (Series 3)")},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_redd_variants[] = {
	{0x00, _RP("Redd")},
	// TODO: Variant description.
	{0x01, _RP("Redd (Series 4)")},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_dr_shrunk_variants[] = {
	{0x00, _RP("Dr. Shrunk")},
	{0x01, _RP("Shrunk")},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_lottie_variants[] = {
	{0x00, _RP("Lottie")},
	// TODO: Variant description.
	{0x01, _RP("Lottie (Series 4)")},
};

// Animal Crossing x Sanrio all have variant type 0x01.
const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_marty_variants[] = {
	{0x01, _RP("Marty (Sanrio)")},
};
const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_chelsea_variants[] = {
	{0x01, _RP("Chelsea (Sanrio)")},
};
const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_chai_variants[] = {
	{0x01, _RP("Chai (Sanrio)")},
};
const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_rilla_variants[] = {
	{0x01, _RP("Rilla (Sanrio)")},
};
const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_toby_variants[] = {
	{0x01, _RP("Toby (Sanrio)")},
};
const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::ac_etoile_variants[] = {
#ifdef RP_UTF16
	{0x01, _RP("\u00C9toile")},
#else /* RP_UTF8 */
	{0x01, _RP("\xC3\x89toile")},
#endif
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::mh_rathalos_variants[] = {
	{0x00, _RP("One-Eyed Rathalos and Rider")},	// NOTE: Not actually assigned.
	{0x01, _RP("One-Eyed Rathalos and Rider (Male)")},
	{0x02, _RP("One-Eyed Rathalos and Rider (Female)")},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::mh_rathian_cheval_variants[] = {
	{0x00, _RP("Rathian and Cheval")},	// NOTE: Not actually assigned.
	{0x01, _RP("Rathian and Cheval")},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::mh_barioth_ayuria_variants[] = {
	{0x00, _RP("Barioth and Ayuria")},	// NOTE: Not actually assigned.
	{0x01, _RP("Barioth and Ayuria")},
};

const AmiiboDataPrivate::char_variant_t AmiiboDataPrivate::mh_qurupeco_dan_variants[] = {
	{0x00, _RP("Qurupeco and Dan")},	// NOTE: Not actually assigned.
	{0x01, _RP("Qurupeco and Dan")},
};

// Character IDs.
const AmiiboDataPrivate::char_id_t AmiiboDataPrivate::char_ids[] = {
	// Super Mario Bros. (character series = 0x000)
	{0x0000, _RP("Mario"), smb_mario_variants, ARRAY_SIZE(smb_mario_variants)},
	{0x0001, _RP("Luigi"), nullptr, 0},
	{0x0002, _RP("Peach"), nullptr, 0},
	{0x0003, _RP("Yoshi"), smb_yoshi_variants, ARRAY_SIZE(smb_yoshi_variants)},
	{0x0004, _RP("Rosalina"), smb_rosalina_variants, ARRAY_SIZE(smb_rosalina_variants)},
	{0x0005, _RP("Bowser"), smb_bowser_variants, ARRAY_SIZE(smb_bowser_variants)},
	{0x0006, _RP("Bowser Jr."), nullptr, 0},
	{0x0007, _RP("Wario"), nullptr, 0},
	{0x0008, _RP("Donkey Kong"), smb_donkey_kong_variants, ARRAY_SIZE(smb_donkey_kong_variants)},
	{0x0009, _RP("Diddy Kong"), nullptr, 0},
	{0x000A, _RP("Toad"), nullptr, 0},
	{0x0013, _RP("Daisy"), nullptr, 0},
	{0x0014, _RP("Waluigi"), nullptr, 0},
	{0x0017, _RP("Boo"), nullptr, 0},

	// Yoshi (character series = 0x008)
	{0x0080, _RP("Poochy"), yoshi_poochy_variants, ARRAY_SIZE(yoshi_poochy_variants)},

	// The Legend of Zelda (character series = 0x010)
	{0x0100, _RP("Link"), tloz_link_variants, ARRAY_SIZE(tloz_link_variants)},
	{0x0101, _RP("Zelda"), tloz_zelda_variants, ARRAY_SIZE(tloz_zelda_variants)},
	{0x0102, _RP("Ganondorf"), tloz_ganondorf_variants, ARRAY_SIZE(tloz_ganondorf_variants)},
	{0x0103, _RP("Midna & Wolf Link"), nullptr, 0},
	// The Legend of Zelda (character series = 0x014)
	{0x0141, _RP("Bokoblin"), nullptr, 0},

	// Animal Crossing (character series = 0x018)
	{0x0180, _RP("Villager"), nullptr, 0},
	{0x0181, _RP("Isabelle"), ac_isabelle_variants, ARRAY_SIZE(ac_isabelle_variants)},
	{0x0182, _RP("DJ KK"), nullptr, 0},
	{0x0182, _RP("K.K. Slider"), ac_kk_slider_variants, ARRAY_SIZE(ac_kk_slider_variants)},
	{0x0183, _RP("Tom Nook"), ac_tom_nook_variants, ARRAY_SIZE(ac_tom_nook_variants)},
	{0x0184, _RP("Timmy & Tommy"), nullptr, 0},
	{0x0185, _RP("Timmy"), ac_timmy_variants, ARRAY_SIZE(ac_timmy_variants)},
	{0x0186, _RP("Tommy"), ac_tommy_variants, ARRAY_SIZE(ac_tommy_variants)},
	{0x0187, _RP("Sable"), nullptr, 0},
	{0x0188, _RP("Mabel"), nullptr, 0},
	{0x0189, _RP("Labelle"), nullptr, 0},
	{0x018A, _RP("Reese"), nullptr, 0},
	{0x018B, _RP("Cyrus"), nullptr, 0},
	{0x018C, _RP("Digby"), ac_digby_variants, ARRAY_SIZE(ac_digby_variants)},
	{0x018D, _RP("Rover"), nullptr, 0},
	{0x018E, _RP("Resetti"), ac_resetti_variants, ARRAY_SIZE(ac_resetti_variants)},
	{0x018F, _RP("Don Resetti"), ac_don_resetti_variants, ARRAY_SIZE(ac_don_resetti_variants)},
	{0x0190, _RP("Brewster"), nullptr, 0},
	{0x0191, _RP("Harriet"), nullptr, 0},
	{0x0192, _RP("Blathers"), nullptr, 0},
	{0x0193, _RP("Celeste"), nullptr, 0},
	{0x0194, _RP("Kicks"), nullptr, 0},
	{0x0195, _RP("Porter"), nullptr, 0},
	{0x0196, _RP("Kapp'n"), nullptr, 0},
	{0x0197, _RP("Leilani"), nullptr, 0},
	{0x0198, _RP("Lelia"), nullptr, 0},
	{0x0199, _RP("Grams"), nullptr, 0},
	{0x019A, _RP("Chip"), nullptr, 0},
	{0x019B, _RP("Nat"), nullptr, 0},
	{0x019C, _RP("Phineas"), nullptr, 0},
	{0x019D, _RP("Copper"), nullptr, 0},
	{0x019E, _RP("Booker"), nullptr, 0},
	{0x019F, _RP("Pete"), nullptr, 0},
	{0x01A0, _RP("Pelly"), nullptr, 0},
	{0x01A1, _RP("Phyllis"), nullptr, 0},
	{0x01A2, _RP("Gulliver"), nullptr, 0},
	{0x01A3, _RP("Joan"), nullptr, 0},
	{0x01A4, _RP("Pascal"), nullptr, 0},
	{0x01A5, _RP("Katrina"), nullptr, 0},
	{0x01A6, _RP("Sahara"), nullptr, 0},
	{0x01A7, _RP("Wendell"), nullptr, 0},
	{0x01A8, _RP("Redd"), ac_redd_variants, ARRAY_SIZE(ac_redd_variants)},
	{0x01A9, _RP("Gracie"), nullptr, 0},
	{0x01AA, _RP("Lyle"), nullptr, 0},
	{0x01AB, _RP("Pave"), nullptr, 0},
	{0x01AC, _RP("Zipper"), nullptr, 0},
	{0x01AD, _RP("Jack"), nullptr, 0},
	{0x01AE, _RP("Franklin"), nullptr, 0},
	{0x01AF, _RP("Jingle"), nullptr, 0},
	{0x01B0, _RP("Tortimer"), nullptr, 0},
	{0x01B1, _RP("Dr. Shrunk"), ac_dr_shrunk_variants, ARRAY_SIZE(ac_dr_shrunk_variants)},
	{0x01B3, _RP("Blanca"), nullptr, 0},
	{0x01B4, _RP("Leif"), nullptr, 0},
	{0x01B5, _RP("Luna"), nullptr, 0},
	{0x01B5, _RP("Luna"), nullptr, 0},
	{0x01B6, _RP("Katie"), nullptr, 0},
	{0x01C1, _RP("Lottie"), ac_lottie_variants, ARRAY_SIZE(ac_lottie_variants)},
	{0x0200, _RP("Cyrano"), nullptr, 0},
	{0x0201, _RP("Antonio"), nullptr, 0},
	{0x0202, _RP("Pango"), nullptr, 0},
	{0x0203, _RP("Anabelle"), nullptr, 0},
	{0x0206, _RP("Snooty"), nullptr, 0},
	{0x0208, _RP("Annalisa"), nullptr, 0},
	{0x0209, _RP("Olaf"), nullptr, 0},
	{0x0214, _RP("Teddy"), nullptr, 0},
	{0x0215, _RP("Pinky"), nullptr, 0},
	{0x0216, _RP("Curt"), nullptr, 0},
	{0x0217, _RP("Chow"), nullptr, 0},
	{0x0219, _RP("Nate"), nullptr, 0},
	{0x021A, _RP("Groucho"), nullptr, 0},
	{0x021B, _RP("Tutu"), nullptr, 0},
	{0x021C, _RP("Ursala"), nullptr, 0},
	{0x021D, _RP("Grizzly"), nullptr, 0},
	{0x021E, _RP("Paula"), nullptr, 0},
	{0x021F, _RP("Ike"), nullptr, 0},
	{0x0220, _RP("Charlise"), nullptr, 0},
	{0x0221, _RP("Beardo"), nullptr, 0},
	{0x0222, _RP("Klaus"), nullptr, 0},
	{0x022D, _RP("Jay"), nullptr, 0},
	{0x022E, _RP("Robin"), nullptr, 0},
	{0x022F, _RP("Anchovy"), nullptr, 0},
	{0x0230, _RP("Twiggy"), nullptr, 0},
	{0x0231, _RP("Jitters"), nullptr, 0},
	{0x0232, _RP("Piper"), nullptr, 0},
	{0x0233, _RP("Admiral"), nullptr, 0},
	{0x0235, _RP("Midge"), nullptr, 0},
	{0x0238, _RP("Jacob"), nullptr, 0},
	{0x023C, _RP("Lucha"), nullptr, 0},
	{0x023D, _RP("Jacques"), nullptr, 0},
	{0x023E, _RP("Peck"), nullptr, 0},
	{0x023F, _RP("Sparro"), nullptr, 0},
	{0x024A, _RP("Angus"), nullptr, 0},
	{0x024B, _RP("Rodeo"), nullptr, 0},
	{0x024D, _RP("Stu"), nullptr, 0},
	{0x024F, _RP("T-Bone"), nullptr, 0},
	{0x024F, _RP("T-Bone"), nullptr, 0},
	{0x0251, _RP("Coach"), nullptr, 0},
	{0x0252, _RP("Vic"), nullptr, 0},
	{0x025D, _RP("Bob"), nullptr, 0},
	{0x025E, _RP("Mitzi"), nullptr, 0},
	{0x025F, _RP("Rosie"), nullptr, 0},	// amiibo Festival variant is in Page 22, amiibo series.
	{0x0260, _RP("Olivia"), nullptr, 0},
	{0x0261, _RP("Kiki"), nullptr, 0},
	{0x0262, _RP("Tangy"), nullptr, 0},
	{0x0263, _RP("Punchy"), nullptr, 0},
	{0x0264, _RP("Purrl"), nullptr, 0},
	{0x0265, _RP("Moe"), nullptr, 0},
	{0x0266, _RP("Kabuki"), nullptr, 0},
	{0x0267, _RP("Kid Cat"), nullptr, 0},
	{0x0268, _RP("Monique"), nullptr, 0},
	{0x0269, _RP("Tabby"), nullptr, 0},
	{0x026A, _RP("Stinky"), nullptr, 0},
	{0x026B, _RP("Kitty"), nullptr, 0},
	{0x026C, _RP("Tom"), nullptr, 0},
	{0x026D, _RP("Merry"), nullptr, 0},
	{0x026E, _RP("Felicity"), nullptr, 0},
	{0x026F, _RP("Lolly"), nullptr, 0},
	{0x0270, _RP("Ankha"), nullptr, 0},
	{0x0271, _RP("Rudy"), nullptr, 0},
	{0x0272, _RP("Katt"), nullptr, 0},
	{0x027D, _RP("Bluebear"), nullptr, 0},
	{0x027E, _RP("Maple"), nullptr, 0},
	{0x027F, _RP("Poncho"), nullptr, 0},
	{0x0280, _RP("Pudge"), nullptr, 0},
	{0x0281, _RP("Kody"), nullptr, 0},
	{0x0282, _RP("Stitches"), nullptr, 0},	// amiibo Festival variant is in Page 22, amiibo series.
	{0x0283, _RP("Vladimir"), nullptr, 0},
	{0x0284, _RP("Murphy"), nullptr, 0},
	{0x0286, _RP("Olive"), nullptr, 0},
	{0x0287, _RP("Cheri"), nullptr, 0},
	{0x028A, _RP("June"), nullptr, 0},
	{0x028B, _RP("Pekoe"), nullptr, 0},
	{0x028C, _RP("Chester"), nullptr, 0},
	{0x028D, _RP("Barold"), nullptr, 0},
	{0x028E, _RP("Tammy"), nullptr, 0},
	{0x028F, _RP("Marty"), ac_marty_variants, ARRAY_SIZE(ac_marty_variants)},
	{0x0299, _RP("Goose"), nullptr, 0},
	{0x029A, _RP("Benedict"), nullptr, 0},
	{0x029B, _RP("Egbert"), nullptr, 0},
	{0x029E, _RP("Ava"), nullptr, 0},
	{0x02A2, _RP("Becky"), nullptr, 0},
	{0x02A3, _RP("Plucky"), nullptr, 0},
	{0x02A4, _RP("Knox"), nullptr, 0},
	{0x02A5, _RP("Broffina"), nullptr, 0},
	{0x02A6, _RP("Ken"), nullptr, 0},
	{0x02B1, _RP("Patty"), nullptr, 0},
	{0x02B2, _RP("Tipper"), nullptr, 0},
	{0x02B7, _RP("Norma"), nullptr, 0},
	{0x02B8, _RP("Naomi"), nullptr, 0},
	{0x02C3, _RP("Alfonso"), nullptr, 0},
	{0x02C4, _RP("Alli"), nullptr, 0},
	{0x02C5, _RP("Boots"), nullptr, 0},
	{0x02C7, _RP("Del"), nullptr, 0},
	{0x02C9, _RP("Sly"), nullptr, 0},
	{0x02CA, _RP("Gayle"), nullptr, 0},
	{0x02CB, _RP("Drago"), nullptr, 0},
	{0x02D6, _RP("Fauna"), nullptr, 0},
	{0x02D7, _RP("Bam"), nullptr, 0},
	{0x02D8, _RP("Zell"), nullptr, 0},
	{0x02D9, _RP("Bruce"), nullptr, 0},
	{0x02DA, _RP("Deirdre"), nullptr, 0},
	{0x02DB, _RP("Lopez"), nullptr, 0},
	{0x02DC, _RP("Fuchsia"), nullptr, 0},
	{0x02DD, _RP("Beau"), nullptr, 0},
	{0x02DE, _RP("Diana"), nullptr, 0},
	{0x02DF, _RP("Erik"), nullptr, 0},
	{0x02E0, _RP("Chelsea"), ac_chelsea_variants, ARRAY_SIZE(ac_chelsea_variants)},
	{0x02EA, _RP("Goldie"), nullptr, 0},	// amiibo Festival variant is in Page 22, amiibo series.
	{0x02EB, _RP("Butch"), nullptr, 0},
	{0x02EC, _RP("Lucky"), nullptr, 0},
	{0x02ED, _RP("Biskit"), nullptr, 0},
	{0x02EE, _RP("Bones"), nullptr, 0},
	{0x02EF, _RP("Portia"), nullptr, 0},
	{0x02F0, _RP("Joan"), nullptr, 0},
	{0x02F0, _RP("Walker"), nullptr, 0},
	{0x02F1, _RP("Daisy"), nullptr, 0},
	{0x02F2, _RP("Cookie"), nullptr, 0},
	{0x02F3, _RP("Maddie"), nullptr, 0},
	{0x02F4, _RP("Bea"), nullptr, 0},
	{0x02F8, _RP("Mac"), nullptr, 0},
	{0x02F9, _RP("Marcel"), nullptr, 0},
	{0x02FA, _RP("Benjamin"), nullptr, 0},
	{0x02FB, _RP("Cherry"), nullptr, 0},
	{0x02FC, _RP("Shep"), nullptr, 0},
	{0x0307, _RP("Bill"), nullptr, 0},
	{0x0307, _RP("Bill"), nullptr, 0},
	{0x0308, _RP("Joey"), nullptr, 0},
	{0x0309, _RP("Pate"), nullptr, 0},
	{0x030A, _RP("Maelle"), nullptr, 0},
	{0x030B, _RP("Deena"), nullptr, 0},
	{0x030C, _RP("Pompom"), nullptr, 0},
	{0x030D, _RP("Mallary"), nullptr, 0},
	{0x030E, _RP("Freckles"), nullptr, 0},
	{0x030F, _RP("Derwin"), nullptr, 0},
	{0x0310, _RP("Drake"), nullptr, 0},
	{0x0311, _RP("Scoot"), nullptr, 0},
	{0x0312, _RP("Weber"), nullptr, 0},
	{0x0313, _RP("Miranda"), nullptr, 0},
	{0x0314, _RP("Ketchup"), nullptr, 0},
	{0x0316, _RP("Gloria"), nullptr, 0},
	{0x0317, _RP("Molly"), nullptr, 0},
	{0x0318, _RP("Quillson"), nullptr, 0},
	{0x0323, _RP("Opal"), nullptr, 0},
	{0x0324, _RP("Dizzy"), nullptr, 0},
	{0x0325, _RP("Big Top"), nullptr, 0},
	{0x0326, _RP("Eloise"), nullptr, 0},
	{0x0327, _RP("Margie"), nullptr, 0},
	{0x0328, _RP("Paolo"), nullptr, 0},
	{0x0329, _RP("Axel"), nullptr, 0},
	{0x032A, _RP("Ellie"), nullptr, 0},
	{0x032C, _RP("Tucker"), nullptr, 0},
	{0x032D, _RP("Tia"), nullptr, 0},
	{0x032E, _RP("Chai"), ac_chai_variants, ARRAY_SIZE(ac_chai_variants)},
	{0x0338, _RP("Lily"), nullptr, 0},
	{0x0339, _RP("Ribbot"), nullptr, 0},
	{0x033A, _RP("Frobert"), nullptr, 0},
	{0x033B, _RP("Camofrog"), nullptr, 0},
	{0x033C, _RP("Drift"), nullptr, 0},
	{0x033D, _RP("Wart Jr."), nullptr, 0},
	{0x033E, _RP("Puddles"), nullptr, 0},
	{0x033F, _RP("Jeremiah"), nullptr, 0},
	{0x0341, _RP("Tad"), nullptr, 0},
	{0x0342, _RP("Cousteau"), nullptr, 0},
	{0x0343, _RP("Huck"), nullptr, 0},
	{0x0344, _RP("Prince"), nullptr, 0},
	{0x0345, _RP("Jambette"), nullptr, 0},
	{0x0347, _RP("Raddle"), nullptr, 0},
	{0x0348, _RP("Gigi"), nullptr, 0},
	{0x0349, _RP("Croque"), nullptr, 0},
	{0x034A, _RP("Diva"), nullptr, 0},
	{0x034B, _RP("Henry"), nullptr, 0},
	{0x0356, _RP("Chevre"), nullptr, 0},
	{0x0357, _RP("Nan"), nullptr, 0},
	{0x0358, _RP("Billy"), nullptr, 0},
	{0x035A, _RP("Gruff"), nullptr, 0},
	{0x035C, _RP("Velma"), nullptr, 0},
	{0x035D, _RP("Kidd"), nullptr, 0},
	{0x035E, _RP("Pashmina"), nullptr, 0},
	{0x0369, _RP("Cesar"), nullptr, 0},
	{0x036A, _RP("Peewee"), nullptr, 0},
	{0x036B, _RP("Boone"), nullptr, 0},
	{0x036D, _RP("Louie"), nullptr, 0},
	{0x036E, _RP("Boyd"), nullptr, 0},
	{0x0370, _RP("Violet"), nullptr, 0},
	{0x0371, _RP("Al"), nullptr, 0},
	{0x0372, _RP("Rocket"), nullptr, 0},
	{0x0373, _RP("Hans"), nullptr, 0},
	{0x0374, _RP("Rilla"), ac_rilla_variants, ARRAY_SIZE(ac_rilla_variants)},
	{0x037E, _RP("Hamlet"), nullptr, 0},
	{0x037F, _RP("Apple"), nullptr, 0},
	{0x0380, _RP("Graham"), nullptr, 0},
	{0x0381, _RP("Rodney"), nullptr, 0},
	{0x0382, _RP("Soleil"), nullptr, 0},
	{0x0383, _RP("Clay"), nullptr, 0},
	{0x0384, _RP("Flurry"), nullptr, 0},
	{0x0385, _RP("Hamphrey"), nullptr, 0},
	{0x0390, _RP("Rocco"), nullptr, 0},
	{0x0392, _RP("Bubbles"), nullptr, 0},
	{0x0393, _RP("Bertha"), nullptr, 0},
	{0x0394, _RP("Biff"), nullptr, 0},
	{0x0395, _RP("Bitty"), nullptr, 0},
	{0x0398, _RP("Harry"), nullptr, 0},
	{0x0399, _RP("Hippeux"), nullptr, 0},
	{0x03A4, _RP("Buck"), nullptr, 0},
	{0x03A5, _RP("Victoria"), nullptr, 0},
	{0x03A6, _RP("Savannah"), nullptr, 0},
	{0x03A7, _RP("Elmer"), nullptr, 0},
	{0x03A8, _RP("Rosco"), nullptr, 0},
	{0x03A9, _RP("Winnie"), nullptr, 0},
	{0x03AA, _RP("Ed"), nullptr, 0},
	{0x03AB, _RP("Cleo"), nullptr, 0},
	{0x03AC, _RP("Peaches"), nullptr, 0},
	{0x03AD, _RP("Annalise"), nullptr, 0},
	{0x03AE, _RP("Clyde"), nullptr, 0},
	{0x03AF, _RP("Colton"), nullptr, 0},
	{0x03B0, _RP("Papi"), nullptr, 0},
	{0x03B1, _RP("Julian"), nullptr, 0},
	{0x03BC, _RP("Yuka"), nullptr, 0},
	{0x03BD, _RP("Alice"), nullptr, 0},
	{0x03BE, _RP("Melba"), nullptr, 0},
	{0x03BF, _RP("Sydney"), nullptr, 0},
	{0x03C0, _RP("Gonzo"), nullptr, 0},
	{0x03C1, _RP("Ozzie"), nullptr, 0},
	{0x03C4, _RP("Canberra"), nullptr, 0},
	{0x03C5, _RP("Lyman"), nullptr, 0},
	{0x03C6, _RP("Eugene"), nullptr, 0},
	{0x03D1, _RP("Kitt"), nullptr, 0},
	{0x03D2, _RP("Mathilda"), nullptr, 0},
	{0x03D3, _RP("Carrie"), nullptr, 0},
	{0x03D6, _RP("Astrid"), nullptr, 0},
	{0x03D7, _RP("Sylvia"), nullptr, 0},
	{0x03D9, _RP("Walt"), nullptr, 0},
	{0x03DA, _RP("Rooney"), nullptr, 0},
	{0x03DB, _RP("Marcie"), nullptr, 0},
	{0x03E6, _RP("Bud"), nullptr, 0},
	{0x03E7, _RP("Elvis"), nullptr, 0},
	{0x03E8, _RP("Rex"), nullptr, 0},
	{0x03EA, _RP("Leopold"), nullptr, 0},
	{0x03EC, _RP("Mott"), nullptr, 0},
	{0x03ED, _RP("Rory"), nullptr, 0},
	{0x03EE, _RP("Lionel"), nullptr, 0},
	{0x03FA, _RP("Nana"), nullptr, 0},
	{0x03FB, _RP("Simon"), nullptr, 0},
	{0x03FC, _RP("Tammi"), nullptr, 0},
	{0x03FD, _RP("Monty"), nullptr, 0},
	{0x03FE, _RP("Elise"), nullptr, 0},
	{0x03FF, _RP("Flip"), nullptr, 0},
	{0x0400, _RP("Shari"), nullptr, 0},
	{0x0401, _RP("Deli"), nullptr, 0},
	{0x040C, _RP("Dora"), nullptr, 0},
	{0x040D, _RP("Limberg"), nullptr, 0},
	{0x040E, _RP("Bella"), nullptr, 0},
	{0x040F, _RP("Bree"), nullptr, 0},
	{0x0410, _RP("Samson"), nullptr, 0},
	{0x0411, _RP("Rod"), nullptr, 0},
	{0x0414, _RP("Candi"), nullptr, 0},
	{0x0415, _RP("Rizzo"), nullptr, 0},
	{0x0416, _RP("Anicotti"), nullptr, 0},
	{0x0418, _RP("Broccolo"), nullptr, 0},
	{0x041A, _RP("Moose"), nullptr, 0},
	{0x041B, _RP("Bettina"), nullptr, 0},
	{0x041C, _RP("Greta"), nullptr, 0},
	{0x041D, _RP("Penelope"), nullptr, 0},
	{0x041E, _RP("Chadder"), nullptr, 0},
	{0x0429, _RP("Octavian"), nullptr, 0},
	{0x042A, _RP("Marina"), nullptr, 0},
	{0x042B, _RP("Zucker"), nullptr, 0},
	{0x0436, _RP("Queenie"), nullptr, 0},
	{0x0437, _RP("Gladys"), nullptr, 0},
	{0x0438, _RP("Sandy"), nullptr, 0},
	{0x0439, _RP("Sprocket"), nullptr, 0},
	{0x043B, _RP("Julia"), nullptr, 0},
	{0x043C, _RP("Cranston"), nullptr, 0},
	{0x043D, _RP("Phil"), nullptr, 0},
	{0x043E, _RP("Blanche"), nullptr, 0},
	{0x043F, _RP("Flora"), nullptr, 0},
	{0x0440, _RP("Phoebe"), nullptr, 0},
	{0x044B, _RP("Apollo"), nullptr, 0},
	{0x044C, _RP("Amelia"), nullptr, 0},
	{0x044D, _RP("Pierce"), nullptr, 0},
	{0x044E, _RP("Buzz"), nullptr, 0},
	{0x0450, _RP("Avery"), nullptr, 0},
	{0x0451, _RP("Frank"), nullptr, 0},
	{0x0452, _RP("Sterling"), nullptr, 0},
	{0x0453, _RP("Keaton"), nullptr, 0},
	{0x0454, _RP("Celia"), nullptr, 0},
	{0x045F, _RP("Aurora"), nullptr, 0},
	{0x0460, _RP("Joan"), nullptr, 0},
	{0x0460, _RP("Roald"), nullptr, 0},
	{0x0461, _RP("Cube"), nullptr, 0},
	{0x0462, _RP("Hopper"), nullptr, 0},
	{0x0463, _RP("Friga"), nullptr, 0},
	{0x0464, _RP("Gwen"), nullptr, 0},
	{0x0465, _RP("Puck"), nullptr, 0},
	{0x0468, _RP("Wade"), nullptr, 0},
	{0x0469, _RP("Boomer"), nullptr, 0},
	{0x046A, _RP("Iggly"), nullptr, 0},
	{0x046B, _RP("Tex"), nullptr, 0},
	{0x046C, _RP("Flo"), nullptr, 0},
	{0x046D, _RP("Sprinkle"), nullptr, 0},
	{0x0478, _RP("Curly"), nullptr, 0},
	{0x0479, _RP("Truffles"), nullptr, 0},
	{0x047A, _RP("Rasher"), nullptr, 0},
	{0x047B, _RP("Hugh"), nullptr, 0},
	{0x047C, _RP("Lucy"), nullptr, 0},
	{0x047D, _RP("Spork/Crackle"), nullptr, 0},
	{0x0480, _RP("Cobb"), nullptr, 0},
	{0x0481, _RP("Boris"), nullptr, 0},
	{0x0482, _RP("Maggie"), nullptr, 0},
	{0x0483, _RP("Peggy"), nullptr, 0},
	{0x0485, _RP("Gala"), nullptr, 0},
	{0x0486, _RP("Chops"), nullptr, 0},
	{0x0487, _RP("Kevin"), nullptr, 0},
	{0x0488, _RP("Pancetti"), nullptr, 0},
	{0x0489, _RP("Agnes"), nullptr, 0},
	{0x0494, _RP("Bunnie"), nullptr, 0},
	{0x0495, _RP("Dotty"), nullptr, 0},
	{0x0496, _RP("Coco"), nullptr, 0},
	{0x0497, _RP("Snake"), nullptr, 0},
	{0x0498, _RP("Gaston"), nullptr, 0},
	{0x0499, _RP("Gabi"), nullptr, 0},
	{0x049A, _RP("Pippy"), nullptr, 0},
	{0x049B, _RP("Tiffany"), nullptr, 0},
	{0x049C, _RP("Genji"), nullptr, 0},
	{0x049D, _RP("Ruby"), nullptr, 0},
	{0x049E, _RP("Doc"), nullptr, 0},
	{0x049F, _RP("Claude"), nullptr, 0},
	{0x04A0, _RP("Francine"), nullptr, 0},
	{0x04A1, _RP("Chrissy"), nullptr, 0},
	{0x04A2, _RP("Hopkins"), nullptr, 0},
	{0x04A3, _RP("OHare"), nullptr, 0},
	{0x04A4, _RP("Carmen"), nullptr, 0},
	{0x04A5, _RP("Bonbon"), nullptr, 0},
	{0x04A6, _RP("Cole"), nullptr, 0},
	{0x04A7, _RP("Mira"), nullptr, 0},
	{0x04A8, _RP("Toby"), ac_toby_variants, ARRAY_SIZE(ac_toby_variants)},
	{0x04B2, _RP("Tank"), nullptr, 0},
	{0x04B3, _RP("Rhonda"), nullptr, 0},
	{0x04B4, _RP("Spike"), nullptr, 0},
	{0x04B6, _RP("Hornsby"), nullptr, 0},
	{0x04B9, _RP("Merengue"), nullptr, 0},
#ifdef RP_UTF16
	{0x04BA, _RP("Ren\u00E9e"), nullptr, 0},
#else /* RP_UTF8 */
	// FIXME: MSVC 2010 interprets \xA9e as 2718 because
	// it's too dumb to realize \x takes *two* nybbles.
	{0x04BA, _RP("Ren\303\251e"), nullptr, 0},
#endif
	{0x04C5, _RP("Vesta"), nullptr, 0},
	{0x04C6, _RP("Baabara"), nullptr, 0},
	{0x04C7, _RP("Eunice"), nullptr, 0},
	{0x04C8, _RP("Stella"), nullptr, 0},
	{0x04C9, _RP("Cashmere"), nullptr, 0},
	{0x04CC, _RP("Willow"), nullptr, 0},
	{0x04CD, _RP("Curlos"), nullptr, 0},
	{0x04CE, _RP("Wendy"), nullptr, 0},
	{0x04CF, _RP("Timbra"), nullptr, 0},
	{0x04D0, _RP("Frita"), nullptr, 0},
	{0x04D1, _RP("Muffy"), nullptr, 0},
	{0x04D2, _RP("Pietro"), nullptr, 0},
#ifdef RP_UTF16
	{0x04D3, _RP("\u00C9toile"), ac_etoile_variants, ARRAY_SIZE(ac_etoile_variants)},
#else /* RP_UTF8 */
	{0x04D3, _RP("\xC3\x89toile"), ac_etoile_variants, ARRAY_SIZE(ac_etoile_variants)},
#endif
	{0x04DD, _RP("Peanut"), nullptr, 0},
	{0x04DE, _RP("Blaire"), nullptr, 0},
	{0x04DF, _RP("Filbert"), nullptr, 0},
	{0x04E0, _RP("Pecan"), nullptr, 0},
	{0x04E1, _RP("Nibbles"), nullptr, 0},
	{0x04E2, _RP("Agent S"), nullptr, 0},
	{0x04E3, _RP("Caroline"), nullptr, 0},
	{0x04E4, _RP("Sally"), nullptr, 0},
	{0x04E5, _RP("Static"), nullptr, 0},
	{0x04E6, _RP("Mint"), nullptr, 0},
	{0x04E7, _RP("Ricky"), nullptr, 0},
	{0x04E8, _RP("Cally"), nullptr, 0},
	{0x04EA, _RP("Tasha"), nullptr, 0},
	{0x04EB, _RP("Sylvana"), nullptr, 0},
	{0x04EC, _RP("Poppy"), nullptr, 0},
	{0x04ED, _RP("Sheldon"), nullptr, 0},
	{0x04EE, _RP("Marshal"), nullptr, 0},
	{0x04EF, _RP("Hazel"), nullptr, 0},
	{0x04FA, _RP("Rolf"), nullptr, 0},
	{0x04FB, _RP("Rowan"), nullptr, 0},
	{0x04FC, _RP("Tybalt"), nullptr, 0},
	{0x04FD, _RP("Bangle"), nullptr, 0},
	{0x04FE, _RP("Leonardo"), nullptr, 0},
	{0x04FF, _RP("Claudia"), nullptr, 0},
	{0x0500, _RP("Bianca"), nullptr, 0},
	{0x050B, _RP("Chief"), nullptr, 0},
	{0x050C, _RP("Lobo"), nullptr, 0},
	{0x050D, _RP("Wolfgang"), nullptr, 0},
	{0x050E, _RP("Whitney"), nullptr, 0},
	{0x050F, _RP("Dobie"), nullptr, 0},
	{0x0510, _RP("Freya"), nullptr, 0},
	{0x0511, _RP("Fang"), nullptr, 0},
	{0x0513, _RP("Vivian"), nullptr, 0},
	{0x0514, _RP("Skye"), nullptr, 0},
	{0x0515, _RP("Kyle"), nullptr, 0},

	// Star Fox (character series = 0x058)
	{0x0580, _RP("Fox"), nullptr, 0},
	{0x0581, _RP("Falco"), nullptr, 0},

	// Metroid (character series = 0x05C)
	{0x05C0, _RP("Samus"), metroid_samus_variants, ARRAY_SIZE(metroid_samus_variants)},

	// F-Zero (character series = 0x060)
	{0x0600, _RP("Captain Falcon"), nullptr, 0},

	// Pikmin (character series = 0x064)
	{0x0640, _RP("Olimar"), pikmin_olimar_variants, ARRAY_SIZE(pikmin_olimar_variants)},

	// Punch-Out!! (character series = 0x06C)
	{0x06C0, _RP("Little Mac"), nullptr, 0},

	// Wii Fit (character series = 0x070)
	{0x0700, _RP("Wii Fit Trainer"), nullptr, 0},

	// Kid Icarus (character series = 0x074)
	{0x0740, _RP("Pit"), nullptr, 0},
	{0x0741, _RP("Dark Pit"), nullptr, 0},
	{0x0742, _RP("Palutena"), nullptr, 0},

	// Classic Nintendo (character series = 0x078)
	{0x0780, _RP("Mr. Game & Watch"), nullptr, 0},
	{0x0781, _RP("R.O.B."), nullptr, 0},	// NES/Famicom variant is in Page 22, amiibo series.
	{0x0782, _RP("Duck Hunt"), nullptr, 0},

	// Mii (character series = 0x07C)
	{0x07C0, _RP("Mii Brawler"), mii_variants, ARRAY_SIZE(mii_variants)},

	// Splatoon (character series = 0x080)
	{0x0800, _RP("Inkling"), splatoon_inkling_variants, ARRAY_SIZE(splatoon_inkling_variants)},
	{0x0801, _RP("Callie"), nullptr, 0},
	{0x0802, _RP("Marie"), nullptr, 0},

	// Mario Sports Superstars (character series = 0x09C)
	{0x09C0, _RP("Mario"), mss_mario_variants, ARRAY_SIZE(mss_mario_variants)},
	{0x09C1, _RP("Luigi"), mss_luigi_variants, ARRAY_SIZE(mss_luigi_variants)},
	{0x09C2, _RP("Peach"), mss_peach_variants, ARRAY_SIZE(mss_peach_variants)},
	{0x09C3, _RP("Daisy"), mss_daisy_variants, ARRAY_SIZE(mss_daisy_variants)},
	{0x09C4, _RP("Yoshi"), mss_yoshi_variants, ARRAY_SIZE(mss_yoshi_variants)},
	{0x09C5, _RP("Wario"), mss_wario_variants, ARRAY_SIZE(mss_wario_variants)},
	{0x09C6, _RP("Waluigi"), mss_waluigi_variants, ARRAY_SIZE(mss_waluigi_variants)},
	{0x09C7, _RP("Donkey Kong"), mss_donkey_kong_variants, ARRAY_SIZE(mss_donkey_kong_variants)},
	{0x09C8, _RP("Diddy Kong"), mss_diddy_kong_variants, ARRAY_SIZE(mss_diddy_kong_variants)},
	{0x09C9, _RP("Bowser"), mss_bowser_variants, ARRAY_SIZE(mss_bowser_variants)},
	{0x09CA, _RP("Bowser Jr."), mss_bowser_jr_variants, ARRAY_SIZE(mss_bowser_jr_variants)},
	{0x09CB, _RP("Boo"), mss_boo_variants, ARRAY_SIZE(mss_boo_variants)},
	{0x09CC, _RP("Baby Mario"), mss_baby_mario_variants, ARRAY_SIZE(mss_baby_mario_variants)},
	{0x09CD, _RP("Baby Luigi"), mss_baby_luigi_variants, ARRAY_SIZE(mss_baby_luigi_variants)},
	{0x09CE, _RP("Birdo"), mss_birdo_variants, ARRAY_SIZE(mss_birdo_variants)},
	{0x09CF, _RP("Rosalina"), mss_rosalina_variants, ARRAY_SIZE(mss_rosalina_variants)},
	{0x09D0, _RP("Metal Mario"), mss_metal_mario_variants, ARRAY_SIZE(mss_metal_mario_variants)},
	{0x09D1, _RP("Pink Gold Peach"), mss_pink_gold_peach_variants, ARRAY_SIZE(mss_pink_gold_peach_variants)},

	// Pokémon (character series = 0x190 - 0x1BC)
	{0x1900+  6, _RP("Charizard"), nullptr, 0},
	{0x1900+ 25, _RP("Pikachu"), nullptr, 0},
	{0x1900+ 39, _RP("Jigglypuff"), nullptr, 0},
	{0x1900+150, _RP("Mewtwo"), nullptr, 0},
	{0x1900+448, _RP("Lucario"), nullptr, 0},
	{0x1900+658, _RP("Greninja"), nullptr, 0},

	// Pokkén Tournament (character series = 0x1D00)
	{0x1D00, _RP("Shadow Mewtwo"), nullptr, 0},

	// Kirby (character series = 0x1F0)
	{0x1F00, _RP("Kirby"), nullptr, 0},
	{0x1F01, _RP("Meta Knight"), nullptr, 0},
	{0x1F02, _RP("King Dedede"), nullptr, 0},
	{0x1F03, _RP("Waddle Dee"), nullptr, 0},

	// BoxBoy! (character series = 0x1F4)
	{0x1F40, _RP("Qbby"), nullptr, 0},

	// Fire Emblem (character series = 0x210)
	{0x2100, _RP("Marth"), nullptr, 0},
	{0x2101, _RP("Ike"), nullptr, 0},
	{0x2102, _RP("Lucina"), nullptr, 0},
	{0x2103, _RP("Robin"), nullptr, 0},
	{0x2104, _RP("Roy"), nullptr, 0},

	// Xenoblade (character series = 0x224)
	{0x2240, _RP("Shulk"), nullptr, 0},

	// Earthbound (character series = 0x228)
	{0x2280, _RP("Ness"), nullptr, 0},
	{0x2281, _RP("Lucas"), nullptr, 0},

	// Chibi-Robo! (character series = 0x22C)
	{0x22C0, _RP("Chibi Robo"), nullptr, 0},

	// Sonic the Hedgehog (character series = 0x320)
	{0x3200, _RP("Sonic"), nullptr, 0},

	// Pac-Man (character series = 0x334)
	{0x3340, _RP("Pac-Man"), nullptr, 0},

	// Mega Man (character series = 0x348)
	{0x3480, _RP("Mega Man"), nullptr, 0},

	// Street Fighter (character series = 0x34C)
	{0x34C0, _RP("Ryu"), nullptr, 0},

	// Monster Hunter (character series = 0x350)
	{0x3500, _RP("One-Eyed Rathalos and Rider"), mh_rathalos_variants, ARRAY_SIZE(mh_rathalos_variants)},
	{0x3501, _RP("Nabiru"), nullptr, 0},
	{0x3502, _RP("Rathian and Cheval"), mh_rathian_cheval_variants, ARRAY_SIZE(mh_rathian_cheval_variants)},
	{0x3503, _RP("Barioth and Ayuria"), mh_barioth_ayuria_variants, ARRAY_SIZE(mh_barioth_ayuria_variants)},
	{0x3504, _RP("Qurupeco and Dan"), mh_qurupeco_dan_variants, ARRAY_SIZE(mh_qurupeco_dan_variants)},

	// Shovel Knight (character series = 0x35C)
	{0x35C0, _RP("Shovel Knight"), nullptr, 0},
};

/**
 * char_id_t bsearch() comparison function.
 * @param a
 * @param b
 * @return
 */
int AmiiboDataPrivate::char_id_t_compar(const void *a, const void *b)
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
const rp_char *const AmiiboDataPrivate::amiibo_series_names[] = {
	_RP("Super Smash Bros."),			// 0x00
	_RP("Super Mario Bros."),			// 0x01
	_RP("Chibi Robo!"),				// 0x02
	_RP("Yarn Yoshi"),				// 0x03
	_RP("Splatoon"),				// 0x04
	_RP("Animal Crossing"),				// 0x05
	_RP("Super Mario Bros. 30th Anniversary"),	// 0x06
	_RP("Skylanders"),				// 0x07
	nullptr,					// 0x08
	_RP("The Legend of Zelda"),			// 0x09
	_RP("Shovel Knight"),				// 0x0A
	nullptr,					// 0x0B
	_RP("Kirby"),					// 0x0C
#ifdef RP_UTF16
	_RP("Pokk\u00E9n Tournament"), 			// 0x0D
#else /* RP_UTF8 */
	_RP("Pokk\xC3\xA9n Tournament"),		// 0x0D
#endif
	nullptr,					// 0x0E
	_RP("Monster Hunter"),				// 0x0F
	_RP("BoxBoy!"),					// 0x10
};

// amiibo IDs.
// Index is the amiibo ID. (aaaa)
// NOTE: amiibo ID is unique across *all* amiibo,
// so we can use a single array for all series.
const AmiiboDataPrivate::amiibo_id_t AmiiboDataPrivate::amiibo_ids[] = {
	// SSB: Wave 1 [0x0000-0x000B]
	{  1, 1, _RP("Mario")},			// 0x0000
	{  2, 1, _RP("Peach")},			// 0x0001
	{  3, 1, _RP("Yoshi")},			// 0x0002
	{  4, 1, _RP("Donkey Kong")},		// 0x0003
	{  5, 1, _RP("Link")},			// 0x0004
	{  6, 1, _RP("Fox")},			// 0x0005
	{  7, 1, _RP("Samus")},			// 0x0006
	{  8, 1, _RP("Wii Fit Trainer")},	// 0x0007
	{  9, 1, _RP("Villager")},		// 0x0008
	{ 10, 1, _RP("Pikachu")},		// 0x0009
	{ 11, 1, _RP("Kirby")},			// 0x000A
	{ 12, 1, _RP("Marth")},			// 0x000B

	// SSB: Wave 2 [0x000C-0x0012]
	{ 15, 2, _RP("Luigi")},			// 0x000C
	{ 14, 2, _RP("Diddy Kong")},		// 0x000D
	{ 13, 2, _RP("Zelda")},			// 0x000E
	{ 16, 2, _RP("Little Mac")},		// 0x000F
	{ 17, 2, _RP("Pit")},			// 0x0010
	{ 21, 3, _RP("Lucario")},		// 0x0011 (Wave 3, out of order)
	{ 18, 2, _RP("Captain Falcon")},	// 0x0012

	// Waves 3+ [0x0013-0x0033]
	{ 19, 3, _RP("Rosalina & Luma")},	// 0x0013
	{ 20, 3, _RP("Bowser")},		// 0x0014
	{ 43, 6, _RP("Bowser Jr.")},		// 0x0015
	{ 22, 3, _RP("Toon Link")},		// 0x0016
	{ 23, 3, _RP("Sheik")},			// 0x0017
	{ 24, 3, _RP("Ike")},			// 0x0018
	{ 42, 6, _RP("Dr. Mario")},		// 0x0019
	{ 32, 4, _RP("Wario")},			// 0x001A
	{ 41, 6, _RP("Ganondorf")},		// 0x001B
	{ 52, 7, _RP("Falco")},			// 0x001C
	{ 40, 6, _RP("Zero Suit Samus")},	// 0x001D
	{ 44, 6, _RP("Olimar")},		// 0x001E
	{ 38, 5, _RP("Palutena")},		// 0x001F
	{ 39, 5, _RP("Dark Pit")},		// 0x0020
	{ 48, 7, _RP("Mii Brawler")},		// 0x0021
	{ 49, 7, _RP("Mii Swordfighter")},	// 0x0022
	{ 50, 7, _RP("Mii Gunner")},		// 0x0023
	{ 33, 4, _RP("Charizard")},		// 0x0024
	{ 36, 4, _RP("Greninja")},		// 0x0025
	{ 37, 4, _RP("Jigglypuff")},		// 0x0026
	{ 29, 3, _RP("Meta Knight")},		// 0x0027
	{ 28, 3, _RP("King Dedede")},		// 0x0028
	{ 31, 4, _RP("Lucina")},		// 0x0029
	{ 30, 4, _RP("Robin")},			// 0x002A
	{ 25, 3, _RP("Shulk")},			// 0x002B
	{ 34, 4, _RP("Ness")},			// 0x002C
	{ 45, 6, _RP("Mr. Game & Watch")},	// 0x002D
	{ 54, 9, _RP("R.O.B. (Famicom)")},	// 0x002E (FIXME: Localized release numbers.)
	{ 47, 6, _RP("Duck Hunt")},		// 0x002F
	{ 26, 3, _RP("Sonic")},			// 0x0030
	{ 27, 3, _RP("Mega Man")},		// 0x0031
	{ 35, 4, _RP("Pac-Man")},		// 0x0032
	{ 46, 6, _RP("R.O.B. (NES)")},		// 0x0033 (FIXME: Localized release numbers.)

	// SMB: Wave 1 [0x0034-0x0039]
	{  1, 1, _RP("Mario")},			// 0x0034
	{  4, 1, _RP("Luigi")},			// 0x0035
	{  2, 1, _RP("Peach")},			// 0x0036
	{  5, 1, _RP("Yoshi")},			// 0x0037
	{  3, 1, _RP("Toad")},			// 0x0038
	{  6, 1, _RP("Bowser")},		// 0x0039

	// Chibi-Robo!
	{  0, 0, _RP("Chibi Robo")},		// 0x003A

	// Unused [0x003B]
	{  0, 0, nullptr},			// 0x003B

	// SMB: Wave 1: Special Editions [0x003C-0x003D]
	{  7, 1, _RP("Mario (Gold Edition)")},	// 0x003C
	{  8, 1, _RP("Mario (Silver Edition)")},// 0x003D

	// Splatoon: Wave 1 [0x003E-0x0040]
	{  0, 1, _RP("Inkling Girl")},		// 0x003E
	{  0, 1, _RP("Inkling Boy")},		// 0x003F
	{  0, 1, _RP("Inkling Squid")},		// 0x0040

	// Yarn Yoshi [0x0041-0x0043]
	{  1, 0, _RP("Green Yarn Yoshi")},	// 0x0041
	{  2, 0, _RP("Pink Yarn Yoshi")},	// 0x0042
	{  3, 0, _RP("Light Blue Yarn Yoshi")},	// 0x0043

	// Animal Crossing Cards: Series 1 [0x0044-0x00A7]
	{  1, 1, _RP("Isabelle")},		// 0x0044
	{  2, 1, _RP("Tom Nook")},		// 0x0045
	{  3, 1, _RP("DJ KK")},			// 0x0046
	{  4, 1, _RP("Sable")},			// 0x0047
	{  5, 1, _RP("Kapp'n")},		// 0x0048
	{  6, 1, _RP("Resetti")},		// 0x0049
	{  7, 1, _RP("Joan")},			// 0x004A
	{  8, 1, _RP("Timmy")},			// 0x004B
	{  9, 1, _RP("Digby")},			// 0x004C
	{ 10, 1, _RP("Pascal")},		// 0x004D
	{ 11, 1, _RP("Harriet")},		// 0x004E
	{ 12, 1, _RP("Redd")},			// 0x004F
	{ 13, 1, _RP("Sahara")},		// 0x0050
	{ 14, 1, _RP("Luna")},			// 0x0051
	{ 15, 1, _RP("Tortimer")},		// 0x0052
	{ 16, 1, _RP("Lyle")},			// 0x0053
	{ 17, 1, _RP("Lottie")},		// 0x0054
	{ 18, 1, _RP("Bob")},			// 0x0055
	{ 19, 1, _RP("Fauna")},			// 0x0056
	{ 20, 1, _RP("Curt")},			// 0x0057
	{ 21, 1, _RP("Portia")},		// 0x0058
	{ 22, 1, _RP("Leonardo")},		// 0x0059
	{ 23, 1, _RP("Cheri")},			// 0x005A
	{ 24, 1, _RP("Kyle")},			// 0x005B
	{ 25, 1, _RP("Al")},			// 0x005C
#ifdef RP_UTF16
	{ 26, 1, _RP("Ren\u00E9e")},		// 0x005D
#else /* RP_UTF8 */
	// FIXME: MSVC 2010 interprets \xA9e as 2718 because
	// it's too dumb to realize \x takes *two* nybbles.
	{ 26, 1, _RP("Ren\303\251e")},		// 0x005D
#endif
	{ 27, 1, _RP("Lopez")},			// 0x005E
	{ 28, 1, _RP("Jambette")},		// 0x005F
	{ 29, 1, _RP("Rasher")},		// 0x0060
	{ 30, 1, _RP("Tiffany")},		// 0x0061
	{ 31, 1, _RP("Sheldon")},		// 0x0062
	{ 32, 1, _RP("Bluebear")},		// 0x0063
	{ 33, 1, _RP("Bill")},			// 0x0064
	{ 34, 1, _RP("Kiki")},			// 0x0065
	{ 35, 1, _RP("Deli")},			// 0x0066
	{ 36, 1, _RP("Alli")},			// 0x0067
	{ 37, 1, _RP("Kabuki")},		// 0x0068
	{ 38, 1, _RP("Patty")},			// 0x0069
	{ 39, 1, _RP("Jitters")},		// 0x006A
	{ 40, 1, _RP("Gigi")},			// 0x006B
	{ 41, 1, _RP("Quillson")},		// 0x006C
	{ 42, 1, _RP("Marcie")},		// 0x006D
	{ 43, 1, _RP("Puck")},			// 0x006E
	{ 44, 1, _RP("Shari")},			// 0x006F
	{ 45, 1, _RP("Octavian")},		// 0x0070
	{ 46, 1, _RP("Winnie")},		// 0x0071
	{ 47, 1, _RP("Knox")},			// 0x0072
	{ 48, 1, _RP("Sterling")},		// 0x0073
	{ 49, 1, _RP("Bonbon")},		// 0x0074
	{ 50, 1, _RP("Punchy")},		// 0x0075
	{ 51, 1, _RP("Opal")},			// 0x0076
	{ 52, 1, _RP("Poppy")},			// 0x0077
	{ 53, 1, _RP("Limberg")},		// 0x0078
	{ 54, 1, _RP("Deena")},			// 0x0079
	{ 55, 1, _RP("Snake")},			// 0x007A
	{ 56, 1, _RP("Bangle")},		// 0x007B
	{ 57, 1, _RP("Phil")},			// 0x007C
	{ 58, 1, _RP("Monique")},		// 0x007D
	{ 59, 1, _RP("Nate")},			// 0x007E
	{ 60, 1, _RP("Samson")},		// 0x007F
	{ 61, 1, _RP("Tutu")},			// 0x0080
	{ 62, 1, _RP("T-Bone")},		// 0x0081
	{ 63, 1, _RP("Mint")},			// 0x0082
	{ 64, 1, _RP("Pudge")},			// 0x0083
	{ 65, 1, _RP("Midge")},			// 0x0084
	{ 66, 1, _RP("Gruff")},			// 0x0085
	{ 67, 1, _RP("Flurry")},		// 0x0086
	{ 68, 1, _RP("Clyde")},			// 0x0087
	{ 69, 1, _RP("Bella")},			// 0x0088
	{ 70, 1, _RP("Biff")},			// 0x0089
	{ 71, 1, _RP("Yuka")},			// 0x008A
	{ 72, 1, _RP("Lionel")},		// 0x008B
	{ 73, 1, _RP("Flo")},			// 0x008C
	{ 74, 1, _RP("Cobb")},			// 0x008D
	{ 75, 1, _RP("Amelia")},		// 0x008E
	{ 76, 1, _RP("Jeremiah")},		// 0x008F
	{ 77, 1, _RP("Cherry")},		// 0x0090
	{ 78, 1, _RP("Rosco")},			// 0x0091
	{ 79, 1, _RP("Truffles")},		// 0x0092
	{ 80, 1, _RP("Eugene")},		// 0x0093
	{ 81, 1, _RP("Eunice")},		// 0x0094
	{ 82, 1, _RP("Goose")},			// 0x0095
	{ 83, 1, _RP("Annalisa")},		// 0x0096
	{ 84, 1, _RP("Benjamin")},		// 0x0097
	{ 85, 1, _RP("Pancetti")},		// 0x0098
	{ 86, 1, _RP("Chief")},			// 0x0099
	{ 87, 1, _RP("Bunnie")},		// 0x009A
	{ 88, 1, _RP("Clay")},			// 0x009B
	{ 89, 1, _RP("Diana")},			// 0x009C
	{ 90, 1, _RP("Axel")},			// 0x009D
	{ 91, 1, _RP("Muffy")},			// 0x009E
	{ 92, 1, _RP("Henry")},			// 0x009F
	{ 93, 1, _RP("Bertha")},		// 0x00A0
	{ 94, 1, _RP("Cyrano")},		// 0x00A1
	{ 95, 1, _RP("Peanut")},		// 0x00A2
	{ 96, 1, _RP("Cole")},			// 0x00A3
	{ 97, 1, _RP("Willow")},		// 0x00A4
	{ 98, 1, _RP("Roald")},			// 0x00A5
	{ 99, 1, _RP("Molly")},			// 0x00A6
	{100, 1, _RP("Walker")},		// 0x00A7

	// Animal Crossing Cards: Series 2 [0x00A8-0x010B]
	{101, 2, _RP("K.K. Slider")},		// 0x00A8
	{102, 2, _RP("Reese")},			// 0x00A9
	{103, 2, _RP("Kicks")},			// 0x00AA
	{104, 2, _RP("Labelle")},		// 0x00AB
	{105, 2, _RP("Copper")},		// 0x00AC
	{106, 2, _RP("Booker")},		// 0x00AD
	{107, 2, _RP("Katie")},			// 0x00AE
	{108, 2, _RP("Tommy")},			// 0x00AF
	{109, 2, _RP("Porter")},		// 0x00B0
	{110, 2, _RP("Lelia")},			// 0x00B1
	{111, 2, _RP("Dr. Shrunk")},		// 0x00B2
	{112, 2, _RP("Don Resetti")},		// 0x00B3
	{113, 2, _RP("Isabelle (Autumn Outfit)")},// 0x00B4
	{114, 2, _RP("Blanca")},		// 0x00B5
	{115, 2, _RP("Nat")},			// 0x00B6
	{116, 2, _RP("Chip")},			// 0x00B7
	{117, 2, _RP("Jack")},			// 0x00B8
	{118, 2, _RP("Poncho")},		// 0x00B9
	{119, 2, _RP("Felicity")},		// 0x00BA
	{120, 2, _RP("Ozzie")},			// 0x00BB
	{121, 2, _RP("Tia")},			// 0x00BC
	{122, 2, _RP("Lucha")},			// 0x00BD
	{123, 2, _RP("Fuchsia")},		// 0x00BE
	{124, 2, _RP("Harry")},			// 0x00BF
	{125, 2, _RP("Gwen")},			// 0x00C0
	{126, 2, _RP("Coach")},			// 0x00C1
	{127, 2, _RP("Kitt")},			// 0x00C2
	{128, 2, _RP("Tom")},			// 0x00C3
	{129, 2, _RP("Tipper")},		// 0x00C4
	{130, 2, _RP("Prince")},		// 0x00C5
	{131, 2, _RP("Pate")},			// 0x00C6
	{132, 2, _RP("Vladimir")},		// 0x00C7
	{133, 2, _RP("Savannah")},		// 0x00C8
	{134, 2, _RP("Kidd")},			// 0x00C9
	{135, 2, _RP("Phoebe")},		// 0x00CA
	{136, 2, _RP("Egbert")},		// 0x00CB
	{137, 2, _RP("Cookie")},		// 0x00CC
	{138, 2, _RP("Sly")},			// 0x00CD
	{139, 2, _RP("Blaire")},		// 0x00CE
	{140, 2, _RP("Avery")},			// 0x00CF
	{141, 2, _RP("Nana")},			// 0x00D0
	{142, 2, _RP("Peck")},			// 0x00D1
	{143, 2, _RP("Olivia")},		// 0x00D2
	{144, 2, _RP("Cesar")},			// 0x00D3
	{145, 2, _RP("Carmen")},		// 0x00D4
	{146, 2, _RP("Rodney")},		// 0x00D5
	{147, 2, _RP("Scoot")},			// 0x00D6
	{148, 2, _RP("Whitney")},		// 0x00D7
	{149, 2, _RP("Broccolo")},		// 0x00D8
	{150, 2, _RP("Coco")},			// 0x00D9
	{151, 2, _RP("Groucho")},		// 0x00DA
	{152, 2, _RP("Wendy")},			// 0x00DB
	{153, 2, _RP("Alfonso")},		// 0x00DC
	{154, 2, _RP("Rhonda")},		// 0x00DD
	{155, 2, _RP("Butch")},			// 0x00DE
	{156, 2, _RP("Gabi")},			// 0x00DF
	{157, 2, _RP("Moose")},			// 0x00E0
	{158, 2, _RP("Timbra")},		// 0x00E1
	{159, 2, _RP("Zell")},			// 0x00E2
	{160, 2, _RP("Pekoe")},			// 0x00E3
	{161, 2, _RP("Teddy")},			// 0x00E4
	{162, 2, _RP("Mathilda")},		// 0x00E5
	{163, 2, _RP("Ed")},			// 0x00E6
	{164, 2, _RP("Bianca")},		// 0x00E7
	{165, 2, _RP("Filbert")},		// 0x00E8
	{166, 2, _RP("Kitty")},			// 0x00E9
	{167, 2, _RP("Beau")},			// 0x00EA
	{168, 2, _RP("Nan")},			// 0x00EB
	{169, 2, _RP("Bud")},			// 0x00EC
	{170, 2, _RP("Ruby")},			// 0x00ED
	{171, 2, _RP("Benedict")},		// 0x00EE
	{172, 2, _RP("Agnes")},			// 0x00EF
	{173, 2, _RP("Julian")},		// 0x00F0
	{174, 2, _RP("Bettina")},		// 0x00F1
	{175, 2, _RP("Jay")},			// 0x00F2
	{176, 2, _RP("Sprinkle")},		// 0x00F3
	{177, 2, _RP("Flip")},			// 0x00F4
	{178, 2, _RP("Hugh")},			// 0x00F5
	{179, 2, _RP("Hopper")},		// 0x00F6
	{180, 2, _RP("Pecan")},			// 0x00F7
	{181, 2, _RP("Drake")},			// 0x00F8
	{182, 2, _RP("Alice")},			// 0x00F9
	{183, 2, _RP("Camofrog")},		// 0x00FA
	{184, 2, _RP("Anicotti")},		// 0x00FB
	{185, 2, _RP("Chops")},			// 0x00FC
	{186, 2, _RP("Charlise")},		// 0x00FD
	{187, 2, _RP("Vic")},			// 0x00FE
	{188, 2, _RP("Ankha")},			// 0x00FF
	{189, 2, _RP("Drift")},			// 0x0100
	{190, 2, _RP("Vesta")},			// 0x0101
	{191, 2, _RP("Marcel")},		// 0x0102
	{192, 2, _RP("Pango")},			// 0x0103
	{193, 2, _RP("Keaton")},		// 0x0104
	{194, 2, _RP("Gladys")},		// 0x0105
	{195, 2, _RP("Hamphrey")},		// 0x0106
	{196, 2, _RP("Freya")},			// 0x0107
	{197, 2, _RP("Kid Cat")},		// 0x0108
	{198, 2, _RP("Agent S")},		// 0x0109
	{199, 2, _RP("Big Top")},		// 0x010A
	{200, 2, _RP("Rocket")},		// 0x010B

	// Animal Crossing Cards: Series 3 [0x010C-0x016F]
	{201, 3, _RP("Rover")},			// 0x010C
	{202, 3, _RP("Blathers")},		// 0x010D
	{203, 3, _RP("Tom Nook")},		// 0x010E
	{204, 3, _RP("Pelly")},			// 0x010F
	{205, 3, _RP("Phyllis")},		// 0x0110
	{206, 3, _RP("Pete")},			// 0x0111
	{207, 3, _RP("Mabel")},			// 0x0112
	{208, 3, _RP("Leif")},			// 0x0113
	{209, 3, _RP("Wendell")},		// 0x0114
	{210, 3, _RP("Cyrus")},			// 0x0115
	{211, 3, _RP("Grams")},			// 0x0116
	{212, 3, _RP("Timmy")},			// 0x0117
	{213, 3, _RP("Digby")},			// 0x0118
	{214, 3, _RP("Don Resetti")},		// 0x0119
	{215, 3, _RP("Isabelle")},		// 0x011A
	{216, 3, _RP("Franklin")},		// 0x011B
	{217, 3, _RP("Jingle")},		// 0x011C
	{218, 3, _RP("Lily")},			// 0x011D
	{219, 3, _RP("Anchovy")},		// 0x011E
	{220, 3, _RP("Tabby")},			// 0x011F
	{221, 3, _RP("Kody")},			// 0x0120
	{222, 3, _RP("Miranda")},		// 0x0121
	{223, 3, _RP("Del")},			// 0x0122
	{224, 3, _RP("Paula")},			// 0x0123
	{225, 3, _RP("Ken")},			// 0x0124
	{226, 3, _RP("Mitzi")},			// 0x0125
	{227, 3, _RP("Rodeo")},			// 0x0126
	{228, 3, _RP("Bubbles")},		// 0x0127
	{229, 3, _RP("Cousteau")},		// 0x0128
	{230, 3, _RP("Velma")},			// 0x0129
	{231, 3, _RP("Elvis")},			// 0x012A
	{232, 3, _RP("Canberra")},		// 0x012B
	{233, 3, _RP("Colton")},		// 0x012C
	{234, 3, _RP("Marina")},		// 0x012D
	{235, 3, _RP("Spork/Crackle")},		// 0x012E
	{236, 3, _RP("Freckles")},		// 0x012F
	{237, 3, _RP("Bam")},			// 0x0130
	{238, 3, _RP("Friga")},			// 0x0131
	{239, 3, _RP("Ricky")},			// 0x0132
	{240, 3, _RP("Deirdre")},		// 0x0133
	{241, 3, _RP("Hans")},			// 0x0134
	{242, 3, _RP("Chevre")},		// 0x0135
	{243, 3, _RP("Drago")},			// 0x0136
	{244, 3, _RP("Tangy")},			// 0x0137
	{245, 3, _RP("Mac")},			// 0x0138
	{246, 3, _RP("Eloise")},		// 0x0139
	{247, 3, _RP("Wart Jr.")},		// 0x013A
	{248, 3, _RP("Hazel")},			// 0x013B
	{249, 3, _RP("Beardo")},		// 0x013C
	{250, 3, _RP("Ava")},			// 0x013D
	{251, 3, _RP("Chester")},		// 0x013E
	{252, 3, _RP("Merry")},			// 0x013F
	{253, 3, _RP("Genji")},			// 0x0140
	{254, 3, _RP("Greta")},			// 0x0141
	{255, 3, _RP("Wolfgang")},		// 0x0142
	{256, 3, _RP("Diva")},			// 0x0143
	{257, 3, _RP("Klaus")},			// 0x0144
	{258, 3, _RP("Daisy")},			// 0x0145
	{259, 3, _RP("Stinky")},		// 0x0146
	{260, 3, _RP("Tammi")},			// 0x0147
	{261, 3, _RP("Tucker")},		// 0x0148
	{262, 3, _RP("Blanche")},		// 0x0149
	{263, 3, _RP("Gaston")},		// 0x014A
	{264, 3, _RP("Marshal")},		// 0x014B
	{265, 3, _RP("Gala")},			// 0x014C
	{266, 3, _RP("Joey")},			// 0x014D
	{267, 3, _RP("Pippy")},			// 0x014E
	{268, 3, _RP("Buck")},			// 0x014F
	{269, 3, _RP("Bree")},			// 0x0150
	{270, 3, _RP("Rooney")},		// 0x0151
	{271, 3, _RP("Curlos")},		// 0x0152
	{272, 3, _RP("Skye")},			// 0x0153
	{273, 3, _RP("Moe")},			// 0x0154
	{274, 3, _RP("Flora")},			// 0x0155
	{275, 3, _RP("Hamlet")},		// 0x0156
	{276, 3, _RP("Astrid")},		// 0x0157
	{277, 3, _RP("Monty")},			// 0x0158
	{278, 3, _RP("Dora")},			// 0x0159
	{279, 3, _RP("Biskit")},		// 0x015A
	{280, 3, _RP("Victoria")},		// 0x015B
	{281, 3, _RP("Lyman")},			// 0x015C
	{282, 3, _RP("Violet")},		// 0x015D
	{283, 3, _RP("Frank")},			// 0x015E
	{284, 3, _RP("Chadder")},		// 0x015F
	{285, 3, _RP("Merengue")},		// 0x0160
	{286, 3, _RP("Cube")},			// 0x0161
	{287, 3, _RP("Claudia")},		// 0x0162
	{288, 3, _RP("Curly")},			// 0x0163
	{289, 3, _RP("Boomer")},		// 0x0164
	{290, 3, _RP("Caroline")},		// 0x0165
	{291, 3, _RP("Sparro")},		// 0x0166
	{292, 3, _RP("Baabara")},		// 0x0167
	{293, 3, _RP("Rolf")},			// 0x0168
	{294, 3, _RP("Maple")},			// 0x0169
	{295, 3, _RP("Antonio")},		// 0x016A
	{296, 3, _RP("Soleil")},		// 0x016B
	{297, 3, _RP("Apollo")},		// 0x016C
	{298, 3, _RP("Derwin")},		// 0x016D
	{299, 3, _RP("Francine")},		// 0x016E
	{300, 3, _RP("Chrissy")},		// 0x016F

	// Animal Crossing Cards: Series 4 [0x0170-0x01D3]
	{301, 4, _RP("Isabelle")},		// 0x0170
	{302, 4, _RP("Brewster")},		// 0x0171
	{303, 4, _RP("Katrina")},		// 0x0172
	{304, 4, _RP("Phineas")},		// 0x0173
	{305, 4, _RP("Celeste")},		// 0x0174
	{306, 4, _RP("Tommy")},			// 0x0175
	{307, 4, _RP("Gracie")},		// 0x0176
	{308, 4, _RP("Leilani")},		// 0x0177
	{309, 4, _RP("Resetti")},		// 0x0178
	{310, 4, _RP("Timmy")},			// 0x0179
	{311, 4, _RP("Lottie")},		// 0x017A
	{312, 4, _RP("Shrunk")},		// 0x017B
	{313, 4, _RP("Pave")},			// 0x017C
	{314, 4, _RP("Gulliver")},		// 0x017D
	{315, 4, _RP("Redd")},			// 0x017E
	{316, 4, _RP("Zipper")},		// 0x017F
	{317, 4, _RP("Goldie")},		// 0x0180
	{318, 4, _RP("Stitches")},		// 0x0181
	{319, 4, _RP("Pinky")},			// 0x0182
	{320, 4, _RP("Mott")},			// 0x0183
	{321, 4, _RP("Mallary")},		// 0x0184
	{322, 4, _RP("Rocco")},			// 0x0185
	{323, 4, _RP("Katt")},			// 0x0186
	{324, 4, _RP("Graham")},		// 0x0187
	{325, 4, _RP("Peaches")},		// 0x0188
	{326, 4, _RP("Dizzy")},			// 0x0189
	{327, 4, _RP("Penelope")},		// 0x018A
	{328, 4, _RP("Boone")},			// 0x018B
	{329, 4, _RP("Broffina")},		// 0x018C
	{330, 4, _RP("Croque")},		// 0x018D
	{331, 4, _RP("Pashmina")},		// 0x018E
	{332, 4, _RP("Shep")},			// 0x018F
	{333, 4, _RP("Lolly")},			// 0x0190
	{334, 4, _RP("Erik")},			// 0x0191
	{335, 4, _RP("Dotty")},			// 0x0192
	{336, 4, _RP("Pierce")},		// 0x0193
	{337, 4, _RP("Queenie")},		// 0x0194
	{338, 4, _RP("Fang")},			// 0x0195
	{339, 4, _RP("Frita")},			// 0x0196
	{340, 4, _RP("Tex")},			// 0x0197
	{341, 4, _RP("Melba")},			// 0x0198
	{342, 4, _RP("Bones")},			// 0x0199
	{343, 4, _RP("Anabelle")},		// 0x019A
	{344, 4, _RP("Rudy")},			// 0x019B
	{345, 4, _RP("Naomi")},			// 0x019C
	{346, 4, _RP("Peewee")},		// 0x019D
	{347, 4, _RP("Tammy")},			// 0x019E
	{348, 4, _RP("Olaf")},			// 0x019F
	{349, 4, _RP("Lucy")},			// 0x01A0
	{350, 4, _RP("Elmer")},			// 0x01A1
	{351, 4, _RP("Puddles")},		// 0x01A2
	{352, 4, _RP("Rory")},			// 0x01A3
	{353, 4, _RP("Elise")},			// 0x01A4
	{354, 4, _RP("Walt")},			// 0x01A5
	{355, 4, _RP("Mira")},			// 0x01A6
	{356, 4, _RP("Pietro")},		// 0x01A7
	{357, 4, _RP("Aurora")},		// 0x01A8
	{358, 4, _RP("Papi")},			// 0x01A9
	{359, 4, _RP("Apple")},			// 0x01AA
	{360, 4, _RP("Rod")},			// 0x01AB
	{361, 4, _RP("Purrl")},			// 0x01AC
	{362, 4, _RP("Static")},		// 0x01AD
	{363, 4, _RP("Celia")},			// 0x01AE
	{364, 4, _RP("Zucker")},		// 0x01AF
	{365, 4, _RP("Peggy")},			// 0x01B0
	{366, 4, _RP("Ribbot")},		// 0x01B1
	{367, 4, _RP("Annalise")},		// 0x01B2
	{368, 4, _RP("Chow")},			// 0x01B3
	{369, 4, _RP("Sylvia")},		// 0x01B4
	{370, 4, _RP("Jacques")},		// 0x01B5
	{371, 4, _RP("Sally")},			// 0x01B6
	{372, 4, _RP("Doc")},			// 0x01B7
	{373, 4, _RP("Pompom")},		// 0x01B8
	{374, 4, _RP("Tank")},			// 0x01B9
	{375, 4, _RP("Becky")},			// 0x01BA
	{376, 4, _RP("Rizzo")},			// 0x01BB
	{377, 4, _RP("Sydney")},		// 0x01BC
	{378, 4, _RP("Barold")},		// 0x01BD
	{379, 4, _RP("Nibbles")},		// 0x01BE
	{380, 4, _RP("Kevin")},			// 0x01BF
	{381, 4, _RP("Gloria")},		// 0x01C0
	{382, 4, _RP("Lobo")},			// 0x01C1
	{383, 4, _RP("Hippeux")},		// 0x01C2
	{384, 4, _RP("Margie")},		// 0x01C3
	{385, 4, _RP("Lucky")},			// 0x01C4
	{386, 4, _RP("Rosie")},			// 0x01C5
	{387, 4, _RP("Rowan")},			// 0x01C6
	{388, 4, _RP("Maelle")},		// 0x01C7
	{389, 4, _RP("Bruce")},			// 0x01C8
	{390, 4, _RP("OHare")},			// 0x01C9
	{391, 4, _RP("Gayle")},			// 0x01CA
	{392, 4, _RP("Cranston")},		// 0x01CB
	{393, 4, _RP("Frobert")},		// 0x01CC
	{394, 4, _RP("Grizzly")},		// 0x01CD
	{395, 4, _RP("Cally")},			// 0x01CE
	{396, 4, _RP("Simon")},			// 0x01CF
	{397, 4, _RP("Iggly")},			// 0x01D0
	{398, 4, _RP("Angus")},			// 0x01D1
	{399, 4, _RP("Twiggy")},		// 0x01D2
	{400, 4, _RP("Robin")},			// 0x01D3

	// Animal Crossing: Character Parfait, Amiibo Festival
	{401, 5, _RP("Isabelle (Parfait)")},		// 0x01D4
	{402, 5, _RP("Goldie (amiibo Festival)")},	// 0x01D5
	{403, 5, _RP("Stitches (amiibo Festival)")},	// 0x01D6
	{404, 5, _RP("Rosie (amiibo Festival)")},	// 0x01D7
	{405, 5, _RP("K.K. Slider (Parfait)")},		// 0x01D8

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
	{  1, 1, _RP("8-bit Mario (Classic Color)")},	// 0x0238
	{  2, 1, _RP("8-bit Mario (Modern Color)")},	// 0x0239

	// Skylanders Series [0x023A-0x023B]
	{  1, 0, _RP("Hammer Slam Bowser")},		// 0x023A
	{  2, 0, _RP("Turbo Charge Donkey Kong")},	// 0x023B
#if 0
	// NOTE: Cannot distinguish between regular and dark
	// variants in amiibo mode.
	{  3, 0, _RP("Dark Hammer Slam Bowser")},	// 0x023A
	{  4, 0, _RP("Dark Turbo Charge Donkey Kong")},	// 0x023B
#endif

	// Unused [0x023C]
	{  0, 0, nullptr},			// 0x023C

	// SSB: Mewtwo (Wave 7) [0x023D]
	{ 51, 7, _RP("Mewtwo")},		// 0x023D

	// Yarn Yoshi: Mega Yarn Yoshi [0x023E]
	{  4, 0, _RP("Mega Yarn Yoshi")},	// 0x023E

	// Animal Crossing Figurines: Wave 1 [0x023F-0x0246]
	{  0, 1, _RP("Isabelle")},		// 0x023F
	{  0, 1, _RP("K.K. Slider")},		// 0x0240
	{  0, 1, _RP("Mabel")},			// 0x0241
	{  0, 1, _RP("Tom Nook")},		// 0x0242
	{  0, 1, _RP("Digby")},			// 0x0243
	{  0, 1, _RP("Lottie")},		// 0x0244
	{  0, 1, _RP("Reese")},			// 0x0245
	{  0, 1, _RP("Cyrus")},			// 0x0246

	// Animal Crossing Figurines: Wave 2 [0x0247-0x024A]
	{  0, 2, _RP("Blathers")},		// 0x0247
	{  0, 2, _RP("Celeste")},		// 0x0248
	{  0, 2, _RP("Resetti")},		// 0x0249
	{  0, 2, _RP("Kicks")},			// 0x024A

	// Animal Crossing Figurines: Wave 4 (out of order) [0x024B]
	{  0, 4, _RP("Isabelle (Summer Outfit)")},// 0x024B

	// Animal Crossing Figurines: Wave 3 [0x024C-0x024E]
	{  0, 3, _RP("Rover")},			// 0x024C
	{  0, 3, _RP("Timmy & Tommy")},		// 0x024D
	{  0, 3, _RP("Kapp'n")},		// 0x024E

	// The Legend of Zelda: Twilight Princess [0x024F]
	{  0, 1, _RP("Midna & Wolf Link")},	// 0x024F

	// Shovel Knight [0x0250]
	{  0, 0, _RP("Shovel Knight")},		// 0x0250

	// SSB: DLC characters (Waves 8+)
	{ 53, 8, _RP("Lucas")},			// 0x0251
	{ 55, 9, _RP("Roy")},			// 0x0252
	{ 56, 9, _RP("Ryu")},			// 0x0253

	// Kirby [0x0254-0x0257]
	{  0, 0, _RP("Kirby")},			// 0x0254
	{  0, 0, _RP("Meta Knight")},		// 0x0255
	{  0, 0, _RP("King Dedede")},		// 0x0256
	{  0, 0, _RP("Waddle Dee")},		// 0x0257

	// SSB: Special amiibo [0x0258]
	{  0, 0, _RP("Mega Man (Gold Edition)")},// 0x0258

	// Unused [0x0259-0x025B]
	{  0, 0, nullptr},			// 0x0259
	{  0, 0, nullptr}, {  0, 0, nullptr},	// 0x025A,0x025B

	// Pokkén Tournament [0x025C]
	{  0, 0, _RP("Shadow Mewtwo")},		// 0x025C

	// Splatoon: Wave 2 [0x025D-0x0261]
	{  0, 2, _RP("Callie")},		// 0x025D
	{  0, 2, _RP("Marie")},			// 0x025E
	{  0, 2, _RP("Inkling Girl (Lime Green)")},// 0x025F
	{  0, 2, _RP("Inkling Boy (Purple)")},	// 0x0260
	{  0, 2, _RP("Inkling Squid (Orange)")},// 0x0261

	// SMB: Wave 2 [0x0262-0x0268]
	{ 12, 2, _RP("Rosalina")},		// 0x0262
	{  9, 2, _RP("Wario")},			// 0x0263
	{ 13, 2, _RP("Donkey Kong")},		// 0x0264
	{ 14, 2, _RP("Diddy Kong")},		// 0x0265
	{ 11, 2, _RP("Daisy")},			// 0x0266
	{ 10, 2, _RP("Waluigi")},		// 0x0267
	{ 15, 2, _RP("Boo")},			// 0x0268

	// Mario Sports Superstars Cards [0x0269-0x02C2]
	{  1,  1, _RP("Mario (Soccer)")},		// 0x0269
	{  2,  1, _RP("Mario (Baseball)")},		// 0x026A
	{  3,  1, _RP("Mario (Tennis)")},		// 0x026B
	{  4,  1, _RP("Mario (Golf)")},			// 0x026C
	{  5,  1, _RP("Mario (Horse Racing)")},		// 0x026D
	{  6,  1, _RP("Luigi (Soccer)")},		// 0x026E
	{  7,  1, _RP("Luigi (Baseball)")},		// 0x026F
	{  8,  1, _RP("Luigi (Tennis)")},		// 0x0270
	{  9,  1, _RP("Luigi (Golf)")},			// 0x0271
	{ 10,  1, _RP("Luigi (Horse Racing)")},		// 0x0272
	{ 11,  1, _RP("Peach (Soccer)")},		// 0x0273
	{ 12,  1, _RP("Peach (Baseball)")},		// 0x0274
	{ 13,  1, _RP("Peach (Tennis)")},		// 0x0275
	{ 14,  1, _RP("Peach (Golf)")},			// 0x0276
	{ 15,  1, _RP("Peach (Horse Racing)")},		// 0x0277
	{ 16,  1, _RP("Daisy (Soccer)")},		// 0x0278
	{ 17,  1, _RP("Daisy (Baseball)")},		// 0x0279
	{ 18,  1, _RP("Daisy (Tennis)")},		// 0x027A
	{ 19,  1, _RP("Daisy (Golf)")},			// 0x027B
	{ 20,  1, _RP("Daisy (Horse Racing)")},		// 0x027C
	{ 21,  1, _RP("Yoshi (Soccer)")},		// 0x027D
	{ 22,  1, _RP("Yoshi (Baseball)")},		// 0x027E
	{ 23,  1, _RP("Yoshi (Tennis)")},		// 0x027F
	{ 24,  1, _RP("Yoshi (Golf)")},			// 0x0280
	{ 25,  1, _RP("Yoshi (Horse Racing)")},		// 0x0281
	{ 26,  1, _RP("Wario (Soccer)")},		// 0x0282
	{ 27,  1, _RP("Wario (Baseball)")},		// 0x0283
	{ 28,  1, _RP("Wario (Tennis)")},		// 0x0284
	{ 29,  1, _RP("Wario (Golf)")},			// 0x0285
	{ 30,  1, _RP("Wario (Horse Racing)")},		// 0x0286
	{ 31,  1, _RP("Waluigi (Soccer)")},		// 0x0287
	{ 32,  1, _RP("Waluigi (Baseball)")},		// 0x0288
	{ 33,  1, _RP("Waluigi (Tennis)")},		// 0x0289
	{ 34,  1, _RP("Waluigi (Golf)")},		// 0x028A
	{ 35,  1, _RP("Waluigi (Horse Racing)")},	// 0x028B
	{ 36,  1, _RP("Donkey Kong (Soccer)")},		// 0x028C
	{ 37,  1, _RP("Donkey Kong (Baseball)")},	// 0x028D
	{ 38,  1, _RP("Donkey Kong (Tennis)")},		// 0x028E
	{ 39,  1, _RP("Donkey Kong (Golf)")},		// 0x028F
	{ 40,  1, _RP("Donkey Kong (Horse Racing)")},	// 0x0290
	{ 41,  1, _RP("Diddy Kong (Soccer)")},		// 0x0291
	{ 42,  1, _RP("Diddy Kong (Baseball)")},	// 0x0292
	{ 43,  1, _RP("Diddy Kong (Tennis)")},		// 0x0293
	{ 44,  1, _RP("Diddy Kong (Golf)")},		// 0x0294
	{ 45,  1, _RP("Diddy Kong (Horse Racing)")},	// 0x0295
	{ 46,  1, _RP("Bowser (Soccer)")},		// 0x0296
	{ 47,  1, _RP("Bowser (Baseball)")},		// 0x0297
	{ 48,  1, _RP("Bowser (Tennis)")},		// 0x0298
	{ 49,  1, _RP("Bowser (Golf)")},		// 0x0299
	{ 50,  1, _RP("Bowser (Horse Racing)")},	// 0x029A
	{ 51,  1, _RP("Bowser Jr. (Soccer)")},		// 0x029B
	{ 52,  1, _RP("Bowser Jr. (Baseball)")},	// 0x029C
	{ 53,  1, _RP("Bowser Jr. (Tennis)")},		// 0x029D
	{ 54,  1, _RP("Bowser Jr. (Golf)")},		// 0x029E
	{ 55,  1, _RP("Bowser Jr. (Horse Racing)")},	// 0x029F
	{ 56,  1, _RP("Boo (Soccer)")},			// 0x02A0
	{ 57,  1, _RP("Boo (Baseball)")},		// 0x02A1
	{ 58,  1, _RP("Boo (Tennis)")},			// 0x02A2
	{ 59,  1, _RP("Boo (Golf)")},			// 0x02A3
	{ 60,  1, _RP("Boo (Horse Racing)")},		// 0x02A4
	{ 61,  1, _RP("Baby Mario (Soccer)")},		// 0x02A5
	{ 62,  1, _RP("Baby Mario (Baseball)")},	// 0x02A6
	{ 63,  1, _RP("Baby Mario (Tennis)")},		// 0x02A7
	{ 64,  1, _RP("Baby Mario (Golf)")},		// 0x02A8
	{ 65,  1, _RP("Baby Mario (Horse Racing)")},	// 0x02A9
	{ 66,  1, _RP("Baby Luigi (Soccer)")},		// 0x02AA
	{ 67,  1, _RP("Baby Luigi (Baseball)")},	// 0x02AB
	{ 68,  1, _RP("Baby Luigi (Tennis)")},		// 0x02AC
	{ 69,  1, _RP("Baby Luigi (Golf)")},		// 0x02AD
	{ 70,  1, _RP("Baby Luigi (Horse Racing)")},	// 0x02AE
	{ 71,  1, _RP("Birdo (Soccer)")},		// 0x02AF
	{ 72,  1, _RP("Birdo (Baseball)")},		// 0x02B0
	{ 73,  1, _RP("Birdo (Tennis)")},		// 0x02B1
	{ 74,  1, _RP("Birdo (Golf)")},			// 0x02B2
	{ 75,  1, _RP("Birdo (Horse Racing)")},		// 0x02B3
	{ 76,  1, _RP("Rosalina (Soccer)")},		// 0x02B4
	{ 77,  1, _RP("Rosalina (Baseball)")},		// 0x02B5
	{ 78,  1, _RP("Rosalina (Tennis)")},		// 0x02B6
	{ 79,  1, _RP("Rosalina (Golf)")},		// 0x02B7
	{ 80,  1, _RP("Rosalina (Horse Racing)")},	// 0x02B8
	{ 81,  1, _RP("Metal Mario (Soccer)")},		// 0x02B9
	{ 82,  1, _RP("Metal Mario (Baseball)")},	// 0x02BA
	{ 83,  1, _RP("Metal Mario (Tennis)")},		// 0x02BB
	{ 84,  1, _RP("Metal Mario (Golf)")},		// 0x02BC
	{ 85,  1, _RP("Metal Mario (Horse Racing)")},	// 0x02BD
	{ 86,  1, _RP("Pink Gold Peach (Soccer)")},		// 0x02BE
	{ 87,  1, _RP("Pink Gold Peach (Baseball)")},		// 0x02BF
	{ 88,  1, _RP("Pink Gold Peach (Tennis)")},		// 0x02C0
	{ 89,  1, _RP("Pink Gold Peach (Golf)")},		// 0x02C1
	{ 90,  1, _RP("Pink Gold Peach (Horse Racing)")},	// 0x02C2

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
	{  2, 1, _RP("One-Eyed Rathalos and Rider (Female)")},	// 0x02E1
	{  1, 1, _RP("One-Eyed Rathalos and Rider (Male)")},	// 0x02E2
	{  3, 1, _RP("Nabiru")},				// 0x02E3
	{  4, 2, _RP("Rathian and Cheval")},			// 0x02E4
	{  5, 2, _RP("Barioth and Ayuria")},			// 0x02E5
	{  6, 2, _RP("Qurupeco and Dan")},			// 0x02E6

	// Animal Crossing: Welcome Amiibo Series [0x02E8-0x031E]
	{  1, 7, _RP("Vivian")},		// 0x02E7
	{  2, 7, _RP("Hopkins")},		// 0x02E8
	{  3, 7, _RP("June")},			// 0x02E9
	{  4, 7, _RP("Piper")},			// 0x02EA
	{  5, 7, _RP("Paolo")},			// 0x02EB
	{  6, 7, _RP("Hornsby")},		// 0x02EC
	{  7, 7, _RP("Stella")},		// 0x02ED
	{  8, 7, _RP("Tybalt")},		// 0x02EE
	{  9, 7, _RP("Huck")},			// 0x02EF
	{ 10, 7, _RP("Sylvana")},		// 0x02F0
	{ 11, 7, _RP("Boris")},			// 0x02F1
	{ 12, 7, _RP("Wade")},			// 0x02F2
	{ 13, 7, _RP("Carrie")},		// 0x02F3
	{ 14, 7, _RP("Ketchup")},		// 0x02F4
	{ 15, 7, _RP("Rex")},			// 0x02F5
	{ 16, 7, _RP("Stu")},			// 0x02F6
	{ 17, 7, _RP("Ursala")},		// 0x02F7
	{ 18, 7, _RP("Jacob")},			// 0x02F8
	{ 19, 7, _RP("Maddie")},		// 0x02F9
	{ 20, 7, _RP("Billy")},			// 0x02FA
	{ 21, 7, _RP("Boyd")},			// 0x02FB
	{ 22, 7, _RP("Bitty")},			// 0x02FC
	{ 23, 7, _RP("Maggie")},		// 0x02FD
	{ 24, 7, _RP("Murphy")},		// 0x02FE
	{ 25, 7, _RP("Plucky")},		// 0x02FF
	{ 26, 7, _RP("Sandy")},			// 0x0300
	{ 27, 7, _RP("Claude")},		// 0x0301
	{ 28, 7, _RP("Raddle")},		// 0x0302
	{ 29, 7, _RP("Julia")},			// 0x0303
	{ 30, 7, _RP("Louie")},			// 0x0304
	{ 31, 7, _RP("Bea")},			// 0x0305
	{ 32, 7, _RP("Admiral")},		// 0x0306
	{ 33, 7, _RP("Ellie")},			// 0x0307
	{ 34, 7, _RP("Boots")},			// 0x0308
	{ 35, 7, _RP("Weber")},			// 0x0309
	{ 36, 7, _RP("Candi")},			// 0x030A
	{ 37, 7, _RP("Leopold")},		// 0x030B
	{ 38, 7, _RP("Spike")},			// 0x030C
	{ 39, 7, _RP("Cashmere")},		// 0x030D
	{ 40, 7, _RP("Tad")},			// 0x030E
	{ 41, 7, _RP("Norma")},			// 0x030F
	{ 42, 7, _RP("Gonzo")},			// 0x0310
	{ 43, 7, _RP("Sprocket")},		// 0x0311
	{ 44, 7, _RP("Snooty")},		// 0x0312
	{ 45, 7, _RP("Olive")},			// 0x0313
	{ 46, 7, _RP("Dobie")},			// 0x0314
	{ 47, 7, _RP("Buzz")},			// 0x0315
	{ 48, 7, _RP("Cleo")},			// 0x0316
	{ 49, 7, _RP("Ike")},			// 0x0317
	{ 50, 7, _RP("Tasha")},			// 0x0318

	// Animal Crossing x Sanrio Series
	{  1, 6, _RP("Rilla")},			// 0x0319
	{  2, 6, _RP("Marty")},			// 0x031A
#ifdef RP_UTF16
	{  3, 6, _RP("\u00C9toile")},		// 0x031B
#else /* RP_UTF8 */
	{  3, 6, _RP("\xC3\x89toile")},		// 0x031B
#endif
	{  4, 6, _RP("Chai")},			// 0x031C
	{  5, 6, _RP("Chelsea")},		// 0x031D
	{  6, 6, _RP("Toby")},			// 0x031E

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
	{  0, 2, _RP("Link (Ocarina of Time)")},	// 0x034B
	{  0, 0, _RP("Unreleased (Majora's Mask)")},	// 0x034C
	{  0, 0, _RP("Unreleased (Twilight Princess)")},// 0x034D
	{  0, 0, _RP("Unreleased (Skyward Sword)")},	// 0x034E
	{  0, 2, _RP("Link (8-bit)")},			// 0x034F
	{  0, 2, _RP("Toon Link (The Wind Waker)")},	// 0x0350
	{  0, 0, nullptr},				// 0x0351
	{  0, 2, _RP("Zelda (The Wind Waker)")},	// 0x0352

	// The Legend of Zelda: Breath of the Wild Series
	{  0, 3, _RP("Link (Archer)")},			// 0x0353
	{  0, 3, _RP("Link (Rider)")},			// 0x0354
	{  0, 3, _RP("Guardian")},			// 0x0355
	{  0, 3, _RP("Zelda")},				// 0x0356
	// Unused [0x0357-0x35B]
	{  0, 0, nullptr},				// 0x0357
	{  0, 0, nullptr}, {  0, 0, nullptr},		// 0x0358,0x0359
	{  0, 0, nullptr}, {  0, 0, nullptr},		// 0x035A,0x035B
	// The Legend of Zelda: Breath of the Wild Series (continued)
	{  0, 3, _RP("Bokoblin")},			// 0x035C

	// Yarn Yoshi: Poochy [0x035D]
	{  5, 0, _RP("Poochy")},			// 0x035D

	// BoxBoy!: Qbby [0x035E]
	{  0, 0, _RP("Qbby")},				// 0x035E

#if 0
	// TODO: Not released yet.

	// SSB: DLC characters.
	{ 57, 10, _RP("Bayonetta")},			// 0x0xxx
	{ 58, 10, _RP("Cloud")},			// 0x0xxx
	{ 59, 10, _RP("Corrin")},			// 0x0xxx
#endif
};

/** AmiiboData **/

/**
 * Look up a character series name.
 * @param char_id Character ID. (Page 21) [must be host-endian]
 * @return Character series name, or nullptr if not found.
 */
const rp_char *AmiiboData::lookup_char_series_name(uint32_t char_id)
{
	static_assert(ARRAY_SIZE(AmiiboDataPrivate::char_series_names) == 0x360/4,
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
const rp_char *AmiiboData::lookup_char_name(uint32_t char_id)
{
	const uint16_t id = (char_id >> 16) & 0xFFFF;

	// Do a binary search.
	const AmiiboDataPrivate::char_id_t key = {id, nullptr, nullptr, 0};
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
const rp_char *AmiiboData::lookup_amiibo_series_name(uint32_t amiibo_id)
{
	// FIXME: gcc-6.3.0 is trying to interpret 0x035E+1 as a
	// floating-point hex constant:
	// error: unable to find numeric literal operator ‘operator""+1’
	static_assert(ARRAY_SIZE(AmiiboDataPrivate::amiibo_ids) == ((0x035E)+1),
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
const rp_char *AmiiboData::lookup_amiibo_series_data(uint32_t amiibo_id, int *pReleaseNo, int *pWaveNo)
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
