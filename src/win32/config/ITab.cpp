/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * ITab.cpp: Property sheet base class for rp-config.                      *
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

#include "stdafx.h"
#include "ITab.hpp"

#include "librpbase/SystemRegion.hpp"
using LibRpBase::SystemRegion;

/**
 * Load a dialog resource using the current i18n settings.
 * @param dwResId Dialog resource ID.
 * @return Pointer to dialog resource, or nullptr if not found.
 */
LPCDLGTEMPLATE ITab::LoadDialog_i18n(DWORD dwResId)
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
		hRsrc = FindResourceEx(HINST_THISCOMPONENT, RT_DIALOG,
				MAKEINTRESOURCE(dwResId),
				wLanguage);
		if (!hRsrc && wLanguageFallback != 0) {
			// Try the fallback language.
			hRsrc = FindResourceEx(HINST_THISCOMPONENT, RT_DIALOG,
					MAKEINTRESOURCE(dwResId),
					wLanguageFallback);
		}
	}

	if (!hRsrc) {
		// Try en_US.
		hRsrc = FindResourceEx(HINST_THISCOMPONENT, RT_DIALOG,
				MAKEINTRESOURCE(dwResId),
				MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
	}

	if (!hRsrc) {
		// Try without specifying a language ID.
		hRsrc = FindResourceEx(HINST_THISCOMPONENT, RT_DIALOG,
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

	return static_cast<LPCDLGTEMPLATE>(LockResource(hGlobal));
}
