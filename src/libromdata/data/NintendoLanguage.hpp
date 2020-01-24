/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NintendoLanguage.hpp: Get the system language for Nintendo systems.     *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
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

		/**
		 * Convert a Nintendo DS/3DS language ID to a language code.
		 * @param langID Nintendo DS/3DS language ID.
		 * @param maxID Maximum language ID. (es, hans, ko, or hant)
		 * @return Language code, or 0 on error.
		 */
		static uint32_t getNDSLanguageCode(int langID, int maxID = 9001);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DATA_NINTENDOLANGUAGE_HPP__ */
