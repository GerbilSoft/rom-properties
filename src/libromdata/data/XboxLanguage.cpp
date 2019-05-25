/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * XboxLanguage.cpp: Get the system language for Microsoft Xbox systems.   *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "XboxLanguage.hpp"
#include "librpbase/SystemRegion.hpp"
using LibRpBase::SystemRegion;

// C includes. (C++ namespace)
#include <cassert>

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
			// USed if the host system language
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
			return XDBF_LANGUAGE_CHINESE;
	}

	// Should not get here...
	assert(!"Invalid code path.");
	return XDBF_LANGUAGE_UNKNOWN;
}

}
