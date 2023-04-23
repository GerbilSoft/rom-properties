/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * ITab.cpp: Property sheet base class for rp-config.                      *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ITab.hpp"

#include "librpbase/SystemRegion.hpp"
using namespace LibRpBase;

/**
 * Load a resource using the current i18n settings.
 * @param lpType Resource type.
 * @param dwResId Resource ID.
 * @return Pointer to resource, or nullptr if not found.
 */
LPVOID ITab::LoadResource_i18n(LPCTSTR lpType, DWORD dwResId)
{
	// NOTE: This function should be updated whenever
	// a new translation is added.

	HRSRC hRsrc = nullptr;
	HGLOBAL hGlobal;

	const uint32_t lc = SystemRegion::getLanguageCode();
	//const uint32_t cc = SystemRegion::getCountryCode();
	WORD wLanguage = 0;		// Desired language code.
	WORD wLanguageFallback = 0;	// Fallback language.

	// Mappings for languages with only a single variant implemented.
	struct lc_mapping_t {
		uint32_t lc;
		WORD wLanguage;
	};
	static const struct lc_mapping_t lc_mappings[] = {
		{'de', MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN)},
		{'es', MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH)},
		{'fr', MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH)},
		{'pt', MAKELANGID(LANG_PORTUGUESE, SUBLANG_PORTUGUESE_BRAZILIAN)},
		{'ru', MAKELANGID(LANG_RUSSIAN, SUBLANG_RUSSIAN_RUSSIA)},
		{'uk', MAKELANGID(LANG_UKRAINIAN, SUBLANG_UKRAINIAN_UKRAINE)},
	};
	static const struct lc_mapping_t *const p_lc_mappings_end = &lc_mappings[ARRAY_SIZE(lc_mappings)];

	auto iter = std::find_if(lc_mappings, p_lc_mappings_end,
		[lc](const struct lc_mapping_t lc_mapping) {
			return (lc == lc_mapping.lc);
		});
	if (iter != p_lc_mappings_end) {
		wLanguage = iter->wLanguage;
	}

	// Search for the requested language.
	if (wLanguage != 0) {
		hRsrc = FindResourceEx(HINST_THISCOMPONENT, lpType,
				MAKEINTRESOURCE(dwResId),
				wLanguage);
		if (!hRsrc && wLanguageFallback != 0) {
			// Try the fallback language.
			hRsrc = FindResourceEx(HINST_THISCOMPONENT, lpType,
					MAKEINTRESOURCE(dwResId),
					wLanguageFallback);
		}
	}

	if (!hRsrc) {
		// Try en_US.
		hRsrc = FindResourceEx(HINST_THISCOMPONENT, lpType,
				MAKEINTRESOURCE(dwResId),
				MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
	}

	if (!hRsrc) {
		// Try without specifying a language ID.
		hRsrc = FindResourceEx(HINST_THISCOMPONENT, lpType,
				MAKEINTRESOURCE(dwResId),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
	}

	if (!hRsrc) {
		// Resource not found.
		return nullptr;
	}

	// Load and "lock" the resource.
	// NOTE: Resource locking doesn't actually lock anything,
	// so we don't have to unlock or free the resource later.
	// (Win16 legacy functionality.)
	hGlobal = LoadResource(HINST_THISCOMPONENT, hRsrc);
	if (!hGlobal) {
		// Unable to load the resource.
		return nullptr;
	}

	return LockResource(hGlobal);
}
