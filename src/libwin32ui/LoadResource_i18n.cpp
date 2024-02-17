/***************************************************************************
 * ROM Properties Page shell extension. (libwin32ui)                       *
 * LoadResource_i18n.hpp: LoadResource() for the specified locale.         *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "LoadResource_i18n.hpp"

#include "librpbase/SystemRegion.hpp"
using namespace LibRpBase;

// C++ includes
#include <algorithm>
#include <array>

namespace LibWin32UI {

/**
 * Load a resource using the current i18n settings.
 * @param hModule Module handle
 * @param lpType Resource type
 * @param dwResId Resource ID
 * @return Pointer to resource, or nullptr if not found.
 */
LPVOID LoadResource_i18n(HMODULE hModule, LPCTSTR lpType, DWORD dwResId)
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
	// Reference: https://learn.microsoft.com/en-us/previous-versions/windows/desktop/indexsrv/valid-locale-identifiers
	struct lc_mapping_t {
		uint32_t lc;
		WORD wLanguage;
	};
	static const std::array<lc_mapping_t, 8> lc_mappings = {{
		{'de', MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN)},
		{'es', MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH)},
		{'fr', MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH)},
		{'it', MAKELANGID(LANG_ITALIAN, SUBLANG_DEFAULT)},
		{'pt', MAKELANGID(LANG_PORTUGUESE, SUBLANG_PORTUGUESE_BRAZILIAN)},
		{'ro', MAKELANGID(LANG_ROMANIAN, SUBLANG_DEFAULT)},
		{'ru', MAKELANGID(LANG_RUSSIAN, SUBLANG_DEFAULT)},
		{'uk', MAKELANGID(LANG_UKRAINIAN, SUBLANG_DEFAULT)},
	}};

	if (lc != 0 && lc != 'en') {
		// Search for the specified language code.
		// NOTE: 'en' is special-cased and skips this search.
		auto iter = std::lower_bound(lc_mappings.cbegin(), lc_mappings.cend(), lc,
			[lc](const lc_mapping_t lc_mapping, uint32_t lc) {
				return (lc_mapping.lc < lc);
			});
		if (iter != lc_mappings.cend() && iter->lc == lc) {
			wLanguage = iter->wLanguage;
		}
	}

	// Search for the requested language.
	if (wLanguage != 0) {
		hRsrc = FindResourceEx(hModule, lpType,
				MAKEINTRESOURCE(dwResId),
				wLanguage);
		if (!hRsrc && wLanguageFallback != 0) {
			// Try the fallback language.
			hRsrc = FindResourceEx(hModule, lpType,
					MAKEINTRESOURCE(dwResId),
					wLanguageFallback);
		}
	}

	if (!hRsrc) {
		// Try en_US.
		hRsrc = FindResourceEx(hModule, lpType,
				MAKEINTRESOURCE(dwResId),
				MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
	}

	if (!hRsrc) {
		// Try without specifying a language ID.
		hRsrc = FindResourceEx(hModule, lpType,
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
	hGlobal = LoadResource(hModule, hRsrc);
	if (!hGlobal) {
		// Unable to load the resource.
		return nullptr;
	}

	return LockResource(hGlobal);
}

} //namespace LibWin32UI
