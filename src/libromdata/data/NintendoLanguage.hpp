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

#ifndef __ROMPROPERTIES_LIBROMDATA_DATA_NINTENDOLANGUAGE_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DATA_NINTENDOLANGUAGE_HPP__

#include "librpbase/common.h"

// C includes.
#include <stdint.h>

namespace LibRomData {

class NintendoLanguage
{
	private:
		// NintendoLanguage is a static class.
		NintendoLanguage();
		~NintendoLanguage();
		RP_DISABLE_COPY(NintendoLanguage)

	public:
		/**
		 * Determine the system language for PAL GameCube.
		 * @return GCN_PAL_Language_ID. (If unknown, defaults to GCN_PAL_LANG_ENGLISH.)
		 */
		static int getGcnPalLanguage(void);

		/**
		 * Determine the system language for Wii.
		 * @return Wii_Language_ID. (If unknown, defaults to WII_LANG_ENGLISH.)
		 */
		static int getWiiLanguage(void);

		/**
		 * Determine the system language for Nintendo DS.
		 * @param version NDS_IconTitleData version.
		 * @return NDS_Language_ID. If unknown, defaults to NDS_LANG_ENGLISH.
		 */
		static int getNDSLanguage(uint16_t version);

		/**
		 * Determine the system language for Nintendo 3DS.
		 * TODO: Verify against the game's region code?
		 * @return N3DS_Language_ID. If unknown, defaults to N3DS_LANG_ENGLISH.
		 */
		static int getN3DSLanguage(void);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DATA_NINTENDOLANGUAGE_HPP__ */
