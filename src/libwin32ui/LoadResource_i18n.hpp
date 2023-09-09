/***************************************************************************
 * ROM Properties Page shell extension. (libwin32ui)                       *
 * LoadResource_i18n.hpp: LoadResource() for the specified locale.         *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "libwin32common/RpWin32_sdk.h"

namespace LibWin32UI {

/**
 * Load a resource using the current i18n settings.
 * @param hModule Module handle
 * @param lpType Resource type
 * @param dwResId Resource ID
 * @return Pointer to resource, or nullptr if not found.
 */
LPVOID LoadResource_i18n(HMODULE hModule, LPCTSTR lpType, DWORD dwResId);

/**
 * Load a dialog resource using the current i18n settings.
 * @param hModule Module handle
 * @param dwResId Dialog resource ID
 * @return Pointer to dialog resource, or nullptr if not found.
 */
static inline LPCDLGTEMPLATE LoadDialog_i18n(HMODULE hModule, DWORD dwResId)
{
	return reinterpret_cast<LPCDLGTEMPLATE>(LoadResource_i18n(hModule, RT_DIALOG, dwResId));
}

/**
 * Load a menu resource using the current i18n settings.
 * @param hModule Module handle
 * @param dwResId Menu resource ID
 * @return HMENU created from the menu resource, or nullptr if not found.
 */
static inline HMENU LoadMenu_i18n(HMODULE hModule, DWORD dwResId)
{
	const MENUTEMPLATE *lpcMenuTemplate =
		reinterpret_cast<const MENUTEMPLATE*>(LoadResource_i18n(hModule, RT_MENU, dwResId));
	return (lpcMenuTemplate ? LoadMenuIndirect(lpcMenuTemplate) : nullptr);
}

}
