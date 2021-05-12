/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * SystemRegion.hpp: Get the system country code.                          *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_SYSTEMREGION_HPP__
#define __ROMPROPERTIES_LIBRPBASE_SYSTEMREGION_HPP__

#include "common.h"

// C includes.
#include <stdint.h>

namespace LibRpBase {

class SystemRegion
{
	private:
		// SystemRegion is a static class.
		SystemRegion();
		~SystemRegion();
		RP_DISABLE_COPY(SystemRegion)

	public:
		/**
		 * Get the system country code. (ISO-3166)
		 * This will always be an uppercase ASCII string.
		 *
		 * NOTE: Some newer country codes may use 3-character abbreviations.
		 * The abbreviation will always be aligned towards the LSB, e.g.
		 * 'US' will be 0x00005553.
		 *
		 * @return ISO-3166 country code as a uint32_t, or 0 on error.
		 */
		static uint32_t getCountryCode(void);

		/**
		 * Get the system language code. (ISO-639)
		 * This will always be a lowercase ASCII string.
		 *
		 * NOTE: Some newer country codes may use 3-character abbreviations.
		 * The abbreviation will always be aligned towards the LSB, e.g.
		 * 'en' will be 0x0000656E.
		 *
		 * @return ISO-639 language code as a uint32_t, or 0 on error.
		 */
		static uint32_t getLanguageCode(void);

		/**
		 * Get a localized name for a language code.
		 * Localized means in that language's language,
		 * e.g. 'es' -> "Español".
		 * @param lc Language code.
		 * @return Localized name, or nullptr if not found.
		 */
		static const char *getLocalizedLanguageName(uint32_t lc);

		// Flag sprite sheet columns/rows.
		static const unsigned int FLAGS_SPRITE_SHEET_COLS = 4;
		static const unsigned int FLAGS_SPRITE_SHEET_ROWS = 4;

		/**
		 * Get the position of a language code's flag icon in the flags sprite sheet.
		 * @param lc		[in] Language code.
		 * @param pCol		[out] Pointer to store the column value. (-1 if not found)
		 * @param pRow		[out] Pointer to store the row value. (-1 if not found)
		 * @param forcePAL	[in,opt] If true, force PAL regions, e.g. always use the 'gb' flag for English.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		ATTR_ACCESS(write_only, 2)
		ATTR_ACCESS(write_only, 3)
		static int getFlagPosition(uint32_t lc, int *pCol, int *pRow, bool forcePAL = false);
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_SYSTEMREGION_HPP__ */
