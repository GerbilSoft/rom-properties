/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * MegaDriveRegions.cpp: Sega Mega Drive region code detection.            *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "MegaDriveRegions.hpp"
#include "librpbase/SystemRegion.hpp"

namespace LibRomData {

// TODO: Separate parser function for Pico,
// since Pico doesn't have worldwide releases.

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

	// Check for invalid region code data.
	// If any of the bytes have the high bit set, the field is invalid.
	for (int i = 0; i < size; i++) {
		if (region_codes[i] & 0x80) {
			// High bit is set.
			return 0;
		}
	}

	unsigned int ret = 0;

	// Check for a hex code.
	if (ISALNUM(region_codes[0]) &&
	    (region_codes[1] == 0 || ISSPACE(region_codes[1])))
	{
		// Single character region code.
		// Assume it's a hex code, *unless* it's 'E'.
		const char code = toupper(region_codes[0]);
		if (code >= '0' && code <= '9') {
			// Numeric code from '0' to '9'.
			ret = code - '0';
		} else if (code == 'E') {
			// 'E'. This is probably Europe.
			// If interpreted as a hex code, this would be
			// Asia, USA, and Europe, with Japan excluded.
			// TODO: Check for other regions? ("EUJ", etc.)
			ret = MD_REGION_EUROPE;
		} else if (code >= 'A' && code <= 'F') {
			// Letter code from 'A' to 'F'.
			ret = (code - 'A') + 10;
		} else if (code == 'W') {
			// "Worldwide". Used by EverDrive OS ROMs.
			ret = MD_REGION_JAPAN | MD_REGION_ASIA |
			      MD_REGION_USA | MD_REGION_EUROPE;
		}
	}
	else if ((region_codes[0] == '8' || region_codes[0] == 'E') &&
	         (region_codes[2] == 0 || ISSPACE(region_codes[2])))
	{
		// Some Pico games have unusual European region codes,
		// e.g. '8F' or 'EF' for France. Handle it here:
		// - '8F' would be parsed as "no regions".
		// - 'EF' might be parsed as "all regions" due to 'F'.
		ret = MD_REGION_EUROPE;
	}
	else if (region_codes[0] < 16)
	{
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
				// Allow spaces in the first three characters.
				// "Psy-O-Blade (Japan)" has "  J".
				if (i >= 3 && (region_codes[i] == 0 || ISSPACE(region_codes[i])))
					break;

				switch (region_codes[i]) {
					case 'J':
					case 'K':	// Korea (Tiny Toon Adventures)
						ret |= MD_REGION_JAPAN | MD_REGION_ASIA;
						break;
					case 'U':
						ret |= MD_REGION_USA;
						break;
					case 'E':
					//case 'F':	// France (Pico) (CONFLICTS WITH HEX)
					case 'G':	// Germany (Pico)
					case 'S':	// Spain (Pico)
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
			return (cc == 'KR' ? MD_BrandingRegion::South_Korea : MD_BrandingRegion::Japan);

		case MD_REGION_USA:
			// USA. May be Brazilian.
			return (cc == 'BR' ? MD_BrandingRegion::Brazil : MD_BrandingRegion::USA);

		case MD_REGION_EUROPE:
			// Europe.
			return MD_BrandingRegion::Europe;

		default:
			// Multi-region ROM.
			break;
	}

	// Multi-region ROM.
	// Determine the system's branding region.
	MD_BrandingRegion md_bregion = MD_BrandingRegion::Unknown;

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
			md_bregion = MD_BrandingRegion::Japan;
			break;

		case 'KR':	// South Korea
			md_bregion = MD_BrandingRegion::South_Korea;
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
			md_bregion = MD_BrandingRegion::USA;
			break;

		case 'BR':	// Brazil
			md_bregion = MD_BrandingRegion::Brazil;
			break;

		default:
			// Assume everything else is Europe.
			md_bregion = MD_BrandingRegion::Europe;
			break;
	}

	// Check for a matching BREGION.
	switch (md_bregion) {
		case MD_BrandingRegion::Japan:
		case MD_BrandingRegion::South_Korea:
			if (md_region & (MD_REGION_JAPAN | MD_REGION_ASIA)) {
				return md_bregion;
			}
			break;

		case MD_BrandingRegion::USA:
		case MD_BrandingRegion::Brazil:
			if (md_region & MD_REGION_USA) {
				return md_bregion;
			}
			break;

		case MD_BrandingRegion::Europe:
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
		return (cc == 'KR' ? MD_BrandingRegion::South_Korea : MD_BrandingRegion::Japan);
	} else if (md_region & MD_REGION_USA) {
		// USA. May be Brazilian.
		return (cc == 'BR' ? MD_BrandingRegion::Brazil : MD_BrandingRegion::USA);
	} else if (md_region & MD_REGION_EUROPE) {
		// Europe.
		return MD_BrandingRegion::Europe;
	}

	// Still no region! Default to Japan.
	return MD_BrandingRegion::Japan;
}

}
