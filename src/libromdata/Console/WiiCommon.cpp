/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiCommon.cpp: Nintendo Wii common functions.                           *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include <stdafx.h>
#include "WiiCommon.hpp"

#include "data/NintendoLanguage.hpp"
#include "gcn_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpText;

// C++ STL classes
using std::string;

namespace LibRomData {

/**
 * Get a multi-language string map from a Wii banner.
 * @param pImet		[in] Wii_IMET_t
 * @param gcnRegion	[in] GameCube region code.
 * @param id4_region	[in] ID4 region.
 * @return Allocated RomFields::StringMultiMap_t, or nullptr on error.
 */
RomFields::StringMultiMap_t *WiiCommon::getWiiBannerStrings(
	const Wii_IMET_t *pImet, uint32_t gcnRegion, char id4_region)
{
	assert(pImet != nullptr);
	if (!pImet) {
		return nullptr;
	}

	// Validate the IMET magic number.
	if (pImet->magic != cpu_to_be32(WII_IMET_MAGIC)) {
		// Not valid.
		return nullptr;
	}

	// Check if English is valid.
	// If it is, we'll de-duplicate fields.
	const bool dedupe_titles = (pImet->names[WII_LANG_ENGLISH][0][0] != cpu_to_be16('\0'));

	RomFields::StringMultiMap_t *const pMap_bannerName = new RomFields::StringMultiMap_t();
	for (int langID = 0; langID < WII_LANG_MAX; langID++) {
		if (langID == 7 || langID == 8) {
			// Unknown languages. Skip them. (Maybe these were Chinese?)
			// TODO: Just skip if NintendoLanguage::getWiiLanguageCode() returns 0?
			continue;
		}

		// Check for empty strings first.
		if (pImet->names[langID][0][0] == 0 &&
		    pImet->names[langID][1][0] == 0)
		{
			// Strings are empty.
			continue;
		}

		if (dedupe_titles && langID != WII_LANG_ENGLISH) {
			// Check if the comments match English.
			// NOTE: Not converting to host-endian first, since
			// u16_strncmp() checks for equality and for 0.
			if (!u16_strncmp(pImet->names[langID][0], pImet->names[WII_LANG_ENGLISH][0],
			                 ARRAY_SIZE(pImet->names[WII_LANG_ENGLISH][0])) &&
			    !u16_strncmp(pImet->names[langID][1], pImet->names[WII_LANG_ENGLISH][1],
			                 ARRAY_SIZE(pImet->names[WII_LANG_ENGLISH][1])))
			{
				// All fields match English.
				continue;
			}
		}

		uint32_t lc;
		if (gcnRegion == GCN_REGION_JPN && id4_region == 'W' && langID == WII_LANG_JAPANESE) {
			// Special case: RVL-001(TWN) has a JPN region code.
			// Game discs with disc ID region 'W' are localized
			// for Taiwan and use Traditional Chinese in the
			// Japanese language slot.
			lc = 'hant';
		} else {
			lc = NintendoLanguage::getWiiLanguageCode(langID);
			assert(lc != 0);
			if (lc == 0)
				continue;
		}

		if (pImet->names[langID][0][0] != cpu_to_be16('\0')) {
			// NOTE: The banner may have two lines.
			// Each line is a maximum of 21 characters.
			// Convert from UTF-16 BE and split into two lines at the same time.
			string info = utf16be_to_utf8(pImet->names[langID][0], ARRAY_SIZE_I(pImet->names[langID][0]));
			if (pImet->names[langID][1][0] != cpu_to_be16('\0')) {
				info += '\n';
				info += utf16be_to_utf8(pImet->names[langID][1], ARRAY_SIZE_I(pImet->names[langID][1]));
			}

			pMap_bannerName->emplace(lc, std::move(info));
		}
	}

	if (pMap_bannerName->empty()) {
		// No strings...
		delete pMap_bannerName;
		return nullptr;
	}

	// Map is done.
	return pMap_bannerName;
}

}
