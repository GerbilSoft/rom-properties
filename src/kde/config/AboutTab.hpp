/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * AboutTab.hpp: About tab for rp-config.                                  *
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

#ifndef __ROMPROPERTIES_KDE_CONFIG_ABOUTTAB_HPP__
#define __ROMPROPERTIES_KDE_CONFIG_ABOUTTAB_HPP__

#include "ITab.hpp"

class AboutTabPrivate;
class AboutTab : public ITab
{
	Q_OBJECT

	public:
		explicit AboutTab(QWidget *parent = nullptr);
		virtual ~AboutTab();

	private:
		typedef ITab super;
		AboutTabPrivate *const d_ptr;
		Q_DECLARE_PRIVATE(AboutTab);
		Q_DISABLE_COPY(AboutTab)

	public:
		/**
		 * Does this tab have defaults available?
		 * If so, the "Defaults" button will be enabled.
		 * Otherwise, it will be disabled.
		 *
		 * AboutTab sets this to false.
		 *
		 * @return True to enable; false to disable.
		 */
		virtual bool hasDefaults(void) const override final { return false; }

	protected:
		// State change event. (Used for switching the UI language at runtime.)
		virtual void changeEvent(QEvent *event) override final;

	public slots:
		/**
		 * Reset the configuration.
		 */
		virtual void reset(void) override final;

		/**
		 * Load the default configuration.
		 * This does NOT save, and will only emit modified()
		 * if it's different from the current configuration.
		 */
		virtual void loadDefaults(void) override final;

		/**
		 * Save the configuration.
		 * @param pSettings QSettings object.
		 */
		virtual void save(QSettings *pSettings) override final;
};

#endif /* __ROMPROPERTIES_KDE_CONFIG_ABOUTTAB_HPP__ */
