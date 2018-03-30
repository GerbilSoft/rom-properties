/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NintendoLanguage.hpp: Get the system language for Nintendo systems.     *
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

#include "NintendoLanguage.hpp"
#include "librpbase/SystemRegion.hpp"
using LibRpBase::SystemRegion;

// C includes. (C++ namespace)
#include <cassert>

// Nintendo system structs.
#include "Console/gcn_banner.h"
#include "Handheld/nds_structs.h"
#include "Handheld/n3ds_structs.h"

namespace LibRomData {

/**
 * Determine the system language for PAL GameCube.
 * @return GCN_PAL_Language_ID. (If unknown, defaults to GCN_PAL_LANG_ENGLISH.)
 */
int NintendoLanguage::getGcnPalLanguage(void)
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
 * Determine the system language for Wii.
 * @return Wii_Language_ID. (If unknown, defaults to WII_LANG_ENGLISH.)
 */
int NintendoLanguage::getWiiLanguage(void)
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
 * Determine the system language for Nintendo DS.
 * @param version NDS_IconTitleData version.
 * @return NDS_Language. If unknown, defaults to NDS_LANG_ENGLISH.
 */
int NintendoLanguage::getNDSLanguage(uint16_t version)
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
			if (version >= NDS_ICON_VERSION_ZH) {
				// NOTE: No distinction between
				// Simplified and Traditional Chinese
				// on Nintendo DS...
				return NDS_LANG_CHINESE;
			}
			// No Chinese title here.
			return NDS_LANG_ENGLISH;
		case 'ko':
			if (version >= NDS_ICON_VERSION_ZH_KO) {
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
int NintendoLanguage::getN3DSLanguage(void)
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
			// TODO: Simplified vs. Traditional?
			// May need to check the country code.
			return N3DS_LANG_CHINESE_SIMP;
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

}
