/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiUData.hpp: Nintendo Wii U publisher data.                            *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

// GameTDB uses ID6 for retail Wii U titles. The publisher ID
// is NOT stored in the plaintext .wud header, so it's not
// possible to use GameTDB unless we hard-code all of the
// publisher IDs here.

#ifndef __ROMPROPERTIES_LIBROMDATA_WIIUDATA_HPP__
#define __ROMPROPERTIES_LIBROMDATA_WIIUDATA_HPP__

#include "config.libromdata.h"
#include "common.h"

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
