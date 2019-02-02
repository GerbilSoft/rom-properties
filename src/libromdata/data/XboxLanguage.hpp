/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * XboxLanguage.hpp: Get the system language for Microsoft Xbox systems.   *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_DATA_XBOXLANGUAGE_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DATA_XBOXLANGUAGE_HPP__

#include "librpbase/common.h"

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
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DATA_XBOXLANGUAGE_HPP__ */
