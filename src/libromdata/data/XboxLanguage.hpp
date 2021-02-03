/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * XboxLanguage.hpp: Get the system language for Microsoft Xbox systems.   *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_DATA_XBOXLANGUAGE_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DATA_XBOXLANGUAGE_HPP__

#include "common.h"

// C includes.
#include <stdint.h>

namespace LibRomData {

class XboxLanguage
{
	private:
		// XboxLanguage is a static class.
		XboxLanguage();
		~XboxLanguage();
		RP_DISABLE_COPY(XboxLanguage)

	public:
		/**
		 * Determine the system language for Xbox 360.
		 * @return XDBF_Language_e. (If unknown, returns XDBF_LANGUAGE_UNKNOWN.)
		 */
		static int getXbox360Language(void);

		/**
		 * Convert an Xbox 360 language ID to a language code.
		 * @param langID Xbox 360 language ID.
		 * @return Language code, or 0 on error.
		 */
		static uint32_t getXbox360LanguageCode(int langID);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DATA_XBOXLANGUAGE_HPP__ */
