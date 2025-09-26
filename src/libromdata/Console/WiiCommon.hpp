/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiCommon.hpp: Nintendo Wii common functions.                           *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/RomFields.hpp"
#include "wii_banner.h"

// C includes (C++ namespace)
#include <cstdint>

// C++ STL classes
#include <array>
#include <string>

namespace LibRomData { namespace WiiCommon {

/**
 * Get a multi-language string map from a Wii banner.
 * @param pImet		[in] Wii_IMET_t
 * @param gcnRegion	[in] GameCube region code.
 * @param id4_region	[in] ID4 region.
 * @return Allocated RomFields::StringMultiMap_t, or nullptr on error.
 */
LibRpBase::RomFields::StringMultiMap_t *getWiiBannerStrings(
	const Wii_IMET_t *pImet, uint32_t gcnRegion, char id4_region);

/**
 * Get a single string from a Wii banner that most closely matches the system language.
 * @param pImet		[in] Wii_IMET_t
 * @param gcnRegion	[in] GameCube region code
 * @param id4_region	[in] ID4 region
 * @return String, or empty string on error.
 */
std::string getWiiBannerStringForSysLC(
	const Wii_IMET_t *pImet, uint32_t gcnRegion, char id4_region);

// Region code bitfield names
extern const std::array<const char*, 7> dsi_3ds_wiiu_region_bitfield_names;

/**
 * Format a DSi/3DS/Wii U region code for display as a metadata property.
 *
 * If a single bit is set, one region will be shown.
 *
 * If multiple bits are set, it will be shown as "JUECKT", with '-'
 * for bits that are not set.
 *
 * @param region_code Region code
 * @param showRegionT If true, include the 'T' region.
 * @return Region code string
 */
std::string getRegionCodeForMetadataProperty(uint32_t region_code, bool showRegionT);

} }
