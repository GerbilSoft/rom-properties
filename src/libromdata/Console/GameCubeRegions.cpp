/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GameCubeRegions.hpp: Nintendo GameCube/Wii region code detection.       *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "GameCubeRegions.hpp"
#include "gcn_structs.h"

// C++ STL classes.
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

		case GCN_REGION_CHN:
			// Possible game ID regions:
			// - C: China
			if (pIsDefault) {
				*pIsDefault = true;
			}
			return C_("Region", "China");

		case GCN_REGION_TWN:
			// Possible game ID regions:
			// - W: Taiwan
			if (pIsDefault) {
				*pIsDefault = true;
			}
			return C_("Region", "Taiwan");

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
	static const char region_tbl[5][4] = {
		"JPN", "USA", "EUR", "ALL", "KOR"
	};
	if (gcnRegion >= ARRAY_SIZE(region_tbl)) {
		return nullptr;
	}
	return region_tbl[gcnRegion];
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
					// NOTE: GameTDB only has "ZH" for boxart, not "ZHCN" or "ZHTW".
					ret.emplace_back("ZH");
					break;
				case 'K':
				case 'T':	// South Korea with Japanese language
				case 'Q':	// South Korea with English language
					ret.emplace_back("KO");
					break;
				case 'C':	// China (unofficial?)
					// NOTE: GameTDB only has "ZH" for boxart, not "ZHCN" or "ZHTW".
					ret.emplace_back("ZH");
					break;

				// Wrong region, but handle it anyway.
				case 'E':	// USA
					ret.emplace_back("US");
					break;
				case 'P':	// Europe (PAL)
				default:	// All others
					ret.emplace_back("EN");
					break;
			}
			ret.emplace_back("JA");
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
					ret.emplace_back("DE");
					break;
				case 'F':	// France
					ret.emplace_back("FR");
					break;
				case 'H':	// Netherlands
					ret.emplace_back("NL");
					break;
				case 'I':	// Italy
					ret.emplace_back("IT");
					break;
				case 'R':	// Russia
					ret.emplace_back("RU");
					break;
				case 'S':	// Spain
					ret.emplace_back("ES");
					break;
				case 'U':	// Australia
					ret.emplace_back("AU");
					break;

				// Wrong region, but handle it anyway.
				case 'E':	// USA
					ret.emplace_back("US");
					break;
				case 'J':	// Japan
					ret.emplace_back("JA");
					break;
			}
			ret.emplace_back("EN");
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
					ret.emplace_back("EN");
					break;
				case 'J':	// Japan
					ret.emplace_back("JA");
					break;
			}
			ret.emplace_back("US");
			break;

		case GCN_REGION_KOR:
			// Possible game ID regions:
			// - K: South Korea
			// - Q: South Korea with Japanese language
			// - T: South Korea with English language
			ret.emplace_back("KO");
			break;

		case GCN_REGION_CHN:
			// Possible game ID regions:
			// - C: China
			// NOTE: GameTDB only has "ZH" for boxart, not "ZHCN" or "ZHTW".
			ret.emplace_back("ZH");
			break;

		case GCN_REGION_TWN:
			// Possible game ID regions:
			// - W: Taiwan
			// NOTE: GameTDB only has "ZH" for boxart, not "ZHCN" or "ZHTW".
			ret.emplace_back("ZH");
			break;

		case GCN_REGION_ALL:
		default:
			// Invalid gcnRegion. Use the game ID by itself.
			// (Usually happens if we can't read BI2.bin,
			// e.g. in WIA images.)
			bool addEN = false;
			switch (idRegion) {
				case 'E':	// USA
					ret.emplace_back("US");
					break;
				case 'P':	// Europe (PAL)
					addEN = true;
					break;
				case 'J':	// Japan
					ret.emplace_back("JA");
					break;
				case 'W':	// Taiwan
					// NOTE: GameTDB only has "ZH" for boxart, not "ZHCN" or "ZHTW".
					ret.emplace_back("ZH");
					break;
				case 'K':	// South Korea
				case 'T':	// South Korea with Japanese language
				case 'Q':	// South Korea with English language
					ret.emplace_back("KO");
					break;
				case 'C':	// China (unofficial?)
					// NOTE: GameTDB only has "ZH" for boxart, not "ZHCN" or "ZHTW".
					ret.emplace_back("ZH");
					break;

				/** PAL regions **/
				case 'D':	// Germany
					ret.emplace_back("DE");
					addEN = true;
					break;
				case 'F':	// France
					ret.emplace_back("FR");
					addEN = true;
					break;
				case 'H':	// Netherlands
					ret.emplace_back("NL");
					addEN = true;
					break;
				case 'I':	// Italy
					ret.emplace_back("IT");
					addEN = true;
					break;
				case 'R':	// Russia
					ret.emplace_back("RU");
					addEN = true;
					break;
				case 'S':	// Spain
					ret.emplace_back("ES");
					addEN = true;
					break;
				case 'U':	// Australia
					ret.emplace_back("AU");
					addEN = true;
					break;
			}
			if (addEN) {
				ret.emplace_back("EN");
			}
			break;
	}

	return ret;
}

}
