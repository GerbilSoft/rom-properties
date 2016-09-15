/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * MegaDriveRegions.cpp: Sega Mega Drive region code detection.            *
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

#include "MegaDriveRegions.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cctype>
#include <cstring>

namespace LibRomData {

/**
 * Parse the region codes field from an MD ROM header.
 * @param region_codes Region codes field.
 * @param size Size of region_codes.
 * @return MD hexadecimal region code. (See MD_RegionCode.)
 */
uint32_t MegaDriveRegions::parseRegionCodes(const char *region_codes, int size)
{
	// Make sure the region codes field is valid.
	assert(region_codes != nullptr);	// NOT checking this in release builds.
	assert(size > 0);
	if (!region_codes || size <= 0)
		return 0;

	uint32_t ret = 0;

	// Check for a hex code.
	if (isalnum(region_codes[0]) &
	    (region_codes[1] == 0 || isspace(region_codes[1])))
	{
		// Single character region code.
		// Assume it's a hex code, *unless* it's 'E'.
		char code = toupper(region_codes[0]);
		if (code >= '0' && code <= '9') {
			// Numeric code from '0' to '9'.
			ret = code - '0';
		} else if (code == 'E') {
			// 'E'. This is probably Europe.
			// If interpreted as a hex code, this would be
			// Asia, USA, and Europe, with Japan excluded.
			ret = MD_REGION_EUROPE;
		} else if (code >= 'A' && code <= 'F') {
			// Letter code from 'A' to 'F'.
			ret = (code - 'A') + 10;
		}
	} else if (region_codes[0] < 16) {
		// Hex code not mapped to ASCII.
		ret = region_codes[0];
	}

	if (ret == 0) {
		// Not a hex code, or the hex code was 0.
		// Hex code being 0 shouldn't happen...

		// Check for string region codes.
		// Some games incorrectly use these.
		if (!strncasecmp(region_codes, "EUR", 3)) {
			ret = MD_REGION_EUROPE;
		} else if (!strncasecmp(region_codes, "USA", 3)) {
			ret = MD_REGION_USA;
		} else if (!strncasecmp(region_codes, "JPN", 3) ||
			   !strncasecmp(region_codes, "JAP", 3))
		{
			ret = MD_REGION_JAPAN | MD_REGION_ASIA;
		} else {
			// Check for old-style JUE region codes.
			// (J counts as both Japan and Asia.)
			for (int i = 0; i < size; i++) {
				if (region_codes[i] == 0 || isspace(region_codes[i]))
					break;
				switch (region_codes[i]) {
					case 'J':
						ret |= MD_REGION_JAPAN | MD_REGION_ASIA;
						break;
					case 'U':
						ret |= MD_REGION_USA;
						break;
					case 'E':
						ret |= MD_REGION_EUROPE;
						break;
					default:
						break;
				}
			}
		}
	}

	return ret;
}

}
