/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * ITab.cpp: Property sheet base class for rp-config.                      *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ITab.hpp"

#include "librpbase/SystemRegion.hpp"
using LibRpBase::SystemRegion;

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

	switch (lc) {
		case 'pt':
			// Default is pt_BR.
			wLanguage = MAKELANGID(LANG_PORTUGUESE, SUBLANG_PORTUGUESE_BRAZILIAN);
			break;

		case 'ru':
			// Default is ru_RU.
			wLanguage = MAKELANGID(LANG_RUSSIAN, SUBLANG_RUSSIAN_RUSSIA);
			break;

		case 'uk':
			// Default is uk_UA.
			wLanguage = MAKELANGID(LANG_UKRAINIAN, SUBLANG_UKRAINIAN_UKRAINE);
			break;

		case 'en':
		default:
			// Default is en_US.
			// TODO: Add other en locales.
			break;
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
