/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * KeyManagerTab.hpp: Key Manager tab for rp-config.                       *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_WIN32_CONFIG_KEYMANAGERTAB_HPP__
#define __ROMPROPERTIES_WIN32_CONFIG_KEYMANAGERTAB_HPP__

#include "ITab.hpp"

class KeyManagerTabPrivate;
class KeyManagerTab : public ITab
{
	public:
		KeyManagerTab();
		virtual ~KeyManagerTab();

	private:
		typedef ITab super;
		RP_DISABLE_COPY(KeyManagerTab)
	private:
		friend class KeyManagerTabPrivate;
		KeyManagerTabPrivate *const d_ptr;

	public:
		/**
		 * Create the HPROPSHEETPAGE for this tab.
		 *
		 * NOTE: This function can only be called once.
		 * Subsequent invocations will return nullptr.
		 *
		 * @return HPROPSHEETPAGE.
		 */
		virtual HPROPSHEETPAGE getHPropSheetPage(void) override final;

		/**
		 * Reset the contents of this tab.
		 */
		virtual void reset(void) override final;

		/**
		 * Load the default configuration.
		 * This does NOT save, and will only emit modified()
		 * if it's different from the current configuration.
		 */
		virtual void loadDefaults(void) override final;

		/**
		 * Save the contents of this tab.
		 */
		virtual void save(void) override final;
};

#endif /* __ROMPROPERTIES_WIN32_CONFIG_KEYMANAGERTAB_HPP__ */
