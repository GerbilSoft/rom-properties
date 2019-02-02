/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * XboxLanguage.cpp: Get the system language for Microsoft Xbox systems.   *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
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
