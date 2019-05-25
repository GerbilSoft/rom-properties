/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * AboutTab.hpp: About tab for rp-config.                                  *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_WIN32_CONFIG_ABOUTTAB_HPP__
#define __ROMPROPERTIES_WIN32_CONFIG_ABOUTTAB_HPP__

#include "ITab.hpp"

class AboutTabPrivate;
class AboutTab : public ITab
{
	public:
		AboutTab();
		virtual ~AboutTab();

	private:
		typedef ITab super;
		RP_DISABLE_COPY(AboutTab)
	private:
		friend class AboutTabPrivate;
		AboutTabPrivate *const d_ptr;

	public:
		/**
		 * Create the HPROPSHEETPAGE for this tab.
		 *
		 * NOTE: This function can only be called once.
		 * Subsequent invocations will return nullptr.
		 *
		 * @return HPROPSHEETPAGE.
		 */
		HPROPSHEETPAGE getHPropSheetPage(void) final;

		/**
		 * Reset the contents of this tab.
		 */
		void reset(void) final;

		/**
		 * Load the default configuration.
		 * This does NOT save, and will only emit modified()
		 * if it's different from the current configuration.
		 */
		void loadDefaults(void) final;

		/**
		 * Save the contents of this tab.
		 */
		void save(void) final;
};

#endif /* __ROMPROPERTIES_WIN32_CONFIG_ABOUTTAB_HPP__ */
