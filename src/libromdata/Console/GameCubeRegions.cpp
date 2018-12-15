/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GameCubeRegions.hpp: Nintendo GameCube/Wii region code detection.       *
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

#include "GameCubeRegions.hpp"
#include "gcn_structs.h"

#include "libi18n/i18n.h"

// C++ includes.
#include <vector>
using std::vector;

namespace LibRomData {

/**
 * Convert a GCN region value (from GCN_Boot_Info or RVL_RegionSetting) to a string.
 * @param gcnRegion	[in] GCN region value.
 * @param idRegion	[in] Game ID region.
 * @param pIsDefault	[out,opt] Set to True if the region string represents the default region for the ID4.
 * @return String, or nullptr if the region value is invalid.
 */
const char *GameCubeRegions::gcnRegionToString(unsigned int gcnRegion, char idRegion, bool *pIsDefault)
{
	/**
	 * There are two region codes for GCN/Wii games:
	 * - BI2.bin (GCN) or Age Rating (Wii)
	 * - Game ID
	 *
	 * The BI2.bin code is what's actually enforced.
	 * The Game ID may provide additional information.
	 *
	 * For games where the BI2.bin code matches the
	 * game ID region, only the BI2.bin region will
	 * be displayed. For others, if the game ID region
	 * is known, it will be printed as text, and the
	 * BI2.bin region will be abbreviated.
	 *
	 * Game ID reference:
	 * - https://github.com/dolphin-emu/dolphin/blob/4c9c4568460df91a38d40ac3071d7646230a8d0f/Source/Core/DiscIO/Enums.cpp
	 */

	switch (gcnRegion) {
		case GCN_REGION_JPN:
			if (pIsDefault) {
				*pIsDefault = false;
			}
			switch (idRegion) {
				case 'J':	// Japan
				default:
					if (pIsDefault) {
						*pIsDefault = true;
					}
					return C_("Region", "Japan");

				case 'W':	// Taiwan
					return C_("Region", "Taiwan");
				case 'K':	// South Korea
				case 'T':	// South Korea with Japanese language
				case 'Q':	// South Korea with English language
					// FIXME: Is this combination possible?
					return C_("Region", "South Korea");
				case 'C':	// China (unofficial?)
					return C_("Region", "China");
			}

		case GCN_REGION_EUR:
			if (pIsDefault) {
				*pIsDefault = false;
			}
			switch (idRegion) {
				case 'P':	// PAL
				case 'X':	// Multi-language release
				case 'Y':	// Multi-language release
				case 'L':	// Japanese import to PAL regions
				case 'M':	// Japanese import to PAL regions
				default:
					if (pIsDefault) {
						*pIsDefault = true;
					}
					return C_("Region", "Europe / Australia");

				case 'D':	// Germany
					return C_("Region", "Germany");
				case 'F':	// France
					return C_("Region", "France");
				case 'H':	// Netherlands
					return C_("Region", "Netherlands");
				case 'I':	// Italy
					return C_("Region", "Italy");
				case 'R':	// Russia
					return C_("Region", "Russia");
				case 'S':	// Spain
					return C_("Region", "Spain");
				case 'U':	// Australia
					return C_("Region", "Australia");
			}

		// USA and South Korea regions don't have separate
		// subregions for other countries.
		case GCN_REGION_USA:
			// Possible game ID regions:
			// - E: USA
			// - N: Japanese import to USA and other NTSC regions.
			// - Z: Prince of Persia - The Forgotten Sands (Wii)
			// - B: Ufouria: The Saga (Virtual Console)
			if (pIsDefault) {
				*pIsDefault = true;
			}
			return C_("Region", "USA");

		case GCN_REGION_KOR:
			// Possible game ID regions:
			// - K: South Korea
			// - Q: South Korea with Japanese language
			// - T: South Korea with English language
			if (pIsDefault) {
				*pIsDefault = true;
			}
			return C_("Region", "South Korea");

		// Other.
		case GCN_REGION_ALL:
			// Region-Free.
			if (pIsDefault) {
				*pIsDefault = true;
			}
			return C_("Region", "Region-Free");

		default:
			break;
	}

	if (pIsDefault) {
		*pIsDefault = true;
	}
	return nullptr;
}

/**
 * Convert a GCN region value (from GCN_Boot_Info or RVL_RegionSetting) to an abbreviation string.
 * Abbreviation string is e.g. "JPN" or "USA".
 * @param gcnRegion	[in] GCN region value.
 * @return Abbreviation string, or nullptr if the region value is invalid.
 */
const char *GameCubeRegions::gcnRegionToAbbrevString(unsigned int gcnRegion)
{
	const char *suffix;
	switch (gcnRegion) {
		case GCN_REGION_JPN:
			suffix = "JPN";
			break;
		case GCN_REGION_USA:
			suffix = "USA";
			break;
		case GCN_REGION_EUR:
			suffix = "EUR";
			break;
		case GCN_REGION_KOR:
			suffix = "KOR";
			break;
		default:
			suffix = nullptr;
			break;
	}
	return suffix;
}

/**
 * Convert a GCN region value (from GCN_Boot_Info or RVL_RegionSetting) to a GameTDB region code.
 * @param gcnRegion GCN region value.
 * @param idRegion Game ID region.
 *
 * NOTE: Mulitple GameTDB region codes may be returned, including:
 * - User-specified fallback region. [TODO]
 * - General fallback region.
 *
 * @return GameTDB region code(s), or empty vector if the region value is invalid.
 */
vector<const char*> GameCubeRegions::gcnRegionToGameTDB(unsigned int gcnRegion, char idRegion)
{
	/**
	 * There are two region codes for GCN/Wii games:
	 * - BI2.bin (GCN) or Age Rating (Wii)
	 * - Game ID
	 *
	 * The BI2.bin code is what's actually enforced.
	 * The Game ID may provide additional information.
	 *
	 * For games where the BI2.bin code matches the
	 * game ID region, only the BI2.bin region will
	 * be displayed. For others, if the game ID region
	 * is known, it will be printed as text, and the
	 * BI2.bin region will be abbreviated.
	 *
	 * Game ID reference:
	 * - https://github.com/dolphin-emu/dolphin/blob/4c9c4568460df91a38d40ac3071d7646230a8d0f/Source/Core/DiscIO/Enums.cpp
	 */
	vector<const char*> ret;

	switch (gcnRegion) {
		case GCN_REGION_JPN:
			switch (idRegion) {
				case 'J':
					break;

				case 'W':	// Taiwan
					ret.push_back("ZHTW");
					break;
				case 'K':
				case 'T':	// South Korea with Japanese language
				case 'Q':	// South Korea with English language
					ret.push_back("KO");
					break;
				case 'C':	// China (unofficial?)
					ret.push_back("ZHCN");
					break;

				// Wrong region, but handle it anyway.
				case 'E':	// USA
					ret.push_back("US");
					break;
				case 'P':	// Europe (PAL)
				default:	// All others
					ret.push_back("EN");
					break;
			}
			ret.push_back("JA");
			break;

		case GCN_REGION_EUR:
			switch (idRegion) {
				case 'P':	// PAL
				case 'X':	// Multi-language release
				case 'Y':	// Multi-language release
				case 'L':	// Japanese import to PAL regions
				case 'M':	// Japanese import to PAL regions
				default:
					break;

				// NOTE: No GameID code for PT.
				// TODO: Implement user-specified fallbacks.

				case 'D':	// Germany
					ret.push_back("DE");
					break;
				case 'F':	// France
					ret.push_back("FR");
					break;
				case 'H':	// Netherlands
					ret.push_back("NL");
					break;
				case 'I':	// Italy
					ret.push_back("NL");
					break;
				case 'R':	// Russia
					ret.push_back("RU");
					break;
				case 'S':	// Spain
					ret.push_back("ES");
					break;
				case 'U':	// Australia
					ret.push_back("AU");
					break;

				// Wrong region, but handle it anyway.
				case 'E':	// USA
					ret.push_back("US");
					break;
				case 'J':	// Japan
					ret.push_back("JA");
					break;
			}
			ret.push_back("EN");
			break;

		// USA and South Korea regions don't have separate
		// subregions for other countries.
		case GCN_REGION_USA:
			// Possible game ID regions:
			// - E: USA
			// - N: Japanese import to USA and other NTSC regions.
			// - Z: Prince of Persia - The Forgotten Sands (Wii)
			// - B: Ufouria: The Saga (Virtual Console)
			switch (idRegion) {
				case 'E':
				default:
					break;

				// Wrong region, but handle it anyway.
				case 'P':	// Europe (PAL)
					ret.push_back("EN");
					break;
				case 'J':	// Japan
					ret.push_back("JA");
					break;
			}
			ret.push_back("US");
			break;

		case GCN_REGION_KOR:
			// Possible game ID regions:
			// - K: South Korea
			// - Q: South Korea with Japanese language
			// - T: South Korea with English language
			ret.push_back("KO");
			break;

		case GCN_REGION_ALL:
		default:
			// Invalid gcnRegion. Use the game ID by itself.
			// (Usually happens if we can't read BI2.bin,
			// e.g. in WIA images.)
			bool addEN = false;
			switch (idRegion) {
				case 'E':	// USA
					ret.push_back("US");
					break;
				case 'P':	// Europe (PAL)
					addEN = true;
					break;
				case 'J':	// Japan
					ret.push_back("JA");
					break;
				case 'W':	// Taiwan
					ret.push_back("ZHTW");
					break;
				case 'K':	// South Korea
				case 'T':	// South Korea with Japanese language
				case 'Q':	// South Korea with English language
					ret.push_back("KO");
					break;
				case 'C':	// China (unofficial?)
					ret.push_back("ZHCN");
					break;

				/** PAL regions **/
				case 'D':	// Germany
					ret.push_back("DE");
					addEN = true;
					break;
				case 'F':	// France
					ret.push_back("FR");
					addEN = true;
					break;
				case 'H':	// Netherlands
					ret.push_back("NL");
					addEN = true;
					break;
				case 'I':	// Italy
					ret.push_back("NL");
					addEN = true;
					break;
				case 'R':	// Russia
					ret.push_back("RU");
					addEN = true;
					break;
				case 'S':	// Spain
					ret.push_back("ES");
					addEN = true;
					break;
				case 'U':	// Australia
					ret.push_back("AU");
					addEN = true;
					break;
			}
			if (addEN) {
				ret.push_back("EN");
			}
			break;
	}

	return ret;
}

}
