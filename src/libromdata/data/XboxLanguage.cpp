/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * XboxLanguage.cpp: Get the system language for Microsoft Xbox systems.   *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "XboxLanguage.hpp"

#include "librpbase/SystemRegion.hpp"
using LibRpBase::SystemRegion;

// Microsoft Xbox system structs.
#include "Console/xbox360_xdbf_structs.h"

namespace LibRomData {

/**
 * Determine the system language for Xbox 360.
 * @return XDBF_Language_e. (If unknown, returns XDBF_LANGUAGE_UNKNOWN.)
 */
int XboxLanguage::getXbox360Language(void)
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
		case 'zh':
		case 'hant':
			return XDBF_LANGUAGE_CHINESE_TRAD;
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
uint32_t XboxLanguage::getXbox360LanguageCode(int langID)
{
	// GCN_PAL_Language_ID system language code mapping.
	static const uint32_t langID_to_lc[XDBF_LANGUAGE_MAX] = {
		0,	// XDBF_LANGUAGE_UNKNOWN
		'en',	// XDBF_LANGUAGE_ENGLISH
		'ja',	// XDBF_LANGUAGE_JAPANESE
		'de',	// XDBF_LANGUAGE_GERMAN
		'fr',	// XDBF_LANGUAGE_FRENCH
		'es',	// XDBF_LANGUAGE_SPANISH
		'it',	// XDBF_LANGUAGE_ITALIAN
		'ko',	// XDBF_LANGUAGE_KOREAN
		'hant',	// XDBF_LANGUAGE_CHINESE_TRAD
	};

	assert(langID >= 0);
	assert(langID < ARRAY_SIZE(langID_to_lc));
	if (langID < 0 || langID >= ARRAY_SIZE(langID_to_lc)) {
		// Out of range.
		return 0;
	}

	return langID_to_lc[langID];
}

}
