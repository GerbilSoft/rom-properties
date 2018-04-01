/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * AboutTab.hpp: About tab for rp-config.                                  *
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
