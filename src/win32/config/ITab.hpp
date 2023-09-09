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

class NOVTABLE ITab
{
	protected:
		explicit ITab() = default;
	public:
		virtual ~ITab() = 0;

	private:
		RP_DISABLE_COPY(ITab)

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
