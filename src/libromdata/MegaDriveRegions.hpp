/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * MegaDriveRegions.hpp: Sega Mega Drive region code detection.            *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_MEGADRIVEREGIONS_HPP__
#define __ROMPROPERTIES_LIBROMDATA_MEGADRIVEREGIONS_HPP__

// C includes.
#include <stdint.h>

namespace LibRomData {

class MegaDriveRegions
{
	private:
		// MegaDriveRegions is a static class.
		MegaDriveRegions();
		~MegaDriveRegions();
		MegaDriveRegions(const MegaDriveRegions &other);
		MegaDriveRegions &operator=(const MegaDriveRegions &other);

	public:
		// Region code bitfields.
		// This corresponds to the later hexadecimal region codes.
		enum MD_RegionCode {
			MD_REGION_JAPAN		= (1 << 0),
			MD_REGION_ASIA		= (1 << 1),
			MD_REGION_USA		= (1 << 2),
			MD_REGION_EUROPE	= (1 << 3),
		};

		/**
		 * Parse the region codes field from an MD ROM header.
		 * @param region_codes Region codes field.
		 * @param size Size of region_codes.
		 * @return MD hexadecimal region code. (See MD_RegionCode.)
		 */
		static uint32_t parseRegionCodes(const char *region_codes, int size);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_MEGADRIVEREGIONS_HPP__ */
