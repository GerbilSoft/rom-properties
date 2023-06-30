/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * ITab.hpp: Property sheet base class for rp-config.                      *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"
#include "libwin32common/RpWin32_sdk.h"

class ITab
{
	protected:
		explicit ITab() = default;
	public:
		virtual ~ITab() = 0;

	private:
		RP_DISABLE_COPY(ITab)

	protected:
		/**
		 * Load a resource using the current i18n settings.
		 * @param lpType Resource type.
		 * @param dwResId Resource ID.
		 * @return Pointer to resource, or nullptr if not found.
		 */
		static LPVOID LoadResource_i18n(LPCTSTR lpType, DWORD dwResId);

		/**
		 * Load a dialog resource using the current i18n settings.
		 * @param dwResId Dialog resource ID.
		 * @return Pointer to dialog resource, or nullptr if not found.
		 */
		static inline LPCDLGTEMPLATE LoadDialog_i18n(DWORD dwResId)
		{
			return reinterpret_cast<LPCDLGTEMPLATE>(LoadResource_i18n(RT_DIALOG, dwResId));
		}

		/**
		 * Load a menu resource using the current i18n settings.
		 * @param dwResId Menu resource ID.
		 * @return HMENU created from the menu resource, or nullptr if not found.
		 */
		static inline HMENU LoadMenu_i18n(DWORD dwResId)
		{
			const MENUTEMPLATE *lpcMenuTemplate =
				reinterpret_cast<const MENUTEMPLATE*>(LoadResource_i18n(RT_MENU, dwResId));
			return (lpcMenuTemplate ? LoadMenuIndirect(lpcMenuTemplate) : nullptr);
		}

	public:
		/**
		 * Create the HPROPSHEETPAGE for this tab.
		 *
		 * NOTE: This function can only be called once.
		 * Subsequent invocations will return nullptr.
		 *
		 * @return HPROPSHEETPAGE.
		 */
		virtual HPROPSHEETPAGE getHPropSheetPage(void) = 0;

		/**
		 * Reset the contents of this tab.
		 */
		virtual void reset(void) = 0;

		/**
		 * Load the default configuration.
		 * This does NOT save, and will only emit modified()
		 * if it's different from the current configuration.
		 */
		virtual void loadDefaults(void) { }

		/**
		 * Save the contents of this tab.
		 */
		virtual void save(void) = 0;
};

/**
 * Both gcc and MSVC fail to compile unless we provide
 * an empty implementation, even though the function is
 * declared as pure-virtual.
 */
inline ITab::~ITab() { }
