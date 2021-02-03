/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiUData.hpp: Nintendo Wii U publisher data.                            *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// GameTDB uses ID6 for retail Wii U titles. The publisher ID
// is NOT stored in the plaintext .wud header, so it's not
// possible to use GameTDB unless we hard-code all of the
// publisher IDs here.

#ifndef __ROMPROPERTIES_LIBROMDATA_WIIUDATA_HPP__
#define __ROMPROPERTIES_LIBROMDATA_WIIUDATA_HPP__

#include "common.h"

// C includes.
#include <stdint.h>

namespace LibRomData {

class WiiUData
{
	private:
		// Static class.
		WiiUData();
		~WiiUData();
		RP_DISABLE_COPY(WiiUData)

	public:
		/**
		 * Look up a Wii U retail disc publisher.
		 *
		 * NOTE: Wii U uses 4-character publisher IDs.
		 * If the first two characters are "00", then it's
		 * equivalent to a 2-character publisher ID.
		 *
		 * @param Wii U retail disc ID4.
		 * @return Packed publisher ID, or 0 if not found.
		 */
		static uint32_t lookup_disc_publisher(const char *id4);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_WIIUDATA_HPP__ */
