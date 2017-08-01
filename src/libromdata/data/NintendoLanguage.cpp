/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NintendoLanguage.hpp: Get the system language for Nintendo systems.     *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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

#include "NintendoLanguage.hpp"
#include "librpbase/SystemRegion.hpp"
using LibRpBase::SystemRegion;

namespace LibRomData {

/**
 * Determine the system language for Nintendo DS.
 * @param version NDS_IconTitleData version.
 * @return NDS_Language. If unknown, defaults to NDS_LANG_ENGLISH.
 */
NDS_Language_ID NintendoLanguage::getNDSLanguage(uint16_t version)
{
	NDS_Language_ID lang;

	switch (SystemRegion::getLanguageCode()) {
		case 'en':
		default:
			lang = NDS_LANG_ENGLISH;
			break;
		case 'ja':
			lang = NDS_LANG_JAPANESE;
			break;
		case 'fr':
			lang = NDS_LANG_FRENCH;
			break;
		case 'de':
			lang = NDS_LANG_GERMAN;
			break;
		case 'it':
			lang = NDS_LANG_ITALIAN;
			break;
		case 'es':
			lang = NDS_LANG_SPANISH;
			break;
		case 'zh':
			if (version >= NDS_ICON_VERSION_ZH) {
				// NOTE: No distinction between
				// Simplified and Traditional Chinese
				// on Nintendo DS...
				lang = NDS_LANG_CHINESE;
			} else {
				// No Chinese title here.
				lang = NDS_LANG_ENGLISH;
			}
			break;
		case 'ko':
			if (version >= NDS_ICON_VERSION_ZH_KO) {
				lang = NDS_LANG_KOREAN;
			} else {
				// No Korean title here.
				lang = NDS_LANG_ENGLISH;
			}
			break;
	}

	return lang;
}

/**
 * Determine the system language for Nintendo 3DS.
 * TODO: Verify against the game's region code?
 * @return N3DS_Language. If unknown, defaults to N3DS_LANG_ENGLISH.
 */
N3DS_Language_ID NintendoLanguage::getN3DSLanguage(void)
{
	N3DS_Language_ID lang;

	switch (SystemRegion::getLanguageCode()) {
		case 'en':
		default:
			lang = N3DS_LANG_ENGLISH;
			break;
		case 'ja':
			lang = N3DS_LANG_JAPANESE;
			break;
		case 'fr':
			lang = N3DS_LANG_FRENCH;
			break;
		case 'de':
			lang = N3DS_LANG_GERMAN;
			break;
		case 'it':
			lang = N3DS_LANG_ITALIAN;
			break;
		case 'es':
			lang = N3DS_LANG_SPANISH;
			break;
		case 'zh':
			// TODO: Simplified vs. Traditional?
			// May need to check the country code.
			lang = N3DS_LANG_CHINESE_SIMP;
			break;
		case 'ko':
			lang = N3DS_LANG_KOREAN;
			break;
		case 'nl':
			lang = N3DS_LANG_DUTCH;
			break;
		case 'pt':
			lang = N3DS_LANG_PORTUGUESE;
			break;
		case 'ru':
			lang = N3DS_LANG_RUSSIAN;
			break;
	}

	return lang;
}

}
