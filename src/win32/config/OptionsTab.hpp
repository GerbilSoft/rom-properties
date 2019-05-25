/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * OptionsTab.hpp: Options tab for rp-config.                              *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_WIN32_CONFIG_OPTIONSTAB_HPP__
#define __ROMPROPERTIES_WIN32_CONFIG_OPTIONSTAB_HPP__

#include "ITab.hpp"

class OptionsTabPrivate;
class OptionsTab : public ITab
{
	public:
		OptionsTab();
		virtual ~OptionsTab();

	private:
		typedef ITab super;
		RP_DISABLE_COPY(OptionsTab)
	private:
		friend class OptionsTabPrivate;
		OptionsTabPrivate *const d_ptr;

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

#endif /* __ROMPROPERTIES_WIN32_CONFIG_OPTIONSTAB_HPP__ */
