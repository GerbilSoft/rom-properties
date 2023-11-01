/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GameCubeRegions.hpp: Nintendo GameCube/Wii region code detection.       *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"

// C++ includes.
#include <vector>

namespace LibRomData {

class GameCubeRegions
{
public:
	// Static class
	GameCubeRegions() = delete;
	~GameCubeRegions() = delete;
private:
	RP_DISABLE_COPY(GameCubeRegions)

public:
	/**
	 * Convert a GCN region value (from GCN_Boot_Info or RVL_RegionSetting) to a string.
	 * @param gcnRegion	[in] GCN region value.
	 * @param idRegion	[in] Game ID region.
	 * @param pIsDefault	[out,opt] Set to True if the region string represents the default region for the ID4.
	 * @return String, or nullptr if the region value is invalid.
	 */
	static const char *gcnRegionToString(unsigned int gcnRegion, char idRegion, bool *pIsDefault = nullptr);

	/**
	 * Convert a GCN region value (from GCN_Boot_Info or RVL_RegionSetting) to an abbreviation string.
	 * Abbreviation string is e.g. "JPN" or "USA".
	 * @param gcnRegion	[in] GCN region value.
	 * @return Abbreviation string, or nullptr if the region value is invalid.
	 */
	static const char *gcnRegionToAbbrevString(unsigned int gcnRegion);

	/**
	 * Convert a GCN region value (from GCN_Boot_Info or RVL_RegionSetting) to a GameTDB language code.
	 * @param gcnRegion GCN region value.
	 * @param idRegion Game ID region.
	 *
	 * NOTE: Mulitple GameTDB language codes may be returned, including:
	 * - User-specified fallback language code for PAL.
	 * - General fallback language code.
	 *
	 * @return GameTDB language code(s), or empty vector if the region value is invalid.
	 * NOTE: The language code may need to be converted to uppercase!
	 */
	 static std::vector<uint16_t> gcnRegionToGameTDB(unsigned int gcnRegion, char idRegion);
};

}
