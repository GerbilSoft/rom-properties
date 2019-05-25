/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * SystemRegion.hpp: Get the system country code.                          *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_SYSTEMREGION_HPP__
#define __ROMPROPERTIES_LIBRPBASE_SYSTEMREGION_HPP__

#include "librpbase/common.h"

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
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_SYSTEMREGION_HPP__ */
