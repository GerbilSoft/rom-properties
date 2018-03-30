/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * MegaDriveRegions.hpp: Sega Mega Drive region code detection.            *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_MEGADRIVEREGIONS_HPP__
#define __ROMPROPERTIES_LIBROMDATA_MEGADRIVEREGIONS_HPP__

#include "librpbase/common.h"

namespace LibRomData {

class MegaDriveRegions
{
	private:
		// MegaDriveRegions is a static class.
		MegaDriveRegions();
		~MegaDriveRegions();
		RP_DISABLE_COPY(MegaDriveRegions)

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
		static unsigned int parseRegionCodes(const char *region_codes, int size);

		// Branding region.
		enum MD_BrandingRegion {
			MD_BREGION_UNKNOWN = 0,

			// Primary regions.
			MD_BREGION_JAPAN,
			MD_BREGION_USA,
			MD_BREGION_EUROPE,

			// Additional regions.
			MD_BREGION_SOUTH_KOREA,
			MD_BREGION_BRAZIL,
		};

		/**
		 * Determine the branding region to use for a ROM.
		 * This is based on the ROM's region code and the system's locale.
		 * @param md_region MD hexadecimal region code.
		 * @return MD branding region.
		 */
		static MD_BrandingRegion getBrandingRegion(unsigned int md_region);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_MEGADRIVEREGIONS_HPP__ */
