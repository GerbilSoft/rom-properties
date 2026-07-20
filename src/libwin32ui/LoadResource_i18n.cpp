/***************************************************************************
 * ROM Properties Page shell extension. (libwin32ui)                       *
 * LoadResource_i18n.hpp: LoadResource() for the specified locale.         *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "LoadResource_i18n.hpp"

#include "librpbase/SystemRegion.hpp"
using namespace LibRpBase;

// C++ includes
#include <array>
using std::array;

namespace LibWin32UI {

// Language code mapping
struct lc_mapping_t {
	uint32_t lc;
	WORD wLanguage;
};

/**
 * bsearch() comparison functoin for lookup_disc_publisher().
 * @param a
 * @param b
 * @return
 */
static int LoadResource_i18n_compar(const void *a, const void *b)
{
       const lc_mapping_t *const pa = static_cast<const lc_mapping_t*>(a);
       const lc_mapping_t *const pb = static_cast<const lc_mapping_t*>(b);
       return (static_cast<int>(pa->lc) - static_cast<int>(pb->lc));
}

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
	const uint32_t cc = SystemRegion::getCountryCode();
	WORD wLanguage = 0;		// Desired language code.
	WORD wLanguageFallback = 0;	// Fallback language.

	// Mappings for languages with only a single variant implemented.
	// Reference: https://learn.microsoft.com/en-us/previous-versions/windows/desktop/indexsrv/valid-locale-identifiers
	static const array<lc_mapping_t, 8> lc_mappings = {{
		{'de', MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN)},
		{'es', MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH)},
		{'fr', MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH)},
		{'it', MAKELANGID(LANG_ITALIAN, SUBLANG_DEFAULT)},
		{'pt', MAKELANGID(LANG_PORTUGUESE, SUBLANG_PORTUGUESE_BRAZILIAN)},
		{'ro', MAKELANGID(LANG_ROMANIAN, SUBLANG_DEFAULT)},
		{'ru', MAKELANGID(LANG_RUSSIAN, SUBLANG_DEFAULT)},
		{'uk', MAKELANGID(LANG_UKRAINIAN, SUBLANG_DEFAULT)},
	}};

	// Mappings for sub-language variants.
	static const array<lc_mapping_t, 2> en_cc_mappings = {{
		// TODO: Map others to en_GB?
		{'GB', MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_UK)},
		{'US', MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)},
	}};

	switch (lc) {
		case 0:
			// No language code...
			break;

		case 'en': {
			// Check for English sub-language codes.
			const lc_mapping_t key = {cc, 0};
			void *ptr = bsearch(&key, en_cc_mappings.data(),
				en_cc_mappings.size(), sizeof(en_cc_mappings[0]),
				LoadResource_i18n_compar);
			if (ptr) {
				const lc_mapping_t *const pMapping = static_cast<const lc_mapping_t*>(ptr);
				wLanguage = pMapping->wLanguage;
			}
			break;
		}

		default: {
			// Search for the specified language code.
			const lc_mapping_t key = {cc, 0};
			void *ptr = bsearch(&key, lc_mappings.data(),
				lc_mappings.size(), sizeof(lc_mappings[0]),
				LoadResource_i18n_compar);
			if (ptr) {
				const lc_mapping_t *const pMapping = static_cast<const lc_mapping_t*>(ptr);
				wLanguage = pMapping->wLanguage;
			}
			break;
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

} // namespace LibWin32UI
