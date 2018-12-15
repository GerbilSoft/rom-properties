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

#ifndef __ROMPROPERTIES_LIBROMDATA_GAMECUBEREGIONS_HPP__
#define __ROMPROPERTIES_LIBROMDATA_GAMECUBEREGIONS_HPP__

#include "librpbase/common.h"

// C++ includes.
#include <vector>

namespace LibRomData {

class GameCubeRegions
{
	private:
		// GameCubeRegions is a static class.
		GameCubeRegions();
		~GameCubeRegions();
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
		static std::vector<const char*> gcnRegionToGameTDB(unsigned int gcnRegion, char idRegion);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_GAMECUBEREGIONS_HPP__ */
