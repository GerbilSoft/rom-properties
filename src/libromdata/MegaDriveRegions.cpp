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
#include "librpbase/SystemRegion.hpp"

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
unsigned int MegaDriveRegions::parseRegionCodes(const char *region_codes, int size)
{
	// Make sure the region codes field is valid.
	assert(region_codes != nullptr);
	assert(size > 0);
	if (!region_codes || size <= 0)
		return 0;

	unsigned int ret = 0;

	// Check for a hex code.
	if (isalnum(region_codes[0]) &&
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

/**
 * Determine the branding region to use for a ROM.
 * This is based on the ROM's region code and the system's locale.
 * @param md_region MD hexadecimal region code.
 * @return MD branding region.
 */
MegaDriveRegions::MD_BrandingRegion MegaDriveRegions::getBrandingRegion(unsigned int md_region)
{
	if (md_region == 0) {
		// No region code. Assume "all regions".
		md_region = ~0;
	}

	// Get the country code.
	const uint32_t cc = LibRpBase::SystemRegion::getCountryCode();

	// Check for a single-region ROM.
	switch (md_region) {
		case MD_REGION_JAPAN:
		case MD_REGION_ASIA:
		case MD_REGION_JAPAN | MD_REGION_ASIA:
			// Japan/Asia. Use Japanese branding,
			// except for South Korea.
			return (cc == 'KR' ? MD_BREGION_SOUTH_KOREA : MD_BREGION_JAPAN);

		case MD_REGION_USA:
			// USA. May be Brazilian.
			return (cc == 'BR' ? MD_BREGION_BRAZIL : MD_BREGION_USA);

		case MD_REGION_EUROPE:
			// Europe.
			return MD_BREGION_EUROPE;

		default:
			// Multi-region ROM.
			break;
	}

	// Multi-region ROM.
	// Determine the system's branding region.
	MD_BrandingRegion md_bregion = MD_BREGION_UNKNOWN;

	switch (cc) {
		// Japanese region
		case 'JP':	// Japan
		case 'IN':	// India
		case 'HK':	// Hong Kong
		case 'MO':	// Macao
		case 'SG':	// Singapore
		case 'MY':	// Malaysia
		case 'BN':	// Brunei
		case 'TH':	// Thailand
		case 'TW':	// Taiwan
		case 'PH':	// Philippines
			// TODO: Add more countries that used JP branding?
			md_bregion = MD_BREGION_JAPAN;
			break;

		case 'KR':	// South Korea
			md_bregion = MD_BREGION_SOUTH_KOREA;
			break;

		case 'US':	// USA
		case 'AG':	// Antigua and Barbuda
		case 'BS':	// The Bahamas
		case 'BB':	// Barbados
		case 'BZ':	// Belize
		case 'CA':	// Canada
		case 'CR':	// Costa Rica
		case 'CU':	// Cuba
		case 'DM':	// Dominica
		case 'DO':	// Dominican Republic
		case 'SV':	// El Salvador
		case 'GL':	// Greenland (TODO: Technically European?)
		case 'GD':	// Grenada
		case 'GT':	// Guatemala
		case 'HT':	// Haiti
		case 'HN':	// Honduras
		case 'JM':	// Jamaica
		case 'MX':	// Mexico
		case 'NI':	// Nicaragua
		case 'PA':	// Panama
		case 'PR':	// Puerto Rico
		case 'KN':	// Saint Kitts and Nevis
		case 'LC':	// Saint Lucia
		case 'VC':	// Saint Vincent and the Grenadines
		case 'TT':	// Trinidad and Tobago
		case 'TC':	// Turks and Caicos Islands
		case 'VI':	// United States Virgin Islands
		case 'UM':	// United States Minor Outlying Islands
			// TODO: Verify that all of these countries
			// use USA branding.
			md_bregion = MD_BREGION_USA;
			break;

		case 'BR':	// Brazil
			md_bregion = MD_BREGION_BRAZIL;
			break;

		default:
			// Assume everything else is Europe.
			md_bregion = MD_BREGION_EUROPE;
			break;
	}

	// Check for a matching BREGION.
	switch (md_bregion) {
		case MD_BREGION_JAPAN:
		case MD_BREGION_SOUTH_KOREA:
			if (md_region & (MD_REGION_JAPAN | MD_REGION_ASIA)) {
				return md_bregion;
			}
			break;

		case MD_BREGION_USA:
		case MD_BREGION_BRAZIL:
			if (md_region & MD_REGION_USA) {
				return md_bregion;
			}
			break;

		case MD_BREGION_EUROPE:
			if (md_region & MD_REGION_EUROPE) {
				return md_bregion;
			}
			break;

		default:
			break;
	}

	// No matching BREGION.
	// Use a default priority list of Japan, USA, Europe.
	if (md_region & (MD_REGION_JAPAN | MD_REGION_ASIA)) {
		// Japan/Asia. Use Japanese branding,
		// except for South Korea.
		return (cc == 'KR' ? MD_BREGION_SOUTH_KOREA : MD_BREGION_JAPAN);
	} else if (md_region & MD_REGION_USA) {
		// USA. May be Brazilian.
		return (cc == 'BR' ? MD_BREGION_BRAZIL : MD_BREGION_USA);
	} else if (md_region & MD_REGION_EUROPE) {
		// Europe.
		return MD_BREGION_EUROPE;
	}

	// Still no region! Default to Japan.
	return MD_BREGION_JAPAN;
}

}
