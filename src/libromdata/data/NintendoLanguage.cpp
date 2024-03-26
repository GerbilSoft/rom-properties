/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NintendoLanguage.hpp: Get the system language for Nintendo systems.     *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "NintendoLanguage.hpp"

#include "librpbase/SystemRegion.hpp"
using namespace LibRpBase;

// Nintendo system structs.
#include "Console/gcn_banner.h"
#include "Console/wii_banner.h"
#include "Handheld/nds_structs.h"
#include "Handheld/n3ds_structs.h"

// C includes (C++ namespace)
#include <cassert>

// C++ STL classes
using std::array;

namespace LibRomData { namespace NintendoLanguage {

/**
 * Determine the system language for PAL GameCube.
 * @return GCN_PAL_Language_ID. (If unknown, defaults to GCN_PAL_LANG_ENGLISH.)
 */
int getGcnPalLanguage(void)
{
	switch (SystemRegion::getLanguageCode()) {
		case 'en':
		default:
			// English. (default)
			// Used if the host system language
			// doesn't match any of the languages
			// supported by PAL GameCubes.
			return GCN_PAL_LANG_ENGLISH;
		case 'de':
			return GCN_PAL_LANG_GERMAN;
		case 'fr':
			return GCN_PAL_LANG_FRENCH;
		case 'es':
			return GCN_PAL_LANG_SPANISH;
		case 'it':
			return GCN_PAL_LANG_ITALIAN;
		case 'nl':
			return GCN_PAL_LANG_DUTCH;
	}

	// Should not get here...
	assert(!"Invalid code path.");
	return GCN_PAL_LANG_ENGLISH;
}

/**
 * Convert a GameCube PAL language ID to a language code.
 * @param langID GameCube PAL language ID.
 * @return Language code, or 0 on error.
 */
uint32_t getGcnPalLanguageCode(int langID)
{
	// GCN_PAL_Language_ID system language code mapping.
	static constexpr array<uint16_t, GCN_PAL_LANG_MAX> langID_to_lc = {{
		'en',	// GCN_PAL_LANG_ENGLISH
		'de',	// GCN_PAL_LANG_GERMAN
		'fr',	// GCN_PAL_LANG_FRENCH
		'es',	// GCN_PAL_LANG_SPANISH
		'it',	// GCN_PAL_LANG_ITALIAN
		'nl',	// GCN_PAL_LANG_DUTCH
	}};

	assert(langID >= 0);
	assert(langID <  static_cast<int>(langID_to_lc.size()));
	if (langID < 0 || langID >= static_cast<int>(langID_to_lc.size())) {
		// Out of range.
		return 0;
	}

	return static_cast<uint32_t>(langID_to_lc[langID]);
}

/**
 * Determine the system language for Wii.
 * @return Wii_Language_ID. (If unknown, defaults to WII_LANG_ENGLISH.)
 */
int getWiiLanguage(void)
{
	switch (SystemRegion::getLanguageCode()) {
		case 'en':
		default:
			// English. (default)
			// Used if the host system language doesn't match
			// any of the languages supported by Wii.
			return WII_LANG_ENGLISH;
		case 'ja':
			return WII_LANG_JAPANESE;
		case 'de':
			return WII_LANG_GERMAN;
		case 'fr':
			return WII_LANG_FRENCH;
		case 'es':
			return WII_LANG_SPANISH;
		case 'it':
			return WII_LANG_ITALIAN;
		case 'nl':
			return WII_LANG_DUTCH;
		case 'ko':
			return WII_LANG_KOREAN;
	}

	// Should not get here...
	assert(!"Invalid code path.");
	return WII_LANG_ENGLISH;
}

/**
 * Convert a Wii language ID to a language code.
 * @param langID GameCube PAL language ID.
 * @return Language code, or 0 on error.
 */
uint32_t getWiiLanguageCode(int langID)
{
	// Wii_Language_ID system language code mapping.
	static constexpr array<uint16_t, WII_LANG_MAX> langID_to_lc = {{
		'ja',	// WII_LANG_JAPANESE
		'en',	// WII_LANG_ENGLISH
		'de',	// WII_LANG_GERMAN
		'fr',	// WII_LANG_FRENCH
		'es',	// WII_LANG_SPANISH
		'it',	// WII_LANG_ITALIAN
		'nl',	// WII_LANG_DUTCH
		0,	// 7: unknown
		0,	// 8: unknown
		'ko',	// WII_LANG_KOREAN
	}};

	assert(langID >= 0);
	assert(langID <  static_cast<int>(langID_to_lc.size()));
	if (langID < 0 || langID >= static_cast<int>(langID_to_lc.size())) {
		// Out of range.
		return 0;
	}

	return static_cast<uint32_t>(langID_to_lc[langID]);
}

/**
 * Determine the system language for Nintendo DS.
 * @param version NDS_IconTitleData version.
 * @return NDS_Language. If unknown, defaults to NDS_LANG_ENGLISH.
 */
int getNDSLanguage(uint16_t version)
{
	switch (SystemRegion::getLanguageCode()) {
		case 'en':
		default:
			return NDS_LANG_ENGLISH;
		case 'ja':
			return NDS_LANG_JAPANESE;
		case 'fr':
			return NDS_LANG_FRENCH;
		case 'de':
			return NDS_LANG_GERMAN;
		case 'it':
			return NDS_LANG_ITALIAN;
		case 'es':
			return NDS_LANG_SPANISH;
		case 'zh':
			if (version >= NDS_ICON_VERSION_HANS) {
				return NDS_LANG_CHINESE_SIMP;
			}
			// No Chinese title here.
			return NDS_LANG_ENGLISH;
		case 'ko':
			if (version >= NDS_ICON_VERSION_HANS_KO) {
				return NDS_LANG_KOREAN;
			}
			// No Korean title here.
			return NDS_LANG_ENGLISH;
	}

	// Should not get here...
	assert(!"Invalid code path.");
	return NDS_LANG_ENGLISH;
}

/**
 * Determine the system language for Nintendo 3DS.
 * TODO: Verify against the game's region code?
 * @return N3DS_Language. If unknown, defaults to N3DS_LANG_ENGLISH.
 */
int getN3DSLanguage(void)
{
	switch (SystemRegion::getLanguageCode()) {
		case 'en':
		default:
			return N3DS_LANG_ENGLISH;
		case 'ja':
			return N3DS_LANG_JAPANESE;
		case 'fr':
			return N3DS_LANG_FRENCH;
		case 'de':
			return N3DS_LANG_GERMAN;
		case 'it':
			return N3DS_LANG_ITALIAN;
		case 'es':
			return N3DS_LANG_SPANISH;
		case 'zh':
			// Check the country code for simplified vs. traditional.
			// NOTE: Defaulting to traditional if country code isn't available,
			// since Nintendo products more commonly have traditional Chinese
			// translations compared to simplified Chinese.
			switch (SystemRegion::getCountryCode()) {
				case 'CN':
				case 'SG':
					return N3DS_LANG_CHINESE_SIMP;
				case 'TW':
				case 'HK':
				default:
					return N3DS_LANG_CHINESE_TRAD;
			}
		case 'ko':
			return N3DS_LANG_KOREAN;
		case 'nl':
			return N3DS_LANG_DUTCH;
		case 'pt':
			return N3DS_LANG_PORTUGUESE;
		case 'ru':
			return N3DS_LANG_RUSSIAN;
	}

	// Should not get here...
	assert(!"Invalid code path.");
	return N3DS_LANG_ENGLISH;
}

/**
 * Convert a Nintendo DS/3DS language ID to a language code.
 * @param langID Nintendo DS/3DS language ID.
 * @param maxID Maximum language ID, inclusive. (es, hans, ko, or hant)
 * @return Language code, or 0 on error.
 */
uint32_t getNDSLanguageCode(int langID, int maxID)
{
	// N3DS_Language_ID system language code mapping.
	static constexpr array<uint32_t, N3DS_LANG_MAX> langID_to_lc = {{
		// 0-7 are the same as Nintendo DS.
		'ja',	// N3DS_LANG_JAPANESE
		'en',	// N3DS_LANG_ENGLISH
		'fr',	// N3DS_LANG_FRENCH
		'de',	// N3DS_LANG_GERMAN
		'it',	// N3DS_LANG_ITALIAN
		'es',	// N3DS_LANG_SPANISH
		'hans',	// N3DS_LANG_CHINESE_SIMP

		// New to Nintendo 3DS.
		'ko',	// N3DS_LANG_KOREAN
		'nl',	// N3DS_LANG_DUTCH
		'pt',	// N3DS_LANG_PORTUGUESE
		'ru',	// N3DS_LANG_RUSSIAN
		'hant',	// N3DS_LANG_CHINESE_TRAD
	}};

	assert(langID >= 0);
	assert(maxID < static_cast<int>(langID_to_lc.size()));
	if (maxID >= static_cast<int>(langID_to_lc.size())) {
		maxID = static_cast<int>(langID_to_lc.size())-1;
	}
	assert(langID <= maxID);
	if (langID < 0 || langID > maxID) {
		// Out of range.
		return 0;
	}

	return langID_to_lc[langID];
}

} }
