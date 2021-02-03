/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiCommon.hpp: Nintendo Wii common functions.                           *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_WIICOMMON_HPP__
#define __ROMPROPERTIES_LIBROMDATA_WIICOMMON_HPP__

#include "librpbase/RomData.hpp"
#include "librpbase/RomFields.hpp"

#include "wii_banner.h"

// C includes.
#include <stdint.h>

namespace LibRomData {

class WiiCommon
{
	private:
		WiiCommon() { }
		~WiiCommon() { }
	private:
		RP_DISABLE_COPY(WiiCommon)

	public:
		/**
		 * Get a multi-language string map from a Wii banner.
		 * @param pImet		[in] Wii_IMET_t
		 * @param gcnRegion	[in] GameCube region code.
		 * @param id4_region	[in] ID4 region.
		 * @return Allocated RomFields::StringMultiMap_t, or nullptr on error.
		 */
		static LibRpBase::RomFields::StringMultiMap_t *getWiiBannerStrings(
			const Wii_IMET_t *pImet, uint32_t gcnRegion, char id4_region);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_WIICOMMON_HPP__ */
