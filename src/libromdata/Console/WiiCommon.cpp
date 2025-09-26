/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiCommon.cpp: Nintendo Wii common functions.                           *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "WiiCommon.hpp"

#include "data/NintendoLanguage.hpp"
#include "gcn_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpText;

// C++ STL classes
using std::array;
using std::string;

namespace LibRomData { namespace WiiCommon {

/**
 * Get a multi-language string map from a Wii banner.
 * @param pImet		[in] Wii_IMET_t
 * @param gcnRegion	[in] GameCube region code
 * @param id4_region	[in] ID4 region
 * @return Allocated RomFields::StringMultiMap_t, or nullptr on error.
 */
RomFields::StringMultiMap_t *getWiiBannerStrings(
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
		if (pImet->names[langID][0][0] == '\0' &&
		    pImet->names[langID][1][0] == '\0')
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
		if (langID == WII_LANG_JAPANESE && gcnRegion == GCN_REGION_JPN && id4_region == 'W') {
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

		if (pImet->names[langID][0][0] != '\0') {
			// NOTE: The banner may have two lines.
			// Each line is a maximum of 21 characters.
			// Convert from UTF-16 BE and split into two lines at the same time.
			string info = utf16be_to_utf8(pImet->names[langID][0], ARRAY_SIZE_I(pImet->names[langID][0]));
			if (pImet->names[langID][1][0] != '\0') {
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

/**
 * Get a single string from a Wii banner that most closely matches the system language.
 * @param pImet		[in] Wii_IMET_t
 * @param gcnRegion	[in] GameCube region code
 * @param id4_region	[in] ID4 region
 * @return String, or empty string on error.
 */
string getWiiBannerStringForSysLC(
	const Wii_IMET_t *pImet, uint32_t gcnRegion, char id4_region)
{
	assert(pImet != nullptr);
	if (!pImet) {
		return {};
	}

	// Validate the IMET magic number.
	if (pImet->magic != cpu_to_be32(WII_IMET_MAGIC)) {
		// Not valid.
		return {};
	}

	// Determine the system's Nintendo language code.
	int langID = NintendoLanguage::getWiiLanguage();

	if (pImet->names[langID][0][0] == '\0' &&
	    pImet->names[langID][1][0] == '\0')
	{
		// Empty strings. Try English.
		langID = WII_LANG_ENGLISH;
		if (pImet->names[langID][0][0] == '\0' &&
		    pImet->names[langID][1][0] == '\0')
		{
			// Empty strings. Try Japanese.
			langID = WII_LANG_JAPANESE;
			if (pImet->names[langID][0][0] == '\0' &&
			    pImet->names[langID][1][0] == '\0')
			{
				// Empty strings. Can't do anything else...
				// TODO: Try all languages?
				return {};
			}
		}
	}

	// NOTE: The banner may have two lines.
	// Each line is a maximum of 21 characters.
	// Convert from UTF-16 BE and split into two lines at the same time.
	string info;
	if (pImet->names[langID][0][0] != '\0') {
		info = utf16be_to_utf8(pImet->names[langID][0], ARRAY_SIZE_I(pImet->names[langID][0]));
		if (pImet->names[langID][1][0] != cpu_to_be16('\0')) {
			info += '\n';
			info += utf16be_to_utf8(pImet->names[langID][1], ARRAY_SIZE_I(pImet->names[langID][1]));
		}
	}

	return info;
}

// Region code bitfield names
const array<const char*, 7> dsi_3ds_wiiu_region_bitfield_names = {{
	NOP_C_("Region", "Japan"),
	NOP_C_("Region", "USA"),
	NOP_C_("Region", "Europe"),
	nullptr,	//NOP_C_("Region", "Australia"),	// NOTE: Not actually used?
	NOP_C_("Region", "China"),
	NOP_C_("Region", "South Korea"),
	NOP_C_("Region", "Taiwan"),
}};

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
string getRegionCodeForMetadataProperty(uint32_t region_code, bool showRegionT)
{
	unsigned int region_count;

	// "Australia" region (bit 3) is present, but skipped with formatting.
	if (showRegionT) {
		region_count = 7;
		region_code &= 0x7F;
	} else {
		region_count = 6;
		region_code &= 0x3F;
	}

	// For multi-region titles, region will be formatted as: "JUECKT"
	// (Australia is ignored...)
	const char *i18n_region = nullptr;
	for (unsigned int i = 0; i < region_count; i++) {
		if (region_code == (1U << i)) {
			i18n_region = dsi_3ds_wiiu_region_bitfield_names[i];
			break;
		}
	}

	// Special-case check for Europe (4) + Australia (8)
	if (region_code == (4 | 8)) {
		i18n_region = dsi_3ds_wiiu_region_bitfield_names[2];
	}

	if (i18n_region) {
		return pgettext_expr("Region", i18n_region);
	}

	// Multi-region
	static const char all_display_regions[] = "JUECKT";
	const unsigned int all_display_regions_count = region_count - 1;

	string s_region_code;
	s_region_code.resize(all_display_regions_count, '-');
	for (unsigned int i = 0; i < region_count; i++) {
		if (i == 3) {
			// Skip Australia.
			continue;
		}

		unsigned int chr_pos = i;
		if (chr_pos >= 4) {
			chr_pos--;
		}

		if (region_code & (1U << i)) {
			s_region_code[chr_pos] = all_display_regions[chr_pos];
		}
	}

	return s_region_code;
}

} }
