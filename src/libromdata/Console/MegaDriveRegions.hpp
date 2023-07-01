/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * MegaDriveRegions.hpp: Sega Mega Drive region code detection.            *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"

namespace LibRomData {

class MegaDriveRegions
{
	public:
		// Static class
		MegaDriveRegions() = delete;
		~MegaDriveRegions() = delete;
	private:
		RP_DISABLE_COPY(MegaDriveRegions)

	public:
		// Region code bitfields.
		// This corresponds to the later hexadecimal region codes.
		enum MD_RegionCode {
			MD_REGION_JAPAN		= (1U << 0),
			MD_REGION_ASIA		= (1U << 1),
			MD_REGION_USA		= (1U << 2),
			MD_REGION_EUROPE	= (1U << 3),
		};

		/**
		 * Parse the region codes field from an MD ROM header.
		 * @param region_codes Region codes field.
		 * @param size Size of region_codes.
		 * @return MD hexadecimal region code. (See MD_RegionCode.)
		 */
		static unsigned int parseRegionCodes(const char *region_codes, int size);

		// Branding region.
		enum class MD_BrandingRegion {
			Unknown = 0,

			// Primary regions.
			Japan,
			USA,
			Europe,

			// Additional regions.
			South_Korea,
			Brazil,
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
