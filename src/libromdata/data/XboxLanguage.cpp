/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * XboxLanguage.cpp: Get the system language for Microsoft Xbox systems.   *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "XboxLanguage.hpp"

#include "librpbase/SystemRegion.hpp"
using namespace LibRpBase;

// Microsoft Xbox system structs.
#include "Console/xbox360_xdbf_structs.h"

namespace LibRomData { namespace XboxLanguage {

/**
 * Determine the system language for Xbox 360.
 * @return XDBF_Language_e. (If unknown, returns XDBF_LANGUAGE_UNKNOWN.)
 */
int getXbox360Language(void)
{
	switch (SystemRegion::getLanguageCode()) {
		case 'en':
		default:
			// English. (default)
			// Used if the host system language
			// doesn't match any of the languages
			// supported by the Xbox 360.
			return XDBF_LANGUAGE_ENGLISH;
		case 'ja':
			return XDBF_LANGUAGE_JAPANESE;
		case 'de':
			return XDBF_LANGUAGE_GERMAN;
		case 'fr':
			return XDBF_LANGUAGE_FRENCH;
		case 'es':
			return XDBF_LANGUAGE_SPANISH;
		case 'it':
			return XDBF_LANGUAGE_ITALIAN;
		case 'ko':
			return XDBF_LANGUAGE_KOREAN;
		case 'zh':	// FIXME: Traditional or Simplified?
		case 'hant':
			return XDBF_LANGUAGE_CHINESE_TRAD;
		case 'pt':
			return XDBF_LANGUAGE_PORTUGUESE;
		case 'hans':
			return XDBF_LANGUAGE_CHINESE_SIMP;
		case 'pl':
			return XDBF_LANGUAGE_POLISH;
		case 'ru':
			return XDBF_LANGUAGE_RUSSIAN;
	}

	// Should not get here...
	assert(!"Invalid code path.");
	return XDBF_LANGUAGE_UNKNOWN;
}

/**
 * Convert an Xbox 360 language ID to a language code.
 * @param langID Xbox 360 language ID.
 * @return Language code, or 0 on error.
 */
uint32_t getXbox360LanguageCode(int langID)
{
	// GCN_PAL_Language_ID system language code mapping.
	static const std::array<uint32_t, 12+1> langID_to_lc = {{
		0,	// XDBF_LANGUAGE_UNKNOWN
		'en',	// XDBF_LANGUAGE_ENGLISH
		'ja',	// XDBF_LANGUAGE_JAPANESE
		'de',	// XDBF_LANGUAGE_GERMAN
		'fr',	// XDBF_LANGUAGE_FRENCH
		'es',	// XDBF_LANGUAGE_SPANISH
		'it',	// XDBF_LANGUAGE_ITALIAN
		'ko',	// XDBF_LANGUAGE_KOREAN
		'hant',	// XDBF_LANGUAGE_CHINESE_TRAD
		'pt',	// XDBF_LANGUAGE_PORTUGUESE
		'hans',	// XDBF_LANGUAGE_CHINESE_SIMP
		'pl',	// XDBF_LANGUAGE_POLISH
		'ru',	// XDBF_LANGUAGE_RUSSIAN
	}};

	assert(langID >= 0);
	assert(langID < static_cast<int>(langID_to_lc.size()));
	if (langID < 0 || langID >= static_cast<int>(langID_to_lc.size())) {
		// Out of range.
		return 0;
	}

	return langID_to_lc[langID];
} }

}
